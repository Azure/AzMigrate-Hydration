
# change the subscription to the target subscription id.

$Region = "northeurope" # change to target region here 
$type = "virtualMachines"
$subscriptionId = ""

Set-AzContext -SubscriptionId $SubscriptionId
$VMSKUs = Get-AzComputeResourceSku | Where-Object { $_.Locations -contains $Region -and $_.ResourceType.Contains("virtualMachines") }
 
$OutTable = @()
 
foreach ($SkuName in $VMSKUs)
{       
    foreach($capability in $SkuName.Capabilities)
    {
        if($capability.Name -contains "DiskControllerTypes")
        {
        #  TODO: Return SCSI or SCSI, NVME or NVME, SCSI
        if ($capability.Value -match "SCSI") {
            $OutTable += New-Object PSObject -Property @{
                "Name" = $SkuName.Name
                "DiskControllerTypes" = $capability.Value
            }
        }
    }
}
}

$OutTable | select Name, DiskControllerTypes | Sort-Object -Property Name | Format-Table