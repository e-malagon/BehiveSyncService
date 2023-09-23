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


#include <uuid.h>

#include <dao/Storage.hpp>
#include <nanolog/NanoLog.hpp>
#include <services/ServiceException.hpp>

namespace Beehive {
namespace Services {
namespace DAO {

rocksdb::TransactionDB *Storage::db;
std::vector<rocksdb::ColumnFamilyHandle *> Storage::handles;
std::unordered_map<std::string, rocksdb::ColumnFamilyHandle *> Storage::handlesMap;

const std::string databaseName = "/tmp/Beehive";
const std::string Storage::DefaultContext = "default";

void Storage::open() {
    std::vector<std::string> columnFamiliesNames;
    std::vector<rocksdb::ColumnFamilyDescriptor> columnFamilies;
    rocksdb::Options dbo;
    rocksdb::TransactionDBOptions tdbo;

    dbo.create_if_missing = true;
    if (!rocksdb::TransactionDB::ListColumnFamilies(dbo, databaseName, &columnFamiliesNames).ok()) {
        if (!rocksdb::TransactionDB::Open(dbo, tdbo, databaseName, &db).ok()) {
            LOG_ERROR << "Unable to open storage";
            exit(1);
        }
        delete db;
        if (!rocksdb::TransactionDB::ListColumnFamilies(dbo, databaseName, &columnFamiliesNames).ok()) {
            LOG_ERROR << "Unable to get column families";
            exit(1);
        }
    }
    for (std::string name : columnFamiliesNames)
        columnFamilies.push_back(rocksdb::ColumnFamilyDescriptor(name, rocksdb::ColumnFamilyOptions()));
    if (!rocksdb::TransactionDB::Open(dbo, tdbo, databaseName, columnFamilies, &handles, &db).ok()) {
        LOG_ERROR << "Unable to open storage";
        exit(1);
    }
    for (rocksdb::ColumnFamilyHandle *handle : handles)
        handlesMap.emplace(handle->GetName(), handle);
}

void Storage::createContext(const std::string &uuid) {
    rocksdb::ColumnFamilyHandle *handle;
    auto handlePtr = handlesMap.find(uuid);
    if (handlePtr != handlesMap.end())
        throw AlreadyExistsException("Context with uuid: " + uuid + " already exists.");
    rocksdb::Status status = db->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), uuid, &handle);
    if (status.ok())
        handlesMap.emplace(handle->GetName(), handle);
    else
        throw StorageErrorException(status.getState());
}

void Storage::deleteContext(const std::string &uuid) {
    auto handlePtr = handlesMap.find(uuid);
    if (handlePtr == handlesMap.end())
        throw NotExistsException("Context doesn't exist");
    if (db->DropColumnFamily(handlePtr->second).ok())
        handlesMap.erase(handlePtr);
}

std::vector<std::string> Storage::getContexts() {
    std::vector<std::string> contexts;
    for (auto handler : handlesMap)
        if(handler.first != "default")
            contexts.push_back(handler.first);
    return contexts;
}

void Storage::putValue(const std::string &key, const std::string &value, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        if (!db->Put(rocksdb::WriteOptions(), handlePtr->second, key, value).ok()) {
            LOG_ERROR << "Unable to save data";
            throw StorageException("Error to save data into " + context, 0);
        }
    } else
        throw StorageException("Context" + context + " doesn't exist.", 0);
}

bool Storage::getValue(const std::string &key, std::string *value, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        if (db->Get(rocksdb::ReadOptions(), handlePtr->second, key, value).ok())
            return true;
    }
    return false;
}

bool Storage::getValues(const std::string &key, std::vector<std::pair<std::string, std::string>> &values, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        rocksdb::Iterator *it = db->NewIterator(rocksdb::ReadOptions(), handlePtr->second);
        for (it->Seek(key); it->Valid() && it->key().starts_with(key); it->Next())
            values.push_back(std::make_pair<std::string, std::string>(it->key().ToString(), it->value().ToString()));
        if (!it->status().ok()) {
            LOG_ERROR << "Error while retrieving data from: " << it->status().ToString();
            throw StorageException("Error while retrieving data from " + context, 0);
        }
        return true;
    }
    return false;
}

bool Storage::deleteValue(const std::string &key, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        if (db->Delete(rocksdb::WriteOptions(), handlePtr->second, key).ok())
            return true;
    }
    return false;
}

Storage::Transaction Storage::begin() {
    return Transaction(db->BeginTransaction(rocksdb::WriteOptions()));
}

void Storage::Transaction::putValue(const std::string &key, const std::string &value, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        if (!_transaction->Put(handlePtr->second, key, value).ok()) {
            LOG_ERROR << "Unable to save data";
            throw StorageException("Error to save data into " + context, 0);
        }
    } else
        throw StorageException("Context" + context + " doesn't exist.", 0);
}

bool Storage::Transaction::getValue(const std::string &key, std::string *value, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        if (_transaction->GetForUpdate(rocksdb::ReadOptions(), handlePtr->second, key, value).ok())
            return true;
    }
    return false;
}

bool Storage::Transaction::deleteValue(const std::string &key, const std::string &context) {
    auto handlePtr = handlesMap.find(context);
    if (handlePtr != handlesMap.end()) {
        if (_transaction->Delete(handlePtr->second, key).ok())
            return true;
    }
    return false;
}

void Storage::close() {
    rocksdb::Status status;
    for (auto handle : handles) {
        status = db->DestroyColumnFamilyHandle(handle);
        if (!status.ok())
            LOG_ERROR << "Unable to close column family";
    }
    delete db;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
