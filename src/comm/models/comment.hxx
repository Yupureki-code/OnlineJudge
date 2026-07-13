#pragma once
#include <odb/core.hxx>
#include <string>
#include <cstdint>

namespace ns_model {

// 评论表 — 对应 sql/solution_comments.sql
#pragma db object table("solution_comments")
struct Comment {
    #pragma db id auto type("INT UNSIGNED")
    uint32_t id;

    #pragma db null
    uint64_t parent_id;  // NULL = 一级评论 (SQL: BIGINT UNSIGNED)

    #pragma db null
    uint32_t reply_to_user_id;

    #pragma db default(0)
    uint32_t like_count;

    #pragma db default(0)
    uint32_t favorite_count;

    uint32_t solution_id;
    uint32_t user_id;

    #pragma db type("TEXT")
    std::string content;

    #pragma db type("TINYINT(1)") default(0)
    bool is_edited;

    #pragma db type("DATETIME") null
    std::string created_at;

    #pragma db type("DATETIME") null
    std::string updated_at;
};

// 评论操作表 — 对应 sql/comment_actions.sql
#pragma db object table("comment_actions")
struct CommentAction {
    #pragma db id auto
    uint64_t id;

    uint32_t comment_id;
    uint32_t user_id;

    #pragma db type("ENUM('like','favorite')")
    std::string action_type;

    #pragma db type("DATETIME") null
    std::string created_at;
};

} // namespace ns_model