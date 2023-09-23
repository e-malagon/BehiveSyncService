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

#include "EntityDAO.h"

#include <sqlite/BinaryDecoder.h>
#include <sqlite/BinaryEncoder.h>

#include <nanolog/NanoLog.hpp>
#include <dao/sql/Connection.h>
#include <dao/sql/ResultSet.h>
#include <sstream>

#include <ServiceException.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

auto c2h = [](char input) {
  if (input >= '0' && input <= '9')
    return input - '0';
  if (input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if (input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw std::invalid_argument("Invalid input character string uuid to binary");
};

std::string EntityDAO::uuid12bin(std::string uuid) {
  uint8_t uuidbin[16];
  const char *uuidChar = uuid.c_str();
  uint8_t indexes[] = { 14, 16, 9, 11, 0, 2, 4, 6, 19, 21, 24, 26, 28, 30, 32, 34 };
  for (int i = 0; i < 16; ++i)
    uuidbin[i] = c2h(uuidChar[indexes[i]]) * 16 + c2h(uuidChar[indexes[i] + 1]);
  return std::string((char*) uuidbin, 16);
}

std::string EntityDAO::uuidt2bin(std::string uuid) {
  uint8_t uuidbin[16];
  const char *uuidChar = uuid.c_str();
  uint8_t indexes[] = { 0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };
  for (int i = 0; i < 16; ++i)
    uuidbin[i] = c2h(uuidChar[indexes[i]]) * 16 + c2h(uuidChar[indexes[i] + 1]);
  return std::string((char*) uuidbin, 16);
}

auto h2c = [](uint8_t input, char *out) {
  if ((input & 0x0f) < 10)
    *(out + 1) = (input & 0x0f) + '0';
  else
    *(out + 1) = (input & 0x0f) + 'a' - 10;
  input >>= 4;
  if (input < 10)
    *(out) = input + '0';
  else
    *(out) = input + 'a' - 10;
};

std::string bin2uuid1(std::string uuid) {
  char uuidDecoded[36];
  const char *uuidBin = uuid.c_str();
  memset(uuidDecoded, '-', 36);
  int indexes[] = { 14, 16, 9, 11, 0, 2, 4, 6, 19, 21, 24, 26, 28, 30, 32, 34 };
  for (int i = 0; i < 16; ++i)
    h2c(uuidBin[i], &uuidDecoded[indexes[i]]);
  return std::string(uuidDecoded, 36);
}

std::string bin2uuidt(std::string uuid) {
  char uuidDecoded[36];
  const char *uuidBin = uuid.c_str();
  memset(uuidDecoded, '-', 36);
  int indexes[] = { 0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34 };
  for (int i = 0; i < 16; ++i)
    h2c(uuidBin[i], &uuidDecoded[indexes[i]]);
  return std::string(uuidDecoded, 36);
}

bool EntityDAO::isEntityAvailable(const std::string &name) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SHOW TABLES LIKE '" + name + "';", false, true);
  res = stmt->executeQuery();
  bool exist = res->next();
  delete res;
  return exist;
}

bool EntityDAO::createEntity(const Config::Entity &entity) {
  SQL::Statement *stmt;
  std::stringstream ss;
  ss << "CREATE TABLE IF NOT EXISTS `" << entity.id << "` (" << std::endl;
  ss << "   `iddataset` int unsigned NOT NULL, " << std::endl;
  for (auto &key : entity.keys) {
    switch (key.second.type) {
    case SqLite::AttributeType::Integer:
      ss << "   `k" + std::to_string(key.first) + "` bigint NOT NULL, " << std::endl;
      break;
    case SqLite::AttributeType::Text:
      ss << "   `k" + std::to_string(key.first) + "` varchar(256) NOT NULL, " << std::endl;
      break;
    case SqLite::AttributeType::Blob:
      ss << "   `k" + std::to_string(key.first) + "` varbinary(256) NOT NULL, " << std::endl;
      break;
    case SqLite::AttributeType::UuidV1:
    case SqLite::AttributeType::UuidV4:
      ss << "   `k" + std::to_string(key.first) + "` binary(16) NOT NULL, " << std::endl;
      break;
    default:
      break;
    }
  }
  ss << "   `key` varbinary(256) NOT NULL, " << std::endl;
  ss << "   `data` varbinary(32768) NOT NULL, " << std::endl;
  ss << "   PRIMARY KEY (`iddataset`";
  for (auto &key : entity.keys) {
    ss << ",`k" + std::to_string(key.first) + "`";
  }
  ss << "), " << std::endl;
  ss << "   CONSTRAINT `fk_entity" << entity.id << "` FOREIGN KEY (`iddataset`) REFERENCES `datasets` (`iddataset`) ON DELETE CASCADE " << std::endl;
  ss << ") ENGINE=InnoDB DEFAULT CHARSET=binary;" << std::endl;
  stmt = _connection->prepareStatement(ss.str(), false, true);
  stmt->executeUpdate();
  delete stmt;
  return true;
}

bool EntityDAO::dropEntity(const std::string &entity) {
  SQL::Statement *stmt;
  std::stringstream ss;
  ss << "DROP TABLE IF EXISTS `" << entity << "`;" << std::endl;
  stmt = _connection->prepareStatement(ss.str(), false, true);
  stmt->executeUpdate();
  delete stmt;
  return true;
}

SQL::ResultSet* EntityDAO::read(uint32_t idDataset, const Config::Entity &entity) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::stringstream ss1;
  std::stringstream ss2;
  ss1 << "SELECT";
  for (auto &key : entity.keys) {
    ss1 << " `k" + std::to_string(key.second.id) + "`,";
    ss2 << ", `k" + std::to_string(key.second.id) + "`";
  }
  ss1 << " `key`,`data` FROM `" << entity.id << "` WHERE `iddataset` = ? ORDER BY `iddataset`" << ss2.str() << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, idDataset);
  res = stmt->executeQuery();
  return res;
}

std::vector<Entities::KeyData> EntityDAO::read(uint32_t idDataset, const sol::table &keys, const Config::Entity &entity) {
  std::vector<Entities::KeyData> keyDataV;
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::stringstream ss1;
  std::unordered_map<int, int> indexes;
  ss1 << "SELECT `key`,`data` FROM `" + entity.id + "` WHERE `iddataset` = ?";
  int index = 2;
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr != entity.keysName2Id.end()) {
      ss1 << " AND `k" + std::to_string(keyMappingsPtr->second) + "` = ?";
      indexes.emplace(keyMappingsPtr->second, index++);
    } else {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return keyDataV;
    }
  }
  ss1 << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, idDataset);
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr == entity.keysName2Id.end()) {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return keyDataV;
    }
    const auto &keyPtr = entity.keys.find(keyMappingsPtr->second);
    if (keyPtr == entity.keys.end()) {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return keyDataV;
    }
    switch (keyPtr->second.type) {
    case SqLite::AttributeType::Integer: {
      if (key.second.get_type() != sol::type::number) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      uint64_t iValue = key.second.as<uint64_t>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      stmt->setLong(indexes[keyMappingsPtr->second], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      stmt->setString(indexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      stmt->setString(indexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      std::string uuid1 = uuid12bin(sValue);
      stmt->setString(indexes[keyMappingsPtr->second], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      std::string uuidt = uuidt2bin(sValue);
      stmt->setString(indexes[keyMappingsPtr->second], uuidt);
    }
      break;
    default:
      break;
    }
  }
  res = stmt->executeQuery();
  while (res->next()) {
    Entities::KeyData keyData;
    keyData.oldPK(res->getString("key"));
    keyData.oldData(res->getString("data"));
    keyDataV.push_back(keyData);
  }
  delete res;
  return keyDataV;
}

Entities::KeyData EntityDAO::read(Entities::Change &change, const Config::Entity &entity) {
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::stringstream ss1;
  std::unordered_map<int, int> indexes;
  ss1 << "SELECT `key`,`data` FROM `" + entity.id + "` WHERE `iddataset` = ?";
  int index = 2;
  for (auto &key : entity.keys) {
    ss1 << " AND `k" + std::to_string(key.first) + "` = ?";
    indexes.emplace(key.first, index++);
  }
  ss1 << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, change.idDataset());
  SqLite::BinaryDecoder oldPK(change.oldPK().data(), change.oldPK().size());
  for (SqLite::BinaryDecoder::Value &value : oldPK) {
    int id = value.id();
    const auto &key = entity.keys.find(id);
    if (key == entity.keys.end())
      throw SchemaDefinitionException("Attribute " + std::to_string(id) + " not found");
    switch (key->second.type) {
    case SqLite::AttributeType::Integer:
      stmt->setLong(indexes[key->first], value.integerValue());
      break;
    case SqLite::AttributeType::Text:
      stmt->setString(indexes[key->first], value.textValue());
      break;
    case SqLite::AttributeType::Blob:
      stmt->setString(indexes[key->first], value.blobValue());
      break;
    case SqLite::AttributeType::UuidV1: {
      std::string uuid1 = uuid12bin(value.textValue());
      stmt->setString(indexes[key->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      std::string uuidt = uuidt2bin(value.textValue());
      stmt->setString(indexes[key->first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  Entities::KeyData keyData;
  res = stmt->executeQuery();
  if (res->next()) {
    keyData.oldPK(res->getString("key"));
    keyData.oldData(res->getString("data"));
  }
  delete res;
  return keyData;
}

int EntityDAO::save(uint32_t idDataset, const sol::table &data, const Config::Entity &entity) {
  SQL::Statement *stmt;
  std::stringstream ss1;
  std::stringstream ss2;
  std::unordered_map<int, int> indexes;
  ss1 << "INSERT INTO `" + entity.id + "`(`iddataset`";
  ss2 << "?";

  int index = 2;
  for (auto &key : entity.keys) {
    ss1 << ", `k" + std::to_string(key.first) + "`";
    ss2 << ", ?";
    indexes.emplace(key.first, index++);
  }
  ss1 << ", `key`, `data`) VALUES(" << ss2.str() << ", ?, ?);";
  stmt = _connection->prepareStatement(ss1.str());
  index = 1;
  stmt->setInt(index++, idDataset);
  SqLite::BinaryEncoder newPK;
  SqLite::BinaryEncoder newData;
  for (auto &key : entity.keys) {
    sol::object keyValue = data[key.second.name];
    if (keyValue.get_type() == sol::type::lua_nil) {
      LOG_FILE_ERROR(_beehive) << "Missing key " << entity.name << "." << key.second.name;
      return 0;
    }
    index++;
    switch (key.second.type) {
    case SqLite::AttributeType::Integer: {
      if (keyValue.get_type() != sol::type::number) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      uint64_t iValue = keyValue.as<uint64_t>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newPK.addInteger(key.first, iValue);
      stmt->setLong(indexes[key.first], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newPK.addText(key.first, sValue);
      stmt->setString(indexes[key.first], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newPK.addBlob(key.first, sValue);
      stmt->setString(indexes[key.first], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newPK.addText(key.first, sValue);
      std::string uuid1 = uuid12bin(sValue);
      stmt->setString(indexes[key.first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newPK.addText(key.first, sValue);
      std::string uuidt = uuidt2bin(sValue);
      stmt->setString(indexes[key.first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  for (auto &col : entity.attributes) {
    sol::object colValue = data[col.second.name];
    if (colValue.get_type() == sol::type::lua_nil && col.second.notnull) {
      LOG_FILE_ERROR(_beehive) << "Missing attribute " << entity.name << "." << col.second.name;
      return 0;
    }
    switch (col.second.type) {
    case SqLite::AttributeType::Integer: {
      if (colValue.get_type() != sol::type::number) {
        LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      long iValue = colValue.as<long>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newData.addInteger(col.first, (uint64_t) iValue);
    }
      break;
    case SqLite::AttributeType::Real: {
      if (colValue.get_type() != sol::type::number) {
        LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      double dValue = colValue.as<double>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newData.addReal(col.first, dValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (colValue.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      std::string sValue = colValue.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newData.addText(col.first, sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (colValue.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      std::string sValue = colValue.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      newData.addBlob(col.first, sValue);
    }
      break;
    default:
      break;
    }
  }
  stmt->setString(index++, newPK.encodedData());
  stmt->setString(index++, newData.encodedData());
  return stmt->executeUpdate();
}

void EntityDAO::save(Entities::Change &change, const Config::Entity &entity) {
  SQL::Statement *stmt;
  std::stringstream ss1;
  std::stringstream ss2;
  std::unordered_map<int, int> indexes;
  ss1 << "INSERT INTO `" + entity.id + "`(`iddataset`";
  ss2 << "?";

  int index = 2;
  for (auto &key : entity.keys) {
    ss1 << ", `k" + std::to_string(key.first) + "`";
    ss2 << ", ?";
    indexes.emplace(key.first, index++);
  }
  ss1 << ", `key`, `data`) VALUES(" << ss2.str() << ", ?, ?);";
  LOG_DEBUG << ss1.str();
  stmt = _connection->prepareStatement(ss1.str());
  index = 1;
  stmt->setInt(index++, change.idDataset());
  SqLite::BinaryDecoder newPK(change.newPK().data(), change.newPK().size());
  for (SqLite::BinaryDecoder::Value &value : newPK) {
    index++;
    int id = value.id();
    const auto &key = entity.keys.find(id);
    if (key == entity.keys.end())
      throw SchemaDefinitionException("Attribute " + std::to_string(id) + " not found");
    switch (key->second.type) {
    case SqLite::AttributeType::Integer:
      stmt->setLong(indexes[key->first], value.integerValue());
      break;
    case SqLite::AttributeType::Text:
      stmt->setString(indexes[key->first], value.textValue());
      break;
    case SqLite::AttributeType::Blob:
      stmt->setString(indexes[key->first], value.blobValue());
      break;
    case SqLite::AttributeType::UuidV1: {
      std::string uuid1 = uuid12bin(value.textValue());
      stmt->setString(indexes[key->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      std::string uuidt = uuidt2bin(value.textValue());
      stmt->setString(indexes[key->first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  stmt->setString(index++, change.newPK());
  stmt->setString(index++, change.newData());
  stmt->executeUpdate();
}

int EntityDAO::update(uint32_t idDataset, const sol::table &keys, const sol::table &data, const Config::Entity &entity) {
  SQL::Statement *selectStmt;
  SQL::Statement *updateStmt;
  SQL::ResultSet *res;
  std::stringstream ss1;
  std::unordered_map<int, int> updateIndexes;
  ss1 << "UPDATE `" + entity.id + "` SET `data` = ? WHERE `iddataset` = ?";
  int index = 3;
  for (auto &key : entity.keys) {
    ss1 << " AND `k" + std::to_string(key.first) + "` = ?";
    updateIndexes.emplace(key.first, index++);
  }
  ss1 << ";";
  updateStmt = _connection->prepareStatement(ss1.str());
  ss1.str("");
  std::unordered_map<int, int> selectIndexes;
  ss1 << "SELECT `key`,`data` FROM `" + entity.id + "` WHERE `iddataset` = ?";
  index = 2;
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr != entity.keysName2Id.end()) {
      ss1 << " AND `k" + std::to_string(keyMappingsPtr->second) + "` = ?";
      selectIndexes.emplace(keyMappingsPtr->second, index++);
    } else {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return 0;
    }
  }
  ss1 << ";";
  selectStmt = _connection->prepareStatement(ss1.str());
  selectStmt->setInt(1, idDataset);
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr == entity.keysName2Id.end()) {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    const auto &keyPtr = entity.keys.find(keyMappingsPtr->second);
    if (keyPtr == entity.keys.end()) {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    switch (keyPtr->second.type) {
    case SqLite::AttributeType::Integer: {
      if (key.second.get_type() != sol::type::number) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      uint64_t iValue = key.second.as<uint64_t>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      selectStmt->setLong(selectIndexes[keyMappingsPtr->second], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      std::string uuid1 = uuid12bin(sValue);
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      std::string uuidt = uuidt2bin(sValue);
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], uuidt);
    }
      break;
    default:
      break;
    }
  }

  int count = 0;
  res = selectStmt->executeQuery();
  while (res->next()) {
    std::string oldKey = res->getString("key");
    std::string oldData = res->getString("data");
    SqLite::BinaryDecoder keyDecoder(oldKey.data(), oldKey.size());
    SqLite::BinaryDecoder dataDecoder(oldData.data(), oldData.size());
    SqLite::BinaryEncoder encoder;
    for (SqLite::BinaryDecoder::Value &value : dataDecoder)
      encoder.addValue(value);

    for (auto &dat : data) {
      std::string name = dat.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      auto attributesMappingsPtr = entity.attributesName2Id.find(name);
      if (attributesMappingsPtr == entity.attributesName2Id.end()) {
        LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
        return 0;
      }
      const auto &colPtr = entity.attributes.find(attributesMappingsPtr->second);
      if (colPtr == entity.attributes.end()) {
        LOG_FILE_ERROR(_beehive) << "Attribute not fount " << entity.name << "." << name;
        return 0;
      }
      if (dat.second.get_type() == sol::type::nil) {
        encoder.addNull(attributesMappingsPtr->second);
      } else {
        switch (colPtr->second.type) {
        case SqLite::AttributeType::Integer: {
          if (dat.second.get_type() != sol::type::number) {
            LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          uint64_t iValue = dat.second.as<uint64_t>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
          encoder.addInteger(attributesMappingsPtr->second, iValue);
        }
          break;
        case SqLite::AttributeType::Real: {
          if (dat.second.get_type() != sol::type::number) {
            LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          double dValue = dat.second.as<double>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
          encoder.addInteger(attributesMappingsPtr->second, dValue);
        }
          break;
        case SqLite::AttributeType::Text: {
          if (dat.second.get_type() != sol::type::string) {
            LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          std::string sValue = dat.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
          encoder.addText(attributesMappingsPtr->second, sValue);
        }
          break;
        case SqLite::AttributeType::Blob: {
          if (dat.second.get_type() != sol::type::string) {
            LOG_FILE_ERROR(_beehive) << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          std::string sValue = dat.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
          encoder.addBlob(attributesMappingsPtr->second, sValue);
        }
          break;
        default:
          break;
        }
      }
    }

    updateStmt->setString(1, encoder.encodedData());
    updateStmt->setInt(2, idDataset);
    for (SqLite::BinaryDecoder::Value &key : keyDecoder) {
      const auto &keyPtr = entity.keys.find(key.id());
      if (keyPtr == entity.keys.end()) {
        LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << key.id();
        return 0;
      }
      switch (key.type()) {
      case SqLite::AttributeType::Integer:
        updateStmt->setLong(updateIndexes[keyPtr->first], key.integerValue());
        break;
      case SqLite::AttributeType::Text: {
        switch (keyPtr->second.type) {
        case SqLite::AttributeType::Text: {
          updateStmt->setString(updateIndexes[keyPtr->first], key.textValue());
        }
          break;
        case SqLite::AttributeType::UuidV1: {
          std::string uuid1 = uuid12bin(key.textValue());
          updateStmt->setString(updateIndexes[keyPtr->first], uuid1);
        }
          break;
        case SqLite::AttributeType::UuidV4: {
          std::string uuidt = uuidt2bin(key.textValue());
          updateStmt->setString(updateIndexes[keyPtr->first], uuidt);
        }
          break;
        default:
          break;
        }
      }
        break;
      case SqLite::AttributeType::Blob:
        updateStmt->setString(updateIndexes[keyPtr->first], key.blobValue());
        break;
      default:
        break;
      }
    }
    count += updateStmt->executeUpdate();
  }
  delete res;
  return count;
}

int EntityDAO::update(Entities::Change &change, const Config::Entity &entity) {
  SQL::Statement *stmt;
  std::stringstream ss1;
  std::stringstream ss2;
  std::unordered_map<int, int> indexes;
  ss1 << "UPDATE `" + entity.id + "` SET ";

  int index = 1;
  for (auto &key : entity.keys) {
    ss1 << "`k" + std::to_string(key.first) + "` = ?, ";
    ss2 << " AND `k" + std::to_string(key.first) + "` = ?";
    indexes.emplace(key.first, index++);
  }
  ss1 << "`key` = ?, `data` = ? WHERE `iddataset` = ?" << ss2.str() << ";";
  LOG_DEBUG << ss1.str();
  stmt = _connection->prepareStatement(ss1.str());
  index = 1;
  if(change.newPK().length() == 0)
    change.newPK(change.oldPK());
  SqLite::BinaryDecoder newPK(change.newPK().data(), change.newPK().size());
  for (SqLite::BinaryDecoder::Value &value : newPK) {
    index++;
    int id = value.id();
    const auto &key = entity.keys.find(id);
    if (key == entity.keys.end())
      throw SchemaDefinitionException("Attribute " + std::to_string(id) + " not found");
    switch (key->second.type) {
    case SqLite::AttributeType::Integer:
      stmt->setLong(indexes[key->first], value.integerValue());
      break;
    case SqLite::AttributeType::Text:
      stmt->setString(indexes[key->first], value.textValue());
      break;
    case SqLite::AttributeType::Blob:
      stmt->setString(indexes[key->first], value.blobValue());
      break;
    case SqLite::AttributeType::UuidV1: {
      std::string uuid1 = uuid12bin(value.textValue());
      stmt->setString(indexes[key->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      std::string uuidt = uuidt2bin(value.textValue());
      stmt->setString(indexes[key->first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  stmt->setString(index++, change.newPK());
  stmt->setString(index++, change.newData());
  stmt->setInt(index, change.idDataset());

  SqLite::BinaryDecoder oldPK(change.oldPK().data(), change.oldPK().size());
  for (SqLite::BinaryDecoder::Value &value : oldPK) {
    int id = value.id();
    const auto &key = entity.keys.find(id);
    if (key == entity.keys.end())
      throw SchemaDefinitionException("Attribute " + std::to_string(id) + " not found");
    switch (key->second.type) {
    case SqLite::AttributeType::Integer:
      stmt->setLong(index + indexes[key->first], value.integerValue());
      break;
    case SqLite::AttributeType::Text:
      stmt->setString(index + indexes[key->first], value.textValue());
      break;
    case SqLite::AttributeType::Blob:
      stmt->setString(index + indexes[key->first], value.blobValue());
      break;
    case SqLite::AttributeType::UuidV1: {
      std::string uuid1 = uuid12bin(value.textValue());
      stmt->setString(index + indexes[key->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      std::string uuidt = uuidt2bin(value.textValue());
      stmt->setString(index + indexes[key->first], uuidt);
    }
    break;
    default:
      break;
    }
  }
  return stmt->executeUpdate();
}

int EntityDAO::remove(uint32_t idDataset, const sol::table &keys, const Config::Entity &entity) {
  SQL::Statement *stmt;
  std::stringstream ss1;
  std::unordered_map<int, int> indexes;
  ss1 << "DELETE FROM `" + entity.id + "` WHERE `iddataset` = ?";

  int index = 2;
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr != entity.keysName2Id.end()) {
      ss1 << " AND `k" + std::to_string(keyMappingsPtr->second) + "` = ?";
      indexes.emplace(keyMappingsPtr->second, index++);
    } else {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return 0;
    }
  }
  ss1 << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, idDataset);
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr == entity.keysName2Id.end()) {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    const auto &keyPtr = entity.keys.find(keyMappingsPtr->second);
    if (keyPtr == entity.keys.end()) {
      LOG_FILE_ERROR(_beehive) << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    switch (keyPtr->second.type) {
    case SqLite::AttributeType::Integer: {
      if (key.second.get_type() != sol::type::number) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      uint64_t iValue = key.second.as<uint64_t>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      stmt->setLong(indexes[keyPtr->first], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      stmt->setString(indexes[keyPtr->first], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      stmt->setString(indexes[keyPtr->first], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      std::string uuid1 = uuid12bin(sValue);
      stmt->setString(indexes[keyPtr->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (key.second.get_type() != sol::type::string) {
        LOG_FILE_ERROR(_beehive) << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>(); // @suppress("Invalid arguments") // @suppress("Symbol is not resolved")
      std::string uuidt = uuidt2bin(sValue);
      stmt->setString(indexes[keyPtr->first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  return stmt->executeUpdate();
}

int EntityDAO::remove(Entities::Change &change, const Config::Entity &entity) {
  SQL::Statement *stmt;
  std::stringstream ss1;
  std::unordered_map<int, int> indexes;
  ss1 << "DELETE FROM `" + entity.id + "` WHERE `iddataset` = ?";
  int index = 2;
  for (auto &key : entity.keys) {
    ss1 << " AND `k" + std::to_string(key.first) + "` = ?";
    indexes.emplace(key.first, index++);
  }
  ss1 << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, change.idDataset());
  SqLite::BinaryDecoder oldPK(change.oldPK().data(), change.oldPK().size());
  for (SqLite::BinaryDecoder::Value &value : oldPK) {
    int id = value.id();
    const auto &key = entity.keys.find(id);
    if (key == entity.keys.end())
      throw SchemaDefinitionException("Attribute " + std::to_string(id) + " not found");
    switch (key->second.type) {
    case SqLite::AttributeType::Integer:
      stmt->setLong(indexes[key->first], value.integerValue());
      break;
    case SqLite::AttributeType::Text:
      stmt->setString(indexes[key->first], value.textValue());
      break;
    case SqLite::AttributeType::Blob:
      stmt->setString(indexes[key->first], value.blobValue());
      break;
    case SqLite::AttributeType::UuidV1: {
      std::string uuid1 = uuid12bin(value.textValue());
      stmt->setString(indexes[key->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      std::string uuidt = uuidt2bin(value.textValue());
      stmt->setString(indexes[key->first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  return stmt->executeUpdate();
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
