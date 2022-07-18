/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: AzureRecovery.cpp

Description	: Definitions for recovery operations entry point routines such as
              Initializing configuration Objects,
              Status update,
              Execution Log upload,
              Starting recovery steps.

History		: 1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "AzureRecovery.h"

#include <ace/ACE.h>
#include <ace/OS.h>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "common/Trace.h"
#include "common/utils.h"
#include "config/RecoveryConfig.h"
#include "config/HostInfoDefs.h"
#include "config/HostInfoConfig.h"
#include "resthelper/CloudBlob.h"

using namespace AzureRecovery;

//
// Glogal status object to maintain current execution status.
//
RecoveryStatus RecoveryStatus::s_status_obj;
RecoveryUpdate GetCurrentStatus()
{
    return RecoveryStatus::Instance().GetUpdate();
}

//
// Global variable for working directory. The working directory will be set as part of
// recovery workflow initialization and it will be accessed using GetWorkingDir() routine
// in entire worflow by different code units.
//
static std::string g_workingDir;

std::string GetWorkingDir()
{
    return g_workingDir;
}

static std::string c_hydrationConfigSettings;

std::string GetHydrationConfigSettings()
{
    return c_hydrationConfigSettings;
}

/*
Method      : InitRecoveryConfig

Description : Initializes the host-info & recovery-info singleton objects, sets the
              current working directory and log levels as per the recovery info config
              objects.

Parameters  : [in] recoveryConfigFile: Recovery info configuration file path
              [in] hostinfoFile: Hostinfo xml file path
              [in] workingDir: Working directory path

Return Code : 0                           -> On success,
              E_RECOVERY_CONF_PARSE_ERROR -> On Parse error.
*/
int InitRecoveryConfig(const std::string& recoveryConfigFile,
    const std::string& hostinfoFile,
    const std::string& workingDir,
    const std::string& hydrationConfigSettings)
{
    int retCode = E_RECOVERY_SUCCESS;
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(!recoveryConfigFile.empty());
    BOOST_ASSERT(!hostinfoFile.empty());

    RecoveryStatus::Instance().UpdateProgress(30,
        Recovery_Status::ExecutionProcessing,
        TASK_DESCRIPTIONS::PARSING_CONFIG);

    std::stringstream errorStream;

    try
    {
        g_workingDir = workingDir;

        c_hydrationConfigSettings = hydrationConfigSettings;

        AzureRecoveryConfig::Init(recoveryConfigFile);

        int ilogLevel = AzureRecoveryConfig::Instance().GetLogLevel();

        Trace::ResetLogLevel(AzureRecoveryConfig::Instance().GetLogLevel());

        HostInfoConfig::Init(hostinfoFile);

#ifndef WIN32
        //
        // Copy is required for the linux shell scripts, as they expect the file name:
        //         hostinfo-<source-hostid>.xml
        // Also the xml file should have proper indent so that the script's search logic works properly.
        if (!CopyHostInfoXmlFile(hostinfoFile, workingDir))
        {
            retCode = E_RECOVERY_PREP_ENV;

            errorStream << "Error copying hostinfo xml file.";

            TRACE_ERROR("%s\n", errorStream.str().c_str());
        }
#endif

    }
    catch (const std::exception& e)
    {
        retCode = E_RECOVERY_CONF_PARSE_ERROR;

        errorStream << "Cought exception at initialization. Exception: " << e.what();

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }
    catch (...)
    {
        retCode = E_RECOVERY_CONF_PARSE_ERROR;

        errorStream << "Cought exception at initialization.";

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }

    if (E_RECOVERY_SUCCESS != retCode)
    {
        RecoveryStatus::Instance().SetStatus(
            Recovery_Status::ExecutionFailed);

        RecoveryStatus::Instance().UpdateErrorDetails(
            retCode,
            errorStream.str());
    }

    TRACE_FUNC_END;
    return retCode;
}

/*
Method      : InitMigrationConfig

Description : Initializes the recovery-info singleton objects, sets the current working
              directory and log levels as per the recovery info config objects.

Parameters  : [in] recoveryConfigFile: Recovery info configuration file path
              [in] workingDir: Working directory path

Return Code : 0                           -> On success,
              E_RECOVERY_CONF_PARSE_ERROR -> On Parse error.
*/
int InitMigrationConfig(const std::string& recoveryConfigFile,
    const std::string& workingDir,
    const std::string& hydrationConfigSettings)
{
    int retCode = E_RECOVERY_SUCCESS;
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(!recoveryConfigFile.empty());
    BOOST_ASSERT(!workingDir.empty());

    RecoveryStatus::Instance().UpdateProgress(30,
        Recovery_Status::ExecutionProcessing,
        TASK_DESCRIPTIONS::PARSING_CONFIG);

    RecoveryStatus::Instance().SetStatusErrorMessage(
        "Parsing config files is in progress");

    std::stringstream errorStream;

    try
    {
        g_workingDir = workingDir;

        c_hydrationConfigSettings = hydrationConfigSettings;

        AzureRecoveryConfig::Init(recoveryConfigFile);

        Trace::ResetLogLevel(AzureRecoveryConfig::Instance().GetLogLevel());
    }
    catch (const std::exception& e)
    {
        retCode = E_RECOVERY_CONF_PARSE_ERROR;

        errorStream << "Cought exception at initialization. Exception: " << e.what();

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }
    catch (...)
    {
        retCode = E_RECOVERY_CONF_PARSE_ERROR;

        errorStream << "Cought exception at initialization.";

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }

    if (E_RECOVERY_SUCCESS != retCode)
    {
        RecoveryStatus::Instance().SetStatus(
            Recovery_Status::ExecutionFailed);

        RecoveryStatus::Instance().UpdateErrorDetails(
            retCode,
            errorStream.str());
    }

    TRACE_FUNC_END;
    return retCode;
}

/*
Method      : InitStatusConfig

Description : Initializes on recovery config signleton object. This initialization should be
              done for the operations which need only recovery configuration parameters. Examples
              of such operations are: Execution log upload, Status uploads.

Parameters  : [in] recoveryConfigFile: Recovery info configuration file path

Return Code : 0                           -> On success,
              E_RECOVERY_CONF_PARSE_ERROR -> On Parse error.
*/
int InitStatusConfig(const std::string& recoveryConfigFile)
{
    int retCode = 0;
    TRACE_FUNC_BEGIN;

    try
    {
        AzureRecoveryConfig::Init(recoveryConfigFile);
    }
    catch (const std::exception& e)
    {
        TRACE_ERROR("Cought exception for recovery file initialization. Exception: %s\n", e.what());

        retCode = E_RECOVERY_CONF_PARSE_ERROR;
    }
    catch (...)
    {
        TRACE_ERROR("Cought Unknown Exception for recovery file initialization.\n");
        retCode = E_RECOVERY_CONF_PARSE_ERROR;
    }

    TRACE_FUNC_END;
    return retCode;
}

/*
Method      : UpdateStatusToBlob

Description : Updates the execution status to status-blob by settings the status metadata properties.

Parameters  : [in] update: RecoveryUpdate structure which will have status information.

Return Code : 0                        -> On success,
              E_RECOVERY_INTERNAL      -> On missing status blob uri,
              E_RECOVERY_BLOB_METADATA -> On error accessing or updating status blob metadata.
*/
int UpdateStatusToBlob(const RecoveryUpdate& update)
{
    int retCode = 0;
    TRACE_FUNC_BEGIN;

    do
    {
        std::string statusBlobUri;

        try
        {
            statusBlobUri = AzureRecoveryConfig::Instance().GetStatusBlobUri();
        }
        catch (const std::exception& exp)
        {
            TRACE_ERROR("Can not get status blob uri. Exception: %s\n", exp.what());
            retCode = E_RECOVERY_INTERNAL;
            break;
        }
        catch (...)
        {
            TRACE_ERROR("Can not get status blob uri. Unknown Exception\n");
            retCode = E_RECOVERY_INTERNAL;
            break;
        }

        AzureStorageRest::CloudPageBlob statusBlob(statusBlobUri);

        AzureStorageRest::metadata_t metadata;
        if (!statusBlob.GetMetadata(metadata))
        {
            TRACE_ERROR("Can not update status. Error accessing status blob metadata\n");

            retCode = E_RECOVERY_BLOB_METADATA;
            break;
        }

        std::stringstream progress, errorCode;
        errorCode << update.ErrorCode;
        progress << update.Progress;

        metadata[BlobStatusMetadataName::ExecutionStatus] = update.Status;
        metadata[BlobStatusMetadataName::RunningTask] = update.TaskDescriptoin;
        metadata[BlobStatusMetadataName::ErrorCode] = errorCode.str();
        metadata[BlobStatusMetadataName::ErrorMessage] = update.ErrorMsg;
        metadata[BlobStatusMetadataName::ExecutionProgress] = progress.str();
        metadata[BlobStatusMetadataName::SourceOSDetails] = update.OsDetails;
        metadata[BlobStatusMetadataName::ErrorData] = update.CustomErrorData;
        metadata[BlobStatusMetadataName::TelemetryData] = update.TelemetryData;

        if (!statusBlob.SetMetadata(metadata))
        {
            TRACE_ERROR("Can not update status. Error updating status blob metadata\n");

            retCode = E_RECOVERY_BLOB_METADATA;
            break;
        }

        TRACE_INFO("Status updated successfully\n");

    } while (false);

    TRACE_FUNC_END;
    return retCode;
}

/*
Method      : UploadExecutionLog

Description : Uploads the logFile content to the execution status blob using its SAS Uri.
              SAS uri should have write permision set.

Parameters  : [in] logFile: File path whose content will be upload to status blob.

Return Code : 0                     -> On success,
              E_RECOVERY_INTERNAL   -> On missing status blob uri,
              E_RECOVERY_LOG_UPLOAD -> On log upload failure.
*/
int UploadExecutionLog(const std::string& logFile)
{
    int retCode = 0;
    TRACE_FUNC_BEGIN;

    do
    {
        std::string statusBlobUri;

        try
        {
            statusBlobUri = AzureRecoveryConfig::Instance().GetStatusBlobUri();
        }
        catch (const std::exception& exp)
        {
            TRACE_ERROR("Can not get status blob uri. Exception: %s\n", exp.what());
            retCode = E_RECOVERY_INTERNAL;
            break;
        }
        catch (...)
        {
            TRACE_ERROR("Can not get status blob uri. Unknown Exception\n");
            retCode = E_RECOVERY_INTERNAL;
            break;
        }

        AzureStorageRest::CloudPageBlob statusBlob(statusBlobUri);

        if (!statusBlob.UploadFileContent(logFile))
        {
            TRACE_ERROR("Log File upload error\n");

            retCode = E_RECOVERY_LOG_UPLOAD;
        }
        else
        {
            TRACE_INFO("Successfuly uploaded the file %s\n",
                logFile.c_str());
        }

    } while (false);

    TRACE_FUNC_END;
    return retCode;
}

/*
Method      : UploadExecutionLog

Description : Uploads the current trace log file content to the execution status blob using its SAS Uri.
              SAS uri should have write permision set.

Parameters  : [in] curr_log: File path whose content will be upload to status blob.

Return Code : 0                     -> On success,
              E_RECOVERY_LOG_UPLOAD -> On log upload failure.
*/
int UploadCurrentTraceLog(const std::string& curr_log)
{
    int retcode = 0;
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(!curr_log.empty());

    try
    {
        //
        // Make a copy of the current log file and upload it.
        //
        std::string temp_log_file = curr_log + ".copy";

        boost::filesystem::copy_file(curr_log, temp_log_file);

        retcode = UploadExecutionLog(temp_log_file);

        boost::filesystem::remove(temp_log_file);
    }
    catch (const boost::filesystem::filesystem_error& exp)
    {
        TRACE_ERROR("Error uploading Log. Exception : %s\n", exp.what());
        retcode = E_RECOVERY_LOG_UPLOAD;
    }
    catch (...)
    {
        TRACE_ERROR("Error uploading Log. Unknown Exception\n");
        retcode = E_RECOVERY_LOG_UPLOAD;
    }

    TRACE_FUNC_END;
    return retcode;
}

/*
Method      : CopyHostInfoXmlFile

Description : Creates a copy of host infor xml file with the name hostinfo-<source-histid>.xml
              under the working directory.

Parameters  : [in] hostinfo_xmlfile: Existing xml file name which has host xml info.
              [in] workingDir: Working directory path

Return Code : true       -> On success,
              false      -> On log upload failure.
*/
bool CopyHostInfoXmlFile(const std::string& hostinfo_xmlfile, const std::string& workingDir)
{
    bool bRetStatus = true;
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(!hostinfo_xmlfile.empty());
    BOOST_ASSERT(!workingDir.empty());

    TRACE_INFO("Copying xml file: %s\n", hostinfo_xmlfile.c_str());

    try
    {
        //
        // construct the xml file path
        // 
        std::string destXmlFilePath = workingDir;
        boost::trim(destXmlFilePath);
        if (!boost::ends_with(destXmlFilePath, ACE_DIRECTORY_SEPARATOR_STR_A))
            destXmlFilePath += ACE_DIRECTORY_SEPARATOR_STR_A;
        destXmlFilePath += "hostinfo-" + HostInfoConfig::Instance().HostId() + ".xml";

        TRACE_INFO("Destination xml file: %s\n", destXmlFilePath.c_str());

        //
        // Copy the file.
        //
        bRetStatus = CopyXmlFile(hostinfo_xmlfile, destXmlFilePath);
    }
    catch (const boost::filesystem::filesystem_error& exp)
    {
        TRACE_ERROR("Error creating hostinfo xml file. Exception : %s\n", exp.what());
        bRetStatus = false;
    }
    catch (...)
    {
        TRACE_ERROR("Error creating hostinfo xml file. Unknown Exception\n");
        bRetStatus = false;
    }

    TRACE_FUNC_END;
    return bRetStatus;
}