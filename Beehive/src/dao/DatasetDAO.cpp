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


#include <dao/DatasetDAO.hpp>
#include <dao/Storage.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

std::string DatasetDAO::prefix("D.");

void DatasetDAO::save(Entities::Dataset &dataset, const std::string &context) {
    /*
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("INSERT INTO `datasets`(`uuid`, `owner`, `idheader`, `status`) VALUES(?, ?, ?, ?);");
  stmt->setString(1, dataset.uuid());
  stmt->setInt(2, dataset.owner());
  stmt->setInt(3, dataset.idHeader());
  stmt->setInt(4, dataset.status());
  stmt->executeUpdate();
  dataset.id(stmt->getLastId());
  */
}

std::unique_ptr<Entities::Dataset> DatasetDAO::read(uint32_t id, const std::string &context) {
    std::unique_ptr<Entities::Dataset> dataset;
    /*
  SQL::Statement *stmt;
  SQL::ResultSet *res;
  stmt = _connection->prepareStatement("SELECT `iddataset`, `uuid`, `owner`, `idheader`, `status` FROM `datasets` WHERE `iddataset` = ?;");
  stmt->setInt(1, id);
  res = stmt->executeQuery();
  if (res->next()) {
    dataset = std::make_unique<Entities::Dataset>();
    dataset->id(res->getInt("iddataset"));
    dataset->uuid(res->getString("uuid"));
    dataset->owner(res->getInt("owner"));
    dataset->idHeader(res->getInt("idheader"));
    dataset->status(res->getInt("status"));
  }
  delete res;
  */
    return dataset;
}

std::unique_ptr<Entities::Dataset> DatasetDAO::read(std::string &uuid, const std::string &context) {
    std::unique_ptr<Entities::Dataset> dataset;
    /*
    SQL::Statement *stmt;
    SQL::ResultSet *res;
    stmt = _connection->prepareStatement("SELECT `iddataset`, `uuid`, `owner`, `idheader`, `status` FROM `datasets` WHERE `uuid` = ?;");
    stmt->setString(1, uuid);
    res = stmt->executeQuery();
    if (res->next()) {
        dataset = std::make_unique<Entities::Dataset>();
        dataset->id(res->getInt("iddataset"));
        dataset->uuid(res->getString("uuid"));
        dataset->owner(res->getInt("owner"));
        dataset->idHeader(res->getInt("idheader"));
        dataset->status(res->getInt("status"));
    }
    delete res;
    */
    return dataset;
}

std::vector<Entities::Dataset> DatasetDAO::readByUser(std::string uuidUser, const std::string &context) {
    std::vector<Entities::Dataset> datasets;
/*
    SQL::Statement *stmt;
    SQL::ResultSet *res;
    stmt = _connection->prepareStatement("SELECT a.`iddataset`, `uuid`, `idheader`, `owner`, a.`status` FROM `datasets` a, `members` b WHERE a.`iddataset` = b.`iddataset` AND b.`iduser` = ? AND b.`status` = 1;");
    stmt->setInt(1, idUser);
    res = stmt->executeQuery();
    while (res->next()) {
        Entities::Dataset dataset;
        dataset.id(res->getInt("iddataset"));
        dataset.uuid(res->getString("uuid"));
        dataset.owner(res->getInt("owner"));
        dataset.idHeader(res->getInt("idheader"));
        dataset.status(res->getInt("status"));
        datasets.push_back(dataset);
    }
    delete res;
    */
    return datasets;
}

int DatasetDAO::update(Entities::Dataset &dataset, const std::string &context) {
/*    SQL::Statement *stmt;
    stmt = _connection->prepareStatement("UPDATE `datasets` SET `idheader` = ?, `status` = ? WHERE `iddataset` = ?;");
    stmt->setInt(1, dataset.idHeader());
    stmt->setInt(2, dataset.status());
    stmt->setInt(3, dataset.id());
    return stmt->executeUpdate();
    */
   return 0;
}

int DatasetDAO::remove(uint32_t id, const std::string &context) {
    /*
    SQL::Statement *stmt;
    stmt = _connection->prepareStatement("DELETE FROM `datasets` WHERE `iddataset` = ?;");
    stmt->setInt(1, id);
    return stmt->executeUpdate();
    */
       return 0;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
