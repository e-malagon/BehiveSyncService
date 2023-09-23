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

#ifndef DATASET_H_
#define DATASET_H_

#include <string>
#include <vector>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Entities {

class Dataset {
public:
  uint32_t id() {
    return _id;
  }

  void id(uint32_t id) {
    _id = id;
  }

  const std::string &uuid() const {
    return _uuid;
  }

  void uuid(std::string uuid) {
    _uuid = uuid;
  }

  uint32_t idHeader() {
    return _idHeader;
  }

  void idHeader(uint32_t idHeader) {
    _idHeader = idHeader;
  }

  uint32_t owner() {
    return _owner;
  }

  void owner(uint32_t owner) {
    _owner = owner;
  }

  uint8_t status() {
    return _status;
  }

  void status(uint8_t status) {
    _status = status;
  }

private:
  uint32_t _id;
  std::string _uuid;
  uint32_t _idHeader;
  uint32_t _owner;
  uint8_t _status;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* DATASET_H_ */
