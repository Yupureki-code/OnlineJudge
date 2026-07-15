#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

// 评论表 — 对应 sql/solution_comments.sql
#pragma db object table("solution_comments")
struct Comment {
    #pragma db id auto type("INT UNSIGNED")
    uint32_t id;

    #pragma db type("BIGINT UNSIGNED") null
    odb::nullable<uint64_t> parent_id;

    #pragma db type("INT UNSIGNED") null
    odb::nullable<uint32_t> reply_to_user_id;

    #pragma db default(0)
    uint32_t like_count;

    #pragma db default(0)
    uint32_t favorite_count;

    uint32_t solution_id;
    uint32_t user_id;

    #pragma db type("TEXT")
    std::string content;

    #pragma db type("TINYINT(1)") null
    odb::nullable<bool> is_edited;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> created_at;

    #pragma db type("DATETIME") null
    odb::nullable<DateTime> updated_at;
};

} // namespace oj::db
