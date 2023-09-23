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

#include <crypto/base64.h>
#include <entities/User.hpp>

#include <string>

namespace Beehive {
namespace Services {
namespace Entities {

class Node {
   public:
    const User& user() const {
        return _user;
    }

    void user(const User &user) {
        _user = user;
    }

    const std::string& key() const {
        return _key;
    }

    void key(const std::string &key) {
        _key = key;
    }

    const std::string& nodeKey() const {
        return _nodeKey;
    }

    void nodeKey(const std::string &nodeKey) {
        _nodeKey = nodeKey;
    }

    const std::string& context() const {
        return _context;
    }

    void context(const std::string &context) {
        _context = context;
    }

    const std::string& module() const {
        return _module;
    }

    void module(std::string module) {
        _module = module;
    }

    const std::string& uuid() const {
        return _uuid;
    }

    void uuid(const std::string &uuid) {
        _uuid = uuid;
    }

    uint32_t version() const {
        return _version;
    }

    void version(uint32_t version) {
        _version = version;
    }

   private:
    User _user;
    std::string _key;
    std::string _nodeKey;
    std::string _context;
    std::string _module;
    std::string _uuid;
    uint32_t _version;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {
void from_json(const json& j, Beehive::Services::Entities::Node& x);
void to_json(json& j, const Beehive::Services::Entities::Node& x);

inline void from_json(const json& j, Beehive::Services::Entities::Node& x) {
    x.user(j.at("user").get<Beehive::Services::Entities::User>());
    std::string encoded = base64_decode(j.at("key").get<std::string>());
    x.key(encoded);
    x.context(j.at("context").get<std::string>());
    x.module(j.at("module").get<std::string>());
    x.uuid(j.at("uuid").get<std::string>());
    x.version(j.at("version").get<uint32_t>());
}

inline void to_json(json& j, const Beehive::Services::Entities::Node& x) {
    j = json::object();
    j["user"] = x.user();
    j["key"] = base64_encode(x.key());
    j["context"] = x.context();
    j["module"] = x.module();
    j["uuid"] = x.uuid();
    j["version"] = x.version();
}
}  // namespace nlohmann