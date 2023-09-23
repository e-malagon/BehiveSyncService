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

#ifndef PREPAREDRESULTSET_H_
#define PREPAREDRESULTSET_H_

#include "ResultSet.h"

#include <mysql.h>
#include <string>
#include <unordered_map>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

class PreparedStatement;

class PreparedResultSet: public ResultSet {
  friend class PreparedStatement;
public:
  virtual ~PreparedResultSet();
  bool next();
  int getInt(const int column) const;
  int getInt(const std::string &column) const;
  long getLong(const int column) const;
  long getLong(const std::string &column) const;
  std::string getString(const int column) const;
  std::string getString(const std::string &column) const;
  std::string getUUID(const int column) const;
  std::string getUUID(const std::string &column) const;
  std::string getBlob(const int column) const;
  std::string getBlob(const std::string &column) const;
  bool isNull(const std::string &column) const;
private:
  PreparedResultSet(MYSQL_STMT *stmt, MYSQL_RES *result, MYSQL_BIND *resultBinds, std::unordered_map<std::string, int> &columns, unsigned int columnCount);
  MYSQL_STMT *stmt;
  MYSQL_RES *result;
  MYSQL_BIND *resultBinds;
  std::unordered_map<std::string, int> &columns;
  unsigned int columnCount;
};

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* PREPAREDRESULTSET_H_ */
