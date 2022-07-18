/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WinUtils.cpp

Description	:   Windows utility functions definitions

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "WinUtils.h"
#include "WmiRecordProcessors.h"
#include "../common/Trace.h"
#include "../common/AzureRecoveryException.h"
#include "RegistryUtil.h"
#include "inmsafecapis.h"

#include <sstream>
#include <list>
#include <atlbase.h>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#pragma comment(lib, "version.lib")

namespace AzureRecovery
{

    /*
    Method      : GetDiskId

    Description : Retrieves the signature or guid of a give disk name.

    Parameters  : [in] deviceName : Name of the disk (Ex: \\.\PhysicalDrive0)
                  [out] diskID: filled with signature of guid of the disk.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD GetDiskId(const std::string& deviceName, std::string& diskID)
    {
        TRACE_FUNC_BEGIN;

        BOOST_ASSERT(!deviceName.empty());

        DWORD dwRet = ERROR_SUCCESS;
        HANDLE hDisk = INVALID_HANDLE_VALUE;
        BOOL bSuccess = FALSE;
        DWORD dwBytesReturned = 0;
        int ncPartitions = 2;
        PDRIVE_LAYOUT_INFORMATION_EX pDli_info = NULL;

        do {
            hDisk = CreateFile(deviceName.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (INVALID_HANDLE_VALUE == hDisk)
            {
                dwRet = GetLastError();
                TRACE_ERROR("%s:Line %d: Could not open disk handle. CreateFile fialed with error %lu\n",
                    __FUNCTION__, __LINE__,
                    dwRet);
                break;
            }

            do {

                size_t cbBuffSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) +
                    ncPartitions * sizeof(PARTITION_INFORMATION_EX);

                pDli_info = (PDRIVE_LAYOUT_INFORMATION_EX)malloc(cbBuffSize);
                if (NULL == pDli_info)
                {
                    dwRet = ERROR_OUTOFMEMORY;
                    TRACE_ERROR("%s:Line %d: Could not allocate memory.\n", __FUNCTION__, __LINE__);
                    break;
                }

                bSuccess = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                    NULL,
                    0,
                    pDli_info,
                    cbBuffSize,
                    &dwBytesReturned,
                    NULL);
                if (!bSuccess)
                {
                    dwRet = GetLastError();
                    if (ERROR_INSUFFICIENT_BUFFER == dwRet)
                    {
                        free(pDli_info);
                        pDli_info = NULL;

                        ncPartitions += 2; //Increment the partitions count for more memory

                        dwRet = ERROR_SUCCESS;
                    }
                    else
                    {
                        TRACE_ERROR("%s:Line %d: IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed with error %lu\n",
                            __FUNCTION__, __LINE__,
                            dwRet);
                        break;
                    }
                }
            } while (!bSuccess);

            if (!bSuccess)
                break;

            //Get DiskId
            if (PARTITION_STYLE_MBR == pDli_info->PartitionStyle)
            {
                std::stringstream signature;
                signature << pDli_info->Mbr.Signature;

                diskID = signature.str();
            }
            else if (PARTITION_STYLE_GPT == pDli_info->PartitionStyle)
            {
                USES_CONVERSION;
                WCHAR szGUID[MAX_PATH] = { 0 };

                if (!StringFromGUID2(pDli_info->Gpt.DiskId, szGUID, MAX_PATH))
                {
                    TRACE_ERROR("%s:Line %d: StringFromGUID2 failed to return guid string\n",
                        __FUNCTION__, __LINE__);

                    szGUID[0] = L'\0';
                }

                diskID = W2A(szGUID);

                // Remove enclosing flower brackets from disk guid.
                boost::trim_if(diskID, boost::is_any_of("{}"));
            }
            else
            {
                TRACE_ERROR("Disk %s does not have recognizable partition style.\n",
                    deviceName.c_str());
            }

        } while (false);

        if (pDli_info) free(pDli_info);
        if (INVALID_HANDLE_VALUE != hDisk) CloseHandle(hDisk);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : GetDiskId

    Description : Retrieves the signature or guid of a give disk name.

    Parameters  : [in] dwDiskNum : Disk serial number
                  [out] diskID   : filled with signature of guid of the disk.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD GetDiskId(DWORD dwDiskNum, std::string& diskID)
    {
        std::stringstream diskName;
        diskName << "\\\\.\\PhysicalDrive" << dwDiskNum;

        return GetDiskId(diskName.str(), diskID);
    }

    /*
    Method      : GetLunDeviceIdMappingOfDataDisks

    Description : Retriesves the LUN -> Disk-Signature/Guid mapping of all the data disks on system (Azure VM)

    Parameters  : [out] lun_device: filled with Lun -> signature/guid entries of disks on success.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD GetLunDeviceIdMappingOfDataDisks(std::map<UINT, std::string>& lun_device, UINT port_num)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        do
        {
            CWin32_DiskDriveWmiRecordProcessor diskDriveRecProcessor;
            CWmiEngine wmiEngine(&diskDriveRecProcessor);

            if (!wmiEngine.init())
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine inititalization error.\n");

                break;
            }

            if (!wmiEngine.ProcessQuery("Select * FROM Win32_DiskDrive where SIZE>0"))
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine processing error.\n %s \n",
                    diskDriveRecProcessor.GetErrMsg().c_str());

                break;
            }

            diskDriveRecProcessor.GetLunDeviceMapForDataDisks(lun_device, port_num);

            auto iterLunDevice = lun_device.begin();
            for (; iterLunDevice != lun_device.end(); iterLunDevice++)
            {
                if (iterLunDevice->second.empty())
                    continue;

                dwRet = GetDiskId(iterLunDevice->second, iterLunDevice->second);
                if (ERROR_SUCCESS != dwRet)
                {
                    TRACE_ERROR("Could not get disk signature/guid for the disk %s\n",
                        iterLunDevice->second.c_str());
                    break;
                }
            }

        } while (false);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : GetVolumeDiskExtents

    Description : Retrieves the volume disk extents of a given volume (volumeGuid), and the disk extent will have
                  disk signature/guid, starting offset and extent lenght.

    Parameters  : [in] volumeGuid: volume guid paht of a volume
                  [out] diskExtents: filled with disk extents of a volume on success.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD GetVolumeDiskExtents(const std::string& volumeGuid, disk_extents_t& diskExtents)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;
        HANDLE hVolume = INVALID_HANDLE_VALUE;
        DWORD dwBytesReturned = 0;
        DWORD cbInBuff = 0;
        PVOLUME_DISK_EXTENTS pVolDiskExt = NULL;
        std::string volumeName = volumeGuid;

        if (volumeName.empty())
        {
            TRACE_ERROR("%s:Line %d: ERROR: Volume Name can not be empty.\n", __FUNCTION__, __LINE__);
            return ERROR_INVALID_PARAMETER;
        }

        do {

            //Remove trailing "\" if exist.
            if (volumeName[volumeName.length() - 1] == '\\')
                volumeName.erase(volumeName.length() - 1);

            //Get the volume handle
            HANDLE hVolume = INVALID_HANDLE_VALUE;
            hVolume = CreateFile(
                volumeName.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
                NULL
            );

            if (hVolume == INVALID_HANDLE_VALUE)
            {
                dwRet = GetLastError();
                TRACE_ERROR("%s:Line %d: Could not open the volume- %s. CreateFile failed with Error Code - %lu \n",
                    __FUNCTION__, __LINE__,
                    volumeName.c_str(),
                    dwRet);
                break;
            }

            //  Allocate default buffer sizes. 
            //  If the volume is created on basic disk then there will be only one disk extent for the volume, 
            //     and this default buffer will be enough to accomodate extent info.
            cbInBuff = sizeof(VOLUME_DISK_EXTENTS);
            pVolDiskExt = (PVOLUME_DISK_EXTENTS)malloc(cbInBuff);
            if (NULL == pVolDiskExt)
            {
                dwRet = ERROR_OUTOFMEMORY;
                TRACE_ERROR("%s:Line %d: Out of memory\n", __FUNCTION__, __LINE__);
                break;
            }

            if (!DeviceIoControl(
                hVolume,
                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL,
                0,
                pVolDiskExt,
                cbInBuff,
                &dwBytesReturned,
                NULL
            ))
            {
                dwRet = GetLastError();

                if (dwRet == ERROR_MORE_DATA)
                {
                    //  If the volume is created on dynamic disk then there will be 
                    //  posibility that more than one extent exist for the volume.
                    //  Calculate the size required to accomodate all extents.
                    cbInBuff = FIELD_OFFSET(VOLUME_DISK_EXTENTS, Extents) +
                        pVolDiskExt->NumberOfDiskExtents * sizeof(DISK_EXTENT);

                    //  Re-allocate the memory to new size
                    pVolDiskExt = (PVOLUME_DISK_EXTENTS)realloc(pVolDiskExt, cbInBuff);
                    if (NULL == pVolDiskExt)
                    {
                        dwRet = ERROR_OUTOFMEMORY;
                        TRACE_ERROR("%s:Line %d: Out of memory\n", __FUNCTION__, __LINE__);
                        break;
                    }

                    if (!DeviceIoControl(
                        hVolume,
                        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                        NULL,
                        0,
                        pVolDiskExt,
                        cbInBuff,
                        &dwBytesReturned,
                        NULL
                    ))
                    {
                        dwRet = GetLastError();
                        TRACE_ERROR("%s:Line %d: Could not get the volume disk extents. DeviceIoControl failed with Error %lu\n",
                            __FUNCTION__,
                            __LINE__,
                            dwRet);
                        break;
                    }
                    else
                    {
                        dwRet = ERROR_SUCCESS;
                    }
                }
                else
                {
                    TRACE_ERROR("%s:Line %d: Could not get the volume disk extents. DeviceIoControl failed with Error %lu\n",
                        __FUNCTION__,
                        __LINE__,
                        dwRet);
                    break;
                }
            }

            //Fill disk_extents_t structure with retrieved disk extents
            for (DWORD i_extent = 0; i_extent < pVolDiskExt->NumberOfDiskExtents; i_extent++)
            {
                //Here disk_id will be a signature if disk is MBR type, a GUID if GPT type.
                std::string disk_id;
                dwRet = GetDiskId(pVolDiskExt->Extents[i_extent].DiskNumber, disk_id);
                if (ERROR_SUCCESS != dwRet)
                {
                    TRACE_ERROR("%s:Line %d: Could not get the disk signature/guid of disk# %lu\n",
                        __FUNCTION__,
                        __LINE__,
                        pVolDiskExt->Extents[i_extent].DiskNumber
                    );
                    break;
                }

                disk_extent extent(
                    disk_id,
                    pVolDiskExt->Extents[i_extent].StartingOffset.QuadPart,
                    pVolDiskExt->Extents[i_extent].ExtentLength.QuadPart
                );

                std::stringstream extentOut;
                extentOut << "Disk-Id: " << extent.disk_id
                    << ", Offset: " << extent.offset
                    << ", Length: " << extent.length;

                TRACE_INFO("Extent Info: %s\n", extentOut.str().c_str());

                diskExtents.push_back(extent);
            }

        } while (false);

        if (NULL != pVolDiskExt)
            free(pVolDiskExt);

        if (INVALID_HANDLE_VALUE != hVolume)
            CloseHandle(hVolume);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : GetVolumeFromDiskExtents

    Description : Identifies the volume on hydration vm using volume disk extents and diskId.

    Parameters  : [in] osVolExtents: disk extents of source volume ( this information should be collected on source)
                  [out] volumeGuid : volume guid path of identified source os volume on hydration-vm.
                  [in, opt] diskId : Disk ID on which the volume disk extent should be matched, if this is empty then
                                     the diskId available in disk_extents_t structrue will be used.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD GetVolumeFromDiskExtents(const disk_extents_t& osVolExtents, std::string& volumeGuid, const std::string& diskId)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;
        HANDLE hVolume = INVALID_HANDLE_VALUE;
        TCHAR volumeName[MAX_PATH] = { 0 };
        bool bFoundVolume = false;

        if (osVolExtents.empty())
        {
            dwRet = ERROR_INVALID_PARAMETER;
            TRACE_WARNING("OS volume extents should not be empty\n");
            return dwRet;
        }

        hVolume = FindFirstVolume(volumeName, ARRAYSIZE(volumeName));
        if (INVALID_HANDLE_VALUE == hVolume)
        {
            dwRet = GetLastError();
            TRACE_ERROR("Could not get volume handle. FindFirstVolume failed with error %lu\n", dwRet);
            return dwRet;
        }

        do
        {
            TRACE_INFO("Verifying disk extents for the volume %s\n", volumeName);

            //Get volume disk extents and compare
            disk_extents_t volExtents;
            if (ERROR_SUCCESS == GetVolumeDiskExtents(volumeName, volExtents))
            {
                if (volExtents.size() != 0 &&
                    volExtents.size() == osVolExtents.size())
                {
                    bFoundVolume = true;
                    for (size_t iOsVolExt = 0;
                        iOsVolExt < osVolExtents.size() && bFoundVolume;
                        iOsVolExt++)
                    {
                        std::string strDiskID = diskId.empty() ? osVolExtents[iOsVolExt].disk_id : diskId;

                        // 
                        // Compare the disk signature/guid and OS volume start offset.
                        // The partition length is not considering as it may change if the volume extend.
                        //
                        bFoundVolume = false;
                        for (size_t iVolExt = 0; iVolExt < volExtents.size(); iVolExt++)
                        {
                            if (0 == lstrcmpi(volExtents[iVolExt].disk_id.c_str(),
                                strDiskID.c_str())
                                &&
                                volExtents[iVolExt].offset == osVolExtents[iOsVolExt].offset)
                            {
                                bFoundVolume = true;
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                TRACE_ERROR("Could not get the disk extents for the volume %s\n", volumeName);
                //
                // Ignoring GetVolumeDiskExtents() failure and proceeding for next volume. 
                // If the required volume does not found then the function will return failure at end.
                //
            }

            //
            // If the OS volume found then stop volume enumeration and return the verified volume name.
            //
            if (bFoundVolume)
            {
                TRACE_INFO("Volume %s is matching with Source OS disk extents\n", volumeName);
                volumeGuid = volumeName;
                break;
            }

            //Get Next Volume
            SecureZeroMemory(volumeName, ARRAYSIZE(volumeName));
            if (!FindNextVolume(hVolume, volumeName, ARRAYSIZE(volumeName)))
            {
                dwRet = GetLastError();
                if (dwRet != ERROR_NO_MORE_FILES)
                {
                    TRACE_ERROR("Could not enumerate all volumes. FindNextVolume failed with error %lu\n", dwRet);
                }
                else
                {
                    TRACE_ERROR("Reached end of volumes enumeration. The Source OS volume was not found\n");
                }

                break;
            }

        } while (true);

        FindVolumeClose(hVolume);

        if (!bFoundVolume &&
            (dwRet == ERROR_NO_MORE_FILES || dwRet == ERROR_SUCCESS))
            dwRet = ERROR_FILE_NOT_FOUND;

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : GetVolumeMountPoints

    Description : Retrieves the list of mountpoints/driveleters of a give volume (volumeGuid)

    Parameters  : [in] volumeGuid: Volume Guid path of a volume
                  [out] mountPoints: List of mount-points/drive-letters of the volume.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD GetVolumeMountPoints(const std::string& volumeGuid, std::list<std::string>& mountPoints)
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!volumeGuid.empty());

        DWORD dwRet = ERROR_SUCCESS;
        BOOL bSuccess = FALSE;
        DWORD ccBuffer = MAX_PATH + 1;
        TCHAR *buffer = NULL;
        do
        {
            buffer = (TCHAR*)malloc(ccBuffer * sizeof(TCHAR));
            if (NULL == buffer)
            {
                dwRet = ERROR_OUTOFMEMORY;
                TRACE_ERROR("Out of memory\n");
                break;
            }
            RtlSecureZeroMemory(buffer, ccBuffer * sizeof(TCHAR));

            bSuccess = GetVolumePathNamesForVolumeName(volumeGuid.c_str(), buffer, ccBuffer, &ccBuffer);
            if (!bSuccess)
            {
                dwRet = GetLastError();
                if (ERROR_MORE_DATA == dwRet)
                {
                    free(buffer); buffer = NULL;
                    dwRet = ERROR_SUCCESS;
                    continue;
                }

                TRACE_ERROR("GetVolumePathNamesForVolumeName failed with error %lu for the volume %s\n",
                    dwRet,
                    volumeGuid.c_str());
                break;
            }

            for (TCHAR* path = buffer; path[0] != '\0'; path += lstrlen(path) + 1)
                mountPoints.push_back(path);

            break;

        } while (true);

        if (buffer) free(buffer);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : AutoSetMountPoint

    Description : Sets a mount-point for a given volume (volumeGuid) and returns the mountpoint

    Parameters  : [in] volumeGuid: Volume guid path of a volume to which the mount point should set.
                  [out] mountPoint: filled with auto generated mountpoint for the volume, if the
                                    volume mounting was successful.

    Return      : ERROR_SUCCESS -> on success
                  win32 error   -> on failure.

    */
    DWORD AutoSetMountPoint(const std::string& volumeGuid, std::string& mountPoint)
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!volumeGuid.empty());
        DWORD dwRet = ERROR_SUCCESS;

        dwRet = GenerateUniqueMntDirectory(mountPoint);
        if (ERROR_SUCCESS != dwRet)
        {
            TRACE_ERROR("Can not create directory\n");
            return dwRet;
        }

        BOOST_ASSERT(!mountPoint.empty());

        //
        // Append '\' if not exist. SetVolumeMountPoint API expects '\' at end of mount path.
        //
        if (!mountPoint.empty() &&
            mountPoint[mountPoint.length() - 1] != '\\')
            mountPoint += "\\";

        if (!SetVolumeMountPoint(mountPoint.c_str(), volumeGuid.c_str()))
        {
            dwRet = GetLastError();

            TRACE_ERROR("Could not set the mount point [%s] to the volume [%s]\n \
                    SetVolumeMountPoint API failed with error %lu",
                mountPoint.c_str(),
                volumeGuid.c_str(),
                dwRet
            );

            mountPoint.clear();
        }

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : OnlineBasicDisk

    Description : Online the given basic disk using Storage Management WMI API

    Parameters  : [in] diskNumber: Offline basic disk number.

    Return      : ERROR_SUCCESS          -> on success
                  ERROR_INTERNAL_ERROR   -> on failure.

    */
    DWORD OnlineBasicDisk(UINT diskNumber)
    {
        TRACE_FUNC_BEGIN;

        DWORD dwRet = ERROR_SUCCESS;

        do
        {
            CMSFT_DiskOnlineMethod onlineDisk;
            CWmiEngine wmiEngine(&onlineDisk, WMI_PROVIDER_NAMESPACE::STORAGE);

            if (!wmiEngine.init())
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine inititalization error.\n");

                break;
            }

            std::stringstream query;
            query << "SELECT * FROM MSFT_Disk WHERE Number = " << diskNumber;

            if (!wmiEngine.ProcessQuery(query.str()))
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine processing error.\n %s.\nQuery: %s\n",
                    onlineDisk.GetErrMsg().c_str(),
                    query.str().c_str());

                break;
            }

            dwRet = onlineDisk.GetReturnCode();
            if (ERROR_SUCCESS != dwRet)
            {
                TRACE_ERROR("MSFT_Disk(%d).Online method failed with error %lu.\n",
                    diskNumber,
                    dwRet);

                dwRet = ERROR_INTERNAL_ERROR;
                break;
            }

        } while (false);

        TRACE_FUNC_END;

        return dwRet;
    }

    /*
    Method      : VerifyDiskSignaturGuidCollision

    Description : Retrieves disk-signature/guids of all the disks on system and compares
                  with give disk signature/guid to validate the collision.

    Parameters  : [in] msft_disk : Disk information to validate signature/guid collision with other disks.

    Return      : ERROR_SUCCESS          -> on success
                  ERROR_INTERNAL_ERROR   -> on failure.
                  ERROR_ALREADY_EXISTS   -> on disk signature/guid collision
    */
    DWORD VerifyDiskSignaturGuidCollision(const mdft_disk_info_t& msft_disk)
    {
        TRACE_FUNC_BEGIN;

        DWORD dwRet = ERROR_SUCCESS;

        do
        {
            CWin32_DiskDriveWmiRecordProcessor diskDriveRecProcessor;
            CWmiEngine wmiEngine(&diskDriveRecProcessor);

            if (!wmiEngine.init())
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine inititalization error.\n");

                break;
            }

            if (!wmiEngine.ProcessQuery("Select * FROM Win32_DiskDrive where SIZE>0"))
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine processing error.\n %s \n",
                    diskDriveRecProcessor.GetErrMsg().c_str());

                break;
            }

            std::map<std::string, win32_disk_info> disks_info;

            diskDriveRecProcessor.GetDiskDriveInfo(disks_info);

            auto iterDisksInfo = disks_info.begin();
            for (; iterDisksInfo != disks_info.end(); iterDisksInfo++)
            {
                std::string signature_guid;
                dwRet = GetDiskId(iterDisksInfo->first, signature_guid);

                if (ERROR_SUCCESS != dwRet)
                {
                    dwRet = ERROR_INTERNAL_ERROR;

                    TRACE_ERROR("Could not get signature/guid of the disk device %s\n",
                        iterDisksInfo->first);
                    break;
                }

                if (boost::iequals(signature_guid, msft_disk.Signature_Guid) &&
                    msft_disk.Number != iterDisksInfo->second.Index)
                {
                    TRACE_WARNING("Found signature/guid collision for the disks: %u, %u\n",
                        msft_disk.Number, iterDisksInfo->second.Index);

                    dwRet = ERROR_ALREADY_EXISTS;

                    break;
                }
            }

        } while (false);

        TRACE_FUNC_END;

        return dwRet;
    }

    /*
    Method      : VerifyOfflineDisksAndBringOnline

    Description : Verifies all the basic disks on the system and tries to bring offline disks to online.
                  Before bringing the disks online, it also validates the reason for disk offline and the
                  disk will be brought online only if the offline reason if a "policy" set by admin/user.

    Parameters  : [in, optional] disk_id: Signature/Guid of the a basic disk to online. If this parameter
                  is specified then the disk with same Signature/Guid will be brought online, remaining
                  disks will not be attempted for online. If this parameter is not specified or empty then
                  all the offline basic disks will be attempted for online.

    Return      : ERROR_SUCCESS          -> on success
                  ERROR_INTERNAL_ERROR   -> on failure.
                  ERROR_ALREADY_EXISTS   -> on disk signature/guid collision
    */
    DWORD VerifyOfflineDisksAndBringOnline(const std::string& disk_id)
    {
        TRACE_FUNC_BEGIN;

        DWORD dwRet = ERROR_SUCCESS;

        do
        {
            CMSFT_DiskRecordProcessor diskRecrdProcessor;
            CWmiEngine wmiEngine(&diskRecrdProcessor, WMI_PROVIDER_NAMESPACE::STORAGE);

            if (!wmiEngine.init())
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine inititalization error.\n");

                break;
            }

            std::stringstream query;
            query << "SELECT * FROM MSFT_Disk WHERE IsOffline = True";

            if (!wmiEngine.ProcessQuery(query.str()))
            {
                dwRet = ERROR_INTERNAL_ERROR;

                TRACE_ERROR("WMI Engine processing error.\n %s.\nQuery: %s\n",
                    diskRecrdProcessor.GetErrMsg().c_str(),
                    query.str().c_str());

                break;
            }

            std::map<UINT, mdft_disk_info_t> disks;
            diskRecrdProcessor.GetDiskInfo(disks);

            auto iterDisk = disks.begin();
            for (; iterDisk != disks.end() &&
                ERROR_SUCCESS == dwRet

                ; iterDisk++)
            {
                if (iterDisk->second.IsOffline)
                {
                    if (Disk_Offline_Reason_Policy == iterDisk->second.OfflineReason)
                    {
                        TRACE_INFO("Found an offline disk and its offline reason is: Policy set by Administrator.\n");

                        //
                        // Verify disk signature or guid collision.
                        //
                        dwRet = VerifyDiskSignaturGuidCollision(iterDisk->second);
                        if (ERROR_SUCCESS != dwRet)
                        {
                            if (ERROR_ALREADY_EXISTS == dwRet)
                            {
                                //
                                // A disk signature/guid collision found. 
                                //
                                // Sometimes the disk offline reason is set to Disk_Offline_Reason_Policy,
                                // but it also has signatur/guid collision. So we need to avoid bringing 
                                // disk online when there is a real collision.
                                //
                                dwRet = Disk_Offline_Reason_Collision;
                            }

                            TRACE_ERROR("Could not bring the Disk-%u online\n", iterDisk->first);

                            break;
                        }

                        //
                        // Try Bringing disk online
                        //
                        if (disk_id.empty() ||
                            boost::iequals(boost::trim_copy_if(disk_id, boost::is_any_of("{}")),
                                boost::trim_copy_if(iterDisk->second.Signature_Guid,
                                    boost::is_any_of("{}")
                                )))
                        {
                            TRACE_INFO("Onlining the Disk-%u\n", iterDisk->first);
                            dwRet = OnlineBasicDisk(iterDisk->first);
                        }
                        else
                        {
                            TRACE_INFO("Skipping the disk online for %s\n", iterDisk->second.Signature_Guid.c_str());
                        }
                    }
                    else
                    {
                        dwRet = iterDisk->second.OfflineReason;

                        TRACE_WARNING("Found an offline disk and its offline reason is: %d.\n", dwRet);
                        break;
                    }
                }
            }

        } while (false);

        TRACE_FUNC_END;

        return dwRet;
    }

    /*
    Method      : SetPrivileges

    Description : Enables/disables particular privilage for the process.

    Parameters  : [in] privilege : Name of the privilage
                  [in] set       : if true then privilage will be enabled, if false then it will be disabled

    Return      : ERROR_SUCCESS          -> on success
                  ERROR_INTERNAL_ERROR   -> on failure.

    */
    DWORD SetPrivileges(const std::string& privilege, bool set)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        TOKEN_PRIVILEGES tp;
        HANDLE hToken = INVALID_HANDLE_VALUE;
        LUID luid;

        do
        {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
            {
                dwRet = GetLastError();
                TRACE_ERROR("OpenProcessToken failed - %lu\n", dwRet);
                break;
            }

            if (!LookupPrivilegeValue(NULL, privilege.c_str(), &luid))
            {
                dwRet = GetLastError();
                TRACE_ERROR("LookupPrivilegeValue failed - %lu\n", dwRet);
                break;
            }

            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = set ? SE_PRIVILEGE_ENABLED : 0;

            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);

            dwRet = GetLastError();
            if (dwRet != ERROR_SUCCESS)
            {
                TRACE_ERROR("AdjustTokenPrivileges failed - %lu\n", dwRet);
                break;
            }

        } while (0);

        if (INVALID_HANDLE_VALUE != hToken)
            CloseHandle(hToken);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : GetSourceOSInstallPath

    Description : Constructs the source OS install path by replacing its source os volume with its corresponding
                  volume name on hydration-vm.

    Parameters  : [in] OSInstallPath : OS install path on source ( Ex: C:\windows\system32 )
                  [in] OSDriveName : OS drive name on source ( Ex: C:\ )
                  [in] SourceOSVolMountPath : Source OS volume name on Hydration-VM ( Ex: F:\ )

    Return      : Returns calculated source os install path on hydration-vm on success ( F:\windows\system32 ), and
                  returns an empty string on failure.

    */
    std::string GetSourceOSInstallPath(
        const std::string& OSInstallPath,
        const std::string& OSDriveName,
        const std::string& SourceOSVolMountPath
    )
    {
        TRACE_FUNC_BEGIN;
        BOOST_ASSERT(!OSInstallPath.empty());
        BOOST_ASSERT(!OSDriveName.empty());
        BOOST_ASSERT(!SourceOSVolMountPath.empty());

        std::string srcOSInstallPath;

        if (boost::istarts_with(OSInstallPath, OSDriveName))
        {
            srcOSInstallPath =
                boost::ireplace_first_copy(OSInstallPath, OSDriveName, SourceOSVolMountPath);

            TRACE_INFO("Source OS Install Path: %s\n", srcOSInstallPath.c_str());
        }
        else
        {
            TRACE_ERROR("OS Install Path [%s] is not started with OS Drive name [%s]",
                OSInstallPath.c_str(),
                OSDriveName.c_str());

            srcOSInstallPath.clear();
        }

        TRACE_FUNC_END;
        return srcOSInstallPath;
    }

    /*
    Method      : PrintDiskPartitions

    Description : Prints all available partitions in the system.

    Parameters  : None.

    Return      : None.

    */
    void PrintAllDiskPartitions()
    {
        TRACE_FUNC_BEGIN;

        do
        {
            CMSFT_PartitionRecordProcessor diskPartitionProcessor;
            CWmiEngine wmiEngine(&diskPartitionProcessor, WMI_PROVIDER_NAMESPACE::STORAGE);

            if (!wmiEngine.init())
            {
                TRACE_ERROR("WMI Engine inititalization error.\n");

                break;
            }

            if (!wmiEngine.ProcessQuery("Select * FROM MSFT_Partition"))
            {
                TRACE_ERROR("WMI Engine processing error.\n %s \n",
                    diskPartitionProcessor.GetErrMsg().c_str());

                break;
            }

            std::list<msft_disk_partition> partitions;
            diskPartitionProcessor.GetDiskParitions(partitions);

            TRACE_INFO("Printing all the paritions.\n");
            auto iterPartition = partitions.begin();
            for (; iterPartition != partitions.end(); iterPartition++)
            {
                iterPartition->Print();
            }

        } while (false);

        TRACE_FUNC_END;
    }

    /*
    Method      : EnableSerialConsole

    Description : Runs BCD Edit and sets parameters for serial console .

    Parameters  : [in] srcSystemPath: Source system32 directory.

    Return      : ERROR_SUCCESS - on success.
                  Any other error code is failure, and it is the exit code of BcdEdit.exe.

    Remarks     : https://docs.microsoft.com/en-us/azure/virtual-machines/windows/prepare-for-upload-vhd-image#verify-the-vm
    */
    DWORD EnableSerialConsole(
        const std::string& srcSystemPath)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        std::string exclude_disk_id;
        if (ERROR_SUCCESS != GetSystemDiskId(exclude_disk_id))
        {
            TRACE_WARNING("Couldn't find the System Disk Id.");
            
            // Continue, at worse we re-update the serial console settings for Hydration VM also.
        }
        BOOST_ASSERT(!exclude_disk_id.empty());

        std::list<std::string> bcdVolume;
        bcdVolume.clear();

        dwRet = FindVolumesWithPath(
            BCD_TOOLS::EFI_BCD_PATH,
            bcdVolume,
            exclude_disk_id);

        if (dwRet != ERROR_SUCCESS ||
            bcdVolume.size() == 0)
        {
                TRACE_INFO("EFI BCD file not found. Check for BIOS.\n");
        }
        else
        {
            // There may be number of partitions with the file present.
            // For example: EFI System partition and C:\. Enable on all stores.
            BOOST_FOREACH(const std::string & bcdVol, bcdVolume)
            {
                std::stringstream bcd_efi_file;
                bcd_efi_file
                    << boost::trim_copy_if(bcdVol, boost::is_any_of("\\"))
                    << ACE_DIRECTORY_SEPARATOR_STR_A
                    << BCD_TOOLS::EFI_BCD_PATH;

                if (UpdateBCDWithConsoleParams(srcSystemPath, bcd_efi_file.str()) != ERROR_SUCCESS)
                {
                    TRACE_WARNING("Failed to update BCD with one or more of serial console commands.\n");
                    // Continue with BIOS updates.
                }
                else
                {
                    return ERROR_SUCCESS;
                }
            }
        }

        bcdVolume.clear();
        dwRet = FindVolumesWithPath(
            BCD_TOOLS::BIOS_BCD_PATH,
            bcdVolume,
            exclude_disk_id);

        if (dwRet != ERROR_SUCCESS ||
            bcdVolume.size() == 0)
        {
            TRACE_WARNING("BIOS BCD file not found.\n");
        }
        else
        {
            BOOST_FOREACH(const std::string & bcdVol, bcdVolume)
            {
                std::stringstream bcd_bios_file;
                bcd_bios_file
                    << boost::trim_copy_if(bcdVol, boost::is_any_of("\\"))
                    << ACE_DIRECTORY_SEPARATOR_STR_A
                    << BCD_TOOLS::BIOS_BCD_PATH;

                if (UpdateBCDWithConsoleParams(srcSystemPath, bcd_bios_file.str()) != ERROR_SUCCESS)
                {
                    TRACE_WARNING("Failed to update BCD with one or more of serial console commands.\n");
                }
                else
                {
                    return ERROR_SUCCESS;
                }
            }
        }

        return dwRet;

        TRACE_FUNC_END;
    }

    /*
        Method      : UpdateBCDWithConsoleParams

        Description : Runs commands required to update Serial console on a Windows VM.

        Parameters  : [in] srcSystemPath: Source system32 directory.
                      [in] bcd_file: Complete path of bcdstore file.

        Return      : ERROR_SUCCESS - on success.
                      Any other error code is failure, and it is the exit code of BcdEdit.exe.
    */
    DWORD UpdateBCDWithConsoleParams(const std::string& srcSystemPath,
        const std::string& bcd_file)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        /*
        * Commands to run:
        * bcdedit.exe /set "{bootmgr}" displaybootmenu yes
        * bcdedit.exe /set "{bootmgr}" timeout 5
        * bcdedit.exe /set "{bootmgr}" bootems yes
        * bcdedit.exe /ems "{current}" ON
        * bcdedit.exe /emssettings EMSPORT:1 EMSBAUDRATE:115200
        */

        std::stringstream serialconsole_cmd_prefix;
        serialconsole_cmd_prefix
            << boost::trim_copy_if(srcSystemPath, boost::is_any_of("\\"))
            << ACE_DIRECTORY_SEPARATOR_STR_A
            << BCD_TOOLS::BCDEDIT_EXE
            << " /store " << bcd_file;

        std::string serial_console_cmd_arr[5] =
        {
            serialconsole_cmd_prefix.str() + " /set \"{bootmgr}\" displaybootmenu yes",
            serialconsole_cmd_prefix.str() + " /set \"{bootmgr}\" timeout 5",
            serialconsole_cmd_prefix.str() + " /set \"{bootmgr}\" bootems yes",
            serialconsole_cmd_prefix.str() + " /ems \"{current}\" ON",
            serialconsole_cmd_prefix.str() + " /emssettings EMSPORT:1 EMSBAUDRATE:115200"
        };

        for (int sc_cmd_itr = 0; sc_cmd_itr < 5; sc_cmd_itr++)
        {
            std::stringstream cmd_output;
            dwRet = RunCommand(serial_console_cmd_arr[sc_cmd_itr], "", cmd_output);
            TRACE_INFO("Output\n%s\n", cmd_output.str().c_str());

            if (ERROR_SUCCESS != dwRet)
            {
                TRACE_ERROR("Command %s exited with exit code: %d\n",
                    serial_console_cmd_arr[sc_cmd_itr].c_str(),
                    dwRet);
            }
        }

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : RunBcdEdit

    Description : Displays the existing EFI BCD entries.

    Parameters  : [in] srcSystemPath: Source system32 directory.
                  [in] activePartitionDrivePath: Active partition drive path.
                  [in] bcdFileRelativePath: BCD store file relative path in active partition.
                       If this param holds empty string then it defaults to EFI BCD path.

    Return      : ERROR_SUCCESS - on success.
                  Any other error code is failure, and it is the exit code of BcdEdit.exe.
    */
    DWORD RunBcdEdit(const std::string& srcSystemPath,
        const std::string& activePartitionDrivePath,
        const std::string& bcdFileRelativePath)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS, finalRet = ERROR_SUCCESS;

        std::string bcd_rel_file = boost::trim_copy(bcdFileRelativePath);
        if (bcd_rel_file.empty())
        {
            bcd_rel_file = BCD_TOOLS::EFI_BCD_PATH;
        }

        std::stringstream bcd_file;
        bcd_file
            << boost::trim_copy_if(activePartitionDrivePath, boost::is_any_of("\\"))
            << ACE_DIRECTORY_SEPARATOR_STR_A
            << bcd_rel_file;

        if (FileExists(bcd_file.str().c_str()))
        {
            TRACE_INFO("File %s exist.\n", bcd_file.str().c_str());
        }
        else
        {
            TRACE_ERROR("File %s does not exist.\n", bcd_file.str().c_str());

            SetLastErrorMsg("File %s does not exist.\n", bcd_file.str().c_str());

            return ERROR_FILE_NOT_FOUND;
        }

        std::stringstream bcdedit_cmd;
        bcdedit_cmd
            << boost::trim_copy_if(srcSystemPath, boost::is_any_of("\\"))
            << ACE_DIRECTORY_SEPARATOR_STR_A
            << BCD_TOOLS::BCDEDIT_EXE
            << " /store " << bcd_file.str()
            << " /enum active /v";

        std::stringstream cmd_output;
        dwRet = RunCommand(bcdedit_cmd.str(), "", cmd_output);
        TRACE_INFO("Output\n%s\n", cmd_output.str().c_str());

        if (ERROR_SUCCESS != dwRet)
        {
            finalRet = dwRet;
            TRACE_ERROR("Command %s exited with exit code: %d\n",
                bcdedit_cmd.str().c_str(),
                dwRet);

            SetLastErrorMsg("BcdEdit command exited with error %d", dwRet);
        }

        TRACE_FUNC_END;
        return finalRet;

    }

    /*
    Method      : RunBcdBootAndUpdateBCD

    Description : BcdBoot command will be run to update the BCD entries on
                  active partition for the mentioned firmware type.

    Parameters  : [in] srcSystemPath: Source system32 directory.
                  [in] activePartitionDrivePath: Active partition drive path.
                  [in] firmwareType: Firmware type for BCD update

    Return      : ERROR_SUCCESS - on success.
                  Any other error code is failure, and it is the exit code of BcdBoot.exe.
    */
    DWORD RunBcdBootAndUpdateBCD(const std::string& srcSystemPath,
        const std::string& activePartitionDrivePath,
        const std::string& firmwareType)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        std::string srcSystem32Dir = boost::trim_copy_if(srcSystemPath, boost::is_any_of("\\"));
        if (!boost::iends_with(srcSystem32Dir, SysConstants::DEFAULT_SYSTEM32_DIR))
        {
            TRACE_ERROR("%s is not a valid source system path.\n", srcSystemPath.c_str());

            SetLastErrorMsg("%s is not a valid system path.\n", srcSystemPath.c_str());

            return ERROR_PATH_NOT_FOUND;
        }

        if (FileExists(srcSystem32Dir))
        {
            TRACE_INFO("Path %s exist and its valid.\n", srcSystem32Dir.c_str());
        }
        else
        {
            TRACE_ERROR("Path %s does not exist.\n", srcSystem32Dir.c_str());

            SetLastErrorMsg("Path %s does not exist.", srcSystem32Dir.c_str());

            return ERROR_PATH_NOT_FOUND;
        }

        std::string srcWindowsDir = boost::ierase_last_copy(srcSystem32Dir, "\\System32");

        std::stringstream bcdboot_cmd;
        bcdboot_cmd << srcSystem32Dir
            << ACE_DIRECTORY_SEPARATOR_STR_A
            << BCD_TOOLS::BCDBOOT_EXE
            << " "
            << srcWindowsDir
            << " /s "
            << boost::trim_copy_if(activePartitionDrivePath, boost::is_any_of("\\"))
            << " /f "
            << firmwareType
            << " /v";

        std::stringstream cmd_output;
        dwRet = RunCommand(bcdboot_cmd.str(), "", cmd_output);
        TRACE_INFO("Output\n%s\n", cmd_output.str().c_str());

        if (ERROR_SUCCESS != dwRet)
        {
            TRACE_ERROR("Command %s exited with exit code: %d\n",
                bcdboot_cmd.str().c_str(),
                dwRet);

            SetLastErrorMsg("bcdboot command failed with error %d.", dwRet);
        }

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : FindVolumesWithPath

    Description : Prints all available partitions in the system.

    Parameters  : [in] relative_path
                  [out] volumes: List volumes where relative_path exists.
                  [int] exclude_disk_id: disk to exclude in the search.

    Return      : ERROR_SUCCESS on success, otherwise win32 error.

    */
    DWORD FindVolumesWithPath(const std::string& relative_path,
        std::list<std::string>& volumes,
        const std::string& exclude_disk_id)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;
        HANDLE hVolume = INVALID_HANDLE_VALUE;
        std::vector<TCHAR> volumeNameBuff(MAX_PATH, 0);
        bool bFoundVolume = false;
        volumes.clear();

        hVolume = FindFirstVolume(&volumeNameBuff[0], volumeNameBuff.size());
        if (INVALID_HANDLE_VALUE == hVolume)
        {
            dwRet = GetLastError();

            TRACE_ERROR(
                "Could not get volume handle. FindFirstVolume failed with error %lu\n",
                dwRet);

            return dwRet;
        }

        do
        {
            // Store the volume name in a variable and reset
            // the volume name buffer for FindNextVolume call.
            std::string volumeName = volumeNameBuff.data();
            volumeNameBuff.assign(MAX_PATH, 0);

            TRACE_INFO("Verifying disk extents for the volume %s.\n",
                volumeName.c_str());

            // Get volume disk extents.
            disk_extents_t volExtents;
            dwRet = GetVolumeDiskExtents(volumeName, volExtents);
            if (dwRet != ERROR_SUCCESS ||
                volExtents.size() == 0)
            {
                TRACE_WARNING(
                    "Could not get extents (or) no extents found for the volume %s. Error %d\n",
                    volumeName.c_str(),
                    dwRet);
                //
                // Ignoring GetVolumeDiskExtents() failure and proceeding for next volume. 
                // If the required volume does not found then the function will return failure at end.
                //

                continue;
            }

            // Verify if this volume belong to exclude disk by checking its disk extents.
            bool shouldSkipVol = false;
            BOOST_FOREACH(const disk_extent& extent, volExtents)
            {
                if (boost::iequals(extent.disk_id, exclude_disk_id))
                {
                    shouldSkipVol = true;

                    TRACE_INFO(
                        "Volume extent belong to exclude disk %s, skipping the volume.\n",
                        exclude_disk_id.c_str());
                    break;
                }
            }

            // If the volume need to skip then skip it
            // and go for next volume.
            if (shouldSkipVol)
                continue;

            // Check if the volume has any file maching with relative path.
            std::string osVolMountPath;
            dwRet = SearchVolumeForPath(volumeName,
                relative_path,
                osVolMountPath);
            if (dwRet == ERROR_SUCCESS)
            {
                volumes.push_back(osVolMountPath);

                TRACE_INFO("Path %s found in volume %s.\n",
                    relative_path.c_str(),
                    osVolMountPath.c_str());

                bFoundVolume = true;
            }
            else if (dwRet == ERROR_FILE_NOT_FOUND)
            {
                TRACE_INFO("Path %s not found in volume %s.\n",
                    relative_path.c_str(),
                    osVolMountPath.c_str());
            }
            else
            {
                TRACE_WARNING("Could not verify path %s in %s. Error %d\n",
                    relative_path.c_str(),
                    osVolMountPath.empty() ? volumeName.c_str() : osVolMountPath.c_str(),
                    dwRet);

                // Ignore the error and continue with next volume, if os volume
                // is not found then the operation will fail at later stage anyway.
            }
        } while (FindNextVolume(
            hVolume,
            &volumeNameBuff[0],
            volumeNameBuff.size()));

        dwRet = GetLastError();
        if (dwRet == ERROR_NO_MORE_FILES || dwRet == ERROR_SUCCESS)
        {
            if (!bFoundVolume)
                dwRet = ERROR_FILE_NOT_FOUND;
            else
                dwRet = ERROR_SUCCESS;
        }
        else
        {
            TRACE_ERROR("Could not enumerating all the volumes. Error %d.\n",
                dwRet);
        }

        // Close volume enumeration handle.
        FindVolumeClose(hVolume);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : SearchVolumeForPath

    Description : Checks if the given related path exists in the volume.

    Parameters  : [in] volume_guid_path
                  [in] relative_path
                  [out] volume_mount_path

    Return      : ERROR_SUCCESS => If path exists
                  ERROR_FILE_NOT_FOUND => If path does not exist.
                  Win32 Error => On any failure.
    */
    DWORD SearchVolumeForPath(
        const std::string& volume_guid_path,
        const std::string& relative_path,
        std::string& volume_mount_path)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        do
        {
            std::list<std::string> volMounts;
            dwRet = GetVolumeMountPoints(volume_guid_path, volMounts);
            if (ERROR_SUCCESS != dwRet)
            {
                TRACE_ERROR("Could not get mount points for OS volume %s. Error %d.\n",
                    volume_guid_path.c_str(),
                    dwRet);
                break;
            }
            else if (volMounts.empty())
            {
                // No mount-point or dirve letter is assigned for os volume.
                // Assing a mount point.
                dwRet = AutoSetMountPoint(volume_guid_path, volume_mount_path);
                if (ERROR_SUCCESS != dwRet)
                {
                    TRACE_ERROR(
                        "Could not assign mount point for source OS volume %s. Error %d.\n",
                        volume_guid_path.c_str(),
                        dwRet);
                    break;
                }
            }
            else
            {
                volume_mount_path = *volMounts.begin();
                TRACE_INFO("Found mountpoint %s for the volume %s.\n",
                    volume_mount_path.c_str(),
                    volume_guid_path.c_str());
            }

            std::stringstream abs_path;
            abs_path
                << boost::trim_right_copy_if(volume_mount_path, boost::is_any_of(DIRECOTRY_SEPERATOR))
                << DIRECOTRY_SEPERATOR
                << boost::trim_left_copy_if(relative_path, boost::is_any_of(DIRECOTRY_SEPERATOR));

            if (FileExists(abs_path.str()))
            {
                TRACE_INFO("File %s found in the volume %s.\n",
                    relative_path.c_str(),
                    volume_mount_path.c_str());

                dwRet = ERROR_SUCCESS;
            }
            else
            {
                TRACE_INFO("File %s not found in the volume %s.\n",
                    relative_path.c_str(),
                    volume_mount_path.c_str());

                dwRet = ERROR_FILE_NOT_FOUND;
            }

        } while (false);

        TRACE_FUNC_END;
        return dwRet;
    }


    /*
    Method      : GetSystemDiskId

    Description : Gets the current system disk id.

    Parameters  : [out] disk_id

    Return      : 0 on success, otherwise a win32 error code.

    */
    DWORD GetSystemDiskId(std::string& disk_id)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;
        DWORD sysDirLen = MAX_PATH_SIZE;
        do
        {
            std::vector<TCHAR> sysDir(MAX_PATH_SIZE, 0);
            if (!GetSystemDirectory(&sysDir[0], sysDir.size()))
            {
                dwRet = GetLastError();
                TRACE_ERROR("Could not get System directory. Error %d.\n", dwRet);
                break;
            }

            std::vector<TCHAR> sysVolPath(MAX_PATH_SIZE, 0);
            if (!GetVolumePathName(sysDir.data(), &sysVolPath[0], sysVolPath.size()))
            {
                dwRet = GetLastError();
                TRACE_ERROR(
                    "Could not get volume for the system directory %s. Error %d.\n",
                    sysDir.data(),
                    dwRet);
                break;
            }

            std::vector<TCHAR> sysVolGuidPath(MAX_PATH_SIZE, 0);
            if (!GetVolumeNameForVolumeMountPoint(
                sysVolPath.data(),
                &sysVolGuidPath[0],
                sysVolGuidPath.size()))
            {
                dwRet = GetLastError();
                TRACE_ERROR(
                    "Could not get volume guid path for the system volume %s. Error %d.\n",
                    sysVolPath.data(),
                    dwRet);
                break;
            }

            disk_extents_t sys_vol_disk_extents;
            dwRet = GetVolumeDiskExtents(
                sysVolGuidPath.data(),
                sys_vol_disk_extents);
            if (ERROR_SUCCESS != dwRet)
            {
                TRACE_ERROR(
                    "Could not get disk extents for the system volume %s. Error %d.\n",
                    sysVolGuidPath.data(),
                    dwRet);
                break;
            }
            else if (sys_vol_disk_extents.size() == 0)
            {
                TRACE_ERROR(
                    "No extents found for the system volume %s.\n",
                    sysVolGuidPath.data());

                dwRet = ERROR_FILE_NOT_FOUND;
                break;
            }

            // Pic the disk id from 1st extent itself.
            disk_id = sys_vol_disk_extents[0].disk_id;
            TRACE_INFO("Found OS disk id: %s.\n", disk_id.c_str());
        } while (false);

        TRACE_FUNC_END;
        return dwRet;
    }

    /*
    Method      : GetFileVersion

    Description : Gets the version string of the given file.

    Parameters  : [in] file_path

    Return      : 0 on success, otherwise a win32 error code.

    */
    DWORD GetFileVersion(const std::string& file_path, std::string& version)
    {
        TRACE_FUNC_BEGIN;
        DWORD dwRet = ERROR_SUCCESS;

        do
        {
            if (!FileExists(file_path))
            {
                TRACE_ERROR("Could not find %s.\n", file_path.c_str());
                dwRet = ERROR_FILE_NOT_FOUND;
                break;
            }

            DWORD dwInfoSize = GetFileVersionInfoSize(file_path.c_str(), NULL);
            if (!dwInfoSize)
            {
                dwRet = GetLastError();
                TRACE_ERROR("Could not get version info size. Error %d.\n", dwRet);
                break;
            }

            std::vector<BYTE> verInfo(dwInfoSize, 0);
            if (!GetFileVersionInfo(file_path.c_str(), 0, verInfo.size(), &verInfo[0]))
            {
                dwRet = GetLastError();
                TRACE_ERROR("Could not get version info. Error %d.\n", dwRet);
                break;
            }

            // Fetch version string from the version info.
            TCHAR* pVer = NULL;
            UINT ccVer = 0;
            if (VerQueryValue(verInfo.data(),
                _T("\\StringFileInfo\\040904B0\\FileVersion"),
                (void**)&pVer,
                &ccVer) &&
                pVer)
            {
                // Adding one additional space for null char as a safety measure.
                std::vector<TCHAR> verStr(dwInfoSize + 1, 0);
                inm_tcscpy_s(&verStr[0], verStr.size() - 1, pVer);
                version = verStr.data();
            }
            else
            {
                TRACE_ERROR("Could not retrieve version string from the version info.\n.");
                dwRet = ERROR_INTERNAL_ERROR;
            }
        } while (false);

        TRACE_FUNC_END;
        return dwRet;
    }
}
