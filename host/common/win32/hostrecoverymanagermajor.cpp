//---------------------------------------------------------------
//  <copyright file="hostrecoverymanager.cpp" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
//
//  Description: Windows implementation for HostRecoveryManager
//
//  History:	15-Aug-2016		veshivan	Created
//
//----------------------------------------------------------------

#include "hostrecoverymanager.h"

#include "registry.h"
#include "localconfigurator.h"
#include "service.h"

#include <winioctl.h>
#include <versionhelpers.h>

#include "InmFltIoctl.h"
#include "InmFltInterface.h"

#include "DiskHelpers.h"
#include <map>
#include <set>
#include <sstream>

#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <regex>

#include "InmageDriverInterface.h"
#include "RegistryOperations.h"

using namespace std;

#define VMWARE_TOOLS_SERVICE_NAME   "VMTools"


void HostRecoveryManager::DisableEnablePlatformServices(std::map<std::string, InmServiceStartType> platformServices)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    InmServiceStatus        svcStatus;
    InmServiceStartType     startType;
    InmServiceStartType     curStartType;
    std::string             strStartType;

    for (auto itService : platformServices) {
        getServiceStatus(itService.first, svcStatus);

        if (INM_SVCSTAT_NOTINSTALLED == svcStatus) {
            DebugPrintf(SV_LOG_INFO,
                "%s Service not installed on this system.\n", itService.first.c_str());
            continue;
        }

        startType = itService.second;

        if (!getStartServiceType(itService.first, curStartType, strStartType)) {
            std::stringstream exceptionMsg;
            exceptionMsg << "Could not get start type for service: "
                << itService.first;
            DebugPrintf(SV_LOG_ERROR, exceptionMsg.str().c_str());
            continue;
        }

        DebugPrintf(SV_LOG_DEBUG, "Service %s Start Type is %s\n", itService.first.c_str(), strStartType.c_str());

        // Change service start type if needed
        if (startType != curStartType) {
            if (changeServiceType(itService.first, startType) != SVS_OK) {
                std::stringstream exceptionMsg;
                exceptionMsg << "Could not enable the service: "
                    << itService.first;
                DebugPrintf(SV_LOG_ERROR, exceptionMsg.str().c_str());
                continue;
            }
            DebugPrintf(SV_LOG_DEBUG, "Changed Service %s Start Type to %d\n", itService.first.c_str(), startType);
        }

        if (INM_SVCTYPE_DISABLED == startType) {

            if (INM_SVCSTAT_STOPPED == svcStatus) {
                DebugPrintf(SV_LOG_DEBUG, "Service %s is already stopped. Skipping stop\n", itService.first.c_str());
                continue;
            }

            // Stop the service
            if (StpService(itService.first) != SVS_OK)
            {
                std::stringstream exceptionMsg;
                exceptionMsg << "Could not stop the service: "
                    << itService.first;

                DebugPrintf(SV_LOG_ERROR, exceptionMsg.str().c_str());
            }
            DebugPrintf(SV_LOG_DEBUG, "Stopped Service %s successfully\n", itService.first.c_str());
            continue;
        }

        if (INM_SVCTYPE_AUTO == startType) {
            if (INM_SVCSTAT_RUNNING == svcStatus) {
                DebugPrintf(SV_LOG_DEBUG, "Service %s is already running. Skipping starting\n", itService.first.c_str());
                continue;
            }

            if (StartSvc(itService.first) != SVS_OK)
            {
                std::stringstream exceptionMsg;
                exceptionMsg << "Could not start the service: "
                    << itService.first;

                DebugPrintf(SV_LOG_ERROR, exceptionMsg.str().c_str());
                continue;
            }
            DebugPrintf(SV_LOG_DEBUG, "Started Service %s successfully\n", itService.first.c_str());
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::DisableEnableVMWareTools(bool bEnable)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    std::map<std::string, InmServiceStartType>        platformServices;
    InmServiceStartType startType = bEnable ? INM_SVCTYPE_AUTO : INM_SVCTYPE_DISABLED;

    platformServices.insert(std::make_pair(VMWARE_TOOLS_SERVICE_NAME, startType));
    DisableEnablePlatformServices(platformServices);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::DisableEnableAzureServices(bool bEnable)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::map<std::string, InmServiceStartType>        platformServices;
    InmServiceStartType startType = bEnable ? INM_SVCTYPE_AUTO : INM_SVCTYPE_DISABLED;

    try {
        LocalConfigurator localConfigurator;
        std::string azureServices = localConfigurator.getAzureServices();

        boost::char_separator<char> delimiter(",");

        boost::tokenizer < boost::char_separator<char> > services(azureServices, delimiter);
        for (auto service : services) {
            // Remove all leading and trailing whitespace
            auto serviceName = std::regex_replace(service, std::regex("^ +| +$"), "$1");
            DebugPrintf(SV_LOG_DEBUG, "Adding service %s StartType: %d\n", serviceName.c_str(), startType);
            platformServices.insert(std::make_pair(serviceName, startType));
        }
    }
    catch (const std::exception &ex) {
        DebugPrintf(SV_LOG_ERROR, "Failed to add azure services. exception: %s\n", ex.what());
        return;
    }

    DisableEnablePlatformServices(platformServices);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::ResetReplicationState()
{
    //
    // Resets the replication state on recovered VM by making following changes:
    //    1. Reset the filter driver state & clear the bitmap files if exist
    //    2. Clear the cache settings
    //

    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    //
    // Call drvutil to stop filtering & delete bitmap files
    //
    LocalConfigurator lConfig;
    std::string strDrvUtilCmd = lConfig.getInstallPath();
    BOOST_ASSERT(!strDrvUtilCmd.empty());
    boost::trim(strDrvUtilCmd);
    if (!boost::ends_with(strDrvUtilCmd, ACE_DIRECTORY_SEPARATOR_STR_A))
        strDrvUtilCmd += ACE_DIRECTORY_SEPARATOR_STR_A;
    strDrvUtilCmd += "drvutil.exe --stopf -stopall -deletebitmap";

    DebugPrintf(SV_LOG_ALWAYS, "Running the command: %s\n",
        strDrvUtilCmd.c_str());

    int exitCode = 0;
    std::string strCmdOut, strCmdError;
    if (!RunInmCommand(strDrvUtilCmd, strCmdOut, strCmdError, exitCode))
    {
        DebugPrintf(SV_LOG_ERROR, "Command execution error.\n");
        DebugPrintf(SV_LOG_ERROR, "Exit code: %d\n", exitCode);
        DebugPrintf(SV_LOG_ERROR, "-----stderror----\n%s\n",
            strCmdError.c_str());
        DebugPrintf(SV_LOG_ERROR, "-----stdout----\n%s\n",
            strCmdOut.c_str());

        THROW_HOST_REC_EXCEPTION(
            "Replication state cleanup failed. Manual intervention is required."
            );
    }

    DebugPrintf(SV_LOG_INFO, "-----stdout----\n%s\n", strCmdOut.c_str());

    //
    // Clear chache settings files.
    //
    std::list<std::string> lstFilesToRemove;
    std::string configFilesPath;
    if (LocalConfigurator::getConfigDirname(configFilesPath))
    {
        BOOST_ASSERT(!configFilesPath.empty());
        boost::trim(configFilesPath);
        if (!boost::ends_with(configFilesPath, ACE_DIRECTORY_SEPARATOR_STR_A))
            configFilesPath += ACE_DIRECTORY_SEPARATOR_STR_A;

        std::string cleanupFileList = lConfig.getRecoveryCleanupFileList();
        boost::char_separator<char> delm(",");
        boost::tokenizer < boost::char_separator<char> > strtokens(cleanupFileList, delm);
        for (boost::tokenizer< boost::char_separator<char> > ::iterator it = strtokens.begin(); it != strtokens.end(); ++it)
        {
            /// remove leading and trailing white space if any
            std::string filename = *it;
            boost::trim(filename);
            lstFilesToRemove.push_back(configFilesPath + filename);
        }
    }
    else
    {
        THROW_HOST_REC_EXCEPTION(
            "Could not get the config directory path. Cache files cleanup won't happen"
            );
    }

    std::list<std::string>::const_iterator iterFile = lstFilesToRemove.begin();
    for (; iterFile != lstFilesToRemove.end(); iterFile++)
    {
        boost::filesystem::path cache_file(*iterFile);
        if (boost::filesystem::exists(cache_file))
        {
            DebugPrintf(SV_LOG_INFO, "Removing the file %s\n", iterFile->c_str());
            boost::filesystem::remove(cache_file);
        }
        else
        {
            DebugPrintf(SV_LOG_INFO, "File %s does not exist.\n", iterFile->c_str());
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

bool HostRecoveryManager::OnlineOfflineResourceDisk(bool bOnline)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    InmageDriverInterface      inmageDriverIntf;
    std::set<SV_ULONG>      diskIndexList;

    if (!inmageDriverIntf.GetDiskIndexList(diskIndexList)) {
        DebugPrintf(SV_LOG_ERROR, "Driver GetDiskIndexList failed with error=%d\n", GetLastError());
        return false;
    }

    std::set<SV_ULONG>  systemDiskIndices;
    std::string         err;
    DWORD               errcode;

    if (!GetSystemDiskList(systemDiskIndices, err, errcode)) {
        DebugPrintf(SV_LOG_ERROR, "Failed to get system disk err=%s errCode=%d\n", err.c_str(), errcode);
        return false;
    }

    if (systemDiskIndices.size() == 0) {
        DebugPrintf(SV_LOG_ERROR, "Failed to get system disk.. No system disk\n");
        return false;
    }

    std::stringstream   ssError;
    if (systemDiskIndices.size() > 1) {
        ssError << "More than one system disks:";
        for (auto diskIndex : systemDiskIndices) {
            ssError << " " << diskIndex;
        }
        DebugPrintf(SV_LOG_ERROR, "%s\n", ssError.str().c_str());
        return false;
    }

    SV_ULONG    ulSystemDiskIndex = *(systemDiskIndices.begin());

    //Offline/Online Resource Disk
    for (auto diskIndex : diskIndexList) {
        if (ulSystemDiskIndex == diskIndex) {
            DebugPrintf(SV_LOG_DEBUG, "Resource Disk Detection: Skipping disk %d as it is boot disk\n", diskIndex);
            continue;
        }

        DiskInterface   diskIntf(diskIndex);
        if (diskIntf.IsResourceDisk()) {
            DebugPrintf(SV_LOG_INFO, "%s Resource Disk %d\n", (bOnline)? "ONLINE" : "OFFLINE", diskIndex);
            return (bOnline) ? diskIntf.OnlineDisk() : diskIntf.OfflineDisk();
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return false;
}

bool HostRecoveryManager::IsDiskRecoveryRequired(InmageDriverInterface&  inmageDriverIntf)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    LocalConfigurator   lConfig;

    bool isRecoveryRequired = false;

    SV_UINT uiWaitTimeSec = lConfig.GetDiskRecoveryWaitTime();

    if (uiWaitTimeSec > 0) {
        DebugPrintf(SV_LOG_ALWAYS, "Waiting for %d secs before invoking Disk Recovery WF\n", uiWaitTimeSec);
        Sleep(uiWaitTimeSec * 1000);
    }

    if (IsWindowsVistaOrGreater() && !IsWindows7OrGreater()) {
        DebugPrintf(SV_LOG_ALWAYS, "Disk Recovery always needed on windows 2008\n");
        return true;
    }

    if (inmageDriverIntf.IsDiskRecoveryRequired()) {
        DebugPrintf(SV_LOG_ALWAYS, "Disk recovery set by driver\n");
        DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
        return true;
    }

    // If system crashed after driver updated boot information.. we need to rely upon registry keys

    std::map<std::string, DWORD> DriversMap = {
        { "storvsc", SERVICE_BOOT_START },
        { "vmbus", SERVICE_BOOT_START }
    };

    DWORD dwSuccess = RegistryInfo::GetDriversStartType(DriversMap);
    if (ERROR_SUCCESS != dwSuccess) {
        DebugPrintf(SV_LOG_DEBUG, "Failed to query start type for drivers stovsc, vmbus\n");
        return false;
    }

    for (auto driver : DriversMap) {
        DebugPrintf(SV_LOG_DEBUG, "Driver %s StartType: %d\n", driver.first.c_str(), driver.second);
        if (SERVICE_BOOT_START != driver.second) {
            DebugPrintf(SV_LOG_ALWAYS, "Driver %s Type %d Disk Recovery is required\n", driver.first.c_str(), driver.second);
            isRecoveryRequired = true;
            break;
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return isRecoveryRequired;
}

void HostRecoveryManager::RunDiskRecoveryWF(bool& isRebootRequired)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    LocalConfigurator       lc;
    std::set<SV_ULONG>      diskIndexList;

    InmageDriverInterface      inmageDriverIntf;

    isRebootRequired = false;

    if (!lc.isMobilityAgent()) {
        DebugPrintf(SV_LOG_DEBUG, "Skipping resetting vm details as it is not mobility agent\n");
        return;
    }

    std::map<std::string, DWORD> DriversMap = {
        { "storvsc", SERVICE_BOOT_START },
        { "vmbus", SERVICE_BOOT_START }
    };

    bool isRecoveryRequired = IsDiskRecoveryRequired(inmageDriverIntf);
    if (!isRecoveryRequired) {
        DebugPrintf(SV_LOG_ALWAYS, "Disk recovery not needed.. Skipping recovery\n");
        DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
        return;
    }

    for (auto driver : DriversMap) {
        DriversMap[driver.first] = SERVICE_BOOT_START;
    }

    RegistryInfo::SetDriversStartType(DriversMap);

    HRESULT         hr = S_OK;
    bool            isDynamic = false;

    if (!inmageDriverIntf.GetDiskIndexList(diskIndexList)) {
        DebugPrintf(SV_LOG_ERROR, "Driver GetDiskIndexList failed with error=%d\n", GetLastError());
        return;
    }

    for (auto diskIndex : diskIndexList) {
        DWORD   dwErr = IsDiskDynamic(diskIndex, isDynamic);

        if (ERROR_SUCCESS != dwErr) {
            DebugPrintf(SV_LOG_ERROR, "Failed to Query DiskType for Disk %d.. Reboot Required", diskIndex);
            isRebootRequired = true;
            break;
        }

        if (isDynamic) {
            DebugPrintf(SV_LOG_ALWAYS, "Disk %d is Dynamic.. Reboot Required", diskIndex);
            isRebootRequired = true;
            break;
        }

        DebugPrintf(SV_LOG_INFO, "Disk %d is Basic\n", diskIndex);
    }

    if (!isRebootRequired) {
        for (auto diskIndex : diskIndexList) {
            DiskInterface   diskIntf(diskIndex);
            if (!diskIntf.IsResourceDisk()) {
                DebugPrintf(SV_LOG_ALWAYS, "Onlining Disk %d\n", diskIndex);
                diskIntf.OnlineDisk();
            }
        }
        DebugPrintf(SV_LOG_ALWAYS, "Onlining Resource Disk\n");
        OnlineOfflineResourceDisk(true);
        DebugPrintf(SV_LOG_ALWAYS, "Reboot not required as system contains only basic disks\n");
        return;
    }

    DebugPrintf(SV_LOG_ALWAYS, "Setting san policy to online for all disks\n");
    inmageDriverIntf.SetSanPolicyToOnlineForAllDisks();

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}
