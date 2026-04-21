cmake_minimum_required(VERSION 3.28.3)

project(OnlineJudge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(OJ_TESTS "null")
set(OJ_QUESTIONS "null")
set(OJ_USERS "null")
set(HOST "null")
set(USER "null")
set(PASSWD "null")
set(MYOJ "null")
set(PORT "null")
set(REDIS "null")

add_subdirectory(src/private)

option(ENABLE_DEBUG "Enable debug output" OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output)
set(CONF_PATH ${CMAKE_SOURCE_DIR}/conf CACHE STRING "default conf file path")
set(HTML_PATH ${CMAKE_SOURCE_DIR}/src/wwwroot CACHE STRING "default html file path")
set(LOG_PATH ${CMAKE_SOURCE_DIR}/log CACHE STRING "default log file path")

configure_file(${CMAKE_SOURCE_DIR}/config.in.h ${CMAKE_SOURCE_DIR}/src/comm/config.h)

# 添加子目录
add_subdirectory(src/compile_server)
add_subdirectory(src/oj_server)