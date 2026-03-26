#ifndef PCBU_DESKTOP_NAMEDPIPESERVER_WIN_H
#define PCBU_DESKTOP_NAMEDPIPESERVER_WIN_H

#ifdef WINDOWS

#include <atomic>
#include <functional>
#include <string>
#include <thread>

// NEW
class NamedPipeServer {
public:
  NamedPipeServer() = default;
  ~NamedPipeServer();

  bool Start(const std::string &pipeName, const std::function<std::string(const std::string &)> &handler);
  void Stop();
  bool IsRunning() const;

private:
  void ServerThread();
  void ProcessClient(void *pipeHandle) const;

  std::string m_PipeName{};
  std::function<std::string(const std::string &)> m_Handler{};
  std::atomic<bool> m_IsRunning{false};
  std::thread m_ServerThread{};
};

#endif

#endif // PCBU_DESKTOP_NAMEDPIPESERVER_WIN_H
