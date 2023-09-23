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

#include "SchemaDAO.h"

#include <dao/sql/Connection.h>
#include <dao/sql/ResultSet.h>
#include <dao/sql/Statement.h>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace DAO {

void SchemaDAO::createSchema(uint32_t beehive) {
  SQL::Statement *stmt;
  stmt = _connection->prepareStatement("CREATE SCHEMA IF NOT EXISTS `" + std::to_string(beehive) + "` DEFAULT CHARACTER SET utf8 COLLATE utf8_bin;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  _connection->switchBeehive(std::to_string(beehive));
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `users` (\n"//
          "`iduser` int unsigned NOT NULL AUTO_INCREMENT,\n"//
          "`identifier` varchar(1024) NOT NULL,\n"//
          "`name` varchar(1024) NOT NULL DEFAULT '',\n"//
          "`kind` tinyint NOT NULL DEFAULT '1',\n"//
          "`identity` varchar(100) NOT NULL DEFAULT '',\n"//
          "`password` varchar(100) NOT NULL DEFAULT '',\n"//
          "`salt` varchar(100) NOT NULL DEFAULT '',\n"//
          "PRIMARY KEY (`iduser`),\n"//
          "UNIQUE KEY `identifier_UNIQUE` (`identifier`)\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `nodes` (\n"//
          "`idnode` int unsigned NOT NULL AUTO_INCREMENT,\n"//
          "`iduser` int unsigned NOT NULL,\n"//
          "`key` binary(16) NOT NULL,\n"//
          "`application` binary(16) NOT NULL DEFAULT '',\n"//
          "`module` binary(16) NOT NULL DEFAULT '',\n"//
          "`uuid` varchar(36) NOT NULL,\n"//
          "`lastsync` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\n"//
          "PRIMARY KEY (`idnode`),\n"//
          "KEY `fk_nodes_users_idx` (`iduser`),\n"//
          "CONSTRAINT `fk_nodes_users` FOREIGN KEY (`iduser`) REFERENCES `users` (`iduser`) ON DELETE CASCADE\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `datasets` (\n"//
          "`iddataset` int unsigned NOT NULL AUTO_INCREMENT,\n"//
          "`uuid` char(36) NOT NULL,\n"//
          "`owner` int unsigned NOT NULL,\n"//
          "`idheader` int unsigned NOT NULL,\n"//
          "`status` tinyint unsigned NOT NULL,\n"//
          "PRIMARY KEY (`iddataset`),\n"//
          "CONSTRAINT `fk_datasets_users` FOREIGN KEY (`owner`) REFERENCES `users` (`iduser`) ON DELETE CASCADE ON UPDATE RESTRICT\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `members` (\n"//
          "`iddataset` int unsigned NOT NULL,\n"//
          "`iduser` int unsigned NOT NULL,\n"//
          "`role` binary(16) NOT NULL,\n"//
          "`name` varchar(256) NOT NULL,\n"//
          "`status` tinyint NOT NULL,\n"//
          "PRIMARY KEY (`iddataset`,`iduser`),\n"//
          "KEY `fk_members_dataset_idx` (`iddataset`),\n"//
          "KEY `fk_members_users_idx` (`iduser`),\n"//
          "CONSTRAINT `fk_members_dataset` FOREIGN KEY (`iddataset`) REFERENCES `datasets` (`iddataset`) ON DELETE CASCADE ON UPDATE RESTRICT,\n"//
          "CONSTRAINT `fk_members_users` FOREIGN KEY (`iduser`) REFERENCES `users` (`iduser`) ON DELETE CASCADE\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `downloaded` (\n"//
          "`idnode` int unsigned NOT NULL,\n"//
          "`iddataset` int unsigned NOT NULL,\n"//
          "`idheader` int unsigned NOT NULL,\n"//
          "`idcell` int unsigned NOT NULL,\n"//
          "PRIMARY KEY (`idnode`,`iddataset`),\n"//
          "KEY `fk_downloaded_dataset_idx` (`iddataset`),\n"//
          "CONSTRAINT `fk_downloaded_dataset` FOREIGN KEY (`iddataset`) REFERENCES `datasets` (`iddataset`) ON DELETE CASCADE,\n"//
          "CONSTRAINT `fk_downloaded_node` FOREIGN KEY (`idnode`) REFERENCES `nodes` (`idnode`) ON DELETE CASCADE\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `push` (\n"//
          "`iddataset` int unsigned NOT NULL,\n"//
          "`uuid` char(36) NOT NULL,\n"//
          "`role` binary(16) NOT NULL,\n"//
          "`until` bigint NOT NULL,\n"//
          "`number` int NOT NULL,\n"//
          "PRIMARY KEY (`iddataset`,`uuid`),\n"//
          "CONSTRAINT `fk_push_dataset` FOREIGN KEY (`iddataset`) REFERENCES `datasets` (`iddataset`) ON DELETE CASCADE\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `headers` (\n"//
          "`iddataset` int unsigned NOT NULL,\n"//
          "`idheader` int unsigned NOT NULL,\n"//
          "`transaction` binary(16) NOT NULL,\n"//
          "`node` int unsigned NOT NULL,\n"//
          "`idnode` int unsigned NOT NULL,\n"//
          "`status` tinyint unsigned NOT NULL DEFAULT '0',\n"//
          "`date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,\n"//
          "PRIMARY KEY (`iddataset`,`idheader`),\n"//
          "CONSTRAINT `fk_headers_datasets` FOREIGN KEY (`iddataset`) REFERENCES `datasets` (`iddataset`) ON DELETE CASCADE\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=utf8;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
  stmt = _connection->prepareStatement( //
      "CREATE TABLE IF NOT EXISTS `changes` (\n"//
          "`iddataset` int unsigned NOT NULL,\n"//
          "`idheader` int unsigned NOT NULL,\n"//
          "`idchange` int unsigned NOT NULL,\n"//
          "`operation` tinyint NOT NULL,\n"//
          "`entity` binary(16) NOT NULL,\n"//
          "`key` varbinary(256) NOT NULL,\n"//
          "`old` varbinary(256) NOT NULL,\n"//
          "`data` varbinary(32768) NOT NULL,\n"//
          "PRIMARY KEY (`iddataset`,`idheader`,`idchange`),\n"//
          "CONSTRAINT `fk_changes_headers` FOREIGN KEY (`iddataset`, `idheader`) REFERENCES `headers` (`iddataset`, `idheader`) ON DELETE CASCADE\n"//
          ") ENGINE=InnoDB DEFAULT CHARSET=binary;", 0, 1);
  stmt->executeUpdate();
  delete stmt;
}

} /* namespace DAO */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */
