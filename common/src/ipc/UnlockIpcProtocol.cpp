#include "UnlockIpcProtocol.h"

#include <spdlog/spdlog.h>

// NEW
std::string UnlockIpcProtocol::CommandToString(UnlockIpcCommand command) {
  switch(command) {
    case UnlockIpcCommand::PING:
      return "PING";
    case UnlockIpcCommand::CREATE_UNLOCK_REQUEST:
      return "CREATE_UNLOCK_REQUEST";
    case UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS:
      return "GET_UNLOCK_REQUEST_STATUS";
    case UnlockIpcCommand::CONSUME_UNLOCK_REQUEST:
      return "CONSUME_UNLOCK_REQUEST";
  }
  return "PING";
}

// NEW
std::optional<UnlockIpcCommand> UnlockIpcProtocol::CommandFromString(const std::string &value) {
  if(value == "PING")
    return UnlockIpcCommand::PING;
  if(value == "CREATE_UNLOCK_REQUEST")
    return UnlockIpcCommand::CREATE_UNLOCK_REQUEST;
  if(value == "GET_UNLOCK_REQUEST_STATUS")
    return UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS;
  if(value == "CONSUME_UNLOCK_REQUEST")
    return UnlockIpcCommand::CONSUME_UNLOCK_REQUEST;
  return {};
}

// NEW
std::string UnlockIpcProtocol::ResultToString(UnlockIpcResult result) {
  switch(result) {
    case UnlockIpcResult::OK:
      return "OK";
    case UnlockIpcResult::PENDING:
      return "PENDING";
    case UnlockIpcResult::ERROR:
      return "ERROR";
    case UnlockIpcResult::UNAUTHORIZED:
      return "UNAUTHORIZED";
    case UnlockIpcResult::NOT_FOUND:
      return "NOT_FOUND";
    case UnlockIpcResult::NOT_IMPLEMENTED:
      return "NOT_IMPLEMENTED";
  }
  return "ERROR";
}

// NEW
std::optional<UnlockIpcResult> UnlockIpcProtocol::ResultFromString(const std::string &value) {
  if(value == "OK")
    return UnlockIpcResult::OK;
  if(value == "PENDING")
    return UnlockIpcResult::PENDING;
  if(value == "ERROR")
    return UnlockIpcResult::ERROR;
  if(value == "UNAUTHORIZED")
    return UnlockIpcResult::UNAUTHORIZED;
  if(value == "NOT_FOUND")
    return UnlockIpcResult::NOT_FOUND;
  if(value == "NOT_IMPLEMENTED")
    return UnlockIpcResult::NOT_IMPLEMENTED;
  return {};
}

// NEW
nlohmann::json UnlockIpcRequest::ToJson() const {
  return {{"version", version}, {"command", UnlockIpcProtocol::CommandToString(command)}, {"payload", payload}};
}

// NEW
std::optional<UnlockIpcRequest> UnlockIpcRequest::FromJson(const std::string &jsonStr) {
  try {
    auto json = nlohmann::json::parse(jsonStr);
    auto command = UnlockIpcProtocol::CommandFromString(json["command"]);
    if(!command.has_value()) {
      spdlog::error("Invalid IPC command '{}'.", json["command"].dump());
      return {};
    }
    auto request = UnlockIpcRequest();
    request.version = json.value("version", 1);
    request.command = command.value();
    request.payload = json.value("payload", nlohmann::json::object());
    return request;
  } catch(const std::exception &ex) {
    spdlog::error("Failed parsing IPC request: {}", ex.what());
    return {};
  }
}

// NEW
nlohmann::json UnlockIpcResponse::ToJson() const {
  return {{"version", version}, {"result", UnlockIpcProtocol::ResultToString(result)}, {"message", message}, {"payload", payload}};
}

// NEW
std::optional<UnlockIpcResponse> UnlockIpcResponse::FromJson(const std::string &jsonStr) {
  try {
    auto json = nlohmann::json::parse(jsonStr);
    auto result = UnlockIpcProtocol::ResultFromString(json["result"]);
    if(!result.has_value()) {
      spdlog::error("Invalid IPC result '{}'.", json["result"].dump());
      return {};
    }
    auto response = UnlockIpcResponse();
    response.version = json.value("version", 1);
    response.result = result.value();
    response.message = json.value("message", "");
    response.payload = json.value("payload", nlohmann::json::object());
    return response;
  } catch(const std::exception &ex) {
    spdlog::error("Failed parsing IPC response: {}", ex.what());
    return {};
  }
}
