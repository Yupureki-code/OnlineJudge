find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)

add_executable(oj_server_legacy_contract_test legacy_contract_test.cpp)
add_executable(oj_admin_legacy_contract_test admin_contract_test.cpp)
add_executable(application_context_test application_context_test.cpp)

target_compile_features(oj_server_legacy_contract_test PRIVATE cxx_std_17)
target_compile_features(oj_admin_legacy_contract_test PRIVATE cxx_std_17)
target_compile_features(application_context_test PRIVATE cxx_std_20)
target_include_directories(oj_server_legacy_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(oj_admin_legacy_contract_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(application_context_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(oj_server_legacy_contract_test PRIVATE phase3_proto_contract)
target_link_libraries(oj_admin_legacy_contract_test PRIVATE phase3_proto_contract)
target_link_libraries(application_context_test PRIVATE oj_runtime)

set(OJ_SERVER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/oj_server/include)
set(OJ_RPC_TEST_LIBRARIES
    oj_runtime phase3_proto_contract protobuf::libprotobuf oj_odb_models
    pthread jsoncpp ctemplate spdlog::spdlog CURL::libcurl
    ${REDIS_PLUS_PLUS_LIB} ${HIREDIS_LIB}
    ${OJ_SERVER_BRPC} ${OJ_SERVER_PROTOC} ${OJ_SERVER_GFLAGS} ${OJ_SERVER_LEVELDB}
    OpenSSL::SSL OpenSSL::Crypto dl z event event_pthreads
)

add_executable(brpc_adapter_test brpc_adapter_test.cpp)
add_executable(oj_service_rpc_test
    oj_service_rpc_test.cpp
    ${CMAKE_SOURCE_DIR}/src/oj_server/src/rpc/oj_service_impl.cpp
)
add_executable(oj_admin_service_rpc_test
    oj_admin_service_rpc_test.cpp
    ${CMAKE_SOURCE_DIR}/src/oj_server/src/rpc/admin_audit_worker.cpp
    ${CMAKE_SOURCE_DIR}/src/oj_server/src/rpc/oj_admin_service_impl.cpp
)
add_executable(static_file_service_test
    static_file_service_test.cpp
    ${CMAKE_SOURCE_DIR}/src/oj_server/src/rpc/static_http_service.cpp
)

foreach(target brpc_adapter_test oj_service_rpc_test oj_admin_service_rpc_test static_file_service_test)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${OJ_SERVER_INCLUDE_DIR}
        ${PROTO_GEN_DIR}
        ${CMAKE_SOURCE_DIR}/src/comm
    )
    target_link_libraries(${target} PRIVATE ${OJ_RPC_TEST_LIBRARIES})
    add_dependencies(${target} generate_phase3_proto)
endforeach()
