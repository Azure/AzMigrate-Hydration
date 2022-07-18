#ifndef PORTABLEHELPERS__MAJOREX__H_
#define PORTABLEHELPERS__MAJOREX__H_

#define _WIN32_WINNT 0x0600
#define FUNCTION_NAME __FUNCTION__

#include <windows.h>
#include <devioctl.h>
#include <ntdddisk.h>
#include <sstream>

#include "portablehelpersmajor.h"
#include "logger.h"

#include "scopeguard.h"

bool IsDiskClustered(const std::string& diskName, bool& bClustered, std::string& errMsg)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    errMsg.clear();

    bool bRet = false;
    bClustered = false;
    std::stringstream   ssError;

    DWORD  bytesReturned = 0;
    DWORD dwSize = sizeof(bClustered);

    HANDLE hDisk;
    
    do {

        hDisk = SVCreateFile(diskName.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (INVALID_HANDLE_VALUE == hDisk)
        {
            ssError << "Failed to open disk " << diskName << " error: " << GetLastError();
            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, ssError.str().c_str());

            errMsg = ssError.str();

            break;
        }

        ON_BLOCK_EXIT(&CloseHandle, hDisk);

        if (!DeviceIoControl(hDisk, IOCTL_DISK_IS_CLUSTERED, NULL, 0, &bClustered, dwSize, &bytesReturned, NULL))
        {
            ssError << "IOCTL_DISK_IS_CLUSTERED failed for the disk : " << diskName << " with error : " << GetLastError();
            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, ssError.str().c_str());

            errMsg = ssError.str();

            break;
        }
        else {

            bRet = true;
        }

    } while (0);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return bRet;
}
#endif