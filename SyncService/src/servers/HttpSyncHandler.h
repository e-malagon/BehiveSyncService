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


#ifndef HTTPSYNCHANDLER_H_
#define SHTTPSYNCHANDLER_H_

#include <crypto/base64.h>
#include <fcgi/FcgiHandler.h>
#include <json/json.hpp>
#include <dao/sql/ConnectionPool.h>
#include <ServiceException.h>
#include <SchemaService.h>

#include <sstream>
#include <functional>
#include <uuid/uuid.h>
#include <nanolog/NanoLog.hpp>
#include <thread>

namespace SyncServer {
namespace Servers {

class HttpSyncHandler {
public:
  HttpSyncHandler(Services::DAO::SQL::ConnectionPool &connectionPool) :
      _connectionPool(connectionPool) {
    std::thread(std::bind(&HttpSyncHandler::refreshAuthToken, this)).detach();
    using namespace std::placeholders;
    _fcgiHandler.addPost("^/api/beehive/(.*)/signin$", false, std::bind(&HttpSyncHandler::signin, this, _1, _2)); // @suppress("Invalid arguments")
  }

  void run(std::string &instance);
  void signin(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);

  void refreshAuthToken();

private:
  Services::DAO::SQL::ConnectionPool &_connectionPool;
  FCGI::FcgiHandler _fcgiHandler;
};

} /* namespace Servers */
} /* namespace SyncServer */

#endif /* HTTPSYNCHANDLER_H_ */
