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

#include <crypto/jwt.h>
#include <dao/NodeDAO.hpp>
#include <dao/UserDAO.hpp>
#include <entities/Developer.hpp>
#include <entities/Node.hpp>
#include <entities/User.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace Beehive {
namespace Services {

class UserService {
   public:
    UserService() {
    }
    virtual ~UserService() {
    }

    static std::unique_ptr<Entities::Developer> checkAdmin();
    std::unique_ptr<Entities::Developer> saveDeveloper(const std::string &email, const std::string &password, const std::string &name, Entities::Developer::Rights rights);
    std::unique_ptr<Entities::Developer> authenticateDeveloper(const std::string &authorization);

    void save(const std::string &identifier, const std::string &name, const std::string &password, const Entities::User::Type type, const std::string &context);
    std::string getUser(const std::string &uuid, const std::string &context);
    std::string getUsers(const std::string &context);
    void update(const std::string &identifier, const std::string &name, const std::string &password, const Entities::User::Type type, const std::string &context);
    void remove(const std::string &uuid, const std::string &context);
    std::unique_ptr<Entities::Node> authenticateUser(const std::string &authorization);
    static bool verifyGoogle(jwt::decoded_jwt decoded);
    static void setGoogleRSARS256PubKeys(std::vector<std::string> keys);


    std::unique_ptr<Entities::Node> signIn(const std::string &jwt, const std::string &module, const std::string &nodeUUID, Entities::User::Type type, const std::string &context);
    std::unique_ptr<Entities::Node> signIn(const std::string &email, const std::string &password, const std::string &module, const std::string &nodeUUID, const std::string &context);
    std::unique_ptr<Entities::Node> signUp(const std::string &name, const std::string &email, const std::string &password, const std::string &module, const std::string &nodeUUID, const std::string &context);
    void signOut(const Entities::Node &node);
    void signOff(const std::string &jwt, Entities::User::Type type, const std::string &context);
    void signOff(const std::string &email, const std::string &password, const std::string &context);
    std::unique_ptr<Entities::Node> reconnect(const std::string &auth);

   private:
    static std::vector<jwt::verifier<jwt::default_clock>> _googleVerifiers;
    static std::mutex _keyMutex;
};

} /* namespace Services */
} /* namespace Beehive */
