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

#ifndef SCHEMASERVICE_H_
#define SCHEMASERVICE_H_

#include <dao/sql/Connection.h>
#include <dao/sql/SQLException.h>
#include <dao/EntityDAO.h>
#include <dao/SchemaDAO.h>
#include <config/Application.h>
#include <config/Bootstrap.h>
#include <config/Config.h>

#include <sqlite/sqlite3.h>
#include <utility>
#include <vector>
#include <memory>
#include <mutex>

namespace SyncServer {
namespace Servers {
namespace Services {

class SchemaService {
public:
  SchemaService(std::shared_ptr<DAO::SQL::Connection> connection, uint32_t beehive) :
      _connection(connection), _entityDAO(connection, beehive), _schemaDAO(connection), _beehive(beehive) {
  }
  virtual ~SchemaService() {
  }

  std::string readApplicationSchema(uint32_t beehive, const std::string uuid);
  std::string readApplicationSchema(uint32_t beehive, const std::string application, const std::string kind, int version);

  bool tryCreateBeehive(uint32_t beehive, const std::string &name, const std::string &email, const std::string &password);
  void createSchemaMaster();
  std::string readSchema(const std::string &uuid, const std::string &application, const std::string &kind, bool asCopy, int requestedVersion);
  std::vector<std::string> readSchemas(const std::string &application, const std::string &kind);
  void updateSchema(const std::string &application, const std::string &uuid, const std::string &schema, const std::string &kind);
  bool removeSchema(const std::string &uuid, const std::string &application, const std::string &kind);
  std::optional<std::string> publishApplication(const std::string &uuid);
  std::optional<std::string> downgradeApplication(const std::string &uuid);
  std::pair<std::string, std::string> exportApplication(const std::string &uuid);
  std::pair<std::string, int> importApplication(const std::string &application);

  static void loadSchemas(sqlite3 *db, uint32_t beehive);
  static Config::Application loadApplicationSchemas(sqlite3 *db, uint32_t beehive, const std::string schema);
  static void loadApplicationSchemas(Config::Bootstrap bootstrap);
  static std::pair<std::string, std::string> getApplicationAndModuleUUID(uint32_t beehive, const std::string application, const std::string module, uint32_t version);
  static Config::Application getApplicationAndModuleUUID(uint32_t beehive, const std::string &application);

private:
  std::shared_ptr<DAO::SQL::Connection> _connection;
  DAO::EntityDAO _entityDAO;
  DAO::SchemaDAO _schemaDAO;
  uint32_t _beehive;

  static std::unordered_map<uint32_t, std::unordered_map<std::string, Config::Application>> _beehives;
  static std::unordered_map<uint32_t, std::unordered_map<std::string, std::string>> _applications;
  static std::mutex _beehivesMutex;
};

} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* SCHEMASERVICE_H_ */
