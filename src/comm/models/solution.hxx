#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

// 题解表 — 对应 sql/solutions.sql
#pragma db object table("solutions")
struct Solution {
    #pragma db id auto type("INT UNSIGNED")
    uint32_t id;

    #pragma db type("VARCHAR(5)")
    std::string question_id;

    #pragma db type("INT UNSIGNED")
    uint32_t user_id;

    #pragma db type("VARCHAR(30)")
    std::string title;

    #pragma db type("TEXT")
    std::string content_md;

    #pragma db type("INT UNSIGNED") default(0)
    uint32_t like_count;

    #pragma db type("INT UNSIGNED") default(0)
    uint32_t favorite_count;

    #pragma db type("INT UNSIGNED") default(0)
    uint32_t comment_count;

    #pragma db type("ENUM('rejected','pending','approved')")
    std::string status;

    #pragma db type("DATETIME")
    DateTime created_at;

    #pragma db type("DATETIME")
    DateTime updated_at;
};

} // namespace oj::db
