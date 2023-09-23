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

#include <dao/DatasetDAO.hpp>
#include <dao/MemberDAO.hpp>
#include <dao/PushDAO.hpp>
#include <config/Context.hpp>
#include <config/Module.hpp>
#include <entities/Dataset.hpp>
#include <entities/Node.hpp>
#include <entities/User.hpp>
#include <entities/Push.hpp>

#include <vector>
#include <memory>

namespace Beehive {
namespace Services {

class DatasetService {
public:
  DatasetService() : _context(nullptr) {
  }

  virtual ~DatasetService() {
  }

  std::vector<Entities::Member> readMembers(Entities::Node &node, uint32_t idDataset, const std::string &context);
  std::vector<Entities::Push> readPush(Entities::Node &node, uint32_t idDataset, const std::string &context);
  bool isPutAllowedForRole(Entities::Node &node, uint32_t idDataset, const std::string &context);

  Entities::Dataset addDataset(Entities::User &user, std::string uuid, const std::string &context);
  std::vector<Entities::Dataset> readDatasets(Entities::User &user, const std::string &context);
  void removeDataset(Entities::User &user, std::string uuidDataset, const std::string &context);

  Entities::Push pushDataset(Entities::Node &node, std::string uuidDataset, std::string &role, uint64_t until, uint32_t number, const std::string &context);
  std::unique_ptr<Entities::Dataset> popDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid, std::string &name, const std::string &context);
  void pullDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid, const std::string &context);
  std::string putDataset(Entities::Node &node, std::string uuidDataset, std::string &email, std::string &name, std::string &role, const std::string &context);
  std::string putDataset(Entities::Node &node, Entities::Dataset &dataset, std::string &email, std::string &name, std::string &role, const std::string &context);
  void leaveDataset(Entities::Node &node, std::string uuidDataset, const std::string &context);

  void updateMember(Entities::Node &node, std::string uuidDatasett, std::string uuidUser, std::string &role, std::string &name, const std::string &context);
  void removeMember(Entities::Node &node, std::string uuidDataset, std::string uuidUser, const std::string &context);

  void context(Config::Context *context) {
    _context = context;
  }

private:
  DAO::DatasetDAO _datasetDAO;
  DAO::MemberDAO _memberDAO;
  DAO::PushDAO _pushDAO;
  Config::Context *_context;
};

} /* namespace Services */
} /* namespace Beehive */
