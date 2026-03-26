#include "NamedPipeServer.Win.h"

#ifdef WINDOWS

#include <Windows.h>
#include <spdlog/spdlog.h>

#include "utils/StringUtils.h"

namespace {
// NEW
bool WriteAll(HANDLE pipe, const void *data, DWORD size) {
  auto offset = 0u;
  while(offset < size) {
    DWORD written{};
    if(!WriteFile(pipe, reinterpret_cast<const char *>(data) + offset, size - offset, &written, nullptr))
      return false;
    offset += written;
  }
  return true;
}

// NEW
bool ReadAll(HANDLE pipe, void *data, DWORD size) {
  auto offset = 0u;
  while(offset < size) {
    DWORD read{};
    if(!ReadFile(pipe, reinterpret_cast<char *>(data) + offset, size - offset, &read, nullptr))
      return false;
    if(read == 0)
      return false;
    offset += read;
  }
  return true;
}
}

// NEW
NamedPipeServer::~NamedPipeServer() {
  Stop();
}

// NEW
bool NamedPipeServer::Start(const std::string &pipeName, const std::function<std::string(const std::string &)> &handler) {
  if(m_IsRunning)
    return true;
  m_PipeName = pipeName;
  m_Handler = handler;
  m_IsRunning = true;
  m_ServerThread = std::thread(&NamedPipeServer::ServerThread, this);
  return true;
}

// NEW
void NamedPipeServer::Stop() {
  if(!m_IsRunning)
    return;
  m_IsRunning = false;
  auto pipePath = StringUtils::ToWideString(m_PipeName);
  auto wakeHandle = CreateFileW(pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if(wakeHandle != INVALID_HANDLE_VALUE)
    CloseHandle(wakeHandle);
  if(m_ServerThread.joinable())
    m_ServerThread.join();
}

// NEW
bool NamedPipeServer::IsRunning() const {
  return m_IsRunning;
}

// NEW
void NamedPipeServer::ServerThread() {
  auto pipePath = StringUtils::ToWideString(m_PipeName);
  while(m_IsRunning) {
    HANDLE pipe = CreateNamedPipeW(pipePath.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                   PIPE_UNLIMITED_INSTANCES, 64 * 1024, 64 * 1024, 0, nullptr);
    if(pipe == INVALID_HANDLE_VALUE) {
      spdlog::error("CreateNamedPipeW() failed. (Code={})", GetLastError());
      break;
    }

    BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if(connected) {
      ProcessClient(pipe);
    }

    FlushFileBuffers(pipe);
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
  }
}

// NEW
void NamedPipeServer::ProcessClient(void *pipeHandle) const {
  auto pipe = static_cast<HANDLE>(pipeHandle);
  DWORD requestSize{};
  if(!ReadAll(pipe, &requestSize, sizeof(requestSize))) {
    spdlog::warn("Failed reading pipe request size. (Code={})", GetLastError());
    return;
  }

  std::string request(requestSize, '\0');
  if(requestSize > 0 && !ReadAll(pipe, request.data(), requestSize)) {
    spdlog::warn("Failed reading pipe request body. (Code={})", GetLastError());
    return;
  }

  auto response = m_Handler ? m_Handler(request) : std::string();
  auto responseSize = static_cast<DWORD>(response.size());
  if(!WriteAll(pipe, &responseSize, sizeof(responseSize)) || (responseSize > 0 && !WriteAll(pipe, response.data(), responseSize))) {
    spdlog::warn("Failed writing pipe response. (Code={})", GetLastError());
  }
}

#endif
