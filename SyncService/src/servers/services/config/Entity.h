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
#include "Transaction.h"

#include <validation/Validator.h>
#include <string/ICaseMap.h>
#include <sqlite/Types.h>
#include <json/json.hpp>

#include <unordered_set>

namespace SyncServer {
namespace Servers {
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
        return SqLite::AttributeType::Integer;
    }

    static std::string getTypeDescription(SqLite::AttributeType type) {
      switch (type) {
      case SqLite::AttributeType::Integer:
      default:
        return "Integer";
      case SqLite::AttributeType::Text:
        return "Text";
      case SqLite::AttributeType::Blob:
        return "Blob";
      case SqLite::AttributeType::UuidV1:
        return "UuidV1";
      case SqLite::AttributeType::UuidV4:
        return "Uuid";
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
      else
        return SqLite::AttributeType::Null;
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
      default:
        return "Null";
      }
    }
  };

  struct Transaction {
    std::string name;
    bool add;
    bool remove;
    std::unordered_set<int> update;
  };

  std::string id;
  std::string uuid;
  std::string layout;
  std::string name;
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
} /* namespace Servers */
} /* namespace SyncServer */

namespace nlohmann {
using SyncServer::Servers::Services::Config::Entity;

void from_json(const json &j, Entity::Attribute &x);
void to_json(json &j, const Entity::Attribute &x);

void from_json(const json &j, Entity::Key &x);
void to_json(json &j, const Entity::Key &x);

void from_json(const json &j, Entity &x);
void to_json(json &j, const Entity &x);

inline void from_json(const json &j, Entity::Attribute &x) {
  x.id = std::stoi(j.at("id").get<std::string>());
  x.name = j.at("name").get<std::string>();
  x.type = Entity::Attribute::getType(j.at("type").get<std::string>());
  x.notnull = j.at("notnull").get<bool>();
  x.check = SyncServer::Servers::Services::Config::get_optional<std::string>(j, "check"); // @suppress("Symbol is not resolved") // @suppress("Invalid arguments")
}

inline void to_json(json &j, const Entity::Attribute &x) {
  j = json::object();
  j["id"] = std::to_string(x.id);
  j["name"] = x.name;
  j["type"] = Entity::Attribute::getTypeDescription(x.type);
  j["notnull"] = x.notnull;
  j["check"] = x.check;
}

inline void from_json(const json &j, Entity::Key &x) {
  x.id = std::stoi(j.at("id").get<std::string>());
  x.name = j.at("name").get<std::string>();
  x.type = Entity::Key::getType(j.at("type").get<std::string>());
}

inline void to_json(json &j, const Entity::Key &x) {
  j = json::object();
  j["id"] = std::to_string(x.id);
  j["name"] = x.name;
  j["type"] = Entity::Key::getTypeDescription(x.type);
}

inline void from_json(const json &j, Entity &x) {
  x.uuid = j.at("uuid").get<std::string>();
  x.layout = j.at("layout").get<std::string>();
  x.name = j.at("name").get<std::string>();
  std::vector<Entity::Key> keys = j.at("keys").get<std::vector<Entity::Key>>();
  std::vector<Entity::Attribute> attributes = j.at("attributes").get<std::vector<Entity::Attribute>>();
  int keyCount = 1;
  int attribuesCount = 1;
  for (Entity::Key &key : keys)
    keyCount = keyCount <= key.id ? key.id + 1 : keyCount;
  for (Entity::Attribute &attribute : attributes)
    attribuesCount = attribuesCount <= attribute.id ? attribute.id + 1 : attribuesCount;
  for (Entity::Key &key : keys) {
    if (key.id == 0)
      key.id = keyCount++;
    x.keys.emplace(key.id, key);
    x.keysId2Name.emplace(key.id, key.name);
    x.keysName2Id.emplace(key.name, key.id);
  }
  for (Entity::Attribute &attribute : attributes) {
    if (attribute.id == 0)
      attribute.id = attribuesCount++;
    x.attributes.emplace(attribute.id, attribute);
    x.attributesId2Name.emplace(attribute.id, attribute.name);
    x.attributesName2Id.emplace(attribute.name, attribute.id);
  }

  uint8_t uuidbin[33] = "0";
  const char *uuidChar = x.uuid.c_str();
  uint8_t indexes[] = { 0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };
  for (int i = 0, k = 1; i < 16; ++i, k += 2) {
    uuidbin[k] = uuidChar[indexes[i]];
    uuidbin[k + 1] = uuidChar[indexes[i] + 1];
  }
  x.id = std::string((char*) uuidbin, 33);
}

inline void to_json(json &j, const Entity &x) {
  j = json::object();
  j["uuid"] = x.uuid;
  j["layout"] = x.layout;
  j["name"] = x.name;
  std::vector<Entity::Key> keys;
  for (auto key : x.keys)
    keys.push_back(key.second);
  j["keys"] = keys;
  std::vector<Entity::Attribute> attributes;
  for (auto attribute : x.attributes)
    attributes.push_back(attribute.second);
  j["attributes"] = attributes;
}
} /* namespace nlohmann */
