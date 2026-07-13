# proto/CMakeLists.txt
# 使用 protoc 生成 C++ 代码

# 查找 protoc 编译器
find_program(PROTOC_COMPILER protoc REQUIRED)

# Protobuf 源文件
set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/common.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/biz_service.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/judge_service.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/gateway.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/mq_message.proto
)

# 生成目录
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

# 为每个 .proto 生成 .pb.h 和 .pb.cc
set(PROTO_GEN_SRCS "")
set(PROTO_GEN_HDRS "")

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GEN_SRC ${PROTO_GEN_DIR}/${PROTO_NAME}.pb.cc)
    set(GEN_HDR ${PROTO_GEN_DIR}/${PROTO_NAME}.pb.h)

    add_custom_command(
        OUTPUT ${GEN_SRC} ${GEN_HDR}
        COMMAND ${PROTOC_COMPILER}
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${PROTO_GEN_DIR}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Protobuf compiling ${PROTO_NAME}.proto"
    )

    list(APPEND PROTO_GEN_SRCS ${GEN_SRC})
    list(APPEND PROTO_GEN_HDRS ${GEN_HDR})
endforeach()

# 生成目标：make proto
add_custom_target(proto ALL DEPENDS ${PROTO_GEN_SRCS} ${PROTO_GEN_HDRS})

# 暴露给父 CMakeLists 使用
set(PROTO_GEN_DIR ${PROTO_GEN_DIR} PARENT_SCOPE)
set(PROTO_GEN_SRCS ${PROTO_GEN_SRCS} PARENT_SCOPE)
set(PROTO_GEN_HDRS ${PROTO_GEN_HDRS} PARENT_SCOPE)