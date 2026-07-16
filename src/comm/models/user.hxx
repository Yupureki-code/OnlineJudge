#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>
#include <odb/nullable.hxx>

#include "types.hxx"

namespace oj::db {

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

    #pragma db type("DATETIME")
    DateTime create_time;

    #pragma db type("DATETIME")
    DateTime last_login;

    #pragma db type("VARCHAR(32)")
    std::string password_algo;

    #pragma db type("DATETIME")
    DateTime password_update_at;

    #pragma db column("avatar") type("VARCHAR(255)") null
    odb::nullable<std::string> avatar_path;
};

} // namespace oj::db
