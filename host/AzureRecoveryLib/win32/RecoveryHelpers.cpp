/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	RecoveryHelpers.cpp

Description	:   Implementations for windows recovery helper functions.

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "RecoveryHelpers.h"
#include "WinUtils.h"
#include "RegistryUtil.h"
#include "WmiRecordProcessors.h"
#include "WinConstants.h"
#include "../AzureRecovery.h"
#include "../common/Trace.h"
#include "../common/utils.h"
#include "../resthelper/HttpClient.h"
#include "../config/HostInfoDefs.h"
#include "../config/HostInfoConfig.h"
#include "../config/RecoveryConfig.h"

#include <vds.h>

#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <ace/Init_ACE.h>

#include <atlbase.h>

/*
Method      : GlobalInit

Description : Global initialization of the library. This function should be called
before using any of the helper functions in this library.

Parameters  : None

Return      : 0 on success, 1 on failure.

*/
int GlobalInit()
{
    if (!HeapSetInformation(NULL,
        HeapEnableTerminationOnCorruption,
        NULL,
        0))
    {
        std::cerr << "Error enabling termination on heap corruption. Error "
            << GetLastError()
            << std::endl;

        //
        // its not a critical failure hence ignoring the error
        //
    }

    if (-1 == ACE::init())
    {
        std::cerr << "ACE Init failed" << std::endl;
        return 1;
    }

    AzureStorageRest::HttpClient::GlobalInitialize();

    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        std::cerr << "Failed to initialize COM library. Error Code 0x"
            << std::hex << hres << std::endl;
        return 1;
    }
    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );

    if (hres == RPC_E_TOO_LATE)
    {
        hres = S_OK;
    }
    if (FAILED(hres))
    {
        std::cerr << "Failed to initialize security  library. Error Code 0x"
            << std::hex << hres << std::endl;
        return 1;
    }

    return 0;
}

/*
Method      : GlobalUnInit

Description : Global uninitialization. No helper function should be called after this function call.

Parameters  : None

Return      : None

*/
void GlobalUnInit()
{
    AzureStorageRest::HttpClient::GlobalUnInitialize();

    CoUninitialize();
}

/*
Method      : StartRecovery

Description : Entry point function for PreRecovery steps execution.

Parameters  : None

Return      : 0 on success, otherwise recovery error code will be returned.

*/
int StartRecovery()
{
    using namespace AzureRecovery;

    TRACE_FUNC_BEGIN;
    int retcode = 0;

    std::stringstream errStream;
    std::string srcOsInstallPath;
    std::string errorMessage;
    std::string curTaskDesc;

    do
    {
        curTaskDesc = TASK_DESCRIPTIONS::PREPARE_DISKS;
        if (!VerifyDisks(errorMessage))
        {
            //
            // Not failing the operation here, if required disk is not available then
            // the next step will throw error.
            //
            /*retcode = E_RECOVERY_DISK_NOT_FOUND;

            errStream << "Error verifying disks. ( " << errorMessage << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;*/
        }

        curTaskDesc = TASK_DESCRIPTIONS::MOUNT_SYSTEM_PARTITIONS;
        if (!PrepareSourceSystemPath(srcOsInstallPath, errorMessage))
        {
            retcode = E_RECOVERY_VOL_NOT_FOUND;

            errStream << "Could not prepare Source OS install path on Hydration VM.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            //
            // OS volume is not identified, print all the partitions details.
            //
            PrintAllDiskPartitions();

            break;
        }

        if (AzureRecoveryConfig::Instance().IsUEFI())
        {
            std::string activePartitionDrive;
            if (!PrepareActivePartitionDrive(activePartitionDrive, errorMessage))
            {
                retcode = E_RECOVERY_COULD_NOT_PREPARE_ACTIVE_PARTITION_DRIVE;

                errStream << "Could not prepare active partition drive for BCD update. ( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                PrintAllDiskPartitions();

                break;
            }

            if (!UpdateBIOSBootRecordsOnOSDisk(
                srcOsInstallPath,
                activePartitionDrive,
                std::string("ALL"),
                errorMessage))
            {
                retcode = E_RECOVERY_COULD_NOT_UPDATE_BCD;

                errStream << "Could not update boot records on active partition. ( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                PrintAllDiskPartitions();

                break;
            }
        }

        if (EnableSerialConsole(srcOsInstallPath) != ERROR_SUCCESS)
        {
            retcode = E_RECOVERY_ENABLE_SERIAL_CONSOLE_FAILED;

            errStream << "Could not enable serial console for the VM. ";

            TRACE_ERROR("%s\n", errStream.str().c_str());
            // Not a critical failure, continue
        }

        if (!PrepareSourceRegistryHives(srcOsInstallPath, errorMessage))
        {
            retcode = E_RECOVERY_PREP_REG;

            errStream << "Could not prepare the source registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());
            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::CHANGE_BOOT_CONFIG;
        if (!MakeChangesForBootDriversOnRegistry(srcOsInstallPath, errorMessage, retcode))
        {
            errStream << "Could not update required boot drivers on registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::CHANGE_SVC_NW_CONFIG;
        if (!MakeChangesForServicesOnRegistry(srcOsInstallPath, errorMessage))
        {
            retcode = E_RECOVERY_SVC_CONFIG;

            errStream << "Could not update required services on registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        bool isWindows2008 = false;

        OSVersion   osVersion;
        if (GetOsVersion(osVersion)) {
            isWindows2008 = ((osVersion.major == 6) && (osVersion.minor == 0));
        }

        SetOSVersionDetails(srcOsInstallPath);

        DWORD dwSanPolicy = (isWindows2008) ? VDS_SAN_POLICY::VDS_SP_OFFLINE : VDS_SAN_POLICY::VDS_SP_ONLINE;

        if (!SetSANPolicy(errorMessage, dwSanPolicy))
        {
            retcode = E_RECOVERY_SVC_CONFIG;

            errStream << "Could not update SanPolicy on registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        if (!ResetInvolfltParameters(errorMessage))
        {
            retcode = E_RECOVERY_INVOLFLT_DRV;

            errStream << "Could not reset involflt keys in source registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        if (!SetRdpParameters(errorMessage))
        {
            retcode = E_RECOVERY_RDP_ENABLE;

            errStream << "Could not set rdp parameters in source registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        //if (!UpdateNewHostId(errorMessage))
        //{
        //	retcode = E_RECOVERY_HOSTID_UPDATE;

        //	errStream << "Could not update new hostid in source registry.( "
        //		<< errorMessage
        //		<< " )";

        //	TRACE_ERROR("%s\n", errStream.str().c_str());

        //	break;
        //}

        if (!SetBootupScriptEx(errorMessage))
        {
            retcode = E_RECOVERY_BOOTUP_SCRIPT;

            errStream << "Could not add bootup script entries in source registry.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        if (boost::iequals(GetHydrationConfigValue(
            GetHydrationConfigSettings(), HydrationConfig::EnableWindowsGAInstallationDR), "true"))
        {
            std::stringstream wingaErrSteam;
            if (VerifyRegistrySettingsForWinGA(srcOsInstallPath.substr(0,2), wingaErrSteam))
            {
                // Not throwing any error since we do not have soft failures in DR.
                errStream << "Guest agent installation was skipped as the VM already has a Guest Agent present. ";
                TRACE_WARNING("%s\n", errStream.str().c_str());

                break;
            }
            else
            {
                if (!AddWindowsGuestAgent(srcOsInstallPath.substr(0, 3), wingaErrSteam))
                {
                    // Log error code and do not fail.
                    errStream
                        << "VM has successfully failed over but Guest agent installation has failed"
                        << ". Please manually install the guest agent on the VM. ";

                    TRACE_WARNING("%s\n", errStream.str().c_str());

                    break;
                }
            }
        }

        if (!IsDotNetFxVersionPresent(srcOsInstallPath.substr(0, 3)))
        {
            TRACE_INFO("No installation of version 4.0+ of Microsoft.NET found.\n");
            
            if (retcode == 0)
            {
                retcode = E_RECOVERY_DOTNET_FRAMEWORK_INCOMPATIBLE;
            }
        }

    } while (false);

    curTaskDesc = TASK_DESCRIPTIONS::UNMOUNT_SYSTEM_PARTITIONS;
    if (!ReleaseSourceRegistryHives(errorMessage))
    {
        retcode = E_RECOVERY_CLEANUP;
        errStream << "Could not unload source registry hives loaded for data massaging.( "
            << errorMessage
            << " )";

        TRACE_ERROR("%s\n", errStream.str().c_str());
    }
    else
    {
        errStream << "All steps executed successfuly";
    }

    curTaskDesc = TASK_DESCRIPTIONS::UPLOAD_LOG;

    // Update status info.
    RecoveryStatus::Instance().UpdateErrorDetails(
        retcode,
        errStream.str());

    RecoveryStatus::Instance().UpdateProgress(100,
        retcode == 0 ?
        Recovery_Status::ExecutionSuccess :
        Recovery_Status::ExecutionFailed,
        curTaskDesc);

    return retcode;
}

/*
Method      : StartMigration

Description : Entry point function for Migration steps execution.

Parameters  : None

Return      : 0 on success, otherwise recovery error code will be returned.

*/
int StartMigration()
{
    using namespace AzureRecovery;

    TRACE_FUNC_BEGIN;
    int retcode = 0;

    std::stringstream errStream;
    std::string errorMessage;
    std::string curTaskDesc;
    do
    {
        curTaskDesc = TASK_DESCRIPTIONS::PREPARE_DISKS;
        if (!OnlineDisks(errorMessage))
        {
            // Ignore any error here. If OS disk online fails 
            // then the later logic will anyway result in failure.

            TRACE_ERROR(
                "Disk online failed for one of the disks. %s.\n",
                errorMessage.c_str());
        }

        std::list<std::string> osVolumes;
        curTaskDesc = TASK_DESCRIPTIONS::MOUNT_SYSTEM_PARTITIONS;
        if (!DiscoverSourceOSVolumes(osVolumes, errorMessage))
        {
            retcode = E_RECOVERY_VOL_NOT_FOUND;

            errStream << "Could not prepare Source OS install path on Hydration VM.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            //
            // OS volume is not identified, print all the partitions details.
            //
            PrintAllDiskPartitions();

            RecoveryStatus::Instance().SetStatusErrorCode(
                retcode,
                "OS Volume");

            break;
        }
        BOOST_ASSERT(osVolumes.size() > 0);

        // Do hydration for all the OS volumes discovered.
        BOOST_FOREACH(const std::string& srcOsVol, osVolumes)
        {
            std::stringstream srcOsInstallPath;
            srcOsInstallPath
                << boost::trim_right_copy_if(srcOsVol, boost::is_any_of(DIRECOTRY_SEPERATOR))
                << SysConstants::DEFAULT_SYSTEM32_DIR;

            TRACE_INFO("Performing hydration on OS install path: %s.\n",
                srcOsInstallPath.str().c_str());

            SetOSVersionDetails(srcOsInstallPath.str());
            std::string osVersion = RecoveryStatus::Instance().GetUpdate().OsDetails;

            // Check if it is Win2k8, if so then AutoMount should be disabled.
            bool isWin2k8 = boost::starts_with(osVersion,
                RegistryConstants::WIN2K8_VERSION_PREFIX);

            // TODO: UEFI commands need not to run on every OS volume found.
            if (AzureRecoveryConfig::Instance().IsUEFI())
            {
                std::string activePartitionDrive;
                if (!PrepareActivePartitionDrive(
                    activePartitionDrive,
                    errorMessage))
                {
                    retcode = E_RECOVERY_COULD_NOT_PREPARE_ACTIVE_PARTITION_DRIVE;

                    errStream << "Could not prepare active partition drive for BCD update. ( "
                        << errorMessage
                        << " )";

                    TRACE_ERROR("%s\n", errStream.str().c_str());

                    PrintAllDiskPartitions();

                    break;
                }
                if (!UpdateBIOSBootRecordsOnOSDisk(
                    srcOsInstallPath.str(),
                    activePartitionDrive,
                    (std::string) "ALL",
                    errorMessage))
                {
                    retcode = E_RECOVERY_COULD_NOT_UPDATE_BCD;

                    errStream << "Could not update BIOS boot records on active partition. ( "
                        << errorMessage
                        << " )";

                    TRACE_ERROR("%s\n", errStream.str().c_str());

                    PrintAllDiskPartitions();

                    break;
                }
            }

            if (EnableSerialConsole(srcOsInstallPath.str()) != ERROR_SUCCESS)
            {
                retcode = E_RECOVERY_ENABLE_SERIAL_CONSOLE_FAILED;

                std::stringstream scErrStream;
                scErrStream << "Could not update serial console for the VM. ";

                TRACE_ERROR("%s\n", scErrStream.str().c_str());

                // Do not fail.
            }

            bool bSuccess = MakeRegistryChangesForMigration(srcOsInstallPath.str(),
                srcOsVol,
                retcode,
                curTaskDesc,
                errStream,
                isWin2k8);

            if (!IsDotNetFxVersionPresent(srcOsVol))
            {
                TRACE_INFO("No installation of version 4.0+ of Microsoft.NET found.\n");

                if (retcode == 0)
                {
                    retcode = E_RECOVERY_DOTNET_FRAMEWORK_INCOMPATIBLE;
                }
            }

            if (!errStream.str().empty())
            {
                // Formatting if errStream already exists. 
                errStream << ". ";
            }

            errStream << (bSuccess ? "Successfully modified" : "Could not modify")
                << " the OS installation: "
                << srcOsInstallPath.str()
                << ".";
        }

        curTaskDesc = TASK_DESCRIPTIONS::UPLOAD_LOG;
    } while (false);

    // Set error details.
    RecoveryStatus::Instance().UpdateErrorDetails(
        retcode,
        errStream.str());

    // Update progress.
    RecoveryStatus::Instance().UpdateProgress(100,
        retcode == 0 ?
        Recovery_Status::ExecutionSuccess :
        Recovery_Status::ExecutionFailed,
        curTaskDesc);

    return retcode;
}

/*
Method      : StartGenConversion

Description : Entry point function for gen conversion steps execution.

Parameters  : None

Return      : 0 on success, otherwise gen conversion error.

*/
int StartGenConversion()
{
    using namespace AzureRecovery;

    TRACE_FUNC_BEGIN;
    int retcode = E_RECOVERY_SUCCESS;

    std::stringstream errStream;
    std::string errorMessage;
    std::string curTaskDesc;
    do
    {
        curTaskDesc = TASK_DESCRIPTIONS::PREPARE_DISKS;
        if (!OnlineDisks(errorMessage))
        {
            // Ignore any error here. If OS disk online fails 
            // then the later logic will anyway result in failure.

            TRACE_ERROR(
                "Disk online failed for one of the disks. %s.\n",
                errorMessage.c_str());
        }

        std::list<std::string> osVolumes;
        curTaskDesc = TASK_DESCRIPTIONS::MOUNT_SYSTEM_PARTITIONS;
        if (!DiscoverSourceOSVolumes(osVolumes, errorMessage))
        {
            retcode = E_RECOVERY_VOL_NOT_FOUND;

            errStream << "Could not prepare Source OS install path on Hydration VM.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            //
            // OS volume is not identified, print all the partitions details.
            //
            PrintAllDiskPartitions();

            RecoveryStatus::Instance().SetStatusErrorCode(
                retcode,
                "OS Volume");

            break;
        }
        BOOST_ASSERT(osVolumes.size() > 0);

        // Do update bcd recodrds for all the OS volumes discovered.
        BOOST_FOREACH(const std::string& srcOsVol, osVolumes)
        {
            std::stringstream srcOsInstallPath;
            srcOsInstallPath
                << boost::trim_right_copy_if(srcOsVol, boost::is_any_of(DIRECOTRY_SEPERATOR))
                << SysConstants::DEFAULT_SYSTEM32_DIR;

            TRACE_INFO("Performing gen conversion on OS install path: %s.\n",
                srcOsInstallPath.str().c_str());

            SetOSVersionDetails(srcOsInstallPath.str());

            // TODO: UEFI commands need not be run on every OS volume found.
            if (AzureRecoveryConfig::Instance().IsUEFI())
            {
                std::string activePartitionDrive;
                if (!PrepareActivePartitionDrive(
                    activePartitionDrive,
                    errorMessage))
                {
                    retcode = E_RECOVERY_COULD_NOT_PREPARE_ACTIVE_PARTITION_DRIVE;

                    errStream << "Could not prepare active partition drive for BCD update. ( "
                        << errorMessage
                        << " )";

                    TRACE_ERROR("%s\n", errStream.str().c_str());

                    PrintAllDiskPartitions();

                    break;
                }

                // Enable serial console on BIOS BCD Path as the VM has been converted.
                if (EnableSerialConsole(srcOsInstallPath.str()) != ERROR_SUCCESS)
                {
                    retcode = E_RECOVERY_ENABLE_SERIAL_CONSOLE_FAILED;

                    errStream << "Could not update serial console for the VM. ";

                    // Do not fail.
                }

                if (!UpdateBIOSBootRecordsOnOSDisk(
                    srcOsInstallPath.str(),
                    activePartitionDrive,
                    (std::string) "BIOS",
                    errorMessage))
                {
                    retcode = E_RECOVERY_COULD_NOT_UPDATE_BCD;

                    errStream << "Could not update BIOS boot records on active partition. ( "
                        << errorMessage
                        << " )";

                    TRACE_ERROR("%s\n", errStream.str().c_str());

                    PrintAllDiskPartitions();

                    break;
                }

                bool isWin2k8 = false;
                OSVersion osVersion;
                if (GetOsVersion(osVersion)) {
                    isWin2k8 = ((osVersion.major == 6) && (osVersion.minor == 0));
                }

                // Since gen-conversion is a critical failure in protection service, using alternate retCode so hydration doesn't fail
                // if any of the registry changes fail.
                int regRetCode = 0;
                std::stringstream regErrStr;

                // Make registry changes, enable DHCP and install guest agent.
                bool bSuccess = MakeRegistryChangesForMigration(srcOsInstallPath.str(),
                    srcOsVol,
                    regRetCode,
                    curTaskDesc,
                    regErrStr,
                    isWin2k8);

                if (!IsDotNetFxVersionPresent(srcOsVol))
                {
                    TRACE_INFO("No installation of version 4.0+ of Microsoft.NET found.\n");
                    
                    if (retcode == 0)
                    {
                        errStream << "Windows VM Agent requires .NET version 4.0+ .";
                        retcode = E_RECOVERY_DOTNET_FRAMEWORK_INCOMPATIBLE;
                    }
                }

                if (!bSuccess)
                {
                    // Log the error and ignore.
                    TRACE_ERROR("Couldn't modify the registry settings for Gen2 VM. %s\n", regErrStr.str().c_str());
                }

                errStream << "Successfully modified the OS installation: "
                    << srcOsInstallPath.str()
                    << ".";
            }
            else
            {
                TRACE_WARNING("Not a UEFI VM, nothing to update.");

                errStream << "Not a Gen2 VM. Nothing to modify in the OS installation: "
                    << srcOsInstallPath.str()
                    << ".";
            }
        }

        curTaskDesc = TASK_DESCRIPTIONS::UPLOAD_LOG;
    } while (false);

    // Set error details.
    RecoveryStatus::Instance().UpdateErrorDetails(
        retcode,
        errStream.str());

    // Update progress.
    RecoveryStatus::Instance().UpdateProgress(100,
        retcode == 0 ?
        Recovery_Status::ExecutionSuccess :
        Recovery_Status::ExecutionFailed,
        curTaskDesc);

    return retcode;
}

namespace AzureRecovery
{

    /*
    Method      : PrepareSourceOSDrive

    Description : Identifies the source system partition on hydration-vm using source system
    volume drive extents information. And also find the mountpoint or drive-letter
    if available for the system partion, if not assigns a unique mount point for the
    partition.

    Parameters  : [out] osVolMountPath: Filled with source system volume mountpoint/drive-letter on success
    [out, optional] osDiskId: Disk guid/signature, if specified then the system volume lookup
    uses this disk-id instead of the id available in disk extents strucutre.

    Return      : true on success, otherwise false.

    */
    bool PrepareSourceOSDrive(std::string& osVolMountPath, const std::string& osDiskId)
    {
        TRACE_FUNC_BEGIN;
        bool bContinueLoop, bSuccess = false;
        int nMaxRetry = 3;

        do
        {
            bContinueLoop = false;

            disk_extents_t srcOsVolDiskExtents;
            bSuccess = HostInfoConfig::Instance().GetOsDiskDriveExtents(srcOsVolDiskExtents);
            if (!bSuccess)
            {
                TRACE_ERROR("Could not get source disk extents from host info configuration\n");
                break;
            }

            std::string srcOSVolGuid;
            DWORD dwRet = GetVolumeFromDiskExtents(srcOsVolDiskExtents, srcOSVolGuid, osDiskId);
            if (ERROR_FILE_NOT_FOUND == dwRet &&
                nMaxRetry-- > 0)
            {
                TRACE_WARNING("Source OS volume is not visible yet. Retrying the volume enumeration ...\n");
                bContinueLoop = true;

                //Sleep for 20sec before retry
                ACE_OS::sleep(20);

                continue;
            }
            else if (ERROR_SUCCESS != dwRet)
            {
                TRACE_ERROR("Source OS volume not found\n");
                bSuccess = false;
                break;
            }

            std::list<std::string> mountPoints;
            if (ERROR_SUCCESS != GetVolumeMountPoints(srcOSVolGuid, mountPoints))
            {
                TRACE_ERROR("Could not get mount points for OS volume.\n");
                bSuccess = false;
                break;
            }

            if (mountPoints.empty())
            {
                //
                // No mount-point or dirve letter is assigned for os volume.
                // Assing a mount point.
                //
                if (ERROR_SUCCESS != AutoSetMountPoint(srcOSVolGuid, osVolMountPath))
                {
                    TRACE_ERROR("Could not assign mount point for source OS volume\n");
                    break;
                }
            }
            else
            {
                //
                // Get the first mount point from the list
                //
                osVolMountPath = *mountPoints.begin();
            }

            bSuccess = true;

        } while (bContinueLoop);

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : PrepareSourceRegistryHives

    Description : Loads the source registry hives (system & software)

    Parameters  : [in] sourceSystemPath: source system path on hydration vm
                  [out] errorMsg: Filled with rrror details on failure.
                  [in] hiveFlag: Hives to load.

    Return      : true if the registry hives are loaded, otherwise false.

    */
    bool PrepareSourceRegistryHives(const std::string& sourceSystemPath,
        std::string& errorMsg,
        BYTE hiveFlag)
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!sourceSystemPath.empty());
        bool bSuccess = false;

        std::stringstream errorStream;

        do
        {
            //
            // Set the required parmissions to load registry hive
            //
            if (SetPrivileges(SE_BACKUP_NAME, true))
            {
                errorStream << "Failed to set SE_BACKUP_NAME privileges.";

                break;
            }

            if (SetPrivileges(SE_RESTORE_NAME, true))
            {
                errorStream << "Failed to set SE_RESTORE_NAME privileges.";

                break;
            }

            //
            // Load registry hives
            //
            if (hiveFlag & HiveFlags::System)
            {
                std::string SystemHivePath = sourceSystemPath + SysConstants::CONFIG_SYSTEM_HIVE_PATH;
                if (LoadRegistryHive(SystemHivePath, RegistryConstants::VM_SYSTEM_HIVE_NAME))
                {
                    errorStream << "Could not load the source system hive from the path: " << SystemHivePath
                        << ". " << GetLastErrorMsg();
                    break;
                }
            }

            if (hiveFlag & HiveFlags::Software)
            {
                std::string SoftwareHivePath = sourceSystemPath + SysConstants::CONFIG_SOFTEARE_HIVE_PAHT;
                if (LoadRegistryHive(SoftwareHivePath, RegistryConstants::VM_SOFTWARE_HIVE_NAME))
                {
                    errorStream << "Could not load the source software hive from the path: " << SoftwareHivePath
                        << ". " << GetLastErrorMsg();
                    break;
                }
            }

            bSuccess = true;

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : ReleaseSourceRegistryHives

    Description : Unloads the previously loaded source registry hives.

    Parameters  : [out] errorMsg: Filled with error details on failure.
                  [in] hiveFlag: Hives to release.

    Return      : true if the source registry hives are unloaded, otherwise false.

    */
    bool ReleaseSourceRegistryHives(std::string& errorMsg, BYTE hiveFlag)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;

        std::stringstream errorStream;

        do
        {
            //
            // Unload the registry hives
            //
            if ((hiveFlag & HiveFlags::System) &&
                UnloadRegistryHive(RegistryConstants::VM_SYSTEM_HIVE_NAME))
            {
                errorStream << "Could not unload source system registry hive.";
                break;
            }

            if ((hiveFlag & HiveFlags::Software) &&
                UnloadRegistryHive(RegistryConstants::VM_SOFTWARE_HIVE_NAME))
            {
                errorStream << "Could not unload source software registry hive.";
                break;
            }

            //
            // Reset the Backup & Restore Privilages for the process.
            //
            if (SetPrivileges(SE_BACKUP_NAME, false))
            {
                TRACE_ERROR("Failed to set SE_BACKUP_NAME privileges.\n");
                //break; //Ignoring this error as its not such critical to fail the operation
            }

            if (SetPrivileges(SE_RESTORE_NAME, false))
            {
                TRACE_ERROR("Failed to set SE_RESTORE_NAME privileges.\n");
                //break; //Ignoring this error as its not such critical to fail the operation
            }

            bSuccess = true;

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : VerifyBootDriversFiles

    Description : Verfies the required driver files existance on source system path
                  In all newer and supported agents we already have added a check in
                  installation path for driver validation. It is very unlikely that
                  someone deletes driver files during runtime. So removing hydration
                  failure due to missing driver file check.

                  Going forward we will add dynamic check to make sure all drivers
                  are present during tag generation. If not we will not generate tags.

    Parameters  : None

    Return      : true always

    */
    bool VerifyBootDriversFiles(const std::string& srcOSInstallPath)
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!srcOSInstallPath.empty());

        //
        // Verify the following driver files exist on the Source OS volumes
        // 
        std::list<std::string> lstDrivers;

        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\Intelide.sys");
        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\pciidex.sys");
        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\atapi.sys");
        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\ataport.sys");
        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\vmstorfl.sys");
        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\storvsc.sys");
        lstDrivers.push_back(srcOSInstallPath + "\\drivers\\vmbus.sys");

        OSVersion   winVer;

        if (GetOsVersion(winVer)) {
            TRACE_INFO("OS Version Major: %d Minor: %d\n", winVer.major, winVer.minor);
        }

        bool driversMissing = false;
        std::string telemData = "";
        std::list<std::string>::const_iterator iterFile = lstDrivers.begin();
        for (; iterFile != lstDrivers.end(); iterFile++)
        {
            if (!FileExists(*iterFile))
            {
                driversMissing = true;
                TRACE_WARNING("Missing driver file : %s\n", iterFile->c_str());
                if (telemData.empty())
                {
                    telemData += *iterFile;
                }
                else
                {
                    telemData = telemData + "*#*" + *iterFile;
                }
            }
        }

        if (driversMissing)
        {
            RecoveryStatus::Instance().SetCustomErrorData(telemData);
            RecoveryStatus::Instance().SetTelemetryData(telemData);

            // [TODO] This has been made a warning for the 2110. Make a critical failure in 2111.
            // Currently Warning is not shown to the customer, trace remains in hydration log.
            // driversMissing = false;
        }

        TRACE_FUNC_END;
        return driversMissing;
    }

    /*
    Method      : SetBootDriversStartTypeOnRegistry

    Description : Enables set of drivers by changing its registry entries on source
    registry hives.

    Parameters  : None

    Return      : true if all the drivers related registry entries are set, otherwise false.

    */
    bool SetBootDriversStartTypeOnRegistry()
    {
        TRACE_FUNC_BEGIN;
        //
        // Set the following drivers start type as boot start.
        // 
        std::map<std::string, DWORD> svcWithNewStartType;
        svcWithNewStartType["Intelide"] = SERVICE_BOOT_START;
        svcWithNewStartType["atapi"] = SERVICE_BOOT_START;
        svcWithNewStartType["storflt"] = SERVICE_BOOT_START;
        svcWithNewStartType["storvsc"] = SERVICE_BOOT_START;
        svcWithNewStartType["vmbus"] = SERVICE_BOOT_START;

        LONG lRetStatus = RegSetServiceStartType(
            RegistryConstants::VM_SYSTEM_HIVE_NAME,
            svcWithNewStartType);

        TRACE_FUNC_END;
        return ERROR_SUCCESS == lRetStatus;
    }

    /*
    Method      : MakeChangesForBootDriversOnRegistry

    Description : Verifies the required driver files on source system partition and enables them
    if they are in disable mode by modified its corresponding registry enties on
    source registry hives.

    Parameters  : [in] srcOSInstallPath: Source system path on hydration vm
                  [out] errorMsg: Filled with error details on failure.
                  [out] retCode: Return Code for success of operation.

    Return      : returns true on success otherwise false.

    */
    bool MakeChangesForBootDriversOnRegistry(const std::string& srcOSInstallPath, std::string& errorMsg, int& retCode)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;

        std::stringstream errorStream;

        do
        {

            bSuccess = !VerifyBootDriversFiles(srcOSInstallPath);
            if (!bSuccess)
            {
                errorStream << "Required drivers missing. " << GetLastErrorMsg();
                retCode = E_RECOVERY_HV_DRIVERS_MISSING;
                // [TODO:shamish] Only keeping this as warning for 2111 so let other hydration steps carry on.
            }

            bSuccess = SetBootDriversStartTypeOnRegistry();
            if (!bSuccess)
            {
                errorStream << "Could not set all the required dirvers start type to auto start. "
                    << GetLastErrorMsg();

                retCode = E_RECOVERY_BOOT_CONFIG;
                break;
            }

            bSuccess = true;

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : MakeChangesForServicesOnRegistry

    Description : Sets the unwanted service start types to disable/manual-start on source
    registry hives so that when recovering vm boots those service will not
    run on azure environment.

    Parameters  : [in] srcOSInstallPath: source system path on hydration-vm.
                  [out] errorMsg: Filled with error details on failure.

    Return      : true on success otherwise false.

    */
    bool MakeChangesForServicesOnRegistry(const std::string& srcOSInstallPath, std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;

        std::stringstream errorStream;

        do
        {
            //
            // Change following services states if they exist.
            // 
            std::map<std::string, DWORD> svcWithNewStartType;

            svcWithNewStartType[ServiceNames::VMWARE_TOOLS] = SERVICE_DISABLED;
            svcWithNewStartType[ServiceNames::AZURE_GUEST_AGENT] = SERVICE_AUTO_START;
            svcWithNewStartType[ServiceNames::AZURE_TELEMETRY_SERVICE] = SERVICE_AUTO_START;
            svcWithNewStartType[ServiceNames::AZURE_RDAGENT_SVC] = SERVICE_AUTO_START;

            if (AzureRecoveryConfig::Instance().EnableRDP()) {
                svcWithNewStartType[ServiceNames::POLICY_AGENT] = SERVICE_DISABLED;
            }

            LONG lRetStatus = RegSetServiceStartType(RegistryConstants::VM_SYSTEM_HIVE_NAME,
                svcWithNewStartType,
                true);
            if (lRetStatus != ERROR_SUCCESS)
            {
                errorStream << "Could not change the service start type for one of the services. "
                    << GetLastErrorMsg();
                break;
            }

            //
            // Change following services states. If the change opration fails or the services does not exist then 
            // the recovering machine may face issues with network & boot
            //
            svcWithNewStartType.clear();
            svcWithNewStartType[ServiceNames::DHCP_SERVICE] = SERVICE_AUTO_START;

            lRetStatus = RegSetServiceStartType(RegistryConstants::VM_SYSTEM_HIVE_NAME,
                svcWithNewStartType);
            if (lRetStatus != ERROR_SUCCESS)
            {
                errorStream << "Could not change the service start type for one of the services. "
                    << GetLastErrorMsg();
                break;
            }

            bSuccess = true;

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : UpdateNewHostId

    Description : Updates the new HostId in registry settings on source registry hives

    Parameters  : [out] errorMsg: Filled with error details on failure.

    Return      : true on success otherwise false.

    */
    bool UpdateNewHostId(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;

        std::stringstream errorStream;

        bool bSuccess = RegSetSVSystemStringValue(
            RegistryConstants::VALUE_NAME_HOST_ID,
            AzureRecoveryConfig::Instance().GetNewHostId()
        ) == ERROR_SUCCESS;

        if (!bSuccess)
        {
            errorMsg = GetLastErrorMsg();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : SetSANPolicy

    Description : Sets the SanPolicy to "Online All" under partmgr service parameters.

    Parameters  : [out] errorMsg: Filled with error details on failure.
                  [in] dwPolicy: San Policy
                  [in] backupOriginal: backup the original San Policy.

    Return      : true on success otherwise false.

    */
    bool SetSANPolicy(std::string& errorMsg, DWORD dwPolicy, bool backupOriginal)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        std::stringstream errorStream;
        DWORD dwOldSanPolicy = 0,
            currControlSet = 0;

        do
        {
            //
            // Backup the current SanPolicy before changing it
            //
            std::string backupErrorMsg;
            if (backupOriginal &&
                !BackupSanPolicy(backupErrorMsg))
            {
                errorStream << "Could not backup SanPolicy. " << backupErrorMsg;
                bSuccess = false;
                break;
            }

            //
            // Get the control sets, enumerate and update SanPolicy for each of them.
            //
            std::vector<std::string> controlSets;
            LONG lRetStatus = RegGetControlSets(RegistryConstants::VM_SYSTEM_HIVE_NAME, controlSets);
            if (ERROR_SUCCESS != lRetStatus)
            {
                errorStream << "Could not get ControlSets from Source VM system hive. "
                    << GetLastErrorMsg();
                bSuccess = false;
                break;
            }

            auto iterControlSet = controlSets.begin();
            for (; iterControlSet != controlSets.end(); iterControlSet++)
            {
                TRACE_INFO("Performing SanPolicy tuning in control set [%s] dwPolicy: %d.\n",
                    iterControlSet->c_str(), dwPolicy);

                //
                // Update the San Policy as provided for this control set
                //
                lRetStatus = RegUpdateSystemHiveDWORDValue(RegistryConstants::VM_SYSTEM_HIVE_NAME,
                    *iterControlSet,
                    RegistryConstants::PARTMGR_PARAMS_KEY,
                    RegistryConstants::VALUE_NAME_SAN_POLICY,
                    dwOldSanPolicy, // Place holder
                    dwPolicy,
                    true);

                if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream.str("");
                    errorStream << "Could not update "
                        << RegistryConstants::VALUE_NAME_SAN_POLICY
                        << " in control set: " << *iterControlSet
                        << ". Error " << lRetStatus << std::endl;
                    TRACE_WARNING(errorStream.str().c_str());
                }
            }
        } while (false);

        TRACE_FUNC_END;
        return bSuccess;
    }



    /*
    Method      : SetAutoMount

    Description : Enables or disables the auto mount.

    Parameters  : [out] errorMsg: Filled with error details on failure.
                  [in] disableAutoMount: If true then disables the auto mount.

    Return      : true on success otherwise false.
    */
    bool SetAutoMount(std::string& errorMsg, bool disableAutoMount)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        std::stringstream errorStream;
        DWORD dwNoAutoMount = disableAutoMount ? 1 : 0,
            dwOldNoAutoMount = 0,
            currControlSet = 0;

        do
        {

            //
            // Get the control sets, enumerate and update SanPolicy for each of them.
            //
            std::vector<std::string> controlSets;
            LONG lRetStatus = RegGetControlSets(RegistryConstants::VM_SYSTEM_HIVE_NAME, controlSets);
            if (ERROR_SUCCESS != lRetStatus)
            {
                errorStream << "Could not get ControlSets from Source VM system hive. "
                    << GetLastErrorMsg();
                bSuccess = false;
                break;
            }

            auto iterControlSet = controlSets.begin();
            for (; iterControlSet != controlSets.end(); iterControlSet++)
            {
                TRACE_INFO("Updating NoAutMount in control set [%s] NoAutoMount: %d.\n",
                    iterControlSet->c_str(), dwNoAutoMount);

                //
                // Update the San Policy as provided for this control set
                //
                lRetStatus = RegAddOrUpdateSystemHiveDWORDValue(
                    RegistryConstants::VM_SYSTEM_HIVE_NAME,
                    *iterControlSet,
                    RegistryConstants::MOUNTMGT_PARAMS_KEY,
                    RegistryConstants::VALUE_NAME_NOAUTOMOUNT,
                    dwOldNoAutoMount, // Place holder
                    dwNoAutoMount,
                    true);

                if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream.str("");
                    errorStream << "Could not update "
                        << RegistryConstants::VALUE_NAME_NOAUTOMOUNT
                        << " in control set: " << *iterControlSet
                        << ". Error " << lRetStatus << std::endl;
                    TRACE_WARNING(errorStream.str().c_str());
                }
            }
        } while (false);

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : BackupSanPolicy

    Description : Backups the current SanPolicy DWORD value under SV-Systems key.

    Parameters  : [out] errorMsg: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool BackupSanPolicy(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;
        std::stringstream errorStream;
        do
        {
            //
            // Check if there is a backup key already exists. If so then no need to backup again, it might have already done
            // and workflow might be ran again.
            //
            DWORD dwSanPolicyBackup = 0;
            LONG lRetStatus = RegGetSVSystemDWORDValue(RegistryConstants::VALUE_NAME_SAN_POLICY_BACKUP, dwSanPolicyBackup);
            if (ERROR_SUCCESS == lRetStatus)
            {
                TRACE_INFO("Registry value %s = %lu already present under SV-Systems key. Not taking backup again.\n",
                    RegistryConstants::VALUE_NAME_SAN_POLICY_BACKUP, dwSanPolicyBackup);
                break;
            }
            else if (ERROR_FILE_NOT_FOUND != lRetStatus)
            {
                errorStream << "Could not check existing backup sanpolicy. " << GetLastErrorMsg();
                bSuccess = false;
                break;
            }

            //
            // Backup the current sanpolicy value.
            //
            DWORD currControlSet = 0;
            lRetStatus = RegGetCurrentControlSetValue(RegistryConstants::VM_SYSTEM_HIVE_NAME, currControlSet);
            if (ERROR_SUCCESS != lRetStatus)
            {
                errorStream << "Could not get the Current ControlSet number. Error " << lRetStatus;
                bSuccess = false;
                break;
            }
            else if (0 == currControlSet)
            {
                errorStream << "Current ControlSet number should not be " << currControlSet;
                bSuccess = false;
                break;
            }
            else
            {
                //
                // Construct the control set key name
                //
                std::stringstream controlSetNum;
                controlSetNum << RegistryConstants::CONTROL_SET_PREFIX
                    << std::setfill('0')
                    << std::setw(3)
                    << currControlSet;

                TRACE_INFO("Current ControlSet is %s\n", controlSetNum.str().c_str());

                //
                // Backup the existing SanPolicy value by storing it under "SV Systems" registry key with name Backup.SanPolicy
                //
                dwSanPolicyBackup = VDS_SAN_POLICY::VDS_SP_UNKNOWN;
                lRetStatus = RegGetSystemHiveDWORDValue(RegistryConstants::VM_SYSTEM_HIVE_NAME,
                    controlSetNum.str(),
                    RegistryConstants::PARTMGR_PARAMS_KEY,
                    RegistryConstants::VALUE_NAME_SAN_POLICY,
                    dwSanPolicyBackup);

                if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not retrieve existing "
                        << RegistryConstants::VALUE_NAME_SAN_POLICY
                        << " in control set: " << controlSetNum.str()
                        << ". Error " << lRetStatus << std::endl;

                    bSuccess = false;
                    break;
                }

                //
                // SanPolicy should not be 0. 0 is miss configuration.
                //
                if (VDS_SAN_POLICY::VDS_SP_UNKNOWN == dwSanPolicyBackup)
                {
                    TRACE_WARNING("SanPolicy value for current controlset %s should not be %lu.\n",
                        controlSetNum.str().c_str(),
                        dwSanPolicyBackup);
                }
                else if (VDS_SAN_POLICY::VDS_SP_ONLINE == dwSanPolicyBackup)
                {
                    TRACE_INFO("SanPolicy is ONLINE ALL already. No changes\n");
                }
                else if (ERROR_SUCCESS != RegSetSVSystemDWORDValue(
                    RegistryConstants::VALUE_NAME_SAN_POLICY_BACKUP,
                    dwSanPolicyBackup))
                {
                    errorStream << "Could not store Old SanPolicy for restore. "
                        << GetLastErrorMsg();

                    bSuccess = false;
                    break;
                }
                else
                {
                    TRACE_INFO("Stored the Old SanPolicy value %lu under SV-Systems key for restore.\n",
                        dwSanPolicyBackup);
                }
            }
        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : ResetInvolfltParameters

    Description : Resets the involflt filter driver registry settings on source registry hives

    Parameters  : [out] errorMsg: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool ResetInvolfltParameters(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        std::stringstream errorStream;

        do
        {
            CRegKey vxInstallPathKey;

            std::string vxInstallPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
                + std::string(RegistryConstants::INMAGE_INSTALL_PATH_X32);

            LONG lRetStatus = vxInstallPathKey.Open(HKEY_LOCAL_MACHINE, vxInstallPath.c_str(), KEY_READ);
            if (ERROR_SUCCESS == lRetStatus)
            {
                //No changes required for 32bit OS. Hence exiting.
                TRACE_INFO("Involflt changes are not needed for 32bit OS.\n");

                break;
            }
            else if (ERROR_FILE_NOT_FOUND != lRetStatus)
            {
                errorStream << "Could not determine OS bits type. RegKeyOpen error " << lRetStatus
                    << " for the key " << vxInstallPath;

                bSuccess = false;
                break;
            }

            vxInstallPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
                + std::string(RegistryConstants::INMAGE_INSTALL_PATH_X64);

            lRetStatus = vxInstallPathKey.Open(HKEY_LOCAL_MACHINE, vxInstallPath.c_str(), KEY_READ);
            if (ERROR_SUCCESS != lRetStatus)
            {
                errorStream << "Could not open " << vxInstallPath
                    << ". Error " << lRetStatus;

                bSuccess = false;
                break;
            }

            std::vector<std::string> contolSets;
            lRetStatus = RegGetControlSets(RegistryConstants::VM_SYSTEM_HIVE_NAME, contolSets);
            if (ERROR_SUCCESS != lRetStatus)
            {
                errorStream << "Could not get ControlSets from Source VM system hive. "
                    << GetLastErrorMsg();
                break;
            }

            auto iterControlSet = contolSets.begin();
            for (; iterControlSet != contolSets.end(); iterControlSet++)
            {
                TRACE_INFO("Performing involflt tuining in control set [%s].\n",
                    (*iterControlSet).c_str());

                std::string strInvolfltParamKey = RegistryConstants::VM_SYSTEM_HIVE_NAME
                    + std::string(DIRECOTRY_SEPERATOR)
                    + *iterControlSet
                    + RegistryConstants::INVOLFLT_PARAM_KEY;

                CRegKey involFltKey;
                lRetStatus = involFltKey.Open(HKEY_LOCAL_MACHINE, strInvolfltParamKey.c_str(), KEY_ALL_ACCESS);
                if (ERROR_FILE_NOT_FOUND == lRetStatus)
                {
                    TRACE_INFO("Key [%s] is not found in control set %s.\n",
                        strInvolfltParamKey.c_str(),
                        iterControlSet->c_str());
                    //
                    // Key is not found in this control set, look for the key in next control set.
                    //
                    continue;
                }
                else if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not open " << strInvolfltParamKey
                        << ". Error " << lRetStatus;

                    bSuccess = false;
                    break;
                }

                std::map<std::string, DWORD> mapDwordValues;
                mapDwordValues[RegistryConstants::VALUE_DATAPOOLSIZE] = 0;
                mapDwordValues[RegistryConstants::VALUE_VOL_DATASIZE_LIMIT] = 0;

                lRetStatus = RegSetDWORDValues(involFltKey, mapDwordValues);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not set values for the key " << strInvolfltParamKey;
                    bSuccess = false;
                    break;
                }
            }

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }


    /*
    Method      : SetRdpParameters

    Description : Sets the rdp related registry parameters on source registry hives

    Parameters  : [out] errorMsg: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool SetRdpParameters(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        if (!AzureRecoveryConfig::Instance().EnableRDP())
        {
            TRACE_INFO("EnableRDP is not set, skipping enable RDP changes.\n");

            TRACE_FUNC_END;
            return bSuccess;
        }

        std::stringstream errorStream;

        do
        {
            std::vector<std::string> contolSets;
            LONG lRetStatus = RegGetControlSets(RegistryConstants::VM_SYSTEM_HIVE_NAME, contolSets);
            if (ERROR_SUCCESS != lRetStatus)
            {
                errorStream << "Could not get ControlSets from Source VM system hive. "
                    << GetLastErrorMsg();

                bSuccess = false;
                break;
            }

            auto iterControlSet = contolSets.begin();
            for (; iterControlSet != contolSets.end(); iterControlSet++)
            {
                TRACE_INFO("Setting Rdp parameters in control set [%s].\n",
                    (*iterControlSet).c_str());

                std::string strRdpAllowKey = RegistryConstants::VM_SYSTEM_HIVE_NAME
                    + std::string(DIRECOTRY_SEPERATOR)
                    + *iterControlSet
                    + RegistryConstants::RDP_ALLOW_KEY;

                CRegKey rdpKey;
                lRetStatus = rdpKey.Open(HKEY_LOCAL_MACHINE, strRdpAllowKey.c_str(), KEY_ALL_ACCESS);
                if (ERROR_FILE_NOT_FOUND == lRetStatus)
                {
                    TRACE_INFO("Key [%s] is not found in control set %s.\n",
                        strRdpAllowKey.c_str(),
                        iterControlSet->c_str());
                    //
                    // Key is not found in this control set, look for the key in next control set.
                    //
                    continue;
                }
                else if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not open " << strRdpAllowKey
                        << ". Error " << lRetStatus;

                    bSuccess = false;
                    break;
                }

                std::map<std::string, DWORD> mapDwordValues;
                mapDwordValues[RegistryConstants::VALUE_TSCONNECTION] = 0;

                lRetStatus = RegSetDWORDValues(rdpKey, mapDwordValues);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not set values for the key " << strRdpAllowKey;
                    bSuccess = false;
                    break;
                }

                std::string strRdpNlaKey = RegistryConstants::VM_SYSTEM_HIVE_NAME
                    + std::string(DIRECOTRY_SEPERATOR)
                    + *iterControlSet
                    + RegistryConstants::RDP_DISABLE_NLA_KEY;
                lRetStatus = rdpKey.Open(HKEY_LOCAL_MACHINE, strRdpNlaKey.c_str(), KEY_ALL_ACCESS);
                if (ERROR_FILE_NOT_FOUND == lRetStatus)
                {
                    TRACE_INFO("Key [%s] is not found in control set %s.\n",
                        strRdpNlaKey.c_str(),
                        iterControlSet->c_str());
                    //
                    // Key is not found in this control set, look for the key in next control set.
                    //
                    continue;
                }
                else if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not open " << strRdpNlaKey
                        << ". Error " << lRetStatus;

                    bSuccess = false;
                    break;
                }

                mapDwordValues.clear();
                mapDwordValues[RegistryConstants::VALUE_SECLAYER] = 0;
                mapDwordValues[RegistryConstants::VALUE_USERAUTH] = 0;

                lRetStatus = RegSetDWORDValues(rdpKey, mapDwordValues);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    errorStream << "Could not set values for the key " << strRdpNlaKey;
                    bSuccess = false;
                    break;
                }
            }
        } while (false);

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : VerifyRegistrySettingsForWinGA

    Description : Verifies registry settings and required files are in place for guest agent.

    Parameters  : [in] srcOSVolume: Source OS Volume.
                  [out] errorstream: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool VerifyRegistrySettingsForWinGA(
        const std::string& srcOSVolume,
        std::stringstream& errorstream)
    {
        TRACE_FUNC_BEGIN;
        bool isSuccess = true;
        try
        {
            /*
                This is a temporary code avoiding loops in effort to avoid Memory Corruption issue.
                This will be refactored before next release to reduce code duplicacy.
            */
            do
            {
                std::vector<std::string> controlSets;
                DWORD lRetStatus = RegGetControlSets(RegistryConstants::VM_SYSTEM_HIVE_NAME, controlSets, std::string(RegistryConstants::VALUE_NAME_LASTKNOWNGOOD));
                if (ERROR_SUCCESS != lRetStatus)
                {
                    TRACE_ERROR("Could not get ControlSets from Source VM system hive.\n");
                    isSuccess = false;
                    break;
                }

                //ArrayLess Implementation
                std::string lastKnownGoodCS = controlSets[0];

                // BEGIN -- WindowsAzureGuestAgent Service check.
                TRACE_INFO("Checking guest agent registry settings for service : %s\n in OS Volume %s\n",
                    ServiceNames::AZURE_GUEST_AGENT,
                    srcOSVolume.c_str());

                std::string winGASubKey = RegistryConstants::VM_SYSTEM_HIVE_NAME +
                    (std::string) DIRECOTRY_SEPERATOR +
                    lastKnownGoodCS +
                    RegistryConstants::SERVICES +
                    (std::string) ServiceNames::AZURE_GUEST_AGENT; // WindowsAzureGuestAgent

                CRegKey winGACurrKey;
                lRetStatus = winGACurrKey.Open(HKEY_LOCAL_MACHINE, winGASubKey.c_str(), KEY_READ);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    TRACE_ERROR("Could not open registry key %s. Error %ld\n",
                        winGASubKey.c_str(),
                        lRetStatus);

                    isSuccess = false;
                    break;
                }

                std::string imagePath = "";
                lRetStatus = RegGetStringValue(winGACurrKey, RegistryConstants::VALUE_IMAGE_PATH, imagePath);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    TRACE_ERROR("Could not read %s value under the key %s. Error %ld\n",
                        RegistryConstants::VALUE_CURRENT_VERSION,
                        winGASubKey.c_str(),
                        lRetStatus);

                    isSuccess = false;
                    break;
                }

                // Image Path will be in reference to paritions on source VM. Replace SourceOS partitions with volume's name on the hydration vm.
                size_t delimiterLocation = imagePath.find("\\");
                if (delimiterLocation != std::string::npos
                    && delimiterLocation == 2)
                {
                    // Format of srcOSVolume is C:\\ so we replace upto \\ and not just before it.
                    imagePath.replace(0, delimiterLocation + 1, srcOSVolume);

                    if (FileExists(imagePath))
                    {
                        // Both registry settings and guest agent files exist.
                        TRACE_INFO("Windows guest agent files exist on source disk. %s \n", imagePath.c_str());
                    }
                    else
                    {
                        TRACE_INFO("Could not find guest agent files on source disk. %s \n", imagePath.c_str());
                        isSuccess = false;
                        break;
                    }
                }
                else
                {
                    TRACE_INFO("Could not find correct image path format for WindowsAzureGuestAgent service. Image path: %s \n", imagePath.c_str());
                    isSuccess = false;
                    break;
                }
                // END -- WindowsAzureGuestAgent Service check.

                // BEGIN -- RdAgent Service check.
                TRACE_INFO("Checking guest agent registry settings for service : %s\n",
                    ServiceNames::AZURE_RDAGENT_SVC);

                winGASubKey = RegistryConstants::VM_SYSTEM_HIVE_NAME +
                    (std::string) DIRECOTRY_SEPERATOR +
                    lastKnownGoodCS +
                    RegistryConstants::SERVICES +
                    (std::string) ServiceNames::AZURE_RDAGENT_SVC; //RdAgent

                lRetStatus = winGACurrKey.Open(HKEY_LOCAL_MACHINE, winGASubKey.c_str(), KEY_READ);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    TRACE_ERROR("Could not open registry key %s. Error %ld\n",
                        winGASubKey.c_str(),
                        lRetStatus);

                    isSuccess = false;
                    break;
                }

                lRetStatus = RegGetStringValue(winGACurrKey, RegistryConstants::VALUE_IMAGE_PATH, imagePath);
                if (ERROR_SUCCESS != lRetStatus)
                {
                    TRACE_ERROR("Could not read %s value under the key %s. Error %ld\n",
                        RegistryConstants::VALUE_CURRENT_VERSION,
                        winGASubKey.c_str(),
                        lRetStatus);

                    isSuccess = false;
                    break;
                }

                // Image Path will be in reference to paritions on source VM. Replace SourceOS partitions with volume's name on the hydration vm.
                delimiterLocation = imagePath.find("\\");
                if (delimiterLocation != std::string::npos
                    && delimiterLocation == 2)
                {
                    // Format of srcOSVolume is C:\\ so we replace upto \\ and not just before it.
                    imagePath.replace(0, delimiterLocation + 1, srcOSVolume);

                    if (FileExists(imagePath))
                    {
                        // Both registry settings and guest agent files exist.
                        TRACE_INFO("Windows guest agent files exist on source disk. %s \n", imagePath.c_str());
                    }
                    else
                    {
                        TRACE_INFO("Could not find guest agent files on source disk. %s \n", imagePath.c_str());
                        isSuccess = false;
                        break;
                    }
                }
                else
                {
                    TRACE_INFO("Could not find correct image path format for RdAgent service. Image path: %s \n", imagePath.c_str());
                    isSuccess = false;
                    break;
                }
                // END -- RdAgent Service check.
            } while (false);
        }
        catch (const std::exception& exp)
        {
            isSuccess = false;
            errorstream << "Could not verify windows guest agent files and settings." << std::endl
                << "Error details: " << exp.what();
            TRACE_ERROR(errorstream.str().c_str());
        }

        TRACE_FUNC_END;
        return isSuccess;
    }

    /*
    Method      : AddWindowsGuestAgent

    Description : Adds Windows Guest Agent file and sets registry setting accordingly.

    Parameters  : [in] srcOSVolume: Source OS Volume.
                  [out] errorMsg: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool AddWindowsGuestAgent(
        const std::string& srcOSVolume,
        std::stringstream& errorStream)
    {
        TRACE_FUNC_BEGIN;
        bool isSuccess = false;

        try
        {
            do
            {
                // Step1. Check whether a Windows GA folder already exists.
                std::stringstream winGAInstallPath;
                winGAInstallPath
                    << boost::trim_right_copy_if(srcOSVolume, boost::is_any_of(DIRECOTRY_SEPERATOR))
                    << SysConstants::WINDOWS_AZURE_DIR;

                if (DirectoryExists(winGAInstallPath.str()))
                {
                    std::string backupDirSuffix;
                    GenerateUuid(backupDirSuffix);
                    backupDirSuffix += ".old";

                    // Create a backup of old WinGA configuration by renaming it to WindowsAzure<UUID>.old
                    RenameDirectory(winGAInstallPath.str(),
                        winGAInstallPath.str() + backupDirSuffix);

                    errorStream << "Windows Azure Folder has been backed up.\n";
                    TRACE_INFO(errorStream.str().c_str());
                }

                std::string imagePath = "";
                if (!TransferWinGARegistrySettings(
                    srcOSVolume,
                    imagePath,
                    errorStream))
                {
                    errorStream << "Transferring Windows Guest Agent Registry settings from Hydration VM to Source VM failed.\n" << GetLastErrorMsg() << std::endl;
                    TRACE_ERROR(errorStream.str().c_str());
                    isSuccess = false;
                    break;
                }
                else
                {
                    // Copy the Windows Azure folder which corresponds to Guest Agent
                    std::stringstream hydVmWinAzureDir;
                    hydVmWinAzureDir
                        << SOURCE_OS_MOUNT_POINT_ROOT
                        << SysConstants::WINDOWS_AZURE_DIR;

                    if (!CreateDirIfNotExist(winGAInstallPath.str()))
                    {
                        errorStream << "Failed to create WindowsAzure folder on source disk. Copying binaries failed\n";
                        TRACE_ERROR(errorStream.str().c_str());
                        break;
                    }

                    // Copy only the Guest Agent Folder.
                    std::string guestAgentFolder = RegistryConstants::WINGA_PACKAGES_DIR_PATH;

                    if (imagePath.length() != 0)
                    {
                        errorStream << "Found the guest agent installation folder from Hydration VM's registry setting: " << imagePath <<std::endl;
                        TRACE_INFO(errorStream.str().c_str());

                        guestAgentFolder = imagePath;
                    }
                    else
                    {
                        // If imagePath is not poulated, try to copy the Packages folder.
                        imagePath = guestAgentFolder;
                    }


                    size_t delimiterLocation = imagePath.find("\\");
                    if (delimiterLocation != std::string::npos
                        && delimiterLocation == 2)
                    {
                        // Format of srcOSVolume is C:\\ so we replace upto \\ and not just before it.
                        imagePath.replace(0, delimiterLocation + 1, srcOSVolume);
                    }

                    TRACE_INFO("Hydration VM Guest Agent Folder: %s\nTarget VM Guest Agent folder: %s\n", guestAgentFolder.c_str(), imagePath.c_str());


                    if (!CopyDirectoryRecursively(
                        guestAgentFolder,
                        imagePath))
                    {
                        errorStream << "Failed to copy Guest agent binaries to source disk." << std::endl;
                        TRACE_ERROR(errorStream.str().c_str());
                        break;
                    }
                }
                isSuccess = true;
            } while (false);
        }
        catch (const std::exception& exp)
        {
            errorStream << "Could not add Windows Guest Agent."
                << "Error details: " << exp.what() << std::endl;
            TRACE_ERROR(errorStream.str().c_str());
        }

        TRACE_FUNC_END;
        return isSuccess;
    }

    /*
     Method     : TransferWinGARegistrySettings

    Description : Checks the Hydration VM and source disk control sets and transfers services from former to latter.

    Parameters  : [in] srcOSVolume: Source OS Volume.
                  [out] imagePath: Image path for the guest agent root folder.
                  [out] errorstream: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool TransferWinGARegistrySettings(
        std::string srcOSVol,
        std::string& imagePath,
        std::stringstream& errorstream)
    {
        TRACE_FUNC_BEGIN;
        bool isSuccess = true;
        try
        {
            do
            {
                // Get the current control set of Hydration VM.
                std::vector<std::string> controlSets;
                if (ERROR_SUCCESS != RegGetControlSets(
                    RegistryConstants::TEMP_VM_SYSTEM_HIVE_NAME, controlSets, std::string(RegistryConstants::VALUE_NAME_CURRENT)))
                {
                    errorstream << "Could not get Current ControlSets from Hydration VM system hive" << GetLastError() << std::endl;
                    TRACE_ERROR(errorstream.str().c_str());

                    // Try for Last Known Good Registry Setting.
                    if (ERROR_SUCCESS != RegGetControlSets(
                        RegistryConstants::TEMP_VM_SYSTEM_HIVE_NAME, controlSets, std::string(RegistryConstants::VALUE_NAME_LASTKNOWNGOOD)))
                    {
                        errorstream << "Could not get LKG ControlSets from Hydration VM system hive" << GetLastError() << std::endl;
                        TRACE_ERROR(errorstream.str().c_str());
                        isSuccess = false;
                        break;
                    }
                }

                DWORD preferredControlSet = 0;
                if (ERROR_SUCCESS !=
                    RegGetCurrentControlSetValue(RegistryConstants::VM_SYSTEM_HIVE_NAME, preferredControlSet))
                {
                    errorstream << "Could not get current control set value for Source OS disk." << GetLastError() << std::endl;
                    TRACE_ERROR(errorstream.str().c_str());
                    if (ERROR_SUCCESS !=
                        RegGetLastKnownGoodControlSetValue(RegistryConstants::VM_SYSTEM_HIVE_NAME, preferredControlSet))
                    {
                        errorstream << "Could not get last known good control set value for Source OS disk." << GetLastError() << std::endl;
                        TRACE_ERROR(errorstream.str().c_str());
                        isSuccess = false;
                        break;
                    }
                }

                // Current Control Set name for source OS Disk.
                std::stringstream controlSetNum;
                controlSetNum << RegistryConstants::CONTROL_SET_PREFIX
                    << std::setfill('0')
                    << std::setw(3)
                    << preferredControlSet;

                // WindowsAzureGuestAgent.
                if (!TransferGuestAgentService(
                    (std::string) ServiceNames::AZURE_GUEST_AGENT,
                    controlSets[0],
                    srcOSVol,
                    controlSetNum.str(),
                    imagePath,
                    errorstream))
                {
                    isSuccess = false;
                    break;
                }

                // RdAgent.
                if (!TransferGuestAgentService(
                    (std::string) ServiceNames::AZURE_RDAGENT_SVC,
                    controlSets[0],
                    srcOSVol,
                    controlSetNum.str(),
                    imagePath,
                    errorstream))
                {
                    isSuccess = false;
                    break;
                }

            } while (false);
        }
        catch (const std::exception& exp)
        {
            isSuccess = false;
            errorstream << "Could not transfer Windows Guest Agent registry settings." << std::endl
                << "Error details: " << exp.what();

            TRACE_ERROR(errorstream.str().c_str());
        }

        TRACE_FUNC_END;
        return isSuccess;
    }

    /*
     Method     : TransferGuestAgentService

    Description : Exports Individual Registry settings from hydration vm and imports on to Source disk.

    Parameters  : [in] serviceToTransfer: Services for which Registry settings are to be copied.
                  [in] currentControlSet: CurrentControlSet on Hydration VM.
                  [in] srcOSVolume: Source OS Volume.
                  [in] controlSetNum: Control Set Number on source disk where settings are to be copied.
                  [out] imagePath: Populated with Root folder of the windows guest agent.
                  [out] errorstream: Filled with error details on failure.

    Return      : true on success otherwise false.
    */
    bool TransferGuestAgentService(
        std::string& serviceToTransfer,
        std::string& currentControlSet,
        std::string& srcOSVol,
        std::string& controlSetNum,
        std::string& imagePath,
        std::stringstream& errorstream)
    {
        TRACE_FUNC_BEGIN;
        HKEY hKey = NULL;
        LPVOID lpMsgBuf = NULL;
        bool isSuccess = true;
        try
        {
            do
            {
                // If any one registry setting transfer fails, we can safely assume the operation to fail and can break.
                std::string hydVmSubKey = RegistryConstants::TEMP_VM_SYSTEM_HIVE_NAME +
                    (std::string) DIRECOTRY_SEPERATOR +
                    currentControlSet +
                    RegistryConstants::SERVICES +
                    serviceToTransfer;

                TRACE_INFO("Copying Registry Settings from Hydration VM : %s\n", serviceToTransfer.c_str());

                std::string keySavePath = srcOSVol + serviceToTransfer + ".reg";

                // Delete registry key if it already exists.
                boost::filesystem::path regPath(keySavePath);
                if (boost::filesystem::exists(regPath))
                {
                    boost::filesystem::remove(regPath);
                }

                // Save WinGA Agent and RdAgent Keys from Hydration VM control sets.
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, hydVmSubKey.c_str(), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
                {
                    TRACE("RegOpenKey Done %s\n", hydVmSubKey.c_str());
                    DWORD dwRes = RegSaveKeyEx(hKey, keySavePath.c_str(), NULL, REG_LATEST_FORMAT);

                    if (ERROR_SUCCESS == RegGetStringValue(hKey, RegistryConstants::VALUE_IMAGE_PATH, imagePath))
                    {
                        TRACE_INFO("Image Path: %s\n", imagePath.c_str());
                    }

                    size_t delimiterLocation = imagePath.find("\\WaAppAgent.exe");
                    if (delimiterLocation != std::string::npos)
                    {
                        imagePath = imagePath.substr(0, delimiterLocation);
                        TRACE_INFO("Guest agent folder root path is %s.\n", imagePath.c_str());
                    }
                    else
                    {
                        TRACE_INFO("Couldn't find path to detect guest agent root folder.\n");
                    }

                    if (dwRes != ERROR_SUCCESS)
                    {
                        errorstream << "Transfer Registry Settings failed with " << GetLastError() << std::endl;

                        TRACE_ERROR(errorstream.str().c_str());
                        isSuccess = false;
                        break;
                    }

                    TRACE("RegSaveKey done %s\n", keySavePath.c_str());

                    // Set hive Name for Source OS Disk. 
                    std::string srcDiskControlSetServices = RegistryConstants::VM_SYSTEM_HIVE_NAME +
                        (std::string)DIRECOTRY_SEPERATOR +
                        controlSetNum +
                        RegistryConstants::SERVICES +
                        serviceToTransfer;

                    HKEY hKeySysHive = NULL;
                    // Try open service registry subkey to check if key already present.
                    DWORD serviceRegKey = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        srcDiskControlSetServices.c_str(),
                        0,
                        KEY_READ,
                        &hKeySysHive);

                    if (serviceRegKey != ERROR_SUCCESS)
                    {
                        DWORD disposableDword = 0;
                        // Create registry subkey if RegOpenKeyEx fails.
                        serviceRegKey = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            srcDiskControlSetServices.c_str(),
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL,
                            &hKeySysHive,
                            &disposableDword);
                    }

                    if (serviceRegKey == ERROR_SUCCESS)
                    {
                        TRACE("Successfully opened/created key at %s\n", srcDiskControlSetServices.c_str());
                        serviceRegKey = RegRestoreKey(hKeySysHive, keySavePath.c_str(), REG_FORCE_RESTORE);
                        if (serviceRegKey != ERROR_SUCCESS)
                        {
                            ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                serviceRegKey,
                                0,
                                (LPTSTR)&lpMsgBuf,
                                0,
                                NULL);

                            errorstream << "RegRestoreKey failed with error " << (LPCTSTR)lpMsgBuf << "\n";
                            TRACE_ERROR(errorstream.str().c_str());
                            isSuccess = false;
                            break;
                        }
                        else
                        {
                            errorstream << "Successfully changed registry settings in control set " << controlSetNum << " for service " << serviceToTransfer << "\n";
                            TRACE_INFO(errorstream.str().c_str());
                        }
                    }
                    else
                    {
                        ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwRes,
                            0,
                            (LPTSTR)&lpMsgBuf,
                            0,
                            NULL);

                        errorstream << "Could not open or create Registry subkey " << srcDiskControlSetServices << " Error:" << (LPCSTR)lpMsgBuf << "\n";
                        TRACE_ERROR(errorstream.str().c_str());
                        break;
                    }

                    RegCloseKey(hKey);
                    RegCloseKey(hKeySysHive);
                }
                else
                {
                    errorstream << "Fetching Registry Settings for Windows Guest Agent failed for " << controlSetNum << " with error " << GetLastErrorMsg() << "\n";
                    TRACE_WARNING(errorstream.str().c_str());
                    isSuccess = false;
                    break;
                }
            } while (false);

            LocalFree(lpMsgBuf);
        }
        catch (const std::exception& exp)
        {
            isSuccess = false;
            errorstream << "Could not transfer Guest Agent registry settings." << std::endl
                << "Error details: " << exp.what();

            TRACE_ERROR(errorstream.str().c_str());
        }

        TRACE_FUNC_END;
        return isSuccess;
    }

    /// <summary>
    /// Checks for .NET Framework folder and version inside source OS Volume.
    /// </summary>
    /// <param name="srcOSVol">Source OS Volume.</param>
    /// <returns>true if a valid version 4.0+ is present, false otherwise.</returns>
    bool IsDotNetFxVersionPresent(const std::string& srcOSVol)
    {
        bool isVersionValid = false;
        TRACE_FUNC_BEGIN;

        std::stringstream dotNetFrameWorkPath;
        dotNetFrameWorkPath
            << boost::trim_right_copy_if(srcOSVol, boost::is_any_of(DIRECOTRY_SEPERATOR))
            << SysConstants::DOTNET_FRAMEWORK_PATH;

        std::string dotNetCompVersion = (std::string)SysConstants::WIN_GA_DOTNET_VER;

        if (!DirectoryExists(dotNetFrameWorkPath.str()))
        {
            TRACE_INFO("A Microsoft.Net installation with version 4.0+ is absent on the boot partition.\n");
            TRACE_FUNC_END;
            return isVersionValid;
        }

        namespace fs = boost::filesystem;
        try
        {
            for (
                fs::directory_iterator file(dotNetFrameWorkPath.str());
                file != fs::directory_iterator();
                ++file
                )
            {
                fs::path current(file->path());
                std::string folderName = current.string().substr(dotNetFrameWorkPath.str().length() + 1);

                if (fs::is_directory(current) &&
                    folderName.rfind("v", 0) == 0)
                {
                    // Folder startswith v.
                    std::string dotNetVer = folderName.substr(1);
                    if (boost::iequals(
                        GetHigherReleaseVersion(dotNetVer, dotNetCompVersion), dotNetVer))
                    {
                        TRACE_INFO("A Microsoft.Net installation with version 4.0+ found on the boot partition. Version: %s\n", dotNetVer.c_str());
                        isVersionValid = true;
                        break;
                    }
                    else
                    {
                        TRACE_INFO("Microsoft.NET Version %s is incompatible.\n", dotNetVer.c_str());
                    }
                }
            }
        }
        catch (fs::filesystem_error const& e)
        {
            TRACE_ERROR(".NET Framework version compatibility check failed with exception %s\n", e.what());
        }
        catch (const std::exception& ex)
        {
            TRACE_ERROR(".NET Framework version compatibility check failed with exception. %s\n", ex.what());
        }
        catch (...)
        {
            TRACE_ERROR(".NET Framework version compatibility check failed with an exception.\n");
        }

        TRACE_FUNC_END;
        return isVersionValid;
    }

    /*
    Method      : RegGetStartupScriptKeys

    Description : Identifies the next available startup script number and constructs the registry key paths
    for startup script. It also look for startup script number backedup under SV-Systems registry
    key, if the number is available then it validates its corresponding registry key paths and reuses
    it if its valid.

    Parameters  : [in] winVer: OSVersion structure representing the OS version of the registry where the statupscript
    will get added.
    [out] key1, key2: Startup key paths.
    [out] scriptNumber: Sequence number under statupscripts registry key.
    [out] errorMsg: Filled with error details on failure.

    Return      : true on success, otherwise false.
    */
    bool RegGetStartupScriptKeys(const OSVersion& winVer,
        std::string& key1, std::string& key2,
        DWORD& scriptNumber,
        std::stringstream& errorSteam)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        std::string strBootScriptKey1Prefix, strBootScriptKey2Prefix;
        strBootScriptKey1Prefix = RegistryConstants::VM_SOFTWARE_HIVE_NAME + (
            winVer > OSVersion(5, 2) ? // 5.2 --> Windows Server 2003
            std::string(RegistryConstants::BOOTUP_SCRIPT_KEY1_2K8ABOVE) :
            std::string(RegistryConstants::BOOTUP_SCRIPT_KEY1_2K8BELOW));

        strBootScriptKey2Prefix = RegistryConstants::VM_SOFTWARE_HIVE_NAME +
            std::string(RegistryConstants::BOOTUP_SCRIPT_KEY2);

        try
        {
            //
            // Check startup script number under sv-systems key, if not found then look for next available number.
            // If the startup script number found under sv-systems key then verify the script name under startup script number key
            // is matching with failover boot-up script name. If it is not matching then the script number might be stale and invalid
            // and go for next available script number, if it is matching then reuse the same script number.
            //
            LONG lReturn = RegGetSVSystemDWORDValue(RegistryConstants::VALUE_NAME_BOOTUP_SCRIPT_ORDER_NUMBER, scriptNumber);
            if (VERIFY_REG_STATUS(lReturn, "Could not read startup script order number"))
            {
                TRACE_INFO("Starup script number %lu found under SV-Systems key.\n", scriptNumber);

                std::stringstream scriptKey1, scriptKey2;
                scriptKey1 << strBootScriptKey1Prefix << scriptNumber;
                scriptKey2 << strBootScriptKey2Prefix << scriptNumber;

                std::string scriptNameFromKey1, scriptNameFromKey2;

                if (RegKeyExists(HKEY_LOCAL_MACHINE, scriptKey1.str()))
                {

                    CRegKey startupScriptKey;
                    lReturn = startupScriptKey.Open(HKEY_LOCAL_MACHINE, (scriptKey1.str() + "\\0").c_str());

                    if (VERIFY_REG_STATUS(lReturn, "Error opening " + scriptKey1.str()) &&
                        VERIFY_REG_STATUS(RegGetStringValue(startupScriptKey.m_hKey,
                            RegistryConstants::VALUE_NAME_SCRIPT, scriptNameFromKey1),
                            "Error accessing registry value Script under " + scriptKey1.str()))
                    {
                        TRACE_INFO("Script Name under %s key is %s\n",
                            scriptKey1.str().c_str(),
                            scriptNameFromKey1.c_str());
                    }
                }

                if (RegKeyExists(HKEY_LOCAL_MACHINE, scriptKey2.str()))
                {
                    CRegKey startupScriptKey;
                    lReturn = startupScriptKey.Open(HKEY_LOCAL_MACHINE, (scriptKey2.str() + "\\0").c_str());

                    if (VERIFY_REG_STATUS(lReturn, "Error opening " + scriptKey2.str()) &&
                        VERIFY_REG_STATUS(RegGetStringValue(startupScriptKey.m_hKey,
                            RegistryConstants::VALUE_NAME_SCRIPT, scriptNameFromKey2),
                            "Error accessing registry value Script under " + scriptKey2.str()))
                    {
                        TRACE_INFO("Script Name under %s key is %s\n",
                            scriptKey2.str().c_str(),
                            scriptNameFromKey2.c_str());
                    }
                }

                if ((scriptNameFromKey1.empty() || boost::iends_with(scriptNameFromKey1, STARTUP_EXE_NAME)) &&
                    (scriptNameFromKey2.empty() || boost::iends_with(scriptNameFromKey2, STARTUP_EXE_NAME)))
                {
                    TRACE_INFO("Reusing the existing startup script number %lu.\n", scriptNumber);
                    key1 = scriptKey1.str();
                    key2 = scriptKey2.str();

                    TRACE_FUNC_END;
                    return bSuccess;
                }
                else if (boost::iends_with(scriptNameFromKey1, STARTUP_EXE_NAME))
                {
                    // One of the key is with STARTUP_EXE_NAME and other is with different name so clear the script value which has STARTUP_EXE_NAME.
                    CRegKey startupScriptKey;
                    lReturn = startupScriptKey.Open(HKEY_LOCAL_MACHINE, (scriptKey1.str() + "\\0").c_str());
                    VERIFY_REG_STATUS(lReturn, "Error clearing miss configured startup script key");
                    VERIFY_REG_STATUS(startupScriptKey.SetStringValue(RegistryConstants::VALUE_NAME_SCRIPT, ""),
                        "Error clearing miss configured startup script name");

                    TRACE_INFO("%s is miss configured. Finding out next available number.\n", scriptKey1.str().c_str());
                }
                else if (boost::iends_with(scriptNameFromKey2, STARTUP_EXE_NAME))
                {
                    // One of the key is with STARTUP_EXE_NAME and other is with different name so clear the script value which has STARTUP_EXE_NAME.
                    CRegKey startupScriptKey;
                    lReturn = startupScriptKey.Open(HKEY_LOCAL_MACHINE, (scriptKey2.str() + "\\0").c_str());
                    VERIFY_REG_STATUS(lReturn, "Error clearing miss configured startup script key");
                    VERIFY_REG_STATUS(startupScriptKey.SetStringValue(RegistryConstants::VALUE_NAME_SCRIPT, ""),
                        "Error clearing miss configured startup script name");

                    TRACE_INFO("%s is miss configured. Finding out next available number.\n", scriptKey2.str().c_str());
                }
                else
                {
                    TRACE_INFO("This number seems to be stale entry under SV-Systems key. Finding out next available number.\n");
                }
            }

            //
            // If script number is not found under SV-Systems then findout next available number.
            //
            bool bNextAvailableKeyFound = false;
            for (int scriptOrderNum = 0;
                scriptOrderNum < RegistryConstants::MAX_STARTUP_SCRIPT_SEARCH_LIMIT;
                scriptOrderNum++)
            {
                std::stringstream scriptKey1, scriptKey2;
                scriptKey1 << strBootScriptKey1Prefix << scriptOrderNum;
                scriptKey2 << strBootScriptKey2Prefix << scriptOrderNum;

                if (!RegKeyExists(HKEY_LOCAL_MACHINE, scriptKey1.str()) &&
                    !RegKeyExists(HKEY_LOCAL_MACHINE, scriptKey2.str()))
                {
                    TRACE_INFO("Available next script order number is %d\n", scriptOrderNum);

                    key1 = scriptKey1.str();
                    key2 = scriptKey2.str();
                    scriptNumber = scriptOrderNum;

                    bNextAvailableKeyFound = true;
                    break;
                }
            }

            if (!bNextAvailableKeyFound)
                THROW_RECVRY_EXCEPTION("Reached max lookup limit for determining next available script order number.\n");
        }
        catch (const std::exception& exp)
        {
            errorSteam << "Could not determine the startup script order number." << std::endl
                << "Error details: " << exp.what();

            bSuccess = false;
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : SetBootupScriptEx

    Description : Sets the bootup script related registry changes on recovering vm system hive.

    Parameters  : [out] errorMsg: Filled with error details on failure.

    Return      : true on success, otherwise false.

    */
    bool SetBootupScriptEx(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;
        std::stringstream errorSteam;

        try
        {
            SetBootupScriptOptions();
        }
        catch (const std::exception& exp)
        {
            errorSteam << "Could not set startup script options." << std::endl
                << "Error details: " << exp.what();

            errorMsg = errorSteam.str();
            bSuccess = false;
        }
        catch (...)
        {
            errorSteam << "Could not set startup script options." << std::endl
                << "Error details: Unknown exception";

            errorMsg = errorSteam.str();
            bSuccess = false;
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : GetOsVersion

    Description : Get OS Version from recovering vm system hive.

    Parameters  : [out] winVer: Filled with OS Version Info.

    Return      : true on success, otherwise false.

    */
    bool GetOsVersion(OSVersion& winVer)
    {
        std::string OsVersion;
        std::stringstream errorSteam;

        if (ERROR_SUCCESS !=
            RegGetOSCurrentVersion(RegistryConstants::VM_SOFTWARE_HIVE_NAME, OsVersion))
        {
            // Logging is done as part of above call. So no need to log.
            return false;
        }

        try
        {
            winVer = OSVersion::FromString(OsVersion);

            // Set OS Details in RecoveryStatus metadata for recovery scenario.
            RecoveryStatus::Instance().SetSourceOSDetails(OsVersion);
        }
        catch (const std::exception& e)
        {
            // Logging is done as part of FromString call. So no need to log.
            return false;
        }
        return true;
    }

    /*
    Method      : SetBootupScript

    Description : Sets the bootup scripts for recovering machine by editing its registry entries.
    The script will be executed on booting recovered machine on azure as Azure VM instance.
    The script will validate assigned drive letters, offline disks etc.

    Parameters  : [out] errorMsg: Filled with error details on failure.

    Return      : true on success, otherwise false.

    */
    bool SetBootupScript(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;

        std::stringstream errorStream;

        do
        {
            std::string srcSystemVol = HostInfoConfig::Instance().OSDrive();
            if (srcSystemVol.empty())
            {
                //
                //If Host info does not have source Os drive name then assume it as C:\
                            //
                srcSystemVol = SOURCE_OS_MOUNT_POINT_ROOT;
            }
            if (srcSystemVol[srcSystemVol.length() - 1] != '\\') srcSystemVol += "\\";

            std::string OsVersion;
            if (ERROR_SUCCESS !=
                RegGetOSCurrentVersion(RegistryConstants::VM_SOFTWARE_HIVE_NAME, OsVersion))
            {
                errorStream << "Could not get the OS version from registry.";

                bSuccess = false;
                break;
            }

            OSVersion winVer;
            try
            {
                winVer = OSVersion::FromString(OsVersion);
            }
            catch (const std::exception& e)
            {
                errorStream << "Exception with OS version string. Exception " << e.what();

                bSuccess = false;
                break;
            }

            //
            // Prepare Startup script command
            //
            std::string vxInstallPath, scriptCmd, scriptParams, newHostId;
            if (ERROR_SUCCESS !=
                RegGetAgentInstallPath(RegistryConstants::VM_SOFTWARE_HIVE_NAME, vxInstallPath))
            {
                errorStream << "Could not get the Agent installation path from registry.";
                bSuccess = false;
                break;
            }
            else if (vxInstallPath.empty())
            {
                errorStream << "VX Install Path value is empty in registry.\n";
                bSuccess = false;
                break;
            }
            if (vxInstallPath[vxInstallPath.length() - 1] != '\\') vxInstallPath += "\\";

            scriptCmd = vxInstallPath + STARTUP_EXE_NAME;

            scriptParams = std::string("-atbooting -resetguid -cloudenv Azure -file \"") +
                vxInstallPath + NW_CHANGES_FILE_PATH + std::string("\"");

            newHostId = AzureRecoveryConfig::Instance().GetNewHostId();
            if (!newHostId.empty())
                scriptParams += std::string(" -newhostid ") + newHostId;

            if (AzureRecoveryConfig::Instance().IsTestFailover())
                scriptParams += std::string(" -testfailover");

            TRACE_INFO("Bootup Script Optins: %s\n", scriptParams.c_str());

            //
            // Get the startup script keys, and script order number.
            //
            DWORD dwScriptNumber = 0;
            std::string bootScriptKey1, bootScriptKey2;
            if (!RegGetStartupScriptKeys(winVer, bootScriptKey1, bootScriptKey2, dwScriptNumber, errorStream))
            {
                bSuccess = false;
                break;
            }

            //
            // Store the script order number under SV-Systems key so that the boot-up script will use this number to
            // cleanup the corresponding registry entries after execution.
            //
            if (ERROR_SUCCESS !=
                RegSetSVSystemDWORDValue(RegistryConstants::VALUE_NAME_BOOTUP_SCRIPT_ORDER_NUMBER, dwScriptNumber))
            {
                errorStream << "Could not store bootup script order number under SV-System registry key."
                    << GetLastErrorMsg();
                bSuccess = false;
                break;
            }

            //
            // Add the bootupscript entries to the registry
            //
            if (ERROR_SUCCESS !=
                RegAddBootUpScript(bootScriptKey1, scriptCmd, scriptParams, srcSystemVol, winVer, false) ||

                ERROR_SUCCESS !=
                RegAddBootUpScript(bootScriptKey2, scriptCmd, scriptParams, srcSystemVol, winVer, false))
            {
                errorStream << "Could not add bootup script registry entries to the source."
                    << GetLastErrorMsg();
                bSuccess = false;
                break;
            }

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : SetBootupScriptOptions

    Description : Sets the bootup script options for recovering machine by editing its registry entries.
    These script options will be used by mobility agents on booting the recovered machine
    on Azure as Azure VM instance.

    Parameters  :

    Return      : Throws an exception on failures, otherwise none.

    */
    void SetBootupScriptOptions()
    {
        TRACE_FUNC_BEGIN;

        DWORD dwTestFailover = AzureRecoveryConfig::Instance().IsTestFailover() ? 1 : 0;
        DWORD dwEnableRDP = AzureRecoveryConfig::Instance().EnableRDP() ? 1 : 0;
        std::string newHostId = AzureRecoveryConfig::Instance().GetNewHostId();
        BOOST_ASSERT(!newHostId.empty());

        TRACE_INFO("Bootup script options: New HostId = %s, TestFailover=%lu\n",
            newHostId.c_str(), dwTestFailover);

        VERIFY_REG_STATUS(RegSetSVSystemStringValue(
            RegistryConstants::VALUE_NAME_RECOVERED_ENV, RegistryConstants::VALUE_DATA_RECOVERED_ENV_AZURE),
            "Bootup script options: Could not set Recovered Env value");

        VERIFY_REG_STATUS(RegSetSVSystemDWORDValue(RegistryConstants::VALUE_NAME_RECOVERY_INPROGRESS, 1),
            "Bootup script options: Could not set Recovery InProgress value");

        VERIFY_REG_STATUS(RegSetSVSystemStringValue(RegistryConstants::VALUE_NAME_NEW_HOSTID, newHostId),
            "Bootup script options: Could not set New HostId value");

        VERIFY_REG_STATUS(RegSetSVSystemDWORDValue(RegistryConstants::VALUE_NAME_TEST_FAILOVER, dwTestFailover),
            "Bootup script options: Could not set Test Failover value");

        VERIFY_REG_STATUS(RegSetSVSystemDWORDValue(RegistryConstants::VALUE_NAME_ENABLE_RDP, dwEnableRDP),
            "Bootup script options: Could not set Enable RDP value");

        TRACE_FUNC_END;
    }

    /*
    Method      : PrepareSourceSystemPath

    Description : Identifies the source OS installed partition and its current mountpoint/volume
    on attached source os disk and then constructs the system path based on the
    source discovery information retrieved from source(host info xml file)

    Parameters  : [out] srcSystemPath : holds the identified source system path on success, else empty.

    Return      : true if source system path is constructed successfuly, otherwise false.

    */
    bool PrepareSourceSystemPath(std::string& srcSystemPath, std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;

        std::stringstream errorStream;

        do
        {
            //
            // Get OSInstallPath, OSDrive and then construct actual system path on mounted location.
            //
            std::string OsInstallPath = HostInfoConfig::Instance().OSInstallPath();
            if (OsInstallPath.empty())
            {
                errorStream << "OS install path was missing or empty in source host information";

                break;
            }

            std::string OSDrive = HostInfoConfig::Instance().OSDrive();
            if (OSDrive.empty())
            {
                errorStream << "OS Drive was missing or empty in source host information";
                break;
            }

            std::string diskId;
            if (AzureRecoveryConfig::Instance().IsUEFI())
                diskId = AzureRecoveryConfig::Instance().GetDiskSignature();

            std::string osVolMountPath;
            if (!PrepareSourceOSDrive(osVolMountPath, diskId))
            {
                errorStream << "Could not mount OS volume. "
                    << GetLastErrorMsg();
                break;
            }

            srcSystemPath = GetSourceOSInstallPath(OsInstallPath,
                OSDrive,
                osVolMountPath);
            if (srcSystemPath.empty())
            {
                errorStream << "Could not find Source OS install path on Hydration VM. "
                    << GetLastErrorMsg();
                break;
            }

            // Erase the trailing slash
            if (srcSystemPath[srcSystemPath.length() - 1] == '\\')
                srcSystemPath.erase(srcSystemPath.length() - 1);

            bSuccess = true;

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : UpdateBIOSBootRecordsOnOSDisk

    Description : Updates the BIOS boot records on OS disk active partition.

    Parameters  : [in] srcSystemPath : source system32 path.
    [in] activePartitionDrivePath: active partition drive path.
    [in] firmwareType: Firmware type of which bcd is to be updated.
    [out] errorMsg : Holds error message on failures.

    Return      : true if boot records are updated successfuly on source OS disk, otherwise false.

    */
    bool UpdateBIOSBootRecordsOnOSDisk(const std::string& srcSystemPath,
        const std::string& activePartitionDrivePath,
        const std::string& firmwareType,
        std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;

        std::stringstream errorStream;

        do
        {
            // Logs the original EFI BCD entries.
            RunBcdEdit(srcSystemPath, activePartitionDrivePath, BCD_TOOLS::EFI_BCD_PATH);

            // Run BcdBoot to update the BIOS and EFI BCD.
            if (ERROR_SUCCESS != RunBcdBootAndUpdateBCD(srcSystemPath, activePartitionDrivePath, firmwareType))
            {
                errorStream << "Failed to update BIOS boot records. "
                    << GetLastErrorMsg();

                break;
            }

            // Log the newly added BIOS BCD entries.
            RunBcdEdit(srcSystemPath, activePartitionDrivePath, BCD_TOOLS::BIOS_BCD_PATH);

            bSuccess = true;

        } while (false);

        if (!bSuccess)
        {
            errorMsg = errorStream.str();

            TRACE_ERROR("%s\n", errorMsg.c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    /*
    Method      : VerifyDisks

    Description : This function first verifies all the required disks are visible to system.
    Then it looks for the offline disks and tries to bring those disks online.
    It also verifies the offline reason before bringing the disk online.
    It only considers basic disks for online.

    Parameters  : [out] errorMsg : contains a reason in text form for not bring any offline disk,
    otherwise empty.

    Return      : true if disks verifcation was successful, otherwise false.

    */
    bool VerifyDisks(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = true;
        bool bAllDisksDiscovered = false;
        int nMaxRetry = 5;

        AzureRecovery::disk_lun_map diskLunMap;
        AzureRecoveryConfig::Instance().GetDiskMap(diskLunMap);

        do
        {
            bAllDisksDiscovered = true;
            std::map<UINT, std::string> lunDeviceId;
            DWORD dwRet = GetLunDeviceIdMappingOfDataDisks(lunDeviceId,
                AzureRecoveryConfig::Instance().GetScsiHostNum());
            if (ERROR_SUCCESS != dwRet)
            {
                bSuccess = false;
                errorMsg = "Error discovering disks";
                TRACE_ERROR("%s\n", errorMsg.c_str());
                break;
            }

            for each (auto diskEntry in diskLunMap)
            {
                if (lunDeviceId.find(diskEntry.second) == lunDeviceId.end())
                {
                    bAllDisksDiscovered = false;
                    TRACE_WARNING("Disk at Lun %d is not visible to system yet.\n",
                        diskEntry.second);

                    if (--nMaxRetry > 0)
                    {
                        TRACE_INFO("Retrying the disk discovery");
                        ACE_OS::sleep(30);
                    }
                    else
                    {
                        std::stringstream msgStream;
                        msgStream << "Required data disk is missing at LUN : " << diskEntry.second;

                        errorMsg = msgStream.str();

                        TRACE_ERROR("%s\n", errorMsg.c_str());
                    }

                    break;
                }
                else
                {
                    TRACE("Disk at Lun %d is discovered.\n", diskEntry.second);
                }
            }

            if (!bAllDisksDiscovered)
            {
                if (nMaxRetry > 0)
                {
                    continue;        // Retry disks discovery
                }
                else
                {
                    bSuccess = false;
                    break;			// Reached max retry attempts.
                }
            }

            //
            // Bring online source os disk
            //
            disk_extents_t os_vol_disk_extents;
            if (!HostInfoConfig::Instance().GetOsDiskDriveExtents(os_vol_disk_extents) ||
                os_vol_disk_extents.empty()
                )
            {
                errorMsg = "Cloud not get source OS volume disk drive extents or the extents information is missing in host information";
                TRACE_ERROR("%s", errorMsg.c_str());
                bSuccess = false;
                break;
            }

            std::string os_disk_id = AzureRecoveryConfig::Instance().IsUEFI() ?
                AzureRecoveryConfig::Instance().GetDiskSignature() : os_vol_disk_extents[0].disk_id;

            DWORD dwOfflineReason = VerifyOfflineDisksAndBringOnline(os_disk_id);
            if (ERROR_SUCCESS != dwOfflineReason)
            {
                bSuccess = false;

                switch (dwOfflineReason)
                {
                case Disk_Offline_Reason_RedundantPath:
                    errorMsg = "The disk is used for multi-path I/O.";
                    break;
                case Disk_Offline_Reason_Snapshot:
                    errorMsg = "The disk is a snapshot disk.";
                    break;
                case Disk_Offline_Reason_Collision:
                    errorMsg = "There was a signature or identifier collision with another disk.";
                    break;
                case Disk_Offline_Reason_ResourceExhaustion:
                    errorMsg = "There were insufficient resources to bring the disk online.";
                    break;
                case Disk_Offline_Reason_CriticalWriteFailures:
                    errorMsg = "There were critical write failures on the disk.";
                    break;
                case Disk_Offline_Reason_DataIntegrityScanRequired:
                    errorMsg = "A data integrity scan is required.";
                    break;

                default:
                    errorMsg = "An internal error occured while discovering or bringing disks online";
                    break;
                }

                TRACE_ERROR("Error verifying disks. Error : %s\n", errorMsg.c_str());
            }
        } while (!bAllDisksDiscovered);

        TRACE_FUNC_END;
        return bSuccess;
    }

    // Method: PrepareActivePartitionDrive
    // 
    //  Description : Identifies the source active partition on hydration-vm using source active
    //                partition volume drive extents information, also finds its drive path.
    // 
    //  Parameters : [out] drivePath : Filled with source active patition volume drive path on success.
    //               [out] errorMsg: Filled with error message on failure.
    //
    //  Return : true on success, otherwise false.
    bool PrepareActivePartitionDrive(std::string& drivePath, std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bContinueLoop, bSuccess = false;
        int nMaxRetry = 3;

        do
        {
            bContinueLoop = false;

            std::string activePartitionDrivePath;
            std::string disk_id = AzureRecoveryConfig::Instance().GetDiskSignature();
            long long partitionOffset = AzureRecoveryConfig::Instance()
                .GetActivePartitionStartingOffset();

            AzureRecovery::disk_extent extent(disk_id, partitionOffset, 0);
            disk_extents_t activePartitionVolDiskExtents;
            activePartitionVolDiskExtents.push_back(extent);

            std::string srcOSVolGuid;
            DWORD dwRet = GetVolumeFromDiskExtents(activePartitionVolDiskExtents, srcOSVolGuid);
            if (ERROR_FILE_NOT_FOUND == dwRet &&
                nMaxRetry-- > 0)
            {
                TRACE_WARNING("ActivePartition volume is not visible yet. Retrying the volume enumeration ...\n");
                bContinueLoop = true;

                //Sleep for 20sec before retry
                ACE_OS::sleep(20);

                continue;
            }
            else if (ERROR_SUCCESS != dwRet)
            {
                TRACE_ERROR("ActivePartition volume not found\n");
                bSuccess = false;
                break;
            }

            std::list<std::string> mountPoints;
            if (ERROR_SUCCESS != GetVolumeMountPoints(srcOSVolGuid, mountPoints))
            {
                TRACE_ERROR("Could not get mount points for ActivePartition volume.\n");
                bSuccess = false;
                break;
            }

            drivePath.clear();
            auto mountpointIter = mountPoints.begin();
            for (; mountpointIter != mountPoints.end(); mountpointIter++)
            {
                TRACE_INFO("Checking path %s for active partition drive letter.\n",
                    mountpointIter->c_str());

                if (boost::trim_copy_if(*mountpointIter, boost::is_any_of("\\")).length() == 2)
                {
                    // Drive letter found.
                    drivePath = *mountpointIter;
                    break;
                }
            }

            if (drivePath.empty())
            {
                //TODO: Instead of failing the operation, assign a drive letter.
                TRACE_ERROR("Could not find drive letter for ActivePartition volume.\n");

                SetLastErrorMsg("Could not find drive letter for active partition");
            }
            else
            {
                bSuccess = true;
            }

        } while (bContinueLoop);

        TRACE_FUNC_END;
        return bSuccess;
    }

    // Method: OnlineDisks
    // 
    // Description : Online any offline disk.
    // 
    // Parameters : [out] errorMsg: Filled with error message on failure.
    // 
    // Return : true on success, otherwise false.
    bool OnlineDisks(std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bRetry = false;
        int nMaxRetry = 5;
        DWORD dwOfflineReason = ERROR_SUCCESS;

        do
        {
            bRetry = false;
            dwOfflineReason = VerifyOfflineDisksAndBringOnline();
            if (ERROR_SUCCESS != dwOfflineReason)
            {
                switch (dwOfflineReason)
                {
                case Disk_Offline_Reason_RedundantPath:
                    errorMsg = "The disk is used for multi-path I/O.";
                    break;
                case Disk_Offline_Reason_Snapshot:
                    errorMsg = "The disk is a snapshot disk.";
                    break;
                case Disk_Offline_Reason_Collision:
                    errorMsg = "There was a signature or identifier collision with another disk.";
                    break;
                case Disk_Offline_Reason_ResourceExhaustion:
                    errorMsg = "There were insufficient resources to bring the disk online.";
                    break;
                case Disk_Offline_Reason_CriticalWriteFailures:
                    errorMsg = "There were critical write failures on the disk.";
                    break;
                case Disk_Offline_Reason_DataIntegrityScanRequired:
                    errorMsg = "A data integrity scan is required.";
                    break;

                default:
                    errorMsg = "An internal error occured while discovering or bringing disks online";
                    bRetry = --nMaxRetry > 0;
                    break;
                }

                TRACE_ERROR("Error verifying disks. Error : %s\n", errorMsg.c_str());
            }
        } while (bRetry);

        TRACE_FUNC_END;
        return dwOfflineReason == ERROR_SUCCESS;
    }

    // Method: DiscoverSourceOSVolumes
    // 
    // Description : Discovers source OS volumes visible to the system.
    // 
    // Parameters : [out] osVolumes
    //              [out] errorMsg
    // 
    // Return : true on success, otherwise false.
    bool DiscoverSourceOSVolumes(
        std::list<std::string>& osVolumes,
        std::string& errorMsg)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;
        int nMaxRetry = 5;

        std::string exclude_disk_id;
        if (ERROR_SUCCESS != GetSystemDiskId(exclude_disk_id))
        {
            errorMsg = "Could not get current system disk id.";

            TRACE_FUNC_END;
            return bSuccess;
        }
        BOOST_ASSERT(!exclude_disk_id.empty());

        do
        {
            osVolumes.clear();
            DWORD dwRet = FindVolumesWithPath(
                SysConstants::SYSTEM_HIVE_FULL_PATH,
                osVolumes,
                exclude_disk_id);

            if (dwRet != ERROR_SUCCESS ||
                osVolumes.size() == 0)
            {
                if (--nMaxRetry > 0)
                {
                    TRACE_WARNING("OS volume not found. Retry will happen after 20sec.\n");
                    ACE_OS::sleep(20);
                }
                else
                {
                    TRACE_ERROR("OS volume not found.");

                    errorMsg = "Could not find OS Volume.";
                    break;
                }
            }
            else
            {
                bSuccess = true;

                if (osVolumes.size() > 1)
                    TRACE_WARNING("Found multiple volumes with OS installation.\n");
                else
                    TRACE_INFO("Found OS volume.\n");
            }
        } while (!bSuccess);

        TRACE_FUNC_END;
        return bSuccess;
    }

    // Method: MakeRegistryChangesForMigration
    // 
    // Description : Make necessary changes in registry hives for migration.
    // 
    // Parameters : [in] osInstallPath
    //              [in] srcOsVol
    //              [out] retcode
    //              [out] curTaskDesc
    //              [out] errStream
    //              [in] disableAutomount
    // 
    // Return : true on success, otherwise false.
    bool MakeRegistryChangesForMigration(
        const std::string& osInstallPath,
        const std::string& srcOsVol,
        int& retcode,
        std::string& curTaskDesc,
        std::stringstream& errStream,
        bool disableAutomount)
    {
        TRACE_FUNC_BEGIN;
        bool bSuccess = false;
        std::string errorMessage;

        do
        {
            if (!PrepareSourceRegistryHives(osInstallPath, errorMessage, HiveFlags::System))
            {
                retcode = E_RECOVERY_PREP_REG;

                errStream << "Could not prepare the source registry.( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                break;
            }

            curTaskDesc = TASK_DESCRIPTIONS::CHANGE_BOOT_CONFIG;
            if (!MakeChangesForBootDriversOnRegistry(osInstallPath, errorMessage, retcode))
            {
                errStream << "Could not update required boot drivers on registry.( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                break;
            }

            curTaskDesc = TASK_DESCRIPTIONS::CHANGE_SVC_NW_CONFIG;
            if (!MakeChangesForServicesOnRegistry(osInstallPath, errorMessage))
            {
                retcode = E_RECOVERY_SVC_CONFIG;

                errStream << "Could not update required services on registry.( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                break;
            }

            DWORD dwSanPolicy = VDS_SAN_POLICY::VDS_SP_ONLINE;
            if (!SetSANPolicy(errorMessage, dwSanPolicy, false))
            {
                retcode = E_RECOVERY_SVC_CONFIG;

                errStream << "Could not update SanPolicy on registry.( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                break;
            }

            if (disableAutomount && !SetAutoMount(errorMessage, disableAutomount))
            {
                retcode = E_RECOVERY_SVC_CONFIG;

                errStream << "Could disable auto mount.( "
                    << errorMessage
                    << " )";

                TRACE_ERROR("%s\n", errStream.str().c_str());

                break;
            }

            // Mark Success here to not fail the call if further calls fail.
            bSuccess = true;
            if (boost::iequals(GetHydrationConfigValue(
                GetHydrationConfigSettings(), HydrationConfig::EnableWindowsGAInstallation), "true"))
            {
                std::stringstream wingaErrSteam;
                if (VerifyRegistrySettingsForWinGA(srcOsVol, wingaErrSteam))
                {
                    // Let customer know that Agent installation was skipped.
                    if (retcode == E_RECOVERY_SUCCESS)
                    {
                        retcode = E_RECOVERY_GUEST_AGENT_ALREADY_PRESENT;
                    }

                    errStream << "Guest agent installation was skipped as the VM already has a Guest Agent present. ";
                    TRACE_WARNING("%s\n", errStream.str().c_str());

                    break;
                }
                else
                {
                    if (!AddWindowsGuestAgent(srcOsVol, wingaErrSteam))
                    {
                        // Log error code and do not fail.
                        if (retcode == E_RECOVERY_SUCCESS)
                        {
                            retcode = E_RECOVERY_GUEST_AGENT_INSTALLATION_FAILED;
                        }

                        errStream
                            << "VM was successfully migrated but Guest Agent installation has failed"
                            << ". Please manually install the guest agent on migrated VM. ";

                        TRACE_WARNING("%s\n", errStream.str().c_str());

                        break;
                    }
                }
            }
            else
            {
                TRACE_INFO("Windows guest agent installation config has been turned off.\n");
            }

        } while (false);

        curTaskDesc = TASK_DESCRIPTIONS::UNMOUNT_SYSTEM_PARTITIONS;
        if (!ReleaseSourceRegistryHives(errorMessage, HiveFlags::System))
        {
            // If any error occured before then retain that error code.
            if (retcode == E_RECOVERY_SUCCESS)
            {
                retcode = E_RECOVERY_CLEANUP;
            }

            errStream << "Could not unload source registry hives loaded for data massaging.( "
                << errorMessage
                << " )";

            TRACE_ERROR("%s\n", errStream.str().c_str());
        }

        TRACE_FUNC_END;
        return bSuccess;
    }

    // Method: SetOSVersionDetails
    // 
    // Description : Retrieves the file version of ntfs.sys file and sets
    //               it as OS version. Actual OS version will be available
    //               in SYSTEM hive, but we don't want to load it just for
    //               os version info, hence relaying on ntfs file version.
    // Parameters : [in] osInstallPath
    // 
    // Return : true on success, otherwise false.
    void SetOSVersionDetails(const std::string& osInstallPath)
    {
        TRACE_FUNC_BEGIN;

        std::string version;
        std::string ntfs_driver_file = osInstallPath + "\\drivers\\ntfs.sys";
        if (GetFileVersion(ntfs_driver_file, version) == ERROR_SUCCESS)
        {
            boost::trim(version);

            TRACE_INFO("Source OS ntfs file version: %s.\n", version.c_str());
            RecoveryStatus::Instance().SetSourceOSDetails(version);
        }
        else
        {
            TRACE_ERROR("Could not get Source OS ntfs file version.\n");
        }

        TRACE_FUNC_END;
    }
}
