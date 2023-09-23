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

#include "MemberDAO.h"

#include <dao/sql/Connection.h>
#include <dao/sql/ResultSet.h>
#include <dao/sql/Statement.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

void MemberDAO::save(Entities::Member &member) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO `members`(`iddataset`, `iduser`, `role`, `name`, `status`) VALUES(?, ?, ?, ?, ?);");
  stmt->setInt(1, member.idDataset());
  stmt->setInt(2, member.idUser());
  stmt->setUUID(3, member.role());
  stmt->setString(4, member.name());
  stmt->setInt(5, member.status());
  stmt->executeUpdate();
}

std::unique_ptr<Entities::Member> MemberDAO::read(uint32_t idDataset, uint32_t idUser) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::unique_ptr<Entities::Member> member;
  stmt = _connection->prepareStatement("SELECT `iddataset`, `iduser`, `role`, `name`, `status` FROM `members` WHERE `iddataset` = ? AND `iduser` = ?;");
  stmt->setInt(1, idDataset);
  stmt->setInt(2, idUser);
  res = stmt->executeQuery();
  if (res->next()) {
    member = std::make_unique<Entities::Member>();
    member->idDataset(res->getInt("iddataset"));
    member->idUser(res->getInt("iduser"));
    member->role(res->getUUID("role"));
    member->name(res->getString("name"));
    member->status(res->getInt("status"));
  }
  delete res;
  return member;
}

std::vector<Entities::Member> MemberDAO::readByDataset(uint32_t idDataset, bool readEmail) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::vector<Entities::Member> members;
  if (readEmail)
    stmt = _connection->prepareStatement("SELECT `iddataset`, a.`iduser`, `role`, a.`name`, `status`, `identifier` FROM `members` a, `users` b WHERE `iddataset` = ? AND a.`iduser` = b.`iduser`;");
  else
    stmt = _connection->prepareStatement("SELECT `iddataset`, `iduser`, `role`, `name`, `status` FROM `members` WHERE `iddataset` = ?;");
  stmt->setInt(1, idDataset);
  res = stmt->executeQuery();
  while (res->next()) {
    Entities::Member member;
    member.idDataset(res->getInt("iddataset"));
    member.idUser(res->getInt("iduser"));
    member.role(res->getUUID("role"));
    member.name(res->getString("name"));
    member.email(readEmail ? res->getString("identifier") : "");
    member.status(res->getInt("status"));
    members.push_back(member);
  }
  delete res;
  return members;
}

int MemberDAO::update(Entities::Member &member) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("UPDATE `members` SET `role` = ?, `name` = ?, `status` = ? WHERE `iddataset` = ? AND `iduser` = ?;");
  stmt->setUUID(1, member.role());
  stmt->setString(2, member.name());
  stmt->setInt(3, member.status());
  stmt->setInt(4, member.idDataset());
  stmt->setInt(5, member.idUser());
  return stmt->executeUpdate();
}

int MemberDAO::remove(uint32_t idDataset, uint32_t idUser) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("DELETE FROM `members` WHERE `iddataset` = ? AND `iduser` = ?;");
  stmt->setInt(1, idDataset);
  stmt->setInt(2, idUser);
  return stmt->executeUpdate();
}

int MemberDAO::removeDataset(uint32_t idDataset) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("DELETE FROM `members` WHERE `iddataset` = ?;");
  stmt->setInt(1, idDataset);
  return stmt->executeUpdate();
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
