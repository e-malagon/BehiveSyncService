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

#include "ChangeDAO.h"

#include <sqlite/BinaryDecoder.h>
#include <sqlite/TextEncoder.h>

#include <config/Entity.h>
#include <dao/sql/Connection.h>
#include <dao/sql/ResultSet.h>
#include <dao/sql/Statement.h>
#include <sqlite/Types.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

void ChangeDAO::save(Entities::Change &change) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO changes(`iddataset`,`idheader`,`idchange`,`operation`,`entity`,`key`,`old`,`data`) VALUES(?, ?, ?, ?, ?, ?, ?, ?);");
  stmt->setInt(1, change.idDataset());
  stmt->setInt(2, change.idHeader());
  stmt->setInt(3, change.idChange());
  stmt->setInt(4, change.operation());
  stmt->setUUID(5, change.entityUUID());
  switch (change.operation()) {
  case SqLite::Operation::Insert:
    stmt->setString(6, change.newPK());
    stmt->setString(7, "");
    stmt->setString(8, change.newData());
    break;
  case SqLite::Operation::Update:
    stmt->setString(6, change.newPK());
    stmt->setString(7, change.oldPK());
    stmt->setString(8, change.newData());
    break;
  case SqLite::Operation::Delete:
    stmt->setString(6, change.oldPK());
    stmt->setString(7, "");
    stmt->setString(8, "");
    break;
  }
  stmt->executeUpdate();
}

std::vector<Entities::Change> ChangeDAO::readByHeader(uint32_t idDataset, uint32_t idHeader, const std::unordered_map<std::string, Config::Entity, Utils::IHasher, Utils::IEqualsComparator> &entities, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::vector<Entities::Change> changes;
  stmt = _connection->prepareStatement("SELECT `iddataset`,`idheader`,`idchange`,`operation`,`entity`,`key`,`old`,`data` FROM `changes` WHERE `iddataset` = ? AND `idheader` = ?;");
  stmt->setInt(1, idDataset);
  stmt->setInt(2, idHeader);
  res = stmt->executeQuery();
  while (res->next()) {
    Entities::Change change;
    std::string entityUUID = res->getUUID("entity");
    auto entityPtr = entities.find(entityUUID);
    auto entityByNodePtr = entitiesByNode.find(entityUUID);
    if (entityPtr == entities.end() || entityByNodePtr == entitiesByNode.end())
      continue;
    change.idDataset(res->getInt("iddataset"));
    change.idHeader(res->getInt("idheader"));
    change.idChange(res->getInt("idchange"));
    change.operation(res->getInt("operation"));
    change.entityName(entityPtr->second.name);
    switch (change.operation()) {
    case SqLite::Operation::Insert: {
      std::string opPK = res->getString("key");
      std::string opData = res->getString("data");
      SqLite::BinaryDecoder newPK(opPK.c_str(), opPK.size());
      SqLite::BinaryDecoder newData(opData.c_str(), opData.size());
      SqLite::TextEncoder newTextPK(entityPtr->second.keysId2Name);
      SqLite::TextEncoder newTextData(entityPtr->second.attributesId2Name);
      for (SqLite::BinaryDecoder::Value &value : newPK) {
        newTextPK.addValue(value);
      }
      for (SqLite::BinaryDecoder::Value &value : newData) {
        if (entityByNodePtr->second.find(value.id()) != entityByNodePtr->second.end())
          newTextData.addValue(value);
      }
      change.newPK(newTextPK.encodedData());
      change.newData(newTextData.encodedData());
      change.oldPK("");
      change.oldData("");
    }
      break;
    case SqLite::Operation::Update: {
      std::string opPK = res->getString("key");
      std::string opOPK = res->getString("old");
      std::string opData = res->getString("data");
      SqLite::BinaryDecoder newPK(opPK.c_str(), opPK.size());
      SqLite::BinaryDecoder oldPK(opOPK.c_str(), opOPK.size());
      SqLite::BinaryDecoder newData(opData.c_str(), opData.size());
      SqLite::TextEncoder newTextPK(entityPtr->second.keysId2Name);
      SqLite::TextEncoder oldTextPK(entityPtr->second.keysId2Name);
      SqLite::TextEncoder newTextData(entityPtr->second.attributesId2Name);
      for (SqLite::BinaryDecoder::Value &value : newPK) {
        newTextPK.addValue(value);
      }
      for (SqLite::BinaryDecoder::Value &value : oldPK) {
        oldTextPK.addValue(value);
      }
      for (SqLite::BinaryDecoder::Value &value : newData) {
        if (entityByNodePtr->second.find(value.id()) != entityByNodePtr->second.end())
          newTextData.addValue(value);
      }
      change.newPK(newTextPK.encodedData());
      change.newData(newTextData.encodedData());
      change.oldPK(oldTextPK.encodedData());
      change.oldData("");
    }
      break;
    case SqLite::Operation::Delete: {
      std::string opPK = res->getString("key");
      SqLite::BinaryDecoder oldPK(opPK.c_str(), opPK.size());
      SqLite::TextEncoder oldTextPK(entityPtr->second.keysId2Name);
      for (SqLite::BinaryDecoder::Value &value : oldPK) {
        oldTextPK.addValue(value);
      }
      change.newPK("");
      change.newData("");
      change.oldPK(oldTextPK.encodedData());
      change.oldData("");
    }
      break;
    }
    changes.push_back(change);
  }
  delete res;
  return changes;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
