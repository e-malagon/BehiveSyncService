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

#ifndef USERSERVICE_H_
#define USERSERVICE_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <dao/UserDAO.h>
#include <dao/NodeDAO.h>
#include <entities/User.h>
#include <entities/Node.h>
#include <crypto/jwt.h>

#include <vector>
#include <memory>
#include <mutex>

namespace SyncServer {
namespace Servers {
namespace Services {

class UserService {
public:
  UserService(std::shared_ptr<DAO::SQL::Connection> connection) :
      _connection(connection), _userDAO(connection), _nodeDAO(connection) {
  }
  virtual ~UserService() {
  }

  std::unique_ptr<Entities::Node> signIn(std::string jwt, const std::string &application, const std::string &module, const std::string nodeUUID, int type);
  std::unique_ptr<Entities::Node> signIn(std::string email, std::string password, const std::string &application, const std::string &module, const std::string nodeUUID);
  std::unique_ptr<Entities::Node> signUp(std::string name, std::string email, std::string password, const std::string &application, const std::string &module, const std::string nodeUUID);
  void signOut(Entities::Node &node);
  void signOff(std::string jwt, int type);
  void signOff(std::string email, std::string password);
  std::unique_ptr<Entities::Node> reconnect(std::string auth);
  std::vector<Entities::User> getUsers();
  std::string getUser(uint32_t identifier);
  uint64_t save(const std::string &name, std::string &email, const std::string &password, int type);
  bool update(uint32_t identifier, const std::string &name, std::string &email, const std::string &password, int type);
  void remove(uint32_t identifier);
  static bool verifyGoogle(jwt::decoded_jwt decoded);
  static void setGoogleRSARS256PubKeys(std::vector<std::string> keys);

private:
  static std::vector<jwt::verifier<jwt::default_clock>> _googleVerifiers;
  static std::mutex _keyMutex;
  std::shared_ptr<DAO::SQL::Connection> _connection;
  DAO::UserDAO _userDAO;
  DAO::NodeDAO _nodeDAO;
};

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* USERSERVICE_H_ */
