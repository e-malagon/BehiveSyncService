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

#ifndef CONNECTIONPOOL_H_
#define CONNECTIONPOOL_H_

#include "Connection.h"
#include "SQLException.h"

#include <set>
#include <deque>
#include <mutex>
#include <memory>
#include <string>
#include <exception>
#include <unordered_map>
#include <nanolog/NanoLog.hpp>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

class ConnectionPool {
public:

  ConnectionPool(size_t pool_size, const std::string &hostName, int port, const std::string &user, const std::string &password, const bool secure) :
      _size(pool_size), _hostName(hostName), _port(port), _user(user), _password(password), _secure(secure) {
  }

  ~ConnectionPool() {
    _pools.clear();
    mysql_library_end();
  }

  class ConnectionPtr {
    friend class ConnectionPool;
  private:
    ConnectionPtr(ConnectionPool *connectionPool, uint32_t database, std::shared_ptr<Connection> connection) :
        _connectionPool(connectionPool), _database(database), _connection(connection) {
    }

  public:
    ~ConnectionPtr() {
      if (_connection.use_count() == 1) {
        std::lock_guard<std::mutex> lock(_connectionPool->_mutex);
        auto poolPtr = _connectionPool->_pools.find(_database);
        if (poolPtr != _connectionPool->_pools.end())
          poolPtr->second.push_back(std::static_pointer_cast<Connection>(_connection));
      }
    }

    std::shared_ptr<Connection> connection() {
      return _connection;
    }

  private:
    ConnectionPool *_connectionPool;
    uint32_t _database;
    std::shared_ptr<Connection> _connection;
  };

  std::shared_ptr<Connection> getUnamedConnection() {
    return std::shared_ptr<Connection>(new Connection(_hostName, _port, _user, _password, 0, _secure));
  }

  ConnectionPtr getNamedConnection(uint32_t beehive) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_pools.find(beehive) == _pools.end()) {
      _pools.emplace(beehive, std::deque<std::shared_ptr<Connection>>());
    }
    auto pool = _pools.find(beehive);
    if (pool->second.size() == 0 && Connection::connections(beehive) < _size) {
      std::shared_ptr<Connection> conn(new Connection(_hostName, _port, _user, _password, beehive, _secure));
      ConnectionPtr connectionPtr(this, beehive, conn);
      return connectionPtr;
    } else if (0 < pool->second.size()) {
      std::shared_ptr<Connection> conn = pool->second.front();
      pool->second.pop_front();
      ConnectionPtr connectionPtr(this, beehive, conn);
      try {
        if (conn->ping()) {
          return connectionPtr;
        } else {
          std::shared_ptr<Connection> conn(new Connection(_hostName, _port, _user, _password, beehive, _secure));
          ConnectionPtr connectionPtr(this, beehive, conn);
          return connectionPtr;
        }
      } catch (SQLException &e) {
        std::shared_ptr<Connection> conn(new Connection(_hostName, _port, _user, _password, beehive, _secure));
        ConnectionPtr connectionPtr(this, beehive, conn);
        return connectionPtr;
      }
    } else {
      throw ConnectionUnavailable("Connection unavailable");
    }
  }

protected:
  size_t _size;
  std::unordered_map<uint32_t, std::deque<std::shared_ptr<Connection>>> _pools;
  std::mutex _mutex;
  std::string _hostName;
  int _port;
  std::string _user;
  std::string _password;
  bool _secure;
};

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* CONNECTIONPOOL_H_ */
