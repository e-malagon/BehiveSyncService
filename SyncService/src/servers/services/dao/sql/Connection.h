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

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "Statement.h"

#include <mysql.h>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

class Connection {
public:
  Connection(const std::string &hostName, int port, const std::string &user, const std::string &password, uint32_t beehive, const bool secure);
  virtual ~Connection();
  void begin() const;
  void commit() const;
  void rollback() const;
  void switchBeehive(const std::string &beehive);
  Statement* prepareStatement(const std::string &statement, bool cached = true, bool asStatic = false);
  void lock(std::string resource);
  void unlock(std::string resource);
  void unlock();
  bool ping();
  static size_t connections(uint32_t dataBase) {
    auto nPtr = _numberConnections.find(dataBase);
    if (nPtr == _numberConnections.end())
      return 0;
    else
      return nPtr->second;
  }
private:
  static std::unordered_map<uint32_t, size_t> _numberConnections;
  MYSQL *_MySQLConnection;
  std::unordered_map<std::string, Statement*> _statements;
  uint32_t _beehive;
};

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* CONNECTION_H_ */
