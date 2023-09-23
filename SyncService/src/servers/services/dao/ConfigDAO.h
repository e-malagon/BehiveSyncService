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

#ifndef CONFIGDAO_H_
#define CONFIGDAO_H_

#include <time.h>
#include <sqlite/sqlite3.h>
#include <config/Config.h>
#include <vector>
#include <optional>
#include <utility>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

class ConfigDAO {
public:
  ConfigDAO(sqlite3 *db) :
      _db(db) {
  }

  int beginTransaction(const std::string id, const char *idDataset);
  int commitTransaction();
  int rollbackTransaction();
  int saveBeehive(uint32_t beehive, const char *idDataset);
  int saveDeveloper(uint32_t beehive, const std::string &name, const std::string &email, const std::string &passwd, const std::string &salt);
  int saveSession(uint32_t beehive, const std::string &email, const std::string &token, time_t now);
  int saveApplication(uint32_t beehive, const std::string &uuid, const std::string &schema);
  int saveEntity(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version);
  int saveTransaction(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version);
  int saveRole(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version);
  int saveModule(uint32_t beehive, const std::string &application, const std::string &uuid, const std::string &schema, int version);
  std::string readDataset(uint32_t beehive);
  std::optional<Config::Config::Developer> readDeveloper(const std::string &email);
  std::optional<std::string> readApplication(uint32_t beehive, const std::string &uuid);
  std::optional<std::pair<std::string, std::string>> readEntity(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  std::optional<std::pair<std::string, std::string>> readTransaction(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  std::optional<std::pair<std::string, std::string>> readRole(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  std::optional<std::pair<std::string, std::string>> readModule(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  std::vector<std::pair<uint32_t, uint64_t>> readBeehives();
  std::vector<std::string> readApplications(uint32_t beehive);
  std::vector<std::pair<std::string, std::string>> readEntities(uint32_t beehive, const std::string &application, int version);
  std::vector<std::pair<std::string, std::string>> readTransactions(uint32_t beehive, const std::string &application, int version);
  std::vector<std::pair<std::string, std::string>> readRoles(uint32_t beehive, const std::string &application, int version);
  std::vector<std::pair<std::string, std::string>> readModules(uint32_t beehive, const std::string &application, int version);
  int removeSession(uint32_t beehive, const std::string &email);
  int removeApplication(uint32_t beehive, const std::string &uuid);
  int removeEntity(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  int removeTransaction(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  int removeRole(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  int removeModule(uint32_t beehive, const std::string &application, const std::string &uuid, int version);
  int removeVersion(uint32_t beehive, const std::string &application, int version);

private:
  sqlite3 *_db;
};

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* CONFIGDAO_H_ */
