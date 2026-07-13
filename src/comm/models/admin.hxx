#pragma once
#include <odb/core.hxx>
#include <string>
#include <cstdint>

namespace ns_model {

// 管理员账户表 — 对应 sql/admin_accounts.sql
#pragma db object table("admin_accounts")
struct AdminAccount {
    #pragma db id auto
    uint32_t admin_id;

    uint32_t uid;

    #pragma db type("VARCHAR(255)")
    std::string password_hash;

    #pragma db type("ENUM('super_admin','admin')")
    std::string role;

    #pragma db type("DATETIME") null
    std::string created_at;
};

// 管理员审计日志表 — 对应 sql/admin_audit_logs.sql
#pragma db object table("admin_audit_logs")
struct AdminAuditLog {
    #pragma db id auto
    uint64_t log_id;

    #pragma db type("VARCHAR(64)")
    std::string request_id;

    uint32_t operator_admin_id;

    #pragma db type("ENUM('super_admin','admin')")
    std::string operator_role;

    #pragma db type("VARCHAR(64)")
    std::string action;

    #pragma db type("VARCHAR(64)")
    std::string resource_type;

    #pragma db type("ENUM('success','denied','failed')")
    std::string result;

    #pragma db type("DATETIME") null
    std::string created_at;

    #pragma db type("LONGTEXT")
    std::string payload_text;
};

// 缓存指标快照表 — 对应 sql/cache_metrics_snapshots.sql
#pragma db object table("cache_metrics_snapshots")
struct CacheMetricsSnapshot {
    #pragma db id auto
    uint64_t id;

    #pragma db type("TINYINT UNSIGNED")
    uint32_t action_type;

    #pragma db default(0)
    uint64_t total_requests;

    #pragma db default(0)
    uint64_t cache_hits;

    #pragma db default(0)
    uint64_t db_fallbacks;

    #pragma db default(0)
    uint64_t total_ms;

    #pragma db type("DATETIME") null
    std::string created_at;
};

} // namespace ns_model