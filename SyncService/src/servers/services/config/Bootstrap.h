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

#include "Common.h"

#include <json/json.hpp>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Config {
using nlohmann::json;

struct Bootstrap {
  struct Database {
    std::string server;
    int64_t port;
    std::string user;
    std::string password;
  };

  struct Application {
    std::string uuid;
    std::string name;
    std::string defaultrole;
    int64_t version;
    int64_t edited;
    std::vector<std::string> entities;
    std::vector<std::string> transactions;
    std::vector<std::string> roles;
    std::vector<std::string> modules;
  };

  Database database;
  Application application;

};
} /* namespace Config */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

namespace nlohmann {
using SyncServer::Servers::Services::Config::Bootstrap;

void from_json(const json &j, Bootstrap::Application &x);
void to_json(json &j, const Bootstrap::Application &x);

void from_json(const json &j, Bootstrap::Database &x);
void to_json(json &j, const Bootstrap::Database &x);

void from_json(const json &j, Bootstrap &x);
void to_json(json &j, const Bootstrap &x);

inline void from_json(const json &j, Bootstrap::Application &x) {
  x.uuid = j.at("uuid").get<std::string>();
  x.name = j.at("name").get<std::string>();
  x.defaultrole = j.at("defaultrole").get<std::string>();
  x.version = j.at("version").get<int64_t>();
  x.edited = j.at("edited").get<int64_t>();
  x.entities = j.at("entities").get<std::vector<std::string>>();
  x.transactions = j.at("transactions").get<std::vector<std::string>>();
  x.roles = j.at("roles").get<std::vector<std::string>>();
  x.modules = j.at("modules").get<std::vector<std::string>>();
}

inline void to_json(json &j, const Bootstrap::Application &x) {
  j = json::object();
  j["uuid"] = x.uuid;
  j["name"] = x.name;
  j["defaultrole"] = x.defaultrole;
  j["version"] = x.version;
  j["edited"] = x.edited;
  j["entities"] = x.entities;
  j["transactions"] = x.transactions;
  j["roles"] = x.roles;
  j["modules"] = x.modules;
}

inline void from_json(const json &j, Bootstrap::Database &x) {
  x.server = j.at("server").get<std::string>();
  x.port = j.at("port").get<int64_t>();
  x.user = j.at("user").get<std::string>();
  x.password = j.at("password").get<std::string>();
}

inline void to_json(json &j, const Bootstrap::Database &x) {
  j = json::object();
  j["server"] = x.server;
  j["port"] = x.port;
  j["user"] = x.user;
  j["password"] = x.password;
}

inline void from_json(const json &j, Bootstrap &x) {
  x.database = j.at("database").get<Bootstrap::Database>();
  x.application = j.at("application").get<Bootstrap::Application>();
}

inline void to_json(json &j, const Bootstrap &x) {
  j = json::object();
  j["database"] = x.database;
  j["application"] = x.application;
}
}
