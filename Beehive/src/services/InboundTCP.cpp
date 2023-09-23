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


#include <services/InboundTCP.hpp>

#include <arpa/inet.h>
#include <cxxabi.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <tcp/SocketWatcher.hpp>
#include <tcp/TCPException.hpp>
#include <unistd.h>

#include <cstring>
#include <nanolog/NanoLog.hpp>
#include <string>
#include <thread>
#include <vector>

namespace Beehive {
namespace Services {

void InboundTCP::start() {
    LOG_INFO << "Starting Bin Server";
    if (0 <= (_tcpSocket = socket(AF_INET6, SOCK_STREAM, 0))) {
        int on = 1;
        struct sockaddr_in6 serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_port = htons(9440);
        serverAddr.sin6_addr = in6addr_any;
        if (setsockopt(_tcpSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) == 0 && bind(_tcpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == 0 && listen(_tcpSocket, 50) == 0) {
            LOG_INFO << "Waiting for incoming connections...";
            int clientSocket = 0;
            while (0 < (clientSocket = accept(_tcpSocket, NULL, NULL))) {
                std::thread([&](int clientSocket) {
                    struct sockaddr_in6 clientaddr;
                    unsigned int addrlen = sizeof(clientaddr);
                    char str[INET6_ADDRSTRLEN];
                    getpeername(clientSocket, (struct sockaddr *)&clientaddr, &addrlen);
                    if (inet_ntop(AF_INET6, &clientaddr.sin6_addr, str, sizeof(str))) {
                        LOG_DEBUG << "Connection received form: " << str << ":" << ntohs(clientaddr.sin6_port);
                    }
                    BinSyncHandlerIntance binSyncHandlerIntance(clientSocket);
                    binSyncHandlerIntance.run();
                    close(clientSocket);
                }, clientSocket).detach();
            }
        } else {
            LOG_ERROR << "Unable to open a socket: " << strerror(errno);
            exit(1);
        }
    } else {
        LOG_ERROR << "Unable to open a socket: " << strerror(errno);
        exit(1);
    }
    LOG_INFO << "Stoping Bin Server";
}

void InboundTCP::finish() {
  close(_tcpSocket);
}

using namespace __cxxabiv1;

void BinSyncHandlerIntance::run(void) {
    try {
        uint8_t option = readUInt8();
        std::unique_ptr<Services::Entities::Node> node;
        switch (option) {
            case 'I': {
                uint16_t crc;
                crc = 0x0000;
                uint16_t sizeToken = readUInt16C(crc);
                char token[sizeToken];
                readCharC(token, sizeToken, crc);
                uint8_t sizeContext = readUInt8C(crc);
                char contextChar[sizeContext];
                readCharC(contextChar, sizeContext, crc);
                uint8_t sizeModule = readUInt8C(crc);
                char moduleChar[sizeModule];
                readCharC(moduleChar, sizeModule, crc);
                uint8_t sizeNodeUUID = readUInt8C(crc);
                char nodeUUID[sizeNodeUUID];
                readCharC(nodeUUID, sizeNodeUUID, crc);
                uint32_t version = readUInt32C(crc);
                uint16_t finalCRC = readUInt16();
                if (finalCRC == crc) {
                    UserService userService;
                    std::string contextString(contextChar, sizeContext);
                    std::string moduleString(moduleChar, sizeModule);
                    node = userService.signIn(std::string(token, sizeToken), contextString, moduleString, std::string(nodeUUID, sizeNodeUUID), 0);
                    writeUInt8(Codes::success);
                    crc = 0x0000;
                    writeCharC(node->user().uuid().data(), node->user().uuid().size(), crc);
                    writeCharC(node->nodeKey().data(), node->nodeKey().size(), crc);
                    writeUInt16(crc);
                } else {
                    writeUInt8(Codes::messageTransmissionError);
                }
            }
                return;
            case 'S': {
                uint16_t crc;
                crc = 0x0000;
                uint16_t sizeEmail = readUInt8C(crc);
                char email[sizeEmail];
                readCharC(email, sizeEmail, crc);
                uint16_t sizePassword = readUInt8C(crc);
                char password[sizePassword];
                readCharC(password, sizePassword, crc);
                uint8_t sizeContext = readUInt8C(crc);
                char contextChar[sizeContext];
                readCharC(contextChar, sizeContext, crc);
                uint8_t sizeModule = readUInt8C(crc);
                char moduleChar[sizeModule];
                readCharC(moduleChar, sizeModule, crc);
                uint8_t sizeNodeUUID = readUInt8C(crc);
                char nodeUUID[sizeNodeUUID];
                readCharC(nodeUUID, sizeNodeUUID, crc);
                uint32_t version = readUInt32C(crc);
                uint16_t finalCRC = readUInt16();
                if (finalCRC == crc) {
                    UserService userService;
                    std::string contextString(contextChar, sizeContext);
                    std::string moduleString(moduleChar, sizeModule);
                    node = userService.signIn(std::string(email, sizeEmail), std::string(password, sizePassword), contextString, moduleString, std::string(nodeUUID, sizeNodeUUID));
                    writeUInt8(Codes::success);
                    crc = 0x0000;
                    writeCharC(node->user().uuid().data(), node->user().uuid().size(), crc);
                    writeCharC(node->nodeKey().data(), node->nodeKey().size(), crc);
                    writeUInt16(crc);
                } else {
                    writeUInt8(Codes::messageTransmissionError);
                }
            }
                return;
            case 'U': {
                uint16_t crc;
                crc = 0x0000;
                uint16_t sizeName = readUInt8C(crc);
                char name[sizeName];
                readCharC(name, sizeName, crc);
                uint16_t sizeEmail = readUInt8C(crc);
                char email[sizeEmail];
                readCharC(email, sizeEmail, crc);
                uint16_t sizePassword = readUInt8C(crc);
                char password[sizePassword];
                readCharC(password, sizePassword, crc);
                uint8_t sizeContext = readUInt8C(crc);
                char contextChar[sizeContext];
                readCharC(contextChar, sizeContext, crc);
                uint8_t sizeModule = readUInt8C(crc);
                char moduleChar[sizeModule];
                readCharC(moduleChar, sizeModule, crc);
                uint8_t sizeNodeUUID = readUInt8C(crc);
                char nodeUUID[sizeNodeUUID];
                readCharC(nodeUUID, sizeNodeUUID, crc);
                uint32_t version = readUInt32C(crc);
                uint16_t finalCRC = readUInt16();
                if (finalCRC == crc) {
                    UserService userService;
                    std::string contextString(contextChar, sizeContext);
                    std::string moduleString(moduleChar, sizeModule);
                    node = userService.signUp(std::string(name, sizeName), std::string(email, sizeEmail), std::string(password, sizePassword), contextString, moduleString, std::string(nodeUUID, sizeNodeUUID));
                    writeUInt8(Codes::success);
                    crc = 0x0000;
                    writeCharC(node->user().uuid().data(), node->user().uuid().size(), crc);
                    writeCharC(node->nodeKey().data(), node->nodeKey().size(), crc);
                    writeUInt16(crc);
                } else {
                    writeUInt8(Codes::messageTransmissionError);
                }
            }
                return;
            case 'F': {
                uint16_t crc;
                crc = 0x0000;
                uint16_t sizeToken = readUInt16C(crc);
                char token[sizeToken];
                readCharC(token, sizeToken, crc);
                uint8_t sizeContext = readUInt8C(crc);
                char contextChar[sizeContext];
                readCharC(contextChar, sizeContext, crc);
                uint16_t finalCRC = readUInt16();
                if (finalCRC == crc) {
                    UserService userService;
                    std::string contextString(contextChar, sizeContext);
                    userService.signOff(std::string(token, sizeToken), 0, contextString);
                    writeUInt8(Codes::success);
                } else {
                    writeUInt8(Codes::messageTransmissionError);
                }
            }
                return;
            case 'G': {
                uint16_t crc;
                crc = 0x0000;
                uint16_t sizeEmail = readUInt8C(crc);
                char email[sizeEmail];
                readCharC(email, sizeEmail, crc);
                uint16_t sizePassword = readUInt8C(crc);
                char password[sizePassword];
                readCharC(password, sizePassword, crc);
                uint8_t sizeContext = readUInt8C(crc);
                char contextChar[sizeContext];
                readCharC(contextChar, sizeContext, crc);
                uint16_t finalCRC = readUInt16();
                if (finalCRC == crc) {
                    UserService userService;
                    std::string contextString(contextChar, sizeContext);
                    userService.signOff(std::string(email, sizeEmail), std::string(password, sizePassword), contextString);
                    writeUInt8(Codes::success);
                } else {
                    writeUInt8(Codes::messageTransmissionError);
                }
            }
                return;
            case 'C': {
                char key[28];
                readChar(key, 28);
                UserService userService;
                node = userService.reconnect(std::string(key, 28));
                node->version(readUInt32());
                writeUInt8(Codes::success);
            } break;
        }
        option = readOperation();
        switch (option) {
            case 'O': {
                UserService userService;
                userService.signOut(*node);
                writeUInt8(Codes::success);
            } break;
            case 'e':
                deleteDataset(*node);
                break;
            case 'g':
                pushDataset(*node);
                break;
            case 'i':
                popDataset(*node);
                break;
            case 'r':
                putDataset(*node);
                break;
            case 't':
                pullDataset(*node);
                break;
            case 's':
                leaveDataset(*node);
                break;
            case 'k':
                updateMember(*node);
                break;
            case 'l':
                deleteMember(*node);
                break;
            case 'z':
                fullSync(*node);
                break;
            default:
                throw TCP::TransmissionErrorException("Unknown message: " + std::to_string(option), 0);
        }
        //_connection->unlock();
    } catch (TCP::TransmissionErrorException &e) {
        //_connection->rollback();
        //_connection->unlock();
        LOG_ERROR << e.what();
        try {
            writeUInt8(Codes::messageTransmissionError);
        } catch (...) {
        }
    } catch (Services::AuthenticationException &e) {
        //_connection->rollback();
        //_connection->unlock();
        try {
            writeUInt8(Codes::userNotFound);
        } catch (...) {
        }
    } catch (std::invalid_argument &e) {
        //_connection->rollback();
        //_connection->unlock();
        LOG_ERROR << e.what();
        try {
            writeUInt8(Codes::internalError);
        } catch (...) {
        }
    } catch (std::runtime_error &e) {
        //_connection->rollback();
        //_connection->unlock();
        LOG_ERROR << e.what();
        try {
            writeUInt8(Codes::internalError);
        } catch (...) {
        }
    } catch (...) {
        //_connection->rollback();
        //_connection->unlock();
        int status = 0;
        char *buff = __cxxabiv1::__cxa_demangle(__cxa_current_exception_type()->name(), NULL, NULL, &status);
        std::string demangled = buff;
        std::free(buff);
        LOG_ERROR << "Unknown exception type: " << demangled;
        try {
            writeUInt8(Codes::internalError);
        } catch (...) {
        }
    }
}

void BinSyncHandlerIntance::deleteDataset(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //_datasetService.removeDataset(node.user(), std::string(uuidDataset, 36));
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
    }
}

void BinSyncHandlerIntance::pushDataset(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        uint16_t size1 = readUInt8C(crc);
        readCharC((char *)_buffer, size1, crc);
        std::string role((char *)_buffer, size1);
        uint64_t until = readUInt64C(crc);
        uint32_t number = readUInt32C(crc);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
            //_storageService.context(&context);
            //_datasetService.context(&context);
            //Services::Entities::Push push = _datasetService.pushDataset(node, std::string(uuidDataset, 36), role, until, number);
            //writeUInt8(Codes::success);
            //crc = 0x0000;
            //writeUUIDC(push.uuid().data(), crc);
            //writeUInt16(crc);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotExistsException &e) {
        writeUInt8(Codes::dataNotFound);
    } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::popDataset(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        readUUIDC((char *)_buffer, crc);
        std::string uuid((char *)_buffer, 36);
        uint8_t size1 = readUInt8C(crc);
        readCharC((char *)_buffer, size1, crc);
        std::string name((char *)_buffer, size1);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
            //_storageService.context(&context);
            //_datasetService.context(&context);
            //std::unique_ptr<Services::Entities::Dataset> dataset = _datasetService.popDataset(node, std::string(uuidDataset, 36), uuid, name);
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotExistsException &e) {
        writeUInt8(Codes::dataNotFound);
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::pullDataset(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        readUUIDC((char *)_buffer, crc);
        std::string uuid((char *)_buffer, 36);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
            //_storageService.context(&context);
            //_datasetService.context(&context);
            //_datasetService.pullDataset(node, std::string(uuidDataset, 36), uuid);
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotExistsException &e) {
        writeUInt8(Codes::dataNotFound);
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::putDataset(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        uint8_t size1 = readUInt8C(crc);
        readCharC((char *)_buffer, size1, crc);
        std::string email((char *)_buffer, size1);
        uint8_t size2 = readUInt8C(crc);
        readCharC((char *)_buffer, size2, crc);
        std::string name((char *)_buffer, size2);
        uint16_t size3 = readUInt8C(crc);
        readCharC((char *)_buffer, size3, crc);
        std::string role((char *)_buffer, size3);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
            //_storageService.context(&context);
            //_datasetService.context(&context);
            //_datasetService.putDataset(node, std::string(uuidDataset, 36), email, name, role);
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotExistsException &e) {
        writeUInt8(Codes::dataNotFound);
    } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::leaveDataset(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //_datasetService.leaveDataset(node, std::string(uuidDataset, 36));
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::updateMember(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        uint32_t idUser = (uint32_t)readUInt64C(crc);
        uint16_t size1 = readUInt8C(crc);
        readCharC((char *)_buffer, size1, crc);
        std::string role((char *)_buffer, size1);
        uint8_t size2 = readUInt8C(crc);
        readCharC((char *)_buffer, size2, crc);
        std::string name((char *)_buffer, size2);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
            //_storageService.context(&context);
            //_datasetService.context(&context);
            //_datasetService.updateMember(node, std::string(uuidDataset, 36), idUser, role, name);
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotExistsException &e) {
        writeUInt8(Codes::dataNotFound);
    } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::deleteMember(Services::Entities::Node &node) {
    try {
        uint16_t crc;
        crc = 0x0000;
        char uuidDataset[36];
        readUUIDC(uuidDataset, crc);
        uint32_t idUser = (uint32_t)readUInt64C(crc);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
            //Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
            //_storageService.context(&context);
            //_datasetService.context(&context);
            //_datasetService.removeMember(node, std::string(uuidDataset, 36), idUser);
            writeUInt8(Codes::success);
        } else {
            writeUInt8(Codes::messageTransmissionError);
        }
    } catch (Services::NotExistsException &e) {
        writeUInt8(Codes::dataNotFound);
    } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
    } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
    }
}

void BinSyncHandlerIntance::fullSync(Services::Entities::Node &node) {
    uint8_t len8;
    uint16_t len16;
    uint16_t crc;
    uint8_t code;
    /*  _connection->commit();
  Services::Config::Context context = Services::SchemaService::getContextAndModuleUUID(node.context());
  _storageService.context(&context);
  _datasetService.context(&context);
  std::unordered_map<std::string, Services::Config::Entity, Services::Utils::IHasher, Services::Utils::IEqualsComparator> &entities = _storageService.entities(node.version());
  auto roles = context.roles[node.version()];
  std::vector<Services::Entities::Dataset> datasets = _datasetService.readDatasets(node.user());
  std::unordered_map<std::string, Services::Entities::Dataset> datasetsMap;
  crc = 0x0000;
  writeUInt16C(datasets.size(), crc);
  for (Services::Entities::Dataset &dataset : datasets) {
    writeUUIDC(dataset.uuid().c_str(), crc);
    datasetsMap.emplace(dataset.uuid(), dataset);
  }
  writeUInt16(crc);
  code = readUInt8();
  while (code == Codes::newContainerAvailable) {
    crc = 0x0000;
    char uuidDataset[36];
    readUUIDC(uuidDataset, crc);
    uint32_t idHeader = readUInt32C(crc);
    uint8_t status = readUInt8C(crc);
    uint16_t finalCRC = readUInt16();
    if (status == 2) {
      Services::Entities::Dataset dataset = _datasetService.addDataset(node.user(), std::string(uuidDataset, 36));
      datasets.push_back(dataset);
      datasetsMap.emplace(dataset.uuid(), dataset);
    }
    auto datasetPtr = datasetsMap.find(std::string(uuidDataset, 36));
    if (datasetPtr == datasetsMap.end())
      throw Services::NotEnoughRightsException("No valid data set received");
    std::pair<uint32_t, uint32_t> readed = _storageService.readLastSynchronizedId(node, datasetPtr->second.id());
    if (finalCRC == crc) {
      if (readed.first < idHeader)
        _storageService.updateLastSynchronizedId(node, datasetPtr->second.id(), idHeader, readed.second);
    } else {
      throw TCP::TransmissionErrorException("Error on transport invalid CRC", 1234);
    }
    bool isMember = _storageService.isMember(node, datasetPtr->second.id());
    code = readUInt8();
    if (code == Codes::newElementAvailable) {
      _datasetService.isPutAllowedForRole(node, datasetPtr->second.id());
      while (code == Codes::newElementAvailable) {
        crc = 0x0000;
        len8 = readUInt8C(crc);
        readCharC((char*) _buffer, len8, crc);
        std::string email((char*) _buffer, len8);
        len8 = readUInt8C(crc);
        readCharC((char*) _buffer, len8, crc);
        std::string name((char*) _buffer, len8);
        len8 = readUInt8C(crc);
        readCharC((char*) _buffer, len8, crc);
        std::string role((char*) _buffer, len8);
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
          _datasetService.putDataset(node, datasetPtr->second, email, name, role);
        } else {
          throw TCP::TransmissionErrorException("Error on transport invalid CRC", 1234);
        }
        code = readUInt8();
      }
    }
    code = readUInt8();
    if (code == Codes::newGroupAvailable) {
      while (code == Codes::newGroupAvailable) {
        Services::Entities::Header header;
        crc = 0x0000;
        header.idDataset(datasetPtr->second.id());
        header.node(node.id());
        header.idNode(readUInt32C(crc));
        len8 = readUInt8C(crc);
        readCharC((char*) _buffer, len8, crc);
        header.transactionName(std::string((char*) _buffer, len8));
        header.status(0);
        header.version(readUInt32C(crc));
        code = readUInt8();
        while (code == Codes::newElementAvailable) {
          Services::Entities::Change change;
          change.idChange(readUInt16C(crc));
          change.operation(readUInt8C(crc));
          len8 = readUInt8C(crc);
          readCharC((char*) _buffer, len8, crc);
          change.entityName(std::string((char*) _buffer, len8));
          len8 = readUInt8C(crc);
          readCharC((char*) _buffer, len8, crc);
          change.newPK(std::string((char*) _buffer, len8));
          len8 = readUInt8C(crc);
          readCharC((char*) _buffer, len8, crc);
          change.oldPK(std::string((char*) _buffer, len8));
          len16 = readUInt16C(crc, 32767);
          readCharC((char*) _buffer, len16, crc);
          change.newData(std::string((char*) _buffer, len16));
          len16 = readUInt16C(crc, 32767);
          readCharC((char*) _buffer, len16, crc);
          change.oldData(std::string((char*) _buffer, len16));
          header.changes().push_back(std::move(change));
          code = readUInt8();
        }
        uint16_t finalCRC = readUInt16();
        if (finalCRC == crc) {
          if (isMember && readed.second < header.idNode())
            _storageService.saveHeader(node, header, idHeader);
        } else {
          throw TCP::TransmissionErrorException("Error on transport invalid CRC", 1234);
        }
        code = readUInt8();
      }
    }
    code = readUInt8();
  }

  for (Services::Entities::Dataset &dataset : datasets) {
    writeUInt8(Codes::newContainerAvailable);
    crc = 0x0000;
    writeUUIDC(dataset.uuid().c_str(), crc);
    writeUInt32C(dataset.idHeader(), crc); // TODO agregar version minima para la aplicacion
    std::unordered_map<std::string, std::unordered_set<int>> entitiesByNode = _storageService.entitiesByNode(node, dataset.id());
    _connection->lock(std::to_string(dataset.id()));
    for (Services::Entities::Member &m : _datasetService.readMembers(node, dataset.id())) {
      writeUInt8(Codes::newGroupAvailable);
      writeUInt32C(m.idUser(), crc);
      auto rolePtr = roles.find(m.role());
      if (rolePtr == roles.end()) {
        LOG_WARN << "Unknown role" << m.role() << " for context " << context.name;
        writeUInt8C(m.role().size(), crc);
        writeCharC(m.role().data(), m.role().size(), crc);
      } else {
        writeUInt8C(rolePtr->second.name.size(), crc);
        writeCharC(rolePtr->second.name.data(), rolePtr->second.name.size(), crc);
      }
      writeUInt8C(m.email().size(), crc);
      writeCharC(m.email().data(), m.email().size(), crc);
      writeUInt8C(m.name().size(), crc);
      writeCharC(m.name().data(), m.name().size(), crc);
      writeUInt8C(m.status(), crc);
    }
    writeUInt8(Codes::success);
    for (Services::Entities::Push &p : _datasetService.readPush(node, dataset.id())) {
      writeUInt8(Codes::newGroupAvailable);
      writeUUIDC(p.uuid().c_str(), crc);
      auto rolePtr = roles.find(p.role());
      if (rolePtr == roles.end()) {
        LOG_WARN << "Unknown role" << p.role() << " for context " << context.name;
        writeUInt8C(p.role().size(), crc);
        writeCharC(p.role().data(), p.role().size(), crc);
      } else {
        writeUInt8C(rolePtr->second.name.size(), crc);
        writeCharC(rolePtr->second.name.data(), rolePtr->second.name.size(), crc);
      }
      writeUInt64C(p.until(), crc);
      writeUInt16C(p.number(), crc);
    }
    writeUInt8(Codes::success);
    writeUInt16(crc);
    std::pair<uint32_t, uint32_t> lastSynchronizedId = _storageService.readLastSynchronizedId(node, dataset.id());
    if (lastSynchronizedId.first == 0 && lastSynchronizedId.second == 0) {
      try {
        for (auto &entity : entitiesByNode) {
          auto entityPtr = entities.find(entity.first);
          if (entityPtr != entities.end()) {
            Services::StorageService::EntityReader entityReader = _storageService.readEntityData(node, dataset.id(), entityPtr->second, entitiesByNode);
            for (auto &change : entityReader) {
              writeUInt8(Codes::newElementAvailable);
              crc = 0x0000;
              writeUInt16C(change.idChange(), crc);
              writeUInt8C(change.operation(), crc);
              writeUInt8C(change.entityName().size(), crc);
              writeCharC(change.entityName().data(), change.entityName().size(), crc);
              writeUInt8C(change.newPK().size(), crc);
              writeCharC(change.newPK().data(), change.newPK().size(), crc);
              writeUInt8C(change.oldPK().size(), crc);
              writeCharC(change.oldPK().data(), change.oldPK().size(), crc);
              writeUInt16C(change.newData().size(), crc);
              writeCharC(change.newData().data(), change.newData().size(), crc);
              writeUInt16C(change.oldData().size(), crc);
              writeCharC(change.oldData().data(), change.oldData().size(), crc);
              writeUInt16(crc);
            }
          }
        }
        writeUInt8(Codes::success);
      } catch (Services::NotExistException &e) {
        writeUInt8(Codes::dataNotFound);
      } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
      } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
      }
    } else {
      try {
        auto transactions = context.transactions[node.version()];
        std::vector<Services::Entities::Header> headers = _storageService.readHeaders(node, dataset.id(), lastSynchronizedId.first);
        for (Services::Entities::Header &header : headers) {
          std::vector<Services::Entities::Change> changes;
          if (header.status() == Codes::success && header.node() != node.id())
            changes = _storageService.readChanges(node, dataset.id(), header.idHeader(), entities, entitiesByNode);
          if (0 < changes.size() || header.node() == node.id()) {
            auto transactionPrt = transactions.find(header.transactionUUID());
            if (transactionPrt != transactions.end() || header.transactionUUID() == "00000000-0000-0000-0000-000000000000") {
              writeUInt8(Codes::newGroupAvailable);
              crc = 0x0000;
              writeUInt32C(header.idHeader(), crc);
              writeUInt32C(header.node() != node.id() ? 0 : header.idNode(), crc);
              if (transactionPrt != transactions.end()) {
                writeUInt8C(transactionPrt->second.name.size(), crc);
                writeCharC(transactionPrt->second.name.data(), transactionPrt->second.name.size(), crc);
              } else {
                writeUInt8C(7, crc);
                writeCharC("Unknown", 7, crc);
              }
              writeUInt8C(header.node() != node.id() || header.status() != Services::StorageService::success ? header.status() : Services::StorageService::approved, crc);
              for (Services::Entities::Change &change : changes) {
                writeUInt8(Codes::newElementAvailable);
                writeUInt16C(change.idChange(), crc);
                writeUInt8C(change.operation(), crc);
                writeUInt8C(change.entityName().size(), crc);
                writeCharC(change.entityName().data(), change.entityName().size(), crc);
                writeUInt8C(change.newPK().size(), crc);
                writeCharC(change.newPK().data(), change.newPK().size(), crc);
                writeUInt8C(change.oldPK().size(), crc);
                writeCharC(change.oldPK().data(), change.oldPK().size(), crc);
                writeUInt16C(change.newData().size(), crc);
                writeCharC(change.newData().data(), change.newData().size(), crc);
                writeUInt16C(change.oldData().size(), crc);
                writeCharC(change.oldData().data(), change.oldData().size(), crc);
              }
              writeUInt8(Codes::success);
              writeUInt16(crc);
            }
          }
        }
        writeUInt8(Codes::success);
      } catch (Services::NotExistException &e) {
        writeUInt8(Codes::dataNotFound);
      } catch (Services::NotEnoughRightsException &e) {
        writeUInt8(Codes::notEnoughRights);
      } catch (Services::InvalidSchemaException &e) {
        writeUInt8(Codes::invalidSchema);
      }
    }
    _connection->unlock(std::to_string(dataset.id()));
  }
  writeUInt8(Codes::success);
  writeUInt8(Codes::success);*/
}

} /* namespace Services */
} /* namespace Beehive */
