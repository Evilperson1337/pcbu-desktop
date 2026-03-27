param(
    [Parameter(Mandatory = $true)]
    [string]$ServiceExe,

    [Parameter(Mandatory = $true)]
    [string]$UnlockCtlExe,

    [Parameter(Mandatory = $true)]
    [ValidateSet('pipe', 'rest')]
    [string]$Mode,

    [string]$ApiToken = 'ci-loopback-token'
)

$ErrorActionPreference = 'Stop'

$settingsDir = 'C:\ProgramData\PCBioUnlock'
$settingsPath = Join-Path $settingsDir 'app_settings.json'

New-Item -Path $settingsDir -ItemType Directory -Force | Out-Null

$settings = @{
    machineID = 'ci-smoke-test-machine'
    installedVersion = 'ci'
    language = 'auto'
    serverIP = 'auto'
    serverMAC = ''
    pairingServerPort = 43295
    unlockServerPort = 43296
    clientSocketTimeout = 120
    clientConnectTimeout = 5
    clientConnectRetries = 2
    winServiceLoopbackPort = 43297
    winServicePipeName = '\\.\pipe\pcbu-unlock-service'
    winServiceEnableLoopbackApi = ($Mode -eq 'rest')
    winServiceLoopbackApiToken = $ApiToken
    winWaitForKeyPress = $false
    winHidePasswordField = $false
    unixSetPasswordPAM = $false
} | ConvertTo-Json -Depth 4

Set-Content -Path $settingsPath -Value $settings -Encoding UTF8

$serviceProcess = Start-Process -FilePath $ServiceExe -PassThru

try {
    Start-Sleep -Seconds 2

    if ($Mode -eq 'rest') {
        & $UnlockCtlExe rest-ping $ApiToken
    }
    else {
        & $UnlockCtlExe ping
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Smoke test failed for mode '$Mode'."
    }
}
finally {
    if ($serviceProcess -and !$serviceProcess.HasExited) {
        Stop-Process -Id $serviceProcess.Id -Force
    }
}
