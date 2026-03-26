#ifndef PCBU_DESKTOP_UNLOCKREQUESTMANAGER_H
#define PCBU_DESKTOP_UNLOCKREQUESTMANAGER_H

#ifdef WINDOWS

#include <map>
#include <mutex>
#include <string>

#include "handler/UnlockHandler.h"

// NEW
struct UnlockServiceRequest {
  std::string id{};
  std::string targetUser{};
  unsigned long sessionId{};
  std::string status{"PENDING"};
  std::string password{};
};

// NEW
class UnlockRequestManager {
public:
  std::string CreateRequest(const std::string &targetUser, unsigned long sessionId);
  std::string GetStatus(const std::string &requestId) const;
  bool HasRequest(const std::string &requestId) const;
  std::string ConsumeApprovedPassword(const std::string &requestId);

private:
  void ProcessRequest(const std::string &requestId);

  mutable std::mutex m_Mutex{};
  std::map<std::string, UnlockServiceRequest> m_RequestState{};
};

#endif

#endif // PCBU_DESKTOP_UNLOCKREQUESTMANAGER_H
