#ifdef WINDOWS

#include <iostream>

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
}

// NEW
int main(int argc, char *argv[]) {
  if(argc < 2) {
    PrintUsage();
    return 1;
  }

  auto pipeName = AppSettings::Get().winServicePipeName;
  if(pipeName.empty())
    pipeName = R"(\\.\pipe\pcbu-unlock-service)";

  auto request = UnlockIpcRequest();
  auto cmd = std::string(argv[1]);
  if(cmd == "ping") {
    request.command = UnlockIpcCommand::PING;
  } else if(cmd == "create" && argc >= 4) {
    request.command = UnlockIpcCommand::CREATE_UNLOCK_REQUEST;
    request.payload = {{"targetUser", argv[2]}, {"targetSessionId", std::stoi(argv[3])}};
  } else if(cmd == "status" && argc >= 3) {
    request.command = UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS;
    request.payload = {{"requestId", argv[2]}};
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
