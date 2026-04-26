include(CMakePackageConfigHelpers)

option(USE_SPDLOG "Enable spdlog backend for Logger" ON)

file(GLOB LOGGER_SRC "src/*.cpp")

file(GLOB LOGGER_PUBLIC_HEADERS
    "include/*.h"
    "include/*.hpp"
)

set(LOGGER_PUBLIC_INCLUDE_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/output/include/Logger")
file(MAKE_DIRECTORY "${LOGGER_PUBLIC_INCLUDE_OUTPUT_DIR}")
foreach(header ${LOGGER_PUBLIC_HEADERS})
    get_filename_component(header_name "${header}" NAME)
    configure_file("${header}" "${LOGGER_PUBLIC_INCLUDE_OUTPUT_DIR}/${header_name}" COPYONLY)
endforeach()

add_library(Logger_static STATIC ${LOGGER_SRC})
add_library(Logger_shared SHARED ${LOGGER_SRC})
add_library(Logger ALIAS Logger_shared)

add_library(Logger::Logger_static ALIAS Logger_static)
add_library(Logger::Logger_shared ALIAS Logger_shared)

if(USE_SPDLOG)
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        target_compile_definitions(Logger_static PRIVATE LOGGER_USE_SPDLOG=1)
        target_compile_definitions(Logger_shared PRIVATE LOGGER_USE_SPDLOG=1)
        target_link_libraries(Logger_static PUBLIC spdlog::spdlog)
        target_link_libraries(Logger_shared PUBLIC spdlog::spdlog)
        message(STATUS "Logger: using spdlog backend")
    else()
        message(WARNING "Logger: USE_SPDLOG=ON but spdlog not found, fallback to legacy backend")
    endif()
endif()

target_include_directories(Logger_static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/output/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(Logger_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/output/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Logger_static PUBLIC pthread)
target_link_libraries(Logger_shared PUBLIC pthread)

set_target_properties(Logger_static PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output/lib
    OUTPUT_NAME Logger
    VERSION 1.0.0
    SOVERSION 1
)

set_target_properties(Logger_shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/output/lib
    OUTPUT_NAME Logger
    VERSION 1.0.0
    SOVERSION 1
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS Logger_static Logger_shared
    EXPORT LoggerTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(DIRECTORY ${LOGGER_PUBLIC_INCLUDE_OUTPUT_DIR}/
        DESTINATION include/Logger
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")

install(EXPORT LoggerTargets
    FILE LoggerTargets.cmake
    NAMESPACE Logger::
    DESTINATION lib/cmake/Logger)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LoggerConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/LoggerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LoggerConfig.cmake
    INSTALL_DESTINATION lib/cmake/Logger
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LoggerConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LoggerConfigVersion.cmake
    DESTINATION lib/cmake/Logger
)