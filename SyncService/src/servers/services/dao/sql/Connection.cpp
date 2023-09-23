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

#include "Connection.h"

#include "PreparedStatement.h"
#include "SQLException.h"
#include "StaticStatement.h"

#include <utility>
#include <nanolog/NanoLog.hpp>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

std::unordered_map<uint32_t, size_t> Connection::_numberConnections;

Connection::Connection(const std::string &hostName, const int port, const std::string &user, const std::string &password, uint32_t beehive, const bool secure) {
  MYSQL *MySQLConRet;
  _MySQLConnection = mysql_init( NULL);
  mysql_options(_MySQLConnection, MYSQL_SET_CHARSET_NAME, "utf8");
  if (secure) {
    mysql_options(_MySQLConnection, MYSQL_OPT_SSL_CA, "/etc/syncserver/server-ca.pem");
    mysql_options(_MySQLConnection, MYSQL_OPT_SSL_CERT, "/etc/syncserver/client-cert.pem");
    mysql_options(_MySQLConnection, MYSQL_OPT_SSL_KEY, "/etc/syncserver/client-key.pem");
  }
  if (0 < beehive)
    MySQLConRet = mysql_real_connect(_MySQLConnection, hostName.c_str(), user.c_str(), password.c_str(), std::to_string(beehive).c_str(), port, NULL, 0);
  else
    MySQLConRet = mysql_real_connect(_MySQLConnection, hostName.c_str(), user.c_str(), password.c_str(), NULL, port, NULL, 0);
  if (MySQLConRet == NULL) {
    std::string error = std::string(mysql_error(_MySQLConnection)) + " [" + hostName + ":" + std::to_string(port) + "]";
    throw SQLException(error, mysql_errno(_MySQLConnection));
  }
  mysql_autocommit(_MySQLConnection, 0);
  if (secure && mysql_get_ssl_cipher(_MySQLConnection) == NULL) {
    mysql_close(_MySQLConnection);
    throw SQLException("SSL required.", 1000);
  }
  _beehive = beehive;
  auto nPtr = _numberConnections.find(beehive);
  if (nPtr == _numberConnections.end())
    _numberConnections[beehive] = 0;
  else
    nPtr->second++;
}

Connection::~Connection() {
  auto nPtr = _numberConnections.find(_beehive);
  if (nPtr == _numberConnections.end())
    _numberConnections[_beehive] = 0;
  else
    nPtr->second--;
  for (auto &statement : _statements) {
    delete statement.second;
  }
  mysql_close(_MySQLConnection);
}

void Connection::begin() const {

}

void Connection::commit() const {
  mysql_commit(_MySQLConnection);
}

void Connection::rollback() const {
  mysql_rollback(_MySQLConnection);
}

void Connection::switchBeehive(const std::string &beehive) {
  mysql_select_db(_MySQLConnection, beehive.c_str());
}

Statement* Connection::prepareStatement(const std::string &statement, bool cached, bool asStatic) {
  if (cached) {
    std::unordered_map<std::string, Statement*>::const_iterator stmtIt = _statements.find(statement);
    if (stmtIt == _statements.end()) {
      Statement *stmt;
      if (asStatic) {
        stmt = new StaticStatement(_MySQLConnection, statement);
      } else {
        stmt = new PreparedStatement(_MySQLConnection, statement);
      }
      _statements[statement] = stmt;
      return stmt;
    } else {
      stmtIt->second->clear();
      return stmtIt->second;
    }
  } else {
    Statement *stmt;
    if (asStatic) {
      stmt = new StaticStatement(_MySQLConnection, statement);
    } else {
      stmt = new PreparedStatement(_MySQLConnection, statement);
    }
    return stmt;
  }
}

void Connection::lock(std::string resource) {
  Statement *stmt;
  ResultSet *res;

  stmt = prepareStatement("SELECT GET_LOCK(?, 5) as lockresource; ");
  stmt->setString(1, resource);
  res = stmt->executeQuery();
  if (res->next()) {
    if (res->getLong("lockresource") != 1) {
      delete res;
      LOG_WARN << "Resource is locked ";
      throw SQLException("Resource is locked", 2550);
    }
  } else {
    delete res;
    LOG_WARN << "Resource is locked ";
    throw SQLException("Resource is locked ", 2550);
  }
  delete res;
}

void Connection::unlock(std::string resource) {
  Statement *stmt;
  stmt = prepareStatement("SELECT RELEASE_LOCK(?); ");
  stmt->setString(1, resource);
  stmt->executeQuery();
}

void Connection::unlock() {
  Statement *stmt;
  stmt = prepareStatement("SELECT RELEASE_ALL_LOCKS(); ");
  stmt->executeQuery();
}

bool Connection::ping() {
  return mysql_ping(_MySQLConnection) == 0;
}

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

