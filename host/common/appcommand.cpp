#include "appcommand.h"
#include "localconfigurator.h"
#include "hostagenthelpers_ported.h" 
#include <boost/shared_array.hpp>
#include <fstream>
#include "ace/Time_Value.h"
#include <boost/lexical_cast.hpp>
#include "portablehelpers.h"
#include "inmsafeint.h"
#include "inmageex.h"

#define APP_CMD_LOG_LEVEL   ((SV_LOG_LEVEL)m_logLevel)

AppCommand::AppCommand(const std::string& cmd, SV_UINT timeout, std::string outfile, bool inherithandles, bool donotLogCmd)
    :m_cmd(cmd),
    m_timeout(timeout),
    m_outFile(outfile),
    m_bInheritHandles(inherithandles),
    m_bDonotLogCmd(donotLogCmd),
    m_bUnlink(outfile.empty()),
    m_logLevel(SV_LOG_DEBUG)
{
}

AppCommand::AppCommand(const std::string& cmd, const std::string& args, SV_UINT timeout, std::string outfile, bool inherithandles, bool donotLogCmd)
    :m_timeout(timeout),
    m_outFile(outfile),
    m_bInheritHandles(inherithandles),
    m_bDonotLogCmd(donotLogCmd),
    m_bUnlink(outfile.empty()),
    m_logLevel(SV_LOG_DEBUG)
{
    m_cmd =
#ifdef SV_WINDOWS
        "\"" + cmd + "\" " + args;
#else
        cmd + " " + args;
#endif
}

void AppCommand::SetLogLevel(int logLevel)
{
    if (logLevel >= SV_LOG_DISABLE && logLevel <= SV_LOG_ALWAYS)
        m_logLevel = logLevel;
}

void AppCommand::CollectCmdOutPut(std::string& output)
{
    if (m_outFile.empty())
        return;

    SV_ULONG fileSize = 0 ;
    std::ifstream stream(m_outFile.c_str());
    if(stream.good())
    {
        stream.seekg(0, std::ios::end);
        fileSize = stream.tellg();
        stream.close();
        ACE_HANDLE handle = ACE_OS::open( m_outFile.c_str(), O_RDONLY);
        if(handle == ACE_INVALID_HANDLE)
        {
            output = "Failed to open file ";
            output += m_outFile;
            output += " for reading ";
        }
        else
        {
            size_t infolen;
            INM_SAFE_ARITHMETIC(infolen = InmSafeInt<SV_ULONG>::Type(fileSize) + 1, INMAGE_EX(fileSize))
            boost::shared_array<char> info (new (std::nothrow) char[infolen]);
            if( fileSize == ACE_OS::read(handle, info.get(), fileSize) )
            {
                info[ fileSize ] = '\0';
                output = info.get();
            }
            ACE_OS::close( handle );
            if( m_bUnlink )
            {
                ACE_OS::unlink(getLongPathName(m_outFile.c_str()).c_str());
            }
        }
    }
    else
    {
        output = "Failed to open file ";
        output += m_outFile ;
        output += " for reading.";
    }
}


SVERROR AppCommand::Run(SV_ULONG& exitCode, std::string& outPut, bool& bProcessActive, const std::string& workindDir, void* handle)
{
    DebugPrintf(APP_CMD_LOG_LEVEL, "ENTERED %s\n", FUNCTION_NAME);
    SVERROR bRet = SVS_FALSE;
    CProcess* processPtr = NULL;
    if (!m_bDonotLogCmd)
    {
        DebugPrintf(APP_CMD_LOG_LEVEL, "Command : %s\n", m_cmd.c_str());
    }

    std::string cmd = m_cmd;
#ifndef SV_WINDOWS
    LocalConfigurator configurator;
    std::stringstream tmp_script_file;
    tmp_script_file << configurator.getInstallPath() << "/scripts/inm_tmp_script" << std::hex << ACE_OS::thr_self() << ".sh";
    std::ofstream script_out(tmp_script_file.str().c_str());
    if (script_out.good())
    {
        script_out << "#!/bin/sh" << std::endl;
        script_out << "exec " << m_cmd << std::endl;//prepending exec to actual command in temporary script so that   actual command runs as a child process instead of becoming a grand process. 
        script_out.close();
        if (chmod(tmp_script_file.str().c_str(), 0700) != 0)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to set execute permisions to the command script.\n");
            outPut = "Faile to set execute permissions to the command script file";
            exitCode = 1;
            remove(tmp_script_file.str().c_str());
            return SVS_FALSE;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to create the command script  file.\n");
        outPut = "Faile to create the command script file.";
        exitCode = 1;
        return SVS_FALSE;
    }
    cmd = tmp_script_file.str();
    if (SVMakeSureDirectoryPathExists(configurator.getCacheDirectory().c_str()).failed())
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to create path directory %s.\n", configurator.getCacheDirectory().c_str());
    }

    if (!m_outFile.empty())
    {
        ACE_HANDLE logHandle = ACE_OS::open(getLongPathName(m_outFile.c_str()).c_str(), O_CREAT | O_WRONLY | O_APPEND, 0666);
        if (ACE_INVALID_HANDLE == logHandle)
        {
            DebugPrintf(SV_LOG_ERROR, "open %s failed. error no:%d\n", m_outFile.c_str(), ACE_OS::last_error());
        }
        else
        {
            ACE_OS::close(logHandle);
        }
    }
#endif
    if (!m_outFile.empty())
    {
        std::ofstream policylogstream;
        policylogstream.open(m_outFile.c_str(), std::ios::app);
        if (policylogstream.is_open() && !m_bDonotLogCmd)
        {
            policylogstream << m_cmd << std::endl;
            policylogstream.close();
        }
    }

    bRet = CProcess::Create(cmd.c_str(), &processPtr,
                            m_outFile.empty() ? NULL : m_outFile.c_str(),
                            workindDir.empty() ? NULL : workindDir.c_str(),
                            handle, m_bInheritHandles, (SV_LOG_LEVEL)m_logLevel);

    char pszError[ THREAD_SAFE_ERROR_STRING_BUFFER_LENGTH];
    if( !bRet.failed() )
    {
        int timeOut = 0;
        while( !processPtr->hasExited() && bProcessActive ) /*( (  bCheckQuitRequested == false ) || ( !Controller::getInstance()->QuitRequested(5) ) ) )*/
        {
            ACE_OS::sleep( 1 );
            if(m_timeout != 0)
            {
                timeOut += 1;
                if( timeOut > m_timeout )
                {
                    DebugPrintf(APP_CMD_LOG_LEVEL, "The command %s taking more time to execute than specified timeout value %d\n", m_bDonotLogCmd? "": m_cmd.c_str(), m_timeout);
                    break;
                }
            }
        }
        if( !processPtr->hasExited() )
        {
            if( processPtr->terminate().failed() )
            {
                DebugPrintf(SV_LOG_ERROR, "Failed to terminate the command %s\n", m_bDonotLogCmd? "": m_cmd.c_str());
                outPut = "Failed to terminate the command ";
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "command terminated\n");
                outPut = "The command taking more time to execute than specified timeout value ";
                outPut += boost::lexical_cast<std::string>(m_timeout);
                outPut += ". command terminated ";
            }
            if (!m_bDonotLogCmd)
            {
                outPut += m_cmd ;
            }
            outPut += "\n";
            exitCode = 1;
        }
        else
        {
            processPtr->getExitCode(&exitCode);
            if( exitCode != 0 )
            {
                outPut = "The command: \n";
                if (!m_bDonotLogCmd)
                {
                    outPut += m_cmd;
                }
                outPut += "\nfailed with:\n \"";
                SVERROR errorCode ( exitCode );
                outPut += errorCode.toString(pszError, ARRAYSIZE(pszError));
                outPut += "\"\n";
            }
            bRet = SVS_OK;
        }
    }
    else
    {
        outPut = "Failed to spawn the command ";
        if (!m_bDonotLogCmd)
        {
            outPut += m_cmd;
        }
        outPut += " Error : ";
        outPut += bRet.toString(pszError, ARRAYSIZE(pszError));
        exitCode = 1;
    }
#ifndef SV_WINDOWS
    remove(tmp_script_file.str().c_str());
#endif
    if(!outPut.empty() && !m_outFile.empty())
        WriteStringIntoFile(m_outFile,outPut);

    SAFE_DELETE(processPtr);
    CollectCmdOutPut(outPut);
    DebugPrintf(APP_CMD_LOG_LEVEL, "Exit Code: %ld\n", exitCode);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}
