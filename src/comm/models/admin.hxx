#pragma once
#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

// 管理员账户表 — 对应 sql/admin_accounts.sql
#pragma db object table("admin_accounts")
struct AdminAccount {
    #pragma db id auto type("INT UNSIGNED")
    uint32_t admin_id;

    #pragma db type("INT UNSIGNED")
    uint32_t uid;

    #pragma db type("VARCHAR(255)")
    std::string password_hash;

    #pragma db type("ENUM('super_admin','admin')")
    std::string role;

    #pragma db type("DATETIME")
    DateTime created_at;
};

} // namespace oj::db
