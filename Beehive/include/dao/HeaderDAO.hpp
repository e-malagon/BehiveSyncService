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

#include <entities/Header.hpp>

#include <memory>
#include <vector>

namespace Beehive {
namespace Services {
namespace DAO {

class HeaderDAO {
      public:
      HeaderDAO() {
      }
      void save(Entities::Header &header, const std::string &context);
      std::unique_ptr<Entities::Header> read(uint32_t idDataset, uint32_t node, uint32_t idNode, const std::string &context);
      std::vector<Entities::Header> readFrom(uint32_t idDataset, uint32_t idHeader, const std::string &context);

   private:
      static std::string prefix;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
