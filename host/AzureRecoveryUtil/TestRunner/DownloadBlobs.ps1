#
# Usage: DownloadBlobs.ps1 -StorageAccount <Storage Account Name> -ContainerName <cloud service name> -StorageKey <Storage-Key> -LocalDownloadFolderPath <Local path to save the downloaded blobs>
#
# Notes: It will download all the blobs in give container to local folder
#
param 
(
    [Parameter(Mandatory=$true)]
    [string]
    $StorageAccount,

    [Parameter(Mandatory=$true)]
    [string]
    $ContainerName,

    [Parameter(Mandatory=$true)]
    [string]
    $StorageKey,

    [Parameter(Mandatory=$true)]
    [string]
    $LocalDownloadFolderPath
)

$connection_string = "DefaultEndpointsProtocol=https;AccountName=$StorageAccount;AccountKey=$StorageKey"

$storage_account_context = New-AzureStorageContext -ConnectionString $connection_string

$blobs = Get-AzureStorageBlob -Container $ContainerName -Context $storage_account_context

foreach ($blob in $blobs)
{
    New-Item -ItemType Directory -Force -Path $LocalDownloadFolderPath

    Get-AzureStorageBlobContent `
    -Container $ContainerName -Blob $blob.Name -Destination $LocalDownloadFolderPath `
    -Context $storage_account_context
}