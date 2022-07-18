$global:cldSericeName = "venustest2k12"
$global:vmName = "venu-w2k12"
$global:containerName = "uploads"
$global:SrcHostId = "B85895B3-0F98-B14B-8E86D5DD3D826F20"
$global:customeScriptFilesList = [System.Collections.ArrayList]@("StartupScript.ps1","AzureRecoveryTools.zip","azurerecovery-B85895B3-0F98-B14B-8E86D5DD3D826F20.conf","hostinfo-B85895B3-0F98-B14B-8E86D5DD3D826F20.xml")

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
