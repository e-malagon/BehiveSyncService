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


#include <services/DatasetService.hpp>

#include <services/ServiceException.hpp>
#include <dao/UserDAO.hpp>
#include <uuid/uuid.h>
#include <json/json.hpp>
#include <nanolog/NanoLog.hpp>
#include <random>
#include <crypto/base64.h>

namespace Beehive {
namespace Services {

std::vector<Entities::Member> DatasetService::readMembers(Entities::Node &node, uint32_t idDataset, const std::string &context) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Role '" + member->role() + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (rolePtr->second.readmembers)
    return _memberDAO.readByDataset(idDataset, rolePtr->second.reademail, context);
  else
    return std::vector<Entities::Member>();
}

std::vector<Entities::Push> DatasetService::readPush(Entities::Node &node, uint32_t idDataset, const std::string &context) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Role '" + member->role() + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (rolePtr->second.manageshare)
    return _pushDAO.readByDataset(idDataset, context);
  else
    return std::vector<Entities::Push>();
}

bool DatasetService::isPutAllowedForRole(Entities::Node &node, uint32_t idDataset, const std::string &context) {
  std::unique_ptr<Entities::Member> member = _memberDAO.read(idDataset, node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Role '" + member->role() + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (!(rolePtr->second.sharedataset))
    throw NotEnoughRightsException("Member not allowed to share the data set");
  return true;
}

Entities::Dataset DatasetService::addDataset(Entities::User &user, std::string uuid, const std::string &context) {
  Entities::Dataset dataset;
  dataset.uuid(uuid);
  dataset.idHeader(0);
  dataset.owner(user.uuid());
  dataset.status(1);
  _datasetDAO.save(dataset, context);
  Entities::Member member;
  member.idDataset(dataset.id());
  member.idUser(user.uuid());
  member.role(_context->defaultrole);
  member.name(user.name());
  member.status(1);
  _memberDAO.save(member, context);
  return dataset;
}

std::vector<Entities::Dataset> DatasetService::readDatasets(Entities::User &user, const std::string &context) {
  return _datasetDAO.readByUser(user.uuid(), context);
}

void DatasetService::removeDataset(Entities::User &user, std::string uuidDataset, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  if (dataset && dataset->owner() == user.uuid()) {
    _memberDAO.removeDataset(dataset->id(), context);
    _datasetDAO.remove(dataset->id(), context);
  } else {
    throw NotEnoughRightsException("Not enough rights to remove the data set");
  }
}

Entities::Push DatasetService::pushDataset(Entities::Node &node, std::string uuidDataset, std::string &role, uint64_t until, uint32_t number, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Schema error when validating the role '" + member->role() + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (!(rolePtr->second.sharedataset))
    throw NotEnoughRightsException("Member not allowed to share the data set");
  auto roleMappingPtr = _context->rolesName2UUID.find(role);
  if (roleMappingPtr == _context->rolesName2UUID.end()) {
    std::string error = "Role '" + role + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  auto roleSharedPtr = _context->roles.find(roleMappingPtr->second);
  if (roleSharedPtr == _context->roles.end()) {
    std::string error = "Schema error when validating the role '" + role + "'";
    LOG_WARN << error;
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
  _pushDAO.save(push, context);
  return push;

}

std::unique_ptr<Entities::Dataset> DatasetService::popDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid, std::string &name, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Push> push = _pushDAO.read(dataset->id(), uuid, context);
  if (!push)
    throw NotExistsException("Push doesn't exist");
  if (push->until() < ((uint64_t) time(NULL)))
    throw NotExistsException("Trying to pop an expired push");
  auto rolePtr = _context->roles.find(push->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Role '" + push->role() + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().uuid(), context);
  if (member) {
    member->role(push->role());
    member->name(name.length() != 0 ? name : node.user().name());
    member->status(1);
    _memberDAO.update(*member, context);
  } else {
    Entities::Member member;
    member.idUser(node.user().uuid());
    member.idDataset(dataset->id());
    member.role(push->role());
    member.name(name.length() != 0 ? name : node.user().name());
    member.status(1);
    _memberDAO.save(member, context);
  }
  if (0 < push->number()) {
    if (1 < push->number()) {     
      push->number(push->number() - 1);
      _pushDAO.update(*push, context);
    } else {
      _pushDAO.remove(dataset->id(), uuid, context);
    }
  }
  return dataset;
}

void DatasetService::pullDataset(Entities::Node &node, std::string uuidDataset, std::string &uuid, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Schema error when validating the role '" + member->role() + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (!(rolePtr->second.manageshare))
    throw NotEnoughRightsException("Member not allowed to manage share");
  _pushDAO.remove(dataset->id(), uuid, context);
}

std::string DatasetService::putDataset(Entities::Node &node, std::string uuidDataset, std::string &email, std::string &name, std::string &role, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  isPutAllowedForRole(node, dataset->id(), context);
  std::string idUser = putDataset(node, *dataset, email, name, role, context);
  return idUser;
}

std::string DatasetService::putDataset(Entities::Node &node, Entities::Dataset &dataset, std::string &email, std::string &name, std::string &role, const std::string &context) {
  auto roleMappingPtr = _context->rolesName2UUID.find(role);
  if (roleMappingPtr == _context->rolesName2UUID.end()) {
    std::string error = "Role '" + role + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  auto roleSharedPtr = _context->roles.find(roleMappingPtr->second);
  if (roleSharedPtr == _context->roles.end()) {
    std::string error = "Role '" + role + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  DAO::Storage::Transaction transaction = DAO::Storage::begin();
  DAO::UserDAO userDAO;
  std::unique_ptr<Entities::User> newMemberUser = userDAO.read(email, _context->uuid);
  std::string uuidUser;
  if (!newMemberUser) {
    Entities::User memberUser;
    memberUser.identifier(email);
    memberUser.name(name);
    memberUser.type(Entities::User::Type::internal);
    userDAO.save(memberUser, _context->uuid, transaction);
    uuidUser = memberUser.uuid();
  } else {
    uuidUser = newMemberUser->uuid();
  }
  std::unique_ptr<Entities::Member> newMember = _memberDAO.read(dataset.id(), uuidUser, context);
  if (newMember) {
    newMember->role(roleMappingPtr->second);
    newMember->name(newMemberUser ? newMemberUser->name() : name);
    newMember->status(1);
    _memberDAO.update(*newMember, context);
  } else {
    Entities::Member member;
    member.idUser(uuidUser);
    member.idDataset(dataset.id());
    member.role(roleMappingPtr->second);
    member.name(newMemberUser ? newMemberUser->name() : name);
    member.status(1);
    _memberDAO.save(member, context);
  }
  transaction.commit();
  return uuidUser;
}

void DatasetService::leaveDataset(Entities::Node &node, std::string uuidDataset, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotExistsException("User is not member of this data set");
  member->status(0);
  _memberDAO.update(*member, context);
}

void DatasetService::updateMember(Entities::Node &node, std::string uuidDataset, std::string uuidUser, std::string &role, std::string &name, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Role '" + member->role() + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (!(rolePtr->second.managemembers))
    throw NotEnoughRightsException("Member not allowed to manage members");
  auto roleMappingPtr = _context->rolesName2UUID.find(role);
  if (roleMappingPtr == _context->rolesName2UUID.end()) {
    std::string error = "Role '" + role + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  auto roleSharedPtr = _context->roles.find(roleMappingPtr->second);
  if (roleSharedPtr == _context->roles.end()) {
    std::string error = "Role '" + role + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  member = _memberDAO.read(dataset->id(), uuidUser, context);
  if (!member || member->status() == 0)
    throw NotExistsException("User is not member of the data set");
  member->name(name);
  member->role(uuidUser == node.user().uuid() ? member->role() : roleMappingPtr->second);
  _memberDAO.update(*member, context);
}

void DatasetService::removeMember(Entities::Node &node, std::string uuidDataset, std::string uuidUser, const std::string &context) {
  std::unique_ptr<Entities::Dataset> dataset = _datasetDAO.read(uuidDataset, context);
  if (!dataset)
    throw NotEnoughRightsException("Data set doesn't exist");
  std::unique_ptr<Entities::Member> member = _memberDAO.read(dataset->id(), node.user().uuid(), context);
  if (!member || member->status() == 0)
    throw NotEnoughRightsException("User is not member of the data set");
  auto rolePtr = _context->roles.find(member->role());
  if (rolePtr == _context->roles.end()) {
    std::string error = "Role '" + member->role() + "' not defined on context '" + _context->name + "'";
    LOG_WARN << error;
    throw InvalidSchemaException(error);
  }
  if (!(rolePtr->second.managemembers))
    throw NotEnoughRightsException("Member not allowed to manage members");
  member = _memberDAO.read(dataset->id(), uuidUser, context);
  if (!member)
    throw NotExistsException("User is not member of the data set");
  member->status(0);
  _memberDAO.update(*member, context);
}

} /* namespace Services */
} /* namespace Beehive */
