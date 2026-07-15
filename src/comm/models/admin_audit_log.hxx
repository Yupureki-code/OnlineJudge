#pragma once

#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("admin_audit_logs")
struct AdminAuditLog {
    #pragma db id auto type("BIGINT UNSIGNED")
    uint64_t log_id;

    #pragma db type("VARCHAR(64)")
    std::string request_id;

    #pragma db type("INT UNSIGNED")
    uint32_t operator_admin_id;

    #pragma db type("INT UNSIGNED")
    uint32_t operator_uid;

    #pragma db type("ENUM('super_admin','admin')")
    std::string operator_role;

    #pragma db type("VARCHAR(64)")
    std::string action;

    #pragma db type("VARCHAR(64)")
    std::string resource_type;

    #pragma db type("ENUM('success','denied','failed')")
    std::string result;

    #pragma db type("DATETIME")
    DateTime created_at;

    #pragma db type("LONGTEXT")
    std::string payload_text;
};

} // namespace oj::db
