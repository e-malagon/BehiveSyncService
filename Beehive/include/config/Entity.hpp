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

#include <json/Common.hpp>
#include <sqlite/Types.hpp>
#include <string/ICaseMap.hpp>
#include <validation/Validator.hpp>

#include <json/json.hpp>
#include <unordered_set>

#include <config/Transaction.hpp>

namespace Beehive {
namespace Services {
namespace Config {

struct Entity {
    struct Key {
        int id;
        std::string name;
        SqLite::AttributeType type;

        static SqLite::AttributeType getType(std::string description) {
            if (description == "Integer")
                return SqLite::AttributeType::Integer;
            else if (description == "Text")
                return SqLite::AttributeType::Text;
            else if (description == "Blob")
                return SqLite::AttributeType::Blob;
            else if (description == "UuidV1")
                return SqLite::AttributeType::UuidV1;
            else if (description == "Uuid")
                return SqLite::AttributeType::UuidV4;
            else
                throw InvalidSchemaException("Unknown data type " + description);
        }

        static std::string getTypeDescription(SqLite::AttributeType type) {
            switch (type) {
                case SqLite::AttributeType::Integer:
                    return "Integer";
                case SqLite::AttributeType::Text:
                    return "Text";
                case SqLite::AttributeType::Blob:
                    return "Blob";
                case SqLite::AttributeType::UuidV1:
                    return "UuidV1";
                case SqLite::AttributeType::UuidV4:
                    return "Uuid";
                default:
                    throw InvalidSchemaException("Unknown data type" + std::to_string(type));
            }
        }
    };

    struct Attribute {
        int id;
        std::string name;
        SqLite::AttributeType type;
        bool notnull;
        std::shared_ptr<std::string> check;
        std::shared_ptr<Utils::Validator> validator;

        static SqLite::AttributeType getType(std::string description) {
            if (description == "Integer")
                return SqLite::AttributeType::Integer;
            else if (description == "Real")
                return SqLite::AttributeType::Real;
            else if (description == "Text")
                return SqLite::AttributeType::Text;
            else if (description == "Blob")
                return SqLite::AttributeType::Blob;
            else if (description == "UuidV1")
                return SqLite::AttributeType::UuidV1;
            else if (description == "Uuid")
                return SqLite::AttributeType::UuidV4;
            else if (description == "Null")
                return SqLite::AttributeType::Null;
            else
                throw InvalidSchemaException("Unknown data type " + description);
        }

        static std::string getTypeDescription(SqLite::AttributeType type) {
            switch (type) {
                case SqLite::AttributeType::Integer:
                    return "Integer";
                case SqLite::AttributeType::Real:
                    return "Real";
                case SqLite::AttributeType::Text:
                    return "Text";
                case SqLite::AttributeType::Blob:
                    return "Blob";
                case SqLite::AttributeType::UuidV1:
                    return "UuidV1";
                case SqLite::AttributeType::UuidV4:
                    return "Uuid";
                case SqLite::AttributeType::Null:
                    return "Null";
                default:
                    throw InvalidSchemaException("Unknown data type" + std::to_string(type));
            }
        }
    };

    struct Transaction {
        std::string name;
        bool add;
        bool remove;
        std::unordered_set<int> update;
    };

    std::string uuid;
    std::string name;
    std::vector<Entity::Key> jsonKeys;
    std::vector<Entity::Attribute> jsonAttributes;
    std::map<int, Key> keys;
    std::map<int, Attribute> attributes;
    std::unordered_map<int, std::string> keysId2Name;
    std::unordered_map<int, std::string> attributesId2Name;
    std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> keysName2Id;
    std::unordered_map<std::string, int, Utils::IHasher, Utils::IEqualsComparator> attributesName2Id;
    std::unordered_map<std::string, Transaction, Utils::IHasher, Utils::IEqualsComparator> transactions;
};

} /* namespace Config */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {
using Beehive::Services::Config::Entity;

void from_json(const json &j, Entity::Attribute &x);
void to_json(json &j, const Entity::Attribute &x);

void from_json(const json &j, Entity::Key &x);
void to_json(json &j, const Entity::Key &x);

void from_json(const json &j, Entity &x);
void to_json(json &j, const Entity &x);

inline void from_json(const json &j, Entity::Attribute &x) {
    x.id = j.at("id").get<int>();
    x.name = j.at("name").get<std::string>();
    x.type = Entity::Attribute::getType(j.at("type").get<std::string>());
    x.notnull = Beehive::Services::Config::get_optional<bool>(j, "notnull", false);
    x.check = Beehive::Services::Config::get_optional<std::string>(j, "check");
}

inline void to_json(json &j, const Entity::Attribute &x) {
    j = json::object();
    j["id"] = x.id;
    j["name"] = x.name;
    j["type"] = Entity::Attribute::getTypeDescription(x.type);
    j["notnull"] = x.notnull;
    j["check"] = x.check;
}

inline void from_json(const json &j, Entity::Key &x) {
    x.id = j.at("id").get<int>();
    x.name = j.at("name").get<std::string>();
    x.type = Entity::Key::getType(j.at("type").get<std::string>());
}

inline void to_json(json &j, const Entity::Key &x) {
    j = json::object();
    j["id"] = x.id;
    j["name"] = x.name;
    j["type"] = Entity::Key::getTypeDescription(x.type);
}

inline void from_json(const json &j, Entity &x) {
    x.uuid = j.at("uuid").get<std::string>();
    x.name = j.at("name").get<std::string>();
    x.jsonKeys = j.at("keys").get<std::vector<Entity::Key>>();
    for (Entity::Key &key : x.jsonKeys) {
        x.keys.emplace(key.id, key);
        x.keysId2Name.emplace(key.id, key.name);
        x.keysName2Id.emplace(key.name, key.id);
    }
    x.jsonAttributes = j.at("attributes").get<std::vector<Entity::Attribute>>();
    for (Entity::Attribute &attribute : x.jsonAttributes) {
        x.attributes.emplace(attribute.id, attribute);
        x.attributesId2Name.emplace(attribute.id, attribute.name);
        x.attributesName2Id.emplace(attribute.name, attribute.id);
    }
}

inline void to_json(json &j, const Entity &x) {
    j = json::object();
    j["uuid"] = x.uuid;
    j["name"] = x.name;
    j["keys"] = x.jsonKeys;
    j["attributes"] = x.jsonAttributes;
}
} /* namespace nlohmann */
