# Service and REST Unlock Guides

This folder contains operational documentation for the Windows service and loopback REST unlock flow introduced around [`pcbu_unlock_service`](desktop/CMakeLists.txt:165), [`pcbu_unlockctl`](desktop/CMakeLists.txt:175), and [`win-pcbiounlock`](natives/win-pcbiounlock/CMakeLists.txt:7).

Available guides:

- setup guide: [`service-rest-setup.md`](docs/service-rest-setup.md)
- configuration guide: [`service-rest-configuration.md`](docs/service-rest-configuration.md)
- quick-start guide: [`service-rest-quick-start.md`](docs/service-rest-quick-start.md)

Implementation references:

- service host: [`UnlockServiceApp`](desktop/service/src/UnlockServiceApp.h:12)
- request manager: [`UnlockRequestManager`](desktop/service/src/UnlockRequestManager.h:26)
- loopback REST server: [`LoopbackRestServer`](desktop/service/src/LoopbackRestServer.h:10)
- CLI client: [`main()`](desktop/tools/unlockctl/src/main.cpp:55)
- provider-side service client: [`CUnlockServiceClient`](natives/win-pcbiounlock/src/CUnlockServiceClient.h:13)
