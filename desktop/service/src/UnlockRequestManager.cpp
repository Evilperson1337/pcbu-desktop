#include "UnlockRequestManager.h"

#ifdef WINDOWS

#include <chrono>
#include <thread>

#include <spdlog/spdlog.h>

#include "utils/StringUtils.h"

// NEW
std::string UnlockRequestManager::CreateRequest(const std::string &ownerUser, const std::string &targetUser, unsigned long sessionId) {
  auto requestId = StringUtils::RandomString(32);
  auto now = std::chrono::steady_clock::now();
  {
    std::lock_guard lock(m_Mutex);
    PruneExpiredRequestsLocked(now);
    auto request = UnlockServiceRequest();
    request.id = requestId;
    request.ownerUser = ownerUser;
    request.targetUser = targetUser;
    request.sessionId = sessionId;
    request.consumeToken = StringUtils::RandomString(48);
    request.createdAt = now;
    request.updatedAt = now;
    m_RequestState.emplace(requestId, std::move(request));
  }
  std::thread(&UnlockRequestManager::ProcessRequest, this, requestId).detach();
  return requestId;
}

// NEW
std::string UnlockRequestManager::GetStatus(const std::string &requestId) const {
  std::lock_guard lock(m_Mutex);
  const_cast<UnlockRequestManager *>(this)->PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return {};
  return it->second.status;
}

// NEW
bool UnlockRequestManager::HasRequest(const std::string &requestId) const {
  std::lock_guard lock(m_Mutex);
  const_cast<UnlockRequestManager *>(this)->PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
  return m_RequestState.contains(requestId);
}

// NEW
bool UnlockRequestManager::IsOwnedBy(const std::string &requestId, const std::string &ownerUser, unsigned long sessionId) const {
  std::lock_guard lock(m_Mutex);
  const_cast<UnlockRequestManager *>(this)->PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return false;
  return it->second.ownerUser == ownerUser && it->second.sessionId == sessionId;
}

// NEW
std::string UnlockRequestManager::GetConsumeToken(const std::string &requestId) const {
  std::lock_guard lock(m_Mutex);
  const_cast<UnlockRequestManager *>(this)->PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return {};
  return it->second.consumeToken;
}

// NEW
std::string UnlockRequestManager::ConsumeApprovedPassword(const std::string &requestId, const std::string &consumeToken) {
  std::lock_guard lock(m_Mutex);
  auto now = std::chrono::steady_clock::now();
  PruneExpiredRequestsLocked(now);
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return {};
  if(it->second.consumeToken.empty() || it->second.consumeToken != consumeToken)
    return {};
  if(it->second.status != "SUCCESS")
    return {};
  if(IsApprovedPayloadExpiredLocked(it->second, now)) {
    spdlog::warn("Approved unlock payload for request '{}' expired before consumption.", requestId);
    it->second.password.clear();
    it->second.status = "FAILED:APPROVAL_EXPIRED";
    it->second.updatedAt = now;
    return {};
  }
  auto password = it->second.password;
  it->second.password.clear();
  it->second.consumeToken.clear();
  it->second.status = "CONSUMED";
  it->second.updatedAt = now;
  return password;
}

// NEW
bool UnlockRequestManager::IsApprovedPayloadExpiredLocked(const UnlockServiceRequest &request, std::chrono::steady_clock::time_point now) const {
  if(request.status != "SUCCESS")
    return false;
  if(request.approvedAt == std::chrono::steady_clock::time_point{})
    return true;
  return now - request.approvedAt >= kApprovedPasswordTtl;
}

// NEW
void UnlockRequestManager::PruneExpiredRequestsLocked(std::chrono::steady_clock::time_point now) {
  for(auto it = m_RequestState.begin(); it != m_RequestState.end();) {
    if(IsApprovedPayloadExpiredLocked(it->second, now)) {
      spdlog::info("Expiring approved unlock payload for request '{}' before terminal TTL cleanup.", it->second.id);
      it->second.password.clear();
      it->second.consumeToken.clear();
      it->second.status = "FAILED:APPROVAL_EXPIRED";
      it->second.updatedAt = now;
    }

    const auto isTerminal = it->second.status == "SUCCESS" || it->second.status == "CONSUMED" || it->second.status.starts_with("FAILED:");
    const auto ttl = isTerminal ? kTerminalRequestTtl : kPendingRequestTtl;
    if(now - it->second.updatedAt >= ttl) {
      spdlog::info("Pruning expired unlock request '{}' with status '{}'", it->second.id, it->second.status);
      it = m_RequestState.erase(it);
      continue;
    }
    ++it;
  }
}

// NEW
void UnlockRequestManager::ProcessRequest(const std::string &requestId) {
  std::string targetUser;
  {
    std::lock_guard lock(m_Mutex);
    PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
    auto it = m_RequestState.find(requestId);
    if(it == m_RequestState.end())
      return;
    targetUser = it->second.targetUser;
    it->second.status = "PROCESSING";
    it->second.updatedAt = std::chrono::steady_clock::now();
  }

  std::function<void(const std::string &)> printMessage = [this, requestId](const std::string &message) {
    std::lock_guard lock(m_Mutex);
    PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
    auto it = m_RequestState.find(requestId);
    if(it != m_RequestState.end())
      it->second.status = std::string("PROCESSING:") + message;
    if(it != m_RequestState.end())
      it->second.updatedAt = std::chrono::steady_clock::now();
  };

  auto handler = UnlockHandler(printMessage);
  std::atomic<bool> isRunning(true);
  auto result = handler.GetResult(targetUser, "Windows-Login-Service", &isRunning);

  std::lock_guard lock(m_Mutex);
  PruneExpiredRequestsLocked(std::chrono::steady_clock::now());
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return;
  if(result.state == UnlockState::SUCCESS) {
    it->second.password = result.password;
    it->second.status = "SUCCESS";
    it->second.approvedAt = std::chrono::steady_clock::now();
  } else {
    it->second.status = std::string("FAILED:") + UnlockStateUtils::ToString(result.state);
    it->second.consumeToken.clear();
    it->second.approvedAt = {};
  }
  it->second.updatedAt = std::chrono::steady_clock::now();
}

#endif
