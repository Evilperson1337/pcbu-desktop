param(
    [Parameter(Mandatory = $true)]
    [string]$Zip,

    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [Parameter(Mandatory = $true)]
    [string[]]$RequiredFiles
)

$ErrorActionPreference = 'Stop'

if (!(Test-Path $Zip)) {
    throw "Missing artifact: $Zip"
}

if (Test-Path $Destination) {
    Remove-Item $Destination -Recurse -Force
}

Expand-Archive -Path $Zip -DestinationPath $Destination -Force

foreach ($file in $RequiredFiles) {
    $filePath = Join-Path $Destination $file
    if (!(Test-Path $filePath)) {
        throw "$file not found in $Zip"
    }
}
