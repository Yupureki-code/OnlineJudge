#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("judge_tasks")
struct JudgeTask {
    #pragma db id auto type("BIGINT UNSIGNED")
    uint64_t id;

    #pragma db type("BIGINT UNSIGNED")
    uint64_t submission_id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    #pragma db type("INT UNSIGNED")
    uint32_t user_id;

    #pragma db type("ENUM('QUEUED','JUDGING','COMPLETED','FAILED')") default("QUEUED")
    std::string status;

    #pragma db type("VARCHAR(64)") null
    odb::nullable<std::string> worker_id;

    #pragma db type("DATETIME")
    DateTime queued_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> started_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> completed_at;

    #pragma db type("TEXT") null
    odb::nullable<std::string> result_json;

    #pragma db type("TEXT") null
    odb::nullable<std::string> error_message;
};

} // namespace oj::db
