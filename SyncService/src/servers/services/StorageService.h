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

#ifndef STORAGESERVICE_H_
#define STORAGESERVICE_H_

#include <json/json.hpp>
#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <dao/DatasetDAO.h>
#include <dao/DownloadedDAO.h>
#include <dao/MemberDAO.h>
#include <dao/HeaderDAO.h>
#include <dao/ChangeDAO.h>
#include <dao/NodeDAO.h>
#include <dao/EntityDAO.h>
#include <dao/SchemaDAO.h>
#include <config/Application.h>
#include <config/Module.h>
#include <entities/Dataset.h>
#include <entities/User.h>
#include <entities/Node.h>
#include <entities/Change.h>
#include <validation/TransactionsManager.h>
#include <SchemaService.h>

#include <list>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

namespace SyncServer {
namespace Servers {
namespace Services {

class StorageService {
public:
  StorageService(std::shared_ptr<DAO::SQL::Connection> connection, uint32_t beehive) :
      _connection(connection), _datasetDAO(connection), _downloadedDAO(connection), _memberDAO(connection), _headerDAO(connection), _changeDAO(connection), _nodeDAO(connection), _entityDAO(connection, beehive), _transactionsManager(beehive), _application(nullptr), _beehive(beehive) {
  }

  virtual ~StorageService() {
  }

  enum ValidationCodes {
    success = 0, //
    approved = 1, //
    skipEntity = 9, //
    entityNotFound = 10, //
    duplicatedEntity = 110, //
    notValidIncomeData = 120, //
    notValidOperation = 130, //
    entityDefinition = 140, //
    userValidation = 150
  };

  class EntityReader {
  public:
    EntityReader(uint32_t idDataset, Config::Entity &entity, DAO::EntityDAO &entityDAO, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode) :
        _idDataset(idDataset), _entity(entity), _entityDAO(entityDAO), _entitiesByNode(entitiesByNode) {
    }
    virtual ~EntityReader() {
    }

    class iterator: public std::iterator<std::input_iterator_tag, Entities::Change> {
      Entities::Change _change;
      DAO::SQL::ResultSet *_resultSet;
      uint32_t _idDataset;
      const Config::Entity &_entity;
      int _idChange;
      const std::unordered_map<std::string, std::unordered_set<int>> &_entitiesByNode;
    public:
      iterator(DAO::SQL::ResultSet *res, uint32_t idDataset, const Config::Entity &entity, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode) :
          _resultSet(res), _idDataset(idDataset), _entity(entity), _idChange(0), _entitiesByNode(entitiesByNode) {
        if (_resultSet && !_resultSet->next()) {
          delete _resultSet;
          _resultSet = nullptr;
        }
      }
      iterator(const iterator &mit) :
          _resultSet(mit._resultSet), _idDataset(mit._idDataset), _entity(mit._entity), _idChange(mit._idChange), _entitiesByNode(mit._entitiesByNode) {
      }
      ~iterator() {
        if (_resultSet) {
          delete _resultSet;
          _resultSet = nullptr;
        }
      }
      iterator& operator++();
      iterator operator++(int);
      bool operator==(const iterator &rhs) const;
      bool operator!=(const iterator &rhs) const;
      Entities::Change& operator*();
    };

    iterator begin();
    iterator end();
  private:
    uint32_t _idDataset;
    Config::Entity &_entity;
    DAO::EntityDAO &_entityDAO;
    const std::unordered_map<std::string, std::unordered_set<int>> &_entitiesByNode;
  };

  ValidationCodes checkHeaderAndTransform(Entities::Node &node, Entities::Header &header);
  ValidationCodes applyChange(Entities::Node &node, Entities::Header &header, Entities::Change &change);

  bool isMember(Entities::Node &node, uint32_t idDataset);
  EntityReader readEntityData(Entities::Node &node, uint32_t idDataset, Config::Entity &entity, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode);
  std::vector<Entities::Header> readHeaders(Entities::Node &node, uint32_t idDataset, uint32_t idHeader);
  std::vector<Entities::Change> readChanges(Entities::Node &node, uint32_t idDataset, uint32_t idHeader, std::unordered_map<std::string, Config::Entity, Utils::IHasher, Utils::IEqualsComparator> &entities, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode);
  std::pair<uint32_t, uint32_t> readLastSynchronizedId(Entities::Node &node, uint32_t idDataset);
  void updateLastSynchronizedId(Entities::Node &node, uint32_t idDataset, uint32_t idHeader, uint32_t idCell);
  void saveHeader(Entities::Node &node, Entities::Header &header, uint32_t idHeader);
  std::unordered_map<std::string, std::unordered_set<int>> entitiesByNode(Entities::Node &node, uint32_t idDataset);
  std::unordered_map<std::string, Config::Entity, Utils::IHasher, Utils::IEqualsComparator>& entities(uint16_t clientVersion);

  void application(Config::Application *application);
private:
  std::shared_ptr<DAO::SQL::Connection> _connection;
  DAO::DatasetDAO _datasetDAO;
  DAO::DownloadedDAO _downloadedDAO;
  DAO::MemberDAO _memberDAO;
  DAO::HeaderDAO _headerDAO;
  DAO::ChangeDAO _changeDAO;
  DAO::NodeDAO _nodeDAO;
  DAO::EntityDAO _entityDAO;
  Utils::TransactionsManager _transactionsManager;
  Config::Application *_application;
  uint32_t _beehive;
};

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* STORAGESERVICE_H_ */
