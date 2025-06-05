# Description: This script adds multiple disks to a VM that is already protected by Azure Site Recovery (ASR).
<#
This script adds multiple disks to a VM that is already protected by Azure Site Recovery (ASR).
The disks are added sequentially, and the script waits for each job to complete before proceeding to the next disk.
The details of the vault, primary fabric, primary container, and VM are specified at the beginning of the script.
These details canbe found in the Azure Portal under the Recovery Services Vault, as part of the JSON view of the VM.
#>

$VaultSubscription = "<vault-subscriptionid>" # Replace with actual subscription ID of the vault
$vaultRGName = "<vault-resource-group-name>" # Replace with actual resource group name of the vault
$vaultName = "<vault-name>" # Replace with actual name of the vault
$primaryFabricName = "<primary-fabric-name>" # Replace with actual name of the primary fabric
$primaryContainerName = "<primary-container-name>" # Replace with actual name of the primary container
$vmName = "<vm-name>" # Replace with actual name of the VM to which disks will be added
#$friendlyName = ""
$cacheStorageAccountName = "<name of the cache storage account>" # Replace with actual name of the cache storage account
$cacheStorageAccountRG = "<resource group of the cache storage account>" # Replace with actual resource group name of the cache storage accountq
$rgName = "<resource-group-name>" # Replace with actual resource group name where the disks are located
$recoveryResourceGroupId = "/subscriptions/<subscription-id>/resourceGroups/<recovery-resource-group-name>" # Replace with actual recovery resource group ARM ID

$dataDiskList = @("<armId-Disk1>", "<armId-Disk2>") # Replace with actual ARM IDs of the disks to be added

Connect-AzAccount
Select-AzSubscription -Subscription $VaultSubscription
$vault = Get-AzRecoveryServicesVault -ResourceGroupName $vaultRGName -Name $vaultName -ErrorAction SilentlyContinue
Set-AzRecoveryServicesAsrVaultSettings -Vault $vault
$primaryFabricObject = Get-AzRecoveryServicesAsrFabric -Name $primaryFabricName;$primaryFabricObject
$primaryContainerObject = Get-AzRecoveryServicesAsrProtectionContainer -Fabric $primaryFabricObject -Name $primaryContainerName;$primaryContainerObject
$protectedItemObject = Get-AsrReplicationProtectedItem -ProtectionContainer $primaryContainerObject -Name $vmName;$protectedItemObject

foreach ($newdisk in $dataDiskList) {
    $newDisk = Get-AzDisk -ResourceGroupName $rgName -DiskName $newdisk.Split("/")[-1];$newDisk
    $storageAccount = Get-AzStorageAccount -ResourceGroupName $cacheStorageAccountRG -Name $cacheStorageAccountName -ErrorAction SilentlyContinue
    # If you have a Disk Encryption Set (DES) for the target disks, uncomment the line below and set the correct value
    #$targetDES = "<armId-of-target-DES>" # Replace with actual ARM ID of the target Disk Encryption Set
    # If you want to use Disk Encryption Set, uncomment the line below and set the correct value. Comment the next line.
    #$disk1=New-AzRecoveryServicesAsrAzureToAzureDiskReplicationConfig -DiskId $newDisk.Id -LogStorageAccountId $storageAccount.Id -ManagedDisk -RecoveryReplicaDiskAccountType Premium_LRS -RecoveryResourceGroupId $recoveryResourceGroupId -RecoveryTargetDiskAccountType Premium_LRS -RecoveryDiskEncryptionSetId $targetDES
    
    $disk1=New-AzRecoveryServicesAsrAzureToAzureDiskReplicationConfig -DiskId $newDisk.Id -LogStorageAccountId $storageAccount.Id -ManagedDisk -RecoveryReplicaDiskAccountType Premium_LRS -RecoveryResourceGroupId $recoveryResourceGroupId -RecoveryTargetDiskAccountType Premium_LRS

    $diskList = New-Object System.Collections.ArrayList
    $disk1
    $diskList.Add($disk1)
    Write-Host "Disks added: $($diskList.Count)"

    $protectedItemObject
    Write-Host "Adding disks to the protected item.."
    $addDRjob = Add-AzRecoveryServicesAsrReplicationProtectedItemDisk -ReplicationProtectedItem $protectedItemObject -AzureToAzureDiskReplicationConfiguration $diskList
    Write-Host "Started the job to add disks to the protected item.."

    Write-Host "Waiting for the job to complete.."
    WaitForJobCompletion -JobId $addDRjob.Name -JobQueryWaitTimeInSeconds 120
    write-host "Job completed successfully"
}
   
