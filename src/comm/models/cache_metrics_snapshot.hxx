#pragma once

#include <cstdint>
#include <string>

#include <odb/core.hxx>

#include "types.hxx"

namespace oj::db {

#pragma db object table("cache_metrics_snapshots")
struct CacheMetricsSnapshot {
    #pragma db id auto type("BIGINT UNSIGNED")
    uint64_t id;

    #pragma db type("TINYINT UNSIGNED")
    uint8_t action_type;

    #pragma db type("BIGINT UNSIGNED") default(0)
    uint64_t total_requests;

    #pragma db type("BIGINT UNSIGNED") default(0)
    uint64_t cache_hits;

    #pragma db type("BIGINT UNSIGNED") default(0)
    uint64_t db_fallbacks;

    #pragma db type("BIGINT UNSIGNED") default(0)
    uint64_t total_ms;

    #pragma db type("DATETIME")
    DateTime created_at;
};

} // namespace oj::db
