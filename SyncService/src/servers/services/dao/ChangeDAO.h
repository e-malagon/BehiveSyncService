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

#ifndef CHANGEDAO_H_
#define CHANGEDAO_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <config/Module.h>
#include <config/Entity.h>
#include <config/Role.h>
#include <entities/Change.h>

#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

class ChangeDAO {
public:
  ChangeDAO(std::shared_ptr<SQL::Connection> connection) :
      _connection(connection) {
  }

  std::shared_ptr<SQL::Connection> connection() const {
    return _connection;
  }

  void save(Entities::Change &change);
  std::vector<Entities::Change> readByHeader(uint32_t idDataset, uint32_t idHeader, const std::unordered_map<std::string, Config::Entity, Utils::IHasher, Utils::IEqualsComparator> &entities, const std::unordered_map<std::string, std::unordered_set<int>> &entitiesByNode);

private:
  std::shared_ptr<SQL::Connection> _connection;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* CHANGEDAO_H_ */
