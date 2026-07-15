# src/judge_service/CMakeLists.txt

# C++20（协程支持）
set(CMAKE_CXX_STANDARD 20)

# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(CURL REQUIRED libcurl)

# brpc 使用手动查找（无 CMake 配置文件）
find_path(BRPC_INCLUDE_DIR NAMES brpc/server.h PATHS /usr/include)
find_library(BRPC_LIBRARY NAMES brpc PATHS /usr/lib/x86_64-linux-gnu)
if(NOT BRPC_INCLUDE_DIR OR NOT BRPC_LIBRARY)
    message(FATAL_ERROR "brpc not found. Please install brpc.")
endif()

# brpc 静态库依赖
find_package(OpenSSL REQUIRED)
find_library(GFLAGS_LIBRARY NAMES gflags PATHS /usr/lib/x86_64-linux-gnu)
find_library(LEVELDB_LIBRARY NAMES leveldb PATHS /usr/lib/x86_64-linux-gnu)
find_library(PROTOC_LIBRARY NAMES protoc PATHS /usr/lib/x86_64-linux-gnu)
find_library(AMQPCPP_LIBRARY NAMES amqpcpp PATHS /usr/local/lib)

# Judge Service 可执行文件
add_executable(judge_service
    main.cpp
    ${CMAKE_SOURCE_DIR}/src/comm/amqp_tcp_impl.cpp
    ${PROTO_GEN_DIR}/common.pb.cc
    ${PROTO_GEN_DIR}/judge_service.pb.cc
    ${PROTO_GEN_DIR}/biz_service.pb.cc
    ${PROTO_GEN_DIR}/mq_message.pb.cc
)

target_include_directories(judge_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
    ${BRPC_INCLUDE_DIR}
    ${LIBEVENT_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(judge_service PRIVATE
    ${BRPC_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${PROTOC_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${CURL_LIBRARIES}
    ${LIBEVENT_LIBRARIES}
    ${GFLAGS_LIBRARY}
    ${LEVELDB_LIBRARY}
    ${AMQPCPP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    dl
    z
    event_pthreads
    fmt
)

# 输出到 output/ 目录
set_target_properties(judge_service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)

# ── Integration Test Binary ──
add_executable(judge_integration_test
    ${CMAKE_SOURCE_DIR}/test/judge_integration_test.cpp
    ${CMAKE_SOURCE_DIR}/src/comm/amqp_tcp_impl.cpp
    ${PROTO_GEN_DIR}/common.pb.cc
    ${PROTO_GEN_DIR}/judge_service.pb.cc
)

target_include_directories(judge_integration_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
    ${BRPC_INCLUDE_DIR}
    ${LIBEVENT_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(judge_integration_test PRIVATE
    ${BRPC_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${PROTOC_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${CURL_LIBRARIES}
    ${LIBEVENT_LIBRARIES}
    ${GFLAGS_LIBRARY}
    ${LEVELDB_LIBRARY}
    ${AMQPCPP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    dl
    z
    event_pthreads
)

set_target_properties(judge_integration_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)

# ── Direct E2E Test (no MQ) ──
add_executable(direct_e2e_test
    ${CMAKE_SOURCE_DIR}/test/direct_e2e_test.cpp
    ${CMAKE_SOURCE_DIR}/src/comm/amqp_tcp_impl.cpp
    ${PROTO_GEN_DIR}/common.pb.cc
    ${PROTO_GEN_DIR}/judge_service.pb.cc
)

target_include_directories(direct_e2e_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
    ${BRPC_INCLUDE_DIR}
    ${LIBEVENT_INCLUDE_DIRS}
    ${PROTOBUF_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(direct_e2e_test PRIVATE
    ${BRPC_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${PROTOC_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${CURL_LIBRARIES}
    ${LIBEVENT_LIBRARIES}
    ${GFLAGS_LIBRARY}
    ${LEVELDB_LIBRARY}
    ${AMQPCPP_LIBRARY}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
    dl
    z
    event_pthreads
    fmt
)

set_target_properties(direct_e2e_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)

# ── LOG_DEBUG Output Verification (no Docker) ──
add_executable(verify_log_output
    ${CMAKE_SOURCE_DIR}/test/verify_log_output.cpp
    ${PROTO_GEN_DIR}/judge_service.pb.cc
    ${PROTO_GEN_DIR}/common.pb.cc
)

target_include_directories(verify_log_output PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
)

target_link_libraries(verify_log_output PRIVATE
    ${PROTOBUF_LIBRARIES}
    pthread
    fmt
)

set_target_properties(verify_log_output PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)

# ── High Concurrency Benchmark ──
add_executable(concurrency_bench
    ${CMAKE_SOURCE_DIR}/test/concurrency_bench.cpp
    ${PROTO_GEN_DIR}/judge_service.pb.cc
    ${PROTO_GEN_DIR}/common.pb.cc
)

target_include_directories(concurrency_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${LIBEVENT_INCLUDE_DIRS}
)

target_link_libraries(concurrency_bench PRIVATE
    ${PROTOBUF_LIBRARIES}
    pthread
    fmt
    ${LIBEVENT_LIBRARIES}
    event_pthreads
)

set_target_properties(concurrency_bench PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)