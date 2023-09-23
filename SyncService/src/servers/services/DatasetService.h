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

#ifndef DATASETSERVICE_H_
#define DATASETSERVICE_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <dao/DatasetDAO.h>
#include <dao/MemberDAO.h>
#include <dao/SchemaDAO.h>
#include <dao/PushDAO.h>
#include <config/Application.h>
#include <config/Module.h>
#include <entities/Dataset.h>
#include <entities/Node.h>
#include <entities/User.h>
#include <entities/Push.h>

#include <vector>
#include <memory>

namespace SyncServer {
namespace Servers {
namespace Services {

class DatasetService {
public:
  DatasetService(std::shared_ptr<DAO::SQL::Connection> connection, uint32_t beehive) :
      _connection(connection), _datasetDAO(connection), _memberDAO(connection), _pushDAO(connection), _application(0), _beehive(beehive) {
  }

  virtual ~DatasetService() {
  }

  std::vector<Entities::Member> readMembers(Entities::Node &node, uint32_t idDataset);
  std::vector<Entities::Push> readPush(Entities::Node &node, uint32_t idDataset);
  bool isPutAllowedForRole(Entities::Node &node, uint32_t idDataset);

  Entities::Dataset addDataset(Entities::User &user, std::string uuid);
  std::vector<Entities::Dataset> readDatasets(Entities::User &user);
  void removeDataset(Entities::User &user, std::string uuidDataset);

  Entities::Push pushDataset(Entities::Node &node, std::string uuidDataset, std::string &role, uint64_t until, uint32_t number);
  std::unique_ptr<Entities::Dataset> popDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid, std::string &name);
  void pullDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid);
  uint32_t putDataset(Entities::Node &node, std::string uuidDataset, std::string &email, std::string &name, std::string &role);
  uint32_t putDataset(Entities::Node &node, Entities::Dataset &dataset, std::string &email, std::string &name, std::string &role);
  void leaveDataset(Entities::Node &node, std::string uuidDataset);

  void updateMember(Entities::Node &node, std::string uuidDatasett, uint32_t idUser, std::string &role, std::string &name);
  void removeMember(Entities::Node &node, std::string uuidDataset, uint32_t idUser);

  void application(Config::Application *application) {
    _application = application;
  }

private:
  std::shared_ptr<DAO::SQL::Connection> _connection;
  DAO::DatasetDAO _datasetDAO;
  DAO::MemberDAO _memberDAO;
  DAO::PushDAO _pushDAO;
  Config::Application *_application;
  uint32_t _beehive;
};

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* DATASETSERVICE_H_ */
