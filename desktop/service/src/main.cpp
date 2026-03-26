#ifdef WINDOWS

#include <exception>

#include <spdlog/spdlog.h>

#include "UnlockServiceApp.h"
#include "storage/LoggingSystem.h"

// NEW
int main() {
  LoggingSystem::Init("unlock_service", true);
  try {
    auto app = UnlockServiceApp();
    if(!app.Start())
      return 1;
    app.Run();
    app.Stop();
  } catch(const std::exception &ex) {
    spdlog::error("Unlock service exception: {}", ex.what());
    LoggingSystem::Destroy();
    return 1;
  }
  LoggingSystem::Destroy();
  return 0;
}

#endif
