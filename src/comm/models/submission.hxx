#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

// 正式提交表 — 由 20260715_01_submissions_and_outbox.sql 创建
#pragma db object table("submissions")
struct Submission {
    #pragma db id auto type("BIGINT UNSIGNED")
    uint64_t submission_id;

    #pragma db type("INT UNSIGNED")
    uint32_t user_id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    #pragma db type("LONGTEXT") null
    odb::nullable<std::string> code;

    #pragma db type("VARCHAR(32)") null
    odb::nullable<std::string> language;

    #pragma db type("VARCHAR(32)") default("PENDING")
    std::string status;

    #pragma db type("LONGTEXT") null
    odb::nullable<std::string> result_json;

    #pragma db type("TINYINT(1)") null
    odb::nullable<bool> is_pass;

    #pragma db type("BIGINT UNSIGNED") null
    odb::nullable<uint64_t> time_used_ms;

    #pragma db type("BIGINT UNSIGNED") null
    odb::nullable<uint64_t> memory_used_bytes;

    #pragma db type("TEXT") null
    odb::nullable<std::string> compile_error;

    #pragma db type("DATETIME")
    DateTime created_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> completed_at;

    #pragma db type("BIGINT UNSIGNED") default(0)
    uint64_t result_version;

    #pragma db type("BIGINT UNSIGNED") null
    odb::nullable<uint64_t> legacy_user_submit_id;
};

} // namespace oj::db
