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

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>

#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

namespace Beehive {
namespace Services {
namespace DAO {

class StorageException : public std::runtime_error {
   public:
    StorageException(const StorageException &e) : StorageException(e.what(), e._errorCode) {
    }

    StorageException(const std::string &what, unsigned errorCode) : runtime_error(what), _errorCode(errorCode), _what(what) {
    }

    virtual ~StorageException() throw() {
    }

    unsigned errorCode() const {
        return _errorCode;
    }

    virtual const char *what() const noexcept override {
        return _what.c_str();
    }

   private:
    unsigned _errorCode;
    const std::string _what;
};

class Storage {
   public:
    Storage() {
    }

    class Transaction;

    static void open();
    static void close();

    static void createContext(const std::string &uuid);
    static void deleteContext(const std::string &uuid);
    static std::vector<std::string> getContexts();

    static void putValue(const std::string &key, const std::string &value, const std::string &context);
    static bool getValue(const std::string &key, std::string *value, const std::string &context);
    static bool getValues(const std::string &key, std::vector<std::pair<std::string, std::string>> &values, const std::string &context);
    static bool deleteValue(const std::string &key, const std::string &context);

    static Transaction begin();

    static const std::string DefaultContext;

    class Transaction {
       public:
        Transaction(rocksdb::Transaction *transaction) : _transaction(transaction), finished(false) {
        }
        ~Transaction() {
            if (_transaction) {
                if(!finished)
                    _transaction->Rollback();
                delete _transaction;
            }
        }

        void putValue(const std::string &key, const std::string &value, const std::string &context);
        bool getValue(const std::string &key, std::string *value, const std::string &context);
        bool deleteValue(const std::string &key, const std::string &context);

        void commit() {
            if (_transaction->Commit().ok())
                finished = true;
            else
                throw StorageException("Unable to commit a transaction.", 0);
        }

        void rollback() {
            if (_transaction->Rollback().ok())
                finished = true;
            else
                throw StorageException("Unable to rollback a transaction.", 0);
        }

       private:
        rocksdb::Transaction *_transaction;
        bool finished;
    };

   private:
    static rocksdb::TransactionDB* db;
    static std::vector<rocksdb::ColumnFamilyHandle*> handles;
    static std::unordered_map<std::string, rocksdb::ColumnFamilyHandle*> handlesMap;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Beehive */
