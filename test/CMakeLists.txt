add_subdirectory(oj_server)

if(TARGET oj_odb_models)
    add_executable(odb_model_mapping_test odb_model_mapping_test.cpp)
    target_compile_features(odb_model_mapping_test PRIVATE cxx_std_20)
    target_link_libraries(odb_model_mapping_test PRIVATE oj_odb_models)
endif()

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(spdlog REQUIRED)
pkg_check_modules(ODB_TEST REQUIRED IMPORTED_TARGET libodb)
pkg_check_modules(ODB_MYSQL_TEST REQUIRED IMPORTED_TARGET libodb-mysql)

if(TARGET oj_odb_models)
    add_subdirectory(model)
endif()

add_executable(latency_monitor_test latency_monitor_test.cpp)
target_compile_features(latency_monitor_test PRIVATE cxx_std_17)
target_include_directories(latency_monitor_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(latency_monitor_test PRIVATE Threads::Threads spdlog::spdlog)

add_executable(logger_test logger_test.cpp)
target_compile_features(logger_test PRIVATE cxx_std_17)
target_include_directories(logger_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(logger_test PRIVATE Threads::Threads spdlog::spdlog)

add_executable(odb_connection_pool_test odb_connection_pool_test.cpp)
target_compile_features(odb_connection_pool_test PRIVATE cxx_std_17)
target_include_directories(odb_connection_pool_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(odb_connection_pool_test PRIVATE
    Threads::Threads
    spdlog::spdlog
    PkgConfig::ODB_TEST
    PkgConfig::ODB_MYSQL_TEST
)
