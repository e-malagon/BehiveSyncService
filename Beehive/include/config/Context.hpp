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

#include <uuid/uuid.h>

#include <config/Entity.hpp>
#include <config/Role.hpp>
#include <config/Transaction.hpp>
#include <config/Module.hpp>
#include <json/Common.hpp>
#include <json/json.hpp>
#include <unordered_map>
#include <unordered_set>

namespace Beehive {
namespace Services {
namespace Config {
using nlohmann::json;

struct Context {
    std::string uuid;
    std::string name;
    std::string defaultrole;
    uint32_t version;

    std::unordered_map<std::string, Entity, Utils::IHasher, Utils::IEqualsComparator> entities;
    std::unordered_map<std::string, Transaction, Utils::IHasher, Utils::IEqualsComparator> transactions;
    std::unordered_map<std::string, Role, Utils::IHasher, Utils::IEqualsComparator> roles;
    std::unordered_map<std::string, Module, Utils::IHasher, Utils::IEqualsComparator> modules;

    std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator> entitiesName2UUID;
    std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator> transactionsName2UUID;
    std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator> rolesName2UUID;
    std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator> modulesName2UUID;

    void check() {
        std::unordered_set<std::string> uuids;
        std::unordered_set<std::string> names;
        uuid_t uuidt;
        if (uuid_parse(uuid.c_str(), uuidt) != 0)
            throw InvalidSchemaException(uuid + " is not a valid uuid.");
        uuids.emplace(uuid);

        names.clear();
        for (auto &entity : entities) {
            uuid_t uuid;
            if (uuid_parse(entity.second.uuid.c_str(), uuid) != 0)
                throw InvalidSchemaException(entity.second.uuid + " is not a valid uuid.");
            if(uuids.find(entity.second.uuid) != uuids.end())
                throw InvalidSchemaException(entity.second.uuid + " is duplicated.");
            uuids.emplace(entity.second.uuid);
            if (entity.second.name.length() == 0)
                throw InvalidSchemaException("Entity name is empty.");
            if(names.find(entity.second.name) != names.end())
                throw InvalidSchemaException("Entity name " + entity.second.name + " is already in use.");
            names.emplace(entity.second.name);
            std::unordered_set<int> subIds;
            std::unordered_set<std::string> subNames;
            for (auto key: entity.second.jsonKeys) {
                if (subIds.find(key.id) != subIds.end())
                    throw InvalidSchemaException("Entity " + entity.second.name + " contains duplicated key id " + std::to_string(key.id) + ".");
                subIds.emplace(key.id);
                if (key.name.length() == 0)
                    throw InvalidSchemaException("Entity " + entity.second.name + " contains empty key name.");
                if (subNames.find(key.name) != subNames.end())
                    throw InvalidSchemaException("Entity " + entity.second.name + " contains duplicated key or attribute name " + key.name + ".");
                subNames.emplace(key.name);
            }
            subIds.clear();
            for (auto attribute: entity.second.jsonAttributes) {
                if (subIds.find(attribute.id) != subIds.end())
                    throw InvalidSchemaException("Entity " + entity.second.name + " contains duplicated attribute id " + std::to_string(attribute.id) + ".");
                subIds.emplace(attribute.id);
                if (attribute.name.length() == 0)
                    throw InvalidSchemaException("Entity " + entity.second.name + " contains empty attribute name.");
                if (subNames.find(attribute.name) != subNames.end())
                    throw InvalidSchemaException("Entity " + entity.second.name + " contains duplicated key or attribute name " + attribute.name + ".");
                subNames.emplace(attribute.name);
            }
        }

        names.clear();
        for (auto &transaction : transactions) {
            uuid_t uuid;
            if (uuid_parse(transaction.second.uuid.c_str(), uuid) != 0)
                throw InvalidSchemaException(transaction.second.uuid + " is not a valid uuid.");
            if(uuids.find(transaction.second.uuid) != uuids.end())
                throw InvalidSchemaException(transaction.second.uuid + " is duplicated.");
            uuids.emplace(transaction.second.uuid);
            if (transaction.second.name.length() == 0)
                throw InvalidSchemaException("Transaction name is empty.");
            if(names.find(transaction.second.name) != names.end())
                throw InvalidSchemaException("Transaction name " + transaction.second.name + " is already in use.");
            names.emplace(transaction.second.name);
            for (auto &entity : transaction.second.entities) {
                auto entityPtr = entities.find(entity.entity);
                if (entityPtr == entities.end())
                    throw InvalidSchemaException("Unknown entity " + entity.entity + ".");
                for (int column : entity.update) {
                    if (entityPtr->second.attributesId2Name.find(column) == entityPtr->second.attributesId2Name.end())
                        throw InvalidSchemaException("Unknown column number " + std::to_string(column) + " for entity " + entityPtr->second.name + ".");
                }
            }
        }

        names.clear();
        for (auto &role : roles) {
            uuid_t uuid;
            if (uuid_parse(role.second.uuid.c_str(), uuid) != 0)
                throw InvalidSchemaException(role.second.uuid + " is not a valid uuid.");
            if(uuids.find(role.second.uuid) != uuids.end())
                throw InvalidSchemaException(role.second.uuid + " is duplicated.");
            uuids.emplace(role.second.uuid);
            if (role.second.name.length() == 0)
                throw InvalidSchemaException("Role name is empty.");
            if(names.find(role.second.name) != names.end())
                throw InvalidSchemaException("Role name " + role.second.name + " is already in use.");
            names.emplace(role.second.name);
            for (auto &entity : role.second.entities) {
                auto entityPtr = entities.find(entity.entity);
                if (entityPtr == entities.end())
                    throw InvalidSchemaException("Unknown entity " + entity.entity + ".");
                for (int column : entity.attributes) {
                    if (entityPtr->second.attributesId2Name.find(column) == entityPtr->second.attributesId2Name.end())
                        throw InvalidSchemaException("Unknown column number " + std::to_string(column) + " for entity " + entityPtr->second.name + ".");
                }
            }
        }

        names.clear();
        for (auto &module : modules) {
            uuid_t uuid;
            if (uuid_parse(module.second.uuid.c_str(), uuid) != 0)
                throw InvalidSchemaException(module.second.uuid + " is not a valid uuid.");
            if(uuids.find(module.second.uuid) != uuids.end())
                throw InvalidSchemaException(module.second.uuid + " is duplicated.");
            uuids.emplace(module.second.uuid);
            if (module.second.name.length() == 0)
                throw InvalidSchemaException("Module name is empty.");
            if(names.find(module.second.name) != names.end())
                throw InvalidSchemaException("Module name " + module.second.name + " is already in use.");
            names.emplace(module.second.name);
            for (auto &entity : module.second.entities) {
                auto entityPtr = entities.find(entity.entity);
                if (entityPtr == entities.end())
                    throw InvalidSchemaException("Unknown entity " + entity.entity + ".");
                for (int column : entity.attributes) {
                    if (entityPtr->second.attributesId2Name.find(column) == entityPtr->second.attributesId2Name.end())
                        throw InvalidSchemaException("Unknown column number " + std::to_string(column) + " for entity " + entityPtr->second.name + ".");
                }
            }
        }
    }
};

} /* namespace Config */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {

void from_json(const json &j, Beehive::Services::Config::Context &x);
void to_json(json &j, const Beehive::Services::Config::Context &x);

inline void from_json(const json &j, Beehive::Services::Config::Context &x) {
    x.uuid = j.at("uuid").get<std::string>();
    x.name = j.at("name").get<std::string>();
    x.defaultrole = Beehive::Services::Config::get_optional<std::string>(j, "defaultrole", "00000000-0000-0000-0000-000000000000");
    x.version = Beehive::Services::Config::get_optional<uint32_t>(j, "version", 1);
    std::vector<Entity> contextEntities = Beehive::Services::Config::get_optional<std::vector<Entity>>(j, "entities", std::vector<Entity>());
    for (Entity entity: contextEntities) {
        x.entities.emplace(entity.uuid, entity);
        x.entitiesName2UUID.emplace(entity.name, entity.uuid);
    }
    std::vector<Transaction> contextTransactions = Beehive::Services::Config::get_optional<std::vector<Transaction>>(j, "transactions", std::vector<Transaction>());
    for (Transaction transaction: contextTransactions) {
        x.transactions.emplace(transaction.uuid, transaction);
        x.transactionsName2UUID.emplace(transaction.name, transaction.uuid);
    }
    std::vector<Role> contextRoles = Beehive::Services::Config::get_optional<std::vector<Role>>(j, "roles", std::vector<Role>());
    for (Role role: contextRoles) {
        x.roles.emplace(role.uuid, role);
        x.rolesName2UUID.emplace(role.name, role.uuid);
    }
    std::vector<Module> contextModules = Beehive::Services::Config::get_optional<std::vector<Module>>(j, "modules", std::vector<Module>());
    for (Module module: contextModules) {
        x.modules.emplace(module.uuid, module);
        x.modulesName2UUID.emplace(module.name, module.uuid);
    }
}

inline void to_json(json &j, const Beehive::Services::Config::Context &x) {
    j = json::object();
    j["uuid"] = x.uuid;
    j["name"] = x.name;
    j["defaultrole"] = x.defaultrole;
    j["version"] = x.version;
    std::vector<Entity> contextEntities;
    for (auto entity : x.entities)
        contextEntities.push_back(entity.second);
    j["entities"] = contextEntities;
    std::vector<Transaction> contextTransactions;
    for (auto transaction : x.transactions)
        contextTransactions.push_back(transaction.second);
    j["transactions"] = contextTransactions;
    std::vector<Role> contextRoles;
    for (auto role : x.roles)
        contextRoles.push_back(role.second);
    j["roles"] = contextRoles;
    std::vector<Module> contextModules;
    for (auto module : x.modules)
        contextModules.push_back(module.second);
    j["modules"] = contextModules;
}
} /* namespace nlohmann */
