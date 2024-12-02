### Hyper-V

If you are using Hyper-V to Azure Site Recovery scenario, you will need to re-install Azure Site Recovery provider version 5.1.8116.0 which is the correct and latest version. To re-install this version, follow the steps below:
1. Uninstall the incorrect provider version (5.24.1025.12). To do this on Hyper-V host server, navigate to Control Panel -> Programs -> Programs and Features ; uninstall "Microsoft Azure Dra Service"
2. Download the correct provider version. To do this on Hyper-V host, go to Azure portal->Recovery services vault->Site Recovery Infrastructure-> Hyper-V Hosts-> "+Add Server". From step # 3 listed here, download the installer and run the downloaded installer file to complete the installation. Registration of host in the vault is not needed here.
3. You can verify the Azure Site Recovery provider version by going to Recovery services vault->Site Recovery Infrastructure-> Hyper-V Hosts. If your Software version for Hyper-V host server is "5.1.8116.0 (latest)," then you have installed the correct version.

### VmWare to Azure (CS/ Classic)

If you are using Classic VMWare to Azure scenario, follow the below steps:
1. Uninstall the incorrect provider version (5.24.1025.12). To do this on configuration server, navigate to Control Panel -> Programs -> Programs and Features ; uninstall "Microsoft Azure Dra Service"
2. Get the configuration server version you are using by going to Azure portal - Recovery Services Vault->Manage->Site Recovery Infrastructure->For VMWare & Physical Machines->Configuration Servers (Classic). Select the server name and its corresponding Software version.
3. Download the MicrosoftAzureSiteRecoveryUnifiedSetup.exe based on configuration server version you are using:
4. a) If you are using 9.63.*,  [download Unified Setup (version 9.63.7233.1)](https://download.microsoft.com/download/6efee2af-c8b9-47de-8b31-9fef7ad75bfb/MicrosoftAzureSiteRecoveryUnifiedSetup.exe)
   
   b) If you are using 9.61.*, [download Unified Setup (version 9.61.7016.1)](https://download.microsoft.com/download/1/6/b/16bb9652-d549-4df7-80e2-e56ab0472cdd/MicrosoftAzureSiteRecoveryUnifiedSetup.exe)
   
   c) If you are using 9.59.*, [download Unified Setup (version 9.59.6930.1)](https://download.microsoft.com/download/2/b/8/2b831be7-c30b-4860-8733-eaafa63e0ba2/MicrosoftAzureSiteRecoveryUnifiedSetup.exe)
6. Extract the exe using following command: MicrosoftAzureSiteRecoveryUnifiedSetup.exe /q /x:\<folder\>
7. Go inside the folder where extraction has happened and run Install the azuresiterecoveryprovider.exe from there to install the provider version.
Navigate to Control Panel -> Programs -> Programs and Features ; verify the program "Microsoft Azure Recovery Services Provider" is installed.
 
### VmWare to Azure (RCM/ Modernized)

If you are using Modernized VMWare to Azure Site Recovery, follow the steps below:
1. Get install path of Azure Site Recovery provider. To do this on appliance, open Registry Editor, go to registry HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Azure Site Recovery, get Data value for key name “InstallPath”
2. Uninstall the incorrect provider version (5.24.1025.12). To do this on appliance, navigate to Control Panel -> Programs -> Programs and Features ; uninstall "Microsoft Azure Dra Service"
3. Download Site Recovery provider
   
   a) Get the process server version you are using on your appliance by going to Recovery Services Vault->Manage-> Site Recovery infrastructure-> For VMWare & physical machines> ASR Replication appliance. Select the Appliance name and get the version for “Process server”.
   
   b) Check rollup update pages [here](https://learn.microsoft.com/en-us/azure/site-recovery/site-recovery-whats-new#supported-updates). Open each rollup page, navigate to “Microsoft Azure Site Recovery replication appliance” section for each roll up page. In Component if “Process server” version matches with process server version derived in step 3.a, download the corresponding Site Recovery Provider installer. Also note the Site Recovery Provider version you are downloading.
6. Open command prompt in admin mode.
7. Run following command: msiexec /i ASRAdapter.msi  INSTALLDIR="\<InstallPath\>" ADDLOCAL=InMageRcmAdapter (Here ASRAdapter.msi is installer downloaded in step 3.a and \<installPath\> is derived in step 1)
8. Go to Recovery Services Vault->Manage-> Site Recovery infrastructure-> For VMWare & physical machines> ASR Replication appliance. Select the Appliance name and get the version for “Site Recovery Provider”. This version should match with Site Recovery Provider version you have noted in step 3.b.
