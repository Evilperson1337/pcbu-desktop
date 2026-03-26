#ifndef PCBU_DESKTOP_UNLOCKIPCPROTOCOL_H
#define PCBU_DESKTOP_UNLOCKIPCPROTOCOL_H

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// NEW
enum class UnlockIpcCommand {
  PING,
  CREATE_UNLOCK_REQUEST,
  GET_UNLOCK_REQUEST_STATUS,
  CONSUME_UNLOCK_REQUEST,
};

// NEW
enum class UnlockIpcResult {
  OK,
  PENDING,
  ERROR,
  UNAUTHORIZED,
  NOT_FOUND,
  NOT_IMPLEMENTED,
};

// NEW
struct UnlockIpcRequest {
  int version{1};
  UnlockIpcCommand command{UnlockIpcCommand::PING};
  nlohmann::json payload{};

  nlohmann::json ToJson() const;
  static std::optional<UnlockIpcRequest> FromJson(const std::string &jsonStr);
};

// NEW
struct UnlockIpcResponse {
  int version{1};
  UnlockIpcResult result{UnlockIpcResult::ERROR};
  std::string message{};
  nlohmann::json payload{};

  nlohmann::json ToJson() const;
  static std::optional<UnlockIpcResponse> FromJson(const std::string &jsonStr);
};

// NEW
class UnlockIpcProtocol {
public:
  static std::string CommandToString(UnlockIpcCommand command);
  static std::optional<UnlockIpcCommand> CommandFromString(const std::string &value);

  static std::string ResultToString(UnlockIpcResult result);
  static std::optional<UnlockIpcResult> ResultFromString(const std::string &value);

private:
  UnlockIpcProtocol() = default;
};

#endif // PCBU_DESKTOP_UNLOCKIPCPROTOCOL_H
