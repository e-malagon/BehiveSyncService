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

#include <crypto/base64.h>
#include <uuid/uuid.h>

#include <fcgi/FcgiHandler.hpp>
#include <functional>
#include <json/json.hpp>
#include <nanolog/NanoLog.hpp>
#include <services/SchemaService.hpp>
#include <services/ServiceException.hpp>
#include <services/UserService.hpp>
#include <sstream>

namespace Beehive {
namespace Services {

class InboundHTTP {
   public:
    InboundHTTP() : _fcgiSocket(0) {
    }

    void start();
    void finish();
    void postContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void getContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void getContexts(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void putContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void deleteContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void linkContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void unlinkContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void getLinkedVersions(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void getLinkedVersion(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);

    void postUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void getUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void getUsers(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void putUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void deleteUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);

    void signUp(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void signIn(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void signOut(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
    void signOff(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);

   private:
    FCGI::FcgiHandler _fcgiHandler;
    int _fcgiSocket;
    static std::string _servicePath;
    static std::string _serviceSocket;
    
};

} /* namespace Services */
} /* namespace Beehive */
