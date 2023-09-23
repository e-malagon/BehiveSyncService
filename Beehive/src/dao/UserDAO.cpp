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


#include <dao/UserDAO.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string UserDAO::prefix("U.");
std::string UserDAO::ixprefix("U.IX.");

void UserDAO::saveDeveloper(Entities::Developer &admin) {
    std::string value = static_cast<nlohmann::json>(admin).dump();
    Storage::putValue(admin.identifier(), value, Storage::DefaultContext);
}

std::unique_ptr<Entities::Developer> UserDAO::readAdmin(const std::string &identifier) {
    std::unique_ptr<Entities::Developer> admin;
    std::string value;
    if (Storage::getValue(identifier, &value, Storage::DefaultContext))
        admin = std::make_unique<Entities::Developer>(static_cast<Entities::Developer>(nlohmann::json::parse(value)));
    return admin;
}

void UserDAO::save(Entities::User &user, const std::string &context, Storage::Transaction &transaction) {
    std::string value = static_cast<nlohmann::json>(user).dump();
    transaction.putValue(prefix + user.identifier(), value, context);
    transaction.putValue(ixprefix + user.uuid(), user.identifier(), context);
}

std::unique_ptr<Entities::User> UserDAO::read(const std::string &identifier, const std::string &context) {
    std::unique_ptr<Entities::User> user;
    std::string userBody;
    if (Storage::getValue(prefix + identifier, &userBody, context)) {
        user = std::make_unique<Entities::User>();
        nlohmann::json bodyJson = nlohmann::json::parse(userBody);
        nlohmann::from_json(bodyJson, *user);
    }
    return user;
}

std::unique_ptr<Entities::User> UserDAO::readByUUID(const std::string &uuid, const std::string &context) {
    std::unique_ptr<Entities::User> user;
    std::string identifier;
    if (Storage::getValue(ixprefix + uuid, &identifier, context)) {
        user = read(identifier, context);
    }
    return user;
}

std::vector<Entities::User> UserDAO::read(const std::string &context) {
    std::vector<Entities::User> users;
    std::vector<std::pair<std::string, std::string>> values;
    if (Storage::getValues(prefix, values, context)) {
        for (auto pair : values) {
            Entities::User user;
            nlohmann::json bodyJson = nlohmann::json::parse(pair.second);
            nlohmann::from_json(bodyJson, user);
            users.push_back(std::move(user));
        }
    }
    return users;
}

void UserDAO::update(Entities::User &user, const std::string &context, Storage::Transaction &transaction) {
    std::string value = static_cast<nlohmann::json>(user).dump();
    transaction.putValue(prefix + user.identifier(), value, context);
}

void UserDAO::remove(const std::string &uuid, const std::string &context, Storage::Transaction &transaction) {
    std::string identifier;
    if (Storage::getValue(ixprefix + uuid, &identifier, context)) {
        transaction.deleteValue(prefix + identifier, context);
        transaction.deleteValue(ixprefix + uuid, context);
    }
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
