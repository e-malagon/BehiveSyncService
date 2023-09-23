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
#include "Entity.h"
#include "Transaction.h"
#include "Role.h"
#include "Module.h"

#include <unordered_map>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Config {
using nlohmann::json;

struct Application {
  std::string uuid;
  std::string name;
  std::string defaultrole;
  int64_t version;
  int64_t edited;

  std::unordered_map<int, std::unordered_map<std::string, Entity, Utils::IHasher, Utils::IEqualsComparator>> entities;
  std::unordered_map<int, std::unordered_map<std::string, Transaction, Utils::IHasher, Utils::IEqualsComparator>> transactions;
  std::unordered_map<int, std::unordered_map<std::string, Role, Utils::IHasher, Utils::IEqualsComparator>> roles;
  std::unordered_map<int, std::unordered_map<std::string, Module, Utils::IHasher, Utils::IEqualsComparator>> modules;

  std::unordered_map<int, std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator>> entitiesName2UUID;
  std::unordered_map<int, std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator>> transactionsName2UUID;
  std::unordered_map<int, std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator>> rolesName2UUID;
  std::unordered_map<int, std::unordered_map<std::string, std::string, Utils::IHasher, Utils::IEqualsComparator>> modulesName2UUID;

};
} /* namespace Config */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

namespace nlohmann {
using SyncServer::Servers::Services::Config::Application;

void from_json(const json &j, Application &x);
void to_json(json &j, const Application &x);

inline void from_json(const json &j, Application &x) {
  x.uuid = j.at("uuid").get<std::string>();
  x.name = j.at("name").get<std::string>();
  x.defaultrole = j.at("defaultrole").get<std::string>();
  x.version = j.at("version").get<int64_t>();
  x.edited = j.at("edited").get<int64_t>();
}

inline void to_json(json &j, const Application &x) {
  j = json::object();
  j["uuid"] = x.uuid;
  j["name"] = x.name;
  j["defaultrole"] = x.defaultrole;
  j["version"] = x.version;
  j["edited"] = x.edited;
}
} /* namespace nlohmann */
