#pragma once

#include <cstdint>
#include <odb/core.hxx>

#include "admin.hxx"
#include "admin_audit_log.hxx"
#include "cache_metrics_snapshot.hxx"
#include "comment.hxx"
#include "legacy_submission.hxx"
#include "question.hxx"
#include "solution.hxx"
#include "user.hxx"

namespace oj::db {

#pragma db view object(User)
struct UserCount {
    #pragma db column("COUNT(" + User::uid + ")")
    uint64_t value;
};

#pragma db view object(Question)
struct QuestionCount {
    #pragma db column("COUNT(" + Question::id + ")")
    uint64_t value;
};

#pragma db view object(Solution)
struct SolutionCount {
    #pragma db column("COUNT(" + Solution::id + ")")
    uint64_t value;
};

#pragma db view object(Comment)
struct CommentCount {
    #pragma db column("COUNT(" + Comment::id + ")")
    uint64_t value;
};

#pragma db view object(AdminAccount)
struct AdminAccountCount {
    #pragma db column("COUNT(" + AdminAccount::admin_id + ")")
    uint64_t value;
};

#pragma db view object(AdminAuditLog)
struct AdminAuditLogCount {
    #pragma db column("COUNT(" + AdminAuditLog::log_id + ")")
    uint64_t value;
};

#pragma db view object(LegacySubmission)
struct LegacySubmissionStats {
    #pragma db column("COUNT(" + LegacySubmission::id + ")")
    uint64_t total_submits;

    #pragma db column("COALESCE(SUM(CASE WHEN " + LegacySubmission::is_pass + " = 1 THEN 1 ELSE 0 END), 0)")
    uint64_t passed_submits;

    #pragma db column("COUNT(DISTINCT CASE WHEN " + LegacySubmission::is_pass + " = 1 THEN " + LegacySubmission::question_id + " END)")
    uint64_t passed_questions;
};

#pragma db view object(CacheMetricsSnapshot)
struct CacheMetricsAggregateView {
    #pragma db column(CacheMetricsSnapshot::action_type)
    uint8_t action_type;

    #pragma db column("COALESCE(SUM(" + CacheMetricsSnapshot::total_requests + "), 0)")
    uint64_t total_requests;

    #pragma db column("COALESCE(SUM(" + CacheMetricsSnapshot::cache_hits + "), 0)")
    uint64_t cache_hits;

    #pragma db column("COALESCE(SUM(" + CacheMetricsSnapshot::db_fallbacks + "), 0)")
    uint64_t db_fallbacks;

    #pragma db column("COALESCE(SUM(" + CacheMetricsSnapshot::total_ms + "), 0)")
    uint64_t total_ms;
};

} // namespace oj::db
