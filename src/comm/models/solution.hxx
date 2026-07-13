#pragma once
#include <odb/core.hxx>
#include <string>
#include <cstdint>

namespace ns_model {

// 题解表 — 对应 sql/solutions.sql
#pragma db object table("solutions")
struct Solution {
    #pragma db id auto
    uint32_t id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    uint32_t user_id;

    #pragma db type("VARCHAR(30)")
    std::string title;

    #pragma db type("TEXT")
    std::string content_md;

    #pragma db default(0)
    uint32_t like_count;

    #pragma db default(0)
    uint32_t favorite_count;

    #pragma db default(0)
    uint32_t comment_count;

    #pragma db type("ENUM('rejected','pending','approved')")
    std::string status;

    #pragma db type("DATETIME") null
    std::string created_at;

    #pragma db type("DATETIME") null
    std::string updated_at;
};

// 题解操作表 — 对应 sql/solution_actions.sql
#pragma db object table("solution_actions")
struct SolutionAction {
    #pragma db id auto
    uint32_t id;

    uint32_t solution_id;
    uint32_t user_id;

    #pragma db type("ENUM('like','comment','favorite')")
    std::string action_type;

    #pragma db type("DATETIME") null
    std::string created_at;
};

} // namespace ns_model