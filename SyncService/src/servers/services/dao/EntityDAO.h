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

#ifndef ENTITYDAO_H_
#define ENTITYDAO_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <dao/sql/ResultSet.h>
#include <config/Entity.h>
#include <entities/Change.h>
#include <entities/KeyData.h>
#include <sol/sol.hpp>

#include <vector>
#include <memory>
#include <unordered_map>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

class EntityDAO {
public:
  EntityDAO(std::shared_ptr<SQL::Connection> connection, uint32_t beehive) :
      _beehive(beehive), _connection(connection) {
  }

  std::shared_ptr<SQL::Connection> connection() const {
    return _connection;
  }

  std::string uuid12bin(std::string uuid);
  std::string uuidt2bin(std::string uuid);
  std::string bin2uuid1(std::string uuid);
  std::string bin2uuidt(std::string uuid);
  bool isEntityAvailable(const std::string &name);
  bool createEntity(const Config::Entity &entity);
  bool dropEntity(const std::string &entity);
  SQL::ResultSet* read(uint32_t idDataset, const Config::Entity &entity);
  std::vector<Entities::KeyData> read(uint32_t idDataset, const sol::table &data, const Config::Entity &entity);
  Entities::KeyData read(Entities::Change &change, const Config::Entity &entity);
  int save(uint32_t idDataset, const sol::table &data, const Config::Entity &entity);
  void save(Entities::Change &change, const Config::Entity &entity);
  int update(uint32_t idDataset, const sol::table &key, const sol::table &data, const Config::Entity &entity);
  int update(Entities::Change &change, const Config::Entity &entity);
  int remove(uint32_t idDataset, const sol::table &key, const Config::Entity &entity);
  int remove(Entities::Change &change, const Config::Entity &entity);
private:
  uint32_t _beehive;
  std::shared_ptr<SQL::Connection> _connection;
  std::shared_ptr<SQL::Statement> _statement;
  std::unordered_map<std::string, int> _indexes;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* ENTITYDAO_H_ */
