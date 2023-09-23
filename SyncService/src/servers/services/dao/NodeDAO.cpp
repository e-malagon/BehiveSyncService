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

#include "NodeDAO.h"

#include <dao/sql/Connection.h>
#include <dao/sql/ResultSet.h>
#include <dao/sql/Statement.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

void NodeDAO::save(Entities::Node &node) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO `nodes`(`idnode`, `iduser`, `key`, `application`, `module`, `uuid`) VALUES(0, ?, ?, ?, ?, ?);");
  stmt->setInt(1, node.user().id());
  stmt->setString(2, node.key());
  stmt->setUUID(3, node.application());
  stmt->setUUID(4, node.module());
  stmt->setString(5, node.uuid());
  stmt->executeUpdate();
  node.id(stmt->getLastId());
}

std::unique_ptr<Entities::Node> NodeDAO::read(uint32_t idNode) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::unique_ptr<Entities::Node> node;
  stmt = _connection->prepareStatement("SELECT `idnode`, `iduser`, `key`, `application`, `module`, `uuid` FROM `nodes` WHERE `idnode` = ?;");
  stmt->setInt(1, idNode);
  res = stmt->executeQuery();
  if (res->next()) {
    node = std::make_unique<Entities::Node>();
    node->id(res->getInt("idnode"));
    node->user().id(res->getInt("iduser"));
    node->key(res->getString("key"));
    node->application(res->getUUID("application"));
    node->module(res->getUUID("module"));
    node->uuid(res->getString("uuid"));
  }
  delete res;
  return node;
}

std::unique_ptr<Entities::Node> NodeDAO::read(std::string uuidNode, uint64_t idUser) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::unique_ptr<Entities::Node> node;
  stmt = _connection->prepareStatement("SELECT `idnode`, `iduser`, `key`, `application`, `module`, `uuid` FROM `nodes` WHERE `iduser` = ? AND `uuid` = ?;");
  stmt->setInt(1, idUser);
  stmt->setString(2, uuidNode);
  res = stmt->executeQuery();
  if (res->next()) {
    node = std::make_unique<Entities::Node>();
    node->id(res->getInt("idnode"));
    node->user().id(res->getInt("iduser"));
    node->key(res->getString("key"));
    node->application(res->getUUID("application"));
    node->module(res->getUUID("module"));
    node->uuid(res->getString("uuid"));
  }
  delete res;
  return node;
}

int NodeDAO::updateKey(Entities::Node &node) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("UPDATE `nodes` SET `key` = ? WHERE `idnode` = ?;");
  stmt->setString(1, node.key());
  stmt->setInt(2, node.id());
  return stmt->executeUpdate();
}

int NodeDAO::update(Entities::Node &node) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("UPDATE `nodes` SET `lastsync` = CURRENT_TIMESTAMP WHERE `idnode` = ?;");
  stmt->setInt(1, node.id());
  return stmt->executeUpdate();
}

int NodeDAO::remove(uint32_t idNode) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("DELETE FROM `nodes` WHERE `idnode` = ?;");
  stmt->setInt(1, idNode);
  return stmt->executeUpdate();
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
