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


#include <dao/EntityDAO.hpp>
#include <dao/Storage.hpp>

#include <sqlite/BinaryDecoder.hpp>
#include <sqlite/BinaryEncoder.hpp>

#include <nanolog/NanoLog.hpp>
#include <sstream>

#include <services/ServiceException.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string EntityDAO::prefix("E.");

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

void* EntityDAO::read(uint32_t idDataset, const Config::Entity &entity, const std::string &context) {
  return 0;
}

std::vector<Entities::KeyData> EntityDAO::read(uint32_t idDataset, const sol::table &keys, const Config::Entity &entity, const std::string &context) {
  std::vector<Entities::KeyData> keyDataV;
  /*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  std::stringstream ss1;
  std::unordered_map<int, int> indexes;
  ss1 << "SELECT `key`,`data` FROM `" + entity.id + "` WHERE `iddataset` = ?";
  int index = 2;
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>();
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr != entity.keysName2Id.end()) {
      ss1 << " AND `k" + std::to_string(keyMappingsPtr->second) + "` = ?";
      indexes.emplace(keyMappingsPtr->second, index++);
    } else {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return keyDataV;
    }
  }
  ss1 << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, idDataset);
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>();
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr == entity.keysName2Id.end()) {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return keyDataV;
    }
    const auto &keyPtr = entity.keys.find(keyMappingsPtr->second);
    if (keyPtr == entity.keys.end()) {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return keyDataV;
    }
    switch (keyPtr->second.type) {
    case SqLite::AttributeType::Integer: {
      if (key.second.get_type() != sol::type::number) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      uint64_t iValue = key.second.as<uint64_t>();
      stmt->setLong(indexes[keyMappingsPtr->second], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>();
      stmt->setString(indexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>();
      stmt->setString(indexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>();
      std::string uuid1 = uuid12bin(sValue);
      stmt->setString(indexes[keyMappingsPtr->second], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return keyDataV;
      }
      std::string sValue = key.second.as<std::string>();
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
  */
  return keyDataV;
}

Entities::KeyData EntityDAO::read(Entities::Change &change, const Config::Entity &entity, const std::string &context) {
    Entities::KeyData keyData;
/*
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
  res = stmt->executeQuery();
  if (res->next()) {
    keyData.oldPK(res->getString("key"));
    keyData.oldData(res->getString("data"));
  }
  delete res;
  */
  return keyData;
}

int EntityDAO::save(uint32_t idDataset, const sol::table &data, const Config::Entity &entity, const std::string &context) {
  /*
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
      LOG_ERROR << "Missing key " << entity.name << "." << key.second.name;
      return 0;
    }
    index++;
    switch (key.second.type) {
    case SqLite::AttributeType::Integer: {
      if (keyValue.get_type() != sol::type::number) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      uint64_t iValue = keyValue.as<uint64_t>();
      newPK.addInteger(key.first, iValue);
      stmt->setLong(indexes[key.first], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>();
      newPK.addText(key.first, sValue);
      stmt->setString(indexes[key.first], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>();
      newPK.addBlob(key.first, sValue);
      stmt->setString(indexes[key.first], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>();
      newPK.addText(key.first, sValue);
      std::string uuid1 = uuid12bin(sValue);
      stmt->setString(indexes[key.first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (keyValue.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << key.second.name;
        return 0;
      }
      std::string sValue = keyValue.as<std::string>();
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
      LOG_ERROR << "Missing attribute " << entity.name << "." << col.second.name;
      return 0;
    }
    switch (col.second.type) {
    case SqLite::AttributeType::Integer: {
      if (colValue.get_type() != sol::type::number) {
        LOG_ERROR << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      long iValue = colValue.as<long>();
      newData.addInteger(col.first, (uint64_t) iValue);
    }
      break;
    case SqLite::AttributeType::Real: {
      if (colValue.get_type() != sol::type::number) {
        LOG_ERROR << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      double dValue = colValue.as<double>();
      newData.addReal(col.first, dValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (colValue.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      std::string sValue = colValue.as<std::string>();
      newData.addText(col.first, sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (colValue.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong attribute type " << entity.name << "." << col.second.name;
        return 0;
      }
      std::string sValue = colValue.as<std::string>();
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
  */
 return 0;
}

void EntityDAO::save(Entities::Change &change, const Config::Entity &entity, const std::string &context) {
  /*
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
  */
}

int EntityDAO::update(uint32_t idDataset, const sol::table &keys, const sol::table &data, const Config::Entity &entity, const std::string &context) {
  /*
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
    std::string name = key.first.as<std::string>();
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr != entity.keysName2Id.end()) {
      ss1 << " AND `k" + std::to_string(keyMappingsPtr->second) + "` = ?";
      selectIndexes.emplace(keyMappingsPtr->second, index++);
    } else {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return 0;
    }
  }
  ss1 << ";";
  selectStmt = _connection->prepareStatement(ss1.str());
  selectStmt->setInt(1, idDataset);
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>();
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr == entity.keysName2Id.end()) {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    const auto &keyPtr = entity.keys.find(keyMappingsPtr->second);
    if (keyPtr == entity.keys.end()) {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    switch (keyPtr->second.type) {
    case SqLite::AttributeType::Integer: {
      if (key.second.get_type() != sol::type::number) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      uint64_t iValue = key.second.as<uint64_t>();
      selectStmt->setLong(selectIndexes[keyMappingsPtr->second], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      std::string uuid1 = uuid12bin(sValue);
      selectStmt->setString(selectIndexes[keyMappingsPtr->second], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
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
      std::string name = dat.first.as<std::string>();
      auto attributesMappingsPtr = entity.attributesName2Id.find(name);
      if (attributesMappingsPtr == entity.attributesName2Id.end()) {
        LOG_ERROR << "Key not fount " << entity.name << "." << name;
        return 0;
      }
      const auto &colPtr = entity.attributes.find(attributesMappingsPtr->second);
      if (colPtr == entity.attributes.end()) {
        LOG_ERROR << "Attribute not fount " << entity.name << "." << name;
        return 0;
      }
      if (dat.second.get_type() == sol::type::nil) {
        encoder.addNull(attributesMappingsPtr->second);
      } else {
        switch (colPtr->second.type) {
        case SqLite::AttributeType::Integer: {
          if (dat.second.get_type() != sol::type::number) {
            LOG_ERROR << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          uint64_t iValue = dat.second.as<uint64_t>();
          encoder.addInteger(attributesMappingsPtr->second, iValue);
        }
          break;
        case SqLite::AttributeType::Real: {
          if (dat.second.get_type() != sol::type::number) {
            LOG_ERROR << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          double dValue = dat.second.as<double>();
          encoder.addInteger(attributesMappingsPtr->second, dValue);
        }
          break;
        case SqLite::AttributeType::Text: {
          if (dat.second.get_type() != sol::type::string) {
            LOG_ERROR << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          std::string sValue = dat.second.as<std::string>();
          encoder.addText(attributesMappingsPtr->second, sValue);
        }
          break;
        case SqLite::AttributeType::Blob: {
          if (dat.second.get_type() != sol::type::string) {
            LOG_ERROR << "Wrong attribute type " << entity.name << "." << name;
            return 0;
          }
          std::string sValue = dat.second.as<std::string>();
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
        LOG_ERROR << "Key not fount " << entity.name << "." << key.id();
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
  */
 return 0;
}

int EntityDAO::update(Entities::Change &change, const Config::Entity &entity, const std::string &context) {
  /*
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
  */
  return 0;
}

int EntityDAO::remove(uint32_t idDataset, const sol::table &keys, const Config::Entity &entity, const std::string &context) {
  /*
  SQL::Statement *stmt;
  std::stringstream ss1;
  std::unordered_map<int, int> indexes;
  ss1 << "DELETE FROM `" + entity.id + "` WHERE `iddataset` = ?";

  int index = 2;
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>();
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr != entity.keysName2Id.end()) {
      ss1 << " AND `k" + std::to_string(keyMappingsPtr->second) + "` = ?";
      indexes.emplace(keyMappingsPtr->second, index++);
    } else {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return 0;
    }
  }
  ss1 << ";";
  stmt = _connection->prepareStatement(ss1.str());
  stmt->setInt(1, idDataset);
  for (auto &key : keys) {
    std::string name = key.first.as<std::string>();
    auto keyMappingsPtr = entity.keysName2Id.find(name);
    if (keyMappingsPtr == entity.keysName2Id.end()) {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    const auto &keyPtr = entity.keys.find(keyMappingsPtr->second);
    if (keyPtr == entity.keys.end()) {
      LOG_ERROR << "Key not fount " << entity.name << "." << name;
      return 0;
    }
    switch (keyPtr->second.type) {
    case SqLite::AttributeType::Integer: {
      if (key.second.get_type() != sol::type::number) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      uint64_t iValue = key.second.as<uint64_t>();
      stmt->setLong(indexes[keyPtr->first], iValue);
    }
      break;
    case SqLite::AttributeType::Text: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      stmt->setString(indexes[keyPtr->first], sValue);
    }
      break;
    case SqLite::AttributeType::Blob: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      stmt->setString(indexes[keyPtr->first], sValue);
    }
      break;
    case SqLite::AttributeType::UuidV1: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      std::string uuid1 = uuid12bin(sValue);
      stmt->setString(indexes[keyPtr->first], uuid1);
    }
      break;
    case SqLite::AttributeType::UuidV4: {
      if (key.second.get_type() != sol::type::string) {
        LOG_ERROR << "Wrong key type " << entity.name << "." << name;
        return 0;
      }
      std::string sValue = key.second.as<std::string>();
      std::string uuidt = uuidt2bin(sValue);
      stmt->setString(indexes[keyPtr->first], uuidt);
    }
      break;
    default:
      break;
    }
  }
  return stmt->executeUpdate();
  */
 return 0;
}

int EntityDAO::remove(Entities::Change &change, const Config::Entity &entity, const std::string &context) {
  /*
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
  */
 return 0;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
