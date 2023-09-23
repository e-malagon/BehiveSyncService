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


#include <dao/NodeDAO.hpp>
#include <dao/Storage.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string NodeDAO::prefix("N.");

void NodeDAO::save(const Entities::Node &node, Storage::Transaction &transaction) {
  nlohmann::json json =static_cast<nlohmann::json>(node);
    std::string value = json.dump();
    transaction.putValue(prefix + node.user().uuid() + node.uuid(), value, Storage::DefaultContext);
}

std::unique_ptr<Entities::Node> NodeDAO::read(const std::string &uuidNode, const std::string &uuidUser) {
  std::unique_ptr<Entities::Node> node;
    std::string value;
    if (Storage::getValue(prefix + uuidUser + uuidNode, &value, Storage::DefaultContext))
        node = std::make_unique<Entities::Node>(static_cast<Entities::Node>(nlohmann::json::parse(value)));
  return node;
}

std::unique_ptr<Entities::Node> NodeDAO::read(const std::string &uuidNode, const std::string &uuidUser, Storage::Transaction &transaction) {
  std::unique_ptr<Entities::Node> node;
    std::string value;
    if (transaction.getValue(prefix + uuidUser + uuidNode, &value, Storage::DefaultContext))
        node = std::make_unique<Entities::Node>(static_cast<Entities::Node>(nlohmann::json::parse(value)));
  return node;
}

int NodeDAO::remove(const std::string &uuidNode, const std::string &uuidUser, Storage::Transaction &transaction) {
  transaction.deleteValue(prefix + uuidUser + uuidNode, Storage::DefaultContext);
 return 0;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
