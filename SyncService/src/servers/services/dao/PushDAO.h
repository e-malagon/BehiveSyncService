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

#ifndef PUSHDAO_H_
#define PUSHDAO_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <entities/Push.h>

#include <vector>
#include <memory>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

class PushDAO {
public:
  PushDAO(std::shared_ptr<SQL::Connection> connection) :
      _connection(connection) {
  }
  void save(Entities::Push &push);
  std::unique_ptr<Entities::Push> read(uint32_t idDataset, std::string &uuid);
  std::vector<Entities::Push> readByDataset(uint32_t idDataset);
  int update(Entities::Push &push);
  int remove(uint32_t idDataset, std::string &uuid);

private:
  std::shared_ptr<SQL::Connection> _connection;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* PUSHDAO_H_ */
