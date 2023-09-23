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

#ifndef HEADER_H_
#define HEADER_H_

#include <entities/Change.h>

#include <string>
#include <vector>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Entities {

class Header {
public:
  uint32_t idDataset() {
    return _idDataset;
  }

  void idDataset(uint32_t idDataset) {
    _idDataset = idDataset;
  }

  uint32_t idHeader() {
    return _idHeader;
  }

  void idHeader(uint32_t idHeader) {
    _idHeader = idHeader;
  }

  std::string transactionName() {
    return _transactionName;
  }

  void transactionName(std::string transactionName) {
    _transactionName = transactionName;
  }

  std::string transactionUUID() {
    return _transactionUUID;
  }

  void transactionUUID(std::string transactionUUID) {
    _transactionUUID = transactionUUID;
  }

  uint32_t node() {
    return _node;
  }

  void node(uint32_t node) {
    _node = node;
  }

  uint32_t idNode() {
    return _idNode;
  }

  void idNode(uint32_t idNode) {
    _idNode = idNode;
  }

  uint8_t status() {
    return _status;
  }

  void status(uint8_t status) {
    _status = status;
  }

  uint32_t version() {
    return _version;
  }

  void version(uint32_t version) {
    _version = version;
  }

  std::vector<Change>& changes() {
    return _changes;
  }

  void changes(const std::vector<Change> changes) {
    _changes = std::move(changes);
  }

private:
  uint32_t _idDataset;
  uint32_t _idHeader;
  std::string _transactionName;
  std::string _transactionUUID;
  uint32_t _node;
  uint32_t _idNode;
  uint8_t _status;
  uint32_t _version;
  std::vector<Change> _changes;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* HEADER_H_ */
