#include "UnlockServiceApp.h"

#ifdef WINDOWS

#include <atomic>
#include <csignal>
#include <thread>

#include <WtsApi32.h>

#include <spdlog/spdlog.h>

#include "ipc/UnlockIpcProtocol.h"
#include "platform/PlatformHelper.h"
#include "storage/AppSettings.h"
namespace {
// NEW
std::atomic<bool> g_Run{true};

// NEW
void HandleSignal(int) {
  g_Run = false;
}

bool IsAuthorizedUnlockTarget(const std::string &targetUser, unsigned long sessionId) {
  auto activeSessionId = WTSGetActiveConsoleSessionId();
  if(activeSessionId == 0xFFFFFFFF) {
    spdlog::warn("Rejecting unlock request because the active console session could not be determined.");
    return false;
  }

  auto currentUser = PlatformHelper::GetCurrentUser();
  if(currentUser.empty()) {
    spdlog::warn("Rejecting unlock request because the active console user could not be determined.");
    return false;
  }

  if(sessionId != activeSessionId) {
    spdlog::warn("Rejecting unlock request for session {} because active console session is {}.", sessionId, activeSessionId);
    return false;
  }

  if(targetUser != currentUser) {
    spdlog::warn("Rejecting unlock request for user '{}' because active console user is '{}'.", targetUser, currentUser);
    return false;
  }

  return true;
}

std::optional<std::pair<std::string, unsigned long>> GetActiveConsoleIdentity() {
  auto activeSessionId = WTSGetActiveConsoleSessionId();
  if(activeSessionId == 0xFFFFFFFF) {
    spdlog::warn("Active console session could not be determined.");
    return {};
  }

  auto currentUser = PlatformHelper::GetCurrentUser();
  if(currentUser.empty()) {
    spdlog::warn("Active console user could not be determined.");
    return {};
  }

  return std::make_pair(currentUser, activeSessionId);
}
}

// NEW
bool UnlockServiceApp::Start() {
  auto settings = AppSettings::Get();
  auto pipeName = settings.winServicePipeName.empty() ? R"(\\.\pipe\pcbu-unlock-service)" : settings.winServicePipeName;
  spdlog::info("Starting unlock service pipe '{}'...", pipeName);
  if(!m_PipeServer.Start(pipeName, [this](const std::string &requestBody) { return HandleRequest(requestBody); })) {
    return false;
  }

  if(settings.winServiceEnableLoopbackApi) {
    m_LoopbackRestServer.Start(settings.winServiceLoopbackPort, settings.winServiceLoopbackApiToken,
                               [this](const std::string &requestBody) { return HandleRequest(requestBody); });
  }

  return true;
}

// NEW
void UnlockServiceApp::Stop() {
  m_LoopbackRestServer.Stop();
  m_PipeServer.Stop();
}

// NEW
void UnlockServiceApp::Run() {
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);
  while(g_Run) {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
}

// NEW
std::string UnlockServiceApp::HandleRequest(const std::string &requestBody) {
  auto request = UnlockIpcRequest::FromJson(requestBody);
  if(!request.has_value()) {
    return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "Invalid request payload."}.ToJson().dump();
  }

  switch(request->command) {
    case UnlockIpcCommand::PING: {
      auto response = UnlockIpcResponse();
      response.result = UnlockIpcResult::OK;
      response.message = "pcbu_unlock_service is online";
      response.payload = {{"serviceMode", "console-scaffold"},
                          {"loopbackApiEnabled", AppSettings::Get().winServiceEnableLoopbackApi},
                          {"loopbackPort", AppSettings::Get().winServiceLoopbackPort},
                          {"loopbackApiTokenConfigured", !AppSettings::Get().winServiceLoopbackApiToken.empty()}};
      return response.ToJson().dump();
    }
    case UnlockIpcCommand::CREATE_UNLOCK_REQUEST: {
      auto targetUser = request->payload.value("targetUser", "");
      auto sessionId = request->payload.value("targetSessionId", 0);
      if(targetUser.empty() || sessionId == 0) {
        return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "targetUser and targetSessionId are required."}.ToJson().dump();
      }

      if(!IsAuthorizedUnlockTarget(targetUser, sessionId)) {
        return UnlockIpcResponse{.result = UnlockIpcResult::UNAUTHORIZED,
                                 .message = "Unlock requests are only allowed for the active console user and session."}
            .ToJson()
            .dump();
      }

      auto requestId = m_RequestManager.CreateRequest(targetUser, targetUser, sessionId);
      auto consumeToken = m_RequestManager.GetConsumeToken(requestId);

      auto response = UnlockIpcResponse();
      response.result = UnlockIpcResult::PENDING;
      response.message = "Unlock request accepted. // TODO: wire into orchestrator and approval cache.";
      response.payload = {{"requestId", requestId},
                          {"consumeToken", consumeToken},
                          {"targetUser", targetUser},
                          {"targetSessionId", sessionId},
                          {"status", "PENDING"}};
      return response.ToJson().dump();
    }
    case UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS: {
      auto requestId = request->payload.value("requestId", "");
      auto consumeToken = request->payload.value("consumeToken", "");
      if(requestId.empty()) {
        return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "requestId is required."}.ToJson().dump();
      }
      if(consumeToken.empty()) {
        return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "consumeToken is required."}.ToJson().dump();
      }

      if(!m_RequestManager.HasRequest(requestId)) {
        return UnlockIpcResponse{.result = UnlockIpcResult::NOT_FOUND, .message = "requestId not found."}.ToJson().dump();
      }

      auto activeIdentity = GetActiveConsoleIdentity();
      if(!activeIdentity.has_value() || !m_RequestManager.IsOwnedBy(requestId, activeIdentity->first, activeIdentity->second)) {
        return UnlockIpcResponse{.result = UnlockIpcResult::UNAUTHORIZED,
                                 .message = "Request status is only available to the active console owner of the request."}
            .ToJson()
            .dump();
      }

      auto status = m_RequestManager.GetStatus(requestId);

      auto response = UnlockIpcResponse();
      response.result = status.starts_with("PENDING") ? UnlockIpcResult::PENDING : UnlockIpcResult::OK;
      response.message = "Status lookup successful.";
      response.payload = {{"requestId", requestId}, {"status", status}};
      return response.ToJson().dump();
    }
    case UnlockIpcCommand::CONSUME_UNLOCK_REQUEST: {
      auto requestId = request->payload.value("requestId", "");
      if(requestId.empty()) {
        return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "requestId is required."}.ToJson().dump();
      }

      auto activeIdentity = GetActiveConsoleIdentity();
      if(!activeIdentity.has_value() || !m_RequestManager.IsOwnedBy(requestId, activeIdentity->first, activeIdentity->second)) {
        return UnlockIpcResponse{.result = UnlockIpcResult::UNAUTHORIZED,
                                 .message = "Request consumption is only available to the active console owner of the request."}
            .ToJson()
            .dump();
      }

      auto password = m_RequestManager.ConsumeApprovedPassword(requestId, consumeToken);
      if(password.empty()) {
        return UnlockIpcResponse{.result = UnlockIpcResult::NOT_FOUND, .message = "Approved unlock payload not available."}.ToJson().dump();
      }

      auto response = UnlockIpcResponse();
      response.result = UnlockIpcResult::OK;
      response.message = "Approved unlock payload consumed.";
      response.payload = {{"requestId", requestId}, {"password", password}};
      return response.ToJson().dump();
    }
  }

  return UnlockIpcResponse{.result = UnlockIpcResult::NOT_IMPLEMENTED, .message = "Command not implemented."}.ToJson().dump();
}

#endif
