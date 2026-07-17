cmake_minimum_required(VERSION 3.28.3)

project(OnlineJudge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

option(ENABLE_DEBUG "Enable debug output" OFF)
option(OJ_USE_LOCAL_LOGGER "Build and use in-tree Logger" ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)
set(CONF_PATH ${CMAKE_SOURCE_DIR}/conf CACHE STRING "default conf file path")
set(HTML_PATH ${CMAKE_SOURCE_DIR}/src/wwwroot CACHE STRING "default html file path")
set(LOG_PATH ${CMAKE_SOURCE_DIR}/log CACHE STRING "default log file path")
set(ADMIN_HOST "0.0.0.0" CACHE STRING "admin server bind host")
set(ADMIN_PORT "8090" CACHE STRING "admin server bind port")
configure_file(${CMAKE_SOURCE_DIR}/config.in.h ${CMAKE_SOURCE_DIR}/src/comm/config.h)

# ==================== 微服务改造新增 ====================
# Protobuf 定义和源码树生成文件目录
set(PROTO_GEN_DIR ${CMAKE_SOURCE_DIR}/src/comm/proto)
find_package(Protobuf REQUIRED)
set(OJ_PROTO_FILES
    ${PROTO_GEN_DIR}/common.proto
    ${PROTO_GEN_DIR}/biz_service.proto
    ${PROTO_GEN_DIR}/judge_service.proto
    ${PROTO_GEN_DIR}/mq_message.proto
    ${PROTO_GEN_DIR}/static_http.proto
)
set(OJ_PROTO_OUTPUTS
    ${PROTO_GEN_DIR}/common.pb.h ${PROTO_GEN_DIR}/common.pb.cc
    ${PROTO_GEN_DIR}/biz_service.pb.h ${PROTO_GEN_DIR}/biz_service.pb.cc
    ${PROTO_GEN_DIR}/judge_service.pb.h ${PROTO_GEN_DIR}/judge_service.pb.cc
    ${PROTO_GEN_DIR}/mq_message.pb.h ${PROTO_GEN_DIR}/mq_message.pb.cc
    ${PROTO_GEN_DIR}/static_http.pb.h ${PROTO_GEN_DIR}/static_http.pb.cc
)
add_custom_command(
    OUTPUT ${OJ_PROTO_OUTPUTS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path=${PROTO_GEN_DIR}
            --cpp_out=${PROTO_GEN_DIR}
            ${OJ_PROTO_FILES}
    COMMAND ${CMAKE_COMMAND}
            -DPROTO_DIR=${PROTO_GEN_DIR}
            -P ${PROTO_GEN_DIR}/trim_generated_whitespace.cmake
    DEPENDS ${OJ_PROTO_FILES} ${PROTO_GEN_DIR}/trim_generated_whitespace.cmake
    COMMENT "Generating Phase 3 protobuf contracts"
    VERBATIM
)
add_custom_target(generate_phase3_proto DEPENDS ${OJ_PROTO_OUTPUTS})

# ODB ORM 模型生成
add_subdirectory(src/comm/models)

# 微服务
# add_subdirectory(src/gateway)       # Gateway 暂未就绪
# add_subdirectory(src/business_service)  # Phase 3 启用
add_subdirectory(src/judge_service)
foreach(JUDGE_TARGET judge_service judge_integration_test direct_e2e_test verify_log_output
                     judge_event_loop_test judge_inbox_test concurrency_bench)
    if(TARGET ${JUDGE_TARGET})
        add_dependencies(${JUDGE_TARGET} generate_phase3_proto)
    endif()
endforeach()

# ==================== 现有单体服务（保持运行） ====================
add_subdirectory(src/oj_server)

# Phase 3 protocol contract verification against the tracked generated sources.
add_library(phase3_proto_contract STATIC
    ${PROTO_GEN_DIR}/common.pb.cc
    ${PROTO_GEN_DIR}/biz_service.pb.cc
    ${PROTO_GEN_DIR}/judge_service.pb.cc
    ${PROTO_GEN_DIR}/mq_message.pb.cc
    ${PROTO_GEN_DIR}/static_http.pb.cc
)
add_dependencies(phase3_proto_contract generate_phase3_proto)
target_include_directories(phase3_proto_contract PUBLIC ${PROTO_GEN_DIR})
target_link_libraries(phase3_proto_contract PUBLIC protobuf::libprotobuf)

add_executable(proto_contract_test test/proto_contract_test.cpp)
target_link_libraries(proto_contract_test PRIVATE phase3_proto_contract)

# Contract manifests are standalone and do not require running legacy services.
add_subdirectory(test)
