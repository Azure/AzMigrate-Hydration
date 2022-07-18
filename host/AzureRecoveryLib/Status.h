/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: Status.h

Description	: Defines constants for Status properties, Task descriptions and
              Error codes for recovery operations. It also defines a Recovery update
              structure which will be used in status update helper functions.

History		: 1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef _AZURE_RECOVERY_STATUS_UPDATE_H_
#define _AZURE_RECOVERY_STATUS_UPDATE_H_

#define NOMINMAX

#include <string>

namespace Recovery_Status
{
    const char ExecutionPending[]    = "Pending";
    const char ExecutionProcessing[] = "Processing";
    const char ExecutionSuccess[]    = "Success";
    const char ExecutionFailed[]     = "Failed";
}

//
// Define Task Descriptions
//

namespace TASK_DESCRIPTIONS
{
    const char PREPARE_ENVIRONMENT[]       = "Preparing Environment";
    const char INITIATE_RECOVERY[]         = "Initiating recovery steps";
    const char PARSING_CONFIG[]            = "Parsing the configuration files";
    const char PREPARE_DISKS[]             = "Preparing disks and their partitions";
    const char DISCOVER_OS_VOLS[]          = "Discovering source OS volumes";
    const char MOUNT_SYSTEM_PARTITIONS[]   = "Mounting system partition to access OS configuration";
    const char CHANGE_BOOT_CONFIG[]        = "Changing boot configuration";
    const char CHANGE_SVC_NW_CONFIG[]      = "Changing services and network configuration";
    const char CHANGE_BOOT_NW_CONFIG[]     = "Changing boot and network configuration";
    const char CHANGE_FSTAB[]              = "Fixing fstab entries";
    const char UNMOUNT_SYSTEM_PARTITIONS[] = "Un-mounting and performing other cleanup";
    const char UPLOAD_LOG[]                = "Uploading Execution Log";
}

//
// Define Error codes here
//
#define E_RECOVERY_SUCCESS          0

#define E_RECOVERY_INTERNAL         1

#define E_RECOVERY_PREP_ENV         2

#define E_RECOVERY_CONF_PARSE_ERROR 3

#define E_RECOVERY_DISK_NOT_FOUND   4

#define E_RECOVERY_VOL_NOT_FOUND    5

#define E_RECOVERY_PREP_REG         6

#define E_RECOVERY_BOOT_CONFIG      7

#define E_RECOVERY_SVC_CONFIG       8

#define E_RECOVERY_CLEANUP          9

#define E_RECOVERY_INVOLFLT_DRV     10

#define E_RECOVERY_BOOTUP_SCRIPT    11

#define E_RECOVERY_LOG_UPLOAD       12

#define E_RECOVERY_BLOB_METADATA    13

#define E_RECOVERY_HOSTID_UPDATE    14

#define E_RECOVERY_RDP_ENABLE       15

#define E_RECOVERY_VOL_UNEXPECTED_FORMAT    16

#define E_RECOVERY_COULD_NOT_MOUNT_SYS_VOL  17

#define E_RECOVERY_COULD_NOT_UPDATE_BCD     18

#define E_RECOVERY_COULD_NOT_PREPARE_ACTIVE_PARTITION_DRIVE 19

#define E_RECOVERY_COULD_NOT_FIX_FSTAB  20

#define E_RECOVERY_OS_UNSUPPORTED       21

#define E_RECOVERY_HV_DRIVERS_MISSING   22

#define E_RECOVERY_TOOLS_MISSING        23

#define E_RECOVERY_SYS_CONF_MISSING     24

#define E_RECOVERY_GUEST_AGENT_ALREADY_PRESENT    25

#define E_RECOVERY_GUEST_AGENT_INSTALLATION_FAILED    26

#define E_RECOVERY_INITRD_IMAGE_GENERATION_FAILED    27

#define E_RECOVERY_SCRIPT_SYNTAX_ERROR    28

#define E_RECOVERY_ENABLE_DHCP_FAILED    29

#define E_RECOVERY_ENABLE_SERIAL_CONSOLE_FAILED    30

#define E_RECOVERY_DOTNET_FRAMEWORK_INCOMPATIBLE    31

namespace BlobStatusMetadataName
{
    const char ExecutionStatus[]   = "ExecutionStatus";
    const char RunningTask[]       = "RunningTask";
    const char ErrorCode[]         = "ErrorCode";
    const char ErrorMessage[]      = "ErrorMessage";
    const char ErrorData[]         = "ErrorData";
    const char ExecutionProgress[] = "ExecutionProgress";
    const char SourceOSDetails[]   = "SourceOSDetails";
    const char TelemetryData[]     = "TelemetryData";
}

typedef struct _RecoveryStatusUpdate
{
    std::string Status;
    std::string TaskDescriptoin;
    std::string ErrorMsg;
    std::string OsDetails;
    std::string CustomErrorData;
    std::string TelemetryData;
    int ErrorCode;
    int Progress;

    _RecoveryStatusUpdate()
        :ErrorCode(0),
         Progress(0),
         Status(Recovery_Status::ExecutionPending)
    {}
} RecoveryUpdate;

class RecoveryStatus
{
    RecoveryUpdate m_update;

    // This memner hold the error details 
    // set by low level functions.
    std::string m_lastErrorMsg;

    static RecoveryStatus s_status_obj;
    RecoveryStatus() {}
    ~RecoveryStatus() {}

public:
    static RecoveryStatus& Instance()
    {
        return s_status_obj;
    }
    const RecoveryUpdate GetUpdate() const
    {
        return m_update;
    }

    void UpdateProgress(
        int progress,
        const std::string& status,
        const std::string& description)
    {
        m_update.Progress = progress;
        m_update.Status = status;
        m_update.TaskDescriptoin = description;
    }

    void UpdateErrorDetails(
        int errorCode,
        const std::string& msg)
    {
        // Update the ErrorCode only if its not already set,
        // otherwise ignore it. To overwrite the ErrorCode
        // caller has to use SetStatusErrorCode.
        if (m_update.ErrorCode == 0)
            m_update.ErrorCode = errorCode;

        m_update.ErrorMsg = msg;
    }

    void SetLastErrorMessage(const std::string& msg)
    {
        if (!msg.empty())
            m_lastErrorMsg = msg;
    }

    std::string GetLastErrorMessge() const
    {
        return m_lastErrorMsg;
    }

    // m_update setter functions.
    void SetStatus(const std::string& status)
    {
        m_update.Status = status;
    }

    void SetTaskDescription(const std::string& description)
    {
        m_update.TaskDescriptoin = description;
    }

    void SetStatusErrorMessage(const std::string& msg)
    {
        m_update.ErrorMsg = msg;
    }

    void SetSourceOSDetails(const std::string& os_details)
    {
        m_update.OsDetails = os_details;
    }

    void SetCustomErrorData(const std::string& error_data)
    {
        m_update.CustomErrorData = error_data;
    }

    void SetTelemetryData(const std::string& telemetry_data)
    {
        m_update.TelemetryData = telemetry_data;
    }

    void SetStatusErrorCode(int errorCode,
        const std::string &error_data = "")
    {
        m_update.ErrorCode = errorCode;
        if (!error_data.empty())
            m_update.CustomErrorData = error_data;
    }

    void SetProgress(int progress)
    {
        if (progress > 0 && progress < 100)
            m_update.Progress = progress;
    }
};

#endif // ~_AZURE_RECOVERY_STATUS_UPDATE_H_