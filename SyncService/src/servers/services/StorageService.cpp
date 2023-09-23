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

#include "StorageService.h"

#include <ServiceException.h>
#include <exprtk/exprtk.hpp>
#include <dao/SchemaDAO.h>
#include <dao/sql/ConnectionPool.h>

#include <nanolog/NanoLog.hpp>
#include <sqlite/TextDecoder.h>
#include <sqlite/TextEncoder.h>
#include <sqlite/BinaryEncoder.h>
#include <uuid/uuid.h>
#include <unordered_map>

namespace SyncServer {
namespace Servers {
namespace Services {

void StorageService::application(Config::Application *application) {
  _application = application;
  try {
    for (auto &transaction : _application->transactions[_application->version]) {
      if (transaction.second.pre != "")
        _transactionsManager.loadValidation(transaction.second.uuid, transaction.second.pre);
      if (transaction.second.post != "")
        _transactionsManager.loadCommit(transaction.second.uuid, transaction.second.post);
    }

    for (auto &entity : _application->entities[_application->version]) {
      for (auto &attribute : entity.second.attributes) {
        if (attribute.second.check && (attribute.second.type == SqLite::AttributeType::Integer || attribute.second.type == SqLite::AttributeType::Real || attribute.second.type == SqLite::AttributeType::Text)) {
          attribute.second.validator = std::make_shared<Utils::Validator>(attribute.second.name, *attribute.second.check, attribute.second.type);
        }
      }
      if (!_entityDAO.isEntityAvailable(entity.second.id) && !_entityDAO.createEntity(entity.second)) {
        LOG_ERROR << "Can't create entity " << entity.second.name;
      }
    }

    _transactionsManager.application(_application);
    _transactionsManager.entityDAO(&_entityDAO);

  } catch (std::exception &e) {
    LOG_ERROR << e.what();
  }

}

bool StorageService::isMember(Entities::Node &node, uint32_t idDataset) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().id());
  return member && member->status() != 0;
}

StorageService::EntityReader StorageService::readEntityData(Entities::Node &node, uint32_t idDataset, Config::Entity &entity, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode) {
  return EntityReader(idDataset, entity, _entityDAO, entitiesByNode);
}

std::vector<Entities::Header> StorageService::readHeaders(Entities::Node &node, uint32_t idDataset, uint32_t idHeader) {
  return _headerDAO.readFrom(idDataset, idHeader);
}

std::vector<Entities::Change> StorageService::readChanges(Entities::Node &node, uint32_t idDataset, uint32_t idHeader, std::unordered_map<std::string, Config::Entity, Utils::IHasher, Utils::IEqualsComparator> &entities, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode) {
  return _changeDAO.readByHeader(idDataset, idHeader, entities, entitiesByNode);
}

void printValue(SqLite::TextDecoder::Value &value) {
  switch (value.type()) {
  case SqLite::AttributeType::Integer:
    LOG_DEBUG << "Attribute: " << value.column() << " Integer: " << value.integerValue();
    break;
  case SqLite::AttributeType::Real:
    LOG_DEBUG << "Attribute: " << value.column() << " Real: " << value.realValue();
    break;
  case SqLite::AttributeType::Text:
    LOG_DEBUG << "Attribute: " << value.column() << " Text: " << value.textValue();
    break;
  case SqLite::AttributeType::Blob:
    LOG_DEBUG << "Attribute: " << value.column() << " Blob: {}";
    break;
  case SqLite::AttributeType::Null:
    LOG_DEBUG << "Attribute: " << value.column() << " Null ";
    break;
  }
}

StorageService::ValidationCodes StorageService::checkHeaderAndTransform(Entities::Node &node, Entities::Header &header) {
  try {
    if (_application->version < header.version())
      throw SchemaDefinitionException("Client version[" + std::to_string(header.version()) + "] is not valid, Current application version is " + std::to_string(_application->version));
    auto transactionMapPtr = _application->transactionsName2UUID[header.version()].find(header.transactionName());
    if (transactionMapPtr == _application->transactionsName2UUID[header.version()].end()) {
      header.transactionUUID("00000000-0000-0000-0000-000000000000");
      throw SchemaDefinitionException("Transaction not found '" + header.transactionName() + "', the transaction will be rolled back.");
    }
    header.transactionUUID(transactionMapPtr->second);
    for (Entities::Change &change : header.changes()) {
      auto entityMapPtr = _application->entitiesName2UUID[header.version()].find(change.entityName());
      if (entityMapPtr == _application->entitiesName2UUID[header.version()].end()) {
        LOG_FILE_WARN(_beehive) << "Entity '" + change.entityName() + "' not defined, it will be ignored.";
        continue;
      }
      change.entityUUID(entityMapPtr->second);
      auto entityPtr = _application->entities[header.version()].find(change.entityUUID());
      if (entityPtr == _application->entities[header.version()].end()) {
        LOG_FILE_WARN(_beehive) << "Entity '" + change.entityName() + "' not defined, it will be ignored.";
        continue;
      }
      const Config::Entity &entity = entityPtr->second;
      auto transactionPtr = entity.transactions.find(header.transactionUUID());
      if (transactionPtr == entity.transactions.end())
        throw OperationNotAllowedException("Entity '" + entity.name + "' can't be modified on transaction '" + header.transactionName() + "', the transaction will be rolled back.");

      switch (change.operation()) {
      case SqLite::Operation::Insert: {
        if (!transactionPtr->second.add)
          throw OperationNotAllowedException("Transaction '" + transactionPtr->second.name + "' can't insert '" + entity.name + "', the transaction will be rolled back.");
        SqLite::TextDecoder newPK(change.newPK().data(), change.newPK().size(), entity.keysName2Id, entity.name, _beehive);
        SqLite::TextDecoder newData(change.newData().data(), change.newData().size(), entity.attributesName2Id, entity.name, _beehive);
        SqLite::BinaryEncoder newBinaryPK;
        SqLite::BinaryEncoder newBinaryData;
        std::unordered_map<int, bool> usedKeys;
        std::unordered_map<int, bool> usedAttribites;
        LOG_DEBUG << "Insert on " << change.entityName();
        LOG_DEBUG << "New primary key";
        for (SqLite::TextDecoder::Value &value : newPK) {
          printValue(value);
          int id = value.id();
          auto keyPtr = entity.keys.find(id);
          if (keyPtr == entity.keys.end())
            throw SchemaDefinitionException("Key attribute not found '" + entity.name + "." + value.column() + "', the transaction will be rolled back.");
          const Config::Entity::Key &key = keyPtr->second;
          if (value.type() == SqLite::AttributeType::Null)
            throw DataValidationException("Key attribute '" + entity.name + "." + keyPtr->second.name + "' can't be null, the transaction will be rolled back.");
          if ((key.type != value.type() && value.type() != SqLite::AttributeType::Text) || (key.type != SqLite::AttributeType::Text && key.type != SqLite::AttributeType::UuidV1 && key.type != SqLite::AttributeType::UuidV4 && value.type() == SqLite::AttributeType::Text))
            throw DataValidationException("Invalid data type for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          if (key.type == SqLite::AttributeType::UuidV1 || key.type == SqLite::AttributeType::UuidV4) {
            uuid_t uuid;
            if (uuid_parse(value.textValue().c_str(), uuid) != 0)
              throw DataValidationException("Invalid data value for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
            if (key.type == SqLite::AttributeType::UuidV1 && uuid_type(uuid) != UUID_TYPE_DCE_TIME)
              throw DataValidationException("Invalid data value for uuid key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          }
          if (usedKeys.find(key.id) != usedKeys.end())
            throw DataValidationException("Duplicated value for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          usedKeys.emplace(key.id, true);
          newBinaryPK.addValue(value);
        }
        LOG_DEBUG << "New data";
        for (SqLite::TextDecoder::Value &value : newData) {
          printValue(value);
          int id = value.id();
          auto attributePtr = entity.attributes.find(id);
          if (attributePtr == entity.attributes.end()) {
            LOG_FILE_WARN(_beehive) << "Attribute '" + entity.name + "." + attributePtr->second.name + "' not defined, it will be ignored.";
            continue;
          }
          const Config::Entity::Attribute &attribute = attributePtr->second;
          if (attribute.notnull && value.type() == SqLite::AttributeType::Null)
            throw DataValidationException("Attribute '" + entity.name + "." + attributePtr->second.name + "' can't be null, the transaction will be rolled back.");
          if ((attribute.type != value.type() && value.type() != SqLite::AttributeType::Text) || (attribute.type != SqLite::AttributeType::Text && attribute.type != SqLite::AttributeType::UuidV1 && attribute.type != SqLite::AttributeType::UuidV4 && value.type() == SqLite::AttributeType::Text))
            throw DataValidationException("Invalid data type for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
          if (attribute.type == SqLite::AttributeType::UuidV1 || attribute.type == SqLite::AttributeType::UuidV4) {
            uuid_t uuid;
            if (uuid_parse(value.textValue().c_str(), uuid) != 0)
              throw DataValidationException("Invalid data value for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            if (attribute.type == SqLite::AttributeType::UuidV1 && uuid_type(uuid) != UUID_TYPE_DCE_TIME)
              throw DataValidationException("Invalid data value for uuid attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            newBinaryData.addUUID(id, std::string((char*) uuid, 16));
          } else {
            newBinaryData.addValue(value);
          }
          if (usedAttribites.find(attribute.id) != usedAttribites.end())
            throw DataValidationException("Duplicated value for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
          usedAttribites.emplace(attribute.id, true);
          if (attributePtr->second.validator) {
            if (attribute.type == SqLite::AttributeType::Integer) {
              if (!attributePtr->second.validator->isValidValue(value.integerValue()))
                throw DataValidationException("Not valid value[" + std::to_string(value.integerValue()) + "] for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            } else if (attribute.type == SqLite::AttributeType::Real) {
              if (!attributePtr->second.validator->isValidValue(value.realValue()))
                throw DataValidationException("Not valid value[" + std::to_string(value.realValue()) + "] for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            } else if (attribute.type == SqLite::AttributeType::Text) {
              if (!attributePtr->second.validator->isValidValue(value.textValue()))
                throw DataValidationException("Not valid value [" + value.textValue() + "] for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            }
          }
        }
        for (auto &key : entity.keys) {
          if (usedKeys.find(key.second.id) == usedKeys.end())
            throw DataValidationException("Key attribute '" + entity.name + "." + key.second.name + "' missing on insert, the transaction will be rolled back.");
        }
        for (auto &attribute : entity.attributes) {
          if (attribute.second.notnull && usedAttribites.find(attribute.second.id) == usedAttribites.end())
            throw DataValidationException("Not null attribute '" + entity.name + "." + attribute.second.name + "' missing on insert, the transaction will be rolled back.");
        }
        change.newPK(newBinaryPK.encodedData());
        change.newData(newBinaryData.encodedData());
      }
        break;
      case SqLite::Operation::Update: {
        if (transactionPtr->second.update.empty())
          throw OperationNotAllowedException("Transaction '" + transactionPtr->second.name + "' can't update '" + entity.name + "', the transaction will be rolled back.");
        SqLite::TextDecoder newPK(change.newPK().data(), change.newPK().size(), entity.keysName2Id, entity.name, _beehive);
        SqLite::TextDecoder newData(change.newData().data(), change.newData().size(), entity.attributesName2Id, entity.name, _beehive);
        SqLite::TextDecoder oldPK(change.oldPK().data(), change.oldPK().size(), entity.keysName2Id, entity.name, _beehive);
        SqLite::TextDecoder oldData(change.oldData().data(), change.oldData().size(), entity.attributesName2Id, entity.name, _beehive);
        SqLite::BinaryEncoder newBinaryPK;
        SqLite::BinaryEncoder newBinaryData;
        SqLite::BinaryEncoder oldBinaryPK;
        SqLite::BinaryEncoder oldBinaryData;
        LOG_DEBUG << "Update on " << change.entityName();
        LOG_DEBUG << "New primary key";
        for (SqLite::TextDecoder::Value &value : newPK) {
          printValue(value);
          int id = value.id();
          auto keyPtr = entity.keys.find(id);
          if (keyPtr == entity.keys.end())
            throw SchemaDefinitionException("Key attribute not found '" + entity.name + "." + value.column() + "', the transaction will be rolled back.");
          const Config::Entity::Key &key = keyPtr->second;
          if (value.type() == SqLite::AttributeType::Null)
            throw DataValidationException("Key attribute '" + entity.name + "." + keyPtr->second.name + "' can't be null, the transaction will be rolled back.");
          if ((key.type != value.type() && value.type() != SqLite::AttributeType::Text) || (key.type != SqLite::AttributeType::Text && key.type != SqLite::AttributeType::UuidV1 && key.type != SqLite::AttributeType::UuidV4 && value.type() == SqLite::AttributeType::Text))
            throw DataValidationException("Invalid data type for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          if (key.type == SqLite::AttributeType::UuidV1 || key.type == SqLite::AttributeType::UuidV4) {
            uuid_t uuid;
            if (uuid_parse(value.textValue().c_str(), uuid) != 0)
              throw DataValidationException("Invalid data value for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
            if (key.type == SqLite::AttributeType::UuidV1 && uuid_type(uuid) != UUID_TYPE_DCE_TIME)
              throw DataValidationException("Invalid data value for uuid key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          }
          newBinaryPK.addValue(value);
        }
        LOG_DEBUG << "New data";
        for (SqLite::TextDecoder::Value &value : newData) {
          printValue(value);
          int id = value.id();
          auto attributePtr = entity.attributes.find(id);
          if (attributePtr == entity.attributes.end()) {
            LOG_FILE_WARN(_beehive) << "Attribute '" + entity.name + "." + attributePtr->second.name + "' not defined, it will be ignored.";
            continue;
          }
          if (transactionPtr->second.update.find(id) == transactionPtr->second.update.end())
            throw DataValidationException("Transaction '" + transactionPtr->second.name + "' can't update attribute '" + entity.name + "." + value.column() + "', the transaction will be rolled back.");
          const Config::Entity::Attribute &attribute = attributePtr->second;
          if (attribute.notnull && value.type() == SqLite::AttributeType::Null)
            throw DataValidationException("Attribute '" + entity.name + "." + attributePtr->second.name + "' can't be null, the transaction will be rolled back.");
          if ((attribute.type != value.type() && value.type() != SqLite::AttributeType::Text) || (attribute.type != SqLite::AttributeType::Text && attribute.type != SqLite::AttributeType::UuidV1 && attribute.type != SqLite::AttributeType::UuidV4 && value.type() == SqLite::AttributeType::Text))
            throw DataValidationException("Invalid data type for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
          if (attribute.type == SqLite::AttributeType::UuidV1 || attribute.type == SqLite::AttributeType::UuidV4) {
            uuid_t uuid;
            if (uuid_parse(value.textValue().c_str(), uuid) != 0)
              throw DataValidationException("Invalid data value for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            if (attribute.type == SqLite::AttributeType::UuidV1 && uuid_type(uuid) != UUID_TYPE_DCE_TIME)
              throw DataValidationException("Invalid data value for uuid attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            newBinaryData.addUUID(id, std::string((char*) uuid, 16));
          } else {
            newBinaryData.addValue(value);
          }
          if (attributePtr->second.validator) {
            if (attribute.type == SqLite::AttributeType::Integer) {
              if (!attributePtr->second.validator->isValidValue(value.integerValue()))
                throw DataValidationException("Not valid value[" + std::to_string(value.integerValue()) + "] for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            } else if (attribute.type == SqLite::AttributeType::Real) {
              if (!attributePtr->second.validator->isValidValue(value.realValue()))
                throw DataValidationException("Not valid value[" + std::to_string(value.realValue()) + "] for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            } else if (attribute.type == SqLite::AttributeType::Text) {
              if (!attributePtr->second.validator->isValidValue(value.textValue()))
                throw DataValidationException("Not valid value [" + value.textValue() + "] for attribute '" + entity.name + "." + attributePtr->second.name + "', the transaction will be rolled back.");
            }
          }
        }
        LOG_DEBUG << "Old primary key";
        for (SqLite::TextDecoder::Value &value : oldPK) {
          printValue(value);
          int id = value.id();
          auto keyPtr = entity.keys.find(id);
          if (keyPtr == entity.keys.end())
            throw SchemaDefinitionException("Key attribute not found '" + entity.name + "." + value.column() + "', the transaction will be rolled back.");
          const Config::Entity::Key &key = keyPtr->second;
          if (value.type() == SqLite::AttributeType::Null)
            throw DataValidationException("Key attribute '" + entity.name + "." + keyPtr->second.name + "' can't be null, the transaction will be rolled back.");
          if (key.type != value.type() && (value.type() != SqLite::AttributeType::Text && (key.type == SqLite::AttributeType::UuidV1 || key.type == SqLite::AttributeType::UuidV4)))
            throw DataValidationException("Invalid data type for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          if (key.type == SqLite::AttributeType::UuidV1 || key.type == SqLite::AttributeType::UuidV4) {
            uuid_t uuid;
            if (uuid_parse(value.textValue().c_str(), uuid) != 0)
              throw DataValidationException("Invalid data value for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
            if (key.type == SqLite::AttributeType::UuidV1 && uuid_type(uuid) != UUID_TYPE_DCE_TIME)
              throw DataValidationException("Invalid data value for uuid key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          }
          oldBinaryPK.addValue(value);
        }
        LOG_DEBUG << "Old data";
        for (SqLite::TextDecoder::Value &value : oldData) {
          printValue(value);
          oldBinaryData.addValue(value);
        }
        change.newPK(newBinaryPK.encodedData());
        change.newData(newBinaryData.encodedData());
        change.oldPK(oldBinaryPK.encodedData());
        change.oldData(oldBinaryData.encodedData());
      }
        break;
      case SqLite::Operation::Delete: {
        if (!transactionPtr->second.remove)
          throw OperationNotAllowedException("Transaction '" + transactionPtr->second.name + "' can't delete '" + entity.name + "', the transaction will be rolled back.");
        SqLite::TextDecoder oldPK(change.oldPK().data(), change.oldPK().size(), entity.keysName2Id, entity.name, _beehive);
        SqLite::TextDecoder oldData(change.oldData().data(), change.oldData().size(), entity.attributesName2Id, entity.name, _beehive);
        SqLite::BinaryEncoder oldBinaryPK;
        SqLite::BinaryEncoder oldBinaryData;
        LOG_DEBUG << "Delete on " << change.entityName();
        LOG_DEBUG << "Old primary key";
        for (SqLite::TextDecoder::Value &value : oldPK) {
          printValue(value);
          int id = value.id();
          auto keyPtr = entity.keys.find(id);
          if (keyPtr == entity.keys.end())
            throw SchemaDefinitionException("Key attribute not found '" + entity.name + "." + value.column() + "', the transaction will be rolled back.");
          const Config::Entity::Key &key = keyPtr->second;
          if (value.type() == SqLite::AttributeType::Null)
            throw DataValidationException("Key attribute '" + entity.name + "." + keyPtr->second.name + "' can't be null, the transaction will be rolled back.");
          if (key.type != value.type() && (value.type() != SqLite::AttributeType::Text && (key.type == SqLite::AttributeType::UuidV1 || key.type == SqLite::AttributeType::UuidV4)))
            throw DataValidationException("Invalid data type for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          if (key.type == SqLite::AttributeType::UuidV1 || key.type == SqLite::AttributeType::UuidV4) {
            uuid_t uuid;
            if (uuid_parse(value.textValue().c_str(), uuid) != 0)
              throw DataValidationException("Invalid data value for key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
            if (key.type == SqLite::AttributeType::UuidV1 && uuid_type(uuid) != UUID_TYPE_DCE_TIME)
              throw DataValidationException("Invalid data value for uuid key attribute '" + entity.name + "." + keyPtr->second.name + "', the transaction will be rolled back.");
          }
          oldBinaryPK.addValue(value);
        }
        LOG_DEBUG << "Old data";
        for (SqLite::TextDecoder::Value &value : oldData) {
          printValue(value);
          oldBinaryData.addValue(value);
        }
        change.oldPK(oldBinaryPK.encodedData());
        change.oldData(oldBinaryData.encodedData());
      }
        break;
      }
    }
  } catch (Services::DataValidationException &e) {
    LOG_FILE_WARN(_beehive) << e.what();
    return ValidationCodes::notValidIncomeData;
  } catch (Services::SchemaDefinitionException &e) {
    LOG_FILE_WARN(_beehive) << e.what();
    return ValidationCodes::entityDefinition;
  } catch (Services::OperationNotAllowedException &e) {
    LOG_FILE_ERROR(_beehive) << e.what();
    return ValidationCodes::notValidOperation;
  } catch (Services::NotExistException &e) {
    LOG_FILE_ERROR(_beehive) << e.what();
    return ValidationCodes::entityNotFound;
  }
  return ValidationCodes::success;
}

StorageService::ValidationCodes StorageService::applyChange(Entities::Node &node, Entities::Header &header, Entities::Change &change) {
  try {
    auto entityMapPtr = _application->entitiesName2UUID[header.version()].find(change.entityName());
    if (entityMapPtr == _application->entitiesName2UUID[header.version()].end()) {
      return ValidationCodes::skipEntity;
    }
    auto entityPtr = _application->entities[header.version()].find(entityMapPtr->second);
    if (entityPtr == _application->entities[header.version()].end()) {
      return ValidationCodes::skipEntity;
    }
    const Config::Entity &entity = entityPtr->second;
    switch (change.operation()) {
    case SqLite::Operation::Insert: {
      try {
        _entityDAO.save(change, entity);
      } catch (DAO::SQL::SQLException &ex) {
        switch (ex.errorCode()) {
        case 1063:
          return ValidationCodes::duplicatedEntity;
        default:
          throw ex;
        }
      }
    }
      break;
    case SqLite::Operation::Update: {
      Entities::KeyData keyData = _entityDAO.read(change, entity);
      SqLite::BinaryDecoder oldDecoder(keyData.oldData().data(), keyData.oldData().size());
      SqLite::BinaryDecoder newDecoder(change.newData().data(), change.newData().size());
      SqLite::BinaryEncoder encoder;
      for (SqLite::BinaryDecoder::Value &value : oldDecoder)
        encoder.addValue(value);
      for (SqLite::BinaryDecoder::Value &value : newDecoder)
        encoder.addValue(value);
      change.newData(encoder.encodedData());
      if (_entityDAO.update(change, entity) != 1)
        return ValidationCodes::entityNotFound;
    }
      break;
    case SqLite::Operation::Delete: {
      if (_entityDAO.remove(change, entity) != 1)
        return ValidationCodes::entityNotFound;
    }
      break;
    }
    return ValidationCodes::success;
  } catch (SchemaDefinitionException &e) {
    LOG_FILE_ERROR(_beehive) << e.what();
    return ValidationCodes::entityDefinition;
  }
}

std::pair<uint32_t, uint32_t> StorageService::readLastSynchronizedId(Entities::Node &node, uint32_t idDataset) {
  return _downloadedDAO.read(node.id(), idDataset);
}

void StorageService::updateLastSynchronizedId(Entities::Node &node, uint32_t idDataset, uint32_t idHeader, uint32_t idCell) {
  _downloadedDAO.save(node.id(), idDataset, idHeader, idCell);
  _connection->commit();
}

void StorageService::saveHeader(Entities::Node &node, Entities::Header &header, uint32_t idHeader) {
  _connection->lock(std::to_string(header.idDataset()));
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(header.idDataset());
  if (!dataset)
    throw ServiceException("Data set doesn't exist", 435);

  dataset->idHeader(dataset->idHeader() + 1);
  header.idHeader(dataset->idHeader());
  header.status(checkHeaderAndTransform(node, header));
  _headerDAO.save(header);
  if (header.status() == ValidationCodes::success) {
    if (_transactionsManager.executeValidation(header)) {
      for (Entities::Change &change : header.changes()) {
        change.idDataset(dataset->id());
        change.idHeader(header.idHeader());
        header.status(applyChange(node, header, change));
        if (header.status() == ValidationCodes::success) {
          _changeDAO.save(change);
        } else if (header.status() != ValidationCodes::skipEntity) {
          _connection->rollback();
          _headerDAO.save(header);
          break;
        } else {
          header.status(ValidationCodes::success);
        }
      }
    } else {
      header.status(ValidationCodes::userValidation);
    }
  }
  if (!_transactionsManager.executeCommit(header)) {
    if (header.status() == ValidationCodes::success)
      header.status(ValidationCodes::userValidation);
    _connection->rollback();
    _headerDAO.save(header);
  }
  _downloadedDAO.save(node.id(), header.idDataset(), idHeader, header.idNode());
  _datasetDAO.update(*dataset);
  _connection->commit();
  _connection->unlock(std::to_string(header.idDataset()));
  if (header.status() == ValidationCodes::success && header.version() != _application->version) {

  }
}

std::unordered_map<std::string, std::unordered_set<int>> StorageService::entitiesByNode(Entities::Node &node, uint32_t idDataset) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + member->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  auto modulePtr = _application->modules[_application->version].find(node.module());
  if (modulePtr == _application->modules[_application->version].end()) {
    std::string error = "Module '" + node.module() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  std::unordered_map<std::string, std::unordered_set<int>> entitiesByNode;
  for (auto &entity : _application->entities[_application->version]) {
    auto entityRolePtr = rolePtr->second.entitiesMap.find(entity.second.uuid);
    auto entityModulePtr = modulePtr->second.entitiesMap.find(entity.second.uuid);
    if (entityRolePtr != rolePtr->second.entitiesMap.end() && entityModulePtr != modulePtr->second.entitiesMap.end()) {
      std::unordered_set<int> attributesByEntity;
      for (auto &attribute : entity.second.attributes) {
        auto attributeRolePtr = entityRolePtr->second.find(attribute.second.id);
        auto attributeModulePtr = entityModulePtr->second.find(attribute.second.id);
        if (attributeRolePtr != entityRolePtr->second.end() && attributeModulePtr != entityModulePtr->second.end())
          attributesByEntity.emplace(attribute.second.id);
      }
      entitiesByNode.emplace(entity.second.uuid, attributesByEntity);
    }
  }
  return entitiesByNode;
}

std::unordered_map<std::string, Config::Entity, Utils::IHasher, Utils::IEqualsComparator>& StorageService::entities(uint16_t clientVersion) {
  return _application->entities[clientVersion];
}

StorageService::EntityReader::iterator& StorageService::EntityReader::iterator::operator++() {
  if (!_resultSet->next()) {
    delete _resultSet;
    _resultSet = nullptr;
  }
  return *this;
}

StorageService::EntityReader::iterator StorageService::EntityReader::iterator::operator++(int) {
  iterator tmp(*this);
  operator++();
  return tmp;
}

bool StorageService::EntityReader::iterator::operator==(const iterator &rhs) const {
  return _resultSet == rhs._resultSet;
}

bool StorageService::EntityReader::iterator::operator!=(const iterator &rhs) const {
  return _resultSet != rhs._resultSet;
}

Entities::Change& StorageService::EntityReader::iterator::operator*() {
  if (_resultSet) {
    std::string opPK = _resultSet->getString("key");
    std::string opData = _resultSet->getString("data");
    SqLite::BinaryDecoder newPK(opPK.c_str(), opPK.size());
    SqLite::BinaryDecoder newData(opData.c_str(), opData.size());
    SqLite::TextEncoder newTextPK(_entity.keysId2Name);
    SqLite::TextEncoder newTextData(_entity.attributesId2Name);
    for (SqLite::BinaryDecoder::Value &value : newPK) {
      newTextPK.addValue(value);
    }
    auto roleEntityPtr = _entitiesByNode.find(_entity.uuid);
    if (roleEntityPtr != _entitiesByNode.end()) {
      for (SqLite::BinaryDecoder::Value &value : newData) {
        if (roleEntityPtr->second.find(value.id()) != roleEntityPtr->second.end())
          newTextData.addValue(value);
      }
    }
    _change.idDataset(_idDataset);
    _change.idHeader(0);
    _change.idChange(_idChange++);
    _change.operation(SqLite::Operation::Insert);
    _change.entityName(_entity.name);
    _change.newPK(newTextPK.encodedData());
    _change.newData(newTextData.encodedData());
    _change.oldPK("");
    _change.oldData("");
  } else {
    _change.idDataset(_idDataset);
    _change.idHeader(0);
    _change.idChange(_idChange++);
    _change.operation(SqLite::Operation::Insert);
    _change.entityName(_entity.name);
    _change.newPK("");
    _change.newData("");
    _change.oldPK("");
    _change.oldData("");
  }
  return _change;
}

StorageService::EntityReader::iterator StorageService::EntityReader::begin() {
  return iterator(_entityDAO.read(_idDataset, _entity), _idDataset, _entity, _entitiesByNode);
}

StorageService::EntityReader::iterator StorageService::EntityReader::end() {
  return iterator(nullptr, _idDataset, _entity, _entitiesByNode);
}

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
