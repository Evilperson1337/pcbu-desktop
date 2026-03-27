#ifndef PCBU_DESKTOP_UNLOCKSERVICEAPP_H
#define PCBU_DESKTOP_UNLOCKSERVICEAPP_H

#ifdef WINDOWS

#include <string>

#include "ipc/NamedPipeServer.Win.h"
#include "LoopbackRestServer.h"
#include "UnlockRequestManager.h"

// NEW
class UnlockServiceApp {
public:
  bool Start();
  void Stop();
  void Run();

private:
  std::string HandleRequest(const std::string &requestBody);

  NamedPipeServer m_PipeServer{};
  LoopbackRestServer m_LoopbackRestServer{};
  UnlockRequestManager m_RequestManager{};
};

#endif

#endif // PCBU_DESKTOP_UNLOCKSERVICEAPP_H
