#include "ServiceHelper.h"
#include "localconfigurator.h"
#include "appcommand.h"
#include "service.h"

SVSTATUS ServiceHelper::StartSVAgents(int timeout, int maxRetryCount)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVSTATUS status = SVE_FAIL;

    bool active = true;
    int retryCount = 0;
    SV_ULONG exitCode = 1;
    std::string cmd, cmdOutput;

#ifdef SV_WINDOWS
    cmd = "net start svagents";
#else
    LocalConfigurator lc;
    cmd = lc.getInstallPath();
    cmd += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cmd += "bin";
    cmd += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cmd += "start";
#endif

    AppCommand objAppCommand(cmd, timeout);
    while (retryCount < maxRetryCount)
    {
        objAppCommand.Run(exitCode, cmdOutput, active, "", NULL);

#ifdef SV_WINDOWS
        InmServiceStatus svcStatus;
        getServiceStatus("svagents", svcStatus);
        InmService svc(svcStatus);
        DebugPrintf(SV_LOG_ALWAYS, "%s : SVAgents service status=%s\n", FUNCTION_NAME, svc.statusAsStr().c_str());
        if (!exitCode || svcStatus == INM_SVCSTAT_RUNNING)
        {
            DebugPrintf(SV_LOG_ALWAYS,
                "%s : SVAgents start succeeded with output : %s\n",
                FUNCTION_NAME, cmdOutput.c_str());
            status = SVS_OK;
            break;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR,
                "%s : SVAgents start failed with exit code : %d, retryCount : %d, output : %s\n",
                FUNCTION_NAME, exitCode, retryCount, cmdOutput.c_str());
        }
#else
        if (!exitCode)
        {
            DebugPrintf(SV_LOG_ALWAYS,
                "%s : SVAgents start succeeded with output : %s\n",
                FUNCTION_NAME, cmdOutput.c_str());
            status = SVS_OK;
            break;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR,
                "%s : SVAgents start failed with exit code : %d, retryCount : %d, output : %s\n",
                FUNCTION_NAME, exitCode, retryCount, cmdOutput.c_str());
        }
#endif
        ACE_OS::sleep(Migration::SleepTime);
        retryCount++;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return status;
}

SVSTATUS ServiceHelper::StopSVAgents(int timeout, int maxRetryCount)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVSTATUS status = SVE_FAIL;

    bool active = true;
    int retryCount = 0;
    SV_ULONG exitCode = 1;
    std::string cmd, cmdOutput;

#ifdef SV_WINDOWS
    cmd = "net stop svagents";
#else
    LocalConfigurator lc;
    cmd = lc.getInstallPath();
    cmd += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cmd += "bin";
    cmd += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cmd += "stop";
    cmd += " svagents";
#endif

    AppCommand objAppCommand(cmd, timeout);
    while (retryCount < maxRetryCount)
    {
        objAppCommand.Run(exitCode, cmdOutput, active, "", NULL);

#ifdef SV_WINDOWS
        InmServiceStatus svcStatus;
        getServiceStatus("svagents", svcStatus);
        InmService svc(svcStatus);
        DebugPrintf(SV_LOG_ALWAYS, "%s : SVAgents service status=%s\n", FUNCTION_NAME, svc.statusAsStr().c_str());

        if (!exitCode || svcStatus == INM_SVCSTAT_STOPPED)
        {
            DebugPrintf(SV_LOG_ALWAYS,
                "%s : SVAgents stop succeeded with output : %s\n",
                FUNCTION_NAME, cmdOutput.c_str());
            status = SVS_OK;
            break;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR,
                "%s : SVAgents stop failed with exit code : %d, retryCount : %d, output : %s\n",
                FUNCTION_NAME, exitCode, retryCount, cmdOutput.c_str());
        }
#else
        if (!exitCode)
        {
            DebugPrintf(SV_LOG_ALWAYS,
                "%s : SVAgents stop succeeded with output : %s\n",
                FUNCTION_NAME, cmdOutput.c_str());
            status = SVS_OK;
            break;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR,
                "%s : SVAgents stop failed with exit code : %d, retryCount : %d, output : %s\n",
                FUNCTION_NAME, exitCode, retryCount, cmdOutput.c_str());
        }
#endif 
        ACE_OS::sleep(Migration::SleepTime);
        retryCount++;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return status;
}

SVSTATUS ServiceHelper::UpdateSymLinkPath(const std::string &linkPath,
    const std::string &filePath, std::string &errMsg)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVSTATUS status = SVS_OK;

    do
    {
        boost::system::error_code ec;
        int retryCount;

        if (boost::filesystem::exists(linkPath, ec))
        {
            retryCount = 0;
            while (retryCount < Migration::MaxRetryCount)
            {
                boost::filesystem::remove_all(linkPath, ec);
                if (ec == boost::system::errc::success)
                {
                    break;
                }
                retryCount++;
                ACE_OS::sleep(Migration::SleepTime);
            }
            if (ec == boost::system::errc::success)
            {
                DebugPrintf(SV_LOG_ALWAYS, "%s : Successfully removed file : %s\n",
                    FUNCTION_NAME, linkPath.c_str());
            }
            else
            {
                errMsg = "Failed to remove symlink file " + linkPath +
                    " with error : " + ec.message();
                status = SVE_FAIL;
                break;
            }
        }

        retryCount = 0;
        while (retryCount < Migration::MaxRetryCount)
        {
            boost::filesystem::create_symlink(filePath, linkPath, ec);
            if (ec == boost::system::errc::success)
            {
                break;
            }
            retryCount++;
            ACE_OS::sleep(Migration::SleepTime);
        }
        if (ec == boost::system::errc::success)
        {
            DebugPrintf(SV_LOG_ALWAYS, "%s : Create sym link succeeded from %s to %s.\n",
                FUNCTION_NAME, linkPath.c_str(), filePath.c_str());
        }
        else
        {
            errMsg = "Failed to create symlink from " + linkPath + " to " +
                filePath + " with error : " + ec.message();
            status = SVE_FAIL;
            break;
        }

    } while(false);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return status;
}

SVSTATUS ServiceHelper::UpdateCSTypeAndRestartSVAgent(const std::string &csType,
    std::string &errMsg)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVSTATUS status = SVS_OK;
    LocalConfigurator lc;

    do
    {
        status = StopSVAgents(
            Migration::ServiceStopTimeOut, Migration::MaxRetryCount);
        if (status != SVS_OK)
        {
            errMsg = "Failed to stop service\n";
            break;
        }

        lc.setCSType(csType);

        std::string installDir = lc.getInstallPath();
        installDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
#ifdef SV_UNIX
        installDir += "bin";
        installDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
#endif
        std::string svagentsLink = installDir + SVAGENTS_LINK;
        std::string svagentsPath = installDir;
        (csType == "CSPrime") ? (svagentsPath += SVAGENTS_RCM) :
            (svagentsPath += SVAGENTS_CS);

        status = UpdateSymLinkPath(svagentsLink, svagentsPath, errMsg);
        if (status != SVS_OK)
        {
            status = SVE_FAIL;
            break;
        }

        std::string s2Path = installDir + (csType == "CSPrime"? S2_RCM : S2_CS);
        lc.setDiffSourceExePathname(s2Path);

        std::string dpPath = installDir + (csType == "CSPrime"? DATAPROTECTION_RCM : DATAPROTECTION_CS);
        lc.setDataProtectionExePathname(dpPath);

        status = ServiceHelper::StartSVAgents(
            Migration::ServiceStartTimeOut, Migration::MaxRetryCount);
        if (status != SVS_OK)
        {
            errMsg = "Failed to start service\n";
            break;
        }
    } while(false);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return status;
}

