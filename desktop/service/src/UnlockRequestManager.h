#ifndef PCBU_DESKTOP_UNLOCKREQUESTMANAGER_H
#define PCBU_DESKTOP_UNLOCKREQUESTMANAGER_H

#ifdef WINDOWS

#include <chrono>
#include <map>
#include <mutex>
#include <string>

#include "handler/UnlockHandler.h"

// NEW
struct UnlockServiceRequest {
  std::string id{};
  std::string ownerUser{};
  std::string targetUser{};
  unsigned long sessionId{};
  std::string consumeToken{};
  std::string status{"PENDING"};
  std::string password{};
  std::chrono::steady_clock::time_point createdAt{};
  std::chrono::steady_clock::time_point updatedAt{};
  std::chrono::steady_clock::time_point approvedAt{};
};

// NEW
class UnlockRequestManager {
public:
  std::string CreateRequest(const std::string &ownerUser, const std::string &targetUser, unsigned long sessionId);
  std::string GetStatus(const std::string &requestId) const;
  bool HasRequest(const std::string &requestId) const;
  std::string ConsumeApprovedPassword(const std::string &requestId, const std::string &consumeToken);
  bool IsOwnedBy(const std::string &requestId, const std::string &ownerUser, unsigned long sessionId) const;
  std::string GetConsumeToken(const std::string &requestId) const;

private:
  static constexpr auto kPendingRequestTtl = std::chrono::minutes(5);
  static constexpr auto kTerminalRequestTtl = std::chrono::minutes(1);
  static constexpr auto kApprovedPasswordTtl = std::chrono::seconds(30);

  void ProcessRequest(const std::string &requestId);
  void PruneExpiredRequestsLocked(std::chrono::steady_clock::time_point now);
  bool IsApprovedPayloadExpiredLocked(const UnlockServiceRequest &request, std::chrono::steady_clock::time_point now) const;

  mutable std::mutex m_Mutex{};
  std::map<std::string, UnlockServiceRequest> m_RequestState{};
};

#endif

#endif // PCBU_DESKTOP_UNLOCKREQUESTMANAGER_H
