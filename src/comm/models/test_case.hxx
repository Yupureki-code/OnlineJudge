#pragma once

#include <cstdint>
#include <string>

#include <odb/core.hxx>

namespace oj::db {

#pragma db object table("tests")
struct TestCase {
    #pragma db id auto type("BIGINT UNSIGNED")
    uint64_t test_case_id;

    #pragma db type("TINYINT")
    int8_t id;

    #pragma db type("CHAR(5)")
    std::string question_id;

    #pragma db column("in") type("TEXT")
    std::string input;

    #pragma db column("out") type("TEXT")
    std::string output;

    #pragma db type("TINYINT(1)") default(0)
    bool is_sample;
};

} // namespace oj::db
