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
#include <string>

namespace Beehive {
namespace Services {
namespace Entities {

class Developer {
   public:
    enum Rights {
        all = 0,
        admin = 1
    };

    const std::string& identifier() const {
        return _identifier;
    }

    void identifier(std::string identifier) {
        _identifier = identifier;
    }

    const std::string& name() const {
        return _name;
    }

    void name(std::string name) {
        _name = name;
    }

    const std::string& password() const {
        return _password;
    }

    void password(std::string password) {
        _password = password;
    }

    const std::string& salt() const {
        return _salt;
    }

    void salt(std::string salt) {
        _salt = salt;
    }

    Rights rights() const {
        return _rights;
    }

    void rights(Rights rights) {
        _rights = rights;
    }

   private:
    std::string _identifier;
    std::string _name;
    std::string _password;
    std::string _salt;
    Rights _rights;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {
void from_json(const json& j, Beehive::Services::Entities::Developer& x);
void to_json(json& j, const Beehive::Services::Entities::Developer& x);

inline void from_json(const json& j, Beehive::Services::Entities::Developer& x) {
    x.identifier(j.at("identifier").get<std::string>());
    x.name(j.at("name").get<std::string>());
    x.password(j.at("password").get<std::string>());
    x.salt(j.at("salt").get<std::string>());
    x.rights(static_cast<Beehive::Services::Entities::Developer::Rights>(j.at("rights").get<int64_t>()));
}

inline void to_json(json& j, const Beehive::Services::Entities::Developer& x) {
    j = json::object();
    j["identifier"] = x.identifier();
    j["name"] = x.name();
    j["password"] = x.password();
    j["salt"] = x.salt();
    j["rights"] = to_underlying(x.rights());
}
}  // namespace nlohmann