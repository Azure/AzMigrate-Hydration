/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WinUtils.h

Description	:   Windows utility functions declarations

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_VOLUME_UTILS_H
#define AZURE_RECOVERY_VOLUME_UTILS_H

#include <list>
#include <map>

#include <Windows.h>
#include <WinIoCtl.h>

#include "WinConstants.h"
#include "../config/HostInfoDefs.h"
#include "../config/HostInfoConfig.h"
#include "../common/utils.h"
#include "OSVersion.h"
#include "WmiRecordProcessors.h"

namespace AzureRecovery
{
    DWORD GetVolumeFromDiskExtents(const disk_extents_t& volExtents,
        std::string& volumeGuid,
        const std::string& diskId = "");

    std::string GetSourceOSInstallPath(const std::string& OSInstallPath,
        const std::string& OSDriveName,
        const std::string& SourceOSVolMountPath);

    DWORD GetDiskId(const std::string& deviceName, std::string& diskID);

    DWORD GetDiskId(DWORD dwDiskNum, std::string& diskID);

    DWORD GetLunDeviceIdMappingOfDataDisks(std::map<UINT, std::string>& lun_device, UINT port_num);

    DWORD GetVolumeDiskExtents(const std::string& volumeGuid,
        disk_extents_t& diskExtents);

    DWORD GetVolumeMountPoints(const std::string& volumeGuid,
        std::list<std::string>& mountPoints);

    DWORD AutoSetMountPoint(const std::string& volumeGuid, std::string& mountPoint);

    DWORD SetPrivileges(const std::string& privilege, bool set);

    DWORD OnlineBasicDisk(UINT diskNumber);

    DWORD VerifyDiskSignaturGuidCollision(const mdft_disk_info_t& msft_disk);

    DWORD VerifyOfflineDisksAndBringOnline(const std::string& disk_id = "");

    DWORD RunBcdEdit(const std::string& srcSystemPath,
        const std::string& activePartitionDrivePath,
        const std::string& bcdFileRelativePath);

    DWORD EnableSerialConsole(const std::string& srcSystemPath);

    DWORD UpdateBCDWithConsoleParams(const std::string& srcSystemPath,
        const std::string& bcd_file);

    DWORD RunBcdBootAndUpdateBCD(const std::string& srcSystemPath,
        const std::string& activePartitionDrivePath,
        const std::string& firmwareType);

    DWORD FindVolumesWithPath(const std::string& relative_path,
        std::list<std::string>& volumes,
        const std::string& exclude_disk_id);

    DWORD SearchVolumeForPath(
        const std::string& volume_guid_path,
        const std::string& relative_path,
        std::string& volume_mount_path);

    DWORD GetSystemDiskId(std::string& disk_id);

    DWORD GetFileVersion(const std::string& file_path, std::string& version);

    void PrintAllDiskPartitions();
}

#endif AZURE_RECOVERY_VOLUME_UTILS_H
