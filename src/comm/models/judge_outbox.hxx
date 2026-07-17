#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("judge_outbox")
struct JudgeOutbox {
    #pragma db id type("VARCHAR(64)")
    std::string message_id;

    #pragma db type("VARCHAR(16)")
    std::string task_type;

    #pragma db type("BIGINT UNSIGNED") null
    odb::nullable<uint64_t> submission_id;

    #pragma db type("VARCHAR(64)") null
    odb::nullable<std::string> custom_task_id;

    #pragma db type("LONGBLOB")
    std::vector<char> payload;

    #pragma db type("VARCHAR(16)") default("PENDING")
    std::string status;

    #pragma db type("INT UNSIGNED") default(0)
    uint32_t attempt_count;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> next_retry_at;

    #pragma db type("VARCHAR(64)") null
    odb::nullable<std::string> lease_owner;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> lease_expires_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> confirm_deadline;

    #pragma db type("DATETIME")
    DateTime created_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> published_at;
};

} // namespace oj::db
