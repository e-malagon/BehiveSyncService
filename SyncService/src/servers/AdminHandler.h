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

#ifndef ADMINHANDLER_H_
#define ADMINHANDLER_H_

#include <crypto/base64.h>
#include <fcgi/FcgiHandler.h>
#include <json/json.hpp>
#include <dao/sql/ConnectionPool.h>
#include <ServiceException.h>
#include <SchemaService.h>
#include <UserService.h>

#include <sstream>
#include <functional>
#include <uuid/uuid.h>
#include <nanolog/NanoLog.hpp>

namespace SyncServer {
namespace Servers {

class AdminHandler {
public:

  AdminHandler(Services::DAO::SQL::ConnectionPool &connectionPool) :
      _connectionPool(connectionPool) {
    using namespace std::placeholders;
    _fcgiHandler.addPost("^/api/config/forms/([a-zA-Z]+)$", true, std::bind(&AdminHandler::postForm, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/values/([a-zA-Z]+)$", true, std::bind(&AdminHandler::postValue, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/signup$", false, std::bind(&AdminHandler::signup, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/signin$", false, std::bind(&AdminHandler::signin, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addDelete("^/api/config/signout$", true, std::bind(&AdminHandler::signout, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/users$", true, std::bind(&AdminHandler::getUsers, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/users/([0-9]+)$", true, std::bind(&AdminHandler::getUser, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/users$", true, std::bind(&AdminHandler::saveUser, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPut("^/api/config/users/([0-9]+)$", true, std::bind(&AdminHandler::updateUser, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addDelete("^/api/config/users/([0-9]+)$", true, std::bind(&AdminHandler::removeUser, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/beehive$", true, std::bind(&AdminHandler::getBeehive, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/schemas/(applications)$", true, std::bind(&AdminHandler::postApplication, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/schemas/(entities|transactions|roles|modules|datasets)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::postSchema, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/schemas/(applications)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::getApplication, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/schemas/(entities|transactions|roles|modules|datasets)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::getSchema, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/schemas/(applications)$", true, std::bind(&AdminHandler::getApplications, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/schemas/(entities|transactions|roles|modules|datasets)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::getSchemas, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addDelete("^/api/config/schemas/(applications)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::deleteApplication, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addDelete("^/api/config/schemas/(entities|transactions|roles|modules|datasets)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::deleteSchema, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/data/(applications)$", true, std::bind(&AdminHandler::getApplicationData, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/data/(entities|transactions|roles|modules|datasets)/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::getSchemaData, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/schemas/publish/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::postPublish, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/schemas/downgrade/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::postDowngrade, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/schemas/export/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&AdminHandler::getExport, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addPost("^/api/config/schemas/import$", true, std::bind(&AdminHandler::postImport, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addGet("^/api/config/log$", true, std::bind(&AdminHandler::getLog, this, _1, _2)); // @suppress("Invalid arguments")
    _fcgiHandler.addDelete("^/api/config/log$", true, std::bind(&AdminHandler::deleteLog, this, _1, _2)); // @suppress("Invalid arguments")
  }

  void run(std::string &instance);
  void postForm(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void postValue(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void signup(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void signin(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void signout(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getUsers(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void saveUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void updateUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void removeUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getBeehive(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void postApplication(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void postSchema(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getApplication(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getSchema(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getApplications(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getSchemas(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void deleteApplication(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void deleteSchema(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getApplicationData(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getSchemaData(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void postPublish(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void postDowngrade(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getExport(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void postImport(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void getLog(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);
  void deleteLog(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response);

private:
  Services::DAO::SQL::ConnectionPool &_connectionPool;
  FCGI::FcgiHandler _fcgiHandler;
};

} /* namespace HTTP */
} /* namespace SyncServer */

#endif /* ADMINHANDLER_H_ */
