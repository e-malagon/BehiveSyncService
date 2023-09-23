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


#include <dao/PushDAO.hpp>
#include <dao/Storage.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string PushDAO::prefix("P.");

void PushDAO::save(Entities::Push &push, const std::string &context) {
/*
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO `push`(`iddataset`, `uuid`, `role`, `until`, `number`) VALUES(?, ?, ?, ?, ?);");
  stmt->setInt(1, push.idDataset());
  stmt->setString(2, push.uuid());
  stmt->setUUID(3, push.role());
  stmt->setLong(4, push.until());
  stmt->setInt(5, push.number());
  stmt->executeUpdate();
  */
}

std::unique_ptr<Entities::Push> PushDAO::read(uint32_t idDataset, std::string &uuid, const std::string &context) {
  std::unique_ptr<Entities::Push> push;
/*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SELECT `iddataset`, `uuid`, `role`, `until`, `number` FROM `push` WHERE `iddataset` = ? AND `uuid` = ?;");
  stmt->setInt(1, idDataset);
  stmt->setString(2, uuid);
  res = stmt->executeQuery();
  if (res->next()) {
    push = std::make_unique<Entities::Push>();
    push->idDataset(res->getInt("iddataset"));
    push->uuid(res->getString("uuid"));
    push->role(res->getUUID("role"));
    push->until(res->getLong("until"));
    push->number(res->getInt("number"));
  }
  delete res;
  */
  return push;
}

std::vector<Entities::Push> PushDAO::readByDataset(uint32_t idDataset, const std::string &context) {
  std::vector<Entities::Push> pushs;
/*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SELECT `iddataset`, `uuid`, `role`, `until`, `number` FROM `push` WHERE `iddataset` = ?;");
  stmt->setInt(1, idDataset);
  res = stmt->executeQuery();
  while (res->next()) {
    Entities::Push push;
    push.idDataset(res->getInt("iddataset"));
    push.uuid(res->getString("uuid"));
    push.role(res->getUUID("role"));
    push.until(res->getLong("until"));
    push.number(res->getInt("number"));
    pushs.push_back(push);
  }
  delete res;
  */
  return pushs;
}

int PushDAO::update(Entities::Push &push, const std::string &context) {
  /*
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("UPDATE `push` SET `role` = ?, `number` = ? WHERE `iddataset` = ? AND `uuid` = ?;");
  stmt->setString(1, push.role());
  stmt->setInt(2, push.number());
  stmt->setInt(3, push.idDataset());
  stmt->setString(4, push.uuid());
  return stmt->executeUpdate();
  */
 return 0;
}

int PushDAO::remove(uint32_t idDataset, std::string &uuid, const std::string &context) {
  /*
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("DELETE FROM `push` WHERE `iddataset` = ? AND `uuid` = ?;");
  stmt->setInt(1, idDataset);
  stmt->setString(2, uuid);
  return stmt->executeUpdate();
  */
 return 0;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
