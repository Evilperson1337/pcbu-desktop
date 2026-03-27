#ifndef PCBU_DESKTOP_LOOPBACKRESTSERVER_H
#define PCBU_DESKTOP_LOOPBACKRESTSERVER_H

#ifdef WINDOWS

#include <functional>
#include <string>
#include <thread>

class LoopbackRestServer {
public:
  LoopbackRestServer() = default;
  ~LoopbackRestServer();

  bool Start(uint16_t port, const std::string &apiToken, std::function<std::string(const std::string &)> requestHandler);
  void Stop();

private:
  void ServerThread(uint16_t port);

  std::function<std::string(const std::string &)> m_RequestHandler{};
  std::string m_ApiToken{};
  std::thread m_ServerThread{};
  void *m_ServerHandle{};
};

#endif

#endif // PCBU_DESKTOP_LOOPBACKRESTSERVER_H
