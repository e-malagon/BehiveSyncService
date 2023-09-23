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


#include <config/Context.hpp>
#include <config/Entity.hpp>
#include <crypto/Crypto.hpp>
#include <dao/Storage.hpp>
#include <json/json.hpp>
#include <nanolog/NanoLog.hpp>
#include <regex>
#include <services/SchemaService.hpp>
#include <services/ServiceException.hpp>
#include <sstream>
#include <unordered_map>

namespace Beehive {
namespace Services {

std::string SchemaService::postContext(const std::string &body) {
    nlohmann::json bodyJson = nlohmann::json::parse(body);
    Config::Context context;
    nlohmann::from_json(bodyJson, context);
    context.check();
    DAO::Storage::createContext(context.uuid);
    nlohmann::json storeJson;
    nlohmann::to_json(storeJson, context);
    std::string contextBody = storeJson.dump();
    DAO::Storage::putValue("Schema", contextBody, context.uuid);
    return contextBody;
}

std::string SchemaService::getContext(const std::string &context) {
    std::string contextBody;
    if (!DAO::Storage::getValue("Schema", &contextBody, context))
        throw StorageErrorException("Schema of context " + context + " was not found.");
    return contextBody;
}

std::string SchemaService::getContexts() {
    nlohmann::json storeJson = DAO::Storage::getContexts();
    std::string contextBody = storeJson.dump();
    return contextBody;
}

std::string SchemaService::putContext(const std::string &body) {
    nlohmann::json bodyJson = nlohmann::json::parse(body);
    Config::Context context;
    nlohmann::from_json(bodyJson, context);
    context.check();
    nlohmann::json storeJson;
    nlohmann::to_json(storeJson, context);
    std::string contextBody = storeJson.dump();
    DAO::Storage::putValue("Schema", contextBody, context.uuid);
    return contextBody;
}

void SchemaService::deleteContext(const std::string &uuid) {
    DAO::Storage::deleteContext(uuid);
}

void SchemaService::linkContext(const std::string &context, const std::string &link) {
    std::regex exp("^</context/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/versions/([0-9]+)>;\\s+rel=\"publish\"$");
    std::smatch match;
    if (std::regex_match(link, match, exp) && match.size() == 3 && context == match[1]) {
        std::string version = match[2];
        if(0 < std::stoi(version)) {
            std::string contextBody;
            if (!DAO::Storage::getValue("Schema", &contextBody, context))
                throw StorageErrorException("Context " + context + " was not found.");
            DAO::Storage::putValue("Schema." + version, contextBody, context);
        } else {
            throw InvalidRequestException("Version must be greater than 0.");
        }
    } else {
        throw InvalidRequestException("Link header is not valid.");
    }
}

void SchemaService::unlinkContext(const std::string &context, const std::string &link) {
    std::regex exp("^</context/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})/versions/([0-9]+)>;\\s+rel=\"publish\"$");
    std::smatch match;
    if (std::regex_match(link, match, exp) && match.size() == 3 && context == match[1]) {
        std::string version = match[2];
        DAO::Storage::deleteValue("Schema." + version, context);
    } else {
        throw InvalidRequestException("Link header is not valid");
    }
}

std::string SchemaService::getLinkedVersions(const std::string &context) {
    std::vector<std::pair<std::string, std::string>> values;
    DAO::Storage::getValues("Schema.", values, context);
    std::vector<std::string> versions;
    for(auto value: values)
        versions.push_back(value.first.substr(7));
    nlohmann::json storeJson = versions;
    std::string contextBody = storeJson.dump();
    return contextBody;
}

std::string SchemaService::getLinkedVersion(const std::string &context, const std::string &version) {
    std::string contextBody;
    if (!DAO::Storage::getValue("Schema." + version, &contextBody, context)) {
        if (!DAO::Storage::getValue("Schema", &contextBody, context))
            throw StorageErrorException("Schema of context " + context + " was not found.");
        nlohmann::json bodyJson = nlohmann::json::parse(contextBody);
        Config::Context ctx;
        nlohmann::from_json(bodyJson, ctx);
        throw StorageErrorException("Schema version " + version + " of context " + ctx.name + " was not found.");
    }
    return contextBody;
}

Config::Context SchemaService::getContext(const std::string &context, int version) {
    std::string contextBody;
    if(version == 0) {
        if (!DAO::Storage::getValue("Schema", &contextBody, context))
            throw StorageErrorException("Schema of context " + context + " was not found.");
    } else {
        if (!DAO::Storage::getValue("Schema." + std::to_string(version), &contextBody, context))
            throw StorageErrorException("Schema version " + std::to_string(version) + " of context " + context + " was not found.");
    }
    nlohmann::json bodyJson = nlohmann::json::parse(contextBody);
    Config::Context ctx;
    nlohmann::from_json(bodyJson, ctx);
    return ctx;
}

Config::Context SchemaService::loadContextSchemas(const std::string schema) {
    Config::Context context = nlohmann::json::parse(schema);
    /*  DAO::ConfigDAO configDAO(config.db);
  for (int i = 0; i <= context.version; i++) {
    for (auto e : configDAO.readEntities(beehive, context.uuid, i)) {
      Config::Entity entity = nlohmann::json::parse(e.second);
      context.entities[i].emplace(entity.uuid, entity);
      context.entitiesName2UUID[i].emplace(entity.name, entity.uuid);
    }
    for (auto t : configDAO.readTransactions(beehive, context.uuid, i)) {
      Config::Transaction transaction = nlohmann::json::parse(t.second);
      context.transactions[i].emplace(transaction.uuid, transaction);
      context.transactionsName2UUID[i].emplace(transaction.name, transaction.uuid);
    }
    for (auto r : configDAO.readRoles(beehive, context.uuid, i)) {
      Config::Role role = nlohmann::json::parse(r.second);
      context.roles[i].emplace(role.uuid, role);
      context.rolesName2UUID[i].emplace(role.name, role.uuid);
    }
    for (auto m : configDAO.readModules(beehive, context.uuid, i)) {
      Config::Module module = nlohmann::json::parse(m.second);
      context.modules[i].emplace(module.uuid, module);
      context.modulesName2UUID[i].emplace(module.name, module.uuid);
    }
    std::unordered_map<std::string, std::unordered_map<std::string, Config::Entity::Transaction, Utils::IHasher, Utils::IEqualsComparator>> transactions;
    for (auto &transaction : context.transactions[i]) {
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
    for (auto &entity : context.entities[i])
      entity.second.transactions = transactions[entity.second.uuid];
  }*/
    return context;
}

} /* namespace Services */
} /* namespace Beehive */
