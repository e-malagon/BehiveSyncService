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

#ifndef STATICSTATEMENT_H_
#define STATICSTATEMENT_H_

#include "ResultSet.h"
#include "Statement.h"

#include <mysql.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {
namespace SQL {

class Connection;

class StaticStatement: public Statement {
  friend class Connection;
public:
  int executeUpdate();
  ResultSet* executeQuery();
  int getLastId();
  void setInt(unsigned int parameterIndex, int value);
  void setLong(unsigned int parameterIndex, long value);
  void setString(unsigned int parameterIndex, const std::string &value);
  void setString(unsigned int parameterIndex, uint8_t *value, const uint16_t len);
  void setUUID(unsigned int parameterIndex, const std::string &value);
  void setBlob(unsigned int parameterIndex, const std::string &value);
private:
  StaticStatement(MYSQL *mysql, const std::string &statement);
  virtual ~StaticStatement();
  void split(std::vector<std::string> &tokens, const std::string &text, char sep);
  std::string getStatement();
  void clear();
  MYSQL *MySQLConnection;
  std::string statement;
  std::vector<std::string> tokens;
  std::unordered_map<int, long> numbers;
  std::unordered_map<int, std::string> strings;
  std::unordered_map<int, int> types;
  unsigned int paramCount;

  static const int NUMBER = 1;
  static const int STRING = 2;
};

} /* namespace SQL */
} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* STATICSTATEMENT_H_ */
