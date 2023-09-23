CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(beehive VERSION 0.1.0)

INCLUDE(CTest)
ENABLE_TESTING()

SET(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(CURL REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED)

FIND_PATH(UUID_INCLUDE_DIR uuid.h PATH_SUFFIXES uuid)
FIND_LIBRARY(UUID_LIBRARY NAMES uuid libuuid)
IF (NOT(UUID_INCLUDE_DIR AND UUID_LIBRARY))
    MESSAGE(FATAL_ERROR "Could not find the UUID library and development files.")
ENDIF (NOT(UUID_INCLUDE_DIR AND UUID_LIBRARY))

FIND_PATH(FCGI_INCLUDE_DIR fcgiapp.h)
FIND_LIBRARY(FCGI_LIBRARY NAMES fcgi libfcgi)
IF (NOT(FCGI_INCLUDE_DIR AND FCGI_LIBRARY))
    MESSAGE(FATAL_ERROR "Could not find the FCGI library and development files.")
ENDIF (NOT(FCGI_INCLUDE_DIR AND FCGI_LIBRARY))

FIND_PATH(ARGON2_INCLUDE_DIR argon2)
FIND_LIBRARY(ARGON2_LIBRARY NAMES argon2 libargon2)
IF (NOT(ARGON2_INCLUDE_DIR AND ARGON2_LIBRARY))
    MESSAGE(FATAL_ERROR "Could not find the ARGON2 library and development files.")
ENDIF (NOT(ARGON2_INCLUDE_DIR AND ARGON2_LIBRARY))

FIND_PATH(ROCKSDB_INCLUDE_DIR db.h PATH_SUFFIXES rocksdb)
FIND_LIBRARY(ROCKSDB_LIBRARIES NAMES rocksdb librocksdb)
IF (NOT(ROCKSDB_INCLUDE_DIR AND ROCKSDB_LIBRARIES))
    MESSAGE(FATAL_ERROR "Could not find the Rocks library and development files.")
ENDIF (NOT(ROCKSDB_INCLUDE_DIR AND ROCKSDB_LIBRARIES))

INCLUDE_DIRECTORIES(
    ${PROJECT_SOURCE_DIR}/include
    ${LUA_INCLUDE_DIR}
    ${UUID_INCLUDE_DIR}
    ${FCGI_INCLUDE_DIR}
    ${ARGON2_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${ROCKSDB_INCLUDE_DIR}
)

SET(HEADER_FILES   
    include/concurrency/SleepyWorker.hpp
    include/concurrency/spinlock.hpp
    include/config/Context.hpp
    include/config/Entity.hpp
    include/config/Module.hpp
    include/config/Role.hpp
    include/config/Transaction.hpp
    include/crypto/base.h
    include/crypto/base64.h
    include/crypto/Crypto.hpp
    include/crypto/jwt.h
    include/crypto/picojson.h
    include/dao/ChangeDAO.hpp
    include/dao/DatasetDAO.hpp
    include/dao/DownloadedDAO.hpp
    include/dao/EntityDAO.hpp
    include/dao/HeaderDAO.hpp
    include/dao/MemberDAO.hpp
    include/dao/NodeDAO.hpp
    include/dao/PushDAO.hpp
    include/dao/UserDAO.hpp
    include/dao/Storage.hpp
    include/entities/Change.hpp
    include/entities/Dataset.hpp
    include/entities/Developer.hpp
    include/entities/Header.hpp
    include/entities/KeyData.hpp
    include/entities/Member.hpp
    include/entities/Node.hpp
    include/entities/Push.hpp
    include/entities/User.hpp
    include/exprtk/exprtk.hpp
    include/fcgi/FcgiHandler.hpp
    include/json/Common.hpp
    include/json/json.hpp
    include/nanolog/NanoLog.hpp
    include/services/InboundHTTP.hpp
    include/services/InboundTCP.hpp
    include/services/OutboundHTTP.hpp
    include/services/DatasetService.hpp
    include/services/SchemaService.hpp
    include/services/ServiceException.hpp
    include/services/StorageService.hpp
    include/services/UserService.hpp
    include/sol/as_args.hpp
    include/sol/as_returns.hpp
    include/sol/base_traits.hpp
    include/sol/bind_traits.hpp
    include/sol/bytecode.hpp
    include/sol/call.hpp
    include/sol/compatibility.hpp
    include/sol/compatibility/compat-5.3.c.h
    include/sol/compatibility/compat-5.3.h
    include/sol/compatibility/compat-5.4.h
    include/sol/compatibility/lua_version.hpp
    include/sol/compatibility/version.hpp
    include/sol/config.hpp
    include/sol/config_setup.hpp
    include/sol/coroutine.hpp
    include/sol/debug.hpp
    include/sol/demangle.hpp
    include/sol/deprecate.hpp
    include/sol/dump_handler.hpp
    include/sol/ebco.hpp
    include/sol/environment.hpp
    include/sol/error.hpp
    include/sol/error_handler.hpp
    include/sol/feature_test.hpp
    include/sol/forward.hpp
    include/sol/forward_detail.hpp
    include/sol/function.hpp
    include/sol/function_result.hpp
    include/sol/function_types.hpp
    include/sol/function_types_core.hpp
    include/sol/function_types_overloaded.hpp
    include/sol/function_types_stateful.hpp
    include/sol/function_types_stateless.hpp
    include/sol/function_types_templated.hpp
    include/sol/in_place.hpp
    include/sol/inheritance.hpp
    include/sol/load_result.hpp
    include/sol/lua_table.hpp
    include/sol/lua_value.hpp
    include/sol/make_reference.hpp
    include/sol/map.hpp
    include/sol/metatable.hpp
    include/sol/object.hpp
    include/sol/object_base.hpp
    include/sol/optional.hpp
    include/sol/optional_implementation.hpp
    include/sol/overload.hpp
    include/sol/pointer_like.hpp
    include/sol/policies.hpp
    include/sol/property.hpp
    include/sol/protect.hpp
    include/sol/protected_function.hpp
    include/sol/protected_function_result.hpp
    include/sol/protected_handler.hpp
    include/sol/proxy_base.hpp
    include/sol/raii.hpp
    include/sol/reference.hpp
    include/sol/resolve.hpp
    include/sol/sol.hpp
    include/sol/stack.hpp
    include/sol/stack_check.hpp
    include/sol/stack_check_get.hpp
    include/sol/stack_check_get_qualified.hpp
    include/sol/stack_check_get_unqualified.hpp
    include/sol/stack_check_qualified.hpp
    include/sol/stack_check_unqualified.hpp
    include/sol/stack_core.hpp
    include/sol/stack_field.hpp
    include/sol/stack_get.hpp
    include/sol/stack_get_qualified.hpp
    include/sol/stack_get_unqualified.hpp
    include/sol/stack_guard.hpp
    include/sol/stack_iterator.hpp
    include/sol/stack_pop.hpp
    include/sol/stack_probe.hpp
    include/sol/stack_proxy.hpp
    include/sol/stack_proxy_base.hpp
    include/sol/stack_push.hpp
    include/sol/stack_reference.hpp
    include/sol/state.hpp
    include/sol/state_handling.hpp
    include/sol/state_view.hpp
    include/sol/string_view.hpp
    include/sol/table.hpp
    include/sol/table_core.hpp
    include/sol/table_iterator.hpp
    include/sol/table_proxy.hpp
    include/sol/thread.hpp
    include/sol/tie.hpp
    include/sol/traits.hpp
    include/sol/trampoline.hpp
    include/sol/tuple.hpp
    include/sol/types.hpp
    include/sol/unicode.hpp
    include/sol/unique_usertype_traits.hpp
    include/sol/unsafe_function.hpp
    include/sol/unsafe_function_result.hpp
    include/sol/userdata.hpp
    include/sol/usertype.hpp
    include/sol/usertype_container.hpp
    include/sol/usertype_container_launch.hpp
    include/sol/usertype_core.hpp
    include/sol/usertype_proxy.hpp
    include/sol/usertype_storage.hpp
    include/sol/usertype_traits.hpp
    include/sol/variadic_args.hpp
    include/sol/variadic_results.hpp
    include/sol/version.hpp
    include/sol/wrapper.hpp
    include/sqlite/BinaryDecoder.hpp
    include/sqlite/BinaryEncoder.hpp
    include/sqlite/DecoderException.hpp
    include/sqlite/TextDecoder.hpp
    include/sqlite/TextEncoder.hpp
    include/sqlite/Types.hpp
    include/string/ICaseMap.hpp
    include/tcp/SocketWatcher.hpp
    include/tcp/TCPException.hpp
    include/tcp/TCPHandler.hpp
    include/validation/TransactionsManager.hpp
    include/validation/Validator.hpp
)

SET(SRC_FILES
    src/crypto/base64.cpp
    src/crypto/Crypto.cpp
    src/dao/ChangeDAO.cpp
    src/dao/DatasetDAO.cpp
    src/dao/DownloadedDAO.cpp
    src/dao/EntityDAO.cpp
    src/dao/HeaderDAO.cpp
    src/dao/MemberDAO.cpp
    src/dao/NodeDAO.cpp
    src/dao/PushDAO.cpp
    src/dao/UserDAO.cpp
    src/dao/Storage.cpp
    src/fcgi/FcgiHandler.cpp
    src/nanolog/NanoLog.cpp
    src/services/InboundHTTP.cpp
    src/services/InboundTCP.cpp
    src/services/OutboundHTTP.cpp
    src/services/DatasetService.cpp
    src/services/SchemaService.cpp
    src/services/StorageService.cpp
    src/services/UserService.cpp
    src/sqlite/BinaryDecoder.cpp
    src/sqlite/BinaryEncoder.cpp
    src/sqlite/TextDecoder.cpp
    src/sqlite/TextEncoder.cpp
    src/string/ICaseMap.cpp
    src/tcp/SocketWatcher.cpp
    src/tcp/TCPHandler.cpp
    src/validation/TransactionsManager.cpp
    src/main.cpp
)

ADD_EXECUTABLE(beehive ${SRC_FILES} ${HEADER_FILES})

TARGET_LINK_LIBRARIES(beehive PRIVATE Threads::Threads
    ${LUA_LIBRARIES}
    ${UUID_LIBRARY}
    ${FCGI_LIBRARY}
    ${ARGON2_LIBRARY}
    ${CURL_LIBRARIES}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${ROCKSDB_LIBRARIES}
)

SET_TARGET_PROPERTIES(beehive PROPERTIES CXX_STANDARD 20)

SET(CPACK_PROJECT_NAME ${PROJECT_NAME})
SET(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
INCLUDE(CPack)
