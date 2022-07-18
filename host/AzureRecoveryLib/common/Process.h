/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	Process.h

Description	:   Utility classes for creating and controlloing the child processes.

History		:   29-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_PROCESS_H
#define AZURE_RECOVERY_PROCESS_H

#include <string>
#include <ace/Process_Manager.h>

namespace AzureRecovery
{
    class CProcess
    {
    public:
        CProcess(const std::string& cmd,
        const std::string& options = "",
        const std::string& log_file = "");

        ~CProcess();

        bool Run(bool waitForChild = false);

        int Wait(long waitTimeSec = 0);

        bool GetConsoleOutput(std::string& log);

        bool GetConsoleStream(std::stringstream& in);

        ACE_exitcode GetExitCode() const;

        bool Terminate();

        bool IsRunning() const;

    private:

        void CloseHandles();

        std::string m_cmd;
        std::string m_cmdOptions;

		bool m_bRandomLogFile;
        std::string m_cmdLog;
        ACE_HANDLE m_hLogFile;

        ACE_Process m_proc;
    };
}

#endif // ~AZURE_RECOVERY_PROCESS_H
