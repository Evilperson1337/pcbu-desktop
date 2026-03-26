#ifndef PCBU_DESKTOP_CUNLOCKSERVICECLIENT_H
#define PCBU_DESKTOP_CUNLOCKSERVICECLIENT_H

#include <optional>
#include <string>

// NEW
class CUnlockServiceClient {
public:
  bool Ping() const;
  std::optional<std::string> CreateUnlockRequest(const std::wstring &qualifiedUserName, unsigned long sessionId) const;
  std::optional<std::string> GetRequestStatus(const std::string &requestId) const;
  std::optional<std::string> ConsumeApprovedPassword(const std::string &requestId) const;
};

#endif // PCBU_DESKTOP_CUNLOCKSERVICECLIENT_H
