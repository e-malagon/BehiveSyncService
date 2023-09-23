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
#include <entities/Node.hpp>

#include <memory>
#include <vector>

namespace Beehive {
namespace Services {
namespace DAO {

class NodeDAO {
   public:
      NodeDAO() {
      }
      void save(const Entities::Node &node, Storage::Transaction &transaction);
      std::unique_ptr<Entities::Node> read(const std::string &uuidNode, const std::string &uuidUser);
      std::unique_ptr<Entities::Node> read(const std::string &uuidNode, const std::string &uuidUser, Storage::Transaction &transaction);
      int remove(const std::string &uuidNode, const std::string &uuidUser, Storage::Transaction &transaction);

   private:
      static std::string prefix;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
