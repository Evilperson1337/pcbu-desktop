#include "CUnlockServiceClient.h"

#include "ipc/NamedPipeClient.Win.h"
#include "ipc/UnlockIpcProtocol.h"
#include "storage/AppSettings.h"
#include "utils/StringUtils.h"

namespace {
// NEW
std::optional<UnlockIpcResponse> Send(const UnlockIpcRequest &request) {
  auto pipeName = AppSettings::Get().winServicePipeName;
  if(pipeName.empty())
    pipeName = R"(\\.\pipe\pcbu-unlock-service)";
  auto responseStr = NamedPipeClient::SendRequest(pipeName, request.ToJson().dump());
  if(!responseStr.has_value())
    return {};
  return UnlockIpcResponse::FromJson(responseStr.value());
}
}

// NEW
bool CUnlockServiceClient::Ping() const {
  auto request = UnlockIpcRequest();
  request.command = UnlockIpcCommand::PING;
  auto response = Send(request);
  return response.has_value() && response->result == UnlockIpcResult::OK;
}

// NEW
std::optional<std::string> CUnlockServiceClient::CreateUnlockRequest(const std::wstring &qualifiedUserName, unsigned long sessionId) const {
  auto request = UnlockIpcRequest();
  request.command = UnlockIpcCommand::CREATE_UNLOCK_REQUEST;
  request.payload = {{"targetUser", StringUtils::FromWideString(qualifiedUserName)}, {"targetSessionId", sessionId}};
  auto response = Send(request);
  if(!response.has_value())
    return {};
  if(response->result != UnlockIpcResult::PENDING && response->result != UnlockIpcResult::OK)
    return {};
  auto requestId = response->payload.value("requestId", std::string());
  if(requestId.empty())
    return {};
  return requestId;
}

// NEW
std::optional<std::string> CUnlockServiceClient::GetRequestStatus(const std::string &requestId) const {
  auto request = UnlockIpcRequest();
  request.command = UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS;
  request.payload = {{"requestId", requestId}};
  auto response = Send(request);
  if(!response.has_value())
    return {};
  auto status = response->payload.value("status", std::string());
  if(status.empty())
    return {};
  return status;
}

// NEW
std::optional<std::string> CUnlockServiceClient::ConsumeApprovedPassword(const std::string &requestId) const {
  auto request = UnlockIpcRequest();
  request.command = UnlockIpcCommand::CONSUME_UNLOCK_REQUEST;
  request.payload = {{"requestId", requestId}};
  auto response = Send(request);
  if(!response.has_value() || response->result != UnlockIpcResult::OK)
    return {};
  auto password = response->payload.value("password", std::string());
  if(password.empty())
    return {};
  return password;
}
