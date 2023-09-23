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

#include "UserDAO.h"

#include <dao/sql/Connection.h>
#include <dao/sql/ResultSet.h>
#include <dao/sql/Statement.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

void UserDAO::save(Entities::User &user) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO `users`(`identifier`,`name`,`kind`,`identity`,`password`,`salt`) VALUES(?, ?, ?, ?, ?, ?);");
  stmt->setString(1, user.identifier());
  stmt->setString(2, user.name());
  stmt->setInt(3, user.kind());
  stmt->setString(4, user.identity());
  stmt->setString(5, user.password());
  stmt->setString(6, user.salt());
  stmt->executeUpdate();
  user.id(stmt->getLastId());
}

std::unique_ptr<Entities::User> UserDAO::read(const std::string &identifier) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::unique_ptr<Entities::User> user;
  stmt = _connection->prepareStatement("SELECT `iduser`, `identifier`, `name`, `kind`, `identity`,`password`,`salt` FROM `users` WHERE `identifier` = ?;");
  stmt->setString(1, identifier);
  res = stmt->executeQuery();
  if (res->next()) {
    user = std::make_unique<Entities::User>();
    user->id(res->getInt("iduser"));
    user->identifier(res->getString("identifier"));
    user->name(res->getString("name"));
    user->kind(res->getInt("kind"));
    user->identity(res->getString("identity"));
    user->password(res->getString("password"));
    user->salt(res->getString("salt"));
  }
  delete res;
  return user;
}

std::unique_ptr<Entities::User> UserDAO::read(const uint32_t identifier) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::unique_ptr<Entities::User> user;
  stmt = _connection->prepareStatement("SELECT `iduser`, `identifier`, `name`, `kind`, `identity`,`password`,`salt` FROM `users` WHERE `iduser` = ?;");
  stmt->setInt(1, identifier);
  res = stmt->executeQuery();
  if (res->next()) {
    user = std::make_unique<Entities::User>();
    user->id(res->getInt("iduser"));
    user->identifier(res->getString("identifier"));
    user->name(res->getString("name"));
    user->kind(res->getInt("kind"));
    user->identity(res->getString("identity"));
    user->password(res->getString("password"));
    user->salt(res->getString("salt"));
  }
  delete res;
  return user;
}

std::vector<Entities::User> UserDAO::read() {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::vector<Entities::User> users;
  stmt = _connection->prepareStatement("SELECT `iduser`, `identifier`, `name`, `kind`, `identity`,`password`,`salt` FROM `users`;");
  res = stmt->executeQuery();
  while (res->next()) {
    Entities::User user;
    user.id(res->getInt("iduser"));
    user.identifier(res->getString("identifier"));
    user.name(res->getString("name"));
    user.kind(res->getInt("kind"));
    user.identity(res->getString("identity"));
    user.password(res->getString("password"));
    user.salt(res->getString("salt"));
    users.push_back(user);
  }
  delete res;
  return users;
}

void UserDAO::update(Entities::User &user) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("UPDATE `users` SET `identifier` = ?, `name` = ?, `kind` = ?, `password` = ?, `salt` = ? WHERE `iduser` = ?;");
  stmt->setString(1, user.identifier());
  stmt->setString(2, user.name());
  stmt->setInt(3, user.kind());
  stmt->setString(4, user.password());
  stmt->setString(5, user.salt());
  stmt->setInt(6, user.id());
  stmt->executeUpdate();

}

int UserDAO::remove(const std::string &identifier) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("DELETE FROM `users` WHERE `identifier` = ?;");
  stmt->setString(1, identifier);
  return stmt->executeUpdate();
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
