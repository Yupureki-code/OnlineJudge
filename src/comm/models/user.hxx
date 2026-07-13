#pragma once
#include <odb/core.hxx>
#include <string>
#include <cstdint>

namespace ns_model {

// 用户表 — 对应 sql/users.sql
#pragma db object table("users")
struct User {
    #pragma db id auto type("INT UNSIGNED")
    uint32_t uid;

    #pragma db type("VARCHAR(20)")
    std::string name;

    #pragma db type("VARCHAR(255)")
    std::string password_hash;

    #pragma db type("VARCHAR(50)")
    std::string email;

    #pragma db type("DATETIME") null
    std::string create_time;

    #pragma db type("DATETIME") null
    std::string last_login;

    #pragma db type("VARCHAR(32)")
    std::string password_algo;

    #pragma db type("DATETIME") null
    std::string password_update_at;

    // role 字段在 admin_accounts 表中，users 表无此字段
    // 头像字段（迁移 20260428_0001 添加）
    #pragma db null
    std::string avatar;
};

} // namespace ns_model