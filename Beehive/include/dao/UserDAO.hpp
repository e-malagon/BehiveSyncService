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

#include <dao/Storage.hpp>
#include <entities/Developer.hpp>
#include <entities/User.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Beehive {
namespace Services {
namespace DAO {

class UserDAO {
   public:
      UserDAO() {
      }

      void saveDeveloper(Entities::Developer &admin);
      std::unique_ptr<Entities::Developer> readAdmin(const std::string &identifier);

      void save(Entities::User &user, const std::string &context, Storage::Transaction &transaction);
      std::unique_ptr<Entities::User> read(const std::string &identifier, const std::string &context);
      std::unique_ptr<Entities::User> readByUUID(const std::string &uuid, const std::string &context);
      std::vector<Entities::User> read(const std::string &context);
      void update(Entities::User &user, const std::string &context, Storage::Transaction &transaction);
      void remove(const std::string &uuid, const std::string &context, Storage::Transaction &transaction);

   private:
      static std::string prefix;
      static std::string ixprefix;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
