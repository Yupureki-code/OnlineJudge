# 主服务器的 CMakeLists.txt

# 添加可执行程序
add_executable(oj_server src/oj_server.cpp)

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
target_include_directories(oj_server PRIVATE ${CMAKE_SOURCE_DIR}/src/oj_server/include)

# 链接库
target_link_libraries(oj_server PRIVATE pthread mysqlclient jsoncpp ctemplate cpp-httplib)