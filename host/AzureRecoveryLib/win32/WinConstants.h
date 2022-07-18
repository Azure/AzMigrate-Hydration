/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WinConstants.h

Description	:   String constants

History		:   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_WIN_CONSTANTS_H
#define AZURE_WIN_CONSTANTS_H

#include <vds.h>

namespace AzureRecovery
{
    const char DIRECOTRY_SEPERATOR[] = "\\";
    const char STARTUP_EXE_NAME[] = "EsxUtil.exe";
    const char NW_CHANGES_FILE_PATH[] = "Failover\\Data\\nw.txt";
    const DWORD MAX_PATH_SIZE = 23767;

    namespace BCD_TOOLS
    {
        const char BCDEDIT_EXE[] = "bcdedit.exe";
        const char BCDBOOT_EXE[] = "bcdboot.exe";
        const char EFI_PATH[] = "EFI\\Microsoft\\Boot";
        const char EFI_BCD_PATH[] = "\\EFI\\Microsoft\\Boot\\BCD";
        const char BIOS_BCD_PATH[] = "\\Boot\\BCD";
    }

    namespace RegistryConstants
    {
        const char VM_SYSTEM_HIVE_NAME[] = "Azure_Recovery_Source_System";
        const char VM_SOFTWARE_HIVE_NAME[] = "Azure_Recovery_Source_Software";

        const char CONTROL_SET_PREFIX[] = "ControlSet";
        const char SERVICES[] = "\\Services\\";
        const char SERVICE_START_VALUE_NAME[] = "Start";
        const char SYSTEM_SELECT_KEY[] = "Select";
        const char TEMP_VM_SYSTEM_HIVE_NAME[] = "SYSTEM";

        const char PARTMGR_PARAMS_KEY[] = "\\Services\\partmgr\\Parameters";
        const char MOUNTMGT_PARAMS_KEY[] = "\\Services\\mountmgr";
        const char INMAGE_SV_SYSTEMS_X64[] = "\\Wow6432Node\\SV Systems";
        const char INMAGE_SV_SYSTEMS_X32[] = "\\SV Systems";
        const char INMAGE_INSTALL_PATH_X64[] = "\\Wow6432Node\\SV Systems\\VxAgent";
        const char INMAGE_INSTALL_PATH_X32[] = "\\SV Systems\\VxAgent";
        const char VALUE_INMAGE_INSTALL_DIR[] = "InstallDirectory";
        const char INVOLFLT_PARAM_KEY[] = "\\Services\\InDskFlt\\Parameters";//"\\Services\\involflt\\Parameters";
        const char WINDOWS_AZURE_GA_KEY[] = "\\Services\\WindowsAzureGuestAgent";
        const char RDAGENT_KEY[] = "\\Services\\RdAgent";
        const char RDP_ALLOW_KEY[] = "\\Control\\Terminal Server";
        const char RDP_DISABLE_NLA_KEY[] = "\\Control\\Terminal Server\\WinStations\\RDP-Tcp";
        const char WINGA_PACKAGES_DIR_PATH[] = "C:\\WindowsAzure\\Packages";
        const char VALUE_DATAPOOLSIZE[] = "DataPoolSize";
        const char VALUE_VOL_DATASIZE_LIMIT[] = "VolumeDataSizeLimit";
        const char VALUE_TSCONNECTION[] = "fDenyTSConnections";
        const char VALUE_SECLAYER[] = "SecurityLayer";
        const char VALUE_USERAUTH[] = "UserAuthentication";

        const char WIN_CURRENT_VERSION_KEY[] = "\\Microsoft\\Windows NT\\CurrentVersion";
        const char VALUE_CURRENT_VERSION[] = "CurrentVersion";

        const char BOOTUP_SCRIPT_KEY2[] = "\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\State\\Machine\\Scripts\\Startup\\";
        const char BOOTUP_SCRIPT_KEY1_2K8BELOW[] = "\\Policies\\Microsoft\\Windows\\System\\Scripts\\Startup\\";
        const char BOOTUP_SCRIPT_KEY1_2K8ABOVE[] = "\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Scripts\\Startup\\";
        const char VXAGENT_REG_KEY[] = "\\VxAgent";

        const char VALUE_NAME_GPO_ID[] = "GPO-ID";
        const char VALUE_NAME_SOM_ID[] = "SOM-ID";
        const char VALUE_NAME_FILESYSPATH[] = "FileSysPath";
        const char VALUE_NAME_DISAPLAYNAME[] = "DisplayName";
        const char VALUE_NAME_GPONAME[] = "GPOName";
        const char VALUE_NAME_SCRIPT_ORDER[] = "PSScriptOrder";
        const char VALUE_NAME_SCRIPT[] = "Script";
        const char VALUE_NAME_SCRIPT_PARAMS[] = "Parameters";
        const char VALUE_NAME_IS_POWERSHELL[] = "IsPowershell";
        const char VALUE_NAME_HOST_ID[] = "HostId";
        const char VALUE_NAME_SAN_POLICY[] = "SanPolicy";
        const char VALUE_NAME_SAN_POLICY_BACKUP[] = "Backup.SanPolicy";
        const char VALUE_NAME_CURRENT[] = "Current";
        const char VALUE_NAME_LASTKNOWNGOOD[] = "LastKnownGood";
        const char VALUE_NAME_BOOTUP_SCRIPT_ORDER_NUMBER[] = "BootupScriptNumber";
        const char VALUE_NAME_NOAUTOMOUNT[] = "NoAutoMount";
        const char VALUE_IMAGE_PATH[] = "ImagePath";

        const char VALUE_NAME_PROD_VERSION[] = "PROD_VERSION";
        const char VALUE_NAME_RECOVERY_INPROGRESS[] = "RecoveryInprogress";
        const char VALUE_NAME_NEW_HOSTID[] = "NewHostId";
        const char VALUE_NAME_TEST_FAILOVER[] = "TestFailover";
        const char VALUE_NAME_ENABLE_RDP[] = "EnableRDP";
        const char VALUE_NAME_RECOVERED_ENV[] = "CloudEnv";

        const char VALUE_DATA_RECOVERED_ENV_AZURE[] = "Azure";

        const char VXAGENT_V2_GA_PROD_VERSION[] = "9.0.0.0";

        const int MAX_STARTUP_SCRIPT_SEARCH_LIMIT = 1000;

        const char WIN2K8_VERSION_PREFIX[] = "6.0.";
    }

    namespace ServiceNames
    {
        const char INMAGE_SVAGENT[] = "svagents";
        const char INMAGE_FXAGENT[] = "frsvc";
        const char INMAGE_APPAGENT[] = "InMage Scout Application Service";

        const char VMWARE_TOOLS[] = "VMTools";

        const char DHCP_SERVICE[] = "Dhcp";

        //Services required to disable for RDP enable
        const char POLICY_AGENT[] = "PolicyAgent";

        const char AZURE_GUEST_AGENT[] = "WindowsAzureGuestAgent";
        const char AZURE_TELEMETRY_SERVICE[] = "WindowsAzureTelemetryService";
        const char AZURE_RDAGENT_SVC[] = "RdAgent";
    }

    namespace SysConstants
    {
        const char DEFAULT_SYSTEM32_DIR[] = "\\Windows\\system32";
        const char CONFIG_SYSTEM_HIVE_PATH[] = "\\config\\system";
        const char CONFIG_SOFTEARE_HIVE_PAHT[] = "\\config\\software";
        const char SYSTEM_HIVE_FULL_PATH[] = "\\Windows\\System32\\config\\system";
        const char DOTNET_FRAMEWORK_PATH[] = "\\Windows\\Microsoft.NET\\Framework";
        const char WINDOWS_AZURE_DIR[] = "\\WindowsAzure";
        const char WIN_GA_DOTNET_VER[] = "4.0";
    }
}

#endif //~AZURE_WIN_CONSTANTS_H
