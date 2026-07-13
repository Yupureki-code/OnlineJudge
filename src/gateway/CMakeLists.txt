# src/gateway/CMakeLists.txt

# 查找依赖
find_package(brpc REQUIRED)
find_package(Protobuf REQUIRED)
find_package(jsoncpp REQUIRED)

# Gateway 可执行文件
add_executable(gateway
    main.cpp
    src/router.cpp
    src/jwt_middleware.cpp
    src/jwt_brpc.cpp
    src/json_converter.cpp
    src/rate_limiter.cpp
)

target_include_directories(gateway PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/comm
    ${PROTO_GEN_DIR}
)

target_link_libraries(gateway PRIVATE
    brpc::brpc
    protobuf::libprotobuf
    jsoncpp_lib
)


# 输出到 output/ 目录
set_target_properties(gateway PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output
)