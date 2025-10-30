<#
.SYNOPSIS
    Azure VM Post-Migration Customization Script.

.DESCRIPTION
    This script automates the post-migration customization tasks for Azure Virtual Machines. It deletes specified VMs, optionally deletes associated Network Interfaces (NICs), moves managed disks to a specified resource group (and optionally to another subscription), and generates Infrastructure-as-Code (IaC) snippets for Terraform to reattach disks to new VMs.

.PARAMETER vmResourceIds
    An array of Azure VM Resource IDs to process. The script retrieves these IDs from the configuration file.

.PARAMETER deleteNics
    Specifies whether to delete NICs associated with the VMs. Accepts 'true' or 'false'.

.PARAMETER targetDiskRG
    (Optional) The target Resource Group to move the VM disks into. If left blank, disks remain in their original Resource Group.

.PARAMETER targetDiskSub
    (Optional) The target Subscription ID to move the VM disks into. If left blank, disks remain in their original subscription.

.PARAMETER persistOSDisk
    Specifies whether to persist the OS disk during VM deletion. Accepts 'true' or 'false'.

.PARAMETER confirmBeforeProceeding
    Specifies whether to prompt for confirmation before proceeding with operations. Accepts 'true' or 'false'.

.PARAMETER deleteVMs
    Specifies whether to delete the VMs after processing their disks. Accepts 'true' or 'false'.

.OUTPUTS
    Generates Terraform (.tf) files containing IaC snippets for managed disks in the specified output directory.

    Logs errors encountered during execution to an error log file.

.EXAMPLE
    ./DiskMigration.ps1
    Processes VMs based on the configuration file and performs the specified operations.

.NOTES
    Requirements:
        - Az PowerShell Module installed and authenticated.
        - Appropriate permissions to delete VMs, NICs, and move resources between resource groups/subscriptions.

    Telemetry integration is included for custom event tracking.

#>

# Function to log telemetry events to the log file
function Write-Telemetry {
    param(
        [string]$EventName,
        [hashtable]$Properties
    )
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "$timestamp - Event: $EventName - Properties: $($Properties | ConvertTo-Json -Compress)"
    Add-Content -Path $logFile -Value $logEntry
}

# Use this function to send telemetry events
# This is a placeholder function. Replace with actual telemetry integration as needed.
# Currently we are logging to a file using this function.
function Send-Telemetry {
    param($EventName, $Properties)
    # Placeholder for telemetry integration
    Write-Host "Telemetry Event: $EventName - $($Properties | ConvertTo-Json -Compress)"
    Write-Telemetry -EventName $EventName -Properties $Properties
}

function Send-AzMigrateTelemetry{
    param(
        [string]$EventName,
        [string]$ScriptRunId,
        [hashtable]$Properties
    )
    if (-not $enableTelemetry) {
        Write-Host "Telemetry is disabled in configuration. Skipping telemetry event: $EventName"
        return
    }

    $scriptPath = $PSScriptRoot
    $telemetryScript = Join-Path -Path $scriptPath -ChildPath "telemetryai.ps1"

    if (Test-Path $telemetryScript) {
        # Call the telemetry script with ResourceID, EventName and Properties
        & $telemetryScript -resource_id $ScriptRunId -event $EventName -properties $($Properties | ConvertTo-Json -Compress)
    } else {
        Write-Warning "Telemetry script not found at $telemetryScript. Skipping telemetry."
    }
}

# Function to generate the data block for a managed disk
function GetDataBlock {
    param (
        [string]$diskName,
        [string]$diskRG,
        [bool] $isOsDisk = $false
    )

    if ($isOsDisk) {
        return @"
# OS disk. Uncomment if you are creating VM using this disk.
# data "azurerm_managed_disk" "$diskName" {
#   name                = "$diskName"
#   resource_group_name = "$diskRG"
# }

"@
    } else {
        return @"
data "azurerm_managed_disk" "$diskName" {
  name                = "$diskName"
  resource_group_name = "$diskRG"
}

"@
    }
}

# Function to generate the attachment block for a managed disk
function GetAttachBlock {
    param (
        [string]$diskName,
        [int]$lun,
        [bool] $isOsDisk = $false
    )

    if ($isOsDisk) {
        return @"
# OS disk attachment. Uncomment if you are creating VM using this disk.
# resource "azurerm_virtual_machine_data_disk_attachment" "$diskName" {
#   managed_disk_id    = data.azurerm_managed_disk.$diskName.id
#   virtual_machine_id = "" # TODO: set VM resource ID
#   lun                = 0
#   caching            = "ReadWrite"
# }

"@
    } else {
        return @"
resource "azurerm_virtual_machine_data_disk_attachment" "$diskName" {
  managed_disk_id    = data.azurerm_managed_disk.$diskName.id
  virtual_machine_id = "" # TODO: set VM resource ID
  lun                = $lun
  caching            = "ReadWrite"
}

"@
    }
}

# Function to load and validate configuration
function Get-Configuration {
    param([string]$configFilePath)
    if (-Not (Test-Path $configFilePath)) {
        Write-Error "Configuration file not found at $configFilePath. Exiting."
        exit
    }
    $config = Get-Content -Path $configFilePath | ConvertFrom-Json
    if (-Not $config.vmResourceIds -or $config.vmResourceIds.Count -eq 0) {
        Write-Error "No VM Resource IDs provided in the configuration file. Exiting."
        exit
    }
    return $config
}

# Function to gather NICs for review
function Get-NICs {
    param([array]$vmResourceIds, [string]$deleteNics)
    $vmNicInfo = @{}
    if ($deleteNics -eq $true) {
        foreach ($vmId in $vmResourceIds) {
            try {
                $vm = Get-AzResource -ResourceId $vmId -ErrorAction Stop
                $vmName = $vm.Name
                $vmRG = $vm.ResourceGroupName
                $vmDetails = Get-AzVM -ResourceGroupName $vmRG -Name $vmName -ErrorAction Stop

                $nicIds = $vmDetails.NetworkProfile.NetworkInterfaces | Select-Object -ExpandProperty Id
                $nicObjs = @()
                foreach ($nicId in $nicIds) {
                    $nic = Get-AzNetworkInterface -ResourceId $nicId -ErrorAction SilentlyContinue
                    if ($nic) {
                        $nicObjs += $nic
                    }
                }
                $vmNicInfo[$vmId] = @{
                    VMName = $vmName
                    NICs   = $nicObjs
                }
            } catch {
                Write-Host "Error retrieving NICs for VM $vmId : $_"
            }
        }
    }
    return $vmNicInfo
}

# Function to wait for active deployments in a resource group
function Wait-For-ActiveDeployments {
    param(
        [string]$resourceGroupName,
        [string[]]$deploymentTypes, # List of deployment types to check
        [int]$timeoutMinutes = 30
    )

    $startTime = Get-Date
    $timeoutTime = $startTime.AddMinutes($timeoutMinutes)

    Write-Host "Checking for active deployments of types '$($deploymentTypes -join ", ")' in Resource Group: $resourceGroupName"

    while ($true) {
        # Get all deployments in the resource group
        $deployments = Get-AzResourceGroupDeployment -ResourceGroupName $resourceGroupName -ErrorAction SilentlyContinue

        if ($deployments) {
            # Filter for active (running) deployments matching any of the specified types
            $activeDeployments = $deployments | Where-Object { 
                $_.ProvisioningState -eq "Running" -and 
                ($deploymentTypes | ForEach-Object { $_ -and ($_.Name -like "*$_*") }) -contains $true
            }

            if ($activeDeployments.Count -gt 0) {
                Write-Host "Active deployments found in Resource Group: $resourceGroupName"
                $activeDeployments | Select-Object DeploymentName, ProvisioningState
                Write-Host "Waiting for active deployments to complete..."
            } else {
                Write-Host "No active deployments of types '$($deploymentTypes -join ", ")' found in Resource Group: $resourceGroupName."
                return
            }
        } else {
            Write-Host "No deployments found in Resource Group: $resourceGroupName."
            return
        }

        # Check for timeout
        if ((Get-Date) -ge $timeoutTime) {
            Write-Host "Timeout reached while waiting for active deployments in Resource Group: $resourceGroupName."
            Send-Telemetry -EventName "DeploymentTimeout" -Properties @{ResourceGroup=$resourceGroupName; DeploymentTypes=$deploymentTypes}
            return
        }

        # Wait for 1 minute before checking again
        Start-Sleep -Seconds 60
    }
}

# Function to shutdown VM, copy disks to target resource group and generate IaC snippets for disks moved.
# This function is used when deleteVMs is false.
function ShutdownVMMoveDisksGenerateIAC {
    param(
        [object]$vmDetails,
        [string]$vmName,
        [string]$vmId,
        [string]$vmRG,
        [string]$targetDiskRG,
        [string]$targetDiskSub,
        [string]$persistOSDisk,
        [string]$sourceSubscriptionId
    )
    # Get VM disks
    $vmDetails = Get-AzVM -ResourceGroupName $vmRG -Name $vmName -ErrorAction Stop
    $osDisk = $vmDetails.StorageProfile.OsDisk.ManagedDisk.Id
    $dataDisks = $vmDetails.StorageProfile.DataDisks | Select-Object -ExpandProperty ManagedDisk | Select-Object -ExpandProperty Id

    # Shutdown VM
    Write-Host "Shutting down VM: $vmName ($vmId)"
    $stopVM = Stop-AzVM -ResourceGroupName $vmRG -Name $vmName -Force -ErrorAction Stop
    Send-Telemetry -EventName "VMShutdown" -Properties @{VM = $vmName}

    # Copy all disks to target resource group.
    $allDisks = @($osDisk) + $dataDisks
    if ($allDisks.Count -eq 0) {
        Write-Host "No disks found for VM: $vmName"
        continue
    }

    $resourceLocation = (Get-AzResourceGroup -Name $vmRG).Location
    Write-Host "Resource Group Location: $resourceLocation"

    $iacSnippet = @()
    # Create new disk from existing disks if the target resource group is in same subscription as the VM.
    if (!$targetDiskSub -or $targetDiskSub -eq $sourceSubscriptionId) {
        foreach ($diskId in $allDisks) {
            if($diskId -eq $osDisk -and $persistOSDisk -eq $false)
            {
                Write-Host "Skipping OS disk: $diskId as it is not persisted."
                Send-Telemetry -EventName "OSDiskSkipped" -Properties @{
                    Disk = $diskId
                    Reason = "Not persisted"
                }
                continue
            }

            $diskName = (Get-AzResource -ResourceId $diskId).Name
            $newDiskName = "$diskName-new-$(Get-Date -Format 'yyyyMMddHHmmss')"

            $disk = Get-AzResource -ResourceId $diskId

            Write-Host "Creating new managed disk from existing disk: $diskId as $newDiskName in Resource Group: $targetDiskRG"

            $newDiskConfig = New-AzDiskConfig -Location $disk.Location `
                -CreateOption Copy -SourceResourceId $diskId

            $newDisk = New-AzDisk -DiskName $newDiskName -Disk $newDiskConfig -ResourceGroupName $targetDiskRG -ErrorAction Stop
            Send-Telemetry -EventName "DiskCreated" -Properties @{
                OriginalDisk = $diskId
                NewDisk = $newDiskName
                TargetRG = $targetDiskRG
            }

            $isOsDisk = if ($diskId -eq $osDisk) { $true } else { $false }
            $iacSnippet += GetDataBlock -diskName $newDiskName -diskRG $targetDiskRG -isOsDisk $isOsDisk
            $iacSnippet += GetAttachBlock -diskName $newDiskName -lun $disk.Lun -isOsDisk $isOsDisk
        }
    }
    else
    {
        Write-Host "Target subscription is different from VM subscription. Copying disks to target resource group in target subscription."
        foreach ($diskId in $allDisks) {
            if($diskId -eq $osDisk -and $persistOSDisk -eq $false)
            {
                Write-Host "Skipping OS disk: $diskId as it is not persisted."
                Send-Telemetry -EventName "OSDiskSkipped" -Properties @{
                    Disk = $diskId
                    Reason = "Not persisted"
                }
                continue
            }

            # Directly copy disk to target resource group and subscription
            $disk = Get-Azdisk -ResourceGroupName $vmRG -DiskName (Get-AzResource -ResourceId $diskId).Name -ErrorAction Stop
            
            Set-AzContext -SubscriptionId $targetDiskSub -ErrorAction Stop
            Write-Host "Switched to Subscription: $targetDiskSub"

            $diskConfig = New-AzDiskConfig -Location $disk.Location `
                -CreateOption Copy -SourceResourceId $disk.Id -SkuName $disk.Sku.Name
            $newDisk = New-AzDisk -DiskName $disk.Name -Disk $diskConfig -ResourceGroupName $targetDiskRG -ErrorAction Stop
            Send-Telemetry -EventName "DiskCopied" -Properties @{
                Disk = $diskId
                TargetRG = $targetDiskRG
                TargetSub = $targetDiskSub
            } 


            # Switch back to the original subscription
            Set-AzContext -SubscriptionId $sourceSubscriptionId -ErrorAction Stop
            Write-Host "Switched back to Subscription: $sourceSubscriptionId"

            $isOsDisk = if ($diskId -eq $osDisk) { $true } else { $false }
            $iacSnippet += GetDataBlock -diskName $diskName -diskRG $targetDiskRG -isOsDisk $isOsDisk
            $iacSnippet += GetAttachBlock -diskName $diskName -lun $disk.Lun -isOsDisk $isOsDisk
        }
    }

    return $iacSnippet
}

# Function to delete VM, move disks to target resource group and generate IaC snippets for disks moved.
# This function is used when deleteVMs is true.
function DeleteVMMoveDisksGenerateIAC {
    param(
        [object]$vmDetails,
        [string]$vmName,
        [string]$vmId,
        [string]$vmRG,
        [string]$targetDiskRG,
        [string]$targetDiskSub,
        [string]$persistOSDisk,
        [string]$sourceSubscriptionId
    )

    # Get VM disks
    $vmDetails = Get-AzVM -ResourceGroupName $vmRG -Name $vmName -ErrorAction Stop
    $osDisk = $vmDetails.StorageProfile.OsDisk.ManagedDisk.Id
    $dataDisks = $vmDetails.StorageProfile.DataDisks | Select-Object -ExpandProperty ManagedDisk | Select-Object -ExpandProperty Id

    # Test: Confirm deletion of VMs. To be removed.
    if ($confirmBeforeProceeding -eq "yes") {
        $confirm = Read-Host "`nProceed with the deletion of VM? (yes/no)"
        if ($confirm -ne "yes") {
            Write-Host "Review declined. Exiting."
            exit
        }
    }

    # Detach disks and delete VM
    Write-Host "Deleting VM: $vmName ($vmId)"
    Remove-AzVM -ResourceGroupName $vmRG -Name $vmName -Force -ErrorAction Stop
    Send-Telemetry -EventName "VMDeleted" -Properties @{VM = $vmName}

    if($persistOSDisk -eq $false) {
        # Delete OS disk if not persisting
        Write-Host "Deleting OS Disk: $osDisk"
        Remove-AzDisk -ResourceId $osDisk -Force -ErrorAction Stop
        Send-Telemetry -EventName "OSDiskDeleted" -Properties @{Disk = $osDisk; VM = $vmName}
    } else {
        Write-Host "Persisting OS Disk: $osDisk"
        Send-Telemetry -EventName "OSDiskPersisted" -Properties @{Disk = $osDisk; VM = $vmName}
    }

    # Move disks if target RG provided
    if ($persistOSDisk) {
        $allDisks = @($osDisk) + $dataDisks
    } else {
        $allDisks = $dataDisks
    }

    if ($allDisks.Count -eq 0) {
        Write-Host "No disks found for VM: $vmName"
        continue
    }

    if ($targetDiskRG) {
        foreach ($diskId in $allDisks) {
            if ($targetDiskSub) {
                # Move disk to another subscription and resource group
                Write-Host "Moving disk $diskId to Resource Group: $targetDiskRG in Subscription: $targetDiskSub"
                Move-AzResource -ResourceId $diskId `
                    -DestinationResourceGroupName $targetDiskRG `
                    -DestinationSubscriptionId $targetDiskSub `
                    -Force -ErrorAction Stop
                Send-Telemetry -EventName "DiskMoved" -Properties @{Disk = $diskId; TargetRG = $targetDiskRG; TargetSub = $targetDiskSub}
            } else {
                # Move disk within same subscription
                Write-Host "Moving disk $diskId to Resource Group: $targetDiskRG"
                Move-AzResource -ResourceId $diskId `
                    -DestinationResourceGroupName $targetDiskRG `
                    -Force -ErrorAction Stop
                Send-Telemetry -EventName "DiskMoved" -Properties @{Disk = $diskId; TargetRG = $targetDiskRG}
            }
        }
    }

    # Generate IAC snippet for each disk
    $iacSnippet = @()
    $lun = 0
    Write-Host "Generating snippets for disks of VM: $vmName"
    foreach ($disk in $vmDetails.StorageProfile.DataDisks) {
        $lun = $disk.Lun  # Get the LUN number of the disk
        $diskRG = if ($targetDiskRG) { $targetDiskRG } else { $vmRG }
        $diskName = $disk.Name

        # Call GetDataBlock and GetAttachBlock to generate the blocks
        $dataBlock = GetDataBlock -diskName $diskName -diskRG $diskRG
        $attachBlock = GetAttachBlock -diskName $diskName -lun $lun

        $iacSnippet += $dataBlock
        $iacSnippet += $attachBlock
    }

    if ($persistOSDisk -eq $true) {
        # If OS disk is persisted, we need to generate the OS disk blocks
        $osDiskDetails = $vmDetails.StorageProfile.OsDisk
        $osDiskRG = if ($targetDiskRG) { $targetDiskRG } else { $vmRG }
        $osDiskName = $osDiskDetails.Name

        # Call GetDataBlock and GetAttachBlock for the OS disk
        $dataBlockOS = GetDataBlock -diskName $osDiskName -diskRG $osDiskRG -isOsDisk $true
        $attachBlockOS = GetAttachBlock -diskName $osDiskName -lun 0 -isOsDisk $true

        $iacSnippet += $dataBlockOS
        $iacSnippet += $attachBlockOS
    }

    return $iacSnippet
}

# Modular function to delete NICs
function Delete-NICs {
    param(
        [array]$nicObjs,
        [string]$vmName,
        [string]$vmId,
        [string]$errorLog
    )

    foreach ($nic in $nicObjs) {
        try {
            Remove-AzNetworkInterface -Name $nic.Name -ResourceGroupName $nic.ResourceGroupName -Force -ErrorAction Stop
            Write-Host "Deleted NIC: $($nic.Name) (ResourceGroup: $($nic.ResourceGroupName))"
            Send-Telemetry -EventName "NICDeleted" -Properties @{NIC = $nic.Name; VM = $vmName}
        } catch {
            $errorMsg = "Error deleting NIC $($nic.Name) for VM $vmName : $_"
            Write-Error $errorMsg
            Add-Content -Path $errorLog -Value $errorMsg
            Send-Telemetry -EventName "Error" -Properties @{NIC = $nic.Name; VM = $vmName; Error = $_}
        }
    }
}

# Modular function to process VM disks
function Process-VMDisks {
    param(
        [object]$vmDetails,
        [string]$vmName,
        [string]$vmId,
        [string]$vmRG,
        [string]$persistOSDisk,
        [string]$targetDiskRG,
        [string]$targetDiskSub,
        [string]$sourceSubscriptionId,
        [bool]$deleteVMs
    )

    if ($deleteVMs) {
        return DeleteVMMoveDisksGenerateIAC -vmDetails $vmDetails -vmName $vmName -vmId $vmId -vmRG $vmRG -persistOSDisk $persistOSDisk -targetDiskRG $targetDiskRG -targetDiskSub $targetDiskSub -sourceSubscriptionId $sourceSubscriptionId
    } else {
        return ShutdownVMMoveDisksGenerateIAC -vmDetails $vmDetails -vmName $vmName -vmId $vmId -vmRG $vmRG -persistOSDisk $persistOSDisk -targetDiskRG $targetDiskRG -targetDiskSub $targetDiskSub -sourceSubscriptionId $sourceSubscriptionId
    }
}

# Modular function to process each VM
function Process-VM {
    param(
        [string]$vmId,
        [hashtable]$vmNicInfo,
        [string]$outputDir,
        [string]$errorLog,
        [bool]$deleteNics,
        [bool]$deleteVMs,
        [string]$persistOSDisk,
        [string]$targetDiskRG,
        [string]$targetDiskSub
    )

    try {
        $sourceSubscriptionId = (($vmId -split '/')[2]).ToLower()
        Write-Host "Extracted Subscription ID: $sourceSubscriptionId"
        Set-AzContext -SubscriptionId $sourceSubscriptionId -ErrorAction Stop
        Write-Host "Switched to Subscription: $sourceSubscriptionId"

        $vm = Get-AzResource -ResourceId $vmId -ErrorAction Stop
        $vmName = $vm.Name
        $vmRG = $vm.ResourceGroupName

        Wait-For-ActiveDeployments -resourceGroupName $vmRG -deploymentTypes @("vm_deploy", "CreateVm") -timeoutMinutes 30

        $vmDetails = Get-AzVM -ResourceGroupName $vmRG -Name $vmName -ErrorAction Stop
        $iacSnippet = Process-VMDisks -vmDetails $vmDetails -vmName $vmName -vmId $vmId -vmRG $vmRG -persistOSDisk $persistOSDisk -targetDiskRG $targetDiskRG -targetDiskSub $targetDiskSub -sourceSubscriptionId $sourceSubscriptionId -deleteVMs $deleteVMs

        $iacSnippet | Out-File -FilePath "$outputDir\$vmName.tf" -Encoding utf8
        Write-Host "IAC snippet generated for disks of VM: $vmName" -ForegroundColor Green

        if ($deleteNics -and $deleteVMs -and $vmNicInfo.ContainsKey($vmId)) {
            Write-Host "Deleting NICs for VM: $vmName ($vmId)"
            Delete-NICs -nicObjs $vmNicInfo[$vmId].NICs -vmName $vmName -vmId $vmId -errorLog $errorLog
        }

    } catch {
        $errorMsg = "Error processing VM $vmId : $_"
        Write-Error $errorMsg
        Add-Content -Path $errorLog -Value $errorMsg
        Send-Telemetry -EventName "Error" -Properties @{VM = $vmId; Error = $_}
    }
}

$scriptRunID = [guid]::NewGuid().ToString()
Write-Host "Script Run ID: $scriptRunID"

# Load configuration file
$config = Get-Configuration -configFilePath ".\config.json"

# Extract configuration values
$vmResourceIds = $config.vmResourceIds
$deleteNics = if ($config.deleteNics -eq "true") { $true } else { $false }
$targetDiskRG = ($config.targetDiskRG).ToLower()
$targetDiskSub = $config.targetDiskSub.ToLower()
$confirmBeforeProceeding = if ($config.confirmBeforeProceeding -eq "true") { $true } else { $false }
$persistOSDisk = if ($config.persistOSDisk -eq "true") { $true } else { $false }
$deleteVMs = if ($config.deleteVMs -eq "true") { $true } else { $false }
$enableTelemetry = if ($config.enableTelemetry -eq "true") { $true } else { $false }

# Print values of all the configs parsed
Write-Host "Parsed Configuration Values:"
Write-Host "VM Resource IDs: $vmResourceIds"
Write-Host "Delete NICs: $deleteNics"
Write-Host "Target Disk Resource Group: $targetDiskRG"
Write-Host "Target Disk Subscription: $targetDiskSub"
Write-Host "Confirm Before Proceeding: $confirmBeforeProceeding"
Write-Host "Persist OS Disk: $persistOSDisk"
Write-Host "Delete VMs: $deleteVMs"

# Validate configuration values
if (-Not $vmResourceIds -or $vmResourceIds.Count -eq 0) {
    Write-Error "No VM Resource IDs provided in the configuration file. Exiting."
    exit
}

$sourceSubscriptionId = (($vmResourceIds[0] -split '/')[2]).ToLower()
Send-AzMigrateTelemetry -EventName "StartDiskMigration" -ScriptRunId $scriptRunID -Properties @{
    SourceSubscriptionId = $sourceSubscriptionId
    TargetSubscriptionId = $targetDiskSub
    Config = $config
}

# Gather NICs for review if needed
$vmNicInfo = Get-NICs -vmResourceIds $config.vmResourceIds -deleteNics $config.deleteNics

# Review VMs and associated NICs to be deleted
Write-Host "`nResources to be processed:"  # Display the resources to be processed
foreach ($vmId in $vmResourceIds) {
    $vmName = $null
    if ($vmNicInfo.ContainsKey($vmId)) {
        $vmName = $vmNicInfo[$vmId].VMName  # Retrieve VM name from NIC info
    } else {
        try {
            $vm = Get-AzResource -ResourceId $vmId -ErrorAction Stop
            $vmName = $vm.Name  # Retrieve VM name from Azure resource
        } catch { $vmName = $vmId }  # Fallback to VM ID if name retrieval fails
    }
    Write-Host "VM: $vmName ($vmId)"  # Display VM details
    if ($deleteNics -eq $true -and $vmNicInfo.ContainsKey($vmId)) {
        $nicObjs = $vmNicInfo[$vmId].NICs
        if ($nicObjs.Count -gt 0) {
            Write-Host "  NICs:"  # Display associated NICs
            foreach ($nic in $nicObjs) {
                Write-Host "    - $($nic.Name) (ResourceGroup: $($nic.ResourceGroupName))"  # NIC details
            }
        } else {
            Write-Host "  NICs: None"  # No NICs associated
        }
    }
}

# Confirm deletion of VMs and NICs (if applicable)
if ($confirmBeforeProceeding -eq $true) {
    $confirm = Read-Host "`nProceed with the above operations? (yes/no)"  # Prompt for confirmation
    if ($confirm -ne "yes") {
        Write-Host "Review declined. Exiting."  # Exit if user declines
        exit
    }
}

# Output Directories
$outputDir = ".\VM_Disk_IAC"  # Directory for IaC snippets
$errorLog = ".\error_log.txt"  # Log file for errors
$logFile = ".\logfile.txt"  # Log file for telemetry

# Create output directory and clear error log
New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
Clear-Content -Path $errorLog -ErrorAction SilentlyContinue

Send-AzMigrateTelemetry -EventName "ProcessDiskMigration" -ScriptRunId $scriptRunID -Properties @{
    SourceSubscriptionId = $sourceSubscriptionId
    TargetSubscriptionId = $targetDiskSub
    Config = $config
}

# Main script execution
foreach ($vmId in $vmResourceIds) {
    Process-VM -vmId $vmId -vmNicInfo $vmNicInfo -outputDir $outputDir -errorLog $errorLog -deleteNics $deleteNics -deleteVMs $deleteVMs -persistOSDisk $persistOSDisk -targetDiskRG $targetDiskRG -targetDiskSub $targetDiskSub
}

Send-AzMigrateTelemetry -EventName "CompleteDiskMigration" -ScriptRunId $scriptRunID -Properties @{
    SourceSubscriptionId = $sourceSubscriptionId
    TargetSubscriptionId = $targetDiskSub
    Config = $config
}

Write-Host "Script execution completed. Check '$outputDir' for IAC snippets and '$errorLog' for errors."