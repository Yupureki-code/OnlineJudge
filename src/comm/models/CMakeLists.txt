# src/comm/models/CMakeLists.txt
# ODB 模型编译：使用 odb 编译器从 .hxx 生成 -odb.hxx/-odb.ixx/-odb.cxx

# 查找 ODB 编译器
find_program(ODB_COMPILER odb)

if(NOT ODB_COMPILER)
    message(WARNING "ODB compiler not found. Install with: apt install odb")
    # 降级：不生成 ODB 代码，仅暴露头文件
    return()
endif()

# 查找 ODB 运行时库
find_package(PkgConfig REQUIRED)
pkg_check_modules(ODB REQUIRED libodb)
pkg_check_modules(ODB_MYSQL REQUIRED libodb-mysql)

# ODB 生成目录
set(ODB_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)
file(MAKE_DIRECTORY ${ODB_GEN_DIR})

# ODB 模型文件列表
set(ODB_MODELS
    user
    question
    solution
    comment
    submission
    admin
)

# 为每个模型生成 ODB 代码
set(ODB_GEN_SRCS "")
set(ODB_GEN_HDRS "")

foreach(MODEL ${ODB_MODELS})
    set(HXX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL}.hxx)
    set(GEN_HXX ${ODB_GEN_DIR}/${MODEL}-odb.hxx)
    set(GEN_IXX ${ODB_GEN_DIR}/${MODEL}-odb.ixx)
    set(GEN_CXX ${ODB_GEN_DIR}/${MODEL}-odb.cxx)

    add_custom_command(
        OUTPUT ${GEN_HXX} ${GEN_IXX} ${GEN_CXX}
        COMMAND ${ODB_COMPILER}
            -d mysql
            --std c++17
            --generate-query
            --generate-session
            --output-dir ${ODB_GEN_DIR}
            ${HXX_FILE}
        DEPENDS ${HXX_FILE}
        COMMENT "ODB compiling ${MODEL}.hxx"
    )

    list(APPEND ODB_GEN_SRCS ${GEN_CXX})
    list(APPEND ODB_GEN_HDRS ${GEN_HXX})
endforeach()

# 生成目标：make odb
add_custom_target(odb ALL DEPENDS ${ODB_GEN_SRCS} ${ODB_GEN_HDRS})

# 暴露给父 CMakeLists
set(ODB_GEN_DIR ${ODB_GEN_DIR} PARENT_SCOPE)
set(ODB_GEN_SRCS ${ODB_GEN_SRCS} PARENT_SCOPE)
set(ODB_INCLUDE_DIRS ${ODB_INCLUDE_DIRS} ${ODB_MYSQL_INCLUDE_DIRS} PARENT_SCOPE)
set(ODB_LIBRARIES ${ODB_LIBRARIES} ${ODB_MYSQL_LIBRARIES} PARENT_SCOPE)