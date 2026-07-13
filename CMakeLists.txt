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
# Protobuf IDL 生成
add_subdirectory(proto)

# ODB ORM 模型生成
add_subdirectory(src/comm/models)

# 微服务
add_subdirectory(src/gateway)
# add_subdirectory(src/business_service)  # Phase 2 启用
add_subdirectory(src/judge_service)

# ==================== 现有单体服务（保持运行） ====================
# 添加子目录
add_subdirectory(src/compile_server)
add_subdirectory(src/oj_server)