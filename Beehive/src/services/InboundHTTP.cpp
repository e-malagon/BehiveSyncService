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


#include <errno.h>
#include <grp.h>
#include <pwd.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <sys/stat.h>
#include <unistd.h>

#include <crypto/Crypto.hpp>
#include <map>
#include <nanolog/NanoLog.hpp>
#include <services/InboundHTTP.hpp>
#include <string/ICaseMap.hpp>

namespace Beehive {
namespace Services {

std::string InboundHTTP::_servicePath = "/context";
std::string InboundHTTP::_serviceSocket = "/var/tmp/beehive.sock";

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

void InboundHTTP::start() {
    using namespace std::placeholders;

    // Context administration services
    _fcgiHandler.addPost("^" + _servicePath + "$", true, std::bind(&InboundHTTP::postContext, this, _1, _2));
    _fcgiHandler.addGet("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&InboundHTTP::getContext, this, _1, _2));
    _fcgiHandler.addGet("^" + _servicePath + "$", true, std::bind(&InboundHTTP::getContexts, this, _1, _2));
    _fcgiHandler.addPut("^" + _servicePath + "$", true, std::bind(&InboundHTTP::putContext, this, _1, _2));
    _fcgiHandler.addDelete("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&InboundHTTP::deleteContext, this, _1, _2));
    _fcgiHandler.addLink("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&InboundHTTP::linkContext, this, _1, _2));
    _fcgiHandler.addUnlink("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&InboundHTTP::unlinkContext, this, _1, _2));
    _fcgiHandler.addGet("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/versions$", true, std::bind(&InboundHTTP::getLinkedVersions, this, _1, _2));
    _fcgiHandler.addGet("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/versions/([0-9]+)$", true, std::bind(&InboundHTTP::getLinkedVersion, this, _1, _2));

    // User administration services
    _fcgiHandler.addPost("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/users$", true, std::bind(&InboundHTTP::postUser, this, _1, _2));
    _fcgiHandler.addGet("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/users/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&InboundHTTP::getUser, this, _1, _2));
    _fcgiHandler.addGet("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/users$", true, std::bind(&InboundHTTP::getUsers, this, _1, _2));
    _fcgiHandler.addPut("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/users$", true, std::bind(&InboundHTTP::putUser, this, _1, _2));
    _fcgiHandler.addDelete("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/users/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})$", true, std::bind(&InboundHTTP::deleteUser, this, _1, _2));

    // Client synchronization services
    _fcgiHandler.addPost("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/synch/signup$", false, std::bind(&InboundHTTP::signUp, this, _1, _2));
    _fcgiHandler.addPost("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/synch/signin$", false, std::bind(&InboundHTTP::signIn, this, _1, _2));
    _fcgiHandler.addPost("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/synch/signout$", true, std::bind(&InboundHTTP::signOut, this, _1, _2));
    _fcgiHandler.addPost("^" + _servicePath + "/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/synch/signoff$", false, std::bind(&InboundHTTP::signOff, this, _1, _2));

    LOG_INFO << "Starting Admin Server";
    _fcgiSocket = FCGX_OpenSocket(_serviceSocket.c_str(), 128);
    if (0 < _fcgiSocket) {
        if (chmod(_serviceSocket.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0) {
            std::cerr << "Unable to set permission for " << _serviceSocket;
            exit(errno);
        }
        if (chown(_serviceSocket.c_str(), getuid(), getgrnam("www-data")->gr_gid) == -1) {
            std::cerr << "Unable to set permission for " << _serviceSocket;
            exit(errno);
        }
    } else {
        std::cerr << "Unable to open " << _serviceSocket;
        exit(1);
    }
    _fcgiHandler.run(_fcgiSocket);
    LOG_INFO << "Stoping Admin Server";
}

void InboundHTTP::finish() {
    close(_fcgiSocket);
    FCGX_ShutdownPending();
}

void InboundHTTP::postContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        SchemaService schemaService;
        response.body = schemaService.postContext(request.body);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::AlreadyExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 409;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 400;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::getContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        std::string uuid(request.matches[1]);
        response.body = schemaService.getContext(uuid);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::getContexts(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        response.body = schemaService.getContexts();
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 400;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::putContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        response.body = schemaService.putContext(request.body);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 400;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::deleteContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        std::string uuid(request.matches[1]);
        schemaService.deleteContext(uuid);
        response.status = 204;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::linkContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        std::string uuid(request.matches[1]);
        auto linkPtr = request.headers.find(FCGI::FcgiHandler::RequestLink);
        if (linkPtr != request.headers.end()) {
            schemaService.linkContext(uuid, linkPtr->second);
            response.status = 202;
        } else {
            nlohmann::json error;
            error["message"] = "Link header is missing.";
            response.body = error.dump();
            response.status = 404;
        }
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::unlinkContext(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        std::string uuid(request.matches[1]);
        auto linkPtr = request.headers.find(FCGI::FcgiHandler::RequestLink);
        if (linkPtr != request.headers.end()) {
            schemaService.unlinkContext(uuid, linkPtr->second);
            response.status = 202;
        } else {
            nlohmann::json error;
            error["message"] = "Link header is missing.";
            response.body = error.dump();
            response.status = 404;
        }
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::getLinkedVersions(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        std::string uuid(request.matches[1]);
        response.body = schemaService.getLinkedVersions(uuid);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::getLinkedVersion(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        UserService userService;
        userService.authenticateDeveloper(request.authorization);
        Services::SchemaService schemaService;
        std::string uuid(request.matches[1]);
        std::string version(request.matches[2]);
        response.body = schemaService.getLinkedVersion(uuid, version);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::postUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        userService.authenticateDeveloper(request.authorization);
        std::string context(request.matches[1]);
        auto rJson = nlohmann::json::parse(request.body);
        std::string email = rJson["email"];
        std::string name = rJson["name"];
        std::string password = rJson["password"];
        std::string type = rJson["type"];
        std::transform(email.begin(), email.end(), email.begin(), ::tolower);
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        userService.save(email, name, password, Entities::User::getType(type), context);
        response.status = 202;
    } catch (Services::AlreadyExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 409;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::getUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        userService.authenticateDeveloper(request.authorization);
        std::string context(request.matches[1]);
        std::string identifier(request.matches[2]);
        std::transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
        response.body = userService.getUser(identifier, context);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::getUsers(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        userService.authenticateDeveloper(request.authorization);
        std::string context(request.matches[1]);
        response.body = userService.getUsers(context);
        response.status = 200;
        response.headers.emplace(FCGI::FcgiHandler::ResponseContentType, FCGI::FcgiHandler::ResponseContentTypeJSON);
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::putUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        userService.authenticateDeveloper(request.authorization);
        std::string context(request.matches[1]);
        auto rJson = nlohmann::json::parse(request.body);
        std::string name = rJson["name"];
        std::string email = rJson["email"];
        std::string password = rJson["password"];
        std::string type = rJson["type"];
        std::transform(email.begin(), email.end(), email.begin(), ::tolower);
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        userService.update(email, name, password, Entities::User::getType(type), context);
        response.status = 202;
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::deleteUser(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        userService.authenticateDeveloper(request.authorization);
        std::string context(request.matches[1]);
        std::string identifier(request.matches[2]);
        std::transform(identifier.begin(), identifier.end(), identifier.begin(), ::tolower);
        userService.remove(identifier, context);
        response.status = 202;
    } catch (Services::NotExistsException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 404;
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::signUp(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        std::string context(request.matches[1]);
        auto rJson = nlohmann::json::parse(request.body);
        std::string email = rJson["email"];
        std::string name = rJson["name"];
        std::string password = rJson["password"];
        std::string moduleName = rJson["moduleName"];
        std::string uuidNode = rJson["uuidNode"];
        std::transform(email.begin(), email.end(), email.begin(), ::tolower);
        std::unique_ptr<Services::Entities::Node> node = userService.signUp(name, email, password, moduleName, uuidNode, context);
        response.status = 200;
        response.body = "{\"sessionId\":\"" + node->nodeKey() + "\"}";
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::signIn(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        std::string context(request.matches[1]);
        auto rJson = nlohmann::json::parse(request.body);
        std::string jwt = Config::get_optional<std::string>(rJson, "jwt", "");
        std::string email = Config::get_optional<std::string>(rJson, "email", "");
        std::string password = Config::get_optional<std::string>(rJson, "password", "");
        std::string moduleName = rJson["moduleName"];
        std::string uuidNode = rJson["uuidNode"];
        std::string type = rJson["type"];
        std::transform(email.begin(), email.end(), email.begin(), ::tolower);
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        Entities::User::Type typeUser = Entities::User::getType(type);
        std::unique_ptr<Services::Entities::Node> node;
        if (typeUser == Entities::User::Type::internal)
            node = userService.signIn(email, password, moduleName, uuidNode, context);
        else
            node = userService.signIn(jwt, moduleName, uuidNode, type, context);
        response.status = 200;
        response.body = "{\"sessionId\":\"" + node->nodeKey() + "\"}";
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::signOut(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        std::unique_ptr<Entities::Node> node = userService.authenticateUser(request.authorization);
        userService.signOut(*node);
    } catch (Services::AuthenticationException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 403;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

void InboundHTTP::signOff(const FCGI::FcgiHandler::Request &request, FCGI::FcgiHandler::Response &response) {
    try {
        Services::UserService userService;
        std::string context(request.matches[1]);
        auto rJson = nlohmann::json::parse(request.body);
        std::string jwt = Config::get_optional<std::string>(rJson, "jwt", "");
        std::string email = Config::get_optional<std::string>(rJson, "email", "");
        std::string password = Config::get_optional<std::string>(rJson, "password", "");
        std::string type = rJson["type"];
        std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        Entities::User::Type typeUser = Entities::User::getType(type);
        if (typeUser == Entities::User::Type::internal)
            userService.signOff(email, password, context);
        else
            userService.signOff(jwt, type, context);
        response.status = 202;
    } catch (Services::ServiceException &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    } catch (std::exception &ex) {
        nlohmann::json error;
        error["message"] = ex.what();
        response.body = error.dump();
        response.status = 500;
    }
}

} /* namespace Services */
} /* namespace Beehive */
