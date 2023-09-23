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


#include <dao/DownloadedDAO.hpp>
#include <dao/Storage.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string DownloadedDAO::prefix("d.");

void DownloadedDAO::save(std::string uuidNode, uint32_t idDataset, uint32_t idHeader, uint32_t idCell, const std::string &context) {
/*  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("REPLACE INTO `downloaded`(`idnode`, `iddataset`, `idheader`, `idcell`) VALUES(?, ?, ?, ?);");
  stmt->setInt(1, idNode);
  stmt->setInt(2, idDataset);
  stmt->setInt(3, idHeader);
  stmt->setInt(4, idCell);
  stmt->executeUpdate();
  */
}

std::pair<uint32_t, uint32_t> DownloadedDAO::read(std::string uuidNode, uint32_t idDataset, const std::string &context) {
  uint32_t idHeader = 0;
  uint32_t idCell = 0;
/*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SELECT `idheader`, `idcell` FROM `downloaded` WHERE `idnode` = ? AND `iddataset` = ?;");
  stmt->setInt(1, _idNode);
  stmt->setInt(2, idDataset);
  res = stmt->executeQuery();
  if (res->next()) {
    idHeader = res->getInt("idheader");
    idCell = res->getInt("idcell");
  }
  delete res;
  */
  return std::make_pair(idHeader, idCell);
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
