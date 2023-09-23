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

#include <atomic>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <json/json.hpp>
#include <sqlite/sqlite3.h>
#include <dao/sql/Connection.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Config {

enum Roles {
  owner = 1, developer = 2
};

struct Config {
  struct Database {
    std::string server;
    int64_t port;
    std::string user;
    std::string password;
  };

  struct Developer {
    std::string email;
    std::string name;
    std::string salt;
    std::string password;
    uint32_t beehive;
    int role;
  };

  Database database;
  std::unordered_set<uint32_t> identifiers;
  sqlite3 *db;
  sqlite3_uint64 idUser;
  char key[37];
};

struct Form {
  std::string kind;
  std::string form;
  std::string value;
};

struct Forms {
  std::vector<Form> forms;
};

void synchronize();
bool loadBootstrap();
bool loadConfig(const std::string &kind, const std::string &instance);

bool alreadyExists(uint32_t beehive, const std::string &owner);

bool loadForms();
bool saveForms();
Form& getForm(const std::string &kind);
void putForm(Form &form);

class ConfigException: public std::runtime_error {
public:
  ConfigException(const ConfigException &e) :
      ConfigException(e.what(), e._errorCode) {
  }

  ConfigException(const std::string &what, unsigned errorCode) :
      runtime_error(what), _errorCode(errorCode), _what(what) {
  }

  virtual ~ConfigException() throw () {
  }

  unsigned errorCode() const {
    return _errorCode;
  }

  virtual const char* what() const noexcept override {
    return _what.c_str();
  }

private:
  unsigned _errorCode;
  const std::string _what;
};

} /* namespace Config */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

namespace nlohmann {

void from_json(const json &j, SyncServer::Servers::Services::Config::Form &x);
void to_json(json &j, const SyncServer::Servers::Services::Config::Form &x);

void from_json(const json &j, SyncServer::Servers::Services::Config::Forms &x);
void to_json(json &j, const SyncServer::Servers::Services::Config::Forms &x);

inline void from_json(const json &j, SyncServer::Servers::Services::Config::Form &x) {
  x.kind = j.at("kind").get<std::string>();
  x.form = j.at("form").get<std::string>();
  x.value = j.at("value").get<std::string>();
}

inline void to_json(json &j, const SyncServer::Servers::Services::Config::Form &x) {
  j = json::object();
  j["kind"] = x.kind;
  j["form"] = x.form;
  j["value"] = x.value;
}

inline void from_json(const json &j, SyncServer::Servers::Services::Config::Forms &x) {
  x.forms = j.at("forms").get<std::vector<SyncServer::Servers::Services::Config::Form>>();
}

inline void to_json(json &j, const SyncServer::Servers::Services::Config::Forms &x) {
  j = json::object();
  j["forms"] = x.forms;
}
} /* namespace nlohmann */

std::ostream& operator<<(std::ostream &os, const SyncServer::Servers::Services::Config::Forms &data);

