#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("user_submits")
struct LegacySubmission {
    #pragma db id auto type("BIGINT UNSIGNED")
    odb::nullable<uint64_t> id;

    #pragma db type("INT UNSIGNED")
    uint32_t user_id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    #pragma db type("TEXT")
    std::string result_json;

    #pragma db type("TINYINT(1)")
    bool is_pass;

    #pragma db type("DATETIME")
    DateTime action_time;
};

} // namespace oj::db
