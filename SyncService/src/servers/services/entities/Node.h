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

#ifndef NODE_H_
#define NODE_H_

#include <entities/User.h>

#include <string>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Entities {

class Node {
public:
  uint32_t id() {
    return _id;
  }

  void id(uint32_t id) {
    _id = id;
  }

  User& user() {
    return _user;
  }

  void user(User user) {
    _user = user;
  }

  std::string& key() {
    return _key;
  }

  void key(std::string key) {
    _key = key;
  }

  std::string& nodeKey() {
    return _nodeKey;
  }

  void nodeKey(std::string nodeKey) {
    _nodeKey = nodeKey;
  }

  std::string& application() {
    return _application;
  }

  void application(std::string application) {
    _application = application;
  }

  std::string& module() {
    return _module;
  }

  void module(std::string module) {
    _module = module;
  }

  std::string& uuid() {
    return _uuid;
  }

  void uuid(std::string uuid) {
    _uuid = uuid;
  }

  uint32_t version() {
    return _version;
  }

  void version(uint32_t version) {
    _version = version;
  }

private:
  uint32_t _id;
  User _user;
  std::string _key;
  std::string _nodeKey;
  std::string _application;
  std::string _module;
  std::string _uuid;
  uint32_t _version;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
#endif /* NODE_H_ */

