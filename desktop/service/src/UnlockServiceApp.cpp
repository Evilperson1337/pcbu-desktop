#include "UnlockServiceApp.h"

#ifdef WINDOWS

#include <atomic>
#include <csignal>
#include <thread>

#include <spdlog/spdlog.h>

#include "ipc/UnlockIpcProtocol.h"
#include "storage/AppSettings.h"
namespace {
// NEW
std::atomic<bool> g_Run{true};

// NEW
void HandleSignal(int) {
  g_Run = false;
}
}

// NEW
bool UnlockServiceApp::Start() {
  auto settings = AppSettings::Get();
  auto pipeName = settings.winServicePipeName.empty() ? R"(\\.\pipe\pcbu-unlock-service)" : settings.winServicePipeName;
  spdlog::info("Starting unlock service pipe '{}'...", pipeName);
  return m_PipeServer.Start(pipeName, [this](const std::string &requestBody) { return HandleRequest(requestBody); });
}

// NEW
void UnlockServiceApp::Stop() {
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
      response.payload = {{"serviceMode", "console-scaffold"}};
      return response.ToJson().dump();
    }
    case UnlockIpcCommand::CREATE_UNLOCK_REQUEST: {
      auto targetUser = request->payload.value("targetUser", "");
      auto sessionId = request->payload.value("targetSessionId", 0);
      if(targetUser.empty() || sessionId == 0) {
        return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "targetUser and targetSessionId are required."}.ToJson().dump();
      }

      auto requestId = m_RequestManager.CreateRequest(targetUser, sessionId);

      auto response = UnlockIpcResponse();
      response.result = UnlockIpcResult::PENDING;
      response.message = "Unlock request accepted. // TODO: wire into orchestrator and approval cache.";
      response.payload = {{"requestId", requestId}, {"targetUser", targetUser}, {"targetSessionId", sessionId}, {"status", "PENDING"}};
      return response.ToJson().dump();
    }
    case UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS: {
      auto requestId = request->payload.value("requestId", "");
      if(requestId.empty()) {
        return UnlockIpcResponse{.result = UnlockIpcResult::ERROR, .message = "requestId is required."}.ToJson().dump();
      }

      if(!m_RequestManager.HasRequest(requestId)) {
        return UnlockIpcResponse{.result = UnlockIpcResult::NOT_FOUND, .message = "requestId not found."}.ToJson().dump();
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

      auto password = m_RequestManager.ConsumeApprovedPassword(requestId);
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
