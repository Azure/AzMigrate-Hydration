
# Using this script to verify the pre-recvery steps
# =================================================
# 1. Place all the vhds of a vm on a folder ( Download vmdks from esx server and convert them into vhds)
#
# 2. Create a config file with following information
#       SUBCRIPTIO_NAME=<Configure a scubscript to powershell and provide the subscription name here>
#       STORAGE_ACCOUNT=<Storage Account name inthe subscription.>
#       OS_VHD_NAME=<OS vhd name in vhds folder, ex: win2k12-os-disk.vhd>
#       VM_NAME=<Name for azure vm >
#       LOGIN_PWD=<VM login passwrd>
#       CLOUD_SERVICE_NAME=<cloud service name>
#       SOURCE_HOST_ID=<Scout agent host-id>
#
# 3. Provide the confg file path and vhds folder path as input arguemnt to this script.
#    Ex: .\TestRunners.ps1 "F:\vmdks\venu-w2k12\runner.conf" "F:\vmdks\venu-w2k12" "Windows"
#
# 4. Pass the OS type information as 3rd argument to the script. Allowed values are "Windows" or "Linux"
# 
# Note-1: This script prompt you two times:
#         First, it will upload vhds, creadtes disks with it and then generates a recovery configuration
#         file and prompts you to replace disk name/guids with vhd name in that file.
#
#         Second, it prompts you to verify the pre-recovery execution status. At this stage the custom script
#         is set for hydration-vm and it is beeing processed. So you can verify logs and pre-recovery steps
#         completion status and then hit enter to create the final recovered-vm with the disks.
#
# Note-2: Be careful in providing vm-names & cloud-service names. If they are not complient with their corresponding
#         naming rules then the operation may fail.
#
# Note-3: Make sure that the storage-account & container specified are valid and exist. And also makesure that the
#         container does not have any blobs belogs to previour executions of this script. If exist then delete them.
#         Its recomended to have a dedicated container for this test, and empty the container once the test is completed
#         so that it does not conflict with next test runs.
#

$global:subName = "Visual Studio Ultimate with MSDN"
$global:storageAccount = ""
$global:containerName = "uploads"
$global:blob_base_uri = ""
$global:osVHDName = ""
$global:vmName = ""
$global:cldSericeName = ""
$global:winImageFilter = "Windows Server 2012 R2 Datacenter"
$global:linuxImageFilter = "OpenLogic 6.6"
$global:vmSize = "Medium"
$global:vmUser = "venu"
$global:vmUserPwd = ""
$global:SrcHostId = ""
$global:runnerConfigFile = ""
$global:configFolder = ""
$global:vhdDiskMappingString = ""
$global:promptAfterCustomScript = ""
$statusBlobName = "recoveryutiltestrunner.status"
#
# Validate command arguments and update the golbal variables.
#
if( $args.Count -ne 3 )
{
    #
    # Usage : <config-file-path> <vhds-folder> <Windows/Linux>
    #
    Write-Error "Argument Error: Invalid arguments"
    Write-Host "Usage: TestRunners.ps1 <config-file-path> <vhds-folder> <Windows/Linux>"
    exit 1 
}
#
# Parse the config file and update the configuration parameters
#
if( Test-Path -Path $args[0] )
{
    $global:runnerConfigFile = $args[0]

    $global:configFolder = $(Get-ChildItem $global:runnerConfigFile).DirectoryName

    $content = Get-Content $args[0]
    foreach( $line in $content )
    {
        if (($line.Length -ne 0 ) -and !$line.StartsWith("#"))
        {
            if($line.StartsWith("SUBCRIPTIO_NAME="))
            {
                $global:subName = $($line.Remove(0,"SUBCRIPTIO_NAME=".Length))
            }
            elseif($line.StartsWith("STORAGE_ACCOUNT="))
            {
                $global:storageAccount = $line.Remove(0,"STORAGE_ACCOUNT=".Length)
            }
            elseif($line.StartsWith("OS_VHD_NAME="))
            {
                $global:osVHDName = $line.Remove(0,"OS_VHD_NAME=".Length)
            }
            elseif($line.StartsWith("VM_NAME="))
            {
                $global:vmName = $line.Remove(0,"VM_NAME=".Length)
            }
            elseif($line.StartsWith("CLOUD_SERVICE_NAME="))
            {
                $global:cldSericeName = $line.Remove(0,"CLOUD_SERVICE_NAME=".Length)
            }
            elseif($line.StartsWith("SOURCE_HOST_ID="))
            {
                $global:SrcHostId = $line.Remove(0,"SOURCE_HOST_ID=".Length)
            }
            elseif($line.StartsWith("VHD_DISK_ID_MAPPING="))
            {
                $global:vhdDiskMappingString = $line.Remove(0,"VHD_DISK_ID_MAPPING=".Length)
            }
            elseif($line.StartsWith("PROMPT_AFTER_SCRIPT="))
            {
                $global:promptAfterCustomScript = $line.Remove(0,"PROMPT_AFTER_SCRIPT=".Length)
            }
            elseif($line.StartsWith("LOGIN_PWD="))
            {
                $global:vmUserPwd = $line.Remove(0,"LOGIN_PWD=".Length)
            }
            else
            {
                Write-Error "Invalid line format in config file: $line"
                exit 1
            }
        }
    }
}
else
{
    Write-Error "Argument error: config file path argument missing or invalid path"
    exit 1
}

if( !$(Test-Path -Path $args[1] -PathType Container) )
{
    Write-Error "Argument error: invalid vhds folder path $args[1]"
    exit 1
}
$global:vhdsPath = $args[1]

$global:osType = $args[2]

$global:blob_base_uri = "https://$global:storageAccount.blob.core.windows.net"
$global:statusBlobSasUri = ""

$global:azureDiskLunMap = [System.Collections.Hashtable]@{}
$global:azureDisks = [System.Collections.ArrayList]@()

$global:customeScriptFilesList = [System.Collections.ArrayList]@()
$global:startupScriptName = ""

# Local VHD & disk0id
$global:vhdNameDiskIdMap = [System.Collections.Hashtable]@{}
$diskEntries = $global:vhdDiskMappingString -split ";"
foreach( $diskEntry in $diskEntries )
{
    $values = $diskEntry -split ":"
    if($values.Count -eq 2)
    {
        $global:vhdNameDiskIdMap.Add($values[0],$values[1])
    }
    else
    {
        Write-Warning "Invalid token found in Vhd-Disk-id mapping value: $diskEntry"
        Write-Warning "Ignoring the token"
    }
}

#
# Set azure subscription
#
Select-AzureSubscription $global:subName -Current
Set-AzureSubscription -SubscriptionName $global:subName -CurrentStorageAccountName $global:storageAccount 

#
# Functions region
#

function CleanupAndExit()
{
}

function UploadVhdFilesAndCreateDisks( )
{
    $container_uri = "$global:blob_base_uri/$global:containerName/"
       
    if ( !$(Test-Path -Path "$global:vhdsPath" -PathType Container) )
    {
        Write-Error "Invalid vhds folder path: $global:vhdsPath"
        exit 1
    }

    $vhdfiles = Get-ChildItem "$global:vhdsPath\*.vhd"

    foreach($fileObj in $vhdfiles )
    {
        Set-AzureStorageBlobContent -File $fileObj.FullName -BlobType Page -Container $global:containerName -Blob $fileObj.Name -Force >> $null
        if ( !$? )
        {
            Write-Error "Error uploading vhd to azure"
            exit 1
        }
        
        if ( $fileObj.Name -eq $global:osVHDName )
        {
            Add-AzureDisk -DiskName $fileObj.Name -MediaLocation $($container_uri+$fileObj.Name) -Label $fileObj.Name -OS $global:osType >> $null
        }
        else
        {
            Add-AzureDisk -DiskName $fileObj.Name -MediaLocation $($container_uri+$fileObj.Name) -Label $fileObj.Name >> $null
        }
        
        if ( !$? )
        {
            Write-Error "Error creating disk with the vhd $($fileObj.Name)"
            exit 1
        }

        $global:azureDisks.Add($fileObj.Name)
    }
}

function UploadFile( $localFile )
{
    Set-AzureStorageBlobContent -File $localFile.FullName -BlobType Block -Container $global:containerName -Blob $localFile.Name -Force >> $null
    if ( !$? )
    {
        Write-Error "Error uploading file to azure : $localFile.Name"
        exit 1
    }
}

function UploadCustomScriptFiles( $confFilesFolder )
{
    if( $global:osType -eq "Windows")
    {
        $global:startupScriptName = "StartupScript.ps1"
    }
    else
    {
        $global:startupScriptName = "StartupScript.sh"
    }

    $global:customeScriptFilesList.Add("$global:startupScriptName")

    #upload StartupScript & RecoveryTools.zip files as well
    $confFilesList = Get-ChildItem "$confFilesFolder\*-$global:SrcHostId.*"
    if ( !$? )
    {
        Write-Error "Config files not found"
        exit 1
    }

    foreach ( $confFile in $confFilesList )
    {
        UploadFile $confFile

        $global:customeScriptFilesList.Add("$($confFile.Name)")
    }

    #upload RecoveryTools.zip file
    $zipfileObj = Get-ChildItem "$confFilesFolder\AzureRecoveryTools.zip"
    if ( !$? )
    {
        Write-Error "Recovery tools zip file is not found"
        exit 1
    }

    UploadFile $zipfileObj

    $global:customeScriptFilesList.Add("$($zipfileObj.Name)")

    #upload StartupScript file
    $startupScriptFileObj = $(Get-ChildItem "$confFilesFolder\StartupScript.*" | select -First 1)
    if ( !$? )
    {
        Write-Error "Startup script file is not found"
        exit 1
    }

    UploadFile $startupScriptFileObj
    $global:customeScriptFilesList.Add("$($startupScriptFileObj.Name)")
}

function CreateStatusBlobSasTocken( )
{
    $blobmetadata = @{}

    #create a dummy file with 0 size
    $tmpFile = "$global:vhdsPath\tmpstatusfile.status"
    New-Item $tmpFile -ItemType file -Force
    if ( !$? )
    {
        Write-Error "Error creating status file"
        exit 1
    }

    Set-AzureStorageBlobContent -Blob $statusBlobName -Container $global:containerName -Metadata $blobmetadata -BlobType Page -File $tmpFile -Force >> $null
    if ( !$? )
    {
        Write-Error "Error creating status file"
        exit 1
    }

    #remove temp status file
    Remove-Item $tmpFile -Force

    $statusBlobObjUri = "$global:blob_base_uri/$global:containerName/$statusBlobName"

    
    $statusBlobSasToken =  (Get-AzureStorageBlob -Blob $statusBlobName -Container $global:containerName | New-AzureStorageBlobSASToken -Permission rw)

    $global:statusBlobSasUri = "$statusBlobObjUri$statusBlobSasToken"
}

function CreateRecoveryConfigFile()
{
    $recoveryConfFile = "$global:configFolder\azurerecovery-$global:SrcHostId.conf"

    # Generate the status blob sas uri
    CreateStatusBlobSasTocken

    "PreRecoveryExecutionBlobSasUri = $global:statusBlobSasUri" | Out-File -Encoding ascii -Force -Width 2048 -FilePath "$recoveryConfFile"
    
    "LogLevel = 4"| Out-File -Encoding ascii -Append -Force -Width 2048 -FilePath "$recoveryConfFile"

    # Add any other vmfig params here

    "[DiskMap]" | Out-File -Encoding ascii -Append -Force -Width 2048 -FilePath "$recoveryConfFile"
    foreach ( $disk in $global:azureDiskLunMap.GetEnumerator() )
    {
        if( $global:vhdNameDiskIdMap.ContainsKey($disk.Key) )
        {
            "$($global:vhdNameDiskIdMap.Item($disk.Key)) = $($disk.Value)" | Out-File -Encoding ascii -Append -Force -Width 2048 -FilePath "$recoveryConfFile"
        }
        else
        {
            Write-Warning "Key $($disk.Key) in vhd-name to disk-id map not found"
        }
    }

    Write-Host "Successfully created the recovery config file"
}

function CreateHydrationVM( )
{
    $imageLable = ""

    if( $osType -eq "Windows")
    {
        $imageLable = $global:winImageFilter
    }
    else
    {
        $imageLable = $global:linuxImageFilter
    }

    $image = Get-AzureVMImage | where { $_.Label -like "$imageLable*" } | sort PublishedDate -Descending | select -ExpandProperty ImageName -First 1

    Write-Host "Hydration-VM Image: $image"

    # create vm configuration
    $($vm = New-AzureVMConfig -Name $vmName -InstanceSize $vmSize -ImageName $image) >> $null

    if( $osType -eq "Windows")
    {
         # add the provisioning configuration to vm config
        $($vm | Add-AzureProvisioningConfig -Windows -AdminUsername $global:vmUser -Password $global:vmUserPwd) >> $null
        
        # add the endpoints configuration to vm config
        #$vm | Add-AzureEndpoint -Name "Remote Desktop" -LocalPort 3389 -Protocol tcp -PublicPort 3389
    }
    else
    {   
         # add the provisioning configuration to vm config
        $($vm | Add-AzureProvisioningConfig -Linux -LinuxUser $global:vmUser -Password $global:vmUserPwd) >> $null
        
        # add the endpoints configuration to vm config
        # $vm | Add-AzureEndpoint -Name "SSH" -LocalPort 22 -Protocol tcp -PublicPort 22
    }

    # Add data disks configuration to vm config
    $diskLunPos = 0
    foreach ( $diskName in $global:azureDisks )
    {
        $($vm | Add-AzureDataDisk -Import -DiskName "$diskName" -LUN $diskLunPos) >> $null

        $global:azureDiskLunMap.Add($diskName , $diskLunPos)

        $diskLunPos++
    }

    $cldSvr = Get-AzureService -ServiceName $global:cldSericeName
    if( !$? )
    {
        Write-Host "Creating cloud service: $global:cldSericeName"

        New-AzureService -ServiceName $global:cldSericeName  -Location $(Get-AzureStorageAccount -StorageAccountName $global:storageAccount).Location
        if( !$? )
        {
            Write-Error "Error creating cloud service for hydration-vm"
            exit 1
        }
    }

    New-AzureVM -ServiceName $global:cldSericeName -VMs $vm -WaitForBoot
    if( !$? )
    {
        Write-Error "Error creating hydration-vm"
        exit 1
    }

    Write-Host "Successfully created the Hydration-VM and attached all the disks"
}

function SetCustomeScriptExtentionForWindows( )
{
    # Custom script for Windows Azure VM
    # Ref : http://azure.microsoft.com/blog/2014/04/24/automating-vm-customization-tasks-using-custom-script-extension/
    #
    $vm = Get-AzureVM -ServiceName $global:cldSericeName -Name $global:vmName
    if( !$? )
    {
        Write-Error "Error retreiving vm object for custome script extention"
        exit 1
    }

    Set-AzureVMCustomScriptExtension -VM $vm -ContainerName $global:containerName -FileName $global:customeScriptFilesList -Run 'StartupScript.ps1' -Argument $global:SrcHostId | Update-AzureVM >> $null
    if( !$? )
    {
        Write-Error "Error updating custome script extention"
        exit 1
    }

    $extNotFound = $true
    do
    {
        $vm = Get-AzureVM -ServiceName $global:cldSericeName -Name $global:vmName
        if( !$? )
        {
            Write-Error "Error retreiving vm object after custome script extension"
            exit 1
        }
        
        $extIndex = 0        
        for( ; $extIndex -lt $vm.ResourceExtensionStatusList.Count; $extIndex++ )
        {
            if( $vm.ResourceExtensionStatusList[$extIndex].HandlerName -ilike "Microsoft.Compute.CustomScriptExtension" )
            {
                if( $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status -ilike "Success")
                {
                    Write-Host "Custome script succeeded"
                    $extNotFound = $false
                }
                elseif( $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status -ilike "Failed" )
                {
                    Write-Error "Custome script Failed"

                    $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.SubStatusList

                    exit 1
                    #$extNotFound = $false
                }
                else
                {
                    Write-Host "Custom script current status: $($vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status)"
                }

                break
            }
        }
    } while ($extNotFound)
}

function SetCustomeScriptExtentionForLinux( )
{
    # Custom script for Azure Linux VM
    # Ref: http://azure.microsoft.com/blog/2014/08/20/automate-linux-vm-customization-tasks-using-customscript-extension/
    #
    $vm = Get-AzureVM -ServiceName $global:cldSericeName -Name $global:vmName
    if( !$? )
    {
        Write-Error "Error retreiving vm object for custome script extention"
        exit 1
    }

    $ExtensionName = 'CustomScriptForLinux'  
    $Publisher = 'Microsoft.OSTCExtensions'  
    $Version = '1.*'
    
    $containeruri = "$global:blob_base_uri/$global:containerName"

    $filesUris = "`"$containeruri/StartupScript.sh`",`"$containeruri/hostinfo-$global:SrcHostId.xml`",`"$containeruri/azurerecovery-$global:SrcHostId.conf`",`"$containeruri/AzureRecoveryTools.zip`""

    $PublicConfiguration = "{`"fileUris`":[$filesUris], `"commandToExecute`": `"sh StartupScript.sh $global:SrcHostId`" }"

    $PrivateConfiguration = "{`"storageAccountName`": `"$global:storageAccount`",`"storageAccountKey`":`"$($(Get-AzureStorageKey –StorageAccountName $global:storageAccount).Primary)`"}"

    Set-AzureVMExtension -ExtensionName $ExtensionName -VM  $vm -Publisher $Publisher -Version $Version -PrivateConfiguration $PrivateConfiguration -PublicConfiguration $PublicConfiguration | Update-AzureVM >> $null
    if( !$? )
    {
        Write-Error "Error updating custome script extention"
        exit 1
    }

    $extNotFound = $true
    do
    {
        $vm = Get-AzureVM -ServiceName $global:cldSericeName -Name $global:vmName
        if( !$? )
        {
            Write-Error "Error retreiving vm object after custome script extension"
            exit 1
        }
        
        $extIndex = 0        
        for( ; $extIndex -lt $vm.ResourceExtensionStatusList.Count; $extIndex++ )
        {
            if( $vm.ResourceExtensionStatusList[$extIndex].HandlerName -ilike "Microsoft.OSTCExtensions.CustomScriptForLinux" )
            {
                if( $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status -ilike "Success")
                {
                    Write-Host "Custome script succeeded"
                    $extNotFound = $false
                }
                elseif( $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status -ilike "Failed" )
                {
                    Write-Error "Custome script Failed"

                    $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.SubStatusList

                    exit 1
                    #$extNotFound = $false
                }
                else
                {
                    Write-Host "Custom script current status: $($vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status)"
                }
                break
            }
        }
    } while ($extNotFound)
}

function AwaitScriptCompletion()
{
    Write-Host "Waiting for the execution status ..."
    $Status = "Pending"

    do 
    {
        $metadata = $(Get-AzureStorageBlob -Blob $statusBlobName -Container $global:containerName).ICloudBlob.Metadata

        if($metadata.ContainsKey("ExecutionStatus"))
        {
            $Status = $metadata["ExecutionStatus"]
        }

        if( $Status -eq "Success" )
        {
            break
        }
        elseif( $Status -eq "Failed" )
        {
            Write-Error "Pre-Recovery steps execution failed. "
            foreach( $metadataItem in $metadata)
            {
                Write-Host "$($metadataItem.Key) : $($metadataItem.Value)"
            }

            exit 1
        }
        else
        {
            Write-Host "$Status..."

            Start-Sleep -Seconds 30
        }
    } while ( $true )

    Write-Host "Execution status: $Status"

    if( $global:promptAfterCustomScript -icontains "yes" )
    {
        Read-Host "Custome script has comleted the execution. Hit Enter to proceed :"
    }
}

function SetCustomeScriptExtention ()
{
    CreateRecoveryConfigFile

    UploadCustomScriptFiles "$global:configFolder"

    if( $osType -eq "Windows")
    {
        SetCustomeScriptExtentionForWindows
    }
    else
    {
        SetCustomeScriptExtentionForLinux
    }
}

function RemoveHydrationVM ( )
{
    Write-Host "Deleting the hydration-vm and its associated VHDs..."
    
    Remove-AzureVM -Name $global:vmName -ServiceName $global:cldSericeName -DeleteVHD >> $null
    if( !$? )
    {
        Write-Error "Error deleting hydration-vm"
        exit 1
    }

    Write-Host "Deleted the hydration-vm"
}

function DettachDataDisksFromHydrationVM ( )
{
    Write-Host "Dettaching disks from Hydration-VM..."

    
    $dataDisks = Get-AzureDataDisk -VM $(Get-AzureVM -Name $global:vmName -ServiceName $global:cldSericeName)
    
    # Assuming no other data disks attached to Hydration-VM
    for( [System.Int32]$diskIndex = 0; $diskIndex -lt $dataDisks.Count; $diskIndex++)
    {
        Get-AzureVM -ServiceName $global:cldSericeName -Name $global:vmName | Remove-AzureDataDisk -LUN $diskIndex | Update-AzureVM
    }

    # Verify that the disks are dittached completely from hydration-vm
    foreach( $disk in $global:azureDisks )
    {
        Write-Host "Checking the disk $disk dettach status..."
        while( ![string]::IsNullOrEmpty($(Get-AzureDisk -DiskName $disk).AttachedTo) )
        {
            Write-Host "Disk $disk is still attached to the VM. Going to wait for 60sec..."
            Start-Sleep -Seconds 60
        }

        Write-Host "Disk $disk is dettached from the VM."
    }

    Write-Host "All the data-disks are dettached from Hydration-VM..."
}

function CreateRecoveredVM( )
{
    # Creating azure vm using powershell commands
    # Ref: https://azure.microsoft.com/en-in/documentation/articles/virtual-machines-ps-create-preconfigure-windows-vms/
    #

    Write-Host "Creating the recovered VM ..."
    # create vm configuration
    $rec_vm = New-AzureVMConfig -Name $vmName -InstanceSize $vmSize -DiskName $global:osVHDName -HostCaching ReadWrite

    if( $osType -eq "Windows")
    {   
        # add the endpoints configuration to vm config
        $rec_vm | Add-AzureEndpoint -Name "Remote Desktop" -LocalPort 3389 -Protocol tcp -PublicPort 3389
    }
    else
    {   
        # add the endpoints configuration to vm config
        $rec_vm | Add-AzureEndpoint -Name "SSH" -LocalPort 22 -Protocol tcp -PublicPort 22
    }

    # Add data disks configuration to vm config
    $diskLunPos = 0
    foreach ( $diskName in $azureDisks )
    {
        if( $diskName -eq $global:osVHDName)
        {
            #skip os disk
            continue
        }

        $rec_vm | Add-AzureDataDisk -Import -DiskName "$diskName" -LUN $diskLunPos

        $diskLunPos++
    }

    New-AzureVM -ServiceName $global:cldSericeName -VMs $rec_vm -WaitForBoot >> $null
    if( !$? )
    {
        Write-Error "Error creating recovered vm"
        exit 1
    }

    Write-Host "Successfully created the recovered VM"
}

function VerifyRecoveredVM()
{
    # TODO: Verify recovery configuration

    Read-Host "Hit Enter after verifying the recovered vm: "
}

function Cleanup()
{
    #
    # Delete the recovered vm and its associated vhds
    #
    Write-Host "Deleting the recovered-vm and its associated VHDs..."
    
    Remove-AzureVM -Name $global:vmName -ServiceName $global:cldSericeName -DeleteVHD >> $null
    if( !$? )
    {
        Write-Error "Error deleting recovered-vm"
        exit 1
    }

    Write-Host "Deleted the recovered-vm"


    #
    # Delete the custome script files uploaded to storage account.
    #

    $files = [System.Collections.ArrayList]@("hostinfo-$global:SrcHostId.xml","azurerecovery-$global:SrcHostId.conf","AzureRecoveryTools.zip","$statusBlobName")

    if ( $global:osType -match "Windows" )
    {
        $files.Add("StartupScript.ps1")
    }
    else
    {
        $files.Add("StartupScript.sh")
    }

    foreach ( $blob in $files )
    {
        Remove-AzureStorageBlob -Blob $blob -Container $global:containerName -Force >> $null
    }
    
}

#
# Workflow
#
function Main ( )
{
    UploadVhdFilesAndCreateDisks 

    CreateHydrationVM 

    SetCustomeScriptExtention

    AwaitScriptCompletion

    DettachDataDisksFromHydrationVM

    RemoveHydrationVM

    CreateRecoveredVM
        
    VerifyRecoveredVM

    Cleanup
}

### Main ###

Main