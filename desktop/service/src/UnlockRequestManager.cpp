#include "UnlockRequestManager.h"

#ifdef WINDOWS

#include <thread>

#include <spdlog/spdlog.h>

#include "utils/StringUtils.h"

// NEW
std::string UnlockRequestManager::CreateRequest(const std::string &targetUser, unsigned long sessionId) {
  auto requestId = StringUtils::RandomString(32);
  {
    std::lock_guard lock(m_Mutex);
    auto request = UnlockServiceRequest();
    request.id = requestId;
    request.targetUser = targetUser;
    request.sessionId = sessionId;
    m_RequestState.emplace(requestId, std::move(request));
  }
  std::thread(&UnlockRequestManager::ProcessRequest, this, requestId).detach();
  return requestId;
}

// NEW
std::string UnlockRequestManager::GetStatus(const std::string &requestId) const {
  std::lock_guard lock(m_Mutex);
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return {};
  return it->second.status;
}

// NEW
bool UnlockRequestManager::HasRequest(const std::string &requestId) const {
  std::lock_guard lock(m_Mutex);
  return m_RequestState.contains(requestId);
}

// NEW
std::string UnlockRequestManager::ConsumeApprovedPassword(const std::string &requestId) {
  std::lock_guard lock(m_Mutex);
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return {};
  if(it->second.status != "SUCCESS")
    return {};
  auto password = it->second.password;
  it->second.password.clear();
  it->second.status = "CONSUMED";
  return password;
}

// NEW
void UnlockRequestManager::ProcessRequest(const std::string &requestId) {
  std::string targetUser;
  {
    std::lock_guard lock(m_Mutex);
    auto it = m_RequestState.find(requestId);
    if(it == m_RequestState.end())
      return;
    targetUser = it->second.targetUser;
    it->second.status = "PROCESSING";
  }

  std::function<void(const std::string &)> printMessage = [this, requestId](const std::string &message) {
    std::lock_guard lock(m_Mutex);
    auto it = m_RequestState.find(requestId);
    if(it != m_RequestState.end())
      it->second.status = std::string("PROCESSING:") + message;
  };

  auto handler = UnlockHandler(printMessage);
  std::atomic<bool> isRunning(true);
  auto result = handler.GetResult(targetUser, "Windows-Login-Service", &isRunning);

  std::lock_guard lock(m_Mutex);
  auto it = m_RequestState.find(requestId);
  if(it == m_RequestState.end())
    return;
  if(result.state == UnlockState::SUCCESS) {
    it->second.password = result.password;
    it->second.status = "SUCCESS";
  } else {
    it->second.status = std::string("FAILED:") + UnlockStateUtils::ToString(result.state);
  }
}

#endif
