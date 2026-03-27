# Service and REST Unlock Quick Start

This guide is a concise path for getting the Windows service unlock and loopback REST unlock flow running quickly.

## 1. Build the binaries

Build the service-related targets using [`pkg/build-service-rest.sh`](pkg/build-service-rest.sh:1).

For a full combined service/REST build, use:

- [`BUILD_COMPONENT=all`](pkg/build-service-rest.sh:25)

Relevant targets:

- [`pcbu_unlock_service`](desktop/CMakeLists.txt:165)
- [`pcbu_unlockctl`](desktop/CMakeLists.txt:175)
- [`win-pcbiounlock`](natives/win-pcbiounlock/CMakeLists.txt:7)

## 2. Configure settings

Create or update the settings file at the directory returned by [`AppSettings::GetBaseDir()`](common/src/storage/AppSettings.cpp:18):

- `C:\ProgramData\PCBioUnlock\app_settings.json`

At minimum, for REST testing set:

- [`winServicePipeName`](common/src/storage/AppSettings.h:24)
- [`winServiceLoopbackPort`](common/src/storage/AppSettings.h:23)
- [`winServiceEnableLoopbackApi`](common/src/storage/AppSettings.h:25)
- [`winServiceLoopbackApiToken`](common/src/storage/AppSettings.h:26)

## 3. Start the service host

Run the built service executable handled by [`main()`](desktop/service/src/main.cpp:11).

That starts:

- named pipe service via [`UnlockServiceApp::Start()`](desktop/service/src/UnlockServiceApp.cpp:52)
- optional loopback REST via [`UnlockServiceApp::Start()`](desktop/service/src/UnlockServiceApp.cpp:59)

## 4. Verify named pipe connectivity

Run:

- `pcbu_unlockctl ping`

This uses the named-pipe client path in [`main()`](desktop/tools/unlockctl/src/main.cpp:103).

Expected result:

- a JSON response from [`UnlockServiceApp::HandleRequest()`](desktop/service/src/UnlockServiceApp.cpp:69)
- `result = OK`

## 5. Verify REST connectivity

If REST is enabled, run:

- `pcbu_unlockctl rest-ping <apiToken>`

This uses the loopback REST client path in [`main()`](desktop/tools/unlockctl/src/main.cpp:67).

Expected result:

- successful JSON response through [`GET /api/ping`](desktop/service/src/LoopbackRestServer.cpp:90)

## 6. Create an unlock request

### Named-pipe path

Run:

- `pcbu_unlockctl create <DOMAIN\user> <sessionId>`

### REST path

Run:

- `pcbu_unlockctl rest-create <apiToken> <DOMAIN\user> <sessionId>`

The service will only authorize the active console user/session through [`IsAuthorizedUnlockTarget()`](desktop/service/src/UnlockServiceApp.cpp:22).

Expected response payload fields include:

- `requestId`
- `consumeToken`
- `status`

The consume token comes from [`UnlockRequestManager::CreateRequest()`](desktop/service/src/UnlockRequestManager.cpp:13).

## 7. Poll request status

### Named-pipe path

- `pcbu_unlockctl status <requestId>`

### REST path

- `pcbu_unlockctl rest-status <apiToken> <requestId>`

Statuses are returned from [`UnlockRequestManager::GetStatus()`](desktop/service/src/UnlockRequestManager.cpp:33).

Common states:

- `PENDING`
- `PROCESSING`
- `PROCESSING:<message>`
- `SUCCESS`
- `FAILED:*`
- `CONSUMED`

## 8. Consume an approved unlock payload

Only consume after the request reaches `SUCCESS`.

### Named-pipe path

- `pcbu_unlockctl consume <requestId> <consumeToken>`

### REST path

- `pcbu_unlockctl rest-consume <apiToken> <requestId> <consumeToken>`

Consumption is protected by:

- owner/session checks in [`UnlockServiceApp::HandleRequest()`](desktop/service/src/UnlockServiceApp.cpp:104)
- consume-token validation in [`ConsumeApprovedPassword()`](desktop/service/src/UnlockRequestManager.cpp:71)
- approval expiry in [`IsApprovedPayloadExpiredLocked()`](desktop/service/src/UnlockRequestManager.cpp:84)

## 9. Understand current limitations

The current implementation supports:

- service-hosted unlock orchestration
- named-pipe control path
- loopback REST control path
- provider-side approved password consumption through [`CUnlockCredential::GetSerialization()`](natives/win-pcbiounlock/src/CUnlockCredential.cpp:363)

But it is still an in-progress architecture and should be treated as a staged implementation, not a fully productized remote unlock platform.

## 10. Useful implementation references

- service app: [`UnlockServiceApp`](desktop/service/src/UnlockServiceApp.h:12)
- loopback REST server: [`LoopbackRestServer`](desktop/service/src/LoopbackRestServer.h:10)
- request manager: [`UnlockRequestManager`](desktop/service/src/UnlockRequestManager.h:26)
- CLI: [`main()`](desktop/tools/unlockctl/src/main.cpp:55)
- provider service client: [`CUnlockServiceClient`](natives/win-pcbiounlock/src/CUnlockServiceClient.h:13)
