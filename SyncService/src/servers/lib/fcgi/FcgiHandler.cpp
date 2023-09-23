/*
Beehive - SQLite synchronization server.

MIT License

Copyright (c) 2021 Edgar Malagón Calderón

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "FcgiHandler.h"

#include <nanolog/NanoLog.hpp>
#include <fcgiapp.h>
#include <cstring>
#include <vector>

extern bool running;

namespace SyncServer {
namespace Servers {
namespace FCGI {

const std::string FcgiHandler::RequestRequestURI("REQUEST_URI");
const std::string FcgiHandler::RequestRequestMethod("REQUEST_METHOD");
const std::string FcgiHandler::RequestContentLength("CONTENT_LENGTH");
const std::string FcgiHandler::RequestContentType("CONTENT_TYPE");
const std::string FcgiHandler::RequestDocumentURI("DOCUMENT_URI");
const std::string FcgiHandler::RequestQueryString("QUERY_STRING");
const std::string FcgiHandler::RequestCookie("HTTP_COOKIE");

const std::string FcgiHandler::ResponseContentLength("Content-Length");
const std::string FcgiHandler::ResponseContentType("Content-Type");
const std::string FcgiHandler::ResponseContentDisposition("Content-Disposition");
const std::string FcgiHandler::ResponseSetCookie("Set-Cookie");
const std::string FcgiHandler::ResponseLocation("Location");

const std::string FcgiHandler::ResponseContentTypeText("text/plain; charset=utf-8");
const std::string FcgiHandler::ResponseContentTypeJSON("application/json; charset=utf-8");
const std::string FcgiHandler::ResponseContentTypeOctet("application/octet-stream");

const std::string FcgiHandler::MethodGet("GET");
const std::string FcgiHandler::MethodHead("HEAD");
const std::string FcgiHandler::MethodPost("POST");
const std::string FcgiHandler::MethodPut("PUT");
const std::string FcgiHandler::MethodDelete("DELETE");
const std::string FcgiHandler::MethodOptions("OPTIONS");
const std::string FcgiHandler::MethodPatch("PATCH");

void FcgiHandler::run(int fcgifd) {
  FCGX_Init();
  FCGX_Request fcgiRequest;
  FCGX_InitRequest(&fcgiRequest, fcgifd, 0);
  LOG_DEBUG << "Waiting for income messages";
  while (running) {
    FCGX_Accept_r(&fcgiRequest);
    Request request;
    Response response;
    readHeaders(fcgiRequest, request.headers);
    readBody(fcgiRequest, request.body, getHeaderIntValue(request.headers, RequestContentLength, 0), request.method);
    request.method = getHeaderStringValue(request.headers, RequestRequestMethod, "");
    request.path = getHeaderStringValue(request.headers, RequestDocumentURI, "");
    request.queryString = decodeQueryString(getHeaderStringValue(request.headers, RequestQueryString, ""));
    request.cookies = getCookies(getHeaderStringValue(request.headers, RequestCookie, ""));

    response.status = 405;
    if (request.method == MethodGet || request.method == MethodHead) {
      dispatchRequest(request, response, _getHandlers, _getSessionMap);
    } else if (request.method == MethodPost) {
      dispatchRequest(request, response, _postHandlers, _postSessionMap);
    } else if (request.method == MethodPut) {
      dispatchRequest(request, response, _putHandlers, _putSessionMap);
    } else if (request.method == MethodDelete) {
      dispatchRequest(request, response, _deleteHandlers, _deleteSessionMap);
    } else if (request.method == MethodOptions) {

    } else if (request.method == MethodPatch) {

    }
    writeResponse(fcgiRequest, request, response);
    FCGX_Finish_r(&fcgiRequest);
  }
}

void FcgiHandler::addGet(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler) {
  _getHandlers.emplace(pattern, std::make_pair(std::regex(pattern), handler));
  _getSessionMap.emplace(pattern, session);
}

void FcgiHandler::addPost(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler) {
  _postHandlers.emplace(pattern, std::make_pair(std::regex(pattern), handler));
  _postSessionMap.emplace(pattern, session);
}

void FcgiHandler::addPut(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler) {
  _putHandlers.emplace(pattern, std::make_pair(std::regex(pattern), handler));
  _putSessionMap.emplace(pattern, session);
}

void FcgiHandler::addDelete(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler) {
  _deleteHandlers.emplace(pattern, std::make_pair(std::regex(pattern), handler));
  _deleteSessionMap.emplace(pattern, session);
}

bool FcgiHandler::hasHeader(const Headers &headers, const std::string &key) {
  return headers.find(key) != headers.end();
}

uint64_t FcgiHandler::getHeaderIntValue(const Headers &headers, const std::string &key, uint64_t def) {
  auto it = headers.find(key);
  if (it != headers.end()) {
    return std::strtoull(it->second.data(), nullptr, 10);
  }
  return def;
}

std::string FcgiHandler::getHeaderStringValue(const Headers &headers, const std::string &key, const std::string &def) {
  auto it = headers.find(key);
  if (it != headers.end()) {
    return it->second.c_str();
  }
  return def;
}

void FcgiHandler::saveBeehive(std::string &cookie, uint32_t beehive) {
  _beehives.emplace(cookie, beehive);
}

void FcgiHandler::saveUser(std::string &cookie, std::string &user) {
  _users.emplace(cookie, user);
}

void FcgiHandler::removeBeehive(const std::string &cookie) {
  auto sessionPtr = _beehives.find(cookie);
  if(sessionPtr != _beehives.end()) {
    _beehives.erase(sessionPtr);
  }
}

void FcgiHandler::removeUser(const std::string &cookie) {
  auto sessionPtr = _users.find(cookie);
  if(sessionPtr != _users.end()) {
    _users.erase(sessionPtr);
  }
}

void FcgiHandler::readHeaders(FCGX_Request &fcgiRequest, Headers &headers) {
  char *header;

  if ((header = FCGX_GetParam(RequestRequestURI.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestRequestURI, header);
  }
  if ((header = FCGX_GetParam(RequestRequestMethod.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestRequestMethod, header);
  }
  if ((header = FCGX_GetParam("HTTP_CONTENT_LENGTH", fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestContentLength, header);
  } else if ((header = FCGX_GetParam(RequestContentLength.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestContentLength, header);
  }
  if ((header = FCGX_GetParam(RequestContentType.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestContentType, header);
  }
  if ((header = FCGX_GetParam(RequestDocumentURI.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestDocumentURI, header);
  }
  if ((header = FCGX_GetParam(RequestQueryString.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestQueryString, header);
  }
  if ((header = FCGX_GetParam(RequestCookie.c_str(), fcgiRequest.envp)) != NULL) {
    headers.emplace(RequestCookie, header);
  }
}

void FcgiHandler::readBody(FCGX_Request &fcgiRequest, std::string &body, uint64_t len, std::string method) {
  if (len != 0) {
    std::vector<unsigned char> buffer(len);
    FCGX_GetStr((char*) &buffer[0], len, fcgiRequest.in);
    body = std::string(buffer.begin(), buffer.end());
  }
}

void FcgiHandler::writeResponse(FCGX_Request &fcgiRequest, const Request &request, Response &response) {
  if (response.status != 200) {
    FCGX_FPrintF(fcgiRequest.out, "Status: %d\r\n", response.status);
  }
  if (!hasHeader(response.headers, ResponseContentType) && !response.body.empty()) {
    FCGX_FPrintF(fcgiRequest.out, "%s: %s\r\n", ResponseContentType.c_str(), ResponseContentTypeText.c_str());
  }
  if (!hasHeader(response.headers, ResponseContentLength) && !response.body.empty()) {
    FCGX_FPrintF(fcgiRequest.out, "%s: %d\r\n", ResponseContentLength.c_str(), response.body.length());
  }
  for (auto header : response.headers) {
    FCGX_FPrintF(fcgiRequest.out, "%s: %s\r\n", header.first.c_str(), header.second.c_str());
  }
  FCGX_FPrintF(fcgiRequest.out, "\r\n%s", response.body.c_str());
}

std::unordered_map<std::string, std::string> FcgiHandler::decodeQueryString(const std::string queryString) {
  std::unordered_map<std::string, std::string> queryStringMap;
  std::size_t namePos = 0;
  auto nameEndPos = std::string::npos;
  auto valuePos = std::string::npos;
  for (std::size_t c = 0; c < queryString.size(); ++c) {
    if (queryString[c] == '&') {
      auto name = queryString.substr(namePos, (nameEndPos == std::string::npos ? c : nameEndPos) - namePos);
      if (!name.empty()) {
        auto value = valuePos == std::string::npos ? std::string() : queryString.substr(valuePos, c - valuePos);
        std::for_each(name.begin(), name.end(), [](char &c) {
          c = ::tolower(c);
        });
        queryStringMap.emplace(std::move(name), decode(value));
      }
      namePos = c + 1;
      nameEndPos = std::string::npos;
      valuePos = std::string::npos;
    } else if (queryString[c] == '=') {
      nameEndPos = c;
      valuePos = c + 1;
    }
  }
  if (namePos < queryString.size()) {
    auto name = queryString.substr(namePos, nameEndPos - namePos);
    if (!name.empty()) {
      auto value = valuePos >= queryString.size() ? std::string() : queryString.substr(valuePos);
      std::for_each(name.begin(), name.end(), [](char &c) {
        c = ::tolower(c);
      });
      queryStringMap.emplace(std::move(name), decode(value));
    }
  }
  return queryStringMap;
}

void FcgiHandler::dispatchRequest(Request &request, Response &response, std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> &handlers, SessionMap &sessionMap) {
  try {
    for (const auto &handler : handlers) {
      if (std::regex_match(request.path, request.matches, handler.second.first)) {
        if (sessionMap[handler.first]) {
          auto cookiePtr = request.cookies.find("session");
          if (cookiePtr != request.cookies.end()) {
            request.session = cookiePtr->second;
            auto beehivePrt = _beehives.find(request.session);
            auto userPtr = _users.find(request.session);
            if (beehivePrt == _beehives.end() || userPtr == _users.end()) {
              LOG_DEBUG << "Not authorized request redirecting to login";
              response.status = 403;
              return;
            } else {
              request.beehive = beehivePrt->second;
              request.user = userPtr->second;
            }
          } else {
            LOG_DEBUG << "Not authorized request redirecting to login page";
            response.status = 403;
            return;
          }
        } else {
          request.beehive = 0;
          request.user = "";
        }
        handler.second.second(request, response);
        return;
      }
    }
    response.status = 404;
  } catch (const std::exception &ex) {
    response.status = 500;
    response.headers.emplace("EXCEPTION_WHAT", ex.what());
  } catch (...) {
    response.status = 500;
    response.headers.emplace("EXCEPTION_WHAT", "UNKNOWN");
  }
}

std::string FcgiHandler::decode(const std::string &value) {
  std::string result;
  result.reserve(value.size() / 3 + (value.size() % 3));
  for (std::size_t i = 0; i < value.size(); ++i) {
    auto &chr = value[i];
    if (chr == '%' && i + 2 < value.size()) {
      auto hex = value.substr(i + 1, 2);
      auto decodedChr = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
      result += decodedChr;
      i += 2;
    } else if (chr == '+')
      result += ' ';
    else
      result += chr;
  }
  return result;
}

std::unordered_map<std::string, std::string> FcgiHandler::getCookies(const std::string &value) {
  std::unordered_map<std::string, std::string> cookies;
  std::size_t name_start_pos = std::string::npos;
  std::size_t name_end_pos = std::string::npos;
  std::size_t value_start_pos = std::string::npos;
  for (std::size_t c = 0; c < value.size(); ++c) {
    if (name_start_pos == std::string::npos) {
      if (value[c] != ' ' && value[c] != ';')
        name_start_pos = c;
    } else {
      if (name_end_pos == std::string::npos) {
        if (value[c] == ';') {
          cookies.emplace(value.substr(name_start_pos, c - name_start_pos), std::string());
          name_start_pos = std::string::npos;
        } else if (value[c] == '=')
          name_end_pos = c;
      } else {
        if (value_start_pos == std::string::npos) {
          if (value[c] == '"' && c + 1 < value.size())
            value_start_pos = c + 1;
          else
            value_start_pos = c;
        } else if (value[c] == '"' || value[c] == ';') {
          cookies.emplace(value.substr(name_start_pos, name_end_pos - name_start_pos), decode(value.substr(value_start_pos, c - value_start_pos)));
          name_start_pos = std::string::npos;
          name_end_pos = std::string::npos;
          value_start_pos = std::string::npos;
        }
      }
    }
  }
  if (name_start_pos != std::string::npos) {
    if (name_end_pos == std::string::npos)
      cookies.emplace(value.substr(name_start_pos), std::string());
    else if (value_start_pos != std::string::npos) {
      if (value.back() == '"')
        cookies.emplace(value.substr(name_start_pos, name_end_pos - name_start_pos), decode(value.substr(value_start_pos, value.size() - 1)));
      else
        cookies.emplace(value.substr(name_start_pos, name_end_pos - name_start_pos), decode(value.substr(value_start_pos)));
    }
  }
  return cookies;
}

FcgiHandler::Values FcgiHandler::getValues(const std::string &body) {
  Values values;
  std::size_t name_start_pos = std::string::npos;
  std::size_t name_end_pos = std::string::npos;
  std::size_t value_start_pos = std::string::npos;
  for (std::size_t c = 0; c < body.size(); ++c) {
    if (name_start_pos == std::string::npos) {
      if (body[c] != ' ' && body[c] != '&')
        name_start_pos = c;
    } else {
      if (name_end_pos == std::string::npos) {
        if (body[c] == '&') {
          values.emplace(body.substr(name_start_pos, c - name_start_pos), std::string());
          name_start_pos = std::string::npos;
        } else if (body[c] == '=')
          name_end_pos = c;
      } else {
        if (value_start_pos == std::string::npos) {
          if (body[c] == '"' && c + 1 < body.size())
            value_start_pos = c + 1;
          else
            value_start_pos = c;
        } else if (body[c] == '"' || body[c] == '&') {
          values.emplace(body.substr(name_start_pos, name_end_pos - name_start_pos), decode(body.substr(value_start_pos, c - value_start_pos)));
          name_start_pos = std::string::npos;
          name_end_pos = std::string::npos;
          value_start_pos = std::string::npos;
        }
      }
    }
  }
  if (name_start_pos != std::string::npos) {
    if (name_end_pos == std::string::npos)
      values.emplace(body.substr(name_start_pos), std::string());
    else if (value_start_pos != std::string::npos) {
      if (body.back() == '"')
        values.emplace(body.substr(name_start_pos, name_end_pos - name_start_pos), decode(body.substr(value_start_pos, body.size() - 1)));
      else
        values.emplace(body.substr(name_start_pos, name_end_pos - name_start_pos), decode(body.substr(value_start_pos)));
    }
  }
  return values;
}
} /* namespace FCGI */
} /* namespace Servers */
} /* namespace SyncServer */
