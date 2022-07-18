/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File        :   RecoveryHelpers.cpp

Description :   Linux Recovery helper function declarations

History     :   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include <ace/Init_ACE.h>
#include <boost/foreach.hpp>
#include <boost/assert.hpp>
#include <sstream>

#include "RecoveryHelpers.h"
#include "LinuxUtils.h"
#include "../AzureRecovery.h"
#include "../common/utils.h"
#include "../common/Trace.h"
#include "../resthelper/HttpClient.h"
#include "../config/HostInfoDefs.h"
#include "../config/HostInfoConfig.h"
#include "../config/RecoveryConfig.h"

/*
Method      : GlobalInit

Description : Global initialization of the library. This function should be called 
              before using any of the helper functions in this library.

Parameters  : None

Return      : None

*/
int GlobalInit()
{
    if (-1 == ACE::init())
    {
        std::cerr << "ACE Init failed" << std::endl;
        return 1;
    }

    AzureStorageRest::HttpClient::GlobalInitialize();

    return 0;
}

/*
Method      : GlobalUnInit

Description : Global un-initialization. No helper function should be called after this function call.

Parameters  : None

Return      : None

*/
void GlobalUnInit()
{
    AzureStorageRest::HttpClient::GlobalUnInitialize();
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
    std::string errorMessage;
    std::string curTaskDesc;
    do
    {
        std::map<std::string, std::string> mapSrcTgtDevices;
        std::map<std::string, MountPointInfo> srcSysMountInfo;
        
        curTaskDesc = TASK_DESCRIPTIONS::PREPARE_DISKS;
        if(!VerifyDisks(mapSrcTgtDevices , errorMessage))
        {
            retcode = E_RECOVERY_DISK_NOT_FOUND;

            errStream << "Error verifying disks. ( " << errorMessage << " )";
            TRACE_ERROR("%s\n", errStream.str().c_str());
            
            break;
        }
        
        curTaskDesc = TASK_DESCRIPTIONS::MOUNT_SYSTEM_PARTITIONS;
        if( !PrepareSourceSysPartitions(mapSrcTgtDevices, srcSysMountInfo, errorMessage) )
        {
            retcode = E_RECOVERY_VOL_NOT_FOUND;

            errStream << "Could not prepare Source System partitions on Hydration VM.";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::CHANGE_BOOT_CONFIG;
        if(!PerformPreRecoveryChanges(mapSrcTgtDevices,srcSysMountInfo, errorMessage))
        {
            retcode = E_RECOVERY_BOOT_CONFIG;

            errStream << "Could not modify source system configuration.";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }
        
        curTaskDesc = TASK_DESCRIPTIONS::UNMOUNT_SYSTEM_PARTITIONS;
        if(!CleanupMountPoints(srcSysMountInfo, errorMessage))
        {
            retcode = E_RECOVERY_CLEANUP;
            errStream << "Could not clean-up source system partitions mount points.";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }
        
        if(!FlushDevicesAndRevertLvmConfig(mapSrcTgtDevices, errorMessage))
        {
            retcode = E_RECOVERY_CLEANUP;
            errStream << "Could not flush the block devices.";

            TRACE_ERROR("%s\n", errStream.str().c_str());

            break;
        }
        curTaskDesc = TASK_DESCRIPTIONS::UPLOAD_LOG;
        
    } while(false);

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
    TRACE_FUNC_BEGIN;
    int retcode = 0;
    namespace AR = AzureRecovery;

    std::stringstream errStream;
    std::string curTaskDesc;

    do
    {
        curTaskDesc = TASK_DESCRIPTIONS::DISCOVER_OS_VOLS;
        if (!AR::DiscoverSourceSystemPartitions())
        {
            errStream << "Error discovering source system partitions. "
                << RecoveryStatus::Instance().GetLastErrorMessge();

            retcode = E_RECOVERY_VOL_NOT_FOUND;
            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::MOUNT_SYSTEM_PARTITIONS;
        if (!AR::MountSourceSystemPartitions())
        {
            errStream << "Error mounting source system partitions. "
                << RecoveryStatus::Instance().GetLastErrorMessge();
            retcode = E_RECOVERY_COULD_NOT_MOUNT_SYS_VOL;
            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::CHANGE_BOOT_NW_CONFIG;
        if (!AR::PrepareSourceOSForAzure())
        {
            errStream << "Error preparing source OS for Azure. "
                << RecoveryStatus::Instance().GetLastErrorMessge();

            retcode = E_RECOVERY_INTERNAL;
            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::CHANGE_FSTAB;
        if (!AR::FixSourceFstabEntries())
        {
            errStream << "Error fixing source etc/fstab entries. "
                << RecoveryStatus::Instance().GetLastErrorMessge();

            retcode = E_RECOVERY_COULD_NOT_FIX_FSTAB;
            break;
        }

        curTaskDesc = TASK_DESCRIPTIONS::UPLOAD_LOG;
    } while (false);

    // Failure from below cleanup steps can be ignored.
    AR::UnmountSourceSystemPartitions();
    AR::DeactivateAllVgs();
    AR::FlushAllBlockDevices();

    // Set error details. And this retcode will be ignored
    // if low level functions had already set error code.
    // Also it is resposibility of low level functions
    // to set the error data specific to the failure.
    RecoveryStatus::Instance().UpdateErrorDetails(
        retcode,
        errStream.str());
    RecoveryStatus::Instance().UpdateProgress(100,
        retcode == 0 ? Recovery_Status::ExecutionSuccess :
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
    TRACE_FUNC_BEGIN;
    int retcode = 0;

    std::stringstream errStream;
    std::string curTaskDesc;

    // TODO: Linux gen conversion steps.
    errStream << "Not Implemented";
    curTaskDesc = TASK_DESCRIPTIONS::UPLOAD_LOG;

    RecoveryStatus::Instance().UpdateErrorDetails(
        retcode,
        errStream.str());

    // Update progress.
    RecoveryStatus::Instance().UpdateProgress(100,
        retcode == 0 ?
        Recovery_Status::ExecutionSuccess :
        Recovery_Status::ExecutionFailed,
        curTaskDesc);

    TRACE_FUNC_END;
    return retcode;
}

namespace AzureRecovery
{

/*
Method      : VerifyDisks

Description : Verifies that that all the disks of source machine are discoverable and then
              create the mapping of source to target device names.

Parameters  : [out] mapSrcTgtDevices : Filled with source -> target device name pairs
              [out] errorMsg         : Filled with error message on failure.

Return      : true on success, otherwise false.

*/
bool VerifyDisks( std::map<std::string, std::string>& mapSrcTgtDevices, 
                  std::string& errorMsg)
{
    int nMaxRetry = 5;
    bool bSuccess = true;
    bool bAllDisksDiscovered = false;
    TRACE_FUNC_BEGIN;
    
    std::stringstream errorStream;
    
    disk_lun_map mapSrcDiskTgtLun;
    AzureRecoveryConfig::Instance().GetDiskMap(mapSrcDiskTgtLun);

    do
    {
        std::map<UINT, std::string> mapLunTgtDevice;
        DWORD dwRet = GetLunDeviceIdMappingOfDataDisks( mapLunTgtDevice );
        if(0 != dwRet)
        {
            errorStream << "Could not get LUN -> Device mapping on hydration VM. "
                        << GetLastErrorMsg();
            
            bSuccess = false;
            break;
        }
        
        //
        // Verify that all the data disks are discovered.
        //
        bAllDisksDiscovered = true;
        disk_lun_cons_iter iterSrcDiskLun = mapSrcDiskTgtLun.begin();
        for(; iterSrcDiskLun != mapSrcDiskTgtLun.end(); iterSrcDiskLun++)
        {
            //
            // Get the original disk name for sanitized-name by looking at host info.
            // 
            std::string srcDiskName;
            TRACE_INFO("Looking for the disk device %s\n", iterSrcDiskLun->first.c_str());
            HostInfoConfig::Instance().FindDiskName(iterSrcDiskLun->first, srcDiskName);

            std::map<UINT, std::string>::const_iterator iterLunTgtDevice =
                mapLunTgtDevice.find(iterSrcDiskLun->second);
            if(iterLunTgtDevice != mapLunTgtDevice.end())
            {
                if( !srcDiskName.empty() )
                    mapSrcTgtDevices[srcDiskName] = iterLunTgtDevice->second;
                else
                    TRACE_ERROR ("Host info does not have disk entry related to %s\n",
                                 iterSrcDiskLun->first.c_str()
                                );
                    // Ignoring the error at this stage as the work-flow will fail at later
                    // point if its a source boot disk and it's not available in mapSrcTgtDevices map.
                    // If its a data disk then this failure does not affect the workflow.
            }
            else
            {
                if( --nMaxRetry > 0)
                {
                    TRACE_WARNING("Disk at Lun %d is not visible to system yet. Retrying the disk discovery\n",
                                  iterSrcDiskLun->second);

                    ACE_OS::sleep(30);
                    
                    bAllDisksDiscovered = false;
                }
                else
                {   // Error in discovering all the disks.
                    errorStream << "Device at LUN " << iterSrcDiskLun->second
                                << " is not available." ;
                
                    bSuccess = false;
                }
                break;
            }
        }
    } while (!bAllDisksDiscovered);
    
    if(!bSuccess)
    {
        errorMsg = errorStream.str();
        
        TRACE_ERROR("%s\n", errorMsg.c_str());
    }
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : MountFstab

Description : Mounts subvolumes of btrfs (for now) mounted at mount-path.

Parameters  : [in] srcRootMountPath : Root Mount path
              [in] devName : File system type

Return      : 0                   -> On Success
              mount return code   -> On failure

*/
DWORD MountFstab(const std::string& srcRootMountPath,
                 const std::string& devName)
{
    DWORD dwRet = 0;

    std::string mntCmd = GetWorkingDir();
    mntCmd += ACE_DIRECTORY_SEPARATOR_STR;
    mntCmd += MOUNT_FSTAB;

    std::string args = srcRootMountPath + " " + devName;

    std::stringstream OutStream;

    dwRet = RunCommand(mntCmd, args, OutStream);
    if( 0 != dwRet )
        TRACE_ERROR("subvolume mount error %d for %s. Retrying ...\n",
                    dwRet,
                    srcRootMountPath.c_str());
        
    TRACE_INFO("SubVolume Log:\n%s\n", 
               OutStream.str().c_str());

    return dwRet;
}

/*
Method      : MountSrcSystemPartition

Description : Identifies the partition number of system partition and calculates its name on target device
              and mounts it to a specified location. If partition is an LV then activates its VG and mounts
              the LV to a specified location.

Parameters  : [in] mountPath: full path of the mount-points
              [in] sysPartitionDetails: system partition details
              [in] mapSrcTgtDevices: Source to target device name mapping

Return      : true on success, otherwise false.

*/
bool MountSrcSystemPartition( const std::string& rootMountPath,
                              const std::string& mountPath,
                              const partition_details& sysPartitionDetails,
                              const std::map<std::string, std::string>& mapSrcTgtDevices
                            )
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    do
    {
        //
        // Identify and mount the source partition
        //
        DWORD dwRet = 0;
        std::string sysPartition;
        if(sysPartitionDetails.VolType == PartitionLayout::LVM)
        {
            sysPartition = sysPartitionDetails.LvName;
            dwRet = ActivateVG(sysPartitionDetails.VgName);
            if(0 != dwRet)
            {
                TRACE_ERROR("Could not activate vg %s on Hydration vm.\n",
                            sysPartitionDetails.VgName.c_str() );
                
                bSuccess = false;
                break;
            }
            
        }
        else if(sysPartitionDetails.VolType == PartitionLayout::FS)
        {
            //
            // Get the device name corresponds to source boot device
            //
            std::string srcBootDeviceName;
            if( mapSrcTgtDevices.find(sysPartitionDetails.DiskName) == mapSrcTgtDevices.end())
            {
                TRACE_ERROR("Source boot disk not found\n");
                
                bSuccess = false;
                break;
            }
            else
            {
                srcBootDeviceName = (mapSrcTgtDevices.find(sysPartitionDetails.DiskName))->second;
                
                TRACE_INFO("Device corresponds to source system device %s is %s\n",
                           sysPartitionDetails.DiskName.c_str(),
                           srcBootDeviceName.c_str()
                           );
            }
            
            //
            // Get the partition corresponds to source sys partition.
            //
            dwRet = GetTargetStandardOrMapperDevicePartition( sysPartitionDetails.Partition,
                                                              sysPartitionDetails.DiskName,
                                                              srcBootDeviceName,
                                                              sysPartition 
                                                             );
                                                            
            if(0 != dwRet)
            {
                TRACE_ERROR("Could not get partition corresponds to source %s partition\n",
                            sysPartitionDetails.MountPoint.c_str() );
                
                bSuccess = false;
                break;
            }
        }
        else
        {
            //ASSERT
            
            TRACE_ERROR("Unknown volume type\n");
            
            bSuccess = false;
            break;
        }
                
        if ((sysPartitionDetails.FsType.compare("btrfs") != 0) ||
            (sysPartitionDetails.MountPoint.compare("/") == 0)) 
        {
        //
        // Mount the source system partition
        //
        dwRet = MountPartition( sysPartition,
                                mountPath,
                                sysPartitionDetails.FsType
                               );
        if(0 != dwRet)
        {
            TRACE_ERROR("Could not mount the system partition %s at %s.\n",
                        sysPartition.c_str(),
                        mountPath.c_str() );
            
            //
            // Set error details.
            //
            SetRecoveryErrorCode(E_RECOVERY_COULD_NOT_MOUNT_SYS_VOL);
            SetLastErrorMsg("Mount failed with error %d.",
                sysPartition.c_str(),
                dwRet);

            bSuccess = false;
        }
        }

        if (sysPartitionDetails.FsType.compare("btrfs") == 0) 
        {
            dwRet = MountFstab(rootMountPath, sysPartition);
            if(0 != dwRet)
            {
                TRACE_ERROR("Could not mount the btrfs subvolumes of %s at %s.\n",
                            sysPartition.c_str(),
                            mountPath.c_str() );
            
                //
                // Set error details.
                //
                SetRecoveryErrorCode(E_RECOVERY_COULD_NOT_MOUNT_SYS_VOL);
                SetLastErrorMsg("Mount failed with error %d.",
                    sysPartition.c_str(),
                    dwRet);

                bSuccess = false;
            }

        }

        
    } while (false);
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : PrepareSourceSysPartitions

Description : Identifies the corresponding source system partitions/LVs on hydration-VM
              and mounts those partitions to system the OS configuration.

Parameters  : [in] mapSrcTgtDevices: source -> target device name mapping.
              [out] srcSysMountInfo: Filled with system partition type(root, boot, etc) to its mount details pairs.
              [out] errorMsg : Filled with error details in failure.

Return      : true on success, otherwise false.

*/
bool PrepareSourceSysPartitions( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                 std::map<std::string, MountPointInfo>& srcSysMountInfo,
                                 std::string& errorMsg)
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    std::stringstream errorStream;
    
    do
    {
        //
        // Scan all source block devices and VGs for partition information.
        //
        DWORD dwRet = ScanDevices(mapSrcTgtDevices);
        if(0 != dwRet)
        {
            errorStream << "Error scanning devices. "
                        << GetLastErrorMsg() ;
            
            bSuccess = false;
            break;
        }
        
        //
        // Get root partition details from host configure
        //
        partition_details rootPartitionDetails;
        const HostInfoConfig& hConf = HostInfoConfig::Instance();
        bSuccess = hConf.GetPartitionInfoFromSystemDevice( ROOT_MOUNT_POINT,
                                                           rootPartitionDetails
                                                          );
        if(!bSuccess)
        {
            errorStream << "Could not find root partition details from host info.";
            
            break;
        }
        
        //
        // Generate the mount directory for source root partition.
        //
        std::string srcRootMountPath;
        dwRet = GenerateUniqueMntDirectory(srcRootMountPath);
        if(0 != dwRet)
        {
            errorStream << "Could not generate unique mount point for root.";
            
            bSuccess = false;
            break;
        }
        
        TRACE_INFO("Source root mount path: %s\n", srcRootMountPath.c_str());
        
        bSuccess = MountSrcSystemPartition(srcRootMountPath, 
                                            srcRootMountPath,
                                            rootPartitionDetails, 
                                            mapSrcTgtDevices);
        if(!bSuccess)
        {
            errorStream << "Could not prepare source root partition. "
                        << GetLastErrorMsg() ;
            
            break;
        }
        
        TRACE_INFO("Source root partition %s successfully mounted at %s\n",
                    rootPartitionDetails.LvName.empty() ? 
                    rootPartitionDetails.Partition.c_str() : rootPartitionDetails.LvName.c_str(),
                    srcRootMountPath.c_str());
        
        //
        // Prepare Mount information for root partition and add to mount information map.
        // This information will be used in entire work-flow.
        //
        MountPointInfo rootMountInfo(rootPartitionDetails.DiskName, srcRootMountPath);
        if(rootPartitionDetails.VolType == PartitionLayout::LVM)
        {
            rootMountInfo.VgName = rootPartitionDetails.VgName;
            rootMountInfo.IsLv = true;
        }
        
        srcSysMountInfo.insert(std::make_pair(ROOT_MOUNT_POINT,rootMountInfo));
        
        //
        // Mount other system volumes under root mount-point if they have dedicated partitions.
        // Trying to mount all possible system volumes to build actual root file system hierarchy.
        //
        std::vector<std::string> nonRootSysPartitions;
        nonRootSysPartitions.push_back(BOOT_MOUNT_POINT);
        nonRootSysPartitions.push_back(ETC_MOUNT_POINT);
        nonRootSysPartitions.push_back(USR_MOUNT_POINT);
        nonRootSysPartitions.push_back(VAR_MOUNT_POINT);
        nonRootSysPartitions.push_back(OPT_MOUNT_POINT);
        nonRootSysPartitions.push_back(USR_LOCAL_MOUNT_POINT);
        nonRootSysPartitions.push_back(HOME_MOUNT_POINT);
        nonRootSysPartitions.push_back(EFI_MOUNT_POINT);
        
        std::vector<std::string>::const_iterator iterSysPartition = nonRootSysPartitions.begin();
        for(; iterSysPartition != nonRootSysPartitions.end(); iterSysPartition++)
        {
            partition_details sysPartitionDetails;
            if( hConf.GetPartitionInfoFromSystemDevice( *iterSysPartition, sysPartitionDetails) )
            {
                std::string srcSysPartMountPoint = srcRootMountPath + *iterSysPartition;
                  
                bSuccess = MountSrcSystemPartition( srcRootMountPath,
                                                    srcSysPartMountPoint, 
                                                    sysPartitionDetails, 
                                                    mapSrcTgtDevices);
                if(!bSuccess)
                {
                    errorStream << "Could not prepare source system partition: "
                                << *iterSysPartition
                                << " . " << GetLastErrorMsg() ;

                    break;
                }
                
                //
                // Prepare Mount information for non root system partition.
                //
                MountPointInfo mountInfo(sysPartitionDetails.DiskName, srcSysPartMountPoint);
                if(sysPartitionDetails.VolType == PartitionLayout::LVM)
                {
                    mountInfo.VgName = sysPartitionDetails.VgName;
                    mountInfo.IsLv = true;
                }
                
                srcSysMountInfo.insert(std::make_pair(*iterSysPartition, mountInfo));
            }
            else
            {
                TRACE_INFO("Source's %s might not be a separate partition\n",
                           iterSysPartition->c_str() );
            }
        }

        // Set OS Version in RecoveryStatus metadata.
        VerifyOSVersion(false, srcRootMountPath);

    } while (false);
    
    if(!bSuccess)
    {
        errorMsg = errorStream.str();
        
        TRACE_ERROR("%s\n", errorMsg.c_str());
    }
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : CreateProtectedDisksMapFile

Description : Creates a disks map file which will be used by Azure recovery scripts as input.

Parameters  : [in] mapSrcTgtDevices: source -> target device name mapping.
              [in] srcSysMountInfo: System partition type(root, boot, etc) to its mount details pairs.

Return      : true on success, otherwise false.

*/
bool CreateProtectedDisksMapFile( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                  const std::map<std::string, MountPointInfo>& srcSysMountInfo)
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    do
    {    
        //
        // Retrieved all source devices from host configuration in same order
        // they have appeared in host info configuration.
        //
        const HostInfoConfig& hConf = HostInfoConfig::Instance();
        std::vector<std::string> protectedSrcDevices;
        hConf.GetDiskNames(protectedSrcDevices);
        
        //
        // Populate sourceDevice list in SourceHostId.PROTECTED_DISKS_LIST file.
        // This file should have source non-os disks in the same order they have available in hostinfo
        // configure file, and the OS disks will be inserted at first.
        // 
        
        std::map<std::string, MountPointInfo>::const_iterator iterSysPartMountInfo =
                                                       srcSysMountInfo.find(BOOT_MOUNT_POINT);
        if(srcSysMountInfo.end() == iterSysPartMountInfo)
        {
            iterSysPartMountInfo = srcSysMountInfo.find(ROOT_MOUNT_POINT);
            if(srcSysMountInfo.end() == iterSysPartMountInfo)
            {
                TRACE_ERROR("Internal error: boot and root partition information is missing.\n");
                
                bSuccess = false;
                break;
            }
        }
        //
        // Make sure the disk with boot/root partition comes first in order.
        //
        std::vector<std::string>::iterator iterSrcDisk = protectedSrcDevices.begin();
        for( ; iterSrcDisk != protectedSrcDevices.end(); iterSrcDisk++)
        {
            if(iterSrcDisk->compare(iterSysPartMountInfo->second.SrcDisk) == 0)
            {
                protectedSrcDevices.erase(iterSrcDisk);
                
                protectedSrcDevices.insert( protectedSrcDevices.begin(),
                                            iterSysPartMountInfo->second.SrcDisk);
                break;
            }
        }
        
        //
        // Write these devices with their target device names to PROTECTED_DISKS_LIST file
        //
        std::string diskListFile = GetWorkingDir() +
                                   ACE_DIRECTORY_SEPARATOR_STR +
                                   hConf.HostId() + 
                                   AZURE_REC_SRC_TGT_MAP_CONF_EXT;
                                   
        std::ofstream protDisksOut(diskListFile.c_str());
        if(!protDisksOut)
        {
            TRACE_ERROR("Could not open file for writing protected disks list. File : %s\n",
                        diskListFile.c_str() );
            
            bSuccess = false;
            break;
        }
        
        TRACE_INFO("Writing source device names to protected disks list file\n");
        iterSrcDisk = protectedSrcDevices.begin();
        for( ; iterSrcDisk != protectedSrcDevices.end(); iterSrcDisk++)
        {
            //
            // Verify weather the source device is protected or not by searching for it
            // in Source<=>Target device mapping. If the device is not protected then
            // skip writing that device name in protected disks list file.
            //
            std::map<std::string, std::string>::const_iterator 
            iterSrcTgtDisks = mapSrcTgtDevices.find(*iterSrcDisk);
            if(mapSrcTgtDevices.end() != iterSrcTgtDisks)
            {
                protDisksOut << *iterSrcDisk << std::endl;
                
                TRACE_INFO("%s\n", iterSrcDisk->c_str() );
            }
            else
            {
                TRACE_ERROR("Source device %s not found in source to target device mapping. This device might not be protected\n",
                            iterSrcDisk->c_str() );
            }
        }
        
        protDisksOut.close();
    } while (false);
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : ModifyConfig

Description : Modifies the system configuration for a given system partition type.

Parameters  : [in] sysPartitionType: System partition type (boot, etc or all)
              [in] mountPoint: Mount point of the system partition.

Return      : true on success, otherwise false.

*/
bool ModifyConfig(const std::string& sysPartitionType, const std::string& mountPoint)
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    do
    {
        std::string workingDir = GetWorkingDir();
        
        std::string srcHostId  = HostInfoConfig::Instance().HostId();
        
        std::string ptConfFile = workingDir + 
                                 ACE_DIRECTORY_SEPARATOR_STR +
                                 srcHostId + 
                                 AZURE_REC_PREP_TGT_CONF_EXT;
        
        std::string ptCmd = workingDir;
        ptCmd += ACE_DIRECTORY_SEPARATOR_STR;
        ptCmd += AZURE_RECOVRY_PREPARE_TGT;
        
        std::string newHostId = AzureRecoveryConfig::Instance().GetNewHostId();
        
        if(newHostId.empty())
        {
            TRACE_ERROR("Empty Host-Id received from recovery config.\n" );
            
            bSuccess = false;
            break;
        }
        
        std::ofstream OutPtConfFile(ptConfFile.c_str(), std::ios::out | std::ios::trunc );
        if(!OutPtConfFile)
        {
            TRACE_ERROR("Could not open the configure file: %s\n",
                        ptConfFile.c_str() );
            
            bSuccess = false;
            break;
        }
        
        // TODO: Add new parameters to take log file path, hostinfo xml file paths
        
        OutPtConfFile << "_MACHINE_UUID_ " << srcHostId << std::endl;
        OutPtConfFile << "_TASK_ " << "PREPARETARGET" << std::endl;
        OutPtConfFile << "_PLAN_NAME_ " << std::endl;
        OutPtConfFile << "_VX_INSTALL_PATH_ " << std::endl;
        OutPtConfFile << "_MOUNT_PATH_ " << mountPoint << std::endl;
        OutPtConfFile << "_MOUNT_PATH_FOR_ " << sysPartitionType << std::endl;
        OutPtConfFile << "_INSTALL_VMWARE_TOOLS_ 0" << std::endl;
        OutPtConfFile << "_UNINSTALL_VMWARE_TOOLS_ 1" << std::endl;
        OutPtConfFile << "_NEW_GUID_ " << newHostId << std::endl;
        OutPtConfFile << "_WORKING_DIRECTORY_ " << workingDir << std::endl;
        
        OutPtConfFile.close();
        
        //
        // Run the Prepare Target script by providing this configure file as input.
        //

        std::stringstream ptOutStream;
        DWORD dwRet = RunCommand(ptCmd, ptConfFile, ptOutStream);
        if(0 != dwRet)
        {
            TRACE_ERROR("Prepare Target script failed with error %d\n",
                        dwRet );
            
            bSuccess = false;
        }

        TRACE_INFO("Prepare Target Script Log:\n%s\n",
                        ptOutStream.str().c_str() );
        
    } while (false);
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : PerformPreRecoveryChanges

Description : This is the entry function for performing actual changes on system configuration.
              And this routine will be called after mounting the system partitions.

Parameters  : [in] mapSrcTgtDevices: source -> target device name mapping.
              [in] srcSysMountInfo: System partition type(root, boot, etc) to its mount details pairs.

Return      : true on success, otherwise false.

*/
bool PerformPreRecoveryChanges( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                const std::map<std::string, MountPointInfo>& srcSysMountInfo,
                                std::string& errorMsg)
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    std::stringstream errorStream;
    
    do
    {
        //
        // Create protected disks list file in current directory where the script runs.
        // This file will be picked by PrepareTarget.sh file for data massaging on /boot, /etc.
        //
        bSuccess = CreateProtectedDisksMapFile( mapSrcTgtDevices, srcSysMountInfo);
        if(!bSuccess)
        {
            errorStream << "Could not create protected disks map input file. "
                        << GetLastErrorMsg() ;
            
            break;
        }
        
        //
        // Perform data massaging on existing system configuration such as enabling dhcp,
        //  updating fstab entries, updating boot image etc.
        //
        
        BOOST_ASSERT(!srcSysMountInfo.empty());
        
        std::map<std::string, MountPointInfo>::const_iterator 
            iterRoot = srcSysMountInfo.find(ROOT_MOUNT_POINT);
            
        if(iterRoot == srcSysMountInfo.end())
        {
            errorStream << "Internal error: Root mount information missing.";
            
            bSuccess = false;
            break;
        }
        
        bSuccess = ModifyConfig("all", iterRoot->second.MountPoint);
        if(!bSuccess)
        {
            errorStream << "Could not modify configuration in system partitions";
            
            break;
        }
        
        //
        // If its a test failover then disable the services on recovering vm.
        //
        if (AzureRecoveryConfig::Instance().IsTestFailover())
        {
            if (DisableService(iterRoot->second.MountPoint,MobilityAgents::VXAgent) ||
                DisableService(iterRoot->second.MountPoint,MobilityAgents::UARespawn))
            {
                errorStream << "Could not disable mobility services";
                
                bSuccess = false;
                break;
            }

            //
            // Disable FXAgen.
            // If FX Agent is not installed on source then this call may fail,
            // hence ignoring return code.
            //
            DisableService(iterRoot->second.MountPoint, MobilityAgents::FXAgent);
        }
        
        //
        // Run pre recovery script to install the WALinuxAgent and other drivers on source system partitions.
        // It also configures Network changes for recovering VM.
        //
        
        std::string rcvrScriptCmd = GetWorkingDir();
        rcvrScriptCmd += ACE_DIRECTORY_SEPARATOR_STR;
        rcvrScriptCmd += AZURE_PRE_RECOVERY_SCRIPT;
        
        std::string rcvrScriptCmdArgs = iterRoot->second.MountPoint;
        rcvrScriptCmdArgs += " " + GetWorkingDir();
        rcvrScriptCmdArgs += " " + GetHydrationConfigSettings();
        
        std::stringstream OutStream;
        DWORD dwRet = RunCommand(rcvrScriptCmd, rcvrScriptCmdArgs, OutStream);
        if( 0 != dwRet )
        {
            errorStream << "Pre Recovery script failed with error " << dwRet;
            
            bSuccess = false;
            //break; //Don't break here, log Command outstream on failure as well.
        }

        std::string err_line, telemetry_data;
        while (std::getline(OutStream, err_line))
        {
            boost::trim(err_line);
            if (boost::istarts_with(err_line,
                PrepareForAzureScript::TELEMETRY_DATA_LINE_BEGIN))
            {
                telemetry_data = boost::erase_first_copy(
                    err_line,
                    PrepareForAzureScript::TELEMETRY_DATA_LINE_BEGIN);
                break;
            }
        }

        RecoveryStatus::Instance().SetTelemetryData(
            telemetry_data);

        TRACE_INFO("Script Console Log:\n%s\n", OutStream.str().c_str());
    
    } while (false);
    
    if(!bSuccess)
    {
        errorMsg = errorStream.str();
        
        TRACE_ERROR("%s\n", errorMsg.c_str());
    }

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : CleanupMountPoints

Description : Un-mount the mounted system partitions, and delete the randomly generated root 
              partition mount folder.

Parameters  : [in] mapSrcTgtDevices: source -> target device name mapping.
              [out] errorMsg: filled with error details on failure.

Return      : true on success, otherwise false.

*/
bool CleanupMountPoints( const std::map<std::string, MountPointInfo>& srcSysMountInfo,
                         std::string& errorMsg )
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    BOOST_ASSERT(!srcSysMountInfo.empty());
    
    std::stringstream errorStream;
    
    do
    {
        //
        // All the mounted mount-points are inserted into srcSysMountInfo map, and the map sorts them based on mount-point name
        // so the longest mount-point path will be bellow to its parent if exist. So iterate the map in reverse direction to
        // get the child mount points first before any of its parent.
        // For Example: Insertion => /, /etc, /boot, /usr, /var, /usr/local, /opt
        // Forward iteration => / ,/boot ,/etc ,/opt ,/usr, /usr/local ,/var
        // Reverse iteration => /var, /usr/local ,/usr ,/opt ,/etc ,/boot ,/
        // 
        std::set<std::string> VGSet;
        std::map<std::string, MountPointInfo>::const_reverse_iterator iterSrcSysMountInfo = srcSysMountInfo.rbegin();
        for(; iterSrcSysMountInfo != srcSysMountInfo.rend(); iterSrcSysMountInfo++ )
        {
            TRACE_INFO("Un-mounting %s\n", iterSrcSysMountInfo->second.MountPoint.c_str());
            
            if(iterSrcSysMountInfo->second.IsLv)
            {
                BOOST_ASSERT(!iterSrcSysMountInfo->second.VgName.empty());
                TRACE_INFO("Un-mounting volume is an LV, and its VG is : %s\n", iterSrcSysMountInfo->second.VgName.c_str());
                VGSet.insert(iterSrcSysMountInfo->second.VgName);
            }
            
            DWORD dwRet = UnMountPartition(iterSrcSysMountInfo->second.MountPoint, true, true);
            if(0 != dwRet)
            {
                errorStream << "Unable to un-mount the partition " << iterSrcSysMountInfo->second.MountPoint;
                
                bSuccess = false;
                break;
            }
        }
        
        //
        // Deactivate the VGs
        //
        std::set<std::string>::const_iterator iterVG = VGSet.begin();
        for( ; iterVG != VGSet.end(); iterVG++)
        {
            DWORD dwRet = DeactivateVG(*iterVG);
            if(0 != dwRet)
            {
                errorStream << "VG " << *iterVG << " de-activation failed with error " << dwRet;
                
                bSuccess = false;
                break;
            }
            else
            {
                TRACE_INFO("VS %s de-activated successfully\n", iterVG->c_str());
            }
        }
    
        if(!bSuccess)
        {
            TRACE_INFO("One of the source system partition VG de-activation failed.\n");
            TRACE_INFO("Skipping root partition mount directory deletion\n");
            
            break;
        }
    
        //
        // Delete the root partition mount directory.
        //
        std::map<std::string, MountPointInfo>::const_iterator iterRootMountInfo= srcSysMountInfo.find(ROOT_MOUNT_POINT);		
        if(iterRootMountInfo == srcSysMountInfo.end() || 
           0 != ACE_OS::rmdir(iterRootMountInfo->second.MountPoint.c_str()))
        {
            errorStream << "Could not delete Source Root Partition mount point on hydration VM. Error "
                        << ACE_OS::last_error() ;
            
            //
            // Not failing the operation on rmdir failure as its not a critical error.
            //
        }
        
    } while (false);
    
    if(!bSuccess)
    {
        errorMsg = errorStream.str();
        
        TRACE_ERROR("%s\n", errorMsg.c_str());
    }
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : FlushDevicesAndRevertLvmConfig

Description : Flush the all disk devices to clear any intermediate buffers.

Parameters  : [in] mapSrcTgtDevices: source -> target device name mapping.

Return      : true on success, otherwise false.

*/
bool FlushDevicesAndRevertLvmConfig( const std::map<std::string, std::string>& mapSrcTgtDevices,
                                     std::string& errorMsg)
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    BOOST_ASSERT(!mapSrcTgtDevices.empty());
    
    std::stringstream errorStream;
    
    do
    {        
        //
        // Flush all the source devices
        //
        if(0 != FlushBlockDevices(mapSrcTgtDevices) )
        {
            errorStream << "Some of the devices might not flushed successfully. "
                        << GetLastErrorMsg() ;
            
            bSuccess = false;
        }
        
        //
        // Un-Register LVs from Hydration VM. Again the return code is not 
        // considering as its related to Hydration-VM LVs configuration.
        //
        std::set<std::string> srcLvs;
        HostInfoConfig::Instance().GetAllLvNames(srcLvs);
        UnRegisterLVs(srcLvs);
        
        //
        // Note: In Caspian V1, there is additional clean-up w.r.t scsi LUN slots and
        //       multipath mapper device name. It is not required here as the Hydration-VM
        //       does not use multipath mapper device names and the scsi LUN slots are not
        //       going to re-use.
        //
        
    } while (false);
    
    if(!bSuccess)
    {
        errorMsg = errorStream.str();
        
        TRACE_ERROR("%s\n", errorMsg.c_str());
    }
    
    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : DiscoverSourceSystemPartitions

Description : Discovers the source root('/') and other system partitions.
              On success, the root partition will mounted at SMS_AZURE_CHROOT
              and rest of the system partition details are discovered and
              updated in the SourceFilesystemTree object.

Parameters  :

Return      : true on success, otherwise false.

*/
bool DiscoverSourceSystemPartitions()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    TRACE_INFO("Fetching the volumes/partitions visible to the system.\n");
    std::vector<volume_details> volumes;
    BlockDeviceDetails::Instance().GetVolumeDetails(volumes, true);

    std::vector<fstab_entry> fs_entries;
    SourceFilesystemTree &sft = SourceFilesystemTree::Instance();

    bool bIsRootFound = false;
    BOOST_FOREACH(const volume_details &volume, volumes)
    {
        // Ignore filesystem type if swap or lvm or empty.
        // TODO: Improve this logic, and have a list of supported fs types.
        if (volume.fstype.empty() ||
            boost::equals(volume.fstype, "swap") ||
            boost::istarts_with(volume.fstype, "lvm"))
        {
            TRACE_INFO("Volume %s filesystem is [%s], skipping it.\n",
                volume.name.c_str(),
                volume.fstype.c_str());

                continue;
        }

        std::string mountpoint = bIsRootFound ? SMS_AZURE_TMP_MNT : SMS_AZURE_CHROOT;
        if (!CreateDirIfNotExist(mountpoint))
        {
            TRACE_ERROR("Could not create directory for mounting source partitions.\n");
            
            bSuccess = false;
            break;
        }

        TRACE_INFO("Checking volume: %s.\n", volume.ToString().c_str());
        std::string partition_dev = "UUID=" + volume.uuid;
        DWORD dwRet = MountPartition(partition_dev,
            mountpoint,
            volume.fstype);
        if (0 == dwRet)
        {
            std::string src_partition;
            if (sft.DetectPartition(mountpoint, src_partition, volume) &&
                boost::equals(src_partition, "/"))
            {
                bIsRootFound = true;

                // If detected partition is a root(/) then verify OS version.
                // If its not in supported OS list then stop the hydration.
                if (!VerifyOSVersion(true, mountpoint))
                {
                    bSuccess = false;
                    TRACE_ERROR("Unsupported OS detected.\n");

                    // VerifyOSVersion sets the error code on unsupported os,
                    // no need to set here again.

                    // Unmount and break the detection loop.
                    UnMountPartition(mountpoint);
                    break;
                }
                
                // Its a supported OS, now read fstab file and update
                // details in SourceFilesystemTree obj. Also, skip unmounting
                // this partition as it will be the chroot and rest of the
                // system partiton will be munted under this chroot mount point.
                ReadFstab(SMS_AZURE_CHROOT_FSTAB, fs_entries);

                sft.UpdateSrcFstabDetails(fs_entries);
            }
            else
            {
                dwRet = UnMountPartition(mountpoint);
                if (0 != dwRet)
                {
                    TRACE_ERROR("Could not un-mount this device %s.\n",
                        partition_dev.c_str());

                    // Check if the mountpoint is still valid, otherwise 
                    // ignore the failure and continue the execution.
                    if (0 == RunCommand(CmdLineTools::FindMnt, mountpoint))
                    {
                        // Mount point is still valid, its real unmount issue.
                        // Return failure.
                        SetRecoveryErrorCode(E_RECOVERY_INTERNAL);
                        SetLastErrorMsg("Unmount failed with error %d", dwRet);

                        bSuccess = false;
                        break;
                    }
                    else
                    {
                        TRACE_INFO("Mountpoint is no more valid, ignoring the umount failure.\n");
                    }
                }
            }
        }
        else
        {
            TRACE_WARNING("Cound not mount this device %s. Moving to next.\n",
                partition_dev.c_str());
        }

        if (!sft.ShouldContinueDetection())
        {
            TRACE_INFO("Required partitions are discovered. Stop mounting...\n");
            break;
        }
    }

    // Apply stanadard device name heuristics 
    // to detect undiscovered standard partitons.
    sft.DetectUndiscoveredStandardPartitions();

    // Update btrfs subvolumes if defined in fstab. And
    // if any subvolume is found then it will be mounted
    // along with source system partitions.
    sft.UpdateFstabSubvolDetails(fs_entries);

    // Check if the required system partitions are discovered,
    // if not then set the error data and return failure. If any
    // of the required source system partitions is not discovered
    // then this function should return failure.
    if (bSuccess && !sft.IsDiscoveryComplete())
    {
        bSuccess = false;

        std::string mntpoints;
        std::vector<std::string> undiscovered_mountpoints;
        sft.GetUndiscoveredSrcSysMountpoints(undiscovered_mountpoints);

        BOOST_FOREACH(const std::string &mnt, undiscovered_mountpoints)
            mntpoints += mnt + ";";

        RecoveryStatus::Instance().SetStatusErrorCode(
            E_RECOVERY_VOL_NOT_FOUND,
            mntpoints);
    }

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : MountSourceSystemPartitions

Description : Root should have already mounted while detecting system
              partitions, it mounts rest of the source system partitions
              if there is any.

Parameters  :

Return      : true on success, otherwise false.

*/
bool MountSourceSystemPartitions()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    // Fetch sft mountpoint entries which are really seperate
    // partitions on source.
    SourceFilesystemTree &sft = SourceFilesystemTree::Instance();
    std::vector<fs_tree_entry> sft_entries;
    sft.GetRealSystemPartitions(sft_entries);

    BOOST_ASSERT(!sft_entries.empty());
    BOOST_ASSERT(FileExists(SMS_AZURE_CHROOT_FSTAB));

    BOOST_FOREACH(const fs_tree_entry& sft_entry, sft_entries)
    {
        // root (/) is already mounted, ignore it
        // and continue with rest.
        if (boost::equals(sft_entry.mountpoint, "/"))
        {
            // If its btrfs volume then mount subvolumes.
            if (sft_entry.src_fstab_entry.IsBtrfsVolume())
            {
                TRACE_INFO("%s is btrfs volume, mounting its subvolumes too.\n",
                    sft_entry.mountpoint.c_str());

                sft_entry.MountSubvolumes();
            }

            continue;
        }

        std::stringstream mnt;
        mnt << SMS_AZURE_CHROOT
            << ACE_DIRECTORY_SEPARATOR_STR
            << boost::trim_left_copy_if(sft_entry.mountpoint,
                boost::is_any_of(ACE_DIRECTORY_SEPARATOR_STR));

        BOOST_ASSERT(!sft_entry.is_discovered);

        std::string partition_dev =
            sft_entry.src_fstab_entry.IsStandardDeviceName() ?
            "UUID=" + sft_entry.vol_info.uuid:
            sft_entry.src_fstab_entry.device;

        TRACE_INFO("Mount %s at %s.\n",
            partition_dev.c_str(),
            mnt.str().c_str());

        BOOST_ASSERT(!partition_dev.empty());

        DWORD dwRet = MountPartition(partition_dev,
            mnt.str(),
            sft_entry.src_fstab_entry.fstype);
        if (0 != dwRet)
        {
            TRACE_ERROR("Could not mount partition on hydration VM for: %s.\n",
                sft_entry.src_fstab_entry.ToString().c_str());

            RecoveryStatus::Instance().SetStatusErrorCode(
                E_RECOVERY_COULD_NOT_MOUNT_SYS_VOL,
                sft_entry.src_fstab_entry.mountpoint);

            bSuccess = false;
            break;
        }

        // If its btrfs volume then mount subvolumes too.
        if (sft_entry.src_fstab_entry.IsBtrfsVolume())
            sft_entry.MountSubvolumes();
        
        TRACE_ERROR("Successfully mounted the partition on hydration: %s.\n",
            sft_entry.src_fstab_entry.ToString().c_str());
    }

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : VerifyOSVersion

Description : Verify is the OS is supported, it also collect the OS information.

Parameters  : [in] setError: If set to false, the function only logs the OS Version
              in telemetry without failing for UnsupportedOSVersion
              [in] mntPath: The mount path where the root partition of source VM is present.

Return      : true on success, otherwise false.

*/
bool VerifyOSVersion(bool setError, std::string mntPath)
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    do
    {
        std::stringstream os_details_script;
        os_details_script << GetWorkingDir()
            << ACE_DIRECTORY_SEPARATOR_STR
            << AZURE_OS_DETAILS_TGT;

        std::stringstream script_args;
        script_args << mntPath
            << " --formated-output";

        std::stringstream osDetailsScriptOut;
        DWORD dwRet = RunCommand(os_details_script.str(),
            script_args.str(),
            osDetailsScriptOut);

        TRACE_INFO("\n%s\n", osDetailsScriptOut.str().c_str());

        if (dwRet != 0)
        {
            TRACE_ERROR("Could not detect source OS. Script failed with error %d\n",
                dwRet);

            bSuccess = false;
            break;
        }

        std::string os_version, os_details;
        GetOSDetailsFromScriptOutput(osDetailsScriptOut,
            os_version,
            os_details);

        // Set os_version or os_details string as metadata to status blob.
        // This will be used for telemetry.

        boost::trim(os_details);
        if (os_details.empty())
        {
            os_details = "Unknown";
            RecoveryStatus::Instance().SetSourceOSDetails(os_version);
        }
        else
        {
            RecoveryStatus::Instance().SetSourceOSDetails(os_details);
        }

        if (setError)
        {
            if (os_version.empty() ||
                !IsSupportedOS(os_version))
            {
                bSuccess = false;

                TRACE_ERROR("Unsupported OS. OS Version: %s OS Details: %s\n",
                    os_version.c_str(),
                    os_details.c_str());

                // Set hydration error code as unsupported OS.
                RecoveryStatus::Instance().SetStatusErrorCode(
                    E_RECOVERY_OS_UNSUPPORTED,
                    os_version);

                SetLastErrorMsg("Unsupported OS version detected.");
            }
        }

    } while (false);

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : PrepareSourceOSForAzure

Description : Prepares the source OS for Azure.

Parameters  :

Return      : true on success, otherwise false.

*/
bool PrepareSourceOSForAzure()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    std::stringstream pre_os_script;
    pre_os_script << GetWorkingDir()
        << ACE_DIRECTORY_SEPARATOR_STR
        << PREPARE_OS_FOR_AZURE_SCRIPT;

    std::stringstream pre_os_script_args;
    pre_os_script_args << SMS_AZURE_CHROOT
        << " "
        << GetHydrationConfigSettings();

    std::stringstream scriptOut;
    DWORD dwRet = RunCommand(pre_os_script.str(),
        pre_os_script_args.str().c_str(),
        scriptOut);

    // Always record error data and telemetry data.
    std::string err_line, err_data, telemetry_data;
    while (std::getline(scriptOut, err_line))
    {
        boost::trim(err_line);
        if (boost::istarts_with(err_line,
            PrepareForAzureScript::ERROR_DATA_LINE_BEGIN))
        {
            err_data = boost::erase_first_copy(
                err_line,
                PrepareForAzureScript::ERROR_DATA_LINE_BEGIN);
        }
        else if (boost::istarts_with(err_line,
            PrepareForAzureScript::TELEMETRY_DATA_LINE_BEGIN))
        {
            telemetry_data = boost::erase_first_copy(
                err_line,
                PrepareForAzureScript::TELEMETRY_DATA_LINE_BEGIN);
        }
    }

    if (E_RECOVERY_SUCCESS != dwRet)
    {
        bSuccess = false;

        TRACE_ERROR("Prepare for Azure script failed with error %d\n",
            dwRet);        

        // Map the script exit code to recovery error code.
        int error_code = E_RECOVERY_SUCCESS;
        switch (dwRet)
        {
        case PrepareForAzureScript::E_INTERNAL:
            error_code = E_RECOVERY_INTERNAL;
            break;
        case PrepareForAzureScript::E_SCRIPT_SYNTAX_ERROR:
            error_code = E_RECOVERY_SCRIPT_SYNTAX_ERROR;
            break;
        case PrepareForAzureScript::E_TOOLS_MISSING:
            error_code = E_RECOVERY_TOOLS_MISSING;
            break;
        case PrepareForAzureScript::E_ARGS:
            error_code = E_RECOVERY_INTERNAL;
            break;
        case PrepareForAzureScript::E_OS_UNSUPPORTED:
            error_code = E_RECOVERY_OS_UNSUPPORTED;
            break;
        case PrepareForAzureScript::E_CONF_MISSING:
            error_code = E_RECOVERY_SYS_CONF_MISSING;
            break;
        case PrepareForAzureScript::E_INITRD_IMAGE_GENERATION_FAILED:
            error_code = E_RECOVERY_INITRD_IMAGE_GENERATION_FAILED;
            break;
        case PrepareForAzureScript::E_HV_DRIVERS_MISSING:
            error_code = E_RECOVERY_HV_DRIVERS_MISSING;
            break;
        case PrepareForAzureScript::E_AZURE_GA_INSTALLATION_FAILED:
            error_code = E_RECOVERY_GUEST_AGENT_INSTALLATION_FAILED;
            break;
        case PrepareForAzureScript::E_ENABLE_DHCP_FAILED:
            error_code = E_RECOVERY_ENABLE_DHCP_FAILED;
            break;
        default:
            // Any other error code is an internal error.
            error_code = E_RECOVERY_INTERNAL;
            break;
        }

        RecoveryStatus::Instance().SetStatusErrorCode(
            error_code,
            err_data);
    }

    RecoveryStatus::Instance().SetTelemetryData(
        telemetry_data);

    TRACE_INFO("Prepare for Azure log:\n%s\n",
        scriptOut.str().c_str());

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : FixSourceFstabEntries

Description : Verifies and fixes the source fstab entries.

Parameters  :

Return      : true on success, otherwise false.

*/
bool FixSourceFstabEntries()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(FileExists(SMS_AZURE_CHROOT_FSTAB));

    TRACE("Fixing source fstab file entries.\n");

    std::vector<volume_details> volumes;
    BlockDeviceDetails::Instance().GetVolumeDetails(volumes);

    // Get device to uuid mapping if azure_sms_blkid.out file is present.
    std::map<std::string, std::string> src_blkid_map;
    GetSourceBlkidDetails(SMS_AZURE_BLKID_OUT, src_blkid_map);

    SourceFilesystemTree &sft = SourceFilesystemTree::Instance();

    std::ifstream ifstab(SMS_AZURE_CHROOT_FSTAB);
    std::ofstream ofstab_new(SMS_AZURE_CHROOT_FSTAB_NEW);

    std::string fstab_line;
    while (std::getline(ifstab, fstab_line))
    {
        TRACE("Processing fstab line: %s.\n", fstab_line.c_str());

        // We skip processing commented lines and malformed entries
        // and write such lines as is to the new fstab file.
        fstab_entry fe;
        if (fstab_entry::Fill(fe, fstab_line))
        {
            if (fe.IsStandardDeviceName())
            {
                // The detection logic builds source => target disk mapping
                // when it detected some of the system partitions. If the
                // partition name in current fstab entry belong to one of 
                // those disks then we apply some heuristics to find 
                // corresponding partitions and replace the standard partition
                // name with uuid. If the volume is not detected then the current
                // fstab entry will be commented.
                volume_details tgt_vol;
                if (sft.FindVolForFstabEntry(fe, tgt_vol) &&
                    !boost::trim_copy(tgt_vol.uuid).empty())
                {
                    // Replace device with UUID.
                    fe.device = "UUID=" + boost::trim_copy(tgt_vol.uuid);

                    fstab_line = fe.GetFstabLine();
                }
                else if (src_blkid_map.find(fe.device) != src_blkid_map.end())
                {
                    // Found device in uuid using source blkid output.
                    fe.device = "UUID=" + src_blkid_map[fe.device];

                    TRACE_INFO("Device uuid is found using source blkid data: %s.\n",
                        fe.device.c_str());

                    fstab_line = fe.GetFstabLine();
                }
                else
                {
                    TRACE_WARNING("Could not determine UUID for the partition %s. Commenting it's fstab line.\n",
                        fe.device.c_str());

                    // If above two condition were not met then comment it.
                    fstab_line = "# " + fstab_line;
                }
            }
            else if (fe.IsNfsDevice())
            {
                TRACE_WARNING("Network filesystem detected, commenting the entry.\n");

                fstab_line = "# " + fstab_line;
            }
            else if (fe.IsPersistentLocalDevice())
            {
                bool bDevFound = false;
                BOOST_FOREACH(const volume_details &vol, volumes)
                {
                    // If device is found then stop the lookup.
                    if (vol == fe)
                    {
                        bDevFound = true;
                        break;
                    }
                }

                // If device is not found then add nofail to options,
                // otherwise no change needed for the entry.
                if (bDevFound)
                {
                    TRACE_INFO("Volume [%s] found in replicated disks.\n",
                        fe.device.c_str());
                }
                else
                {
                    fstab_line = fe.GetFstabLineWithOptions("nofail");

                    TRACE_WARNING("Volume [%s] not found in replicated disks.\n",
                        fe.device.c_str());
                }
            }
            else
            {
                TRACE_INFO("[%s] could be a non-persistent device, ignoring it.\n",
                    fe.device.c_str());
            }
        }

        // Write line to new fstab
        ofstab_new << fstab_line << std::endl;
    }

    // Swap the files.
    bSuccess = 
        RenameFile(SMS_AZURE_CHROOT_FSTAB, SMS_AZURE_CHROOT_FSTAB_BCK) &&
        RenameFile(SMS_AZURE_CHROOT_FSTAB_NEW, SMS_AZURE_CHROOT_FSTAB);

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : UnmountSourceSystemPartitions

Description : Unmounts chroot directory.

Parameters  :

Return      : true on success, otherwise false.

*/
bool UnmountSourceSystemPartitions()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    std::stringstream mountinfo;
    DWORD dwRet = RunCommand(CmdLineTools::FindMnt,
        SMS_AZURE_CHROOT,
        mountinfo);

    TRACE_INFO("%s mount info:\n%s\n",
        SMS_AZURE_CHROOT,
        mountinfo.str().c_str());

    // If no partition is not mounted at chroot mountpoint
    // then skip unmount attempt on it.
    if (dwRet == 1)
    {
        TRACE_WARNING("%s is not a valid mountpoint. Skipping unmount.\n",
            SMS_AZURE_CHROOT);
    }
    else if (UnMountPartition(SMS_AZURE_CHROOT, true, true) != 0)
    {
        bSuccess = false;

        TRACE_ERROR("Could not unmount the source root parition.\n");
    }
    else
    {
        TRACE_INFO("Successfully unmounted %s\n.",
            SMS_AZURE_CHROOT);
    }

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : DeactivateAllVgs

Description : Deactivates all the VGs visible to the system.

Parameters  :

Return      : true on success, otherwise false.

*/
bool DeactivateAllVgs()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;
    
    // Get list of VGs visible to the system.
    std::vector<std::string> vgs;
    GetVgList(vgs);

    BOOST_FOREACH(const std::string& vg, vgs)
    {
        TRACE_INFO("Deactivating the VG: %s.\n", vg.c_str());

        // Ignoring the return code.
        DeactivateVG(vg);
    }

    TRACE_FUNC_END;
    return bSuccess;
}

/*
Method      : FlushAllBlockDevices

Description : Flushes the buffers of all block devices in the system.

Parameters  :

Return      : true on success, otherwise false.

*/
bool FlushAllBlockDevices()
{
    bool bSuccess = true;
    TRACE_FUNC_BEGIN;

    // Get list of disk devices in the system.
    std::vector<std::string> devices;
    BlockDeviceDetails::Instance().GetDiskNames(devices);

    // Flush all the devices.
    bSuccess = FlushBlockDevices(devices) == 0;

    TRACE_FUNC_END;
    return bSuccess;
}

} // ~AzureRecovery
