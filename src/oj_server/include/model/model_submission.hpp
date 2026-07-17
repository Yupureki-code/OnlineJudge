#pragma once

#include "model_base.hpp"

#include "../../../comm/models/odb_models.hpp"
#include "../../../comm/proto/biz_service.pb.h"
#include "../../../comm/proto/common.pb.h"
#include "../../../comm/proto/mq_message.pb.h"

#include <google/protobuf/util/json_util.h>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>
#include <odb/mysql/connection.hxx>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace oj::model
{

class ModelSubmission : public ModelBase
{
public:
    enum class ApplyOutcome
    {
        Persisted,
        Idempotent,
        Conflict,
        NotFound,
        RetryableFailure
    };

    bool CreateSubmissionWithOutbox(uint32_t user_id,
                                    const std::string& question_id,
                                    const std::string& code,
                                    const std::string& language,
                                    oj::mq::JudgeTaskMessage task,
                                    oj::db::Submission* created)
    {
        if (created == nullptr || user_id == 0 || question_id.empty() ||
            task.message_id().empty() || task.has_custom_task_id()) return false;
        try {
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            oj::db::Submission submission{};
            submission.user_id = user_id;
            submission.question_id = question_id;
            submission.code = code;
            submission.language = language;
            submission.status = "QUEUED";
            submission.created_at = Now();
            submission.result_version = 0;
            database->persist(submission);

            task.set_submission_id(submission.submission_id);
            std::string payload;
            if (!task.SerializeToString(&payload)) return false;
            oj::db::JudgeOutbox outbox{};
            outbox.message_id = task.message_id();
            outbox.task_type = "submission";
            outbox.submission_id = submission.submission_id;
            outbox.payload.assign(payload.begin(), payload.end());
            outbox.status = "PENDING";
            outbox.attempt_count = 0;
            outbox.created_at = submission.created_at;
            database->persist(outbox);
            transaction.commit();
            *created = std::move(submission);
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("CreateSubmissionWithOutbox failed: {}", error.what());
            return false;
        }
    }

    bool CreateCustomOutbox(oj::mq::JudgeTaskMessage task)
    {
        if (task.message_id().empty() || !task.has_custom_task_id() ||
            task.custom_task_id().empty()) return false;
        try {
            std::string payload;
            if (!task.SerializeToString(&payload)) return false;
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            oj::db::JudgeOutbox outbox{};
            outbox.message_id = task.message_id();
            outbox.task_type = "custom";
            outbox.custom_task_id = task.custom_task_id();
            outbox.payload.assign(payload.begin(), payload.end());
            outbox.status = "PENDING";
            outbox.attempt_count = 0;
            outbox.created_at = Now();
            database->persist(outbox);
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("CreateCustomOutbox failed: {}", error.what());
            return false;
        }
    }

    bool GetSubmission(uint64_t submission_id, oj::db::Submission* submission)
    {
        if (submission == nullptr || submission_id == 0) return false;
        try {
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            std::unique_ptr<oj::db::Submission> found(database->find<oj::db::Submission>(submission_id));
            if (!found) return false;
            *submission = *found;
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("GetSubmission failed: {}", error.what());
            return false;
        }
    }

    bool ListSubmissions(uint32_t user_id, const std::string& question_id,
                         const std::string& status, uint32_t page, uint32_t page_size,
                         std::vector<oj::db::Submission>* submissions, uint64_t* total)
    {
        if (submissions == nullptr || total == nullptr || user_id == 0 || page == 0 ||
            page_size == 0 || (!question_id.empty() && !IsAllDigits(question_id)) ||
            !ValidStatusFilter(status)) return false;
        submissions->clear();
        *total = 0;
        try {
            using Query = odb::query<oj::db::Submission>;
            Query query(Query::user_id == user_id);
            if (!question_id.empty()) query = query && Query::question_id == question_id;
            if (!status.empty()) query = query && Query::status == status;
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            auto* connection = dynamic_cast<odb::mysql::connection*>(&transaction.connection());
            if (connection == nullptr || !CountSubmissions(connection->handle(), user_id,
                    question_id, status, total)) return false;
            const uint64_t offset = static_cast<uint64_t>(page - 1) * page_size;
            query += " ORDER BY submission_id DESC LIMIT " + std::to_string(page_size) +
                     " OFFSET " + std::to_string(offset);
            odb::result<oj::db::Submission> result = database->query<oj::db::Submission>(query, false);
            submissions->reserve(page_size);
            for (const auto& row : result) submissions->push_back(row);
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("ListSubmissions failed: {}", error.what());
            return false;
        }
    }

    bool ClaimPendingOutbox(std::size_t limit, const std::string& owner,
                            uint32_t lease_seconds, uint32_t confirm_timeout_seconds,
                            std::vector<oj::db::JudgeOutbox>* rows)
    {
        if (rows == nullptr || limit == 0 || owner.empty()) return false;
        rows->clear();
        try {
            using Query = odb::query<oj::db::JudgeOutbox>;
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            const int64_t now_micros = NowMicros();
            const auto now = oj::util::TimeUtil::IntToDateTime(now_micros);
            Query claimable =
                (Query::status == "PENDING" &&
                 (Query::next_retry_at.is_null() || Query::next_retry_at <= now)) ||
                (Query::status == "SENDING" &&
                 (Query::lease_expires_at.is_null() || Query::lease_expires_at <= now));
            claimable += " ORDER BY created_at ASC, message_id ASC LIMIT " +
                         std::to_string(limit) + " FOR UPDATE SKIP LOCKED";
            odb::result<oj::db::JudgeOutbox> result = database->query<oj::db::JudgeOutbox>(
                claimable, true);
            for (auto row : result) {
                const bool expired = now_micros - oj::util::TimeUtil::DateTimeToInt(row.created_at) >
                                     kOutboxMaxAgeMicros;
                if (expired || row.attempt_count >= kOutboxMaxAttempts) {
                    MarkOutboxTerminal(*database, &row, expired ? "OUTBOX_EXPIRED" : "OUTBOX_RETRIES_EXHAUSTED");
                    continue;
                }
                row.status = "SENDING";
                row.lease_owner = owner;
                row.lease_expires_at = oj::util::TimeUtil::IntToDateTime(
                    now_micros + static_cast<int64_t>(lease_seconds) * 1000000);
                row.confirm_deadline = oj::util::TimeUtil::IntToDateTime(
                    now_micros + static_cast<int64_t>(confirm_timeout_seconds) * 1000000);
                row.next_retry_at.reset();
                ++row.attempt_count;
                database->update(row);
                rows->push_back(std::move(row));
                if (rows->size() >= limit) break;
            }
            transaction.commit();
            RecoverCustomTaskCache(*rows);
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("FetchPendingOutbox failed: {}", error.what());
            return false;
        }
    }

    bool RecoverCustomTaskCaches()
    {
        try {
            using Query = odb::query<oj::db::JudgeOutbox>;
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            odb::result<oj::db::JudgeOutbox> result = database->query<oj::db::JudgeOutbox>(
                Query::task_type == "custom" &&
                Query::created_at >= oj::util::TimeUtil::IntToDateTime(NowMicros() - 600LL * 1000000),
                false);
            std::vector<oj::db::JudgeOutbox> rows(result.begin(), result.end());
            transaction.commit();
            RecoverCustomTaskCache(rows);
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("RecoverCustomTaskCaches failed: {}", error.what());
            return false;
        }
    }

    bool CleanupExpiredResultReceipts()
    {
        try {
            using Query = odb::query<oj::db::JudgeResultReceipt>;
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            database->erase_query<oj::db::JudgeResultReceipt>(
                Query::expires_at < oj::util::TimeUtil::IntToDateTime(NowMicros()));
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("CleanupExpiredResultReceipts failed: {}", error.what());
            return false;
        }
    }

    bool MarkOutboxPublished(const std::string& message_id, const std::string& owner)
    {
        return UpdateOutbox(message_id, owner, true, 0);
    }

    bool MarkOutboxFailed(const std::string& message_id, const std::string& owner,
                          uint32_t retry_delay_seconds)
    {
        return UpdateOutbox(message_id, owner, false, retry_delay_seconds);
    }

    bool GetOutbox(const std::string& message_id, oj::db::JudgeOutbox* outbox)
    {
        if (message_id.empty() || outbox == nullptr) return false;
        try {
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            std::unique_ptr<oj::db::JudgeOutbox> row(database->find<oj::db::JudgeOutbox>(message_id));
            if (!row) return false;
            *outbox = *row;
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("GetOutbox failed: {}", error.what());
            return false;
        }
    }

    bool GetResultReceipt(const std::string& message_id, oj::db::JudgeResultReceipt* receipt)
    {
        if (message_id.empty() || receipt == nullptr) return false;
        try {
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            std::unique_ptr<oj::db::JudgeResultReceipt> row(
                database->find<oj::db::JudgeResultReceipt>(message_id));
            if (!row) return false;
            *receipt = *row;
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("GetResultReceipt failed: {}", error.what());
            return false;
        }
    }

    ApplyOutcome ApplyJudgeResult(const oj::biz::UpdateJudgeResultRequest& request,
                                  const oj::common::JudgeResult& result,
                                  const oj::db::DateTime& custom_expires_at)
    {
        try {
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            std::unique_ptr<oj::db::JudgeResultReceipt> existing(
                database->find<oj::db::JudgeResultReceipt>(request.message_id()));
            const std::vector<char> payload(request.result_payload().begin(), request.result_payload().end());
            if (existing) {
                return ReceiptMatches(*existing, request, payload)
                    ? ApplyOutcome::Idempotent : ApplyOutcome::Conflict;
            }

            oj::db::JudgeResultReceipt receipt{};
            receipt.message_id = request.message_id();
            receipt.result_payload = payload;
            receipt.created_at = Now();
            receipt.applied_at = receipt.created_at;
            if (request.has_submission_id()) {
                std::unique_ptr<oj::db::Submission> submission(
                    database->find<oj::db::Submission>(request.submission_id()));
                if (!submission) return ApplyOutcome::NotFound;
                receipt.submission_id = request.submission_id();
                submission->status = StatusName(result.status());
                std::string result_json;
                if (!google::protobuf::util::MessageToJsonString(result, &result_json).ok())
                    return ApplyOutcome::RetryableFailure;
                submission->result_json = result_json;
                submission->is_pass = result.status() == oj::common::SUBMISSION_STATUS_ACCEPTED;
                submission->time_used_ms = result.time_used_ms();
                submission->memory_used_bytes = result.memory_used_bytes();
                if (!result.compile_error().empty()) submission->compile_error = result.compile_error();
                submission->completed_at = result.completed_at() > 0
                    ? oj::util::TimeUtil::IntToDateTime(result.completed_at() * 1000000)
                    : receipt.created_at;
                ++submission->result_version;
                database->update(*submission);
                oj::db::LegacySubmission history{};
                history.user_id = submission->user_id;
                history.question_id = submission->question_id;
                history.result_json = result_json;
                history.is_pass = submission->is_pass;
                history.action_time = receipt.created_at;
                database->persist(history);
            } else if (request.has_custom_task_id()) {
                receipt.custom_task_id = request.custom_task_id();
                receipt.expires_at = custom_expires_at;
            } else {
                return ApplyOutcome::Conflict;
            }
            database->persist(receipt);
            transaction.commit();
            return ApplyOutcome::Persisted;
        } catch (const std::exception& error) {
            LOG_ERROR("ApplyJudgeResult failed: {}", error.what());
            oj::db::JudgeResultReceipt existing{};
            if (GetResultReceipt(request.message_id(), &existing))
                return ReceiptMatches(existing, request,
                    std::vector<char>(request.result_payload().begin(), request.result_payload().end()))
                    ? ApplyOutcome::Idempotent : ApplyOutcome::Conflict;
            return ApplyOutcome::RetryableFailure;
        }
    }

private:
    static bool ValidStatusFilter(const std::string& status)
    {
        return status.empty() || (status.size() <= 32 &&
            std::all_of(status.begin(), status.end(), [](unsigned char ch) {
                return std::isupper(ch) != 0 || std::isdigit(ch) != 0 || ch == '_';
            }));
    }

    static bool CountSubmissions(MYSQL* connection, uint32_t user_id,
                                 const std::string& question_id,
                                 const std::string& status, uint64_t* total)
    {
        if (connection == nullptr || total == nullptr) return false;
        std::string sql = "SELECT COUNT(*) FROM submissions WHERE user_id=" +
                          std::to_string(user_id);
        if (!question_id.empty()) sql += " AND question_id='" + question_id + "'";
        if (!status.empty()) sql += " AND status='" + status + "'";
        if (mysql_real_query(connection, sql.data(), sql.size()) != 0) return false;
        std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
            mysql_store_result(connection), mysql_free_result);
        if (!result) return false;
        MYSQL_ROW row = mysql_fetch_row(result.get());
        if (row == nullptr || row[0] == nullptr) return false;
        char* end = nullptr;
        errno = 0;
        const unsigned long long value = std::strtoull(row[0], &end, 10);
        if (errno != 0 || end == row[0] || *end != '\0') return false;
        *total = value;
        return true;
    }

    static bool ReceiptMatches(const oj::db::JudgeResultReceipt& receipt,
                               const oj::biz::UpdateJudgeResultRequest& request,
                               const std::vector<char>& payload)
    {
        const bool same_target =
            (request.has_submission_id() && receipt.submission_id &&
             receipt.submission_id.get() == request.submission_id()) ||
            (request.has_custom_task_id() && receipt.custom_task_id &&
             receipt.custom_task_id.get() == request.custom_task_id());
        return same_target && receipt.result_payload == payload;
    }

    void RecoverCustomTaskCache(const std::vector<oj::db::JudgeOutbox>& rows)
    {
        const int64_t now = std::time(nullptr);
        for (const auto& row : rows) {
            const std::string payload(row.payload.begin(), row.payload.end());
            oj::mq::JudgeTaskMessage task;
            if (!task.ParseFromString(payload) || !task.has_custom_task_id()) continue;
            const int ttl = static_cast<int>(task.created_at() + 600 - now);
            if (ttl <= 0) continue;
            const std::string key = "judge:custom:" + task.custom_task_id();
            std::string cached;
            if (_cache.GetStringByAnyKey(key, &cached)) continue;
            Json::Value state;
            state["task_id"] = task.custom_task_id();
            state["message_id"] = task.message_id();
            state["user_id"] = task.user_id();
            state["status"] = "QUEUED";
            state["expires_at"] = Json::Int64(task.created_at() + 600);
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "";
            _cache.SetStringByAnyKey(key, Json::writeString(writer, state), ttl);
        }
    }

    static int64_t NowMicros()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    static oj::db::DateTime Now()
    {
        return oj::util::TimeUtil::IntToDateTime(NowMicros());
    }

    static std::string StatusName(oj::common::SubmissionStatus status)
    {
        const auto* value = oj::common::SubmissionStatus_descriptor()->FindValueByNumber(status);
        if (value == nullptr) return "SYSTEM_ERROR";
        std::string name = value->name();
        constexpr std::string_view prefix = "SUBMISSION_STATUS_";
        if (name.rfind(prefix, 0) == 0) name.erase(0, prefix.size());
        return name;
    }

    static void MarkOutboxTerminal(odb::database& database, oj::db::JudgeOutbox* row,
                                   const std::string& reason)
    {
        row->status = "DEAD";
        row->lease_owner.reset();
        row->lease_expires_at.reset();
        row->confirm_deadline.reset();
        database.update(*row);
        if (!row->submission_id) return;
        std::unique_ptr<oj::db::Submission> submission(
            database.find<oj::db::Submission>(row->submission_id.get()));
        if (!submission || (submission->status != "QUEUED" && submission->status != "JUDGING")) return;
        submission->status = "SYSTEM_ERROR";
        submission->result_json = std::string("{\"status\":\"SYSTEM_ERROR\",\"reason\":\"") +
                                  reason + "\"}";
        submission->is_pass = false;
        submission->completed_at = Now();
        ++submission->result_version;
        database.update(*submission);
    }

    bool UpdateOutbox(const std::string& message_id, const std::string& owner,
                      bool published, uint32_t retry_delay_seconds)
    {
        if (message_id.empty() || owner.empty()) return false;
        try {
            auto database = AcquireDatabase();
            odb::transaction transaction(database->begin());
            std::unique_ptr<oj::db::JudgeOutbox> row(database->find<oj::db::JudgeOutbox>(message_id));
            if (!row) return false;
            if (row->status != "SENDING" || !row->lease_owner || row->lease_owner.get() != owner)
                return false;
            if (published) {
                row->status = "PUBLISHED";
                row->published_at = Now();
                row->next_retry_at.reset();
            } else {
                row->status = "PENDING";
                row->next_retry_at = oj::util::TimeUtil::IntToDateTime(
                    NowMicros() + static_cast<int64_t>(retry_delay_seconds) * 1000000);
            }
            row->lease_owner.reset();
            row->lease_expires_at.reset();
            row->confirm_deadline.reset();
            database->update(*row);
            transaction.commit();
            return true;
        } catch (const std::exception& error) {
            LOG_ERROR("UpdateOutbox failed: {}", error.what());
            return false;
        }
    }

    static constexpr int64_t kOutboxMaxAgeMicros = 15LL * 60 * 1000000;
    static constexpr uint32_t kOutboxMaxAttempts = 20;
};

} // namespace oj::model
