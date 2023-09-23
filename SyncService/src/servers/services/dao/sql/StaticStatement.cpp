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

#include "StaticStatement.h"

#include "StaticResultSet.h"
#include "SQLException.h"

#include <uuid/uuid.h>
#include <stddef.h>
#include <sys/types.h>
#include <sstream>
#include <algorithm>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

StaticStatement::StaticStatement(MYSQL *mysql, const std::string &statement) :
    Statement(), statement(statement) {
  this->MySQLConnection = mysql;
  paramCount = count(statement.begin(), statement.end(), '?');
  split(tokens, statement, '?');
}

StaticStatement::~StaticStatement() {
}

int StaticStatement::executeUpdate() {
  if (mysql_query(MySQLConnection, getStatement().c_str()) != 0) {
    throw SQLException(mysql_error(MySQLConnection), mysql_errno(MySQLConnection));
  }
  return mysql_affected_rows(MySQLConnection);
}

ResultSet* StaticStatement::executeQuery() {
  if (mysql_query(MySQLConnection, getStatement().c_str()) != 0) {
    throw SQLException(mysql_error(MySQLConnection), mysql_errno(MySQLConnection));
  }
  return new StaticResultSet(MySQLConnection);
}

int StaticStatement::getLastId() {
  return mysql_insert_id(MySQLConnection);
}

void StaticStatement::setInt(unsigned int parameterIndex, int value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  numbers[parameterIndex] = value;
  types[parameterIndex] = NUMBER;
}

void StaticStatement::setLong(unsigned int parameterIndex, long value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  numbers[parameterIndex] = value;
  types[parameterIndex] = NUMBER;
}

void StaticStatement::setString(unsigned int parameterIndex, const std::string &value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  ulong bytes = value.length();
  char valueEsc[(2 * bytes) + 1];
  mysql_real_escape_string(MySQLConnection, valueEsc, value.c_str(), bytes);
  strings[parameterIndex] = valueEsc;
  types[parameterIndex] = STRING;
}

void StaticStatement::setString(unsigned int parameterIndex, uint8_t *value, const uint16_t len) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  char valueEsc[(2 * len) + 1];
  mysql_real_escape_string(MySQLConnection, valueEsc, (const char*) value, len);
  strings[parameterIndex] = valueEsc;
  types[parameterIndex] = STRING;
}

void StaticStatement::setUUID(unsigned int parameterIndex, const std::string &value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  uuid_t uuid;
  if (uuid_parse(value.c_str(), uuid) != 0) {
    throw SQLException("Invalid uuid data type.", 0);
  }
  strings[parameterIndex] = std::string((char*) uuid, 16);
  types[parameterIndex] = STRING;
}

void StaticStatement::setBlob(unsigned int parameterIndex, const std::string &value) {
  if (parameterIndex < 1 || paramCount < parameterIndex) {
    throw OutOfBoundsException("Invalid parameter index.");
  }
  ulong bytes = value.length();
  char valueEsc[(2 * bytes) + 1];
  mysql_real_escape_string(MySQLConnection, valueEsc, value.c_str(), bytes);
  strings[parameterIndex] = valueEsc;
  types[parameterIndex] = STRING;
}

void StaticStatement::split(std::vector<std::string> &tokens, const std::string &text, char delimiter) {
  size_t start = 0U;
  size_t end = text.find(delimiter);
  while (end != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
    end = text.find(delimiter, start);
  }
  tokens.push_back(text.substr(start, end - start));
}

std::string StaticStatement::getStatement() {
  std::stringstream ss;
  unsigned int parameter = 1;
  for (std::string &token : tokens) {
    ss << token;
    if (parameter <= paramCount) {
      if (types.count(parameter) != 0) {
        switch (types.at(parameter)) {
        case NUMBER:
          ss << numbers[parameter];
          parameter++;
          break;
        case STRING:
          ss << "'" << strings[parameter] << "'";
          parameter++;
          break;
        }
      } else {
        throw MissingParameterException("Parameter is missing.");
      }
    }
  }
  return ss.str();
}

void StaticStatement::clear() {
  numbers.clear();
  strings.clear();
  types.clear();
}

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
