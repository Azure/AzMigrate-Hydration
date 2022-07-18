#pragma once 

#include "svtypes.h"
#include <set>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <atlbase.h>
#include "..\scopeguard\scopeguard.h"

#define DOS_NAME_PREFIX     "\\\\?\\"
#define DISK_NAME_PREFIX        "\\\\.\\PhysicalDrive"

extern DRIVE_LAYOUT_INFORMATION_EX* IoctlDiskGetDriveLayoutEx(const HANDLE &h);
extern bool GetSystemDiskList(std::set<SV_ULONG>& diskIndices, std::string &err, DWORD &errcode);
extern bool HasUefi(SV_LONG diskIndex);
extern DRIVE_LAYOUT_INFORMATION_EX* IoctlDiskGetDriveLayoutEx(const std::string &diskname);
extern DWORD IsDiskDynamic(SV_LONG diskIndex, bool& bDiskDynamic);
extern bool IsDiskDynamic(PDRIVE_LAYOUT_INFORMATION_EX pLayoutEx);

extern BOOL GetBusType(SV_ULONG ulDiskIndex, STORAGE_BUS_TYPE& busType, std::string& errorMessage);
extern BOOL GetBusType(std::string deviceName, STORAGE_BUS_TYPE& busType, std::string& errorMessage);
extern BOOL GetBusType(HANDLE hDisk, STORAGE_BUS_TYPE& busType, std::string& errorMessage);

extern bool OfflineDisk(HANDLE hDisk);
extern bool OnlineDisk(HANDLE hDisk);
extern bool OnlineOfflineDisk(HANDLE hDisk, bool online, bool readOnly, bool persist);
extern std::set<ULONG> GetAvailableDiskIndices(
                                ULONG   ulMaxDiskIndex,
                                ULONG   ulMaxConsMissingIndices);

DWORD GetDiskSectorSize(const DWORD dwIndex, DWORD& dwBytesPerSector, std::string& errorString);
bool IsDiskDynamic(PDRIVE_LAYOUT_INFORMATION_EX pLayoutEx);
bool SystemDiskUEFICheck(std::string &errormsg);
bool GetRootDiskLayoutInfo(PDRIVE_LAYOUT_INFORMATION_EX &pLayoutEx, DWORD& dwBytesPerSector, std::string &err, DWORD &errcode);
bool IsSystemDriveFixedDrive();

BOOL GetBusType(SV_ULONG ulDiskIndex, STORAGE_BUS_TYPE& busType, std::string& errorMessage);
BOOL GetBusType(std::string deviceName, STORAGE_BUS_TYPE& busType, std::string& errorMessage);
BOOL GetBusType(HANDLE hDisk, STORAGE_BUS_TYPE& busType, std::string& errorMessage);


BOOL GetDeviceAttributes(ULONG ulDiskIndex,
    std::string& vendorId,
    std::string& productId,
    std::string& serialNumber,
    std::string& errormessage);

BOOL GetDeviceAttributes(const HANDLE& hDevice,
    const std::string & diskname,
    std::string& vendorId,
    std::string& productId,
    std::string& serialNumber,
    std::string& errormessage);

extern bool GetDiskGuidFromDli(DRIVE_LAYOUT_INFORMATION_EX *pDli, std::string &diskGuid);
extern std::string GetDiskGuid(const HANDLE &h);
extern bool GetDiskGuid(ULONG ulDiskIndex, std::string& deviceId);

class DiskInterface {
private:
    HANDLE          m_hDisk;
    SV_ULONG        m_ulIndex;
    std::string     m_errorMessage;
public:
    DiskInterface(SV_ULONG ulDiskIndex);
    ~DiskInterface();

    bool    Open();
    bool    GetBusType(STORAGE_BUS_TYPE& busType);
    bool    OfflineDisk();
    bool    OnlineDisk();
    bool    OnlineOfflineDisk(bool online, bool readOnly, bool persist);
    std::string GetErrorMessage() { return m_errorMessage; }
    bool    IsResourceDisk();
};
