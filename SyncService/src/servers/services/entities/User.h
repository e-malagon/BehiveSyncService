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

#ifndef USER_H_
#define USER_H_

#include <string>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Entities {

class User {
public:
  uint32_t id() {
    return _id;
  }

  void id(uint32_t id) {
    _id = id;
  }

  const std::string& identifier() const {
    return _identifier;
  }

  void identifier(std::string identifier) {
    _identifier = identifier;
  }

  const std::string& name() const {
    return _name;
  }

  void name(std::string name) {
    _name = name;
  }

  uint32_t kind() {
    return _kind;
  }

  void kind(uint32_t kind) {
    _kind = kind;
  }

  const std::string& identity() const {
    return _identity;
  }

  void identity(std::string identity) {
    _identity = identity;
  }

  const std::string& password() const {
    return _password;
  }

  void password(std::string password) {
    _password = password;
  }

  const std::string& salt() const {
    return _salt;
  }

  void salt(std::string salt) {
    _salt = salt;
  }

private:
  uint32_t _id;
  std::string _identifier;
  std::string _name;
  uint32_t _kind;
  std::string _identity;
  std::string _password;
  std::string _salt;
};

} /* namespace Entities */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* USER_H_ */
