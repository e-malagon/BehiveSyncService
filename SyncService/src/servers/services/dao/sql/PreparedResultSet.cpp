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

#include "PreparedResultSet.h"
#include "SQLException.h"

#include <uuid/uuid.h>
#include <utility>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

PreparedResultSet::PreparedResultSet(MYSQL_STMT *stmt, MYSQL_RES *result, MYSQL_BIND *resultBinds, std::unordered_map<std::string, int> &columns, unsigned int columnCount) :
    ResultSet(), stmt(stmt), result(result), resultBinds(resultBinds), columns(columns), columnCount(columnCount) {
  if (0 < columnCount) {
    for (unsigned int i = 0; i < columnCount; i++) {
      if (resultBinds[i].buffer_length < result->fields[i].max_length) {
        if (resultBinds[i].buffer != 0) {
          delete[] (char*) resultBinds[i].buffer;
        }
        resultBinds[i].buffer = new char[result->fields[i].max_length];
        resultBinds[i].buffer_length = result->fields[i].max_length;
      }
    }
    mysql_stmt_bind_result(stmt, resultBinds);
  }
}

PreparedResultSet::~PreparedResultSet() {
  mysql_free_result(result);
}

bool PreparedResultSet::next() {
  int status = mysql_stmt_fetch(stmt);
  return status != 1 && status != MYSQL_NO_DATA;
}

int PreparedResultSet::getInt(const int column) const {
  if (resultBinds[column].buffer_type != MYSQL_TYPE_LONG && resultBinds[column].buffer_type != MYSQL_TYPE_SHORT && resultBinds[column].buffer_type != MYSQL_TYPE_INT24 && resultBinds[column].buffer_type != MYSQL_TYPE_TINY) {
    throw SQLException("Invalid data type.", 0);
  }
  if (*resultBinds[column].is_null) {
    throw NullColumnException("Null returned from database.");
  }
  if (resultBinds[column].buffer_type == MYSQL_TYPE_TINY) {
    return ((char*) resultBinds[column].buffer)[0];
  } else if (resultBinds[column].buffer_type == MYSQL_TYPE_SHORT) {
    return ((short*) resultBinds[column].buffer)[0];
  } else {
    return ((int*) resultBinds[column].buffer)[0];
  }
}

int PreparedResultSet::getInt(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getInt(it->second);
}

long PreparedResultSet::getLong(const int column) const {
  if (resultBinds[column].buffer_type != MYSQL_TYPE_LONGLONG) {
    throw SQLException("Invalid data type.", 0);
  }
  if (*resultBinds[column].is_null) {
    throw NullColumnException("Null returned from database.");
  }
  return ((long*) resultBinds[column].buffer)[0];
}

long PreparedResultSet::getLong(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getLong(it->second);
}

std::string PreparedResultSet::getString(const int column) const {
  if (resultBinds[column].buffer_type != MYSQL_TYPE_STRING && resultBinds[column].buffer_type != MYSQL_TYPE_VAR_STRING && resultBinds[column].buffer_type != MYSQL_TYPE_VARCHAR) {
    throw SQLException("Invalid data type.", 0);
  }
  if (*resultBinds[column].is_null) {
    throw NullColumnException("Null returned from database.");
  }
  return std::string(reinterpret_cast<char const*>(resultBinds[column].buffer), *resultBinds[column].length);
}

std::string PreparedResultSet::getString(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getString(it->second);
}

std::string PreparedResultSet::getUUID(const int column) const {
  if (resultBinds[column].buffer_type != MYSQL_TYPE_STRING && resultBinds[column].buffer_type != MYSQL_TYPE_VAR_STRING && resultBinds[column].buffer_type != MYSQL_TYPE_VARCHAR) {
    throw SQLException("Invalid data type.", 0);
  }
  if (*resultBinds[column].is_null) {
    throw NullColumnException("Null returned from database.");
  }
  char plain[37];
  uuid_unparse_lower(reinterpret_cast<unsigned char const*>(resultBinds[column].buffer), plain);
  return std::string(plain, 36);
}

std::string PreparedResultSet::getUUID(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getUUID(it->second);
}

std::string PreparedResultSet::getBlob(const int column) const {
  if (resultBinds[column].buffer_type != MYSQL_TYPE_BLOB && resultBinds[column].buffer_type != MYSQL_TYPE_LONG_BLOB && resultBinds[column].buffer_type != MYSQL_TYPE_MEDIUM_BLOB && resultBinds[column].buffer_type != MYSQL_TYPE_TINY_BLOB) {
    throw SQLException("Invalid data type.", 0);
  }
  if (*resultBinds[column].is_null) {
    throw NullColumnException("Null returned from database.");
  }
  return std::string(reinterpret_cast<char const*>(resultBinds[column].buffer), *resultBinds[column].length);
}

std::string PreparedResultSet::getBlob(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getBlob(it->second);
}

bool PreparedResultSet::isNull(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return *resultBinds[it->second].is_null;
}

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
