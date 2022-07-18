#include "portablehelpers.h"

#include <winioctl.h>
#include "InmFltIoctl.h"
#include "InmFltInterface.h"

#include "InmageDriverInterface.h"

InmageDriverInterface::InmageDriverInterface() :
m_hInmageDevice(INVALID_HANDLE_VALUE)
{
    m_hInmageDevice = CreateFile(INMAGE_DISK_FILTER_DOS_DEVICE_NAME, 
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
    if (INVALID_HANDLE_VALUE == m_hInmageDevice) {
        DebugPrintf(SV_LOG_ERROR, "Failed to open device %s error=%d\n", INMAGE_DISK_FILTER_DOS_DEVICE_NAME, GetLastError());
    }
}

InmageDriverInterface::~InmageDriverInterface()
{
    if (INVALID_HANDLE_VALUE != m_hInmageDevice) {
        CloseHandle(m_hInmageDevice);
    }

    m_hInmageDevice = INVALID_HANDLE_VALUE;
}

bool InmageDriverInterface::IsDiskRecoveryRequired()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    DRIVER_FLAGS_OUTPUT     driverFlags = { 0 };

    DWORD dwBytes;
    if (!DeviceIoControl(m_hInmageDevice, IOCTL_INMAGE_GET_DRIVER_FLAGS, NULL, 0, &driverFlags, sizeof(driverFlags), &dwBytes, NULL)){
        DebugPrintf(SV_LOG_ERROR, "IOCTL_INMAGE_GET_DRIVER_FLAGS failed with error=%d\n", GetLastError());
        return false;
    }

    DebugPrintf(SV_LOG_DEBUG, "Driver Flags: 0x%08x\n", driverFlags.ulFlags);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return TEST_FLAG(driverFlags.ulFlags, DRIVER_FLAG_DISK_RECOVERY_NEEDED);
}

void InmageDriverInterface::SetSanPolicyToOnlineForAllDisks()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    SET_SAN_POLICY  sanPolicy = { 0 };
    sanPolicy.ulFlags |= SET_SAN_ALL_DEVICES_FLAGS;
    sanPolicy.ulPolicy = 0;

    DWORD dwBytes;
    if (!DeviceIoControl(m_hInmageDevice, IOCTL_INMAGE_SET_SAN_POLICY, &sanPolicy, sizeof(sanPolicy), NULL, 0, &dwBytes, NULL)) {
        DebugPrintf(SV_LOG_ERROR, "IOCTL_INMAGE_SET_SAN_POLICY failed for device %s error=%d\n", INMAGE_DISK_FILTER_DOS_DEVICE_NAME, GetLastError());
        return;
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

bool    InmageDriverInterface::GetDiskIndexList(std::set<SV_ULONG>&  diskIndexList)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    PDISKINDEX_INFO     pDiskIndexList = NULL;
    DWORD               size, dwBytes;
    std::vector<UCHAR>  Buffer(256 * sizeof(SV_ULONG));

    pDiskIndexList = (PDISKINDEX_INFO)&Buffer[0];
    memset(pDiskIndexList, 0, Buffer.size());

    BOOL bSuccess = DeviceIoControl(m_hInmageDevice, IOCTL_INMAGE_GET_DISK_INDEX_LIST, NULL, 0, pDiskIndexList, Buffer.size(), &dwBytes, NULL);

    if (bSuccess){
        for (ULONG ulIndex = 0; ulIndex < pDiskIndexList->ulNumberOfDisks; ulIndex++) {
            DebugPrintf(SV_LOG_DEBUG, "%s: Adding disk %d\n", FUNCTION_NAME, pDiskIndexList->aDiskIndex[ulIndex]);
            diskIndexList.insert(pDiskIndexList->aDiskIndex[ulIndex]);
        }
    }
    else {
        DebugPrintf(SV_LOG_ERROR, "IOCTL_INMAGE_GET_DISK_INDEX_LIST failed for device %s err=%d\n",
            INMAGE_DISK_FILTER_DOS_DEVICE_NAME,
            GetLastError());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return (bSuccess == TRUE);
}
