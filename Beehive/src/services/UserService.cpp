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


#include <crypto/base64.h>
#include <crypto/jwt.h>

#include <algorithm>
#include <cctype>
#include <crypto/Crypto.hpp>
#include <nanolog/NanoLog.hpp>
#include <random>
#include <services/ServiceException.hpp>
#include <services/UserService.hpp>
#include <sstream>
#include <string/ICaseMap.hpp>
#include <string>
#include <uuid/uuid.h>

namespace Beehive {
namespace Services {

std::vector<jwt::verifier<jwt::default_clock>> UserService::_googleVerifiers;
std::mutex UserService::_keyMutex;

std::unique_ptr<Entities::Developer> UserService::checkAdmin() {
    DAO::UserDAO userDAO;
    const std::string adminUserName = "admin@beehive";
    std::unique_ptr<Entities::Developer> admin = userDAO.readAdmin(adminUserName);
    if (!admin) {
        std::string salt = Services::Crypto::randomSalt();
        std::string passwd = Services::Crypto::passwordHash("Beehive01", salt);
        admin = std::make_unique<Entities::Developer>();
        admin->identifier(adminUserName);
        admin->name("Administrator");
        admin->password(passwd);
        admin->salt(salt);
        admin->rights(Entities::Developer::Rights::all);
        userDAO.saveDeveloper(*admin);
    }
    return admin;
}

std::unique_ptr<Entities::Developer> UserService::saveDeveloper(const std::string &email, const std::string &password, const std::string &name, Entities::Developer::Rights rights) {
    DAO::UserDAO userDAO;
    std::string salt = Services::Crypto::randomSalt();
    std::string passwd = Services::Crypto::passwordHash(password, salt);
    std::unique_ptr<Entities::Developer> admin = std::make_unique<Entities::Developer>();
    admin->identifier(email);
    admin->name(name);
    admin->password(passwd);
    admin->salt(salt);
    admin->rights(rights);
    userDAO.saveDeveloper(*admin);
    return admin;
}

std::unique_ptr<Entities::Developer> UserService::authenticateDeveloper(const std::string &authorization) {
    Utils::IEqualsComparator comparator;
    if (comparator(authorization.substr(0, 5), "Basic")) {
        DAO::UserDAO userDAO;
        std::string plain = base64_decode(authorization.substr(6));
        int pos = plain.find(':');
        if (pos <= 0)
            throw AuthenticationException("Invalid authorization header.");

        auto developer = userDAO.readAdmin(plain.substr(0, pos));
        if(!developer)
            throw AuthenticationException("Invalid authorization header.");
        std::string passwd = Services::Crypto::passwordHash(plain.substr(pos + 1), developer->salt());
        if (passwd != developer->password())
            throw AuthenticationException("Invalid authorization header.");
        return developer;
    } else 
        throw AuthenticationException("Invalid authorization header.");
}

void UserService::save(const std::string &identifier, const std::string &name, const std::string &password, const Entities::User::Type type, const std::string &context) {
    DAO::UserDAO userDAO;
    std::unique_ptr<Entities::User> user = userDAO.read(identifier, context);
    if (!user) {
        uuid_t uuid;
        char plainUuid[37];
        uuid_generate_time_safe(uuid);
        uuid_unparse_lower(uuid, plainUuid);
        user = std::make_unique<Entities::User>();
        user->uuid(std::string(plainUuid, 36));
        user->identifier(identifier);
        user->name(name);
        user->type(type);
        if (user->type() == Entities::User::Type::internal) {
            if (0 < password.length()) {
                std::string salt = Services::Crypto::randomSalt();
                std::string passwd = Services::Crypto::passwordHash(password, salt);
                user->password(passwd);
                user->salt(salt);
            }
        } else {
            user->password("");
            user->salt("");
        }
        DAO::Storage::Transaction transaction = DAO::Storage::begin();
        userDAO.save(*user, context, transaction);
        transaction.commit();
    } else {
        throw AlreadyExistsException("User already exists.");
    }
}

std::string UserService::getUser(const std::string &uuid, const std::string &context) {
    DAO::UserDAO userDAO;
    std::unique_ptr<Entities::User> user = userDAO.readByUUID(uuid, context);
    std::ostringstream formStream;
    nlohmann::json jUser;
    if (user) {
        jUser["uuid"] = user->uuid();
        jUser["identifier"] = user->identifier();
        jUser["name"] = user->name();
        jUser["type"] = Entities::User::getTypeDescription(user->type());
    } else {
        throw NotExistsException("User doesn't exist.");
    }
    return jUser.dump();
}

std::string UserService::getUsers(const std::string &context) {
    DAO::UserDAO userDAO;
    std::vector<Entities::User> users = userDAO.read(context);
    nlohmann::json jUsers = nlohmann::json::array();
    for(Entities::User user : users) {
        nlohmann::json jUser;
        jUser["uuid"] = user.uuid();
        jUser["identifier"] = user.identifier();
        jUser["name"] = user.name();
        jUser["type"] = Entities::User::getTypeDescription(user.type());
        jUsers.push_back(jUser);
    }
    return jUsers.dump();
}

void UserService::update(const std::string &identifier, const std::string &name, const std::string &password, const Entities::User::Type type, const std::string &context) {
    DAO::UserDAO userDAO;
    std::unique_ptr<Entities::User> user = userDAO.read(identifier, context);
    if (user) {
        user->name(name);
        user->type(type);
        if (user->type() == Entities::User::Type::internal) {
            if (0 < password.length()) {
                std::string salt = Services::Crypto::randomSalt();
                std::string passwd = Services::Crypto::passwordHash(password, salt);
                user->password(passwd);
                user->salt(salt);
            }
        } else {
            user->password("");
            user->salt("");
        }
        DAO::Storage::Transaction transaction = DAO::Storage::begin();
        userDAO.update(*user, context, transaction);
        transaction.commit();
    } else {
        throw NotExistsException("User doesn't exist.");
    }
}

void UserService::remove(const std::string &uuid, const std::string &context) {
    DAO::UserDAO userDAO;
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    userDAO.remove(uuid, context, transaction);
    transaction.commit();
}

std::unique_ptr<Entities::Node> UserService::authenticateUser(const std::string &authorization) {
    Utils::IEqualsComparator comparator;
    if (comparator(authorization.substr(0, 4), "User")) {
        return reconnect(authorization.substr(5));
    } else 
        throw AuthenticationException("Invalid authorization header.");
}

std::unique_ptr<Entities::Node> UserService::signIn(const std::string &jwt, const std::string &module, const std::string &nodeUUID, Entities::User::Type type, const std::string &context) {
    std::string email;
    std::string name;
    switch (type) {
        case 1: {
            auto decoded = jwt::decode(jwt);
            std::string issuer = decoded.get_payload_claim("iss").as_string();
            if (issuer == "accounts.google.com") {
                if (!verifyGoogle(decoded))
                    throw AuthenticationException("No valid signature found");
                email = decoded.get_payload_claim("email").as_string();
                name = decoded.get_payload_claim("name").as_string();
            } else {
                throw AuthenticationException("Unknown issuer");
            }
        } break;
        default:
            throw AuthenticationException("Unknown authentication method");
            break;
    }
    std::transform(email.begin(), email.end(), email.begin(), ::tolower);
    DAO::UserDAO userDAO;
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    std::unique_ptr<Entities::User> user = userDAO.read(email, context);
    if (!user) {
        user = std::make_unique<Entities::User>();
        user->identifier(email);
        user->name(name);
        user->type(static_cast<Entities::User::Type>(type));
        user->password("");
        user->salt("");
        userDAO.save(*user, context, transaction);
    }
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0, 256);
    char rawKey[16 + sizeof(uuid_t) + sizeof(uuid_t)];
    for (int i = 0; i < 16; ++i)
        rawKey[i] = dist(mt);
    DAO::NodeDAO nodeDAO;
    std::unique_ptr<Entities::Node> node = nodeDAO.read(nodeUUID, user->uuid(), transaction);
    if (!node) {
        node = std::make_unique<Entities::Node>();
        node->user(std::move(*user));
        node->key(std::string(rawKey, 16));
        node->context(context);
        node->module(module);
        node->uuid(nodeUUID);
        nodeDAO.save(*node, transaction);
    } else {
        node->key(std::string(rawKey, 16));
        nodeDAO.save(*node, transaction);
    }
    transaction.commit();
    uuid_t plainUuid;
    uuid_parse(node->uuid().c_str(), plainUuid);
    memcpy(rawKey + 16, plainUuid, sizeof(uuid_t));
    uuid_parse(node->user().uuid().c_str(), plainUuid);
    memcpy(rawKey + 16 + sizeof(uuid_t), plainUuid, sizeof(uuid_t));
    node->nodeKey(base64_encode((unsigned char const *)rawKey, 16 + sizeof(uuid_t) + sizeof(uuid_t)));
    return node;
}

std::unique_ptr<Entities::Node> UserService::signIn(const std::string &email, const std::string &password, const std::string &module, const std::string &nodeUUID, const std::string &context) {
    DAO::UserDAO userDAO;
    std::unique_ptr<Entities::User> user = userDAO.read(email, context);
    if (!user) {
        throw AuthenticationException("User doesn't exist.");
    } else if (user->type() != Entities::User::Type::internal) {
        throw AuthenticationException("Authentication method not allowed.");
    } else {
        std::string passwd = Services::Crypto::passwordHash(password, user->salt());
        if (user->password() != passwd) {
            throw AuthenticationException("Bad password.");
        }
    }
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0, 256);
    char rawKey[16 + sizeof(uuid_t) + sizeof(uuid_t)];
    for (int i = 0; i < 16; ++i)
        rawKey[i] = dist(mt);
    DAO::NodeDAO nodeDAO;
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    std::unique_ptr<Entities::Node> node = nodeDAO.read(nodeUUID, user->uuid(), transaction);
    if (!node) {
        node = std::make_unique<Entities::Node>();
        node->user(std::move(*user));
        node->key(std::string(rawKey, 16));
        node->context(context);
        node->module(module);
        node->uuid(nodeUUID);
        nodeDAO.save(*node, transaction);
    } else {
        node->key(std::string(rawKey, 16));
        nodeDAO.save(*node, transaction);
    }
    transaction.commit();
    uuid_t plainUuid;
    uuid_parse(node->uuid().c_str(), plainUuid);
    memcpy(rawKey + 16, plainUuid, sizeof(uuid_t));
    uuid_parse(node->user().uuid().c_str(), plainUuid);
    memcpy(rawKey + 16 + sizeof(uuid_t), plainUuid, sizeof(uuid_t));
    node->nodeKey(base64_encode((unsigned char const *)rawKey, 16 + sizeof(uuid_t) + sizeof(uuid_t)));
    return node;
}

std::unique_ptr<Entities::Node> UserService::signUp(const std::string &name, const std::string &email, const std::string &password, const std::string &module, const std::string &nodeUUID, const std::string &context) {
    DAO::UserDAO userDAO;
    std::unique_ptr<Entities::User> user = userDAO.read(email, context);
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    if (!user) {
        std::string salt = Services::Crypto::randomSalt();
        std::string passwd = Services::Crypto::passwordHash(password, salt);
        user = std::make_unique<Entities::User>();
        user->identifier(email);
        user->name(name);
        user->type(Entities::User::Type::internal);
        user->password(passwd);
        user->salt(salt);        
        userDAO.save(*user, context, transaction);
    } else {
        if (user->password() == "") {
            std::string salt = Services::Crypto::randomSalt();
            std::string passwd = Services::Crypto::passwordHash(password, salt);
            user->type(Entities::User::Type::internal);
            user->password(passwd);
            user->salt(salt);
            userDAO.update(*user, "", transaction);
        } else {
            std::string passwd = Services::Crypto::passwordHash(password, user->salt());
            if (user->password() != passwd) {
                throw AuthenticationException("Bad password");
            }
        }
    }
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0, 256);
    char rawKey[16 + sizeof(uuid_t) + sizeof(uuid_t)];
    for (int i = 0; i < 16; ++i)
        rawKey[i] = dist(mt);
    DAO::NodeDAO nodeDAO;
    std::unique_ptr<Entities::Node> node = nodeDAO.read(nodeUUID, user->uuid(), transaction);
    if (!node) {
        node = std::make_unique<Entities::Node>();
        node->user(std::move(*user));
        node->key(std::string(rawKey, 16));
        node->context(context);
        node->module(module);
        node->uuid(nodeUUID);
        nodeDAO.save(*node, transaction);
    } else {
        node->key(std::string(rawKey, 16));
        nodeDAO.save(*node, transaction);
    }
    transaction.commit();
    uuid_t plainUuid;
    uuid_parse(node->uuid().c_str(), plainUuid);
    memcpy(rawKey + 16, plainUuid, sizeof(uuid_t));
    uuid_parse(node->user().uuid().c_str(), plainUuid);
    memcpy(rawKey + 16 + sizeof(uuid_t), plainUuid, sizeof(uuid_t));
    node->nodeKey(base64_encode((unsigned char const *)rawKey, 16 + sizeof(uuid_t) + sizeof(uuid_t), true));
    return node;
}

void UserService::signOut(const Entities::Node &node) {
    DAO::NodeDAO nodeDAO;
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    nodeDAO.remove(node.uuid(), node.user().uuid(), transaction);
    transaction.commit();
}

void UserService::signOff(const std::string &jwt, Entities::User::Type type, const std::string &context) {
    std::string email;
    std::string name;
    switch (type) {
        case Entities::User::Type::internal: {
            email = jwt;
            name = jwt;
        } break;
        case Entities::User::Type::google: {
            auto decoded = jwt::decode(jwt);
            std::string issuer = decoded.get_payload_claim("iss").as_string();
            if (issuer == "accounts.google.com") {
                if (!verifyGoogle(decoded))
                    throw AuthenticationException("No valid signature found");
                email = decoded.get_payload_claim("email").as_string();
                name = decoded.get_payload_claim("name").as_string();
            } else {
                throw AuthenticationException("Unknown issuer");
            }
        } break;
        default:
            throw AuthenticationException("Unknown authentication method");
            break;
    }
    std::transform(email.begin(), email.end(), email.begin(), ::tolower);
    DAO::UserDAO userDAO;
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    userDAO.remove(email, context, transaction);
    transaction.commit();
}

void UserService::signOff(const std::string &email, const std::string &password, const std::string &context) {
    DAO::UserDAO userDAO;
    std::unique_ptr<Entities::User> user = userDAO.read(email, context);
    if (!user) {
        throw AuthenticationException("User doesn't exist");
    } else if (user->type() != Entities::User::Type::internal) {
        throw AuthenticationException("Authentication method not allowed");
    } else {
        std::string passwd = Services::Crypto::passwordHash(password, user->salt());
        if (user->password() != passwd) {
            throw AuthenticationException("Bad password");
        }
    }
    DAO::Storage::Transaction transaction = DAO::Storage::begin();
    userDAO.remove(email, context, transaction);
    transaction.commit();
}

std::unique_ptr<Entities::Node> UserService::reconnect(const std::string &auth) {
    char rawKey[16 + sizeof(uuid_t) + sizeof(uuid_t)];
    std::string decoded = base64_decode(auth);
    decoded.copy(rawKey, 16 + sizeof(uuid_t) + sizeof(uuid_t), 0);
    uuid_t plainUuid;
    char uuidChar[37];
    memcpy((void *)plainUuid, rawKey + 16, sizeof(uuid_t));
    uuid_unparse_lower(plainUuid, uuidChar);
    std::string uuidNode(uuidChar, 36);
    memcpy((void *)plainUuid, rawKey + 16 + sizeof(uuid_t), sizeof(uuid_t));
    uuid_unparse_lower(plainUuid, uuidChar);
    std::string uuidUser(uuidChar, 36);
    DAO::NodeDAO nodeDAO;
    std::unique_ptr<Entities::Node> node = nodeDAO.read(uuidNode, uuidUser);
    if (node && node->key() == std::string(rawKey, 16)) {
        DAO::UserDAO userDAO;
        std::unique_ptr<Entities::User> user = userDAO.read(node->user().uuid(), node->context());
        if (user) {
            node->user(*user);
            return node;
        } else {
            throw AuthenticationException("Not valid credentials.");
        }
    } else {
        throw AuthenticationException("Not valid credentials.");
    }
}

bool UserService::verifyGoogle(jwt::decoded_jwt decoded) {
    std::lock_guard<std::mutex> lock(_keyMutex);
    bool valid = false;
    for (auto verifier : _googleVerifiers) {
        try {
            verifier.verify(decoded);
            valid = true;
            break;
        } catch (std::runtime_error &e) {
        }
    }
    return valid;
}

void UserService::setGoogleRSARS256PubKeys(std::vector<std::string> keys) {
    std::lock_guard<std::mutex> lock(_keyMutex);
    _googleVerifiers.clear();
    for (std::string key : keys) {
        _googleVerifiers.push_back(jwt::verify().allow_algorithm(jwt::algorithm::rs256(key, "", "", "")).with_issuer("accounts.google.com"));
    }
}

} /* namespace Services */
} /* namespace Beehive */
