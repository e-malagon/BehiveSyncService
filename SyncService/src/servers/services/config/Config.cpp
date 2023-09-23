
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

#include "Config.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <exception>
#include <unordered_map>
#include <condition_variable>
#include <sqlite/sqlite3.h>
#include <nanolog/NanoLog.hpp>
#include <config/Bootstrap.h>
#include <services/SchemaService.h>
#include <dao/ConfigDAO.h>

std::mutex beehivesMutex;
SyncServer::Servers::Services::Config::Config config;
SyncServer::Servers::Services::Config::Forms forms;
std::unordered_map<std::string, SyncServer::Servers::Services::Config::Form&> formsMap;
std::mutex notifyMutex;
std::condition_variable notify;
bool runSync(false);

bool running(true);

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Config {

const char *create01 = "CREATE TABLE IF NOT EXISTS beehives(id TEXT NOT NULL, dataset TEXT NOT NULL, PRIMARY KEY(id));";
const char *create02 = "CREATE TABLE IF NOT EXISTS databases(id TEXT NOT NULL, server TEXT NOT NULL, port INTEGER NOT NULL, user TEXT NOT NULL, password TEXT NOT NULL, min INTEGER NOT NULL, max INTEGER NOT NULL, PRIMARY KEY(id));";
const char *create03 = "CREATE TABLE IF NOT EXISTS developers(beehive TEXT NOT NULL, email TEXT NOT NULL, name TEXT NOT NULL, role INTEGER NOT NULL, password TEXT NOT NULL, salt TEXT NOT NULL, PRIMARY KEY(beehive,email));";
const char *create04 = "CREATE TABLE IF NOT EXISTS applications(beehive TEXT NOT NULL, id TEXT NOT NULL, schema TEXT NOT NULL, PRIMARY KEY(beehive,id));";
const char *create05 = "CREATE TABLE IF NOT EXISTS entities(beehive TEXT NOT NULL, application TEXT NOT NULL, id TEXT NOT NULL, schema TEXT NOT NULL, version INTEGER NOT NULL, PRIMARY KEY(beehive,application,id,version));";
const char *create06 = "CREATE TABLE IF NOT EXISTS modules(beehive TEXT NOT NULL, application TEXT NOT NULL, id TEXT NOT NULL, schema TEXT NOT NULL,  version INTEGER NOT NULL, PRIMARY KEY(beehive,application,id,version));";
const char *create07 = "CREATE TABLE IF NOT EXISTS roles(beehive TEXT NOT NULL, application TEXT NOT NULL, id TEXT NOT NULL, schema TEXT NOT NULL, version INTEGER NOT NULL, PRIMARY KEY(beehive,application,id,version));";
const char *create08 = "CREATE TABLE IF NOT EXISTS transactions(beehive TEXT NOT NULL, application TEXT NOT NULL, id TEXT NOT NULL, schema TEXT NOT NULL, version INTEGER NOT NULL, PRIMARY KEY(beehive,application,id,version));";
const char *create09 = "CREATE TABLE IF NOT EXISTS sessions(beehive TEXT NOT NULL, email TEXT NOT NULL, token INTEGER NOT NULL, last INTEGER NOT NULL, PRIMARY KEY(beehive,email));";

using namespace std::chrono_literals;

static void errorLogCallback(void *extra, int iErrCode, const char *zMsg){
  LOG_ERROR << "SQLite error (" << iErrCode << ") " << zMsg;
}

void openConfigDatabase(const std::string path, const std::string &kind, const std::string &instance) {
  sqlite3_config(SQLITE_CONFIG_LOG, errorLogCallback, NULL);

  if (sqlite3_open(path.c_str(), &config.db) != SQLITE_OK) {
    LOG_ERROR << "Unable to open configuration database: " << path;
    return;
  }
  if (sqlite3_exec(config.db, create01, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create01;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create02, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create02;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create03, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create03;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create04, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create04;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create05, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create05;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create06, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create06;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create07, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create07;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create08, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create08;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }
  if (sqlite3_exec(config.db, create09, 0, 0, 0) != SQLITE_OK) {
    LOG_ERROR << "Unable to execute: " << create09;
    sqlite3_close(config.db);
    config.db = nullptr;
    return;
  }

  std::string password = kind + instance;
  std::thread syncThread([](sqlite3 *db, const std::string &kind, const std::string &instance, const std::string &password) {
    while (beehive_register(db, "main", "00000000", kind.c_str(), kind.size(), kind.c_str(), kind.size(), password.c_str(), password.size(), "Beehive", 7, kind.c_str(), kind.size(), instance.c_str(), instance.size(), &config.idUser, config.key) != BEEHIVE_OK) {
      LOG_ERROR << "Unable to attach to the master beehive.";
      std::this_thread::sleep_for(5min);
    }
    while (running) {
      if(beehive_synchronize(db, "main", config.key, 0) != BEEHIVE_OK) {
        LOG_ERROR << "Error while synchronizing " << instance;
      }
      runSync = false;
      auto start = std::chrono::steady_clock::now();
      std::unique_lock<std::mutex> lock(notifyMutex);
      notify.wait_for(lock, 5min, [&start] {
          auto now = std::chrono::steady_clock::now();
          return runSync || (now - start) > 5min;
      });
    }
  }, config.db, kind, instance, password);
  syncThread.detach();
}

void synchronize() {
  std::unique_lock<std::mutex> lock(notifyMutex);
  runSync = true;
  notify.notify_all();
}

bool loadBootstrap() {
  try {
    std::ifstream configifs("/etc/syncserver/bootstrap.json");
    Bootstrap bootstrap = json::parse(configifs);
    config.database.server = bootstrap.database.server;
    config.database.port = bootstrap.database.port;
    config.database.user = bootstrap.database.user;
    config.database.password = bootstrap.database.password;
    config.db = nullptr;
    SchemaService::loadApplicationSchemas(bootstrap);
    return true;
  } catch (std::exception &e) {
    LOG_ERROR << "Unable to read configuration file: " << e.what();
    return false;
  }
}

bool loadConfig(const std::string &kind, const std::string &instance) {
  try {
    if (kind == "beehive") {
      openConfigDatabase("/var/lib/syncserver/beehive." + instance + ".db", kind, instance);
    } else if (kind == "developer") {
      openConfigDatabase("/var/lib/syncserver/developer.db", kind, instance);
    } else {
      LOG_ERROR << "Unknown server mode";
      return false;
    }
    if (!config.db)
      return false;
    DAO::ConfigDAO configDAO(config.db);
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(config.db, "SELECT server, port, user, password FROM databases;", -1, &stmt, 0);
    if (rc == SQLITE_OK) {
      rc = sqlite3_step(stmt);
      if (rc == SQLITE_ROW) {
        config.database.server = (const char*) sqlite3_column_text(stmt, 0);
        config.database.port = sqlite3_column_int(stmt, 1);
        config.database.user = (const char*) sqlite3_column_text(stmt, 2);
        config.database.password = (const char*) sqlite3_column_text(stmt, 3);
      }
      sqlite3_finalize(stmt);
      if (rc != SQLITE_ROW) {
        LOG_ERROR << "No database connection";
        return false;
      }
    }
    for (auto c : configDAO.readBeehives()) {
      SchemaService::loadSchemas(config.db, c.first);
    }
    return true;
  } catch (std::exception &e) {
    LOG_ERROR << "Unable to read configuration file: " << e.what();
    return false;
  }
}

bool alreadyExists(uint32_t beehive, const std::string &owner) {
  if (beehive != 0) {
    std::lock_guard<std::mutex> lock(beehivesMutex);
    auto identifierPtr = config.identifiers.find(beehive);
    if (identifierPtr != config.identifiers.end()) {
      sqlite3_stmt *stmt;
      int rc = sqlite3_prepare_v2(config.db, "SELECT * FROM developers WHERE beehive = ? AND email = ? AND role = 1;", -1, &stmt, 0);
      if (rc == SQLITE_OK) {
        if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, owner.c_str(), owner.length(), 0)) == SQLITE_OK)
          rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc == SQLITE_DONE)
          return true;
      }
      throw ServiceException("Unable to retreive current status of the beehive", 1234);
    } else
      return false;
  } else
    return false;
}

bool loadForms() {
  try {
    std::ifstream configifs("/etc/syncserver/forms.conf");
    forms = json::parse(configifs);
    formsMap.clear();
    for (Form &form : forms.forms) {
      formsMap.emplace(form.kind, form);
    }
    return true;
  } catch (std::exception &e) {
    LOG_ERROR << "Unable to read configuration file: " << e.what();
    return false;
  }
}

bool saveForms() {
  try {
    std::ofstream configofs("/var/lib/syncserver/forms.conf");
    configofs << forms;
    return true;
  } catch (std::exception &e) {
    LOG_ERROR << "Unable to write configuration file: " << e.what();
    return false;
  }
}

Form& getForm(const std::string &kind) {
  auto formPtr = formsMap.find(kind);
  if (formPtr != formsMap.end()) {
    return formPtr->second;
  } else {
    throw ConfigException("Invalid form kind: " + kind, 0);
  }
}

void putForm(Form &form) {
  auto formPtr = formsMap.find(form.kind);
  if (formPtr != formsMap.end()) {
    formPtr->second.form = form.form;
    formPtr->second.value = form.value;
  }
}

} /* namespace Config */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

std::ostream& operator<<(std::ostream &os, const SyncServer::Servers::Services::Config::Forms &data) {
  nlohmann::json j = data;
  os << j.dump(4);
  return os;
}
