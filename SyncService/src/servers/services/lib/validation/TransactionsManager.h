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

#ifndef TRIGGERMANAGER_H_
#define TRIGGERMANAGER_H_

#include <sol/sol.hpp>
#include <sqlite/Types.h>
#include <dao/EntityDAO.h>
#include <config/Application.h>
#include <entities/Header.h>

#include <nanolog/NanoLog.hpp>
#include <sqlite/BinaryDecoder.h>
#include <string/ICaseMap.h>
#include <unordered_map>

namespace SyncServer {
namespace Servers {
namespace Services {
namespace Utils {

inline void onPanic(sol::optional<std::string> message) {
  LOG_ERROR << "Lua is in a panic state and will now abort() the application: " << message.value();
  if (message) {
    //LOG_FILE(ERROR, _beehive) << message.value() << std::endl; // TODO check this error to know if user must be notified
  }
}

class TransactionsManager {
public:
  TransactionsManager(uint32_t beehive) :
      _beehive(beehive), _lua(sol::c_call<decltype(&onPanic), &onPanic>) {
    _lua.open_libraries(sol::lib::table, sol::lib::string, sol::lib::math, sol::lib::base);
    _lua.set_function("log", [&](std::string message) {
      LOG_FILE_INFO(_beehive) << message;
    });

    _lua.set_function("read", [&](std::string entity, sol::table data) {
      return readEntity(entity, data);
    });

    _lua.set_function("save", [&](std::string entity, sol::table data) {
      return saveEntity(entity, data);
    });

    _lua.set_function("update", [&](std::string entity, sol::table data) {
      return updateEntity(entity, data);
    });

    _lua.set_function("remove", [&](std::string entity, sol::table key) {
      return removeEntity(entity, key);
    });
  }

  virtual ~TransactionsManager() {
  }

  void application(Config::Application *application) {
    _application = application;
  }

  void entityDAO(DAO::EntityDAO *entityDAO) {
    _entityDAO = entityDAO;
  }

  sol::table readEntity(std::string entity, sol::table data);
  int saveEntity(std::string entity, sol::table data);
  int updateEntity(std::string entity, sol::table data);
  int removeEntity(std::string entity, sol::table key);

  void loadValidation(std::string transaction, std::string script);
  void loadCommit(std::string transaction, std::string script);
  bool executeValidation(Entities::Header &header);
  bool executeCommit(Entities::Header &header);

private:
  uint32_t _beehive;
  sol::state _lua;
  std::unordered_map<std::string, sol::load_result, IHasher, IEqualsComparator> _onValidation;
  std::unordered_map<std::string, sol::load_result, IHasher, IEqualsComparator> _onCommit;
  Config::Application *_application;
  DAO::EntityDAO *_entityDAO;
};

} /* namespace Utils */
} /* namespace Services */
} /* namespace Servers */
} /* namespace SyncServer */

#endif /* TRIGGERMANAGER_H_ */
