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

#include "AdminHandler.h"

#include <dao/ConfigDAO.h>
#include <crypto/Crypto.h>
#include <config/Config.h>
#include <string/ICaseMap.h>
#include <nanolog/NanoLog.hpp>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <errno.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <unistd.h>
#include <map>

extern SyncServer::Servers::Services::Config::Config config;

namespace SyncServer {
namespace Servers {

std::string getDescription(int kind) {
  switch (kind) {
  case 0:
    return "Internal";
  case 1:
    return "Google";
  default:
    return "Unknown";
  }
}

void AdminHandler::run(std::string &instance) {
  int fcgifd = FCGX_OpenSocket("/var/run/syncserver/config.sock", 128);
  if (0 < fcgifd) {
    if (chown("/var/run/syncserver/config.sock", getuid(), getgrnam("www-data")->gr_gid) == -1) {
      std::cerr << "Unable to set permission for /var/run/syncserver/config.sock";
      exit(errno);
    }
  } else {
    std::cerr << "Unable to open /var/run/syncserver/config.sock";
    exit(1);
  }
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(config.db, "SELECT beehive, email, token, last FROM sessions;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    rc = sqlite3_step(stmt);
    while (rc == SQLITE_ROW) {
      uint32_t beehive = sqlite3_column_int(stmt, 0);
      std::string email((const char*) sqlite3_column_text(stmt, 1));
      std::string cookie((const char*) sqlite3_column_text(stmt, 2));
      _fcgiHandler.saveBeehive(cookie, beehive);
      _fcgiHandler.saveUser(cookie, email);
      rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
  }

  _fcgiHandler.run(fcgifd);
}

void AdminHandler::postForm(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    std::string kind(request.matches[1]);
    Services::Config::Form &oldForm = Services::Config::getForm(kind);
    oldForm.form = request.body;
    Services::Config::saveForms();
    response.status = 201;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::postValue(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    std::string kind(request.matches[1]);
    Services::Config::Form &oldForm = Services::Config::getForm(kind);
    oldForm.value = request.body;
    Services::Config::saveForms();
    response.status = 201;
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::signup(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::SchemaService schemaService(_connectionPool.getUnamedConnection(), request.beehive);
    auto values = _fcgiHandler.getValues(request.body);
    auto namePtr = values.find("name");
    auto emailPtr = values.find("email");
    auto passwordPtr = values.find("password");
    if (namePtr != values.end() && emailPtr != values.end() && passwordPtr != values.end()) {
      Services::DAO::ConfigDAO configDAO(config.db);
      std::optional<Services::Config::Config::Developer> developer = configDAO.readDeveloper(emailPtr->second);
      if (developer) {
        response.status = 302;
        response.headers.emplace(FCGI::FcgiHandler::ResponseLocation, "/exists.html");
        return;
      }
      uint32_t beehive;
      uint32_t beehiveName = 0;
      for (int i = 0; i < 1500 && !beehive; i++) {
        beehiveName = Services::Crypto::getBeehiveHash(emailPtr->second + std::to_string(beehiveName));
        if (!Services::Config::alreadyExists(beehiveName, emailPtr->second))
          beehive = beehiveName;
      }
      if (beehive && schemaService.tryCreateBeehive(beehive, namePtr->second, emailPtr->second, passwordPtr->second)) {
        std::string cookie = Services::Crypto::getCookie();
        configDAO.saveSession(beehive, emailPtr->second, cookie, time(NULL));
        _fcgiHandler.saveBeehive(cookie, beehive);
        _fcgiHandler.saveUser(cookie, emailPtr->second);
        response.status = 302;
        response.headers.emplace(FCGI::FcgiHandler::ResponseSetCookie, "session=" + cookie);
        response.headers.emplace(FCGI::FcgiHandler::ResponseLocation, "/applications.html");
      } else {
        response.status = 400;
      }
    } else {
      response.status = 400;
    }
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::signin(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    auto values = _fcgiHandler.getValues(request.body);
    auto emailPtr = values.find("email");
    auto passwordPtr = values.find("password");
    if (emailPtr != values.end() && passwordPtr != values.end()) {
      Services::DAO::ConfigDAO configDAO(config.db);
      std::optional<Services::Config::Config::Developer> developer = configDAO.readDeveloper(emailPtr->second);
      if (developer) {
        std::string password = Services::Crypto::getPasswordHash(passwordPtr->second, developer->salt);
        if (developer->password == password) {
          std::string cookie = Services::Crypto::getCookie();
          configDAO.saveSession(developer->beehive, emailPtr->second, cookie, time(NULL));
          _fcgiHandler.saveBeehive(cookie, developer->beehive);
          _fcgiHandler.saveUser(cookie, emailPtr->second);
          response.status = 302;
          response.headers.emplace(FCGI::FcgiHandler::ResponseSetCookie, "session=" + cookie);
          response.headers.emplace(FCGI::FcgiHandler::ResponseLocation, "/applications.html");
        } else {
          response.status = 400;
        }
      } else {
        response.status = 400;
      }

    } else {
      response.status = 400;
    }
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::signout(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::ConfigDAO configDAO(config.db);
    configDAO.removeSession(request.beehive, request.user);
    _fcgiHandler.removeBeehive(request.session);
    _fcgiHandler.removeUser(request.session);
    response.status = 200;
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getUsers(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::UserService userService(connectionPtr.connection());
    std::string kind(request.matches[1]);
    std::vector<Services::Entities::User> users = userService.getUsers();
    std::ostringstream body;
    std::string comma("");
    body << "{\"data\":[";
    for (Services::Entities::User &user : users) {
      body << comma << "[";
      body << "\"" << user.id() << "\",";
      body << "\"" << user.identifier() << "\",";
      body << "\"" << user.name() << "\",";
      body << "\"";
      body << getDescription(user.kind());
      body << "\"";
      body << "]";
      comma = ",";
    }
    body << "]}";
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::UserService userService(connectionPtr.connection());
    uint32_t identifier = std::stoi(request.matches[1]);
    response.status = 200;
    response.body = userService.getUser(identifier);
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::saveUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::UserService userService(connectionPtr.connection());
    auto rJson = nlohmann::json::parse(request.body);
    std::string name = rJson["name"];
    std::string email = rJson["email"];
    std::string password = rJson["password"];
    std::string type = rJson["type"];
    uint64_t id = userService.save(name, email, password, std::stoi(type));
    if (id != 0) {
      std::ostringstream body;
      body << "[";
      body << "\"" << id << "\",";
      body << rJson["email"] << ",";
      body << rJson["name"] << ",";
      body << "\"" << getDescription(std::stoi(type)) << "\"";
      body << "]";
      response.status = 201;
      response.body = body.str();
      response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } else {
      response.status = 409;
    }
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::updateUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::UserService userService(connectionPtr.connection());
    uint32_t identifier = std::stoi(request.matches[1]);
    auto rJson = nlohmann::json::parse(request.body);
    std::string name = rJson["name"];
    std::string email = rJson["email"];
    std::string password = rJson["password"];
    std::string type = rJson["type"];
    if (userService.update(identifier, name, email, password, std::stoi(type))) {
      std::ostringstream body;
      body << "[";
      body << "\"" << identifier << "\",";
      body << rJson["email"] << ",";
      body << rJson["name"] << ",";
      body << "\"" << getDescription(std::stoi(type)) << "\"";
      body << "]";
      response.status = 200;
      response.body = body.str();
      response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } else {
      response.status = 409;
    }
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::removeUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::UserService userService(connectionPtr.connection());
    uint32_t identifier = std::stoi(request.matches[1]);
    response.status = 200;
    userService.remove(identifier);
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getBeehive(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    std::ostringstream body;
    body << "{\"beehive\":\"" << request.beehive << "\"}";
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::postApplication(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    auto rJson = nlohmann::json::parse(request.body);
    std::string uuid = rJson["uuid"];
    schemaService.updateSchema(uuid, uuid, request.body, kind);
    std::ostringstream body;
    body << "[";
    body << rJson["name"] << ",";
    body << rJson["uuid"] << ",";
    body << rJson["version"];
    body << "]";
    response.status = 201;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::postSchema(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string application(request.matches[2]);
    auto rJson = nlohmann::json::parse(request.body);
    std::string uuid = rJson["uuid"];
    schemaService.updateSchema(application, uuid, request.body, kind);
    std::ostringstream body;
    body << "[";
    body << rJson["name"] << ",";
    body << rJson["uuid"] << ",";
    body << rJson["version"];
    body << "]";
    response.status = 201;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::ServiceException &ex) {
    response.status = 500;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getApplication(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string uuid(request.matches[2]);
    response.status = 200;
    response.body = schemaService.readSchema(uuid, uuid, kind, request.queryString.find("copy") != request.queryString.end(), 0);
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getSchema(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string application(request.matches[2]);
    std::string uuid(request.matches[3]);
    int requestedVersion = 0;
    auto versionPrt = request.queryString.find("version");
    if (versionPrt != request.queryString.end())
      requestedVersion = std::stoi(versionPrt->second);
    response.status = 200;
    response.body = schemaService.readSchema(uuid, application, kind, request.queryString.find("copy") != request.queryString.end(), requestedVersion);
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getApplications(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::vector<std::string> schemas = schemaService.readSchemas(kind, kind);
    std::map<std::string, std::string, Services::Utils::ILessComparator> schemasMap;
    for (std::string &schema : schemas) {
      auto rJson = nlohmann::json::parse(schema);
      std::string name = rJson["name"];
      schemasMap.emplace(name, schema);
    }
    std::ostringstream body;
    std::string comma("");
    body << "[";
    for (auto schema : schemasMap) {
      body << comma << schema.second;
      comma = ",";
    }
    body << "]";
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getSchemas(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string application(request.matches[2]);
    std::vector<std::string> schemas = schemaService.readSchemas(application, kind);
    std::map<std::string, std::string, Services::Utils::ILessComparator> schemasMap;
    for (std::string &schema : schemas) {
      auto rJson = nlohmann::json::parse(schema);
      std::string name = rJson["name"];
      schemasMap.emplace(name, schema);
    }
    std::ostringstream body;
    std::string comma("");
    body << "[";
    for (auto schema : schemasMap) {
      body << comma << schema.second;
      comma = ",";
    }
    body << "]";
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::deleteApplication(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string uuid(request.matches[2]);
    schemaService.removeSchema(uuid, uuid, kind);
    response.status = 204;
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::deleteSchema(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string application(request.matches[2]);
    std::string uuid(request.matches[3]);
    if (schemaService.removeSchema(uuid, application, kind))
      response.status = 204;
    else
      response.status = 403;
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getApplicationData(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::vector<std::string> schemas = schemaService.readSchemas(kind, kind);
    std::map<std::string, std::string, Services::Utils::ILessComparator> schemasMap;
    for (std::string &schema : schemas) {
      auto rJson = nlohmann::json::parse(schema);
      std::string name = rJson["name"];
      schemasMap.emplace(name, schema);
    }
    std::ostringstream body;
    std::string comma("");
    body << "{\"data\":[";
    for (auto schema : schemasMap) {
      auto rJson = nlohmann::json::parse(schema.second);
      body << comma << "[";
      body << rJson["name"] << ",";
      body << rJson["uuid"] << ",";
      body << rJson["version"];
      body << "]";
      comma = ",";
    }
    body << "]}";
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getSchemaData(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string kind(request.matches[1]);
    std::string application(request.matches[2]);
    std::vector<std::string> schemas = schemaService.readSchemas(application, kind);
    std::map<std::string, std::string, Services::Utils::ILessComparator> schemasMap;
    for (std::string &schema : schemas) {
      auto rJson = nlohmann::json::parse(schema);
      std::string name = rJson["name"];
      schemasMap.emplace(name, schema);
    }
    std::ostringstream body;
    std::string comma("");
    body << "{\"data\":[";
    for (auto schema : schemasMap) {
      auto rJson = nlohmann::json::parse(schema.second);
      body << comma << "[";
      body << rJson["name"] << ",";
      body << rJson["uuid"];
      body << "]";
      comma = ",";
    }
    body << "]}";
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::postPublish(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string uuid(request.matches[1]);
    std::optional<std::string> aSchema = schemaService.publishApplication(uuid);
    auto rJson = nlohmann::json::parse(aSchema.value());
    std::ostringstream body;
    body << "[";
    body << rJson["name"] << ",";
    body << rJson["uuid"] << ",";
    body << rJson["version"];
    body << "]";
    response.status = 201;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::postDowngrade(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string uuid(request.matches[1]);
    std::optional<std::string> aSchema = schemaService.downgradeApplication(uuid);
    auto rJson = nlohmann::json::parse(aSchema.value());
    std::ostringstream body;
    body << "[";
    body << rJson["name"] << ",";
    body << rJson["uuid"] << ",";
    body << rJson["version"];
    body << "]";
    response.status = 201;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getExport(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::string uuid(request.matches[1]);
    std::pair<std::string, std::string> result = schemaService.exportApplication(uuid);
    response.body = result.second;
    response.status = 200;
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeOctet);
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentDisposition, "attachment; filename=\"" + result.first + ".json\"");
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::postImport(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    Services::DAO::SQL::ConnectionPool::ConnectionPtr connectionPtr = _connectionPool.getNamedConnection(request.beehive);
    Services::SchemaService schemaService(connectionPtr.connection(), request.beehive);
    std::pair<std::string, int> result = schemaService.importApplication(request.body);
    auto rJson = nlohmann::json::parse(result.first);
    std::ostringstream body;
    body << "[";
    body << rJson["name"] << ",";
    body << rJson["uuid"] << ",";
    body << rJson["version"];
    body << "]";
    response.status = result.second;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::getLog(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    std::ifstream::pos_type poss = 0;
    auto possPtr = request.queryString.find("poss");
    if (possPtr != request.queryString.end())
      poss = std::stol(possPtr->second);
    std::vector<std::string> lines;
    std::ifstream file("/var/log/syncserver/" + std::to_string(request.beehive) + ".log");
    if (file.is_open()) {
      file.seekg(poss);
      std::string line;
      while (std::getline(file, line)) {
        if (0 < line.size())
          lines.push_back(line);
        poss += line.size() + 1;
      }
      file.close();
    }
    nlohmann::json rJson;
    rJson["poss"] = (unsigned long) poss;
    rJson["lines"] = lines;
    std::ostringstream body;
    body << rJson;
    response.status = 200;
    response.body = body.str();
    response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

void AdminHandler::deleteLog(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
  try {
    std::string file = "/var/log/syncserver/" + std::to_string(request.beehive) + ".log";
    if (truncate(file.c_str(), 0) != 0) {
      LOG_ERROR << "An error occurred when truncating file";
    }
    response.status = 200;
  } catch (Services::NotExistException &ex) {
    response.status = 404;
  } catch (std::exception &ex) {
    LOG_ERROR << ex.what();
    response.status = 500;
  }
}

} /* namespace HTTP */
} /* namespace SyncServer */
