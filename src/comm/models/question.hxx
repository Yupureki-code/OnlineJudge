#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

// 题目表 — 对应 sql/questions.sql
#pragma db object table("questions")
struct Question {
    #pragma db id type("VARCHAR(5)")
    std::string id;

    #pragma db type("VARCHAR(128)")
    std::string title;

    #pragma db type("TEXT")
    std::string desc;

    #pragma db type("VARCHAR(5)")
    std::string star;  // 难度: 简单/中等/困难

    #pragma db type("DATETIME")
    DateTime create_time;

    #pragma db type("DATETIME")
    DateTime update_time;

    #pragma db type("TINYINT")
    int8_t cpu_limit;

    int32_t memory_limit;
};

} // namespace oj::db
