# Service and REST Unlock Configuration Guide

This guide covers the runtime configuration currently used by the Windows service unlock and loopback REST unlock features.

## Settings location

On Windows, application settings are stored under the base directory returned by [`AppSettings::GetBaseDir()`](common/src/storage/AppSettings.cpp:18):

- `C:\ProgramData\PCBioUnlock`

The settings file name is [`app_settings.json`](common/src/storage/AppSettings.h:49).

## Relevant settings

The settings structure is defined in [`PCBUAppStorage`](common/src/storage/AppSettings.h:10).

The service/REST-related settings are:

- [`winServiceLoopbackPort`](common/src/storage/AppSettings.h:23)
- [`winServicePipeName`](common/src/storage/AppSettings.h:24)
- [`winServiceEnableLoopbackApi`](common/src/storage/AppSettings.h:25)
- [`winServiceLoopbackApiToken`](common/src/storage/AppSettings.h:26)

## Default values

The current defaults are set in [`AppSettings::Load()`](common/src/storage/AppSettings.cpp:37):

- loopback port: `43297` from [`AppSettings::Load()`](common/src/storage/AppSettings.cpp:109)
- pipe name: `\\.\pipe\pcbu-unlock-service` from [`AppSettings::Load()`](common/src/storage/AppSettings.cpp:110)
- REST disabled by default: [`winServiceEnableLoopbackApi = false`](common/src/storage/AppSettings.cpp:111)
- loopback API token generated automatically: [`StringUtils::RandomString(48)`](common/src/storage/AppSettings.cpp:112)

## How the settings are used

### Named pipe

The service host reads the pipe name in [`UnlockServiceApp::Start()`](desktop/service/src/UnlockServiceApp.cpp:52).

The native client path reads the same setting in [`CUnlockServiceClient`](natives/win-pcbiounlock/src/CUnlockServiceClient.cpp:10).

The CLI also reads the same setting in [`main()`](desktop/tools/unlockctl/src/main.cpp:61).

### Loopback REST

The loopback listener is only started when [`winServiceEnableLoopbackApi`](common/src/storage/AppSettings.h:25) is enabled in [`UnlockServiceApp::Start()`](desktop/service/src/UnlockServiceApp.cpp:59).

The server binds to `127.0.0.1` in [`LoopbackRestServer::ServerThread()`](desktop/service/src/LoopbackRestServer.cpp:163).

REST requests must include the token checked by [`HasValidApiToken()`](desktop/service/src/LoopbackRestServer.cpp:28).

Supported auth headers:

- `X-PCBU-API-Token`
- `Authorization: Bearer <token>`

## Recommended configuration practices

### For named-pipe-only testing

Set:

- [`winServiceEnableLoopbackApi`](common/src/storage/AppSettings.h:25) = `false`

Use [`pcbu_unlockctl ping`](desktop/tools/unlockctl/src/main.cpp:15) to test pipe connectivity.

### For loopback REST testing

Set:

- [`winServiceEnableLoopbackApi`](common/src/storage/AppSettings.h:25) = `true`
- keep a known [`winServiceLoopbackApiToken`](common/src/storage/AppSettings.h:26)

Use [`pcbu_unlockctl rest-ping <apiToken>`](desktop/tools/unlockctl/src/main.cpp:19).

### For CI or scripted smoke tests

The smoke-test harness in [`smoke-test-service-rest.ps1`](pkg/win/smoke-test-service-rest.ps1:1) writes a deterministic settings file under the same settings path and toggles REST enablement based on mode.

## Current authorization model

At this point the service enforces:

- active console user/session validation in [`IsAuthorizedUnlockTarget()`](desktop/service/src/UnlockServiceApp.cpp:22)
- request ownership validation in [`UnlockRequestManager::IsOwnedBy()`](desktop/service/src/UnlockRequestManager.cpp:50)
- approval expiry in [`IsApprovedPayloadExpiredLocked()`](desktop/service/src/UnlockRequestManager.cpp:84)
- per-request consume token validation in [`ConsumeApprovedPassword()`](desktop/service/src/UnlockRequestManager.cpp:71)

This means configuration alone is not enough; the caller must also satisfy the current runtime authorization checks.
