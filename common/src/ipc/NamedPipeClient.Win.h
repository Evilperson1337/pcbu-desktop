#ifndef PCBU_DESKTOP_NAMEDPIPECLIENT_WIN_H
#define PCBU_DESKTOP_NAMEDPIPECLIENT_WIN_H

#ifdef WINDOWS

#include <chrono>
#include <optional>
#include <string>

// NEW
class NamedPipeClient {
public:
  static std::optional<std::string> SendRequest(const std::string &pipeName, const std::string &request,
                                                std::chrono::milliseconds timeout = std::chrono::seconds(5));

private:
  NamedPipeClient() = default;
};

#endif

#endif // PCBU_DESKTOP_NAMEDPIPECLIENT_WIN_H
