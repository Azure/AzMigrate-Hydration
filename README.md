# Announcement

The Azure Migrate VMware Agentless Migration service (https://learn.microsoft.com/en-us/azure/migrate/vmware/tutorial-migrate-vmware) released support in 2023 for a simplified appliance infrastructure that no longer depends on **KeyVault, ServiceBus,** and **EventHub** artifacts. This newer infrastructure provides a streamlined experience for customer onboarding and enhanced security as we continue to invest in modernizing our platform.

## Important Timelines for Customers Using the Legacy-Infrastructure based Appliance

- **By May 15, 2025:** Deprecation communication would be shared with impacted customers. 
- **Before July 15, 2025:** Complete/Disable existing replications. Re-initialize legacy-infrastructure appliances to SBM-infrastructure appliances.
- **Post July 15, 2025:** Enable-Replication and Migration operations from appliances that haven’t been reinitialized will no longer be supported. 

## Steps to identify Legacy-Infrastructure Appliance

In order to identify if an appliance is using the legacy infrastructure, login to the appliance, and confirm that the gateway.json file located at 'C:\ProgramData\Microsoft Azure\Config' on the appliance does not contain the following key:

   "UseStorageMessaging": "True" 

## Migration Steps for Customers Using the Legacy-Infrastructure based Appliance

- **Complete/Disable existing replications:**  
  Complete the migration of any already enabled virtual machines or disable the existing replications.
  
- **Re-initialize Replication Appliance:**  
  You can either retire the older appliance and register a new one in your migration projects for new servers or use the provided script to clean up and reinitialize your current appliances for the new infrastructure.
  
- **Stop using legacy appliances for new VMs:**  
  In case you have legacy appliances lying around, do not enable migrations from appliances that haven’t been reinitialized. Instead use the modernized appliances that have been newly set up from scratch or re-initialized using the script to enable new migrations.

If you have any questions or need assistance, please reach out to Microsoft Support at your earliest convenience.

# Execution Details

## Note

1. Ensure that there are no ongoing replications going through the appliance, and all replicating VMs have either been migrated or disabled before running the script.
2. In case multiple appliances are registered to migrate VMs within the same migrate project, please modify the script to ensure that in **Section 2** mentioned below, the script deletes the resources for the container mapping objects which have valid KeyVault resource mentioned in `value.properties.providerspecificdetails`.
3. Please note that if you reinitialize the infrastructure on the existing appliance, there will be no need to repeat the discovery and assessment process.

The script is designed to be run on any machine—including the appliance VM. When you execute it, you’ll be prompted to log in to Azure with the required credentials. Additionally, you’ll need to update information such as your tenant, subscription, Migrate Project Name, and Resource Group Name before the execution proceeds. Please review the script and replace required values.

The script is organized into the following sections:

1. **Pre-Requisites:**  
   - Updates the execution policy on your machine.  
   - Installs the required PowerShell packages, Chocolatey, and armclient.

2. **Infrastructure Cleanup:**  
   - Retrieves infrastructure details from Azure using armclient.  
   - Deletes any migration-related infrastructure and artifacts that are no longer needed.

3. **Appliance VM Configuration Cleanup:**  
   - Resets the configuration on the appliance VM (this section must be run on the appliance VM).

4. **Re-initialize the Appliance Infrastructure (Optional):**  
   - Initializes the Azure replication policy and associates it with the recovery services vault.

Running section 4 from PowerShell is optional. After successfully running the script up to Section 3, enabling any new replication from the portal will allow you to specify the desired location, Resource Group, and other necessary details to automatically set up the new migration infrastructure.
