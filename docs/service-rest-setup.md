# Service and REST Unlock Setup Guide

This guide explains how to build and stage the Windows service unlock and loopback REST unlock components.

## Components

The current Windows unlock/service stack is split across three binaries:

- service host: [`pcbu_unlock_service`](desktop/CMakeLists.txt:165)
- CLI / REST test client: [`pcbu_unlockctl`](desktop/CMakeLists.txt:175)
- credential provider companion DLL: [`win-pcbiounlock`](natives/win-pcbiounlock/CMakeLists.txt:7)

The core runtime behavior is implemented in:

- service host startup: [`main()`](desktop/service/src/main.cpp:11)
- service command handling: [`UnlockServiceApp::HandleRequest()`](desktop/service/src/UnlockServiceApp.cpp:69)
- loopback REST listener: [`LoopbackRestServer::Start()`](desktop/service/src/LoopbackRestServer.cpp:59)

## Prerequisites

For Windows builds, the project expects:

- CMake 3.22+
- Visual Studio 2022 build tools
- Qt 6 configured through `QT_BASE_DIR`
- vcpkg configured through `VCPKG_ROOT`

The top-level project enforces the vcpkg requirement in [`CMakeLists.txt`](CMakeLists.txt:22).

## Local build options

### Full desktop + installer build

Use [`pkg/build-desktop.sh`](pkg/build-desktop.sh:1) when you want the full installer/application build.

### Service / REST build path

Use [`pkg/build-service-rest.sh`](pkg/build-service-rest.sh:1) when you want the service/REST/companion artifacts.

Supported selectors in [`BUILD_COMPONENT`](pkg/build-service-rest.sh:25):

- `all`
- `service`
- `rest`
- `companion`

Optional helper flag:

- [`INCLUDE_SMOKE_DEPS`](pkg/build-service-rest.sh:29) adds the extra executable needed for smoke tests without changing the packaged output.

## Example local builds

### Build all service-related artifacts

Set:

- `ARCH=x64` or `ARCH=arm64`
- `QT_BASE_DIR=<your Qt root>`
- `BUILD_COMPONENT=all`

Then run [`pkg/build-service-rest.sh`](pkg/build-service-rest.sh:1).

### Build only the service host bundle

Set:

- `BUILD_COMPONENT=service`

Then run [`pkg/build-service-rest.sh`](pkg/build-service-rest.sh:1).

### Build only the REST client/test bundle

Set:

- `BUILD_COMPONENT=rest`

Then run [`pkg/build-service-rest.sh`](pkg/build-service-rest.sh:1).

### Build only the original companion DLL bundle

Set:

- `BUILD_COMPONENT=companion`

Then run [`pkg/build-service-rest.sh`](pkg/build-service-rest.sh:1).

## Produced artifacts

The packaging script currently emits:

- combined: [`PCBioUnlock-ServiceRest-${ARCH}.zip`](pkg/build-service-rest.sh:78)
- service-only: [`PCBioUnlock-ServiceUnlock-${ARCH}.zip`](pkg/build-service-rest.sh:81)
- REST-only: [`PCBioUnlock-RestUnlock-${ARCH}.zip`](pkg/build-service-rest.sh:84)
- companion-only: [`PCBioUnlock-PCBioUnlock-${ARCH}.zip`](pkg/build-service-rest.sh:87)

## CI workflows

Windows CI/release workflows are split as follows:

- all-in-one: [`build-all.yml`](.github/workflows/build-all.yml)
- service-only: [`build-service.yml`](.github/workflows/build-service.yml)
- REST-only: [`build-rest.yml`](.github/workflows/build-rest.yml)
- companion-only: [`build-companion.yaml`](.github/workflows/build-companion.yaml)

Shared workflow helpers:

- Windows setup: [`setup-windows-build`](.github/actions/setup-windows-build/action.yml)
- archive verification: [`verify-zip-contents`](.github/actions/verify-zip-contents/action.yml)
- runtime smoke test wrapper: [`run-service-rest-smoke`](.github/actions/run-service-rest-smoke/action.yml)
