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

#ifndef DOWNLOADEDDAO_H_
#define DOWNLOADEDDAO_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>

#include <memory>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

class DownloadedDAO {
public:
  DownloadedDAO(std::shared_ptr<SQL::Connection> connection) :
      _connection(connection) {
  }
  void save(uint32_t idNode, uint32_t idDataset, uint32_t idHeader, uint32_t idCell);
  std::pair<uint32_t, uint32_t> read(uint32_t _idNode, uint32_t idDataset);

private:
  std::shared_ptr<SQL::Connection> _connection;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* MEMBERDAO_H_ */
