find_program(ODB_COMPILER odb)

if(NOT ODB_COMPILER)
    message(FATAL_ERROR "ODB compiler not found; Phase 5 production targets require generated ODB models")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(ODB REQUIRED IMPORTED_TARGET libodb)
pkg_check_modules(ODB_MYSQL REQUIRED IMPORTED_TARGET libodb-mysql)

set(ODB_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
file(MAKE_DIRECTORY "${ODB_GEN_DIR}")

set(ODB_MODELS
    user
    question
    solution
    comment
    submission
    admin
    test_case
    solution_action
    comment_action
    admin_audit_log
    cache_metrics_snapshot
    judge_outbox
    judge_result_receipt
    legacy_submission
    judge_task
    model_counts
)

set(ODB_GEN_SRCS)
set(ODB_GEN_HDRS)
foreach(MODEL IN LISTS ODB_MODELS)
    set(HXX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${MODEL}.hxx")
    set(GEN_HXX "${ODB_GEN_DIR}/${MODEL}-odb.hxx")
    set(GEN_IXX "${ODB_GEN_DIR}/${MODEL}-odb.ixx")
    set(GEN_CXX "${ODB_GEN_DIR}/${MODEL}-odb.cxx")

    add_custom_command(
        OUTPUT "${GEN_HXX}" "${GEN_IXX}" "${GEN_CXX}"
        COMMAND "${ODB_COMPILER}"
            -d mysql
            --std c++20
            --generate-query
            --generate-session
            --output-dir "${ODB_GEN_DIR}"
            "${HXX_FILE}"
        DEPENDS "${HXX_FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/types.hxx"
        COMMENT "ODB compiling ${MODEL}.hxx"
        VERBATIM
    )

    list(APPEND ODB_GEN_SRCS "${GEN_CXX}")
    list(APPEND ODB_GEN_HDRS "${GEN_HXX}")
endforeach()

add_library(oj_odb_models STATIC ${ODB_GEN_SRCS} ${ODB_GEN_HDRS})
target_compile_features(oj_odb_models PUBLIC cxx_std_20)
target_include_directories(oj_odb_models
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${ODB_GEN_DIR}"
)
target_link_libraries(oj_odb_models
    PUBLIC
        PkgConfig::ODB
        PkgConfig::ODB_MYSQL
)

add_custom_target(odb DEPENDS oj_odb_models)
