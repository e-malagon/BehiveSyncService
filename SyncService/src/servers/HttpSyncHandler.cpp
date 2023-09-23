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

#include "HttpSyncHandler.h"

#include <crypto/Crypto.h>
#include <config/Config.h>
#include <json/json.hpp>
#include <crypto/jwt.h>
#include <UserService.h>
#include <entities/Node.h>

#include <thread>
#include <chrono>
#include <string>
#include <functional>
#include <curl/curl.h>
#include <nanolog/NanoLog.hpp>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <unistd.h>

extern bool running;

namespace SyncServer {
namespace Servers {

void HttpSyncHandler::run(std::string &instance) {
  int fcgifd = FCGX_OpenSocket("/var/run/syncserver/beehive.sock", 128);
  if (0 < fcgifd) {
    if (chown("/var/run/syncserver/beehive.sock", getuid(), getgrnam("www-data")->gr_gid) == -1) {
      std::cerr << "Unable to set permission for /var/run/syncserver/beehive.sock";
      exit(errno);
    }
  } else {
    std::cerr << "Unable to open /var/run/syncserver/beehive.sock";
    exit(1);
  }
  _fcgiHandler.run(fcgifd);
}

void HttpSyncHandler::signin(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    LOG_DEBUG << "request: " << request.body;
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(0); //TODO _connectionPool.getNamedConnection(request.matches[1]);
    Services::UserService userService(connectionPtr.connection());
    std::unique_ptr<Services::Entities::Node> node = userService.signIn(request.body, "web", "web", "abcd", 1);
    std::string id = std::to_string(node->id());
    std::string key = base64_encode(node->key(), true);
    response.headers.emplace(FCGI::FcgiHandler::ResponseSetCookie, "sessionId=" + id + "_" + key + "; Secure; SameSite=Strict; Path=/api/beehive");
    response.status = 200;
    response.body = "{\"sessionId\":\"" + id + "_" + key + "\"}";
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  std::string *data = (std::string*) userp;
  size_t realsize = size * nmemb;
  data->append((const char*) contents, realsize);
  return realsize;
}

void HttpSyncHandler::refreshAuthToken() {
  std::chrono::system_clock::time_point expires = std::chrono::system_clock::now();
  while (running) {
    if (expires <= std::chrono::system_clock::now()) {
      CURL *curl_handle;
      CURLcode res;
      std::string headers;
      std::string response;
      curl_global_init(CURL_GLOBAL_ALL);
      curl_handle = curl_easy_init();
      curl_easy_setopt(curl_handle, CURLOPT_URL, "https://www.googleapis.com/oauth2/v1/certs");
      curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, WriteMemoryCallback);
      curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &headers);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void* )&response);
      res = curl_easy_perform(curl_handle);
      if (res != CURLE_OK) {
        LOG_ERROR << "curl_easy_perform() failed: " << curl_easy_strerror(res);
        expires = std::chrono::system_clock::now() + std::chrono::seconds(3600);
      } else {
        std::size_t found = headers.find("max-age=");
        if (found != std::string::npos) {
          headers = headers.substr(found += 8, 10);
          expires = std::chrono::system_clock::now() + std::chrono::seconds(std::stoi(headers));
        } else {
          expires = std::chrono::system_clock::now() + std::chrono::seconds(3600);
        }
        std::vector<std::string> keys;
        auto jResponse = nlohmann::json::parse(response);
        for (auto &certificate : jResponse) {
          keys.push_back(certificate.get<std::string>());
        }
        Services::UserService::setGoogleRSARS256PubKeys(keys);
      }
      curl_easy_cleanup(curl_handle);
    }
    std::this_thread::sleep_until(expires);
  }
  curl_global_cleanup();
}

} /* namespace Servers */
} /* namespace SyncServer */
