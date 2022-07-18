$statusBlobObjUri = "https://portalvhdsyw750pd688v6b.blob.core.windows.net/uploads/recoveryutiltestrunner.status"

Set-AzureSubscription -SubscriptionId a7f1d5a6-e549-4093-af3b-8fef2664d927 -CurrentStorageAccountName portalvhdsyw750pd688v6b

$statusBlobSasUri =  ( Get-AzureStorageBlob -Blob recoveryutiltestrunner.status -Container uploads | New-AzureStorageBlobSASToken -Permission rw -FullUri )

Write-Host $statusBlobSasUri