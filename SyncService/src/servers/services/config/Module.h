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

#include <json/json.hpp>
#include "Common.h"

#include <unordered_map>
#include <unordered_set>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Config {

struct Module {

  struct Entity {
    std::string entity;
    std::string entity_name;
    std::vector<std::string> attributes;
  };

  std::string uuid;
  std::string layout;
  std::string name;
  std::vector<Entity> entities;
  std::unordered_map<std::string, std::unordered_set<int>> entitiesMap;
};

} /* namespace Config */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

namespace nlohmann {
using SyncServer::Servers::Services::Config::Module;

void from_json(const json &j, Module::Entity &x);
void to_json(json &j, const Module::Entity &x);

void from_json(const json &j, Module &x);
void to_json(json &j, const Module &x);

inline void from_json(const json &j, Module::Entity &x) {
  x.entity = j.at("entity").get<std::string>();
  x.entity_name = j.at("entity_name").get<std::string>();
  x.attributes = j.at("attributes").get<std::vector<std::string>>();
}

inline void to_json(json &j, const Module::Entity &x) {
  j = json::object();
  j["entity"] = x.entity;
  j["entity_name"] = x.entity_name;
  j["attributes"] = x.attributes;
}

inline void from_json(const json &j, Module &x) {
  x.uuid = j.at("uuid").get<std::string>();
  x.layout = j.at("layout").get<std::string>();
  x.name = j.at("name").get<std::string>();
  x.entities = j.at("entities").get<std::vector<Module::Entity>>();
  for (Module::Entity &entity : x.entities) {
    std::unordered_set<int> attributes;
    for (std::string &attribute : entity.attributes)
      attributes.emplace(std::stoi(attribute));
    x.entitiesMap.emplace(entity.entity, attributes);
  }
}

inline void to_json(json &j, const Module &x) {
  j = json::object();
  j["uuid"] = x.uuid;
  j["layout"] = x.layout;
  j["name"] = x.name;
  j["entities"] = x.entities;
}
} /* namespace nlohmann */
