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


#pragma once

#include <fcgiapp.h>

#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Beehive {
namespace Services {
namespace FCGI {

class AuthenticationException : public std::runtime_error {
   public:
    AuthenticationException(const AuthenticationException &e) : AuthenticationException(e.what(), e._errorCode) {
    }

    AuthenticationException(const std::string &what, unsigned errorCode) : runtime_error(what), _errorCode(errorCode), _what(what) {
    }

    virtual ~AuthenticationException() throw() {
    }

    unsigned errorCode() const {
        return _errorCode;
    }

    virtual const char *what() const noexcept override {
        return _what.c_str();
    }

   private:
    unsigned _errorCode;
    const std::string _what;
};

class FcgiHandler {
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
    const static std::string RequestAuthorization;
    const static std::string RequestLink;

    const static std::string ResponseContentLength;
    const static std::string ResponseContentType;
    const static std::string ResponseContentDisposition;
    const static std::string ResponseSetCookie;
    const static std::string ResponseLocation;
    const static std::string ResponseAllow;

    const static std::string ResponseContentTypeText;
    const static std::string ResponseContentTypeJSON;
    const static std::string ResponseContentTypeOctet;

    const static std::string MethodPost;
    const static std::string MethodGet;
    const static std::string MethodHead;
    const static std::string MethodPut;
    const static std::string MethodDelete;
    const static std::string MethodOptions;
    const static std::string MethodPatch;
    const static std::string MethodLink;
    const static std::string MethodUnlink;

    struct Request {
        std::string method;
        std::string path;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
        std::smatch matches;
        std::unordered_map<std::string, std::string> queryString;
        std::unordered_map<std::string, std::string> cookies;
        std::string user;
        std::string session;
        std::string authorization;
    };

    struct Response {
        int status = -1;
        std::unordered_map<std::string, std::string> headers;
        std::string body;
    };

    void run(int fcgiSocket);
    void addPost(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void addGet(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void addPut(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void addDelete(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void addPatch(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void addLink(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void addUnlink(std::string pattern, bool session, std::function<void(const Request &, Response &)> handler);
    void saveUser(std::string &cookie, std::string &user);
    void removeUser(const std::string &cookie);
    std::unordered_map<std::string, std::string> getValues(const std::string &body);

   private:
    void readHeaders(FCGX_Request &fcgiRequest, std::unordered_map<std::string, std::string> &headers);
    void readBody(FCGX_Request &fcgiRequest, std::string &body, uint64_t len, std::string method);
    void writeResponse(FCGX_Request &fcgiRequest, const Request &request, Response &response, bool onlyHeaders);
    std::unordered_map<std::string, std::string> decodeQueryString(const std::string queryString);
    void dispatchRequest(Request &request, Response &response, std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> &handlers, std::unordered_map<std::string, bool> &sessionMap);
    std::string decode(const std::string &value);
    std::unordered_map<std::string, std::string> getCookies(const std::string &value);
    bool hasHeader(const std::unordered_map<std::string, std::string> &headers, const std::string &key);
    uint64_t getHeaderIntValue(const std::unordered_map<std::string, std::string> &headers, const std::string &key, uint64_t def = 0);
    std::string getHeaderStringValue(const std::unordered_map<std::string, std::string> &headers, const std::string &key, const std::string &def);

    std::unordered_map<std::string, std::string> _users;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _postHandlers;
    std::unordered_map<std::string, bool> _postSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _getHandlers;
    std::unordered_map<std::string, bool> _getSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _putHandlers;
    std::unordered_map<std::string, bool> _putSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _deleteHandlers;
    std::unordered_map<std::string, bool> _deleteSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _patchHandlers;
    std::unordered_map<std::string, bool> _patchSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _linkHandlers;
    std::unordered_map<std::string, bool> _linkSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::function<void(const Request &, Response &)>>> _unlinkHandlers;
    std::unordered_map<std::string, bool> _unlinkSessionMap;
    std::unordered_map<std::string, std::pair<std::regex, std::unordered_set<std::string>>> _allowed;
    std::unordered_set<std::string> _generalAllowed;
};

} /* namespace FCGI */
} /* namespace Services */
} /* namespace Beehive */
