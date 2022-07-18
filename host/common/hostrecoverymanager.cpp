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

#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

#include "hostrecoverymanager.h"
#include "portablehelpersmajor.h"
#include "localconfigurator.h"
#include "service.h"

#include "biosidoperations.h"

using namespace std;

#define VMWARE_TOOLS_SERVICE_NAME "VMTools"
const std::string INVALID_UUID("00000000-0000-0000-0000-000000000000");

bool HostRecoveryManager::GetPersistedVMInfo(string& persistedHypervisorName,
                                             string& persistedSystemUUID,
                                             bool& persistedIsAzureVm)
{
    LocalConfigurator   lc;
    persistedIsAzureVm = lc.getIsAzureVm();

    return
        lc.getHypervisorName(persistedHypervisorName) &&
        lc.getSystemUUID(persistedSystemUUID);
}

void HostRecoveryManager::PersistSystemUUId(const std::string& systemUUID)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    LocalConfigurator   lc;

    if (systemUUID.empty()) {
        DebugPrintf(SV_LOG_ERROR,"Skipping persist uuid as SystemUUID is empty");
        return;
    }

    try {
        lc.setSystemUUID(systemUUID);
    }
    catch (ContextualException &e) {
        DebugPrintf(SV_LOG_ERROR, "Failed to update systemuuid with exception: %s\n", e.what());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::PersistHypervisorInfo(const std::string& hypervisorName)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    LocalConfigurator   lc;

    if (hypervisorName.empty()) {
        DebugPrintf(SV_LOG_ERROR, "Skipping persist hypervisor as Hypervisor info is empty");
        return;
    }

    try {
        lc.setHypervisorName(hypervisorName);
    }
    catch (ContextualException &e) {
        DebugPrintf(SV_LOG_ERROR, "Failed to update hypervisor with exception: %s\n", e.what());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::PersistIsAzureVm(bool bAzureVm)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    LocalConfigurator   lc;

    try {
        lc.setIsAzureVm(bAzureVm);
    }
    catch (ContextualException &e) {
        DebugPrintf(SV_LOG_ERROR, "Failed to update isAzureVm with exception: %s\n", e.what());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::PersistVMInfo(const string& hypervisorName,
                                        const string& systemUUID,
                                        bool bIsAzureVm)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    if (hypervisorName.empty() || systemUUID.empty()) {
        DebugPrintf(SV_LOG_ERROR,
            "%s: hypervisor or, systemUUID is empty", FUNCTION_NAME);
        return;
    }

    PersistSystemUUId(systemUUID);
    PersistHypervisorInfo(hypervisorName);
    PersistIsAzureVm(bIsAzureVm);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

bool HostRecoveryManager::IsVMInfoMatching(const std::string& hypervisor,
    const std::string& systemUUID,
    bool bIsAzureVm)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    bool bRecInfoMatching = false;

    std::string persistedHypervisorName;
    std::string persistedSystemUUID;
    bool persistedIsAzureVm;

    //
    // Retrieve persisted info and compare. If persisted info is 
    // not present then GetPersistedRecoveryInfo() will return false
    // and retun false as nothing to compare.
    //
    if (!GetPersistedVMInfo(
        persistedHypervisorName,
        persistedSystemUUID,
        persistedIsAzureVm))
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to get Persisted info\n");
        return false;
    }

    bRecInfoMatching =
        boost::iequals(hypervisor, persistedHypervisorName) &&
        boost::iequals(systemUUID, persistedSystemUUID) &&
        (persistedIsAzureVm == bIsAzureVm);

    DebugPrintf(SV_LOG_ALWAYS,
        "Persisted VM Info: Hypervisor: %s, System UUID: %s, IsAzureVM %d\n",
        persistedHypervisorName.c_str(),
        persistedSystemUUID.c_str(),
        persistedIsAzureVm);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return bRecInfoMatching;
}

bool HostRecoveryManager::IsRecoveryInProgress(bool & bIsHydrationWorkflow, bool& bIsClone)
{
    bool bHypervisorChanged = false,
        bSystemUuidChanged = false,
        bIsAzureVm = false,
        bVmTypeChanged = false,
        bIsRecoveryInProgress = false,
        bIsFailoverDetected = false;

    GetRecoveryInfo(bIsHydrationWorkflow,
        bSystemUuidChanged,
        bHypervisorChanged,
        bIsAzureVm,
        bVmTypeChanged,
        bIsFailoverDetected);

    if (bIsHydrationWorkflow)
        return bIsHydrationWorkflow;

    if (!bSystemUuidChanged)
        return bSystemUuidChanged;

    // this is to retain the existing V2A Legacy behavior. this should be same as IsRecoveryInProgressEx
    // after handling Linux post hydration steps in agent instead of vCon scripts at boot time.
    // or remove on EOL of V2A Legacy
#ifdef SV_WINDOWS
    bIsRecoveryInProgress = bIsAzureVm;
#endif

    bIsClone = !bHypervisorChanged && !bIsAzureVm;

    return (bIsRecoveryInProgress || bIsClone);
}

bool HostRecoveryManager::IsRecoveryInProgressEx(bool & bIsHydrationWorkflow, bool& bIsClone)
{
    bool bHypervisorChanged = false,
        bSystemUuidChanged = false,
        bIsAzureVm = false,
        bVmTypeChanged = false,
        bIsRecoveryInProgress = false,
        bIsFailoverDetected = false;


    GetRecoveryInfo(bIsHydrationWorkflow,
        bSystemUuidChanged,
        bHypervisorChanged,
        bIsAzureVm,
        bVmTypeChanged,
        bIsFailoverDetected);

    if (bIsHydrationWorkflow)
        return bIsHydrationWorkflow;

    if (!bSystemUuidChanged)
        return bSystemUuidChanged;

    if (bIsFailoverDetected)
        return bIsFailoverDetected;

    bIsRecoveryInProgress = bSystemUuidChanged;

    bIsClone = !bHypervisorChanged && !bVmTypeChanged;

    return (bIsRecoveryInProgress || bIsClone);
}

void HostRecoveryManager::GetRecoveryInfo(bool & bIsHydrationWorkflow,
    bool& bSystemUuidChanged,
    bool& bHypervisorChanged,
    bool& bIsAzureVm,
    bool& bVmTypeChanged,
	bool& bIsFailoverDetected)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    bIsHydrationWorkflow = false;
    bHypervisorChanged = false;
    bSystemUuidChanged = false;
    bIsAzureVm = false;
    bVmTypeChanged = false;

    LocalConfigurator   lc;

    // Recovery is only meaningful for mobility agent
    if (!lc.isMobilityAgent()) {
        DebugPrintf(SV_LOG_DEBUG, "Not running as mobility agent.. skipping recovery check\n");
        return;
    }

#ifdef SV_WINDOWS
    //
    // ### Hydration ###
    ///
    // Check if it is hydration workflow.
    // 
    bIsHydrationWorkflow = ::IsRecoveryInProgress();
    if (bIsHydrationWorkflow)
    {
        DebugPrintf(SV_LOG_INFO,
            "Hydration has happened. Recovery is in pogress.");

        DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
        return;
    }
#endif
    //
    // ### No-Hydration / Clone ###
    // 

    //
    // Check if the Persisted VM info availability
    //
    std::string persistedHypervisorName;
    std::string persistedSystemUUID;
    bool persistedIsAzureVm;

    if (GetPersistedVMInfo(persistedHypervisorName, persistedSystemUUID, persistedIsAzureVm))
    {
        DebugPrintf(SV_LOG_INFO,
            "Persisted VM info available: Hypervisor: %s, System UUID: %s, IsAzureVm %d\n",
            persistedHypervisorName.c_str(),
            persistedSystemUUID.c_str(),
            persistedIsAzureVm);
    }
    else
    {
        DebugPrintf(SV_LOG_INFO,
            "Persisted VM info not available. It might not be a recovery.\n");

        return;
    }

    //
    // Discover the System UUID and Hypervisor
    //
    string systemUUID = GetSystemUUID();

    // Skip recovery detection if System UUID is empty.
    if (systemUUID.empty()) {
        DebugPrintf(SV_LOG_ERROR, "IsRecoveryInProgress: Could not retrieve System UUID or got empty UUID.");
        return;
    }

    if (boost::iequals(systemUUID, INVALID_UUID)) {
        DebugPrintf(SV_LOG_ERROR, "IsRecoveryInProgress: Got Invalid UUID.");
        return;
    }

    string hypervisor, hypervisorversion;
    if (!IsVirtual(hypervisor, hypervisorversion))
    {
        hypervisor = PHYSICALMACHINE;
    }

    bSystemUuidChanged = !boost::iequals(systemUUID, persistedSystemUUID);

    if (persistedHypervisorName.empty() ||
        persistedSystemUUID.empty() ||
        boost::iequals(persistedSystemUUID, INVALID_UUID) ||
        !bSystemUuidChanged) {
        DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
        return;
    }

    //
    // Compare discovered VM info with persisted values to detect the VM recovery.
    // On Vmware if system UUID changes it is a clone
    // On failover if system UUID changes
    //
    bIsAzureVm = IsAzureVirtualMachine();

    // If hypervisor has not changed, it means it is a clone except when 
    //      Hyper-v VM is migrated to Azure,
    //      Clone of Azure VM after failover from on-prem - [Migraton of Azure VM not supported using V2A,
    //          use A2A and such recovery is handled in AzureVmRecoveryManager ]
    //      Azure stack to Azure migration - [ this is not handled by this logic.
    //          it is detected as clone and that is fine as no failback is supported ]
    //      AVS scenario involved where the decision is made based on the FailoverVmBiosid received from rcm
    //          it is detected as failover if the rcm received biosid matches with the current system biosid
    // Note that the only difference between clone and failover is to set the new hostId as BIOS-ID and the source control plane
    // for the AVS scenario
    // (see CompleteRecovery()). Except for the V2A Legacy there is no such requirement in other providers. 
    bHypervisorChanged = !boost::iequals(hypervisor, persistedHypervisorName);
    bVmTypeChanged = (bIsAzureVm != persistedIsAzureVm);

    DebugPrintf(SV_LOG_INFO,
        "Current VM Info: Hypervisor: %s, System UUID: %s, IsAzureVm %d\n",
        hypervisor.c_str(),
        systemUUID.c_str(),
        bIsAzureVm);

    string failoverVmBiosId = lc.getFailoverVmBiosId();
    if (!failoverVmBiosId.empty()){
        DebugPrintf(SV_LOG_DEBUG, "Failover Vm BiosId is %s.\n",
            failoverVmBiosId.c_str());
    }

    if ( bVmTypeChanged || (bHypervisorChanged || 
        (!failoverVmBiosId.empty() && (boost::iequals(failoverVmBiosId, systemUUID) ||
        boost::iequals(failoverVmBiosId, BiosID::GetByteswappedBiosID(systemUUID)))))) {
        bIsFailoverDetected = true;
        DebugPrintf(SV_LOG_ALWAYS, "Failover Detected Persisted UUID: %s CurrentUUID: %s\n", persistedSystemUUID.c_str(), systemUUID.c_str());
    }
    else
    {
        DebugPrintf(SV_LOG_ALWAYS, "Clone Detected Persisted UUID: %s CurrentUUID: %s\n", persistedSystemUUID.c_str(), systemUUID.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return;
}

void HostRecoveryManager::ResetVMInfo(void)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    // VM Info is applicable only for mobility agent
    LocalConfigurator   lc;
    if (!lc.isMobilityAgent()) {
        DebugPrintf(SV_LOG_DEBUG, "Skipping resetting vm details as it is not mobility agent\n");
        return;
    }

    //
    // Discover the hypervisor and get the system UUID
    //
    string systemUUID = GetSystemUUID();
    string hypervisor, hypervisorVersion;
    if (!IsVirtual(hypervisor, hypervisorVersion))
    {
        hypervisor = PHYSICALMACHINE;
    }

    //
    // Skip Reset VM Info if hypervisor and UUID are empty.
    //
    if (hypervisor.empty() || systemUUID.empty()) {
        DebugPrintf(SV_LOG_ERROR, "ResetVMInfo: Hypervisor and System UUID should not be empty.");
        return;
    }

    bool bIsAzureVm = IsAzureVirtualMachine();

    DebugPrintf(SV_LOG_INFO,
        "Current VM Info: Hypervisor: %s, System UUID: %s, IsAzureVm %d\n",
        hypervisor.c_str(),
        systemUUID.c_str(),
        bIsAzureVm);

    //
    // Compare the discovered info with persisted info
    //
    if (IsVMInfoMatching(hypervisor, systemUUID, bIsAzureVm))
    {
        DebugPrintf(SV_LOG_DEBUG,
            "No change detected in IsAzureVM, Hypervisor and SystemUUID.\n");
        return;
    }

    PersistVMInfo(hypervisor, systemUUID, bIsAzureVm);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::UpdateHostId(const std::string& hostId)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::string     newHostId = boost::trim_copy(hostId);

    if (newHostId.empty()) {
        DebugPrintf(SV_LOG_ERROR, "UpdateHostId: HostId should not be empty");
        return;
    }

    //
    // Update HostId in drscount.conf file
    //
    LocalConfigurator lConfig;
    lConfig.setHostId(newHostId);

    DebugPrintf(SV_LOG_ALWAYS, "Updated HostId: %s\n", newHostId.c_str());

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::ResetResourceId()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::string     oldResourceId;
    //
    // Reset ResourceId in drscount.conf file
    // Vx Service will automatically generate this id
    //
    LocalConfigurator lConfig;
    oldResourceId = lConfig.getResourceId();

    DebugPrintf(SV_LOG_ALWAYS, "Resetting ResourceId Old ResourceId: %s\n", oldResourceId.c_str());

    lConfig.setResourceId("");

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::CompleteRecovery()
{
    return CompleteRecovery(false);
}

void HostRecoveryManager::CompleteRecovery(bool bClone)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    //
    // Reset the replication on recovered VM
    //
    DebugPrintf(SV_LOG_INFO, "Resetting replication state.\n");
    ResetReplicationState();

    //
    // Disable/Enable underlying Hypervisor related tools/services
    // for failover VM
    //
#ifdef SV_WINDOWS
    bool isRebootRequired = false;
#endif

    if (!bClone) {
        DebugPrintf(SV_LOG_ALWAYS, "Disabling or Enabling hypervisor tools.\n");
        DisableEnablePlatformTools();

#ifdef SV_WINDOWS
        RunDiskRecoveryWF(isRebootRequired);
#endif
    }

    //
    // Update the HostId.
    //      For clone generate a new host ID.
    //      For no-hydration failover use system uuid.
    //
    std::string     newHostId = "";
    std::string     oldHostId;

    LocalConfigurator   lc;
    oldHostId = lc.getHostId();

    if (!bClone) {
        newHostId = GetSystemUUID();
    }
    else {
        boost::uuids::uuid guid = boost::uuids::random_generator()();
        newHostId = boost::lexical_cast<string>(guid);

        ResetResourceId();
    }

    if (!oldHostId.empty()) {
        DebugPrintf(SV_LOG_ALWAYS, "%s detected Old Host Id: %s New HostId : %s.\n", (bClone) ? "Clone" : "Failover", oldHostId.c_str(), newHostId.c_str());
    }
    else {
        DebugPrintf(SV_LOG_ALWAYS, "%s detected Updating HostId with : %s.\n", (bClone) ? "Clone" : "Failover", newHostId.c_str());
    }

    UpdateHostId(newHostId);

    //
    // Reset the vm info to current system values. And this step should be the 
    // last one in failover/clone.
    //
    ResetVMInfo();

#ifdef SV_WINDOWS
    if (isRebootRequired) {
        DebugPrintf(SV_LOG_ALWAYS, "Azure Site Recovery: Shutting down system to recover dynamic disks\n");

        if (!RebootMachine()) {
            system("shutdown /r /t 0 /c \"Azure Site Recovery: Fixing Failover VM\"");
        }
    }
#endif

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}

void HostRecoveryManager::DisableEnablePlatformTools()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::string hypervisor, hypervisorVersion;
    if (!IsVirtual(hypervisor, hypervisorVersion))
    {
        hypervisor = PHYSICALMACHINE;
    }

    DisableEnableVMWareTools(boost::iequals(hypervisor, VMWARENAME));

#ifdef SV_WINDOWS
    // Enable Azure services if needed
    DisableEnableAzureServices(IsAgentRunningOnAzureVm());
#endif

    //
    // Based on the requirements, add other platform specific tools handling 
    // logic routine and call it here similar to DisableEnableVMWareTools().
    //

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
}