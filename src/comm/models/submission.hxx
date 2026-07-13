#pragma once
#include <odb/core.hxx>
#include <string>
#include <cstdint>

namespace ns_model {

// 用户提交记录表 — 对应 sql/user_submits.sql
#pragma db object table("user_submits")
struct UserSubmit {
    #pragma db id auto
    uint32_t id;

    uint32_t user_id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    #pragma db type("TEXT")
    std::string result_json;

    #pragma db type("BIT(1)")
    bool is_pass;

    #pragma db type("DATETIME") null
    std::string action_time;
};

// 判题任务追踪表 — 对应 sql/judge_tasks.sql
#pragma db object table("judge_tasks")
struct JudgeTask {
    #pragma db id auto
    uint64_t id;

    uint64_t submission_id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    uint32_t user_id;

    #pragma db type("ENUM('QUEUED','JUDGING','COMPLETED','FAILED')") default("QUEUED")
    std::string status;

    #pragma db type("VARCHAR(64)") null
    std::string worker_id;

    #pragma db type("DATETIME") null
    std::string queued_at;

    #pragma db type("DATETIME") null
    std::string started_at;

    #pragma db type("DATETIME") null
    std::string completed_at;

    #pragma db type("TEXT") null
    std::string result_json;

    #pragma db type("TEXT") null
    std::string error_message;
};

} // namespace ns_model