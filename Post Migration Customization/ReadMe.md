# Azure VM Post-Migration Customization Script

## Overview
This script automates post-migration tasks for Azure Virtual Machines (VMs). It provides functionality to delete VMs, optionally delete associated Network Interfaces (NICs), move managed disks to a specified resource group (and optionally to another subscription), and generate Infrastructure-as-Code (IaC) snippets for Terraform to reattach disks to new VMs.

## Prerequisites
To run this script, ensure the following requirements are met:
1. **Az PowerShell Module**: Installed and authenticated.
2. **Permissions**: Appropriate permissions to delete VMs, NICs, and move resources between resource groups/subscriptions.
3. **Configuration File**: A valid `config.json` file with the required parameters.

## Configuration File (`config.json`)
The script relies on the `config.json` file to define the operations and resources to be processed. Below is an explanation of each configuration parameter:

### Parameters
- **`vmResourceIds`**:
  - An array of Azure VM Resource IDs to process.
  - Example: `["/subscriptions/<subscription-id>/resourceGroups/<resource-group>/providers/Microsoft.Compute/virtualMachines/<vm-name>"]`
  - **Impact**: Specifies the VMs to be processed. If incorrect IDs are provided, the script will fail.

- **`deleteNics`**:
  - Specifies whether to delete NICs associated with the VMs. Accepts `"true"` or `"false"`.
  - **Impact**: If set to `"true"`, NICs associated with the VMs will be deleted. If `"false"`, NICs will remain intact. If deleteVMs is false, NICs will remain intact even if this setting is marked as true.

- **`deleteVMs`**:
  - Specifies whether to delete the VMs. Accepts `"true"` or `"false"`.
  - **Impact**: If `"true"`, the VMs will be deleted. If `"false"`, the VMs will remain intact. The OS disk and Data disks will be copied to chosen resource groups. Original OS disk and Data disks will not be deleted.

- **`targetDiskRG`**:
  - The target Resource Group to move the VM disks into.
  - Example: `"CPC-akgu-M85R0H-vmw-rg"`
  - **Impact**: If provided, disks will be moved to this resource group. If left blank, disks remain in their original resource group.

- **`targetDiskSub`**:
  - The target Subscription ID to move the VM disks into.
  - Example: `"4Bd2aa0f-2bd2-4d67-91a8-5a4533d58600"`
  - **Impact**: If provided, disks will be moved to this subscription. If left blank, disks remain in their original subscription.

- **`persistOSDisk`**:
  - Specifies whether to persist the OS disk. Accepts `"true"` or `"false"`.
  - **Impact**: If `"true"`, the OS disk will be retained. If `"false"`, the OS disk will be deleted. In case of deleteVMs is false, OS disk is also not deleted irrespective of the value added here.

- **`confirmBeforeProceeding`**:
  - Specifies whether to prompt for confirmation before proceeding. Accepts `"true"` or `"false"`.
  - **Impact**: If `"true"`, the script will prompt for confirmation before executing operations. If `"false"`, the script will proceed without confirmation.

- **`enableTelemetry`**:
  - Specifies whether to enable telemetry. Accepts `"true"` or `"false"`.
  - **Impact**: If `"true"`, telemetry events will be sent. If `"false"`, telemetry will be disabled.

- **`generateIaasVmIacTemplateDiskConfig`**:
  - Specifies whether to generate a disk configuration JSON file for IaaS VM IAC templates. Accepts `"true"` or `"false"`.
  - **Impact**: If `"true"`, generates a `disk-config.json` file in the `VM_Disk_IAC` directory containing disk configuration details that can be consumed by IAC templates generated using Azure Migrate Generate IaC flow for IaaS VMs Lift and Shift strategy. If `"false"`, only Terraform `.tf` files are generated.
  - **Output**: Creates a structured JSON file with disk properties including VM name, disk name, size, storage type, caching settings, LUN numbers, and metadata tags for each disk processed.

## How to Use
1. **Prepare the Configuration File**:
   - Create or update the `config.json` file with the required parameters.
   - Ensure the file is located in the same directory as the script.

2. **Run the Script**:
   - Open a PowerShell terminal.
   - Navigate to the directory containing the script.
   - Authenticate with Azure using the following command:
     ```powershell
     az login
     ```
   - Execute the script using the following command:
     ```powershell
     ./DiskMigration.ps1
     ```

3. **Follow Prompts**:
   - If `confirmBeforeProceeding` is set to `"true"`, review the resources and confirm the operations.

4. **Check Outputs**:
   - IaC snippets will be generated in the `VM_Disk_IAC` directory.
   - If `generateIaasVmIacTemplateDiskConfig` is enabled, a `disk-config.json` file will be created in the `VM_Disk_IAC` directory.
   - Errors will be logged in the `error_log.txt` file.
   - Telemetry logs will be saved in the `logfile.txt` file.

## How to Get Resource ID of a Migrated VM
If you have migrated a VM using Azure Server Migration, you can retrieve its Resource ID by following these steps:

1. **Log in to Azure Portal**:
   - Open [Azure Portal](https://portal.azure.com).

2. **Navigate to the Resource Group**:
   - Locate the Resource Group where the migrated VM resides.

3. **Find the Virtual Machine**:
   - In the Resource Group, look for the Virtual Machine resource.

4. **Copy the Resource ID**:
   - Click on the Virtual Machine.
   - In the Overview section, locate the `Resource ID` field.
   - Copy the Resource ID for use in the `config.json` file.

Alternatively, you can use Azure CLI or PowerShell:

### Using Azure CLI
Run the following command:
```bash
az vm show --resource-group <resource-group-name> --name <vm-name> --query id --output tsv
```
Replace `<resource-group-name>` and `<vm-name>` with the appropriate values.

### Using PowerShell
Run the following command:
```powershell
(Get-AzVM -ResourceGroupName <resource-group-name> -Name <vm-name>).Id
```
Replace `<resource-group-name>` and `<vm-name>` with the appropriate values.

## Impact on Resources

### Scenario 1: Deleting VMs and NICs
- **VMs**: Deleted as per the configuration (`deleteVMs` set to `"true"`).
- **NICs**: Deleted if `deleteNics` is set to `"true"`. Retained if `deleteVMs` is `"false"`, regardless of `deleteNics` value.
- **Disks**: Moved to the specified resource group and/or subscription.
- **IaC Snippets**: Generated for reattaching disks to new VMs.

### Scenario 2: Retaining VMs and NICs
- **VMs**: Retained as per the configuration (`deleteVMs` set to `"false"`).
- **NICs**: Retained if `deleteNics` is set to `"false"`. Retained even if `deleteNics` is `"true"` when `deleteVMs` is `"false"`.
- **Disks**: Copied to the specified resource group and/or subscription. Original disks are not deleted.
- **IaC Snippets**: Generated for reattaching disks to new VMs.

### Scenario 3: Persisting OS Disk
- **OS Disk**: Retained if `persistOSDisk` is set to `"true"`. Always retained when VMs are retained (`deleteVMs` set to `"false"`).
- **Data Disks**: Moved or copied to the specified resource group and/or subscription.
- **IaC Snippets**: Generated for reattaching disks to new VMs.

### Scenario 4: Moving Disks Across Subscriptions
- **Disks**: Moved to the specified resource group in the target subscription when VMs are deleted (`deleteVMs` set to `"true"`). Copied to the specified resource group in the target subscription when VMs are retained (`deleteVMs` set to `"false"`).
- **IaC Snippets**: Generated for reattaching disks to new VMs.

## Notes
- Ensure you have appropriate permissions for all operations defined in the configuration file.
- Review the configuration file carefully to avoid unintended resource modifications.
- The script logs telemetry events for tracking purposes. Replace the placeholder telemetry integration with your own if needed.
- Confirm all settings in the `config.json` file before running the script to ensure desired outcomes.

## Example Configuration
```json
{
    "vmResourceIds": [
        "/subscriptions/4bd2aa0f-2bd2-4d67-91a8-5a4533d58600/resourceGroups/testrg/providers/Microsoft.Compute/virtualMachines/vm17"
    ],
    "deleteNics": "false",
    "deleteVMs": "false",
    "targetDiskRG": "targetRG",
    "targetDiskSub": "c6570921-c122-407e-ae0c-d496d7aa05f8",
    "persistOSDisk": "true",
    "generateIaasVmIacTemplateDiskConfig": "true",
    "confirmBeforeProceeding": "true",
    "enableTelemetry": "true"
}
```

## Unsupported Scenario
- **Moving Ultra Disks across resource groups**: The script does not support moving ultra disks across resource groups as Azure does not allow creating ultra disks from snapshots. It can be moved via Azcopy or disk to disk copy inside a VM.
