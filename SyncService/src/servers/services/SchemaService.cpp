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

#include "SchemaService.h"

#include <ServiceException.h>
#include <crypto/Crypto.h>
#include <json/json.hpp>
#include <config/Entity.h>
#include <dao/SchemaDAO.h>
#include <dao/ConfigDAO.h>
#include <config/Config.h>

#include <sqlite/sqlite3.h>
#include <nanolog/NanoLog.hpp>
#include <uuid/uuid.h>
#include <sstream>
#include <regex>
#include <unordered_map>

extern SyncServer::Servers::Services::Config::Config config;

namespace SyncServer {
namespace Servers {
namespace Services {

std::unordered_map<uint32_t, std::unordered_map<std::string, Config::Application>> SchemaService::_beehives;
std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> SchemaService::_applications;
std::mutex SchemaService::_beehivesMutex;

void SchemaService::loadSchemas(sqlite3 *db, uint32_t beehive) {
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, "SELECT id, schema FROM applications WHERE beehive = ?;", -1, &stmt, 0);
  if (rc == SQLITE_OK && (rc = sqlite3_bind_int(stmt, 1, beehive)) == SQLITE_OK) {
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      std::string schema((const char*) sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
      Config::Application application = SchemaService::loadApplicationSchemas(db, beehive, schema);
      std::lock_guard<std::mutex> lock(_beehivesMutex);
      _beehives[beehive].emplace(application.uuid, application);
      _applications[beehive].emplace(application.name, application.uuid);
    }
    sqlite3_finalize(stmt);
  }
}

Config::Application SchemaService::loadApplicationSchemas(sqlite3 *db, uint32_t beehive, const std::string schema) {
  Config::Application application = nlohmann::json::parse(schema);
  DAO::ConfigDAO configDAO(config.db);
  for (int i = 0; i <= application.version; i++) {
    for (auto e : configDAO.readEntities(beehive, application.uuid, i)) {
      Config::Entity entity = nlohmann::json::parse(e.second);
      application.entities[i].emplace(entity.uuid, entity);
      application.entitiesName2UUID[i].emplace(entity.name, entity.uuid);
    }
    for (auto t : configDAO.readTransactions(beehive, application.uuid, i)) {
      Config::Transaction transaction = nlohmann::json::parse(t.second);
      application.transactions[i].emplace(transaction.uuid, transaction);
      application.transactionsName2UUID[i].emplace(transaction.name, transaction.uuid);
    }
    for (auto r : configDAO.readRoles(beehive, application.uuid, i)) {
      Config::Role role = nlohmann::json::parse(r.second);
      application.roles[i].emplace(role.uuid, role);
      application.rolesName2UUID[i].emplace(role.name, role.uuid);
    }
    for (auto m : configDAO.readModules(beehive, application.uuid, i)) {
      Config::Module module = nlohmann::json::parse(m.second);
      application.modules[i].emplace(module.uuid, module);
      application.modulesName2UUID[i].emplace(module.name, module.uuid);
    }
    std::unordered_map<std::string, std::unordered_map<std::string, Config::Entity::Transaction, Utils::IHasher, Utils::IEqualsComparator>> transactions;
    for (auto &transaction : application.transactions[i]) {
      for (auto entity : transaction.second.entities) {
        Config::Entity::Transaction entityTransaction;
        entityTransaction.name = transaction.second.name;
        entityTransaction.add = entity.add;
        entityTransaction.remove = entity.remove;
        for (std::string updatable : entity.update)
          entityTransaction.update.emplace(std::stoi(updatable));
        transactions[entity.entity].emplace(transaction.second.uuid, entityTransaction);
      }
    }
    for (auto &entity : application.entities[i])
      entity.second.transactions = transactions[entity.second.uuid];
  }
  return application;
}

void SchemaService::loadApplicationSchemas(Config::Bootstrap bootstrap) {
  Config::Application application;
  application.uuid = bootstrap.application.uuid;
  application.name = bootstrap.application.name;
  application.defaultrole = bootstrap.application.defaultrole;
  application.version = bootstrap.application.version;
  application.edited = bootstrap.application.edited;
  for (std::string &schema : bootstrap.application.entities) {
    Config::Entity entity = nlohmann::json::parse(schema);
    application.entities[0].emplace(entity.uuid, entity);
    application.entitiesName2UUID[0].emplace(entity.name, entity.uuid);
  }
  for (std::string &schema : bootstrap.application.transactions) {
    Config::Transaction transaction = nlohmann::json::parse(schema);
    application.transactions[0].emplace(transaction.uuid, transaction);
    application.transactionsName2UUID[0].emplace(transaction.name, transaction.uuid);
  }
  for (std::string &schema : bootstrap.application.roles) {
    Config::Role role = nlohmann::json::parse(schema);
    application.roles[0].emplace(role.uuid, role);
    application.rolesName2UUID[0].emplace(role.name, role.uuid);
  }
  for (std::string &schema : bootstrap.application.modules) {
    Config::Module module = nlohmann::json::parse(schema);
    application.modules[0].emplace(module.uuid, module);
    application.modulesName2UUID[0].emplace(module.name, module.uuid);
  }
  std::unordered_map<std::string, std::unordered_map<std::string, Config::Entity::Transaction, Utils::IHasher, Utils::IEqualsComparator>> transactions;
  for (auto &transaction : application.transactions[0]) {
    for (auto entity : transaction.second.entities) {
      Config::Entity::Transaction entityTransaction;
      entityTransaction.name = transaction.second.name;
      entityTransaction.add = entity.add;
      entityTransaction.remove = entity.remove;
      for (std::string updatable : entity.update)
        entityTransaction.update.emplace(std::stoi(updatable));
      transactions[entity.entity].emplace(transaction.second.uuid, entityTransaction);
    }
  }
  for (auto &entity : application.entities[0])
    entity.second.transactions = transactions[entity.second.uuid];
  std::lock_guard<std::mutex> lock(_beehivesMutex);
  _beehives[0].emplace(application.uuid, application);
  _applications[0].emplace(application.name, application.uuid);
}

std::pair<std::string, std::string> SchemaService::getApplicationAndModuleUUID(uint32_t beehive, const std::string application, const std::string module, uint32_t version) {
  std::lock_guard<std::mutex> lock(_beehivesMutex);
  auto applicationPtr = _applications[beehive].find(application);
  if (applicationPtr == _applications[beehive].end()) {
    LOG_FILE_WARN(beehive) << "Unknown application '" << application << "', connection refused.";
    return std::make_pair("", "");
  }
  auto appPtr = _beehives[beehive].find(applicationPtr->second);
  if (appPtr == _beehives[beehive].end()) {
    LOG_FILE_WARN(beehive) << "Error on application schema (" << application << "), connection refused.";
    return std::make_pair("", "");
  }
  if (appPtr->second.version < version) {
    LOG_FILE_WARN(beehive) << "Client version " << version <<  " is higher than current version " << appPtr->second.version << ", connection refused to '" << application << "'.";
    return std::make_pair("", "");
  }
  auto modulePtr = appPtr->second.modulesName2UUID[version].find(module);
  if (modulePtr == appPtr->second.modulesName2UUID[version].end()) {
    LOG_FILE_WARN(beehive) << "Unknown module '" << module << "', connection refused.";
    return std::make_pair("", "");
  }
  return std::make_pair(applicationPtr->second, modulePtr->second);
}

Config::Application SchemaService::getApplicationAndModuleUUID(uint32_t beehive, const std::string &application) {
  std::lock_guard<std::mutex> lock(_beehivesMutex);
  auto beehivePtr = SchemaService::_beehives.find(beehive);
  if (beehivePtr == SchemaService::_beehives.end()) {
    std::string error = "Beehive '" + std::to_string(beehive) + "' not defined, connection refused.";
    LOG_ERROR << error;
    throw InvalidSchemaException(error);
  }
  auto applicationPtr = beehivePtr->second.find(application);
  if (applicationPtr == beehivePtr->second.end()) {
    std::string error = "Application '" + application + "' not defined, connection refused.";
    LOG_FILE_WARN(beehive) << error;
    throw InvalidSchemaException(error);
  }
  return applicationPtr->second;
}

bool SchemaService::tryCreateBeehive(uint32_t beehive, const std::string &name, const std::string &email, const std::string &password) {
  char idDataset[36];
  int rc = SQLITE_ABORT;
  DAO::ConfigDAO configDAO(config.db);
  try {
    if (gatherer_create_honeycomb(config.db, "main", idDataset) == BEEHIVE_OK) {
      if (configDAO.beginTransaction("CreateBeehive", idDataset) == SQLITE_OK) {
        rc = configDAO.saveBeehive(beehive, idDataset);
        std::string salt = Services::Crypto::getSalt();
        std::string passwd = Services::Crypto::getPasswordHash(password, salt);
        if (rc == SQLITE_OK)
          rc = configDAO.saveDeveloper(beehive, name, email, passwd, salt);
        if (rc == SQLITE_OK)
          rc = gatherer_put_member(config.db, "main", idDataset, 1, "", 0, "", 0, "Beehive", 7);// TODO change beehive to int
        if (rc == SQLITE_OK)
          rc = gatherer_put_member(config.db, "main", idDataset, 2, "Management", 10, "Management", 10, "Management", 10);
        try {
          if (rc == SQLITE_OK)
            _schemaDAO.createSchema(beehive);
        } catch (std::exception &e) {
          LOG_ERROR << "Unable to create schema: " << beehive;
          rc = SQLITE_ABORT;
        }
        if (rc == SQLITE_OK) {
          rc = configDAO.commitTransaction();
          Config::synchronize();
        } else
          configDAO.rollbackTransaction();
      }
    }
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
  return rc == SQLITE_OK;
}

void SchemaService::createSchemaMaster() {
  _schemaDAO.createSchema(_beehive);
}

std::string SchemaService::readSchema(const std::string &uuid, const std::string &application, const std::string &kind, bool asCopy, int requestedVersion) {
  DAO::ConfigDAO configDAO(config.db);
  if (uuid == "00000000-0000-0000-0000-000000000000") {
    Config::Form form = Config::getForm(kind);
    auto formJson = nlohmann::json::parse(form.value);
    uuid_t uuid;
    char uuidString[37];
    uuid_generate_time_safe(uuid);
    uuid_unparse_lower(uuid, uuidString);
    formJson["uuid"] = std::string(uuidString, 36);
    if (kind != "applications") {
      formJson["layout"] = application;
    }
    std::ostringstream formStream;
    formStream << formJson;
    return "{\"schema\":" + form.form + ",\"startval\":" + formStream.str() + "}";
  } else {
    Config::Form form = Config::getForm(kind);
    if (kind == "applications") {
      std::optional<std::string> schema = configDAO.readApplication(_beehive, application);
      if (!schema)
        throw InvalidSchemaException("Application doesn't exist");
      return "{\"schema\":" + form.form + ",\"startval\":" + schema.value() + "}";
    } else {
      std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, application);
      if (!applicationSchema)
        throw InvalidSchemaException("Application doesn't exist");
      Config::Application app = nlohmann::json::parse(applicationSchema.value());
      std::optional<std::pair<std::string, std::string>> schema;
      if (kind == "entities")
        schema = configDAO.readEntity(_beehive, application, uuid, requestedVersion ? requestedVersion : app.version);
      else if (kind == "transactions")
        schema = configDAO.readTransaction(_beehive, application, uuid, requestedVersion ? requestedVersion : app.version);
      else if (kind == "roles")
        schema = configDAO.readRole(_beehive, application, uuid, requestedVersion ? requestedVersion : app.version);
      else if (kind == "modules")
        schema = configDAO.readModule(_beehive, application, uuid, requestedVersion ? requestedVersion : app.version);
      if (asCopy) {
        uuid_t uuid;
        char uuidString[37];
        uuid_generate_time_safe(uuid);
        uuid_unparse_lower(uuid, uuidString);
        auto schemaJson = nlohmann::json::parse(schema.value().second);
        std::string name = schemaJson["name"];
        schemaJson["uuid"] = std::string(uuidString, 36);
        schemaJson["name"] = name + " copy";
        std::ostringstream schemaStream;
        schemaStream << schemaJson;
        schema.value().second = schemaStream.str();
      }
      return "{\"schema\":" + form.form + ",\"startval\":" + schema.value().second + "}";
    }
  }
}

std::vector<std::string> SchemaService::readSchemas(const std::string &application, const std::string &kind) {
  DAO::ConfigDAO configDAO(config.db);
  std::vector<std::string> schemasVector;
  if (kind == "applications") {
    for (auto app : configDAO.readApplications(_beehive))
      schemasVector.push_back(app);
    return schemasVector;
  } else {
    if (application != "00000000-0000-0000-0000-000000000000") {
      std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, application);
      if (!applicationSchema)
        throw InvalidSchemaException("Application doesn't exist");
      Config::Application app = nlohmann::json::parse(applicationSchema.value());
      std::vector<std::pair<std::string, std::string>> schemas;
      if (kind == "entities")
        schemas = configDAO.readEntities(_beehive, application, app.version);
      else if (kind == "transactions")
        schemas = configDAO.readTransactions(_beehive, application, app.version);
      else if (kind == "roles")
        schemas = configDAO.readRoles(_beehive, application, app.version);
      else if (kind == "modules")
        schemas = configDAO.readModules(_beehive, application, app.version);
      for (auto schema : schemas)
        schemasVector.push_back(schema.second);
    } else {
      if (kind == "roles") {
        uuid_t uuid;
        char uuidString[37];
        uuid_generate_time_safe(uuid);
        uuid_unparse_lower(uuid, uuidString);
        Config::Role owner;
        owner.uuid = std::string(uuidString, 36);
        owner.name = "Owner";
        owner.layout = "00000000-0000-0000-0000-000000000000";
        owner.readmembers = std::make_shared<bool>(true);
        owner.managemembers = std::make_shared<bool>(true);
        owner.reademail = std::make_shared<bool>(false);
        owner.sharedataset = std::make_shared<bool>(true);
        owner.manageshare = std::make_shared<bool>(true);
        nlohmann::json ownerJson = owner;
        std::ostringstream ownerStream;
        ownerStream << ownerJson;
        schemasVector.push_back(ownerStream.str());
      }
    }
    return schemasVector;
  }
}

void SchemaService::updateSchema(const std::string &application, const std::string &uuid, const std::string &schema, const std::string &kind) {
  DAO::ConfigDAO configDAO(config.db);
  try {
    if (kind == "applications") {
      if (configDAO.readApplication(_beehive, application)) {
        std::string idDataset = configDAO.readDataset(_beehive);
        if (idDataset.length() != 0 && configDAO.beginTransaction("UpdateApplication", idDataset.c_str()) == SQLITE_OK) {
          if (configDAO.saveApplication(_beehive, application, schema) == SQLITE_OK) {
            configDAO.commitTransaction();
            Config::synchronize();
          } else
            configDAO.rollbackTransaction();
        }
      } else {
        std::string idDataset = configDAO.readDataset(_beehive);
        if (idDataset.length() != 0 && configDAO.beginTransaction("AddApplication", idDataset.c_str()) == SQLITE_OK) {
          if (configDAO.saveApplication(_beehive, application, schema) == SQLITE_OK) {
            Config::Application app = nlohmann::json::parse(schema);
            Config::Role owner;
            owner.uuid = app.defaultrole;
            owner.name = "Owner";
            owner.layout = app.uuid;
            owner.readmembers = std::make_shared<bool>(true);
            owner.managemembers = std::make_shared<bool>(true);
            owner.reademail = std::make_shared<bool>(false);
            owner.sharedataset = std::make_shared<bool>(true);
            owner.manageshare = std::make_shared<bool>(true);
            nlohmann::json ownerJson = owner;
            std::ostringstream ownerStream;
            ownerStream << ownerJson;
            std::string ownerSchemaString = ownerStream.str();
            if (configDAO.saveRole(_beehive, application, owner.uuid, ownerSchemaString, 0) == SQLITE_OK) {
              configDAO.commitTransaction();
              Config::synchronize();
            } else
              configDAO.rollbackTransaction();
          } else
            configDAO.rollbackTransaction();
        }
      }
    } else {
      std::optional<std::string> appSchema = configDAO.readApplication(_beehive, application);
      if (!appSchema)
        throw InvalidSchemaException("Application doesn't exist");
      std::string idDataset = configDAO.readDataset(_beehive);
      if (idDataset.length() != 0 && configDAO.beginTransaction("UpdateApplication", idDataset.c_str()) == SQLITE_OK) {
        Config::Application app = nlohmann::json::parse(appSchema.value());
        if (app.edited < app.version) {
          app.edited = app.version;
          nlohmann::json applicationJson = app;
          std::ostringstream applicationStream;
          applicationStream << applicationJson;
          std::string applicationSchemaString = applicationStream.str();
          if (configDAO.saveApplication(_beehive, application, applicationSchemaString) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update application", 5245);
          }
        }
        if (kind == "entities") {
          std::optional<std::pair<std::string, std::string>> oldSchema = configDAO.readEntity(_beehive, application, uuid, app.version);
          if (oldSchema) {
            Config::Entity oldEntity = nlohmann::json::parse(oldSchema->second);
            Config::Entity newEntity = nlohmann::json::parse(schema);
            oldEntity.name = newEntity.name;
            oldEntity.attributes = newEntity.attributes;
            nlohmann::json entityJson = oldEntity;
            std::ostringstream entityStream;
            entityStream << entityJson;
            std::string entitySchemaString = entityStream.str();
            std::optional<std::pair<std::string, std::string>> ownerSchema = configDAO.readRole(_beehive, application, app.defaultrole, app.version);
            Config::Role owner = nlohmann::json::parse(ownerSchema->second);
            for (Config::Role::Entity &e : owner.entities) {
              if (e.entity == oldEntity.uuid) {
                int max = 1;
                for (std::string a : e.attributes) {
                  int c = std::stoi(a);
                  max = max < c ? c : max;
                }
                for (auto attribute : oldEntity.attributes)
                  if (max < attribute.second.id)
                    e.attributes.push_back(std::to_string(attribute.second.id));
              }
            }
            nlohmann::json ownerJson = owner;
            std::ostringstream ownerStream;
            ownerStream << ownerJson;
            std::string ownerSchemaString = ownerStream.str();
            if (configDAO.saveRole(_beehive, application, app.defaultrole, ownerSchemaString, app.version) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
            if (configDAO.saveEntity(_beehive, application, uuid, entitySchemaString, app.version) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          } else {
            Config::Entity entity = nlohmann::json::parse(schema);
            nlohmann::json entityJson = entity;
            std::ostringstream entityStream;
            entityStream << entityJson;
            std::string entitySchemaString = entityStream.str();
            _entityDAO.createEntity(entity);
            std::optional<std::pair<std::string, std::string>> ownerSchema = configDAO.readRole(_beehive, application, app.defaultrole, app.version);
            Config::Role owner = nlohmann::json::parse(ownerSchema->second);
            Config::Role::Entity roleEntity;
            roleEntity.entity = entity.uuid;
            roleEntity.entity_name = entity.name;
            for (auto attribute : entity.attributes)
              roleEntity.attributes.push_back(std::to_string(attribute.second.id));
            owner.entities.push_back(roleEntity);
            nlohmann::json ownerJson = owner;
            std::ostringstream ownerStream;
            ownerStream << ownerJson;
            std::string ownerSchemaString = ownerStream.str();
            if (configDAO.saveRole(_beehive, application, app.defaultrole, ownerSchemaString, app.version) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
            if (configDAO.saveEntity(_beehive, application, uuid, entitySchemaString, app.version) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          }
        } else if (kind == "transactions") {
          std::optional<std::pair<std::string, std::string>> oldSchema = configDAO.readTransaction(_beehive, application, uuid, app.version);
          if (!oldSchema) {
            Config::Transaction transaction = nlohmann::json::parse(schema);
            std::optional<std::pair<std::string, std::string>> ownerSchema = configDAO.readRole(_beehive, application, app.defaultrole, app.version);
            Config::Role owner = nlohmann::json::parse(ownerSchema->second);
            owner.transactions.push_back(transaction.uuid);
            nlohmann::json ownerJson = owner;
            std::ostringstream ownerStream;
            ownerStream << ownerJson;
            std::string ownerSchemaString = ownerStream.str();
            if (configDAO.saveRole(_beehive, application, app.defaultrole, ownerSchemaString, app.version) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          }
          if (configDAO.saveTransaction(_beehive, application, uuid, schema, app.version) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update owner", 5245);
          }
        } else if (kind == "roles") {
          if (configDAO.saveRole(_beehive, application, uuid, schema, app.version) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update owner", 5245);
          }
        } else if (kind == "modules") {
          if (configDAO.saveModule(_beehive, application, uuid, schema, app.version) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update owner", 5245);
          }
        }
        configDAO.commitTransaction();
        Config::synchronize();
      }
    }
    if (kind == "applications") {
      std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, application);
      Config::Application appn = SchemaService::loadApplicationSchemas(config.db, _beehive, applicationSchema.value());
      std::lock_guard<std::mutex> lock(_beehivesMutex);
      _beehives[_beehive][appn.uuid] = appn;
      _applications[_beehive][appn.name] = appn.uuid;
    } else {
      std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, application);
      Config::Application appn = SchemaService::loadApplicationSchemas(config.db, _beehive, applicationSchema.value());
      std::lock_guard<std::mutex> lock(_beehivesMutex);
      _beehives[_beehive][appn.uuid] = appn;
      _applications[_beehive][appn.name] = appn.uuid;
    }
  } catch (DAO::SQL::SQLException &e) {
    LOG_ERROR << e.what();
    throw ServiceException("A error occurred when saving the schema", 0);
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
}

bool SchemaService::removeSchema(const std::string &uuid, const std::string &application, const std::string &kind) {
  DAO::ConfigDAO configDAO(config.db);
  try {
    if (kind == "applications") {
      std::string idDataset = configDAO.readDataset(_beehive);
      if (idDataset.length() != 0 && configDAO.beginTransaction("RemoveApplication", idDataset.c_str()) == SQLITE_OK) {
        std::optional<std::string> appSchema = configDAO.readApplication(_beehive, application);
        if (!appSchema)
          throw InvalidSchemaException("Application doesn't exist");
        Config::Application app = nlohmann::json::parse(appSchema.value());
        for (int i = 0; i <= app.version; i++) {
          for (auto e : configDAO.readEntities(_beehive, uuid, i)) {
            Config::Entity entity = nlohmann::json::parse(e.second);
            _entityDAO.dropEntity(entity.id);
            configDAO.removeEntity(_beehive, application, entity.uuid, i);
          }
          for (auto t : configDAO.readTransactions(_beehive, uuid, i)) {
            Config::Transaction transaction = nlohmann::json::parse(t.second);
            configDAO.removeTransaction(_beehive, application, transaction.uuid, i);
          }
          for (auto r : configDAO.readRoles(_beehive, uuid, i)) {
            Config::Role role = nlohmann::json::parse(r.second);
            configDAO.removeRole(_beehive, application, role.uuid, i);
          }
          for (auto m : configDAO.readModules(_beehive, uuid, i)) {
            Config::Module module = nlohmann::json::parse(m.second);
            configDAO.removeModule(_beehive, application, module.uuid, i);
          }
        }
        configDAO.removeApplication(_beehive, application);
        configDAO.commitTransaction();
        Config::synchronize();
      }
    } else {
      std::string idDataset = configDAO.readDataset(_beehive);
      if (idDataset.length() != 0 && configDAO.beginTransaction("UpdateApplication", idDataset.c_str()) == SQLITE_OK) {
        std::optional<std::string> appSchema = configDAO.readApplication(_beehive, application);
        if (!appSchema)
          throw InvalidSchemaException("Application doesn't exist");
        Config::Application app = nlohmann::json::parse(appSchema.value());
        if (app.edited < app.version) {
          app.edited = app.version;
          nlohmann::json applicationJson = app;
          std::ostringstream applicationStream;
          applicationStream << applicationJson;
          std::string applicationSchemaString = applicationStream.str();
          if (configDAO.saveApplication(_beehive, application, applicationSchemaString) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update application", 5245);
          }
        }
        if (kind == "entities") {
          try {
            std::optional<std::pair<std::string, std::string>> schema = configDAO.readEntity(_beehive, application, uuid, app.version);
            if (schema) {
              Config::Entity entity = nlohmann::json::parse(schema->second);
              _entityDAO.dropEntity(entity.id);
            }
          } catch (DAO::SQL::SQLException &e) {
            if (e.errorCode() != 1051)
              throw e;
          }
          std::optional<std::pair<std::string, std::string>> ownerSchema = configDAO.readRole(_beehive, application, app.defaultrole, app.version);
          Config::Role owner = nlohmann::json::parse(ownerSchema->second);
          for (std::vector<Config::Role::Entity>::iterator current = owner.entities.begin(); current != owner.entities.end(); current++) {
            if (current->entity == uuid) {
              owner.entities.erase(current);
              break;
            }
          }
          nlohmann::json ownerJson = owner;
          std::ostringstream ownerStream;
          ownerStream << ownerJson;
          std::string ownerSchemaString = ownerStream.str();
          if (configDAO.saveRole(_beehive, application, app.defaultrole, ownerSchemaString, app.version) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update owner", 5245);
          }
          configDAO.removeEntity(_beehive, application, uuid, app.version);
        } else if (kind == "transactions") {
          std::optional<std::pair<std::string, std::string>> ownerSchema = configDAO.readRole(_beehive, application, app.defaultrole, app.version);
          Config::Role owner = nlohmann::json::parse(ownerSchema->second);
          for (std::vector<std::string>::iterator current = owner.transactions.begin(); current != owner.transactions.end(); current++) {
            if (*current == uuid) {
              owner.transactions.erase(current);
              break;
            }
          }
          nlohmann::json ownerJson = owner;
          std::ostringstream ownerStream;
          ownerStream << ownerJson;
          std::string ownerSchemaString = ownerStream.str();
          if (configDAO.saveRole(_beehive, application, app.defaultrole, ownerSchemaString, app.version) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update owner", 5245);
          }
          configDAO.removeTransaction(_beehive, application, uuid, app.version);
        } else if (kind == "roles") {
          if (uuid != app.defaultrole) {
            configDAO.removeRole(_beehive, application, uuid, app.version);
          } else {
            return false;
          }
        } else if (kind == "modules") {
          configDAO.removeModule(_beehive, application, uuid, app.version);
        }
      }
      configDAO.commitTransaction();
      Config::synchronize();
    }
    if (kind == "applications") {
      std::lock_guard<std::mutex> lock(_beehivesMutex);
      _beehives[_beehive].erase(uuid);
      _applications[_beehive].erase(uuid);
    } else {
      std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, application);
      Config::Application appn = SchemaService::loadApplicationSchemas(config.db, _beehive, applicationSchema.value());
      std::lock_guard<std::mutex> lock(_beehivesMutex);
      _beehives[_beehive][appn.uuid] = appn;
      _applications[_beehive][appn.name] = appn.uuid;
    }
    return true;
  } catch (DAO::SQL::SQLException &e) {
    LOG_ERROR << e.errorCode() << ": " << e.what();
    throw ServiceException("A error occurred when deleting the the schema", 0);
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
}

std::optional<std::string> SchemaService::publishApplication(const std::string &uuid) {
  DAO::ConfigDAO configDAO(config.db);
  try {
    std::string idDataset = configDAO.readDataset(_beehive);
    if (idDataset.length() != 0 && configDAO.beginTransaction("PublishApplication", idDataset.c_str()) == SQLITE_OK) {
      std::optional<std::string> appSchema = configDAO.readApplication(_beehive, uuid);
      Config::Application app = nlohmann::json::parse(appSchema.value());
      for (auto e : configDAO.readEntities(_beehive, uuid, app.version)) {
        if (configDAO.saveEntity(_beehive, uuid, e.first, e.second, app.version + 1) != SQLITE_OK) {
          configDAO.rollbackTransaction();
          throw ServiceException("Unable to update owner", 5245);
        }
      }
      for (auto t : configDAO.readTransactions(_beehive, uuid, app.version)) {
        if (configDAO.saveTransaction(_beehive, uuid, t.first, t.second, app.version + 1) != SQLITE_OK) {
          configDAO.rollbackTransaction();
          throw ServiceException("Unable to update owner", 5245);
        }
      }
      for (auto r : configDAO.readRoles(_beehive, uuid, app.version)) {
        if (configDAO.saveRole(_beehive, uuid, r.first, r.second, app.version + 1) != SQLITE_OK) {
          configDAO.rollbackTransaction();
          throw ServiceException("Unable to update owner", 5245);
        }
      }
      for (auto m : configDAO.readModules(_beehive, uuid, app.version)) {
        if (configDAO.saveModule(_beehive, uuid, m.first, m.second, app.version + 1) != SQLITE_OK) {
          configDAO.rollbackTransaction();
          throw ServiceException("Unable to update owner", 5245);
        }
      }
      app.version = app.version + 1;
      nlohmann::json applicationJson = app;
      std::ostringstream applicationStream;
      applicationStream << applicationJson;
      std::string applicationSchemaString = applicationStream.str();
      if (configDAO.saveApplication(_beehive, uuid, applicationSchemaString) != SQLITE_OK) {
        configDAO.rollbackTransaction();
        throw ServiceException("Unable to update application", 5245);
      }
      configDAO.commitTransaction();
      Config::synchronize();
      {
        std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, uuid);
        Config::Application appn = SchemaService::loadApplicationSchemas(config.db, _beehive, applicationSchema.value());
        std::lock_guard<std::mutex> lock(_beehivesMutex);
        _beehives[_beehive][appn.uuid] = appn;
        _applications[_beehive][appn.name] = appn.uuid;
      }
      return std::make_optional(applicationSchemaString);
    } else
      return std::nullopt;
  } catch (DAO::SQL::SQLException &e) {
    LOG_ERROR << e.errorCode() << ": " << e.what();
    throw ServiceException("A error occurred when publishing the application", 0);
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
}

std::optional<std::string> SchemaService::downgradeApplication(const std::string &uuid) {
  DAO::ConfigDAO configDAO(config.db);
  try {
    std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, uuid);
    Config::Application app = nlohmann::json::parse(applicationSchema.value());
    if (app.edited < app.version) {
      std::string idDataset = configDAO.readDataset(_beehive);
      if (idDataset.length() != 0 && configDAO.beginTransaction("DowngradeApplication", idDataset.c_str()) == SQLITE_OK) {
        for (; app.edited < app.version; app.version--)
          configDAO.removeVersion(_beehive, uuid, app.version);
        app.version = app.edited;
        nlohmann::json applicationJson = app;
        std::ostringstream applicationStream;
        applicationStream << applicationJson;
        std::string applicationSchemaString = applicationStream.str();
        if (configDAO.saveApplication(_beehive, uuid, applicationSchemaString) != SQLITE_OK) {
          configDAO.rollbackTransaction();
          throw ServiceException("Unable to update application", 5245);
        }
        configDAO.commitTransaction();
        Config::synchronize();
        {
          std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, uuid);
          Config::Application appn = SchemaService::loadApplicationSchemas(config.db, _beehive, applicationSchema.value());
          std::lock_guard<std::mutex> lock(_beehivesMutex);
          _beehives[_beehive][appn.uuid] = appn;
          _applications[_beehive][appn.name] = appn.uuid;
        }
        return applicationSchemaString;
      } else
        return "";
    } else
      return applicationSchema.value();
  } catch (DAO::SQL::SQLException &e) {
    LOG_ERROR << e.errorCode() << ": " << e.what();
    throw ServiceException("A error occurred when publishing the application", 0);
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
}

std::pair<std::string, std::string> SchemaService::exportApplication(const std::string &uuid) {
  DAO::ConfigDAO configDAO(config.db);
  try {
    std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, uuid);
    Config::Application app = nlohmann::json::parse(applicationSchema.value());
    std::ostringstream body;
    std::string comma0 = "";
    std::string comma1;
    std::string name = app.name;
    body << "{";
    body << "\"uuid\":\"" << app.uuid << "\",";
    body << "\"name\":\"" << app.name << "\",";
    body << "\"defaultrole\":\"" << app.defaultrole << "\",";
    body << "\"version\":" << app.version - 1 << ",";
    body << "\"versions\":[";
    for (int i = 0; i <= app.version - 1; i++) {
      body << comma0 << "{";
      body << "\"version\":" << i << ",";
      body << "\"entities\":[";
      comma1 = "";
      for (auto e : configDAO.readEntities(_beehive, uuid, i)) {
        body << comma1 << e.second;
        comma1 = ",";
      }
      body << "],";
      body << "\"transactions\":[";
      comma1 = "";
      for (auto t : configDAO.readTransactions(_beehive, uuid, i)) {
        body << comma1 << t.second;
        comma1 = ",";
      }
      body << "],";
      body << "\"roles\":[";
      comma1 = "";
      for (auto r : configDAO.readRoles(_beehive, uuid, i)) {
        body << comma1 << r.second;
        comma1 = ",";
      }
      body << "],";
      body << "\"modules\":[";
      comma1 = "";
      for (auto m : configDAO.readModules(_beehive, uuid, i)) {
        body << comma1 << m.second;
        comma1 = ",";
      }
      body << "]";
      body << "}";
      comma0 = ",";
    }
    body << "]";
    body << "}";
    return std::make_pair(name + " V" + std::to_string(app.version - 1), body.str());
  } catch (DAO::SQL::SQLException &e) {
    LOG_ERROR << e.errorCode() << ": " << e.what();
    throw ServiceException("A error occurred when publishing the application", 0);
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
}

std::pair<std::string, int> SchemaService::importApplication(const std::string &application) {
  DAO::ConfigDAO configDAO(config.db);
  try {
    std::string idDataset = configDAO.readDataset(_beehive);
    if (idDataset.length() != 0 && configDAO.beginTransaction("ImportApplication", idDataset.c_str()) == SQLITE_OK) {
      auto applicationJson = nlohmann::json::parse(application);
      std::string uuid = applicationJson["uuid"];
      std::string name = applicationJson["name"];
      std::string defaultrole = applicationJson["defaultrole"];
      int version = applicationJson["version"];
      int startVersion = -1;
      int responseCode = 409;
      std::string responseSchema;
      std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, uuid);
      if (applicationSchema) {
        Config::Application app = nlohmann::json::parse(applicationSchema.value());
        startVersion = app.version;
        if (startVersion < version) {
          app.name = name;
          app.defaultrole = defaultrole;
          app.version = version;
          nlohmann::json applicationJson = app;
          std::ostringstream applicationStream;
          applicationStream << applicationJson;
          responseSchema = applicationStream.str();
          if (configDAO.saveApplication(_beehive, uuid, responseSchema) != SQLITE_OK) {
            configDAO.rollbackTransaction();
            throw ServiceException("Unable to update application", 5245);
          }
          responseCode = 200;
        } else {
          LOG_DEBUG << "Version " << version << " already exists, current version is " << startVersion;
          return std::make_pair(applicationSchema.value(), 409);
        }
      } else {
        Config::Application app;
        app.uuid = uuid;
        app.name = name;
        app.defaultrole = defaultrole;
        app.version = version;
        app.edited = version;
        nlohmann::json applicationJson = app;
        std::ostringstream applicationStream;
        applicationStream << applicationJson;
        responseSchema = applicationStream.str();
        if (configDAO.saveApplication(_beehive, uuid, responseSchema) != SQLITE_OK) {
          configDAO.rollbackTransaction();
          throw ServiceException("Unable to update application", 5245);
        }
        responseCode = 201;
      }
      for (auto applicationVersion : applicationJson["versions"].items()) {
        int currentVersion = applicationVersion.value()["version"];
        if (startVersion < currentVersion) {
          auto entities = applicationVersion.value()["entities"];
          for (auto entity : entities.items()) {
            std::string entityUUID = entity.value()["uuid"];
            std::ostringstream entityStream;
            entityStream << entity.value();
            std::string entityStreamString = entityStream.str();
            if (configDAO.saveEntity(_beehive, uuid, entityUUID, entityStreamString, currentVersion) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          }
          auto transactions = applicationVersion.value()["transactions"];
          for (auto transaction : transactions.items()) {
            std::string transactionUUID = transaction.value()["uuid"];
            std::ostringstream transactionStream;
            transactionStream << transaction.value();
            std::string transactionStreamString = transactionStream.str();
            if (configDAO.saveTransaction(_beehive, uuid, transactionUUID, transactionStreamString, currentVersion) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          }
          auto roles = applicationVersion.value()["roles"];
          for (auto role : roles.items()) {
            std::string roleUUID = role.value()["uuid"];
            std::ostringstream roleStream;
            roleStream << role.value();
            std::string roleStreamString = roleStream.str();
            if (configDAO.saveRole(_beehive, uuid, roleUUID, roleStreamString, currentVersion) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          }
          auto modules = applicationVersion.value()["modules"];
          for (auto module : modules.items()) {
            std::string moduleUUID = module.value()["uuid"];
            std::ostringstream moduleStream;
            moduleStream << module.value();
            std::string moduleStreamString = moduleStream.str();
            if (configDAO.saveModule(_beehive, uuid, moduleUUID, moduleStreamString, currentVersion) != SQLITE_OK) {
              configDAO.rollbackTransaction();
              throw ServiceException("Unable to update owner", 5245);
            }
          }
        }
      }
      configDAO.commitTransaction();
      Config::synchronize();
      {
        std::optional<std::string> applicationSchema = configDAO.readApplication(_beehive, uuid);
        Config::Application appn = SchemaService::loadApplicationSchemas(config.db, _beehive, applicationSchema.value());
        std::lock_guard<std::mutex> lock(_beehivesMutex);
        _beehives[_beehive][appn.uuid] = appn;
        _applications[_beehive][appn.name] = appn.uuid;
      }
      return std::make_pair(responseSchema, responseCode);
    } else
      return std::make_pair("", 500);
  } catch (DAO::SQL::SQLException &e) {
    LOG_ERROR << e.errorCode() << ": " << e.what();
    throw ServiceException("A error occurred when publishing the application", 0);
  } catch (std::exception &e) {
    configDAO.rollbackTransaction();
    throw e;
  }
}

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
