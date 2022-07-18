#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "inmcertutil.h"
#include "appcommand.h"
#include "localconfigurator.h"

SV_ULONG RunUrlAccessCommand(const std::string& uriInput,
    const std::string& proxy)
{
    LocalConfigurator lc;
    SV_ULONG lExitCode = SVS_OK;
    std::string cmd = lc.getAzureServicesAccessCheckCmd();
    if (cmd.empty())
    {
        lExitCode = SVE_FAIL;
        DebugPrintf(SV_LOG_ERROR,
            "%s : Access check command not found in configurator.\n",
            FUNCTION_NAME);

        return lExitCode;
    }

    if (uriInput.empty())
    {
        lExitCode = SVE_FAIL;
        DebugPrintf(SV_LOG_ERROR, "%s : empty URI.\n", FUNCTION_NAME);
        return lExitCode;
    }

    std::string uri(uriInput);
    const std::string HTTPS_PREFIX = "https://";
    if (!boost::starts_with(uri, HTTPS_PREFIX))
        uri = HTTPS_PREFIX + uri;

    cmd += " ";
    cmd += uri;
    cmd += " ";
    cmd += boost::lexical_cast<std::string>(30);

    if (!proxy.empty())
    {
        cmd += " ";
        cmd += proxy;
    }

    DebugPrintf(SV_LOG_DEBUG, "Running cmd \"%s\"\n", cmd.c_str());
    std::string cmdOutput;
    SV_UINT uTimeOut = 60;

    std::stringstream tmpOpFile;
    tmpOpFile << "tmp_rcmclipid_";
    tmpOpFile << ACE_OS::getpid();

    boost::filesystem::path tmpFileName(lc.getInstallPath());
    tmpFileName /= tmpOpFile.str();

    AppCommand command(cmd, uTimeOut, tmpFileName.string());
    bool bProcessActive = true;
    SVERROR ret = command.Run(lExitCode, cmdOutput, bProcessActive);

    boost::system::error_code ec;
    if (!boost::filesystem::remove(tmpFileName, ec))
    {
        DebugPrintf(SV_LOG_ERROR,
            "%s: failed to delete file %s with error %s\n",
            FUNCTION_NAME,
            tmpFileName.string().c_str(),
            ec.message().c_str());
    }

    if (ret.failed())
    {
        lExitCode = SVE_FAIL;
        DebugPrintf(SV_LOG_ERROR,
            "%s : failed to run command %s with return status %s.\n",
            FUNCTION_NAME,
            cmd.c_str(), ret.toString());
    }

    DebugPrintf(SV_LOG_ALWAYS, "%s: command output \n %s \n", FUNCTION_NAME, cmdOutput.c_str());

    return lExitCode;
}
