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

#include "PreparedStatement.h"

#include "PreparedResultSet.h"
#include "SQLException.h"

#include <uuid/uuid.h>
#include <string.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

PreparedStatement::PreparedStatement(MYSQL *mysql, const std::string &statement) :
    Statement(), statement(statement) {
  this->MySQLConnection = mysql;
  stmt = mysql_stmt_init(MySQLConnection);
  if (stmt == NULL) {
    throw SQLException("Out of memory.", 0);
  }
  if (mysql_stmt_prepare(stmt, statement.c_str(), statement.length()) != 0) {
    throw SQLException(mysql_error(MySQLConnection), mysql_errno(MySQLConnection));
  }
  paramCount = mysql_stmt_param_count(stmt);
  if (0 < paramCount) {
    paramBinds = new MYSQL_BIND[paramCount];
    memset(paramBinds, 0, sizeof(MYSQL_BIND) * paramCount);
  }
  resultLengths = 0;
  resultBinds = 0;
  columnCount = 0;
  nulls = 0;
}

PreparedStatement::~PreparedStatement() {
  if (0 < paramCount) {
    for (unsigned int i = 0; i < paramCount; i++) {
      if (paramBinds[i].buffer != 0) {
        delete[] (char*) paramBinds[i].buffer;
      }
      if (paramBinds[i].length != 0) {
        delete[] paramBinds[i].length;
      }
    }
    delete[] paramBinds;
  }
  if (0 < columnCount) {
    delete[] nulls;
    delete[] resultLengths;
    for (unsigned int i = 0; i < columnCount; i++) {
      if (resultBinds[i].buffer != 0) {
        delete[] (char*) resultBinds[i].buffer;
        resultBinds[i].buffer = 0;
      }
    }
    delete[] resultBinds;
  }
  mysql_stmt_free_result(stmt);
  mysql_stmt_close(stmt);
}

int PreparedStatement::executeUpdate() {
  if (mysql_stmt_bind_param(stmt, paramBinds) != 0) {
    throw SQLException(mysql_stmt_error(stmt), 0);
  }
  unsigned long cursor = CURSOR_TYPE_NO_CURSOR;
  mysql_stmt_attr_set(stmt, STMT_ATTR_CURSOR_TYPE, &cursor);

  if (mysql_stmt_execute(stmt) != 0) {
    throw SQLException(mysql_error(MySQLConnection), mysql_errno(MySQLConnection));
  }
  return mysql_affected_rows(MySQLConnection);
}

ResultSet* PreparedStatement::executeQuery() {
  if (mysql_stmt_bind_param(stmt, paramBinds) != 0) {
    throw SQLException(mysql_stmt_error(stmt), 0);
  }

  my_bool setMaxLen = 1;
  mysql_stmt_attr_set(stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &setMaxLen);

  if (mysql_stmt_execute(stmt) != 0) {
    throw SQLException(mysql_error(MySQLConnection), mysql_errno(MySQLConnection));
  }

  if (mysql_stmt_store_result(stmt)) {
    throw SQLException(mysql_stmt_error(stmt), 0);
  }

  MYSQL_RES *result;
  if ((result = mysql_stmt_result_metadata(stmt)) == NULL) {
    throw SQLException(mysql_stmt_error(stmt), 0);
  }
  if (resultBinds == 0) {
    columnCount = mysql_num_fields(result);
    if (0 < columnCount) {
      resultBinds = new MYSQL_BIND[columnCount];
      memset(resultBinds, 0, sizeof(MYSQL_BIND) * columnCount);
      resultLengths = new unsigned long[columnCount];
      memset(resultLengths, 0, sizeof(unsigned long) * columnCount);
      nulls = new my_bool[columnCount];
      memset(nulls, 0, sizeof(my_bool) * columnCount);
      for (unsigned int i = 0; i < columnCount; i++) {
        columns[result->fields[i].name] = i;
        resultBinds[i].buffer_type = result->fields[i].type;
        resultBinds[i].is_null = 0;
        resultBinds[i].buffer = new char[result->fields[i].max_length];
        resultBinds[i].buffer_length = result->fields[i].max_length;
        resultBinds[i].length = resultLengths + i;
        resultBinds[i].is_null = nulls + i;
      }
      mysql_stmt_bind_result(stmt, resultBinds);
    } else {
      memset(resultLengths, 0, sizeof(unsigned long) * columnCount);
      memset(nulls, 0, sizeof(my_bool) * columnCount);
    }
  }
  return new PreparedResultSet(stmt, result, resultBinds, columns, columnCount);
}

int PreparedStatement::getLastId() {
  return mysql_insert_id(MySQLConnection);
}

void PreparedStatement::setInt(unsigned int parameterIndex, int value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  if (paramBinds[parameterIndex - 1].buffer != 0 && paramBinds[parameterIndex - 1].buffer_length < sizeof(int)) {
    delete[] (char*) paramBinds[parameterIndex - 1].buffer;
    paramBinds[parameterIndex - 1].buffer = 0;
    paramBinds[parameterIndex - 1].buffer_length = 0;
  }
  if (paramBinds[parameterIndex - 1].buffer == 0) {
    paramBinds[parameterIndex - 1].buffer = new int[1];
    paramBinds[parameterIndex - 1].buffer_length = sizeof(int[1]);
  }
  paramBinds[parameterIndex - 1].buffer_type = MYSQL_TYPE_LONG;
  ((int*) paramBinds[parameterIndex - 1].buffer)[0] = value;
  paramBinds[parameterIndex - 1].is_null = 0;
}

void PreparedStatement::setLong(unsigned int parameterIndex, long value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  if (paramBinds[parameterIndex - 1].buffer != 0 && paramBinds[parameterIndex - 1].buffer_length < sizeof(long)) {
    delete[] (char*) paramBinds[parameterIndex - 1].buffer;
    paramBinds[parameterIndex - 1].buffer = 0;
    paramBinds[parameterIndex - 1].buffer_length = 0;
  }
  if (paramBinds[parameterIndex - 1].buffer == 0) {
    paramBinds[parameterIndex - 1].buffer = new long[1];
    paramBinds[parameterIndex - 1].buffer_length = sizeof(long[1]);
  }
  paramBinds[parameterIndex - 1].buffer_type = MYSQL_TYPE_LONGLONG;
  ((long*) paramBinds[parameterIndex - 1].buffer)[0] = value;
  paramBinds[parameterIndex - 1].is_null = 0;
}

void PreparedStatement::setString(unsigned int parameterIndex, const std::string &value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  if (paramBinds[parameterIndex - 1].buffer != 0 && paramBinds[parameterIndex - 1].buffer_length < value.size()) {
    delete[] (char*) paramBinds[parameterIndex - 1].buffer;
    paramBinds[parameterIndex - 1].buffer = 0;
    paramBinds[parameterIndex - 1].buffer_length = 0;
  }
  if (paramBinds[parameterIndex - 1].buffer == 0) {
    paramBinds[parameterIndex - 1].buffer = new char[value.size()];
    paramBinds[parameterIndex - 1].buffer_length = value.size();
  }
  if (paramBinds[parameterIndex - 1].length == 0) {
    paramBinds[parameterIndex - 1].length = new unsigned long[1];
  }
  paramBinds[parameterIndex - 1].buffer_type = MYSQL_TYPE_STRING;
  memcpy(paramBinds[parameterIndex - 1].buffer, value.data(), value.size());
  paramBinds[parameterIndex - 1].is_null = 0;
  paramBinds[parameterIndex - 1].length[0] = value.size();
}

void PreparedStatement::setString(unsigned int parameterIndex, uint8_t *value, const uint16_t len) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  if (paramBinds[parameterIndex - 1].length == 0) {
    paramBinds[parameterIndex - 1].length = new unsigned long[1];
  }
  paramBinds[parameterIndex - 1].buffer_type = MYSQL_TYPE_STRING;
  paramBinds[parameterIndex - 1].buffer = value;
  paramBinds[parameterIndex - 1].is_null = 0;
  paramBinds[parameterIndex - 1].length[0] = len;
}

void PreparedStatement::setUUID(unsigned int parameterIndex, const std::string &value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  if (paramBinds[parameterIndex - 1].buffer != 0 && paramBinds[parameterIndex - 1].buffer_length < 16) {
    delete[] (char*) paramBinds[parameterIndex - 1].buffer;
    paramBinds[parameterIndex - 1].buffer = 0;
    paramBinds[parameterIndex - 1].buffer_length = 0;
  }
  if (paramBinds[parameterIndex - 1].buffer == 0) {
    paramBinds[parameterIndex - 1].buffer = new char[16];
    paramBinds[parameterIndex - 1].buffer_length = 16;
  }
  if (paramBinds[parameterIndex - 1].length == 0) {
    paramBinds[parameterIndex - 1].length = new unsigned long[1];
  }
  uuid_t uuid;
  if (uuid_parse(value.c_str(), uuid) != 0) {
    throw SQLException("Invalid uuid data type.", 0);
  }
  paramBinds[parameterIndex - 1].buffer_type = MYSQL_TYPE_STRING;
  memcpy(paramBinds[parameterIndex - 1].buffer, uuid, 16);
  paramBinds[parameterIndex - 1].is_null = 0;
  paramBinds[parameterIndex - 1].length[0] = 16;
}

void PreparedStatement::setBlob(unsigned int parameterIndex, const std::string &value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  if (paramBinds[parameterIndex - 1].buffer != 0 && paramBinds[parameterIndex - 1].buffer_length < value.size()) {
    delete[] (char*) paramBinds[parameterIndex - 1].buffer;
    paramBinds[parameterIndex - 1].buffer = 0;
    paramBinds[parameterIndex - 1].buffer_length = 0;
  }
  if (paramBinds[parameterIndex - 1].buffer == 0) {
    paramBinds[parameterIndex - 1].buffer = new char[value.size()];
    paramBinds[parameterIndex - 1].buffer_length = value.size();
  }
  if (paramBinds[parameterIndex - 1].length == 0) {
    paramBinds[parameterIndex - 1].length = new unsigned long[1];
  }
  paramBinds[parameterIndex - 1].buffer_type = MYSQL_TYPE_BLOB;
  memcpy(paramBinds[parameterIndex - 1].buffer, value.data(), value.size());
  paramBinds[parameterIndex - 1].is_null = 0;
  paramBinds[parameterIndex - 1].length[0] = value.size();
}

void PreparedStatement::clear() {
  for (unsigned int i = 0; i < paramCount; i++) {
    if (paramBinds[i].buffer != 0) {
      delete[] (char*) paramBinds[i].buffer;
      paramBinds[i].buffer = 0;
    }
    if (paramBinds[i].length != 0) {
      delete[] paramBinds[i].length;
      paramBinds[i].length = 0;
    }
  }
  mysql_stmt_free_result(stmt);
  mysql_stmt_reset(stmt);
}

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
