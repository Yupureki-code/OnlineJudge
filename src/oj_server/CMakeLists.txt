# 主服务器的 CMakeLists.txt

# 添加可执行程序
#add_executable(oj_server src/oj_server.cpp)
add_executable(oj_admin src/oj_admin.cpp)

set(SERVICE_MACHINE_CONF ${CONF_PATH}/service_machine.conf)

if(EXISTS ${SERVICE_MACHINE_CONF})
    message(STATUS "find service_machine.conf : ${SERVICE_MACHINE_CONF}")
else()
    message(FATAL_ERROR "cannot find service_machine.conf : ${SERVICE_MACHINE_CONF}")
endif()

if(EXISTS ${HTML_PATH})
    message(STATUS "find html : ${HTML_PATH}")
else()
    message(FATAL_ERROR "cannot find html : ${HTML_PATH}")
endif()

# 包含头文件目录
#target_include_directories(oj_server PRIVATE ${CMAKE_SOURCE_DIR}/src/oj_server/include)
target_include_directories(oj_admin PRIVATE ${CMAKE_SOURCE_DIR}/src/oj_server/include)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# 链接库
#target_link_libraries(oj_server PRIVATE pthread mysqlclient jsoncpp ctemplate cpp-httplib Logger CURL::libcurl OpenSSL::Crypto)
target_link_libraries(oj_admin PRIVATE pthread mysqlclient jsoncpp ctemplate cpp-httplib Logger CURL::libcurl OpenSSL::Crypto)

# Redis client libraries (redis++ depends on hiredis)
find_library(REDIS_PLUS_PLUS_LIB NAMES redis++ redis++_static)
find_library(HIREDIS_LIB NAMES hiredis hiredis_static)

if(NOT REDIS_PLUS_PLUS_LIB)
    message(FATAL_ERROR "redis++ library not found. Please install redis++ and retry.")
endif()

if(NOT HIREDIS_LIB)
    message(FATAL_ERROR "hiredis library not found. Please install hiredis and retry.")
endif()

#target_link_libraries(oj_server PRIVATE ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIB})
target_link_libraries(oj_admin PRIVATE ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIB})