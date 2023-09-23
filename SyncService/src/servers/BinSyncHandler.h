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

#ifndef BINSYNCHANDLER_H_
#define BINSYNCHANDLER_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <dao/sql/ConnectionPool.h>
#include <UserService.h>
#include <DatasetService.h>
#include <StorageService.h>
#include <tcp/TCPHandler.h>

namespace SyncServer {
namespace Servers {

class BinSyncHandler {
public:
  BinSyncHandler(Services::DAO::SQL::ConnectionPool &connectionPool) :
      _connectionPool(connectionPool) {
  }

  virtual ~BinSyncHandler() {
  }

  void run(std::string &instance);
private:
  Services::DAO::SQL::ConnectionPool &_connectionPool;
};

class BinSyncHandlerIntance: public TCP::TCPHandler {
public:
  BinSyncHandlerIntance(int socket, std::shared_ptr<Services::DAO::SQL::Connection> connection, uint32_t beehive) :
      TCPHandler(socket), _connection(connection), _userService(connection), _datasetService(connection, beehive), _storageService(connection, beehive), _beehive(beehive) {
    _buffer = new uint8_t[32000];
  }

  virtual ~BinSyncHandlerIntance() {
    delete _buffer;
  }
  void run(void);
private:

  void deleteDataset(Services::Entities::Node &node);
  void pushDataset(Services::Entities::Node &node);
  void popDataset(Services::Entities::Node &node);
  void pullDataset(Services::Entities::Node &node);
  void putDataset(Services::Entities::Node &node);
  void leaveDataset(Services::Entities::Node &node);
  void updateMember(Services::Entities::Node &node);
  void deleteMember(Services::Entities::Node &node);
  void fullSync(Services::Entities::Node &node);

  enum Codes {
    success = 0, //
    messageTransmissionError = 1, //
    newContainerAvailable = 40, //
    newGroupAvailable = 50, //
    newElementAvailable = 51, //
    dataNotFound = 99, //
    userNotFound = 100, //
    notEnoughRights = 110, //
    invalidSchema = 120, //
    internalError = 255 //
  };

  std::shared_ptr<Services::DAO::SQL::Connection> _connection;
  Services::UserService _userService;
  Services::DatasetService _datasetService;
  Services::StorageService _storageService;
  uint32_t _beehive;
  uint8_t *_buffer;
};

} /* namespace Servers */
} /* namespace SyncServer */

#endif /* BINSYNCHANDLER_H_ */
