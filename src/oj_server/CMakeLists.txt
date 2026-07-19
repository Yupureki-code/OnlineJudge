find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
pkg_check_modules(OJ_RUNTIME_ODB REQUIRED IMPORTED_TARGET libodb)
pkg_check_modules(OJ_RUNTIME_ODB_MYSQL REQUIRED IMPORTED_TARGET libodb-mysql)

find_library(OJ_SERVER_BRPC NAMES brpc REQUIRED)
find_library(OJ_SERVER_GFLAGS NAMES gflags REQUIRED)
find_library(OJ_SERVER_LEVELDB NAMES leveldb REQUIRED)
find_library(OJ_SERVER_PROTOC NAMES protoc REQUIRED)
find_library(OJ_SERVER_AMQPCPP NAMES amqpcpp REQUIRED)
find_library(REDIS_PLUS_PLUS_LIB NAMES redis++ redis++_static REQUIRED)
find_library(HIREDIS_LIB NAMES hiredis hiredis_static REQUIRED)
find_library(OJ_SERVER_CURL NAMES curl
    PATHS /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE} /lib/${CMAKE_LIBRARY_ARCHITECTURE}
    NO_DEFAULT_PATH REQUIRED)

set(OJ_SERVER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/oj_server/include)
set(OJ_BRPC_LIBRARIES
    ${OJ_SERVER_BRPC} ${OJ_SERVER_PROTOC} ${OJ_SERVER_GFLAGS} ${OJ_SERVER_LEVELDB}
    ${OJ_SERVER_AMQPCPP}
    OpenSSL::SSL OpenSSL::Crypto dl z event event_pthreads
)
set(OJ_LEGACY_LIBRARIES
    pthread jsoncpp ctemplate spdlog::spdlog ${OJ_SERVER_CURL}
    ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIB}
)

add_library(oj_runtime STATIC
    src/runtime/business_executor.cpp
    src/runtime/application_context.cpp
)
target_compile_features(oj_runtime PUBLIC cxx_std_20)
target_include_directories(oj_runtime PUBLIC ${OJ_SERVER_INCLUDE_DIR})
target_link_libraries(oj_runtime PUBLIC
    pthread spdlog::spdlog
    PkgConfig::OJ_RUNTIME_ODB PkgConfig::OJ_RUNTIME_ODB_MYSQL
)

add_executable(oj_server
    ${CMAKE_SOURCE_DIR}/src/comm/amqp_tcp_impl.cpp
    src/oj_server.cpp
    src/judge/outbox_publisher.cpp
    src/rpc/oj_service_impl.cpp
    src/rpc/static_http_service.cpp
)
add_executable(oj_admin
    src/oj_admin.cpp
    src/rpc/admin_audit_worker.cpp
    src/rpc/oj_admin_service_impl.cpp
    src/rpc/static_http_service.cpp
)
foreach(target oj_server oj_admin)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE ${OJ_SERVER_INCLUDE_DIR} ${PROTO_GEN_DIR} ${CMAKE_SOURCE_DIR}/src/comm)
    add_dependencies(${target} generate_phase3_proto)
endforeach()

target_link_libraries(oj_server PRIVATE
    oj_runtime phase3_proto_contract protobuf::libprotobuf oj_odb_models
    PkgConfig::OJ_RUNTIME_ODB PkgConfig::OJ_RUNTIME_ODB_MYSQL
    ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIB}
    ${OJ_LEGACY_LIBRARIES} ${OJ_BRPC_LIBRARIES}
)
target_link_libraries(oj_admin PRIVATE
    oj_runtime phase3_proto_contract protobuf::libprotobuf oj_odb_models
    PkgConfig::OJ_RUNTIME_ODB PkgConfig::OJ_RUNTIME_ODB_MYSQL
    ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIB}
    ${OJ_LEGACY_LIBRARIES} ${OJ_BRPC_LIBRARIES}
)

if(NOT EXISTS ${HTML_PATH})
    message(FATAL_ERROR "cannot find html : ${HTML_PATH}")
endif()
