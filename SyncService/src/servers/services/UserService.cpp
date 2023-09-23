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

#include "UserService.h"

#include <ServiceException.h>

#include <random>
#include <string>
#include <cctype>
#include <sstream>
#include <algorithm>
#include <nanolog/NanoLog.hpp>
#include <crypto/jwt.h>
#include <crypto/base64.h>
#include <crypto/Crypto.h>
#include <config/Config.h>

namespace SyncServer {
namespace Servers {
namespace Services {

std::vector<jwt::verifier<jwt::default_clock>> UserService::_googleVerifiers;
std::mutex UserService::_keyMutex;

std::unique_ptr<Entities::Node> UserService::signIn(std::string jwt, const std::string &application, const std::string &module, const std::string nodeUUID, int type) {
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
  }
    break;
  default:
    throw AuthenticationException("Unknown authentication method");
    break;
  }
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  std::unique_ptr<Entities::User> user = _userDAO.read(email);
  if (!user) {
    user = std::make_unique<Entities::User>();
    user->identifier(email);
    user->name(name);
    user->kind(type);
    user->identity("");
    user->password("");
    user->salt("");
    _userDAO.save(*user);
  }
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 256);
  char rawKey[20];
  for (int i = 0; i < 16; ++i)
    rawKey[i] = dist(mt);
  std::unique_ptr<Entities::Node> node = _nodeDAO.read(nodeUUID, user->id());
  if (!node) {
    node = std::make_unique<Entities::Node>();
    node->user(std::move(*user));
    node->key(std::string(rawKey, 16));
    node->application(application);
    node->module(module);
    node->uuid(nodeUUID);
    _nodeDAO.save(*node);
  } else {
    node->key(std::string(rawKey, 16));
    _nodeDAO.updateKey(*node);
  }
  _connection->commit();
  uint32_t id = node->id();
  memcpy(rawKey + 16, &id, sizeof(uint32_t));
  node->nodeKey(base64_encode((unsigned char const*) rawKey, 20));
  return node;
}

std::unique_ptr<Entities::Node> UserService::signIn(std::string email, std::string password, const std::string &application, const std::string &module, const std::string nodeUUID) {
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  std::unique_ptr<Entities::User> user = _userDAO.read(email);
  if (!user) {
    throw AuthenticationException("User doesn't exist");
  } else if (user->kind() != 0) {
    throw AuthenticationException("Authentication method not allowed");
  } else {
    std::string passwd = Services::Crypto::getPasswordHash(password, user->salt());
    if (user->password() != passwd) {
      throw AuthenticationException("Bad password");
    }
  }
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 256);
  char rawKey[20];
  for (int i = 0; i < 16; ++i)
    rawKey[i] = dist(mt);
  std::unique_ptr<Entities::Node> node = _nodeDAO.read(nodeUUID, user->id());
  if (!node) {
    node = std::make_unique<Entities::Node>();
    node->user(std::move(*user));
    node->key(std::string(rawKey, 16));
    node->application(application);
    node->module(module);
    node->uuid(nodeUUID);
    _nodeDAO.save(*node);
  } else {
    node->key(std::string(rawKey, 16));
    _nodeDAO.updateKey(*node);
  }
  _connection->commit();
  uint32_t id = node->id();
  memcpy(rawKey + 16, &id, sizeof(uint32_t));
  node->nodeKey(base64_encode((unsigned char const*) rawKey, 20));
  return node;
}

std::unique_ptr<Entities::Node> UserService::signUp(std::string name, std::string email, std::string password, const std::string &application, const std::string &module, const std::string nodeUUID) {
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  std::unique_ptr<Entities::User> user = _userDAO.read(email);
  if (!user) {
    std::string salt = Services::Crypto::getSalt();
    std::string passwd = Services::Crypto::getPasswordHash(password, salt);
    user = std::make_unique<Entities::User>();
    user->identifier(email);
    user->name(name);
    user->kind(0);
    user->identity("");
    user->password(passwd);
    user->salt(salt);
    _userDAO.save(*user);
  } else {
    if (user->password() == "") {
      std::string salt = Services::Crypto::getSalt();
      std::string passwd = Services::Crypto::getPasswordHash(password, salt);
      user->kind(0);
      user->password(passwd);
      user->salt(salt);
      _userDAO.update(*user);
    } else {
      std::string passwd = Services::Crypto::getPasswordHash(password, user->salt());
      if (user->password() != passwd) {
        throw AuthenticationException("Bad password");
      }
    }
  }
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 256);
  char rawKey[20];
  for (int i = 0; i < 16; ++i)
    rawKey[i] = dist(mt);
  std::unique_ptr<Entities::Node> node = _nodeDAO.read(nodeUUID, user->id());
  if (!node) {
    node = std::make_unique<Entities::Node>();
    node->user(std::move(*user));
    node->key(std::string(rawKey, 16));
    node->application(application);
    node->module(module);
    node->uuid(nodeUUID);
    _nodeDAO.save(*node);
  } else {
    node->key(std::string(rawKey, 16));
    _nodeDAO.updateKey(*node);
  }
  _connection->commit();
  uint32_t id = node->id();
  memcpy(rawKey + 16, &id, sizeof(uint32_t));
  node->nodeKey(base64_encode((unsigned char const*) rawKey, 20));
  return node;
}

void UserService::signOut(Entities::Node &node) {
  _nodeDAO.remove(node.id());
  _connection->commit();
}

void UserService::signOff(std::string jwt, int type) {
  std::string email;
  std::string name;
  switch (type) {
  case 0: {
    email = jwt;
    name = jwt;
  }
    break;
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
  }
    break;
  default:
    throw AuthenticationException("Unknown authentication method");
    break;
  }
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  _userDAO.remove(email);
  _connection->commit();
}

void UserService::signOff(std::string email, std::string password) {
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  std::unique_ptr<Entities::User> user = _userDAO.read(email);
  if (!user) {
    throw AuthenticationException("User doesn't exist");
  } else if (user->kind() != 0) {
    throw AuthenticationException("Authentication method not allowed");
  } else {
    std::string passwd = Services::Crypto::getPasswordHash(password, user->salt());
    if (user->password() != passwd) {
      throw AuthenticationException("Bad password");
    }
  }
  _userDAO.remove(email);
  _connection->commit();
}

std::unique_ptr<Entities::Node> UserService::reconnect(std::string auth) {
  _connection->commit();
  char rawKey[20];
  uint32_t idNode;
  std::string decoded = base64_decode(auth);
  decoded.copy(rawKey, 20, 0);
  memcpy((void*) &idNode, rawKey + 16, sizeof(uint32_t));
  std::unique_ptr<Entities::Node> node = _nodeDAO.read(idNode);
  if (node && node->key() == std::string(rawKey, 16)) {
    std::unique_ptr<Entities::User> user = _userDAO.read(node->user().id());
    if (user) {
      node->user(std::move(*_userDAO.read(node->user().id())));
      return node;
    } else {
      throw AuthenticationException("Not valid credentials");
    }
  } else {
    throw AuthenticationException("Not valid credentials");
  }
}

std::vector<Entities::User> UserService::getUsers() {
  _connection->commit();
  return _userDAO.read();
}

std::string UserService::getUser(uint32_t identifier) {
  _connection->commit();
  std::unique_ptr<Entities::User> user = _userDAO.read(identifier);
  Config::Form form = Config::getForm("users");
  std::ostringstream formStream;
  if (user) {
    formStream << "{";
    formStream << "\"iduser\":" << user->id() << ",";
    formStream << "\"name\":\"" << user->name() << "\",";
    formStream << "\"email\":\"" << user->identifier() << "\",";
    formStream << "\"password\":\"\",";
    formStream << "\"type\":\"" << user->kind() << "\"";
    formStream << "}";
  } else {
    formStream << "{}";
  }
  return "{\"schema\":" + form.form + ",\"startval\":" + formStream.str() + "}";
}

uint64_t UserService::save(const std::string &name, std::string &email, const std::string &password, int type) {
  _connection->commit();
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  std::unique_ptr<Entities::User> user = _userDAO.read(email);
  if (!user) {
    std::string salt = Services::Crypto::getSalt();
    std::string passwd = Services::Crypto::getPasswordHash(password, salt);
    user = std::make_unique<Entities::User>();
    user->identifier(email);
    user->name(name);
    user->kind(type);
    user->identity("");
    user->password(passwd);
    user->salt(salt);
    _userDAO.save(*user);
    _connection->commit();
    return user->id();
  }
  return 0;
}

bool UserService::update(uint32_t identifier, const std::string &name, std::string &email, const std::string &password, int type) {
  _connection->commit();
  std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  std::unique_ptr<Entities::User> user = _userDAO.read(identifier);
  if (user) {
    user->identifier(email);
    user->name(name);
    user->kind(type);
    user->identity("");
    if (type == 0) {
      if (0 < password.length()) {
        std::string salt = Services::Crypto::getSalt();
        std::string passwd = Services::Crypto::getPasswordHash(password, salt);
        user->password(passwd);
        user->salt(salt);
      }
    } else {
      user->password("");
      user->salt("");
    }
    _userDAO.update(*user);
    _connection->commit();
    return true;
  } else {
    return false;
  }
}

void UserService::remove(uint32_t identifier) {
  _connection->commit();
  std::unique_ptr<Entities::User> user = _userDAO.read(identifier);
  if (user) {
    _userDAO.remove(user->identifier());
    _connection->commit();
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
} /* namespace Servers */
} /* namespace SyncServer */
