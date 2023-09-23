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

#include "DatasetService.h"

#include <ServiceException.h>
#include <dao/UserDAO.h>
#include <uuid/uuid.h>
#include <json/json.hpp>
#include <nanolog/NanoLog.hpp>
#include <random>
#include <crypto/base64.h>

namespace SyncServer {
namespace Servers {
namespace Services {

std::vector<Entities::Member> DatasetService::readMembers(Entities::Node &node, uint32_t idDataset) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + member->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (*rolePtr->second.readmembers)
    return _memberDAO.readByDataset(idDataset, *rolePtr->second.reademail);
  else
    return std::vector<Entities::Member>();
}

std::vector<Entities::Push> DatasetService::readPush(Entities::Node &node, uint32_t idDataset) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + member->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (*rolePtr->second.manageshare)
    return _pushDAO.readByDataset(idDataset);
  else
    return std::vector<Entities::Push>();
}

bool DatasetService::isPutAllowedForRole(Entities::Node &node, uint32_t idDataset) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + member->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (!(*rolePtr->second.sharedataset))
    throw NotEnoughRightsException("Member not allowed to share the data set");
  return true;
}

Entities::Dataset DatasetService::addDataset(Entities::User &user, std::string uuid) {
  Entities::Dataset dataset;
  dataset.uuid(uuid);
  dataset.idHeader(0);
  dataset.owner(user.id());
  dataset.status(1);
  _datasetDAO.save(dataset);
  Entities::Member member;
  member.idDataset(dataset.id());
  member.idUser(user.id());
  member.role(_application->defaultrole);
  member.name(user.name());
  member.status(1);
  _memberDAO.save(member);
  return dataset;
}

std::vector<Entities::Dataset> DatasetService::readDatasets(Entities::User &user) {
  return _datasetDAO.readByUser(user.id());
}

void DatasetService::removeDataset(Entities::User &user, std::string uuidDataset) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  if (dataset && dataset->owner() == user.id()) {
    _memberDAO.removeDataset(dataset->id());
    _datasetDAO.remove(dataset->id());
    _connection->commit();
  } else {
    throw NotEnoughRightsException("Not enough rights to remove the data set");
  }
}

Entities::Push DatasetService::pushDataset(Entities::Node &node, std::string uuidDataset, std::string &role, uint64_t until, uint32_t number) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Schema error when validating the role '" + member->role() + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (!(*rolePtr->second.sharedataset))
    throw NotEnoughRightsException("Member not allowed to share the data set");
  auto roleMappingPtr = _application->rolesName2UUID[node.version()].find(role);
  if (roleMappingPtr == _application->rolesName2UUID[node.version()].end()) {
    std::string error = "Role '" + role + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  auto roleSharedPtr = _application->roles[_application->version].find(roleMappingPtr->second);
  if (roleSharedPtr == _application->roles[_application->version].end()) {
    std::string error = "Schema error when validating the role '" + role + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  Entities::Push push;
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0, 256);
  char rawKey[27];
  for (int i = 0; i < 27; ++i)
    rawKey[i] = dist(mt);
  push.idDataset(dataset->id());
  push.uuid(base64_encode((unsigned char const*) rawKey, 27));
  push.role(roleMappingPtr->second);
  push.until(until);
  push.number(number);
  _pushDAO.save(push);
  _connection->commit();
  return push;

}

std::unique_ptr<Entities::Dataset> DatasetService::popDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid, std::string &name) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Push> push = _pushDAO.read(dataset->id(), uuid);
  if (!push)
    throw NotExistException("Push doesn't exist");
  if (push->until() < ((uint64_t) time(NULL)))
    throw NotExistException("Trying to pop an expired push");
  auto rolePtr = _application->roles[_application->version].find(push->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + push->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().id());
  if (member) {
    member->role(push->role());
    member->name(name.length() != 0 ? name : node.user().name());
    member->status(1);
    _memberDAO.update(*member);
  } else {
    Entities::Member member;
    member.idUser(node.user().id());
    member.idDataset(dataset->id());
    member.role(push->role());
    member.name(name.length() != 0 ? name : node.user().name());
    member.status(1);
    _memberDAO.save(member);
  }
  if (0 < push->number()) {
    if (1 < push->number()) {
      push->number(push->number() - 1);
      _pushDAO.update(*push);
    } else {
      _pushDAO.remove(dataset->id(), uuid);
    }
  }
  _connection->commit();
  return dataset;
}

void DatasetService::pullDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Schema error when validating the role '" + member->role() + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (!(*rolePtr->second.manageshare))
    throw NotEnoughRightsException("Member not allowed to manage share");
  _pushDAO.remove(dataset->id(), uuid);
  _connection->commit();
}

uint32_t DatasetService::putDataset(Entities::Node &node, std::string uuidDataset, std::string &email, std::string &name, std::string &role) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  isPutAllowedForRole(node, dataset->id());
  uint32_t idUser = putDataset(node, *dataset, email, name, role);
  _connection->commit();
  return idUser;
}

uint32_t DatasetService::putDataset(Entities::Node &node, Entities::Dataset &dataset, std::string &email, std::string &name, std::string &role) {
  auto roleMappingPtr = _application->rolesName2UUID[node.version()].find(role);
  if (roleMappingPtr == _application->rolesName2UUID[node.version()].end()) {
    std::string error = "Role '" + role + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  auto roleSharedPtr = _application->roles[_application->version].find(roleMappingPtr->second);
  if (roleSharedPtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + role + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  DAO::UserDAO userDAO(_connection);
  std::unique_ptr<Entities::User> newMemberUser = userDAO.read(email);
  int idUser;
  if (!newMemberUser) {
    Entities::User memberUser;
    memberUser.identifier(email);
    memberUser.name(name);
    memberUser.kind(1);
    memberUser.identity("");
    userDAO.save(memberUser);
    idUser = memberUser.id();
  } else {
    idUser = newMemberUser->id();
  }
  std::unique_ptr<Entities::Member> newMember = _memberDAO.read(dataset.id(), idUser);
  if (newMember) {
    newMember->role(roleMappingPtr->second);
    newMember->name(newMemberUser ? newMemberUser->name() : name);
    newMember->status(1);
    _memberDAO.update(*newMember);
  } else {
    Entities::Member member;
    member.idUser(idUser);
    member.idDataset(dataset.id());
    member.role(roleMappingPtr->second);
    member.name(newMemberUser ? newMemberUser->name() : name);
    member.status(1);
    _memberDAO.save(member);
  }
  return idUser;
}

void DatasetService::leaveDataset(Entities::Node &node, std::string uuidDataset) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().id());
  if (!member || member->status() == 0)
    throw NotExistException("User is not member of this data set");
  member->status(0);
  _memberDAO.update(*member);
  _connection->commit();
}

void DatasetService::updateMember(Entities::Node &node, std::string uuidDataset, uint32_t idUser, std::string &role, std::string &name) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + member->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (!(*rolePtr->second.managemembers))
    throw NotEnoughRightsException("Member not allowed to manage members");
  auto roleMappingPtr = _application->rolesName2UUID[node.version()].find(role);
  if (roleMappingPtr == _application->rolesName2UUID[node.version()].end()) {
    std::string error = "Role '" + role + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  auto roleSharedPtr = _application->roles[_application->version].find(roleMappingPtr->second);
  if (roleSharedPtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + role + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  member = _memberDAO.read(dataset->id(), idUser);
  if (!member || member->status() == 0)
    throw NotExistException("User is not member of the data set");
  member->name(name);
  member->role(idUser == node.user().id() ? member->role() : roleMappingPtr->second);
  _memberDAO.update(*member);
  _connection->commit();
}

void DatasetService::removeMember(Entities::Node &node, std::string uuidDataset, uint32_t idUser) {
  _connection->commit();
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().id());
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _application->roles[_application->version].find(member->role());
  if (rolePtr == _application->roles[_application->version].end()) {
    std::string error = "Role '" + member->role() + "' not defined on application '" + _application->name + "'";
    LOG_FILE_WARN(_beehive) << error;
    throw InvalidSchemaException(error);
  }
  if (!(*rolePtr->second.managemembers))
    throw NotEnoughRightsException("Member not allowed to manage members");
  member = _memberDAO.read(dataset->id(), idUser);
  if (!member)
    throw NotExistException("User is not member of the data set");
  member->status(0);
  _memberDAO.update(*member);
  _connection->commit();
}

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
