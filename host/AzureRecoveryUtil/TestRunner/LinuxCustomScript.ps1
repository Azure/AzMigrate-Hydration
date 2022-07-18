#$vm = Get-AzureVM -ServiceName "venustest" -Name "venu_rhel6u5"
#$PublicConfiguration = '{"commandToExecute": "sh -c pwd"}' 
#
#$ExtensionName = 'CustomScriptForLinux'  
#$Publisher = 'Microsoft.OSTCExtensions'  
#$Version = '1.*' 
#Set-AzureVMExtension -ExtensionName $ExtensionName -VM  $vm -Publisher $Publisher -Version $Version -PublicConfiguration $PublicConfiguration  | Update-AzureVM

$global:SrcHostId = "1234567890"

    $containeruri = "abc.xyz"
    $files = [System.Collections.ArrayList]@("$containeruri/hostinfo-$global:SrcHostId.xml","$containeruri/azurerecovery-$global:SrcHostId.conf","$containeruri/AzureRecoveryTools.zip")

    $files.Add("$containeruri/StartupScript.sh")


$extNotFound = $true
do
{
    $vm = Get-AzureVM -ServiceName "venustest" -Name "venu_rhel6u5"
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
                
                Write-Host "$($vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Message)"

                exit 1
                #$extNotFound = $false
            }
            else
            {
                Write-Host "Custom script current status: $($vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Status)"
            }

            Write-Host $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.FormattedMessage.Message
            Write-Host $vm.ResourceExtensionStatusList[$extIndex].ExtensionSettingStatus.Code

            break
        }
    }
} while ($extNotFound)