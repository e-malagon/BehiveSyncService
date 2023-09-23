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

#ifndef FCGIHANDLER_H_
#define FCGIHANDLER_H_

#include <unordered_map>
#include <vector>
#include <regex>
#include <fcgiapp.h>

namespace SyncServer {
namespace Servers {
namespace FCGI {

class FcgiHandler {
public:
  struct Request;
  struct Response;
  using SessionMap = std::unordered_map<std::string, bool>;
  using Headers = std::unordered_map<std::string, std::string>;
  using Values = std::unordered_map<std::string, std::string>;
  using Match = std::smatch;
public:
  FcgiHandler() {
  }

  virtual ~FcgiHandler() {
  }

  const static std::string RequestRequestURI;
  const static std::string RequestRequestMethod;
  const static std::string RequestContentLength;
  const static std::string RequestContentType;
  const static std::string RequestDocumentURI;
  const static std::string RequestQueryString;
  const static std::string RequestCookie;

  const static std::string ResponseContentLength;
  const static std::string ResponseContentType;
  const static std::string ResponseContentDisposition;
  const static std::string ResponseSetCookie;
  const static std::string ResponseLocation;

  const static std::string ResponseContentTypeText;
  const static std::string ResponseContentTypeJSON;
  const static std::string ResponseContentTypeOctet;

  const static std::string MethodGet;
  const static std::string MethodHead;
  const static std::string MethodPost;
  const static std::string MethodPut;
  const static std::string MethodDelete;
  const static std::string MethodOptions;
  const static std::string MethodPatch;

  void run(int fcgifd);
  void addGet(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
  void addPost(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
  void addPut(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
  void addDelete(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
  void saveBeehive(std::string &cookie, uint32_t beehive);
  void saveUser(std::string &cookie, std::string &user);
  void removeBeehive(const std::string &cookie);
  void removeUser(const std::string &cookie);
  Values getValues(const std::string &body);

  struct Request {
    std::string method;
    std::string path;
    Headers headers;
    std::string body;
    Match matches;
    std::unordered_map<std::string, std::string> queryString;
    std::unordered_map<std::string, std::string> cookies;
    uint32_t beehive;
    std::string user;
    std::string session;
  };

  struct Response {
    int status = -1;
    Headers headers;
    std::string body;
  };

private:

  void readHeaders(FCGX_Request &fcgiRequest, Headers &headers);
  void readBody(FCGX_Request &fcgiRequest, std::string &body, uint64_t len, std::string method);
  void writeResponse(FCGX_Request &fcgiRequest, const Request &request, Response &response);
  std::unordered_map<std::string, std::string> decodeQueryString(const std::string queryString);
  void dispatchRequest(Request &request, Response &response, std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> &handlers, SessionMap &sessionMap);
  std::string decode(const std::string &value);
  std::unordered_map<std::string, std::string> getCookies(const std::string &value);
  bool hasHeader(const Headers &headers, const std::string &key);
  uint64_t getHeaderIntValue(const Headers &headers, const std::string &key, uint64_t def = 0);
  std::string getHeaderStringValue(const Headers &headers, const std::string &key, const std::string &def);

  std::unordered_map<std::string, uint32_t> _beehives;
  std::unordered_map<std::string, std::string> _users;
  std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _postHandlers;
  SessionMap _postSessionMap;
  std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _getHandlers;
  SessionMap _getSessionMap;
  std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _putHandlers;
  SessionMap _putSessionMap;
  std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _deleteHandlers;
  SessionMap _deleteSessionMap;
};

} /* namespace FCGI */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* FCGIHANDLER_H_ */
