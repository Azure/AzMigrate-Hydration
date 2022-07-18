/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: RecoveryHelpers.h

Description	: Delcarations for windows recovery helper functions.

History		: 1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_HELPERS_H
#define AZURE_RECOVERY_HELPERS_H

#include <string>
#include "OSVersion.h"

namespace AzureRecovery
{
    namespace HiveFlags
    {
        BYTE None = 0;
        BYTE System = 1;
        BYTE Software = 2;
        BYTE All = 3;
    }

    bool AddWindowsGuestAgent(const std::string& srcOSVolume, std::stringstream& errorMsg);

    bool TransferWinGARegistrySettings(std::string srcOSVol, std::string& imagePath, std::stringstream& errorstream);

    bool VerifyRegistrySettingsForWinGA(const std::string& srcOSVolume, std::stringstream& errorstream);

    bool VerifyDisks(std::string& errorMsg);

    bool PrepareActivePartitionDrive(std::string& drivePath, std::string& errorMsg);

    bool PrepareSourceSystemPath(std::string& srcSystemPath, std::string& errorMsg);

    bool UpdateBIOSBootRecordsOnOSDisk(const std::string& srcSystemPath, const std::string& activePartitionDrivePath, const std::string& firmwareType,std::string& errorMsg);

    bool PrepareSourceOSDrive(std::string& osVolMountPath, const std::string& osDiskId = "");

    bool PrepareSourceRegistryHives(const std::string& sourceSystemPath, std::string& errorMsg, BYTE hiveFlag = HiveFlags::All);

    bool VerifyBootDriversFiles(const std::string& srcOSInstallPath);

    bool MakeChangesForBootDriversOnRegistry(const std::string& srcOSInstallPath, std::string& errorMsg, int &retCode);

    bool MakeChangesForServicesOnRegistry(const std::string& srcOSInstallPath, std::string& errorMsg);

    bool BackupSanPolicy(std::string& errorMsg);

    bool SetSANPolicy(std::string& errorMsg, DWORD dwSanPolicy, bool backupOriginal = true);

    bool SetAutoMount(std::string& errorMsg, bool disableAutoMount);

    bool SetBootDriversStartTypeOnRegistry();

    bool SetBootupScript(std::string& errorMsg);

    bool SetBootupScriptEx(std::string& errorMsg);

    void SetBootupScriptOptions();

    bool UpdateNewHostId(std::string& errorMsg);

    bool ResetInvolfltParameters(std::string& errorMsg);

    bool SetRdpParameters(std::string& errorMsg);

    bool ReleaseSourceRegistryHives(std::string& errorMsg, BYTE hiveFlag = HiveFlags::All);

    void SetHelpersLevelErrorMsg();

    bool GetOsVersion(OSVersion& winVer);

    bool OnlineDisks(std::string& errorMsg);

    bool DiscoverSourceOSVolumes(std::list<std::string>& osVolumes, std::string& errorMsg);

    bool MakeRegistryChangesForMigration(const std::string& osInstallPath,
        const std::string& srcOsVol,
        int& retcode,
        std::string& curTaskDesc,
        std::stringstream& errStream,
        bool disableAutomount);

    bool TransferGuestAgentService(
        std::string& serviceToTransfer,
        std::string& currentControlSet,
        std::string& srcOSVol,
        std::string& controlSetNum,
        std::string& imagePath,
        std::stringstream& errorstream);

    void SetOSVersionDetails(const std::string& osInstallPath);

    std::string GetHydrationConfigValue(
        const std::string& hydrationConfigSettings,
        const std::string& hydrationConfigKeyToCheck);

    bool IsDotNetFxVersionPresent(const std::string& srcOSVol);
}

#endif // ~AZURE_RECOVERY_HELPERS_H