# src/judge_service/CMakeLists.txt

# C++20（协程支持）
set(CMAKE_CXX_STANDARD 20)

# 查找依赖
find_package(brpc REQUIRED)
find_package(Protobuf REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)

# Judge Service 可执行文件
add_executable(judge_service
    main.cpp
)

target_include_directories(judge_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
    ${LIBEVENT_INCLUDE_DIRS}
)

target_link_libraries(judge_service PRIVATE
    brpc::brpc
    protobuf::libprotobuf
    jsoncpp_lib
    curl
    ${LIBEVENT_LIBRARIES}
)

# 输出到 output/ 目录
set_target_properties(judge_service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)