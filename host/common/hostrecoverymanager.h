//---------------------------------------------------------------
//  <copyright file="hostrecoverymanager.h" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
//
//  Description: HostRecoveryManager header file
//
//  History:	15-Aug-2016		veshivan	Created
//
//----------------------------------------------------------------

#ifndef HOSTRECOVERYMANAGER__H
#define HOSTRECOVERYMANAGER__H

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <stdexcept>
#include <map>

#include "service.h"

#ifdef SV_WINDOWS
class InmageDriverInterface;
#endif
//
// Class : HostRecoveryManager
//
// Description: A helper class with the functions to fecilitate  
//   recovery operations at guest level, such as: 
//   -> discover and persist guest info for recovery detection
//   -> detect the recovery 
//   -> perform necessary actions on recovery.
//
class HostRecoveryManager
{
    //
    // Retrieves the persisted VM info
    //
    static bool GetPersistedVMInfo(std::string& persistedHypervisorName,
        std::string& persistedSystemUUID,
        bool& persistedIsAzureVm);

    // 
    // Persists the given VM info
    //
    static void PersistVMInfo(const std::string& persistedHypervisorName,
        const std::string& persistedSystemUUID,
        bool persistedIsAzureVm);

    //
    // Checks jf given VM info matches with the persisted VM info.
    //
    // Returns true if given VM info matche with persisted VM info,
    // otherwise false.
    //
    static bool IsVMInfoMatching(const std::string& hypervisor,
        const std::string& systemUUID,
        bool persistedIsAzureVm);

    //
    // Updates the host-id
    //
    static void UpdateHostId(const std::string& hostId);

    //
    // Enable/Disable VMWare Tools on the VM
    //
    static void DisableEnableVMWareTools(bool bEnable);

    //
    // Enable/Disable platform specific tools on recovery
    //
    static void DisableEnablePlatformTools();

    static void DisableEnablePlatformServices(std::map<std::string, InmServiceStartType> platformServices);

    //
    // Enable/Disable Azure Services on the VM
    //
    static void DisableEnableAzureServices(bool bEnable);

    //
    // Reset resource Id.
    //
    static void ResetResourceId();

    //
    // Persist system uuid
    //
    static void PersistSystemUUId(const std::string& persistedSystemUUID);

    //
    // Persist hypervisor info
    //
    static void PersistHypervisorInfo(const std::string& persistedHypervisorName);

    //
    // Persiste IsAzureVm flag
    //
    static void PersistIsAzureVm(bool bAzureVm);

    /// get the information about hydration, clone, if agent is running on an Azure VM
    static void GetRecoveryInfo(bool & bIsHydrationWorkflow,
        bool& bHypervisorChanged,
        bool& bSystemUuidChanged,
        bool& bIsAzureVm,
        bool& bVmTypeChanged,
        bool& bIsFailoverDetected);

public:
#ifdef SV_WINDOWS
    static void RunDiskRecoveryWF(bool& isRebootRequired);
    static bool OnlineOfflineResourceDisk(bool bOnline);
    static bool IsDiskRecoveryRequired(InmageDriverInterface&  inmageDriverIntf);
#endif
    //
    // Determines does the recovery is in progress. It considers both
    // hydration and zero-hydration factors to determine the recovery
    // in progress situation. It also sets bIsHydrationWorkflow=true if
    // the recovery is via hydration.
    //
    // Returns true if the recovery is in progress, otherwise false.
    //
    static bool IsRecoveryInProgress(bool & bIsHydrationWorkflow)
    {
        bool clonePlaceHolder;
        return IsRecoveryInProgress(bIsHydrationWorkflow, clonePlaceHolder);
    }

    /// \brief for V2A Legacy implementation
    /// \returns
    /// \li true if
    ///         a hydration is detected on Windows OS
    ///         a clone is detected
    ///         a no-hydration recovery required on Windows OS
    /// \li false otherwise
    static bool IsRecoveryInProgress(bool & bIsHydrationWorkflow, bool& bIsClone);

    /// \brief for V2A RCM implementation
    /// \returns
    /// \li true if
    ///         a hydration is detected on Windows OS
    ///         a clone is detected
    ///         a no-hydration recovery requried on Windows OS
    ///         a a recovery required on Linux OS on failover
    /// \li false otherwise
    static bool IsRecoveryInProgressEx(bool & bIsHydrationWorkflow, bool& bIsClone);

    //
    // Determines does the recovery is in progress. It considers both
    // hydration and zero-hydration factors to determine the recovery
    // in progress situation.
    //
    // Returns true if the recovery is in progress, otherwise false.
    //
    static bool IsRecoveryInProgress(void)
    {
        bool placeHolder;
        bool clonePlaceHolder;
        return IsRecoveryInProgress(placeHolder, clonePlaceHolder);
    }

    //
    // Resets the persisted VM info which is used for detecting the
    // recovery state of the VM.
    // 
    static void ResetVMInfo(void);

    //
    // Executes the necessary steps required to complete recovery
    //
    static void CompleteRecovery();

    static void CompleteRecovery(bool IsClone);
    //
    // Resets the replication state on recovered VM 
    //
    static void ResetReplicationState();
};


//
// Class : HostRecoveryException
// 
// Description: Exception for Host Recovery Operations
//
class HostRecoveryException : public std::exception 
{
public:
    HostRecoveryException(const char* filename, int line, const char* function)
    {
        std::ostringstream stream;
        stream << filename << '(' << line << ")[" << function << ']';
        m_str = stream.str();
    }

    ~HostRecoveryException() throw() {}

    template <typename T>
    HostRecoveryException& operator()(T const& t) {
        std::ostringstream stream;
        stream << " : " << t;
        m_str += stream.str();
        return *this;
    }

    HostRecoveryException& operator()() { return *this; }

    const char* what() const throw() {
        return m_str.c_str();
    }

private:
    mutable std::string m_str;
};

#define THROW_HOST_REC_EXCEPTION(x) throw HostRecoveryException(__FILE__,__LINE__,__FUNCTION__)(x)

#endif // ~ HOSTRECOVERYMANAGER__H
