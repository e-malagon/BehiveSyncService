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

#include "ConfigDAO.h"

#include <config/Config.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

int ConfigDAO::beginTransaction(const std::string id, const char *idDataset) {
  sqlite3_stmt *stmt;
  std::string sentence = "BEGIN TRANSACTION " + id + " ON ?;";
  int rc = sqlite3_prepare_v2(_db, sentence.c_str(), -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_text(stmt, 1, idDataset, 36, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::commitTransaction() {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "COMMIT TRANSACTION;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::rollbackTransaction() {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "ROLLBACK TRANSACTION;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveBeehive(uint32_t beehive, const char *idDataset) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO beehives VALUES(?, ?);", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, idDataset, 36, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveDeveloper(uint32_t beehive, const std::string &name, const std::string &email, const std::string &passwd, const std::string &salt) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO developers VALUES(?, ?, ?, ?, ?, ?);", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, email.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, name.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, 1)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 5, passwd.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 6, salt.c_str(), -1, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveSession(uint32_t beehive, const std::string &email, const std::string &token, time_t now) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO sessions VALUES(?, ?, ?, ?) ON CONFLICT(beehive, email) DO UPDATE SET token=excluded.token, last=excluded.last;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, email.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, token.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, now)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveApplication(uint32_t beehive, const std::string &uuid, const std::string &schema) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO applications VALUES(?, ?, ?) ON CONFLICT(beehive, id) DO UPDATE SET schema=excluded.schema;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, schema.c_str(), -1, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveEntity(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO entities VALUES(?, ?, ?, ?, ?) ON CONFLICT(beehive, application, id, version) DO UPDATE SET schema=excluded.schema;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 4, schema.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 5, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveTransaction(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO transactions VALUES(?, ?, ?, ?, ?) ON CONFLICT(beehive, application, id, version) DO UPDATE SET schema=excluded.schema;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 4, schema.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 5, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveRole(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO roles VALUES(?, ?, ?, ?, ?) ON CONFLICT(beehive, application, id, version) DO UPDATE SET schema=excluded.schema;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 4, schema.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 5, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::saveModule(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "INSERT INTO modules VALUES(?, ?, ?, ?, ?) ON CONFLICT(beehive, application, id, version) DO UPDATE SET schema=excluded.schema;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 4, schema.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 5, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

std::string ConfigDAO::readDataset(uint32_t beehive) {
  std::string idDataset;
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "SELECT dataset FROM beehives WHERE id = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
      idDataset = std::string((char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
      rc = SQLITE_OK;
    }
    sqlite3_finalize(stmt);
  }
  return idDataset;
}

std::optional<Config::Config::Developer> ConfigDAO::readDeveloper(const std::string &email) {
  std::optional<Config::Config::Developer> developer;
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "SELECT name, role, password, salt, beehive FROM developers WHERE email = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_text(stmt, 1, email.c_str(), -1, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
      developer = std::make_optional<Config::Config::Developer>();
      developer->name = (const char*) sqlite3_column_text(stmt, 0);
      developer->role = sqlite3_column_int(stmt, 1);
      developer->password = (const char*) sqlite3_column_text(stmt, 2);
      developer->salt = (const char*) sqlite3_column_text(stmt, 3);
      developer->beehive = sqlite3_column_int(stmt, 4);
      rc = SQLITE_OK;
    }
    sqlite3_finalize(stmt);
  }
  return developer;
}

std::optional<std::string> ConfigDAO::readApplication(uint32_t beehive, const std::string &uuid) {
  std::optional<std::string> application;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM applications WHERE beehive = ? AND id = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, uuid.c_str(), -1, 0)) == SQLITE_OK) {
      if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        application = std::make_optional(schema);
      }
    }
    sqlite3_finalize(stmt);
  }

  return application;
}

std::optional<std::pair<std::string, std::string>> ConfigDAO::readEntity(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  std::optional<std::pair<std::string, std::string>> entity;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM entities WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        entity = std::make_optional(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return entity;
}

std::optional<std::pair<std::string, std::string>> ConfigDAO::readTransaction(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  std::optional<std::pair<std::string, std::string>> transaction;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM transactions WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        transaction = std::make_optional(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return transaction;
}

std::optional<std::pair<std::string, std::string>> ConfigDAO::readRole(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  std::optional<std::pair<std::string, std::string>> role;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM roles WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        role = std::make_optional(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return role;
}

std::optional<std::pair<std::string, std::string>> ConfigDAO::readModule(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  std::optional<std::pair<std::string, std::string>> module;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM modules WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        module = std::make_optional(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return module;
}

std::vector<std::pair<uint32_t, uint64_t>> ConfigDAO::readBeehives() {
  std::vector<std::pair<uint32_t, uint64_t>> beehives;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, dataset FROM beehives;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      uint64_t id = sqlite3_column_int(stmt, 0);
      uint64_t dataset = sqlite3_column_int64(stmt, 1);
      beehives.push_back(std::make_pair(id, dataset));
    }
    sqlite3_finalize(stmt);
  }
  return beehives;
}

std::vector<std::string> ConfigDAO::readApplications(uint32_t beehive) {
  std::vector<std::string> applications;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM applications WHERE beehive = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        applications.push_back(schema);
      }
    }
    sqlite3_finalize(stmt);
  }
  return applications;
}

std::vector<std::pair<std::string, std::string>> ConfigDAO::readEntities(uint32_t beehive, const std::string &application, int version) {
  std::vector<std::pair<std::string, std::string>> entities;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM entities WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        entities.push_back(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return entities;
}

std::vector<std::pair<std::string, std::string>> ConfigDAO::readTransactions(uint32_t beehive, const std::string &application, int version) {
  std::vector<std::pair<std::string, std::string>> transactions;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM transactions WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        transactions.push_back(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return transactions;
}

std::vector<std::pair<std::string, std::string>> ConfigDAO::readRoles(uint32_t beehive, const std::string &application, int version) {
  std::vector<std::pair<std::string, std::string>> roles;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM roles WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        roles.push_back(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return roles;
}

std::vector<std::pair<std::string, std::string>> ConfigDAO::readModules(uint32_t beehive, const std::string &application, int version) {
  std::vector<std::pair<std::string, std::string>> modules;
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "SELECT id, schema FROM modules WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK) {
      while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        std::string id((const char*) sqlite3_column_text(stmt, 0), sqlite3_column_bytes(stmt, 0));
        std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
        modules.push_back(std::make_pair(id, schema));
      }
    }
    sqlite3_finalize(stmt);
  }
  return modules;
}

int ConfigDAO::removeSession(uint32_t beehive, const std::string &email) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "DELETE FROM sessions WHERE beehive = ? AND email = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, email.c_str(), -1, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::removeApplication(uint32_t beehive, const std::string &uuid) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "DELETE FROM applications WHERE beehive = ? AND id = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, uuid.c_str(), -1, 0)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::removeEntity(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "DELETE FROM entities WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::removeTransaction(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "DELETE FROM transactions WHERE beehive = ? AND application = ? AND id = ? AND version = ?", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::removeRole(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "DELETE FROM roles WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::removeModule(uint32_t beehive, const std::string &application, const std::string &uuid, int version) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(_db, "DELETE FROM modules WHERE beehive = ? AND application = ? AND id = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 3, uuid.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 4, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

int ConfigDAO::removeVersion(uint32_t beehive, const std::string &application, int version) {
  sqlite3_stmt *stmt;
  int rc = SQLITE_OK;
  rc = sqlite3_prepare_v2(_db, "DELETE FROM entities WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "DELETE FROM transactions WHERE beehive = ? AND application = ? AND version = ?", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "DELETE FROM roles WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  if (rc == SQLITE_OK)
    rc = sqlite3_prepare_v2(_db, "DELETE FROM modules WHERE beehive = ? AND application = ? AND version = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    if ((rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK && (rc = sqlite3_bind_text(stmt, 2, application.c_str(), -1, 0)) == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 3, version)) == SQLITE_OK)
      rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      rc = SQLITE_OK;
  }
  return rc;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
