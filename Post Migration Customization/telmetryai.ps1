[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)][string] $InstrumentationKey = "92d368e0-d3a4-431d-9ff0-4b610f0a2911",
    [Parameter(Mandatory = $false)][string] $module_source = "azmalziac/diskmigrationpsscript",
    [Parameter(Mandatory = $true)][string] $event,
    [Parameter(Mandatory = $true)][string] $resource_id,
    [Parameter(Mandatory = $true)][string] $properties
)


Write-Host "Using properties: $properties"
# App Insights ingestion endpoint
$ingestionEndpoint = "https://dc.services.visualstudio.com/v2/track"

# Ensure TLS 1.2 for older environments
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch { }

# Parse properties: accept JSON or plain text
$parsedProps = @{}
$parsedProps["properties"] = $properties

# Add standard fields into properties
$parsedProps["module_source"] = $module_source
$parsedProps["resource_id"]  = $resource_id
$parsedProps["event"]        = $event
$parsedProps["timestamp"]    = (Get-Date).ToUniversalTime().ToString("o")

# Create telemetry payload
$telemetryData = @{
    # For v2 track API, the envelope name is the type without iKey
    name = "Microsoft.ApplicationInsights.Event"
    time = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ss.fffZ")
    iKey = $InstrumentationKey
    tags = @{
        "ai.device.os" = [string][System.Environment]::OSVersion.Platform
        "ai.device.osVersion" = [string][System.Environment]::OSVersion.Version
        "ai.internal.sdkVersion" = "ps:telemetryai/1.0.0"
    }
    data = @{
        baseType = "EventData"
        baseData = @{
            ver = 2
            name = $event
            properties = $parsedProps
            measurements = @{}
        }
    }
}

# Convert to JSON
$jsonPayload = $telemetryData | ConvertTo-Json -Depth 10 -Compress

try {
    # Send to App Insights
    $response = Invoke-RestMethod -Uri $ingestionEndpoint -Method Post -Body $jsonPayload -ContentType "application/json"
    Write-Host "Event '$event' sent successfully to App Insights"
    Write-Verbose ($jsonPayload)
    if ($response) { Write-Verbose ("Response: " + ($response | ConvertTo-Json -Depth 5 -Compress)) }
}
catch {
    Write-Error "Failed to send telemetry: $($_.Exception.Message)"
    exit 1
}