#ifdef WINDOWS

#include <iostream>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "ipc/NamedPipeClient.Win.h"
#include "ipc/UnlockIpcProtocol.h"
#include "storage/AppSettings.h"

namespace {
// NEW
void PrintUsage() {
  std::cout << "Usage:\n";
  std::cout << "  pcbu_unlockctl ping\n";
  std::cout << "  pcbu_unlockctl create <DOMAIN\\user> <sessionId>\n";
  std::cout << "  pcbu_unlockctl status <requestId>\n";
  std::cout << "  pcbu_unlockctl consume <requestId> <consumeToken>\n";
  std::cout << "  pcbu_unlockctl rest-ping <apiToken>\n";
  std::cout << "  pcbu_unlockctl rest-create <apiToken> <DOMAIN\\user> <sessionId>\n";
  std::cout << "  pcbu_unlockctl rest-status <apiToken> <requestId>\n";
  std::cout << "  pcbu_unlockctl rest-consume <apiToken> <requestId> <consumeToken>\n";
}

// NEW
int PrintResponse(const std::string &responseStr) {
  auto response = UnlockIpcResponse::FromJson(responseStr);
  if(!response.has_value()) {
    std::cerr << "Invalid response payload.\n";
    return 1;
  }
  std::cout << response->ToJson().dump(2) << std::endl;
  return response->result == UnlockIpcResult::ERROR || response->result == UnlockIpcResult::UNAUTHORIZED ? 1 : 0;
}

int PrintHttpResponse(const httplib::Result &result) {
  if(!result) {
    std::cerr << "Failed to communicate with unlock service REST API.\n";
    return 1;
  }
  std::cout << result->body << std::endl;
  return result->status >= 200 && result->status < 300 ? 0 : 1;
}

httplib::Client CreateRestClient(const PCBUAppStorage &settings) {
  auto client = httplib::Client("127.0.0.1", settings.winServiceLoopbackPort);
  client.set_connection_timeout(5, 0);
  client.set_read_timeout(5, 0);
  client.set_write_timeout(5, 0);
  return client;
}
}

// NEW
int main(int argc, char *argv[]) {
  if(argc < 2) {
    PrintUsage();
    return 1;
  }

  auto pipeName = AppSettings::Get().winServicePipeName;
  auto settings = AppSettings::Get();
  if(pipeName.empty())
    pipeName = R"(\\.\pipe\pcbu-unlock-service)";

  auto cmd = std::string(argv[1]);
  if(cmd.rfind("rest-", 0) == 0) {
    if(!settings.winServiceEnableLoopbackApi) {
      std::cerr << "Loopback REST API is disabled in app settings.\n";
      return 1;
    }

    auto client = CreateRestClient(settings);
    httplib::Headers headers;
    if(argc >= 3) {
      headers.emplace("X-PCBU-API-Token", argv[2]);
    }

    if(cmd == "rest-ping" && argc >= 3) {
      return PrintHttpResponse(client.Get("/api/ping", headers));
    }
    if(cmd == "rest-create" && argc >= 5) {
      auto body = nlohmann::json{{"targetUser", argv[3]}, {"targetSessionId", std::stoi(argv[4])}}.dump();
      return PrintHttpResponse(client.Post("/api/unlock/requests", headers, body, "application/json"));
    }
    if(cmd == "rest-status" && argc >= 4) {
      auto path = std::string("/api/unlock/requests/") + argv[3];
      return PrintHttpResponse(client.Get(path, headers));
    }
    if(cmd == "rest-consume" && argc >= 5) {
      auto path = std::string("/api/unlock/requests/") + argv[3] + "/consume";
      auto body = nlohmann::json{{"requestId", argv[3]}, {"consumeToken", argv[4]}}.dump();
      return PrintHttpResponse(client.Post(path, headers, body, "application/json"));
    }

    PrintUsage();
    return 1;
  }

  auto request = UnlockIpcRequest();
  if(cmd == "ping") {
    request.command = UnlockIpcCommand::PING;
  } else if(cmd == "create" && argc >= 4) {
    request.command = UnlockIpcCommand::CREATE_UNLOCK_REQUEST;
    request.payload = {{"targetUser", argv[2]}, {"targetSessionId", std::stoi(argv[3])}};
  } else if(cmd == "status" && argc >= 3) {
    request.command = UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS;
    request.payload = {{"requestId", argv[2]}};
  } else if(cmd == "consume" && argc >= 4) {
    request.command = UnlockIpcCommand::CONSUME_UNLOCK_REQUEST;
    request.payload = {{"requestId", argv[2]}, {"consumeToken", argv[3]}};
  } else {
    PrintUsage();
    return 1;
  }

  auto response = NamedPipeClient::SendRequest(pipeName, request.ToJson().dump());
  if(!response.has_value()) {
    std::cerr << "Failed to communicate with unlock service.\n";
    return 1;
  }
  return PrintResponse(response.value());
}

#endif
