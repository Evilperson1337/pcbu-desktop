#include "LoopbackRestServer.h"

#ifdef WINDOWS

#include "ipc/UnlockIpcProtocol.h"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace {
std::string MakeJsonError(int status, const std::string &message) {
  auto response = UnlockIpcResponse();
  response.result = status == 403 ? UnlockIpcResult::UNAUTHORIZED : UnlockIpcResult::ERROR;
  response.message = message;
  return response.ToJson().dump();
}

void SetJsonResponse(httplib::Response &res, int status, const std::string &body) {
  res.status = status;
  res.set_content(body, "application/json");
}

bool IsLoopbackRequest(const httplib::Request &req) {
  return req.remote_addr == "127.0.0.1" || req.remote_addr == "::1";
}

bool HasValidApiToken(const httplib::Request &req, const std::string &apiToken) {
  if(apiToken.empty()) {
    return false;
  }

  auto headerToken = req.get_header_value("X-PCBU-API-Token");
  if(!headerToken.empty()) {
    return headerToken == apiToken;
  }

  auto authHeader = req.get_header_value("Authorization");
  constexpr std::string_view bearerPrefix = "Bearer ";
  if(authHeader.starts_with(bearerPrefix)) {
    return authHeader.substr(bearerPrefix.size()) == apiToken;
  }

  return false;
}

std::string BuildCommandBody(UnlockIpcCommand command, const nlohmann::json &payload = {}) {
  auto request = UnlockIpcRequest();
  request.command = command;
  request.payload = payload;
  return request.ToJson().dump();
}
}

LoopbackRestServer::~LoopbackRestServer() {
  Stop();
}

bool LoopbackRestServer::Start(uint16_t port, const std::string &apiToken, std::function<std::string(const std::string &)> requestHandler) {
  Stop();
  m_ApiToken = apiToken;
  m_RequestHandler = std::move(requestHandler);
  m_ServerHandle = new httplib::Server();
  auto *server = static_cast<httplib::Server *>(m_ServerHandle);

  auto ensureAuthorized = [this](const httplib::Request &req, httplib::Response &res) {
    if(!IsLoopbackRequest(req)) {
      SetJsonResponse(res, 403, MakeJsonError(403, "Loopback access only."));
      return false;
    }
    if(!HasValidApiToken(req, m_ApiToken)) {
      SetJsonResponse(res, 401, MakeJsonError(401, "Missing or invalid API token."));
      return false;
    }
    return true;
  };

  auto executeCommand = [this, ensureAuthorized](const httplib::Request &req, httplib::Response &res, UnlockIpcCommand command, const nlohmann::json &payload = {}) {
    if(!ensureAuthorized(req, res)) {
      return;
    }
    if(!m_RequestHandler) {
      SetJsonResponse(res, 500, MakeJsonError(500, "Request handler not configured."));
      return;
    }

    SetJsonResponse(res, 200, m_RequestHandler(BuildCommandBody(command, payload)));
  };

  server->Get("/api/ping", [executeCommand](const httplib::Request &req, httplib::Response &res) {
    executeCommand(req, res, UnlockIpcCommand::PING);
  });

  server->Post("/api/unlock/requests", [executeCommand](const httplib::Request &req, httplib::Response &res) {
    try {
      auto payload = nlohmann::json::parse(req.body);
      executeCommand(req, res, UnlockIpcCommand::CREATE_UNLOCK_REQUEST, payload);
    } catch(const std::exception &) {
      SetJsonResponse(res, 400, MakeJsonError(400, "Invalid JSON request payload."));
    }
  });

  server->Get(R"(/api/unlock/requests/([A-Za-z0-9]+))", [executeCommand](const httplib::Request &req, httplib::Response &res) {
    executeCommand(req, res, UnlockIpcCommand::GET_UNLOCK_REQUEST_STATUS, {{"requestId", req.matches[1].str()}});
  });

  server->Post(R"(/api/unlock/requests/([A-Za-z0-9]+)/consume)", [executeCommand](const httplib::Request &req, httplib::Response &res) {
    try {
      auto payload = nlohmann::json::parse(req.body.empty() ? "{}" : req.body);
      payload["requestId"] = req.matches[1].str();
      executeCommand(req, res, UnlockIpcCommand::CONSUME_UNLOCK_REQUEST, payload);
    } catch(const std::exception &) {
      SetJsonResponse(res, 400, MakeJsonError(400, "Invalid JSON request payload."));
    }
  });

  server->Post("/api/unlock", [this, ensureAuthorized](const httplib::Request &req, httplib::Response &res) {
    if(!ensureAuthorized(req, res)) {
      return;
    }
    if(!m_RequestHandler) {
      SetJsonResponse(res, 500, MakeJsonError(500, "Request handler not configured."));
      return;
    }
    SetJsonResponse(res, 200, m_RequestHandler(req.body));
  });

  server->Post("/api/unlock/command", [this, ensureAuthorized](const httplib::Request &req, httplib::Response &res) {
    if(!ensureAuthorized(req, res)) {
      return;
    }
    if(!m_RequestHandler) {
      SetJsonResponse(res, 500, MakeJsonError(500, "Request handler not configured."));
      return;
    }
    SetJsonResponse(res, 200, m_RequestHandler(req.body));
  });

  server->Get("/api/unlock", [ensureAuthorized](const httplib::Request &req, httplib::Response &res) {
    if(!ensureAuthorized(req, res)) {
      return;
    }
    auto body = nlohmann::json{{"message", "Use POST /api/unlock/requests, GET /api/unlock/requests/{requestId}, POST /api/unlock/requests/{requestId}/consume, or GET /api/ping."}}.dump();
    SetJsonResponse(res, 200, body);
  });

  m_ServerThread = std::thread(&LoopbackRestServer::ServerThread, this, port);
  return true;
}

void LoopbackRestServer::Stop() {
  auto *server = static_cast<httplib::Server *>(m_ServerHandle);
  if(server) {
    server->stop();
  }
  if(m_ServerThread.joinable()) {
    m_ServerThread.join();
  }
  delete server;
  m_ServerHandle = nullptr;
}

void LoopbackRestServer::ServerThread(uint16_t port) {
  auto *server = static_cast<httplib::Server *>(m_ServerHandle);
  if(!server) {
    return;
  }

  spdlog::info("Starting loopback REST server on 127.0.0.1:{}", port);
  if(!server->listen("127.0.0.1", port)) {
    spdlog::error("Failed to start loopback REST server on 127.0.0.1:{}", port);
  }
}

#endif
