#include "stdafx.h"

#include <winioctl.h>
#include <Windows.h>
#include <diskguid.h>
#include <tchar.h>
#include <strsafe.h>

#include "DiskHelpers.h"


/* Issues the IOCTL_DISK_GET_DRIVE_LAYOUT_EX, returns dli if ioctl succeeds, which must be freed by caller
*/
DRIVE_LAYOUT_INFORMATION_EX* IoctlDiskGetDriveLayoutEx(const HANDLE &h)
{
    DRIVE_LAYOUT_INFORMATION_EX *dli = NULL;
    int EXPECTEDPARTITIONS = 2;
    bool bRetry = false;
    unsigned int drivelayoutsize;
    DWORD bytesreturned;
    DWORD err;

    do
    {
        if (dli)
        {
            free(dli);
            dli = NULL;
        }
        drivelayoutsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + ((EXPECTEDPARTITIONS - 1) * sizeof(PARTITION_INFORMATION_EX));
        dli = (DRIVE_LAYOUT_INFORMATION_EX *)calloc(1, drivelayoutsize);

        if (NULL == dli)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to allocate %u bytes for %d expected partitions, "
                "for IOCTL_DISK_GET_DRIVE_LAYOUT_EX\n", drivelayoutsize,
                EXPECTEDPARTITIONS);
            break;
        }

        bytesreturned = 0;
        err = 0;
        if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, dli, drivelayoutsize, &bytesreturned, NULL))
        {
            break;
        }
        else if ((err = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
        {
            DebugPrintf(SV_LOG_DEBUG, "with EXPECTEDPARTITIONS = %d, IOCTL_DISK_GET_DRIVE_LAYOUT_EX says insufficient buffer\n", EXPECTEDPARTITIONS);
            EXPECTEDPARTITIONS *= 2;
            bRetry = true;
            continue;
        }
        else
        {
            std::stringstream ss;
            ss << "IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed with error " << err;
            DebugPrintf(SV_LOG_ERROR, "%s\n", ss.str().c_str());
            break;
        }
    } while (bRetry);

    return dli;
}

DRIVE_LAYOUT_INFORMATION_EX* IoctlDiskGetDriveLayoutEx(const std::string &diskname)
{
    std::stringstream ss;

    HANDLE  h = CreateFile(diskname.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        ss << "Failed to open disk " << diskname << " with error " << GetLastError();
        DebugPrintf(SV_LOG_ERROR, "%s\n", ss.str().c_str());
        return 0;
    }
    DRIVE_LAYOUT_INFORMATION_EX *dli = IoctlDiskGetDriveLayoutEx(h);
    if (!dli)
        DebugPrintf(SV_LOG_ERROR, "Failed to get drive layout information for disk %s.\n", diskname.c_str());

    CloseHandle(h);
    return dli;
}


DWORD GetDiskSectorSize(const DWORD dwIndex, DWORD& dwBytesPerSector, std::string& errorString)
{
    std::stringstream       ssDiskName, ssError;
    ssDiskName << "\\\\.\\PhysicalDrive" << dwIndex;
    DWORD                 dwStatus = ERROR_SUCCESS;

    HANDLE  hDisk = CreateFile(ssDiskName.str().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hDisk)
    {
        ssError << "Failed to open disk " << ssDiskName.str() << " with error " << GetLastError();
        errorString = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", errorString.c_str());
        return GetLastError();
    }

    DWORD cbReturned = 0;
    DISK_GEOMETRY disk_geometry = { 0 };

    if (!DeviceIoControl(hDisk,
        IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL,
        0,
        &disk_geometry,
        sizeof(DISK_GEOMETRY),
        &cbReturned,
        NULL))
    {
        ssError << "Failed to get geometry for " << ssDiskName.str() << " with error " << GetLastError();
        errorString = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", errorString.c_str());
        dwStatus = GetLastError();
    }
    else {
        dwBytesPerSector = disk_geometry.BytesPerSector;
    }
    CloseHandle(hDisk);
    return dwStatus;
}

DRIVE_LAYOUT_INFORMATION_EX* IoctlDiskGetDriveLayoutEx(const DWORD dwIndex)
{
    std::stringstream       ssDiskName;
    ssDiskName << "\\\\.\\PhysicalDrive" << dwIndex;
    return IoctlDiskGetDriveLayoutEx(ssDiskName.str());
}

bool IsDiskDynamic(PDRIVE_LAYOUT_INFORMATION_EX pLayoutEx)
{
    /* TODO: unify this reference across all modules */
    const GUID PARTITION_LDM_METADATA_GUID = {
        0x5808C8AA,
        0x7E8F,
        0x42E0,
        0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3
    }; // Logical Disk Manager (LDM) metadata partition on a dynamic disk

    if (NULL == pLayoutEx) {
        return false;
    }

    bool isDiskDynamic = false;

    const std::string DYNAMIC_DISK_VGNAME = "__INMAGE__DYNAMIC__DG__";

    if (NULL == pLayoutEx) {
        return false;
    }

    for (DWORD i = 0; i < pLayoutEx->PartitionCount; i++)
    {
        PARTITION_INFORMATION_EX &partInfoEx = pLayoutEx->PartitionEntry[i];

        if (PARTITION_STYLE_MBR == partInfoEx.PartitionStyle)
        {
            if (PARTITION_LDM == partInfoEx.Mbr.PartitionType)
            {
                isDiskDynamic = true;
                break;
            }
        }
        else if (PARTITION_STYLE_GPT == partInfoEx.PartitionStyle)
        {
            if (IsEqualGUID(partInfoEx.Gpt.PartitionType, PARTITION_LDM_METADATA_GUID))
            {
                isDiskDynamic = true;
                break;
            }
        }
    }

    return isDiskDynamic;
}

bool HasUefi(SV_LONG diskIndex)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    PDRIVE_LAYOUT_INFORMATION_EX    pLayoutEx = IoctlDiskGetDriveLayoutEx(diskIndex);

    if (NULL == pLayoutEx) {
        DebugPrintf(SV_LOG_ERROR, "Failed to get drive layout for disk %d\n", diskIndex);
        DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
        return false;
    }

    ON_BLOCK_EXIT(boost::bind<void>(&free, pLayoutEx));

    if (PARTITION_STYLE_GPT != pLayoutEx->PartitionStyle) {
        DebugPrintf(SV_LOG_DEBUG, "Disk %d does not contain UEFI partition Layout is %s\n", diskIndex,
            (PARTITION_STYLE_MBR == pLayoutEx->PartitionStyle) ? "MBR" : "RAW");

        DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
        return false;
    }

    for (DWORD i = 0; i < pLayoutEx->PartitionCount; i++)
    {
        PARTITION_INFORMATION_EX &partInfoEx = pLayoutEx->PartitionEntry[i];
        if (IsEqualGUID(partInfoEx.Gpt.PartitionType, PARTITION_SYSTEM_GUID)) {
            DebugPrintf(SV_LOG_DEBUG, "Disk %d Partition %d is UEFI\n", diskIndex, i);

            DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
            return true;
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "Disk %d doesn't contain UEFI Partition\n", diskIndex);
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return false;
}

DWORD IsDiskDynamic(SV_LONG diskIndex, bool& bDiskDynamic)
{
    bool isDiskDynamic = false;

    bDiskDynamic = false;

    std::stringstream       diskName;
    diskName << "\\\\.\\PhysicalDrive" << diskIndex;

    PDRIVE_LAYOUT_INFORMATION_EX    pLayoutEx = IoctlDiskGetDriveLayoutEx(diskName.str());

    if (NULL == pLayoutEx) {
        DebugPrintf(SV_LOG_DEBUG, "Failed to get Layout for Disk %d error=%d\n", diskIndex, GetLastError());
        return GetLastError();
    }

    isDiskDynamic = IsDiskDynamic(pLayoutEx);

    free(pLayoutEx);

    bDiskDynamic = isDiskDynamic;

    return ERROR_SUCCESS;
}


bool GetRootDiskLayoutInfo(PDRIVE_LAYOUT_INFORMATION_EX &pLayoutEx, DWORD& dwBytesPerSector, std::string &err, DWORD &errcode)
{
    std::vector<char>       BootVolume(MAX_PATH + 1);
    std::vector<char>       SystemDrive(MAX_PATH + 1);
    std::vector<char>       VolumeName(MAX_PATH + 1);
    std::stringstream       ssError;
    HANDLE                  hVolume = INVALID_HANDLE_VALUE;
    PVOLUME_DISK_EXTENTS    pDiskExtents = NULL;
    std::vector<UCHAR>      buffer(sizeof(DRIVE_LAYOUT_INFORMATION_EX) + 127 * sizeof(PARTITION_INFORMATION_EX));

    pDiskExtents = (PVOLUME_DISK_EXTENTS)&buffer[0];

    if (0 == GetSystemDirectory(&BootVolume[0], MAX_PATH)) {
        errcode = GetLastError();
        ssError << "GetSystemDirectory  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, ssError.str().c_str());
        return false;
    }

    StringCchPrintf(&SystemDrive[0], MAX_PATH, "%s", DOS_NAME_PREFIX);

    if (0 == GetVolumePathName(&BootVolume[0], &SystemDrive[strlen(&SystemDrive[0])], MAX_PATH - strlen(&SystemDrive[0]))){
        errcode = GetLastError();
        ssError << "GetVolumePathName for directory " << &BootVolume[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, ssError.str().c_str());
        return false;
    }

    UINT uiDriveType = GetDriveType(&SystemDrive[0]);
    // Fail check when system is noot booted from system drive
    if (DRIVE_FIXED != uiDriveType) {
        ssError << "SystemDrive " << &SystemDrive[0] << " is not fixed drive type: " << uiDriveType;
        err = ERROR_INVALID_DRIVE_OBJECT;
        DebugPrintf(SV_LOG_ERROR, ssError.str().c_str());
        return false;
    }

    SystemDrive[strlen(&SystemDrive[0]) - 1] = '\0';

    hVolume = CreateFile(&SystemDrive[0], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hVolume) {
        errcode = GetLastError();
        ssError << "CreateFile for volume " << &SystemDrive[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "CreateFile for volume %s failed with err=%d\n", &SystemDrive[0], errcode);
        return false;
    }

    DWORD bytes;
    if (!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents, buffer.size(), &bytes, NULL)) {
        errcode = GetLastError();
        ssError << "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS for volume " << &SystemDrive[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS for volume %s failed with error=%d\n", &SystemDrive[0], errcode);
        CloseHandle(hVolume);
        return false;
    }

    CloseHandle(hVolume);

    ULONG bootDiskIndex = pDiskExtents->Extents[0].DiskNumber;

    if (ERROR_SUCCESS != GetDiskSectorSize(bootDiskIndex, dwBytesPerSector, err)) {
        return false;
    }

    pLayoutEx = IoctlDiskGetDriveLayoutEx(bootDiskIndex);

    if (NULL == pLayoutEx) {
        errcode = GetLastError();
        ssError << "IoctlDiskGetDriveLayoutEx for volume " << &SystemDrive[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "IoctlDiskGetDriveLayoutEx for volume %s failed with error=%d\n", &SystemDrive[0], errcode);
        return false;
    }

    return true;
}

bool GetDiskGuid(ULONG ulDiskIndex, std::string& deviceId)
{
    std::stringstream       ssDeviceName;
    ssDeviceName << DISK_NAME_PREFIX << ulDiskIndex;

    std::stringstream   ssErrorMsg;

    HANDLE hDisk = CreateFileA(ssDeviceName.str().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (INVALID_HANDLE_VALUE == hDisk) {
        ssErrorMsg << "Failed to open device: " << ssDeviceName.str() << "Error Code " << GetLastError();
        DebugPrintf(SV_LOG_ERROR, "%s\n", ssErrorMsg.str().c_str());
        return false;
    }

    ON_BLOCK_EXIT(boost::bind<void>(&CloseHandle, hDisk));

    deviceId = GetDiskGuid(hDisk);

    return true;
}


std::string GetDiskGuid(const HANDLE &h)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    if (INVALID_HANDLE_VALUE == h) {
        DebugPrintf(SV_LOG_ERROR, "Handle provided to find disk GUID is invalid.\n");
        return std::string();
    }

    DRIVE_LAYOUT_INFORMATION_EX *dli = IoctlDiskGetDriveLayoutEx(h);
    if (!dli) {
        DebugPrintf(SV_LOG_ERROR, "Failed to get drive layout information.\n");
        return std::string();
    }

    std::string diskGuid;
    if (GetDiskGuidFromDli(dli, diskGuid))
        DebugPrintf(SV_LOG_DEBUG, "DiskGuid found is %s.\n", diskGuid.c_str());
    else
        DebugPrintf(SV_LOG_ERROR, "No DiskGuid found.\n");

    free(dli);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return diskGuid;
}


bool GetDiskGuidFromDli(DRIVE_LAYOUT_INFORMATION_EX *pDli, std::string &diskGuid)
{
    bool bResult = false;
    std::vector<wchar_t>   guidBuffer(MAX_PATH+1, 0);

    if ((pDli->PartitionStyle == PARTITION_STYLE_MBR) &&
        (pDli->Mbr.Signature)) /* the ioctl is wrongly writing PARTITION_STYLE_MBR (value 0) for raw disks. Hence check if signature is non zero */
    {
        // Adding braces {} around signature to make disk GUID uniform as these braces come for gpt GUID
        diskGuid = "{";
        diskGuid += std::to_string(pDli->Mbr.Signature);
        diskGuid += "}";
        bResult = true;
    }
    else if (pDli->PartitionStyle == PARTITION_STYLE_GPT)
    {
        StringFromGUID2(pDli->Gpt.DiskId, &guidBuffer[0], MAX_PATH);
        diskGuid = CW2A(&guidBuffer[0]);

        bResult = true;
    }
    return bResult;
}


bool IsUEFIBoot(void)
{
    bool isUefi = false;

    PDRIVE_LAYOUT_INFORMATION_EX    pLayoutEx = NULL;
    DWORD                           dwBytesPerSector = 0;
    std::string err;
    DWORD errcode;
    if (!GetRootDiskLayoutInfo(pLayoutEx, dwBytesPerSector, err, errcode))
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to get DiskLayout err=%d\n", errcode);
        return false;
    }
    ON_BLOCK_EXIT(boost::bind<void>(&free, pLayoutEx));

    if (PARTITION_STYLE_GPT == pLayoutEx->PartitionStyle) {
        isUefi = true;
    }

    DebugPrintf(SV_LOG_INFO, "Bios Mode is %s\n", isUefi? "UEFI" : "BIOS");
    return isUefi;
}

BOOL GetDeviceAttributes(ULONG ulDiskIndex,
    std::string& vendorId,
    std::string& productId,
    std::string& serialNumber,
    std::string& errormessage)
{
    std::stringstream   ssDeviceName;
    ssDeviceName << "\\\\.\\PhysicalDrive" << ulDiskIndex;

    HANDLE hDisk = CreateFileA(ssDeviceName.str().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hDisk)
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open device %s error=%d\n", ssDeviceName.str().c_str(), GetLastError());
        return FALSE;
    }

    BOOL bSuccess = GetDeviceAttributes(hDisk, ssDeviceName.str(), vendorId, productId, serialNumber, errormessage);
    CloseHandle(hDisk);
    return bSuccess;
}

BOOL GetDeviceAttributes(const HANDLE& hDevice,
    const std::string & diskname,
    std::string& vendorId,
    std::string& productId,
    std::string& serialNumber,
    std::string& errormessage)
{
    std::vector<SV_UCHAR>           Buffer;
    STORAGE_PROPERTY_QUERY          queryProperty;
    PSTORAGE_DEVICE_DESCRIPTOR      pDeviceDescriptor;
    BOOL                            bRet = FALSE;
    std::stringstream               sserror;

    vendorId.clear();
    productId.clear();
    serialNumber.clear();

    ZeroMemory(&queryProperty, sizeof(queryProperty));
    queryProperty.PropertyId = StorageDeviceProperty;
    queryProperty.QueryType = PropertyStandardQuery;

    // Get size of device descriptor
    STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader = { 0 };
    DWORD dwBytesReturned = 0;

    bRet = DeviceIoControl(hDevice,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &queryProperty, sizeof(queryProperty),
        &storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER),
        &dwBytesReturned, NULL);

    if (!bRet) {
        sserror << FUNCTION_NAME << ": 1: IOCTL_STORAGE_QUERY_PROPERTY failed for disk " << diskname << " error: " << GetLastError();
        errormessage = sserror.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", errormessage.c_str());
        return FALSE;
    }

    // Get Storage Device Descriptor
    const DWORD dwSize = storageDescriptorHeader.Size;
    Buffer.resize(storageDescriptorHeader.Size, 0);

    // Get the storage device descriptor
    bRet = DeviceIoControl(hDevice,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &queryProperty, sizeof(STORAGE_PROPERTY_QUERY),
        &Buffer[0], dwSize,
        &dwBytesReturned, NULL);
    if (!bRet) {
        sserror << FUNCTION_NAME << ": 2: IOCTL_STORAGE_QUERY_PROPERTY failed for disk " << diskname << " error: " << GetLastError();
        errormessage = sserror.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", errormessage.c_str());
        return FALSE;
    }

    pDeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)&Buffer[0];
    if (pDeviceDescriptor->VendorIdOffset < dwSize) {
        vendorId = (char *)(&Buffer[0] + pDeviceDescriptor->VendorIdOffset);
        // Remove trailing whitespace
        boost::algorithm::trim(vendorId);
        DebugPrintf(SV_LOG_DEBUG, "disk %s VendorId: %s\n", diskname.c_str(), vendorId.c_str());
    }

    if (pDeviceDescriptor->ProductIdOffset < dwSize) {
        productId = (char *)(&Buffer[0] + pDeviceDescriptor->ProductIdOffset);
        boost::algorithm::trim(productId);
        DebugPrintf(SV_LOG_DEBUG, "disk %s productId: %s\n", diskname.c_str(), productId.c_str());
    }

    if (pDeviceDescriptor->SerialNumberOffset < dwSize) {
        serialNumber = (char *)(&Buffer[0] + pDeviceDescriptor->SerialNumberOffset);
        boost::algorithm::trim(serialNumber);
        DebugPrintf(SV_LOG_DEBUG, "disk %s serialNumber: %s\n", diskname.c_str(), serialNumber.c_str());
    }

    return TRUE;
}

bool GetSystemDiskList(std::set<SV_ULONG>& diskIndices, std::string &err, DWORD &errcode)
{
    std::vector<char>       BootVolume(MAX_PATH + 1);
    std::vector<char>       SystemDrive(MAX_PATH + 1);
    std::vector<char>       VolumeName(MAX_PATH + 1);
    std::stringstream       ssError;
    HANDLE                  hVolume = INVALID_HANDLE_VALUE;
    PVOLUME_DISK_EXTENTS    pDiskExtents = NULL;
    std::vector<UCHAR>      buffer(sizeof(DRIVE_LAYOUT_INFORMATION_EX) + 127 * sizeof(PARTITION_INFORMATION_EX));

    pDiskExtents = (PVOLUME_DISK_EXTENTS)&buffer[0];

    if (0 == GetSystemDirectory(&BootVolume[0], MAX_PATH)) {
        errcode = GetLastError();
        ssError << "GetSystemDirectory  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, ssError.str().c_str());
        return false;
    }

    StringCchPrintf(&SystemDrive[0], MAX_PATH, "%s", DOS_NAME_PREFIX);

    if (0 == GetVolumePathName(&BootVolume[0], &SystemDrive[strlen(&SystemDrive[0])], MAX_PATH - strlen(&SystemDrive[0]))){
        errcode = GetLastError();
        ssError << "GetVolumePathName for directory " << &BootVolume[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, ssError.str().c_str());
        return false;
    }

    UINT uiDriveType = GetDriveType(&SystemDrive[0]);
    // Fail check when system is noot booted from system drive
    if (DRIVE_FIXED != uiDriveType) {
        ssError << "SystemDrive " << &SystemDrive[0] << " is not fixed drive type: " << uiDriveType;
        err = ERROR_INVALID_DRIVE_OBJECT;
        DebugPrintf(SV_LOG_ERROR, ssError.str().c_str());
        return false;
    }

    SystemDrive[strlen(&SystemDrive[0]) - 1] = '\0';

    hVolume = CreateFile(&SystemDrive[0], GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hVolume) {
        errcode = GetLastError();
        ssError << "CreateFile for volume " << &SystemDrive[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "CreateFile for volume %s failed with err=%d\n", &SystemDrive[0], errcode);
        return false;
    }

    ON_BLOCK_EXIT(boost::bind<void>(&CloseHandle, hVolume));

    DWORD bytes;
    if (!DeviceIoControl(hVolume, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, pDiskExtents, buffer.size(), &bytes, NULL)) {
        errcode = GetLastError();
        ssError << "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS for volume " << &SystemDrive[0] << "  failed with err=" << errcode << std::endl;
        err = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS for volume %s failed with error=%d\n", &SystemDrive[0], errcode);
        return false;
    }

    for (int index = 0; index < pDiskExtents->NumberOfDiskExtents; index++) {
        diskIndices.insert(pDiskExtents->Extents[index].DiskNumber);
    }

    return true;
}

BOOL GetBusType(SV_ULONG ulDiskIndex, STORAGE_BUS_TYPE& busType, std::string& errorMessage)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    std::stringstream       ssDiskName;
    ssDiskName << DISK_NAME_PREFIX << ulDiskIndex;

    return GetBusType(ssDiskName.str(), busType, errorMessage);
}

BOOL GetBusType(std::string deviceName, STORAGE_BUS_TYPE& busType, std::string& errorMessage)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    HANDLE                      hDisk = INVALID_HANDLE_VALUE;
    std::stringstream           ssErrorMsg;

    busType = BusTypeUnknown;

    hDisk = CreateFileA(deviceName.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (INVALID_HANDLE_VALUE == hDisk) {
        ssErrorMsg << __FUNCTION__ << "Failed to open device: " << deviceName << "Error Code " << GetLastError();
        errorMessage = ssErrorMsg.str();

        DebugPrintf(SV_LOG_ERROR, "%s\n", ssErrorMsg.str().c_str());
        return false;
    }

    ON_BLOCK_EXIT(boost::bind<void>(&CloseHandle, hDisk));

    BOOL bStatus = GetBusType(hDisk, busType, errorMessage);

    if (bStatus) {
        DebugPrintf(SV_LOG_INFO, "Device: %s BusType: %d\n", deviceName.c_str(), busType);
    }
    else {
        DebugPrintf(SV_LOG_DEBUG, "%s\n", errorMessage.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return bStatus;
}

BOOL GetBusType(HANDLE hDisk, STORAGE_BUS_TYPE& busType, std::string& errorMessage)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::stringstream           ssErrorMsg;
    DWORD                       dwBytes;
    std::vector<BYTE>           Buffer(sizeof(PSTORAGE_DEVICE_DESCRIPTOR));
    PSTORAGE_DEVICE_DESCRIPTOR  pStorageDescriptor = NULL;
    STORAGE_PROPERTY_QUERY      storageProperty;
    DWORD                       retryCount = 0;
    DWORD                       dwInput;
    BOOL                        bResult = FALSE;
    STORAGE_DESCRIPTOR_HEADER   storageHeader = { 0 };
    PVOID                       pBuffer;

    ZeroMemory(&storageProperty, sizeof(storageProperty));

    storageProperty.PropertyId = StorageDeviceProperty;
    storageProperty.QueryType = PropertyStandardQuery;

    busType = BusTypeUnknown;

    pBuffer = (PVOID)&storageHeader;
    dwInput = sizeof(storageHeader);

    do {

        bResult = DeviceIoControl(hDisk,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &storageProperty,
            sizeof(storageProperty),
            pBuffer,
            dwInput,
            &dwBytes,
            NULL
            );

        if (!bResult) {
            ssErrorMsg << "IOCTL_STORAGE_QUERY_PROPERTY failed with error = " << GetLastError();
            errorMessage = ssErrorMsg.str();
            DebugPrintf(SV_LOG_DEBUG, "%s\n", errorMessage.c_str());
            break;
        }

        if (retryCount == 1) {
            busType = pStorageDescriptor->BusType;
            DebugPrintf(SV_LOG_DEBUG, "BusType %d\n", busType);
            break;
        }

        dwInput = storageHeader.Size;
        Buffer.resize(dwInput);

        pStorageDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)&Buffer[0];
        pBuffer = pStorageDescriptor;
        retryCount++;
    } while (retryCount < 2);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return bResult;
}


std::set<ULONG>
GetAvailableDiskIndices(
ULONG   ulMaxDiskIndex,
ULONG   ulMaxConsMissingIndices)
{
    std::stringstream   ssDeviceName;
    ULONG               ulDiskIndex = 0;
    ULONG               ulConsMissingIndices = 0;
    std::set<ULONG>     diskIndices;
    HANDLE              hDisk = INVALID_HANDLE_VALUE;

    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    while (ulDiskIndex < ulMaxDiskIndex ||
        (ulConsMissingIndices < ulMaxConsMissingIndices)) {

        ULONG ulDeviceNum = ulDiskIndex;
        ++ulDiskIndex;

        ssDeviceName.str("");
        ssDeviceName << DISK_NAME_PREFIX << ulDeviceNum;

        hDisk = CreateFileA(ssDeviceName.str().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (INVALID_HANDLE_VALUE != hDisk) {
            DebugPrintf(SV_LOG_INFO, "Adding Disk %s\n", ssDeviceName.str().c_str());
            CloseHandle(hDisk);
            diskIndices.insert(ulDeviceNum);
            ulConsMissingIndices = 0;
            continue;
        }

        ulConsMissingIndices++;
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return diskIndices;
}

DiskInterface::DiskInterface(SV_ULONG ulIndex) :
m_ulIndex(ulIndex),
m_hDisk(INVALID_HANDLE_VALUE)
{
    if (!Open()) {
        throw std::exception(GetErrorMessage().c_str());
    }
}

DiskInterface::~DiskInterface()
{
    if (INVALID_HANDLE_VALUE != m_hDisk) {
        CloseHandle(m_hDisk);
    }
    m_hDisk = INVALID_HANDLE_VALUE;

}

bool DiskInterface::Open()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    std::stringstream       ssDiskName;
    std::stringstream       ssError;
    ssDiskName << DISK_NAME_PREFIX << m_ulIndex;

    m_hDisk = CreateFileA(ssDiskName.str().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (INVALID_HANDLE_VALUE == m_hDisk) {
        ssError << "Failed to open disk " << m_ulIndex << " Error: " << GetLastError();
        m_errorMessage = ssError.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", m_errorMessage.c_str());
        return false;
    }
    return true;
}

bool DiskInterface::GetBusType(STORAGE_BUS_TYPE& busType) 
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::stringstream           ssErrorMsg;
    DWORD                       dwBytes;
    std::vector<BYTE>           Buffer(sizeof(PSTORAGE_DEVICE_DESCRIPTOR));
    PSTORAGE_DEVICE_DESCRIPTOR  pStorageDescriptor = NULL;
    STORAGE_PROPERTY_QUERY      storageProperty;
    DWORD                       retryCount = 0;
    DWORD                       dwInput;
    BOOL                        bResult = FALSE;
    STORAGE_DESCRIPTOR_HEADER   storageHeader = { 0 };
    PVOID                       pBuffer;

    ZeroMemory(&storageProperty, sizeof(storageProperty));

    storageProperty.PropertyId = StorageDeviceProperty;
    storageProperty.QueryType = PropertyStandardQuery;

    busType = BusTypeUnknown;

    pBuffer = (PVOID)&storageHeader;
    dwInput = sizeof(storageHeader);

    do {

        bResult = DeviceIoControl(m_hDisk,
            IOCTL_STORAGE_QUERY_PROPERTY,
            &storageProperty,
            sizeof(storageProperty),
            pBuffer,
            dwInput,
            &dwBytes,
            NULL
            );

        if (!bResult) {
            ssErrorMsg << "IOCTL_STORAGE_QUERY_PROPERTY failed with error = " << GetLastError();
            m_errorMessage = ssErrorMsg.str();
            DebugPrintf(SV_LOG_DEBUG, "%s\n", m_errorMessage.c_str());
            break;
        }

        if (retryCount == 1) {
            busType = pStorageDescriptor->BusType;
            DebugPrintf(SV_LOG_DEBUG, "BusType %d\n", busType);
            break;
        }

        dwInput = storageHeader.Size;
        Buffer.resize(dwInput);

        pStorageDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)&Buffer[0];
        pBuffer = pStorageDescriptor;
        retryCount++;
    } while (retryCount < 2);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return (bResult == TRUE);

}

bool DiskInterface::OfflineDisk()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool rv = OnlineOfflineDisk(false, false, false);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return rv;
}

bool DiskInterface::OnlineDisk()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool rv = OnlineOfflineDisk(true, false, false);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return rv;
}

bool DiskInterface::OnlineOfflineDisk(bool online, bool readOnly, bool persist)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    BOOL rv = FALSE;
    std::stringstream           ssErrorMsg;

    SET_DISK_ATTRIBUTES attributes = { 0 };
    attributes.Version = sizeof(SET_DISK_ATTRIBUTES);
    attributes.Persist = persist;
    attributes.Attributes = (online) ? 0L : DISK_ATTRIBUTE_OFFLINE;
    attributes.Attributes |= (readOnly) ? DISK_ATTRIBUTE_READ_ONLY : 0L;
    attributes.AttributesMask = DISK_ATTRIBUTE_OFFLINE | DISK_ATTRIBUTE_READ_ONLY;

    DWORD dwBytesReturned;

    rv = DeviceIoControl(m_hDisk,
        IOCTL_DISK_SET_DISK_ATTRIBUTES,
        &attributes,
        sizeof(SET_DISK_ATTRIBUTES),
        NULL,
        0,
        &dwBytesReturned,
        NULL);

    if (!rv) 
    {
        ssErrorMsg << "Error: IOCTL_DISK_SET_DISK_ATTRIBUTES failed for disk " << m_ulIndex << " Error: " << GetLastError();
        m_errorMessage = ssErrorMsg.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", m_errorMessage.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return (TRUE == rv);
}

bool    DiskInterface::IsResourceDisk()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    STORAGE_BUS_TYPE    busType;
    bool                isResourceDisk;
    if (!GetBusType(busType)) {
        DebugPrintf(SV_LOG_ERROR, "Failed to get BusType Error: %d", GetLastError());
        return false;
    }

    isResourceDisk = ((STORAGE_BUS_TYPE::BusTypeAta == busType) || (STORAGE_BUS_TYPE::BusTypeAtapi == busType));
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return isResourceDisk;
}
