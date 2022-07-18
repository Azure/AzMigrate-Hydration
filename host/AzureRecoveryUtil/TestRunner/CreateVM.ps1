#
# Usage: CreateVM.ps1 -SubscriptionName <Subscription Name> -StorageAccount <Storage Account Name> -cldService <cloud service name> -vmName <VM-Name> -osType <Windows/Linux> -blbUrls <urls seperated by ;>
#
# Notes: The first URL in blbUrls list should be the OS blob Url.
#
param 
(
    [Parameter(Mandatory=$true)]
    [string]
    $SubscriptionName,

    [Parameter(Mandatory=$true)]
    [string]
    $StorageAccount,

    [Parameter(Mandatory=$true)]
    [string]
    $cldService,

    [Parameter(Mandatory=$true)]
    [string]
    $vmName,

    [Parameter(Mandatory=$true)]
    [string]
    $osType,

    [Parameter(Mandatory=$true)]
    [string]
    $blbUrls
)


$global:diskBlobUrls = $blbUrls.Split(";")
if ( $diskBlobUrls.Count -eq 0 )
{
    Write-Error " --diskBlobUrls option is mising or its value is empty"

    exit 1
}

# TODO: Change the size to higher if more than 4 data disks are going to attach to the VM.
$vmSize = "Medium"

function RemoveDiskAndRetainBlob( $blbMediaLocation )
{
    Get-AzureDisk | ForEach-Object { 
         if ( $_.MediaLink -ieq $blbMediaLocation )
         {
            $diskName = $_.DiskName

            Write-Host "Deleting the disk $diskName associated with the blob : $blbMediaLocation ..."

            Remove-AzureDisk -DiskName $diskName
            if( !$? )
            {
                Write-Warning "Could not delete the disk $diskName"
                exit 1
            }

            # Wait for the disk deletion
            $diskExist = $true
            while ( $diskExist )
            {
                Write-Host "Waiting for the Disk $diskName deletion ..."

                $diskExist = $false

                Get-AzureDisk | ForEach-Object { if( $_.DiskName -ieq $diskName ) { $diskExist = $true; Start-Sleep -Seconds 10 ; break } }
            }

            Write-Host "Successfully deleted the disk $diskName."

            return;
         }
    }
}

function CreateVM( )
{
    # Creating azure vm using powershell commands
    # Ref: https://azure.microsoft.com/en-in/documentation/articles/virtual-machines-ps-create-preconfigure-windows-vms/
    #
    
    # Remove any disk lease on OS blob by deleting the Azure Disk
    RemoveDiskAndRetainBlob $global:diskBlobUrls[0]

    # Create new disk with the OS blob and mark that it contains OS.
    Add-AzureDisk -DiskName "$vmName-OsDisk" -MediaLocation $global:diskBlobUrls[0] -OS $osType
    if( !$? )
    {
        Write-Warning "Could not create Azure disk with the blob : $($global:diskBlobUrls[0])"

        exit 1
    }

    Write-Host "Creating the recovered VM ..."
    # create vm configuration
    $rec_vm = New-AzureVMConfig -Name $vmName -InstanceSize $vmSize -DiskName "$vmName-OsDisk" -HostCaching ReadWrite

    if( $osType -eq "Windows")
    {   
        # add the endpoints configuration to vm config
        $rec_vm | Add-AzureEndpoint -Name "Remote Desktop" -LocalPort 3389 -Protocol tcp -PublicPort 3389
    }
    else # Linux
    {   
        # add the endpoints configuration to vm config
        $rec_vm | Add-AzureEndpoint -Name "SSH" -LocalPort 22 -Protocol tcp -PublicPort 22
    }

    # Add data disks configuration to vm config
    $diskLunPos = -1
    foreach ( $diskUrl in $global:diskBlobUrls )
    {
        if( $diskLunPos -ne -1)
        {
            RemoveDiskAndRetainBlob $diskUrl

            $rec_vm | Add-AzureDataDisk -ImportFrom -MediaLocation "$diskUrl" -LUN $diskLunPos -DiskLabel "$vmName-datadisk-$diskLunPos"
        }

        $diskLunPos++
    }

    New-AzureVM -ServiceName $cldService -VMs $rec_vm -WaitForBoot >> $null
    if( !$? )
    {
        Write-Error "Error creating recovered vm"
        exit 1
    }

    Write-Host "Successfully created the recovered VM"
}

# Set default subscription
Select-AzureSubscription -SubscriptionName $SubscriptionName
if( !$? )
{
    Write-Warning "Could not not set default Azure subscription to $($SubscriptionName)"

    exit 1
}

# Set default storage account for subscription
Set-AzureSubscription -SubscriptionName $SubscriptionName -CurrentStorageAccountName $StorageAccount
if( !$? )
{
    Write-Warning "Could not not set default Azure storage account to $($StorageAccount)"

    exit 1
}

# Start CreateVM
CreateVM