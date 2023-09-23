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


#include <AdminHandler.h>
#include <BinSyncHandler.h>
#include <HttpSyncHandler.h>
#include <config/Config.h>
#include <dao/sql/ConnectionPool.h>
#include <tclap/CmdLine.h>

#include <nanolog/NanoLog.hpp>
#include <fcgiapp.h>
#include <csignal>
#include <unistd.h>
#include <thread>

extern bool running;
extern int binSocket;
void signalHandler(int signal) {
  running = false;
  if (binSocket)
    close(binSocket);
  FCGX_ShutdownPending();
}

extern SyncServer::Servers::Services::Config::Config config;

using SyncServer::Servers::Services::Config::loadBootstrap;
using SyncServer::Servers::Services::Config::loadConfig;
using SyncServer::Servers::Services::Config::loadForms;

constexpr unsigned int switchstring(const char *str, int h = 0) {
  return !str[h] ? 5381 : (switchstring(str, h + 1) * 33) ^ str[h];
}

int main(int argc, char *argv[]) {
  try {
    std::string version = "1.0 - " + std::string(__DATE__) + " " + std::string(__TIME__);
    TCLAP::CmdLine cmd("Beehive smart sync server", ' ', version);
    TCLAP::ValueArg<std::string> modeArg("m", "mode", "Operation mode[master|beehive|developer]", false, "master", "string");
    TCLAP::ValueArg<std::string> instanceArg("i", "instance", "Instance Id", false, "00000000", "string");
    cmd.add(modeArg);
    cmd.add(instanceArg);
    cmd.parse(argc, argv);

    nanolog::initialize(nanolog::GuaranteedLogger(), "/var/log/syncserver", "syncserver", 1);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    LOG_DEBUG << "Starting...";
    try {
      switch (switchstring(modeArg.getValue().c_str())) {
      case switchstring("master"):
        if (loadBootstrap()) {
          SyncServer::Servers::Services::DAO::SQL::ConnectionPool connectionPool(5, config.database.server, config.database.port, config.database.user, config.database.password, false);
          std::thread binSync([&connectionPool, &modeArg] {
            try {
              SyncServer::Servers::BinSyncHandler binSyncHandler(connectionPool);
              binSyncHandler.run(modeArg.getValue());
            } catch (std::system_error &e) {
              LOG_ERROR << e.what();
            }
          });
          binSync.join();
        }
        break;
      case switchstring("beehive"):
        if (loadConfig(modeArg.getValue(), instanceArg.getValue())) {
          SyncServer::Servers::Services::DAO::SQL::ConnectionPool connectionPool(5, config.database.server, config.database.port, config.database.user, config.database.password, false);
          std::thread binSync([&connectionPool, &modeArg] {
            try {
              SyncServer::Servers::BinSyncHandler binSyncHandler(connectionPool);
              binSyncHandler.run(modeArg.getValue());
            } catch (std::system_error &e) {
              LOG_ERROR << e.what();
            }
          });
          std::thread httpSync([&connectionPool, &modeArg] {
            try {
              SyncServer::Servers::HttpSyncHandler httpSyncHandler(connectionPool);
              httpSyncHandler.run(modeArg.getValue());
            } catch (std::system_error &e) {
              LOG_ERROR << e.what();
            }
          });
          binSync.join();
          httpSync.join();
        }
        break;
      case switchstring("developer"):
        if (loadConfig(modeArg.getValue(), instanceArg.getValue()) && loadForms()) {
          SyncServer::Servers::Services::DAO::SQL::ConnectionPool connectionPool(5, config.database.server, config.database.port, config.database.user, config.database.password, false);
          std::thread binSync([&connectionPool, &modeArg] {
            try {
              SyncServer::Servers::BinSyncHandler binSyncHandler(connectionPool);
              binSyncHandler.run(modeArg.getValue());
            } catch (std::system_error &e) {
              LOG_ERROR << e.what();
            }
          });
          std::thread admin([&connectionPool, &modeArg] {
            try {
              SyncServer::Servers::AdminHandler adminHandler(connectionPool);
              adminHandler.run(modeArg.getValue());
            } catch (std::system_error &e) {
              LOG_ERROR << e.what();
            }
          });
          std::thread httpSync([&connectionPool, &modeArg] {
            try {
              SyncServer::Servers::HttpSyncHandler httpSyncHandler(connectionPool);
              httpSyncHandler.run(modeArg.getValue());
            } catch (std::system_error &e) {
              LOG_ERROR << e.what();
            }
          });
          binSync.join();
          admin.join();
          httpSync.join();
        }
        break;
      }
    } catch (std::system_error &e) {
      LOG_ERROR << e.what();
    }
  } catch (TCLAP::ArgException &e) {
    std::cerr << e.error() << " for arg " << e.argId();
  } catch (std::system_error &e) {
    std::cerr << e.what();
  }
  return EXIT_SUCCESS;
}
