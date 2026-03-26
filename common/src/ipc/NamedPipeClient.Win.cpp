#include "NamedPipeClient.Win.h"

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
std::optional<std::string> NamedPipeClient::SendRequest(const std::string &pipeName, const std::string &request, std::chrono::milliseconds timeout) {
  auto pipePath = StringUtils::ToWideString(pipeName);
  if(!WaitNamedPipeW(pipePath.c_str(), static_cast<DWORD>(timeout.count()))) {
    spdlog::error("WaitNamedPipeW() failed. (Code={})", GetLastError());
    return {};
  }

  HANDLE pipe = CreateFileW(pipePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if(pipe == INVALID_HANDLE_VALUE) {
    spdlog::error("CreateFileW() for named pipe failed. (Code={})", GetLastError());
    return {};
  }

  DWORD mode = PIPE_READMODE_BYTE;
  if(!SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr)) {
    spdlog::warn("SetNamedPipeHandleState() failed. (Code={})", GetLastError());
  }

  auto requestSize = static_cast<DWORD>(request.size());
  if(!WriteAll(pipe, &requestSize, sizeof(requestSize)) || !WriteAll(pipe, request.data(), requestSize)) {
    spdlog::error("Failed writing named pipe request. (Code={})", GetLastError());
    CloseHandle(pipe);
    return {};
  }

  DWORD responseSize{};
  if(!ReadAll(pipe, &responseSize, sizeof(responseSize))) {
    spdlog::error("Failed reading named pipe response size. (Code={})", GetLastError());
    CloseHandle(pipe);
    return {};
  }
  std::string response(responseSize, '\0');
  if(responseSize > 0 && !ReadAll(pipe, response.data(), responseSize)) {
    spdlog::error("Failed reading named pipe response body. (Code={})", GetLastError());
    CloseHandle(pipe);
    return {};
  }

  FlushFileBuffers(pipe);
  CloseHandle(pipe);
  return response;
}

#endif
