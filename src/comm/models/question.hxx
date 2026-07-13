#pragma once
#include <odb/core.hxx>
#include <string>
#include <cstdint>

namespace ns_model {

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

    #pragma db type("DATETIME") null
    std::string create_time;

    #pragma db type("DATETIME") null
    std::string update_time;

    #pragma db type("TINYINT")
    int32_t cpu_limit;

    int32_t memory_limit;
};

// 测试用例表 — 对应 sql/tests.sql
#pragma db object table("tests")
struct TestCase {
    #pragma db id auto type("TINYINT")
    uint32_t id;

    #pragma db type("CHAR(5)")
    std::string question_id;

    #pragma db column("`in`") type("TEXT")
    std::string input;

    #pragma db column("`out`") type("TEXT")
    std::string output;

    #pragma db type("TINYINT(1)") default(0)
    bool is_sample;
};

} // namespace ns_model