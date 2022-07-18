/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: main.cpp

Description	: Implements the command line interface for AzureRecoveryUtil.

History		: 7-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include <string>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "command_options.h"
#include "AzureRecovery.h"
#include "common/Trace.h"

namespace po = boost::program_options;

/*
Method      : Usage

Description : Disaplays the AzureRecovery Util usage

Parameters  : [in] options: options description
              [in] optype: represents type of operation. It can be empty if operation-type info
                           not available.

Return code : None
*/
void Usage(po::options_description& options, const std::string& optype = "")
{
    std::string operation_sepcifc_msg = "Usage: --operation " +
        (optype.empty() ? std::string("<operation-type>") : optype) +
        " [other options]";

    std::cout << std::endl
        << operation_sepcifc_msg << std::endl
        << std::endl
        << options << std::endl;
}

/*
Method      : StartStatusUpdate

Description : Updates the status information specified on command line options to status blob.

Parameters  : command line arguments

Return code : 0 on success, any other interger on failure.
*/
int StartStatusUpdate(int argc, char* argv[], bool help)
{
    std::string rec_conf_file;
    std::string task_desc;
    std::string error_msg;
    std::string execution_status;

    int error_code = 0;
    int progress = 0;

    try
    {
        po::options_description status_opt("Status Update options");
        status_opt.add_options()
            (CMD_OPTION::RECV_INFO_FILE,
                po::value<std::string>(&rec_conf_file),
                "Recovery config filepath"
                )
                (CMD_OPTION::STATUS,
                    po::value<std::string>(&execution_status),
                    "Execution status [Processing/Success/Failed]"
                    )
                (CMD_OPTION::TASK_DESC,
                    po::value<std::string>(&task_desc),
                    "Description of current task"
                    )
                (CMD_OPTION::ERROR_CODE,
                    po::value<int>(&error_code),
                    "Error code (number)"
                    )
                (CMD_OPTION::ERROR_MSG,
                    po::value<std::string>(&error_msg),
                    "Error Message"
                    )
                (CMD_OPTION::PROGRESS,
                    po::value<int>(&progress),
                    "Execution progress [0 - 100]"
                    );

        if (help)
        {
            Usage(status_opt, OPERATION::UPDATE_STATUS);

            return 0;
        }

        po::command_line_parser parser(argc, argv);
        parser.options(status_opt).allow_unregistered();

        po::variables_map cmd_values;
        po::store(parser.run(), cmd_values);
        po::notify(cmd_values);

        if (!cmd_values.count(CMD_OPTION::STATUS) ||
            !cmd_values.count(CMD_OPTION::TASK_DESC) ||
            !cmd_values.count(CMD_OPTION::ERROR_CODE) ||
            !cmd_values.count(CMD_OPTION::ERROR_MSG) ||
            !cmd_values.count(CMD_OPTION::PROGRESS) ||
            !cmd_values.count(CMD_OPTION::RECV_INFO_FILE))
        {
            std::cout << "Command error: required argument missing" << std::endl;

            Usage(status_opt, OPERATION::UPDATE_STATUS);

            return 1;
        }
    }
    catch (boost::program_options::error& exp)
    {
        std::cout << "Status update Command error:" << std::endl;

        std::cerr << exp.what() << std::endl;

        return 1;
    }

    //
    // Log for status updates. This log will not be uploaded to status blob.
    //
    Trace::Init(STATUS_WORKFLOW_LOG_FILE);

    if (0 != InitStatusConfig(rec_conf_file))
    {
        TRACE_ERROR("Could not update status\n");
        return 1;
    }

    RecoveryUpdate statusUpdate;
    statusUpdate.ErrorCode = error_code;
    statusUpdate.ErrorMsg = error_msg;
    statusUpdate.Status = execution_status;
    statusUpdate.Progress = progress;
    statusUpdate.TaskDescriptoin = task_desc;

    return UpdateStatusToBlob(statusUpdate);
}

/*
Method      : StartRecovery

Description : Starts the pre-recovery steps execution if the command line options are valid.

Parameters  : command line arguments

Return code : 0 on success, any other interger on failure. Even on recovery steps execution failure
              the function still returns 0, but the status would be set to failed on status blob.
*/
int StartRecovery(int argc, char* argv[], bool help)
{
    std::string hostinfo_file;
    std::string recv_info_file;
    std::string working_dir;
    std::string hydration_config_settings;

    try
    {
        po::options_description rec_opt("Recovery options");
        rec_opt.add_options()
            (CMD_OPTION::RECV_INFO_FILE,
                po::value<std::string>(&recv_info_file),
                "Recovery configuration file path"
                )
            (CMD_OPTION::HOSTINFO_FILE,
                po::value<std::string>(&hostinfo_file),
                "Host info xml file path"
                )
            (CMD_OPTION::WORKING_DIR,
                po::value<std::string>(&working_dir),
                "working directory for the tool"
                )
            (CMD_OPTION::HYDRATION_CONFIG_SETTINGS,
                po::value<std::string>(&hydration_config_settings),
                "Config settings for operations in hydration");

        if (help)
        {
            Usage(rec_opt, OPERATION::RECOVERY);

            return 0;
        }

        po::command_line_parser parser(argc, argv);
        parser.options(rec_opt).allow_unregistered();

        po::variables_map cmd_values;
        po::store(parser.run(), cmd_values);
        po::notify(cmd_values);

        if (!cmd_values.count(CMD_OPTION::WORKING_DIR) ||
            !cmd_values.count(CMD_OPTION::HOSTINFO_FILE) ||
            !cmd_values.count(CMD_OPTION::RECV_INFO_FILE))
        {
            std::cout << "Command error: required argument missing" << std::endl;

            Usage(rec_opt, OPERATION::RECOVERY);

            return 1;
        }
    }
    catch (boost::program_options::error& exp)
    {
        std::cout << "Recovery Command error:" << std::endl;

        std::cerr << exp.what() << std::endl;

        return 1;
    }

    //
    // Initialize logger
    //
    std::string LogFile = working_dir +
        ( boost::ends_with(working_dir, ACE_DIRECTORY_SEPARATOR_STR_A) ?
            "" : ACE_DIRECTORY_SEPARATOR_STR_A ) +
        std::string(RECOVERY_UTIL_LOG_FILE);

    Trace::Init(LogFile);

    do
    {
        //
        // Initialize the configuration objects. If initialization fails then update the 
        // status to blob and exit.
        //
        int ret_code = InitRecoveryConfig(recv_info_file, hostinfo_file, working_dir, hydration_config_settings);

        //
        // Update the current execution status to status-blob
        //
        if (UpdateStatusToBlob(GetCurrentStatus()))
        {
            std::cerr << "Status update error" << std::endl;
        }

        if (0 != ret_code)
            break;

        //
        // Start data massaging steps. Here the StartRecovery return code is not considering 
        // as its already reflected to execution status error code.
        //
        StartRecovery();


        //
        // Upload the log file content to status blob
        //
        UploadCurrentTraceLog(LogFile);

        //
        // Update the current execution status to status-blob
        //
        int retryCount = 3;
        while (retryCount > 0)
        {
            retryCount--;

            if (UpdateStatusToBlob(GetCurrentStatus()))
            {
                std::cerr << "Status update error" << std::endl;

                if (retryCount == 0)
                {
                    return 1;
                }
            }
            else
            {
                break;
            }
        }

    } while (false);

    return 0;
}

/*
Method      : StartUploadLog

Description : Uploads the the execution trace log as status blob content if the commandline
              arguments are proper.

Parameters  : command line arguments

Return code : 0 on success, any other interger on failure.
*/
int StartUploadLog(int argc, char* argv[], bool help)
{
    std::string rec_conf_file;
    std::string log_file;

    try
    {
        po::options_description log_opt("Execution Log upload options");
        log_opt.add_options()
            (CMD_OPTION::RECV_INFO_FILE,
                po::value<std::string>(&rec_conf_file),
                "Recovery config filepath"
                )
                (CMD_OPTION::LOG_FILE,
                    po::value<std::string>(&log_file),
                    "Upload log file path"
                    );

        if (help)
        {
            Usage(log_opt, OPERATION::UPLOAD_LOG);

            return 0;
        }

        po::command_line_parser parser(argc, argv);
        parser.options(log_opt).allow_unregistered();

        po::variables_map cmd_values;
        po::store(parser.run(), cmd_values);
        po::notify(cmd_values);

        if (!cmd_values.count(CMD_OPTION::LOG_FILE) ||
            !cmd_values.count(CMD_OPTION::RECV_INFO_FILE))
        {
            std::cout << "Command error: required argument missing" << std::endl;

            Usage(log_opt, OPERATION::UPLOAD_LOG);

            return 1;
        }
    }
    catch (boost::program_options::error& exp)
    {
        std::cout << "Log upload Command error:" << std::endl;

        std::cerr << exp.what() << std::endl;

        return 1;
    }

    //
    // Trace log for logupload. This log will not be uploaded to status blob.
    //
    Trace::Init(STATUS_WORKFLOW_LOG_FILE);

    int ret_code = InitStatusConfig(rec_conf_file);

    if (0 == ret_code)
    {
        ret_code = UploadExecutionLog(log_file);
    }

    return ret_code;
}

/*
Method      : StartMigration

Description : Starts the migration steps execution if the command line options are valid.

Parameters  : command line arguments

Return code : 0 on success, any other interger on failure. Even on recovery steps execution failure
              the function still returns 0, but the status would be set to failed on status blob.
*/
int StartMigration(int argc, char* argv[], bool help)
{
    std::string recv_info_file;
    std::string working_dir;
    std::string hydration_config_settings;

    try
    {
        po::options_description rec_opt("Migration options");
        rec_opt.add_options()
            (CMD_OPTION::RECV_INFO_FILE,
                po::value<std::string>(&recv_info_file),
                "Recovery configuration file path")
            (CMD_OPTION::WORKING_DIR,
                po::value<std::string>(&working_dir),
                "working directory for the tool")
            (CMD_OPTION::HYDRATION_CONFIG_SETTINGS,
                po::value<std::string>(&hydration_config_settings),
                "Config settings for operations in hydration");

        if (help)
        {
            Usage(rec_opt, OPERATION::MIGRATION);

            return 0;
        }

        po::command_line_parser parser(argc, argv);
        parser.options(rec_opt).allow_unregistered();

        po::variables_map cmd_values;
        po::store(parser.run(), cmd_values);
        po::notify(cmd_values);

        if (!cmd_values.count(CMD_OPTION::WORKING_DIR) ||
            !cmd_values.count(CMD_OPTION::RECV_INFO_FILE))
        {
            std::cout << "Command error: required argument missing" << std::endl;

            Usage(rec_opt, OPERATION::MIGRATION);

            return 1;
        }
    }
    catch (boost::program_options::error& exp)
    {
        std::cout << "Recovery Command error:" << std::endl;

        std::cerr << exp.what() << std::endl;

        return 1;
    }

    //
    // Initialize logger
    //
    std::string LogFile = working_dir +
        (boost::ends_with(working_dir, ACE_DIRECTORY_SEPARATOR_STR_A) ?
            "" : ACE_DIRECTORY_SEPARATOR_STR_A) +
        std::string(RECOVERY_UTIL_LOG_FILE);

    Trace::Init(LogFile);

    do
    {
        //
        // Initialize the configuration objects. If initialization fails then update the 
        // status to blob and exit.
        //
        int ret_code = InitMigrationConfig(recv_info_file, working_dir, hydration_config_settings);

        //
        // Update the current execution status to status-blob
        //
        if (UpdateStatusToBlob(GetCurrentStatus()))
        {
            std::cerr << "Status update error" << std::endl;
        }

        if (0 != ret_code)
            break;

        //
        // Start migration steps.
        // Here the StartMigration return code is not considering 
        // as its already reflected to execution status error code.
        //
        StartMigration();

        //
        // Upload the log file content to status blob
        //
        UploadCurrentTraceLog(LogFile);

        //
        // Update the current execution status to status-blob
        //

        int retryCount = 3;
        while (retryCount > 0)
        {
            retryCount--;

            if (UpdateStatusToBlob(GetCurrentStatus()))
            {
                std::cerr << "Status update error" << std::endl;

                if (retryCount <= 0)
                {
                    return 1;
                }
            }
            else
            {
                break;
            }
        }

    } while (false);

    return 0;
}

/*
Method      : StartGenConversion

Description : Starts the gen conversion steps execution if the command line options are valid.

Parameters  : command line arguments

Return code : 0 on success, any other interger on failure. Even on failure the function
              returns 0, but the status would be set to failed on status blob.
*/
int StartGenConversion(int argc, char* argv[], bool help)
{
    std::string recv_info_file;
    std::string working_dir;
    std::string hydration_config_settings;
    try
    {
        po::options_description rec_opt("Gen conversion options");
        rec_opt.add_options()
            (CMD_OPTION::RECV_INFO_FILE,
                po::value<std::string>(&recv_info_file),
                "Recovery configuration file path")
                (CMD_OPTION::WORKING_DIR,
                    po::value<std::string>(&working_dir),
                    "working directory for the tool")
                    (CMD_OPTION::HYDRATION_CONFIG_SETTINGS,
                        po::value<std::string>(&hydration_config_settings),
                        "Config settings for operations in hydration");

        if (help)
        {
            Usage(rec_opt, OPERATION::GENCONVERSION);

            return 0;
        }

        po::command_line_parser parser(argc, argv);
        parser.options(rec_opt).allow_unregistered();

        po::variables_map cmd_values;
        po::store(parser.run(), cmd_values);
        po::notify(cmd_values);

        if (!cmd_values.count(CMD_OPTION::WORKING_DIR) ||
            !cmd_values.count(CMD_OPTION::RECV_INFO_FILE))
        {
            std::cout << "Command error: required argument missing" << std::endl;

            Usage(rec_opt, OPERATION::MIGRATION);

            return 1;
        }
    }
    catch (boost::program_options::error& exp)
    {
        std::cout << "GenConversion Command error:" << std::endl;

        std::cerr << exp.what() << std::endl;

        return 1;
    }

    //
    // Initialize logger
    //
    std::string LogFile = working_dir +
        (boost::ends_with(working_dir, ACE_DIRECTORY_SEPARATOR_STR_A) ?
            "" : ACE_DIRECTORY_SEPARATOR_STR_A) +
        std::string(RECOVERY_UTIL_LOG_FILE);

    Trace::Init(LogFile);

    do
    {
        //
        // Initialize the configuration objects. If initialization fails
        // then update the status to blob and exit.
        // Config initialization is same for migration and genconversion.
        //
        int ret_code = InitMigrationConfig(recv_info_file, working_dir, hydration_config_settings);

        //
        // Update the current execution status to status-blob
        //
        if (UpdateStatusToBlob(GetCurrentStatus()))
        {
            std::cerr << "Status update error" << std::endl;
        }

        if (0 != ret_code)
            break;

        //
        // Start genconversion steps.
        // Here the StartGenConversion return code is not considering 
        // as its already reflected to execution status error code.
        //
        StartGenConversion();

        //
        // Upload the log file content to status blob
        //
        UploadCurrentTraceLog(LogFile);

        //
        // Update the current execution status to status-blob
        // Retry a maximum of three times.
        //
        int retryCount = 3;
        while (retryCount > 0)
        {
            retryCount--;

            if (UpdateStatusToBlob(GetCurrentStatus()))
            {
                std::cerr << "Status update error" << std::endl;

                if (retryCount <= 0)
                {
                    return 1;
                }
            }
            else
            {
                break;
            }
        }
    } while (false);

    return 0;
}

int main(int argc, char* argv[])
{
    int ret_code = 0;

    bool bHelp = false;
    std::string operationtype;

    GlobalInit();

    try
    {
        po::options_description common("Azure Recovery tool command line options");
        common.add_options()
            (CMD_OPTION::OPERATION,
                po::value<std::string>(&operationtype),
                "Type of operation [recovery/updatestatus/uploadlog/migration]"
                )
                (CMD_OPTION::HELP,
                    po::value<std::string>(&operationtype),
                    "Help options [recovery/updatestatus/uploadlog/migration]"
                    );

        po::command_line_parser parser(argc, argv);
        parser.options(common).allow_unregistered();

        po::variables_map cmd_values;
        po::store(parser.run(), cmd_values);
        po::notify(cmd_values);

        bHelp = cmd_values.count(CMD_OPTION::HELP);

        if (boost::iequals(operationtype, OPERATION::RECOVERY))
        {
            ret_code = StartRecovery(argc, argv, bHelp);
        }
        else if (boost::iequals(operationtype, OPERATION::UPDATE_STATUS))
        {
            ret_code = StartStatusUpdate(argc, argv, bHelp);
        }
        else if (boost::iequals(operationtype, OPERATION::UPLOAD_LOG))
        {
            ret_code = StartUploadLog(argc, argv, bHelp);
        }
        else if (boost::iequals(operationtype, OPERATION::MIGRATION))
        {
            ret_code = StartMigration(argc, argv, bHelp);
        }
        else if (boost::iequals(operationtype, OPERATION::GENCONVERSION))
        {
            ret_code = StartGenConversion(argc, argv, bHelp);
        }
        else
        {
            std::cout << "Command Error: Invalid command option" << std::endl << std::endl;

            Usage(common);

            ret_code = 1;
        }
    }
    catch (boost::program_options::error& exp)
    {
        std::cout << "Command error:" << std::endl << std::endl;

        std::cerr << exp.what() << std::endl;

        ret_code = 1;
    }
    catch (...)
    {
        std::cout << "Unkown exception" << std::endl << std::endl;

        ret_code = 1;
    }

    GlobalUnInit();

    return ret_code;
}