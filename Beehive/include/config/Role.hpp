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

#include <json/json.hpp>
#include <unordered_map>
#include <unordered_set>

namespace Beehive {
namespace Services {
namespace Config {

struct Role {
    struct Entity {
        std::string entity;
        std::vector<int> attributes;
    };

    std::string uuid;
    std::string name;
    bool readmembers;
    bool managemembers;
    bool reademail;
    bool sharedataset;
    bool manageshare;
    std::vector<Entity> entities;
    std::vector<std::string> transactions;
    std::unordered_map<std::string, std::unordered_set<int>> entitiesMap;
};

} /* namespace Config */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {
using Beehive::Services::Config::Role;

void from_json(const json &j, Role::Entity &x);
void to_json(json &j, const Role::Entity &x);

void from_json(const json &j, Role &x);
void to_json(json &j, const Role &x);

inline void from_json(const json &j, Role::Entity &x) {
    x.entity = j.at("entity").get<std::string>();
    x.attributes = j.at("attributes").get<std::vector<int>>();
}

inline void to_json(json &j, const Role::Entity &x) {
    j = json::object();
    j["entity"] = x.entity;
    j["attributes"] = x.attributes;
}

inline void from_json(const json &j, Role &x) {
    x.uuid = j.at("uuid").get<std::string>();
    x.name = j.at("name").get<std::string>();
    x.readmembers = Beehive::Services::Config::get_optional<bool>(j, "readmembers", false);
    x.managemembers = Beehive::Services::Config::get_optional<bool>(j, "managemembers", false);
    x.reademail = Beehive::Services::Config::get_optional<bool>(j, "reademail", false);
    x.sharedataset = Beehive::Services::Config::get_optional<bool>(j, "sharedataset", false);
    x.manageshare = Beehive::Services::Config::get_optional<bool>(j, "manageshare", false);
    x.entities = j.at("entities").get<std::vector<Role::Entity>>();
    x.transactions = j.at("transactions").get<std::vector<std::string>>();
    for (Role::Entity &entity : x.entities) {
        std::unordered_set<int> attributes;
        for (int attribute : entity.attributes)
            attributes.emplace(attribute);
        x.entitiesMap.emplace(entity.entity, attributes);
    }
}

inline void to_json(json &j, const Role &x) {
    j = json::object();
    j["uuid"] = x.uuid;
    j["name"] = x.name;
    j["readmembers"] = x.readmembers;
    j["managemembers"] = x.managemembers;
    j["reademail"] = x.reademail;
    j["sharedataset"] = x.sharedataset;
    j["manageshare"] = x.manageshare;
    j["entities"] = x.entities;
    j["transactions"] = x.transactions;
}
} /* namespace nlohmann */
