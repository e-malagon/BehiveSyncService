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


#include <dao/HeaderDAO.hpp>
#include <dao/Storage.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string HeaderDAO::prefix("H.");

void HeaderDAO::save(Entities::Header &header, const std::string &context) {
/*
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO `headers`(`iddataset`, `idheader`, `transaction`, `node`, `idnode`, `status`) VALUES(?, ?, ?, ?, ?, ?);");
  stmt->setInt(1, header.idDataset());
  stmt->setInt(2, header.idHeader());
  stmt->setUUID(3, header.transactionUUID());
  stmt->setInt(4, header.node());
  stmt->setInt(5, header.idNode());
  stmt->setInt(6, header.status());
  stmt->executeUpdate();
  */
}

std::unique_ptr<Entities::Header> HeaderDAO::read(uint32_t idDataset, uint32_t node, uint32_t idNode, const std::string &context) {
  std::unique_ptr<Entities::Header> header;
/*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SELECT `iddataset`, `idheader`, `transaction`, `node`, `idnode`, `date`, `status` FROM `headers` WHERE `iddataset` = ? AND `node` = ? AND `idnode` = ?;");
  stmt->setInt(1, idDataset);
  stmt->setInt(2, node);
  stmt->setInt(3, idNode);
  res = stmt->executeQuery();
  if (res->next()) {
    header = std::make_unique<Entities::Header>();
    header->idDataset(res->getInt("iddataset"));
    header->idHeader(res->getInt("idheader"));
    header->transactionUUID(res->getUUID("transaction"));
    header->node(res->getInt("node"));
    header->idNode(res->getInt("idnode"));
    header->status(res->getInt("status"));
  }
  delete res;
  */
  return header;
}

std::vector<Entities::Header> HeaderDAO::readFrom(uint32_t idDataset, uint32_t idHeader, const std::string &context) {
  std::vector<Entities::Header> headers;
/*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SELECT `iddataset`, `idheader`, `transaction`, `node`, `idnode`, `date`, `status` FROM `headers` WHERE `iddataset` = ? AND `idheader` > ? ORDER BY `iddataset`, `idheader`;");
  stmt->setInt(1, idDataset);
  stmt->setInt(2, idHeader);
  res = stmt->executeQuery();
  while (res->next()) {
    Entities::Header header;
    header.idDataset(res->getInt("iddataset"));
    header.idHeader(res->getInt("idheader"));
    header.transactionUUID(res->getUUID("transaction"));
    header.node(res->getInt("node"));
    header.idNode(res->getInt("idnode"));
    header.status(res->getInt("status"));
    headers.push_back(header);
  }
  delete res;
  */
  return headers;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
