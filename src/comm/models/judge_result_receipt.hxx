#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("judge_result_receipts")
struct JudgeResultReceipt {
    #pragma db id type("VARCHAR(64)")
    std::string message_id;

    #pragma db type("BIGINT UNSIGNED") null
    odb::nullable<uint64_t> submission_id;

    #pragma db type("VARCHAR(64)") null
    odb::nullable<std::string> custom_task_id;

    #pragma db type("LONGBLOB")
    std::vector<char> result_payload;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> applied_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> expires_at;

    #pragma db type("DATETIME")
    DateTime created_at;
};

} // namespace oj::db
