# ================================================================
# 1. Pre-Requisites:  
#    - Updates the execution policy on your machine  
#    - Installs the required PowerShell packages, Chocolatey, and armclient
# ================================================================

# Set execution policy
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Force

# Uninstall AzureRm
Get-InstalledModule -Name AzureRM* | Uninstall-Module -Force

# Install Az
if (Get-InstalledModule -Name Az) {
Write-Output "Az already installed"
}
else
{
Write-Output "Installing Az"
Install-Module -Name Az -AllowClobber -Force
}

Import-Module -Name Az

# Install chocolatey and armclient
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
choco install armclient --source=https://chocolatey.org/api/v2/

# Install Az.Migrate
if (Get-InstalledModule -Name Az.Migrate) {
Write-Output "Az.Migrate already installed"
}
else
{
Write-Output "Installing Az.Migrate"
Install-Module -Name Az.Migrate -AllowClobber -Force
}

Import-Module -Name Az.Migrate

# Install Az.RecoveryServices
if (Get-InstalledModule -Name Az.RecoveryServices) {
Write-Output "Az.RecoveryServices already installed"
}
else
{
Write-Output "Installing Az.RecoveryServices"
Install-Module -Name Az.RecoveryServices -AllowClobber -Force
}

Import-Module -Name Az.RecoveryServices



# ================================================================
# 2. Infrastructure Cleanup:  
#    - Retrieves infrastructure details from Azure using armclient  
#    - Deletes any migration-related infrastructure and artifacts that are no longer needed
# ================================================================



# Replace constants here.
$TenantId = <Replace Tenant Id value here>
$SubscriptionId = <Replace Subscription Id value here>
$RGName = <Replace Resource Group Name here>
$RSVaultName = <Replace Recovery Service Vault Name here>
$ProjectName = <Replace Project Name here>


# Connect to the tenant and subscription using your azure credentials.
Write-Host "Connecting to your azure account."
Connect-AzAccount -TenantId $TenantId -Subscription $SubscriptionId

# Get existing container mapping using armclient.
$GetContainerMappingInput = [System.String]::Concat("/subscriptions/", $SubscriptionId, "/resourceGroups/", $RGName, "/providers/Microsoft.RecoveryServices/vaults/", $RSVaultName, "/replicationProtectionContainerMappings?api-version=2024-01-01")
Write-Host "Fetching replication policy details for your migrate appliance."
$ContainerMapping = armclient get $GetContainerMappingInput|ConvertFrom-json

if ($ContainerMapping.value.Count -gt 1)
{
	Write-Host "Multiple container-mapping objects found. Ensure that the script has been already modified to choose the correct mapping object before proceeding."
	Write-Host "Press 'y' to continue and 'n' to stop."

	# Read a key without echoing it to the screen
	$key = $host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

	# Check the key pressed (case-insensitive)
	switch -regex ($key.Character) {
		'^[Yy]$' {
			Write-Host "Continuing the script..."
			# Place the rest of your script here
			break
		}
		'^[Nn]$' {
			Write-Host "Stopping the script."
			exit
		}
		default {
			Write-Host "Invalid input. Exiting script."
			exit
		}
	}
}

# Extract required resources from container mapping.

# Key vault
$KeyVaultId = $ContainerMapping.value.properties.providerspecificdetails.keyvaultid
$KeyVaultUri = $ContainerMapping.value.properties.providerspecificdetails.keyVaultUri
if ($KeyVaultUri -match 'https://([^/]+).vault.azure.net/') {
    $KeyVaultName = $matches[1]
}

# Gateway Storage Account (Leaving the log storage account to customer discretion.)
$GwsaId = $ContainerMapping.value.properties.providerspecificdetails.storageAccountId
$GwsaName = ($GwsaId -split '/')[-1]

# Service Bus Namespace
$SbSecretName = $ContainerMapping.value.properties.providerspecificdetails.serviceBusConnectionStringSecretName
$SbSecretvalue = Get-AzKeyVaultSecret -VaultName $KeyVaultName -Name $SbSecretName -AsPlainText
$SbEndpoint = ($SbSecretvalue -split ';' | Where-Object { $_ -like 'Endpoint=*' })
if ($SbEndpoint -match 'Endpoint=sb://([^/]+).servicebus.windows.net/') {
    $SbNamespaceName = $matches[1]
}
$SbNamespaceResourceId = (Get-AzServiceBusNamespace -ResourceGroupName $RGName -Name $SbNamespaceName).Id



# Deleting existing container mapping using armclient.
$DeleteContainerMappingInput = [System.String]::Concat($ContainerMapping.value.id, "?api-version=2024-01-01")
Write-Host "Dissociating replication policy for your migrate appliance."
armclient delete $DeleteContainerMappingInput

# Wait till container mapping gets deleted.
$maxRetries    = 5
$attempt       = 0
$resultCount   = 1    # initialize with a nonzero value so the loop starts

# Continue looping until either:
#    • There are no objects (resultCount equals 0)
#    • OR the number of attempts has reached the maxRetrie
do{
    $attempt++
    Write-Host "Attempt # {$attempt}: Executing armclient get..."

    # Execute the command
    $ContainerMappingRemaining = armclient get $GetContainerMappingInput|ConvertFrom-json

    $resultCount = $ContainerMappingRemaining.value.Count

    # If there are still objects and you haven't reached max attempts, wait before retrying
    if ( ($resultCount -ne 0) -and ($attempt -lt $maxRetries) ) {
        Write-Host "{$resultCount} container mapping object(s) returned. Waiting before retrying..."
        Start-Sleep -Seconds 15  # Adjust the wait time as needed
    }
}while( ($resultCount -ne 0) -and ($attempt -lt $maxRetries) )

# Final check after exiting the loop
if ($resultCount -eq 0) {
    Write-Host "No container mapping objects returned on attempt #{$attempt}. Proceeding..."
} else {
    Write-Host "Max retries reached ({$attempt} attempts) and container mapping objects are still being returned."
}



# Deleting remaining artifacts from customer subscription that are no longer required for migration.

# KeyVault
$lockName = [System.String]::Concat($ProjectName, "lock")
Write-Host "Removing resource lock {$lockName} in resource group {$RGName} for Keyvault resource {$KeyVaultName}."
Remove-AzResourceLock -ResourceGroupName $RGName -LockName $lockName -ResourceName $KeyVaultName -ResourceType Microsoft.KeyVault/vaults -Force
Write-Host "Removing keyvault resource {$KeyVaultId}."
Remove-AzResource -ResourceId $KeyVaultId -Force

# Service Bus Namespace
Write-Host "Removing resource lock {$lockName} in resource group {$RGName} for Service Bus Namespace resource {$SbNamespaceName}."
Remove-AzResourceLock -ResourceGroupName $RGName -LockName $lockName -ResourceName $SbNamespaceName -ResourceType Microsoft.ServiceBus/namespaces -Force
Write-Host "Removing service bus namespace resource {$SbNamespaceResourceId}."
Remove-AzResource -ResourceId $SbNamespaceResourceId -Force

# Gateway Storage Account
if ($GwsaName -match 'migrategwsa([^/]+)') {
    $GwsaSuffix = $matches[1]
	$LsaName = [System.String]::Concat("migratelsa", $GwsaSuffix)
}
Write-Host "Removing resource lock {$lockName} in resource group {$RGName} for storage account resource {$GwsaName}."
Remove-AzResourceLock -ResourceGroupName $RGName -LockName $lockName -ResourceName $GwsaName -ResourceType Microsoft.Storage/storageAccounts -Force
Write-Host "Removing storage account resource {$GwsaId}."
Remove-AzResource -ResourceId $GwsaId -Force

# Purge the delete keyvault resource as required.
Write-Host "Manually purge keyvault resource {$KeyVaultName} if found in soft-deleted state, as well as the log storage account {$LsaName}. These resources would be no longer used for migration."


# =============================================================================
# 3. Appliance VM Configuration Cleanup:  
#    - Resets the configuration on the appliance VM (this section must be run on the appliance VM)
# =============================================================================

# stop asrgwy service on appliance.
Stop-Service -Name "asrgwy"

# Delete gateway.json from config location.
Remove-Item -Path "C:\ProgramData\Microsoft Azure\Config\gateway.json"


# =============================================================================
# 4. Re-initialize the appliance infrastructure. This section is optional, and enabling any new replication from the portal will allow you to specify the desired location, Resource Group, and other necessary details to automatically set up the new migration infrastructure.
# =============================================================================

$vault = Get-AzRecoveryServicesVault -ResourceGroupName $RGName -Name $RSVaultName

if ($vault.Identity.Type -eq "None")
{
	Write-Host "Enabling System Assigned Managed Identity in Recovery Services Vault {$RSVaultName}."
	Update-AzRecoveryServicesVault -ResourceGroupName $RGName -Name $RSVaultName -IdentityType "SystemAssigned"
}

$TargetLocation = <Replace Target Location here>
Write-Host "Initializing replication infrastructure for migration appliance."
Initialize-AzMigrateReplicationInfrastructure -ResourceGroupName $RGName -ProjectName $ProjectName -Scenario agentlessVMware -TargetRegion $TargetLocation