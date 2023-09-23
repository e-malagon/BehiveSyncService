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


#pragma once

#include <entities/Node.hpp>
#include <services/DatasetService.hpp>
#include <services/StorageService.hpp>
#include <services/UserService.hpp>
#include <tcp/TCPHandler.hpp>

#include <string>

namespace Beehive {
namespace Services {

class InboundTCP {
   public:
    InboundTCP() {
    }

    virtual ~InboundTCP() {
    }

    void start();
    void finish();

   private:
   int _tcpSocket;
};

class BinSyncHandlerIntance : public TCP::TCPHandler {
   public:
    BinSyncHandlerIntance(int socket) : TCPHandler(socket) {
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
        success = 0,                   //
        messageTransmissionError = 1,  //
        newContainerAvailable = 40,    //
        newGroupAvailable = 50,        //
        newElementAvailable = 51,      //
        dataNotFound = 99,             //
        userNotFound = 100,            //
        notEnoughRights = 110,         //
        invalidSchema = 120,           //
        internalError = 255            //
    };

    //std::shared_ptr<Services::DAO::SQL::Connection> _connection;
    //Services::UserService _userService;
    //Services::DatasetService _datasetService;
    //Services::StorageService _storageService;
    uint8_t *_buffer;
};

} /* namespace Services */
} /* namespace Beehive */
