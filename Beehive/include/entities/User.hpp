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

class User {
   public:
    enum Type {
        internal = 0,
        google = 1,
        unknown = 10000
    };

    const std::string& uuid() const {
        return _uuid;
    }

    void uuid(const std::string &uuid) {
        _uuid = uuid;
    }

    const std::string& identifier() const {
        return _identifier;
    }

    void identifier(const std::string &identifier) {
        _identifier = identifier;
    }

    const std::string& name() const {
        return _name;
    }

    void name(const std::string &name) {
        _name = name;
    }

    Type type() const {
        return _type;
    }

    void type(Type type) {
        _type = type;
    }

    const std::string& password() const {
        return _password;
    }

    void password(const std::string &password) {
        _password = password;
    }

    const std::string& salt() const {
        return _salt;
    }

    void salt(const std::string &salt) {
        _salt = salt;
    }

    static Type getType(const std::string &type) {
        if(type == "internal")
            return Type::internal;
        else if (type == "google")
            return Type::google;
        else
            return Type::unknown;
    }

    static const std::string getTypeDescription(Type type) {
        switch (type)
        {
        case Type::internal:
            return "internal";
        case Type::google:
            return "google";
        case Type::unknown:
        default:
            return "unknown";
        }
    }

   private:
    std::string _uuid;
    std::string _identifier;
    std::string _name;
    Type _type;
    std::string _password;
    std::string _salt;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Beehive */

namespace nlohmann {
void from_json(const json& j, Beehive::Services::Entities::User& x);
void to_json(json& j, const Beehive::Services::Entities::User& x);

inline void from_json(const json& j, Beehive::Services::Entities::User& x) {
    x.uuid(j.at("uuid").get<std::string>());
    x.identifier(j.at("identifier").get<std::string>());
    x.name(j.at("name").get<std::string>());
    x.type(static_cast<Beehive::Services::Entities::User::Type>(j.at("type").get<int64_t>()));
    x.password(j.at("password").get<std::string>());
    x.salt(j.at("salt").get<std::string>());
}

inline void to_json(json& j, const Beehive::Services::Entities::User& x) {
    j = json::object();
    j["uuid"] = x.uuid();
    j["identifier"] = x.identifier();
    j["name"] = x.name();
    j["type"] = to_underlying(x.type());
    j["password"] = x.password();
    j["salt"] = x.salt();
}
}  // namespace nlohmann