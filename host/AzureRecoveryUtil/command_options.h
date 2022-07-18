#ifndef _AZURE_RECOVERY_UTIL_CMD_OPTIONS_
#define _AZURE_RECOVERY_UTIL_CMD_OPTIONS_

#define STATUS_WORKFLOW_LOG_FILE	"AzureRecovery_StatusUpdate.log"
#define RECOVERY_UTIL_LOG_FILE		"AzureRecoveryUtil.log"

namespace CMD_OPTION
{
    const char HELP[] = "help";
    const char OPERATION[] = "operation";
    const char HOSTINFO_FILE[] = "hostinfofile";
    const char RECV_INFO_FILE[] = "recoveryinfofile";
    const char WORKING_DIR[] = "workingdir";
    const char HYDRATION_CONFIG_SETTINGS[] = "hydrationconfigsettings";

    const char STATUS[] = "status";
    const char TASK_DESC[] = "taskdescription";
    const char ERROR_CODE[] = "errorcode";
    const char ERROR_MSG[] = "errormsg";
    const char PROGRESS[] = "progress";
    const char STATUS_FILE[] = "statusfile";

    const char LOG_FILE[] = "logfile";
}

namespace OPERATION
{
    const char RECOVERY[] = "recovery";
    const char UPDATE_STATUS[] = "updatestatus";
    const char UPLOAD_LOG[] = "uploadlog";
    const char MIGRATION[] = "migration";
    const char GENCONVERSION[] = "genconversion";
}

#endif //~ _AZURE_RECOVERY_UTIL_CMD_OPTIONS_