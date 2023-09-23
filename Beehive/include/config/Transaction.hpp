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

namespace Beehive {
namespace Services {
namespace Config {

struct Transaction {
    struct Entity {
        std::string entity;
        bool add;
        bool remove;
        std::vector<int> update;
    };

    std::string uuid;
    std::string name;
    std::vector<Entity> entities;
    std::string pre;
    std::string post;
};

} /* namespace Config */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {
using Beehive::Services::Config::Transaction;

void from_json(const json &j, Transaction::Entity &x);
void to_json(json &j, const Transaction::Entity &x);

void from_json(const json &j, Transaction &x);
void to_json(json &j, const Transaction &x);

inline void from_json(const json &j, Transaction::Entity &x) {
    x.entity = j.at("entity").get<std::string>();
    x.add = j.at("add").get<bool>();
    x.remove = j.at("remove").get<bool>();
    x.update = j.at("update").get<std::vector<int>>();
}

inline void to_json(json &j, const Transaction::Entity &x) {
    j = json::object();
    j["entity"] = x.entity;
    j["add"] = x.add;
    j["remove"] = x.remove;
    j["update"] = x.update;
}

inline void from_json(const json &j, Transaction &x) {
    x.uuid = j.at("uuid").get<std::string>();
    x.name = j.at("name").get<std::string>();
    x.entities = j.at("entities").get<std::vector<Transaction::Entity>>();
    x.pre = j.at("pre").get<std::string>();
    x.post = j.at("post").get<std::string>();
}

inline void to_json(json &j, const Transaction &x) {
    j = json::object();
    j["uuid"] = x.uuid;
    j["name"] = x.name;
    j["entities"] = x.entities;
    j["pre"] = x.pre;
    j["post"] = x.post;
}
} /* namespace nlohmann */
