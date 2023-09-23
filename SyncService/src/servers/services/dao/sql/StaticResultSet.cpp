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

#include "StaticResultSet.h"

#include "SQLException.h"

#include <uuid/uuid.h>
#include <cstdlib>
#include <utility>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

StaticResultSet::StaticResultSet(MYSQL *mysql) :
    ResultSet(), mysql(mysql) {
  if ((result = mysql_store_result(mysql)) == NULL) {
    throw SQLException(mysql_error(mysql), mysql_errno(mysql));
  }
  columnCount = mysql_num_fields(result);
  if (0 < columnCount) {
    for (unsigned int i = 0; i < columnCount; i++) {
      columns[result->fields[i].name] = i;
    }
  }
  row = NULL;
  lengths = NULL;
}

StaticResultSet::~StaticResultSet() {
  mysql_free_result(result);
}

bool StaticResultSet::next() {
  row = mysql_fetch_row(result);
  lengths = mysql_fetch_lengths(result);
  return row;
}

int StaticResultSet::getInt(const int column) const {
  if (result->fields[column].type != MYSQL_TYPE_LONG && result->fields[column].type != MYSQL_TYPE_SHORT && result->fields[column].type != MYSQL_TYPE_INT24 && result->fields[column].type != MYSQL_TYPE_TINY) {
    throw SQLException("Invalid data type.", 0);
  }
  if (!row[column]) {
    throw NullColumnException("Null returned from database.");
  }
  return std::stoi(row[column]);
}

int StaticResultSet::getInt(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getInt(it->second);
}

long StaticResultSet::getLong(const int column) const {
  if (result->fields[column].type != MYSQL_TYPE_LONGLONG) {
    throw SQLException("Invalid data type.", 0);
  }
  if (!row[column]) {
    throw NullColumnException("Null returned from database.");
  }
  return std::stol(row[column]);
}

long StaticResultSet::getLong(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getLong(it->second);
}

std::string StaticResultSet::getString(const int column) const {
  if (result->fields[column].type != MYSQL_TYPE_STRING && result->fields[column].type != MYSQL_TYPE_VAR_STRING && result->fields[column].type != MYSQL_TYPE_VARCHAR) {
    throw SQLException("Invalid data type.", 0);
  }
  if (!row[column]) {
    throw NullColumnException("Null returned from database.");
  }
  return std::string(reinterpret_cast<char const*>(row[column]), lengths[column]);
}

std::string StaticResultSet::getString(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getString(it->second);
}

std::string StaticResultSet::getUUID(const int column) const {
  if (result->fields[column].type != MYSQL_TYPE_STRING && result->fields[column].type != MYSQL_TYPE_VAR_STRING && result->fields[column].type != MYSQL_TYPE_VARCHAR) {
    throw SQLException("Invalid data type.", 0);
  }
  if (!row[column]) {
    throw NullColumnException("Null returned from database.");
  }
  char plain[37];
  uuid_unparse_lower(reinterpret_cast<unsigned char const*>(row[column]), plain);
  return std::string(plain, 36);
}

std::string StaticResultSet::getUUID(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getUUID(it->second);
}

std::string StaticResultSet::getBlob(const int column) const {
  if (result->fields[column].type != MYSQL_TYPE_BLOB && result->fields[column].type != MYSQL_TYPE_LONG_BLOB && result->fields[column].type != MYSQL_TYPE_MEDIUM_BLOB && result->fields[column].type != MYSQL_TYPE_TINY_BLOB) {
    throw SQLException("Invalid data type.", 0);
  }
  if (!row[column]) {
    throw NullColumnException("Null returned from database.");
  }
  return std::string(reinterpret_cast<char const*>(row[column]), lengths[column]);
}

std::string StaticResultSet::getBlob(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return getBlob(it->second);
}

bool StaticResultSet::isNull(const std::string &column) const {
  std::unordered_map<std::string, int>::const_iterator it = columns.find(column);
  if (it == columns.end()) {
    throw SQLException("Invalid column name [" + column + "].", 0);
  }
  return !row[it->second];
}

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
