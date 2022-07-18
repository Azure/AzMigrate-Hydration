/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	Process.cpp

Description	:   Utility classes for creating and controlloing the child processes.

History		:   29-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "Process.h"
#include "utils.h"
#include "Trace.h"

#include <sstream>
#include <ace/OS.h>

namespace AzureRecovery
{
//

/*
Method      : CProcess::CProcess

Description : CProcess constructor

Parameters  : [in] cmd: child process binary/script path
              [in, optional] options: arguments to the process
			  [in, optional] log_file: file path to redirect child process stdout & stderr
Return      : 

*/
CProcess::CProcess( const std::string& cmd,
                    const std::string& options,
                    const std::string& log_file)
{
    ACE_ASSERT(!cmd.empty());

    m_cmd = cmd;
    m_cmdOptions = options;

	
	m_bRandomLogFile = log_file.empty();

    m_cmdLog = log_file;

    m_hLogFile = ACE_INVALID_HANDLE;
}

/*
Method      : CProcess::~CProcess

Description : CProcess destructor

Parameters  : None

Return      : None

*/
CProcess::~CProcess()
{
    CloseHandles();

    //
    // Not validating return code as its a destructor.
    //
	if (!m_cmdLog.empty() && m_bRandomLogFile)
		ACE_OS::unlink(m_cmdLog.c_str());
}

/*
Method      : CProcess::Run

Description : Forks the child process. If waitForChild is set then
              block the call until child process exits, otherwise it returns immediatly
			  after creating the child process.

Parameters  : [in] waitForChild : set true to wait for the child process exit.

Return      :  true on successful creation of child process, otherwise false.

*/
bool CProcess::Run(bool waitForChild)
{
    TRACE_FUNC_BEGIN;

    bool bSuccess = true;

    do
    {
        ACE_Process_Options options;
        if (0 != ( m_cmdOptions.empty() ?
			options.command_line(m_cmd.c_str()) :
			options.command_line("%s %s", m_cmd.c_str(), m_cmdOptions.c_str()))
			)
        {
            TRACE_ERROR("Error in forming command line options for the command: %s",
                        m_cmd.c_str());

            bSuccess = false;
            break;
        }

        //
        // Generate random filename if log file is not provided.
        //
        if (m_cmdLog.empty())
        {
			std::string logPrefix;
			if (!GenerateUuid(logPrefix))
            {
                TRACE_ERROR("Could not generate unique name for log file\n.");
                //
                // Take a fixed log file name.
                //
				logPrefix = "Tmp_ChildProc_";
            }

			std::string pwd = GetCurrentDir();
			if (!pwd.empty() && pwd[pwd.length() - 1] != ACE_DIRECTORY_SEPARATOR_CHAR_A)
				pwd += ACE_DIRECTORY_SEPARATOR_CHAR_A;

			m_cmdLog = pwd + logPrefix + ".log";
        }

        m_hLogFile = ACE_OS::open(m_cmdLog.c_str(), O_CREAT | O_TRUNC | O_WRONLY);
        if (ACE_INVALID_HANDLE == m_hLogFile)
        {
            TRACE_ERROR("Could not open log file for child process IO redirection. Error %d\n",
                         ACE_OS::last_error());

            m_hLogFile = ACE_INVALID_HANDLE;

            bSuccess = false;
            break;
        }

        if (0 != options.set_handles(ACE_INVALID_HANDLE, m_hLogFile, m_hLogFile))
        {
            TRACE_ERROR("Error in setting standard handles to child process: %s",
                        m_cmd.c_str());

            bSuccess = false;
            break;
        }

        pid_t pid = m_proc.spawn(options);
        if (ACE_INVALID_PID == pid)
        {
            TRACE_ERROR("Could not spawn child processs. Error %d\n",
                        ACE_OS::last_error());

            bSuccess = false;
            break;
        }

        TRACE("Child process created with pid: %d.\n", pid);

        if (waitForChild)
        {
            TRACE("Waiting for the child process ...\n");

            if (-1 == m_proc.wait())
            {
                TRACE_ERROR("Error in waiting for child processs. Error %d\n",
                    ACE_OS::last_error());

                //
                // Terminating the process on wait error.
                //
                m_proc.terminate();

                bSuccess = false;
                break;
            }

            TRACE("Child process exited with return code: %d\n", m_proc.return_value());
        }
        
    } while (false);

    TRACE_FUNC_END;

    return bSuccess;
}

/*
Method      : CProcess::CloseHandles

Description : Closes the handles related child process stdout & stderr

Parameters  : None

Return      : None

*/
void CProcess::CloseHandles()
{
    TRACE_FUNC_BEGIN;

    if (ACE_INVALID_HANDLE != m_hLogFile)
    {
        ACE_OS::close(m_hLogFile);
        m_hLogFile = ACE_INVALID_HANDLE;
    }

    m_proc.close_dup_handles();

    TRACE_FUNC_END;
}

/*
Method      : CProcess::Wait

Description : waits for the process for specified interval

Parameters  : [in] waitTimeSec : wait time in secs

Return      : child process id of the it exited before wait time, 
              0 if the timeout happened,
			  -1 for wait failure.

*/
int CProcess::Wait(long waitTimeSec)
{
    TRACE_FUNC_BEGIN;
    ACE_Time_Value waitTime(waitTimeSec);
    
    pid_t pid = m_proc.wait(waitTime);

    TRACE_FUNC_END;

    return pid;
}

/*
Method      : CProcess::GetExitCode

Description : Gets the exit code of child process if its exited

Parameters  : None

Return      : returns process exit code if its exited, -1 if the process is still running.

*/
ACE_exitcode CProcess::GetExitCode() const
{
    return m_proc.return_value();
}

/*
Method      : CProcess::Terminate

Description : Terminates the process

Parameters  : None

Return      : true if the process termination was successful, othwewise false.

*/
bool CProcess::Terminate()
{
    return m_proc.terminate() != -1;
}

/*
Method      : CProcess::IsRunning

Description : verified the running status of child process.

Parameters  :None

Return      : true if the process is in running state, otherwise false.

*/
bool CProcess::IsRunning() const
{
    return m_proc.running();
}

/*
Method      : CProcess::GetConsoleOutput

Description : Retrieved the child process stdout & stderr and fills into a string out param

Parameters  : [out] log: string object which will hold child process stdout & stderr

Return      : true on success, otherwise false.

*/
bool CProcess::GetConsoleOutput(std::string& log)
{
	TRACE_FUNC_BEGIN;

	bool bSuccess = true;

	do
	{
		std::stringstream logStream;

		bSuccess = GetConsoleStream(logStream);
		if (bSuccess)
		{
			log = logStream.str();
		}
	} while (false);

	TRACE_FUNC_END;

	return bSuccess;
}

/*
Method      : CProcess::GetConsoleStream

Description : Retrieved the child process stdout & stderr. If the child process still running
              then it returns immediatly with false return code.

Parameters  : [out] in : stream object which will have child process stdout stderr

Return      : true on success, otherwise false.

*/
bool CProcess::GetConsoleStream(std::stringstream& in)
{
	TRACE_FUNC_BEGIN;

	bool bSuccess = true;

	do
	{
		if (IsRunning())
		{
			TRACE_WARNING("Child process is still running.\n");

			bSuccess = CopyAndReadTextFile(m_cmdLog,in);
			break;
		}

		//
		// Close the opened handle related to log file
		//
		CloseHandles();

		std::ifstream LogIn(m_cmdLog.c_str());
		if (LogIn)
		{

			in << LogIn.rdbuf();

		}
		else
		{
			TRACE_ERROR("Can not open child process log file %s\n",
				m_cmdLog.c_str());

			bSuccess = false;
		}
	} while (false);

	TRACE_FUNC_END;

	return bSuccess;
}

}
