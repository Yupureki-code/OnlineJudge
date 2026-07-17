#include "model/model_submission.hpp"

#include <odb/mysql/database.hxx>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

void Cleanup(odb::mysql::database& database, uint32_t user_id, const std::string& question_id,
             const std::string& formal_message, const std::string& custom_message,
             uint64_t submission_id)
{
    odb::transaction transaction(database.begin());
    using ReceiptQuery = odb::query<oj::db::JudgeResultReceipt>;
    using OutboxQuery = odb::query<oj::db::JudgeOutbox>;
    using LegacyQuery = odb::query<oj::db::LegacySubmission>;
    database.erase_query<oj::db::JudgeResultReceipt>(
        ReceiptQuery::message_id == formal_message || ReceiptQuery::message_id == custom_message);
    database.erase_query<oj::db::JudgeOutbox>(
        OutboxQuery::message_id == formal_message || OutboxQuery::message_id == custom_message);
    if (submission_id != 0) database.erase<oj::db::Submission>(submission_id);
    database.erase_query<oj::db::LegacySubmission>(
        LegacyQuery::user_id == user_id && LegacyQuery::question_id == question_id);
    database.erase_query<oj::db::Question>(odb::query<oj::db::Question>::id == question_id);
    transaction.commit();
}
}

int main()
{
    constexpr uint32_t user_id = 1;
    const std::string question_id = "99999";
    const std::string suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const std::string formal_message = "formal-" + suffix;
    const std::string custom_message = "custom-" + suffix;
    odb::mysql::database database("oj_server", "password", "myoj", "127.0.0.1", 3306);
    Cleanup(database, user_id, question_id, formal_message, custom_message, 0);
    {
        odb::transaction transaction(database.begin());
        oj::db::Question question{};
        question.id = question_id;
        question.title = "Phase 7 persistence test";
        question.desc = "test";
        question.star = "easy";
        question.create_time = oj::util::TimeUtil::IntToDateTime(std::time(nullptr) * 1000000LL);
        question.update_time = question.create_time;
        question.cpu_limit = 1;
        question.memory_limit = 64 * 1024 * 1024;
        question.visible = true;
        database.persist(question);
        transaction.commit();
    }
    setenv("OJ_DB_HOST", "127.0.0.1", 1);
    setenv("OJ_DB_PORT", "3306", 1);
    setenv("OJ_DB_USER", "oj_server", 1);
    setenv("OJ_DB_PASS", "password", 1);
    setenv("OJ_DB_NAME", "myoj", 1);
    Check(oj::model::ModelBase::StartDatabase("submission_persistence_test"),
          "model database starts");

    oj::model::ModelSubmission model;
    oj::mq::JudgeTaskMessage task;
    task.set_message_id(formal_message);
    task.set_user_id(user_id);
    task.set_question_id(question_id);
    task.set_code("int main(){}");
    task.set_language("cpp17");
    task.set_time_limit_ms(1000);
    task.set_memory_limit_mb(64);
    task.set_created_at(std::time(nullptr));
    task.add_test_cases()->set_index(1);
    oj::db::Submission submission{};
    Check(model.CreateSubmissionWithOutbox(user_id, question_id, task.code(), task.language(), task,
                                           &submission),
          "submission and outbox commit atomically");
    oj::db::JudgeOutbox outbox{};
    Check(model.GetOutbox(formal_message, &outbox), "formal outbox is readable");
    oj::mq::JudgeTaskMessage snapshot;
    Check(snapshot.ParseFromArray(outbox.payload.data(), static_cast<int>(outbox.payload.size())) &&
              snapshot.submission_id() == submission.submission_id,
          "outbox carries immutable submission snapshot");

    oj::common::JudgeResult result;
    result.set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    result.set_time_used_ms(12);
    result.set_memory_used_bytes(4096);
    result.set_completed_at(std::time(nullptr));
    oj::biz::UpdateJudgeResultRequest callback;
    callback.set_message_id(formal_message);
    callback.set_submission_id(submission.submission_id);
    result.SerializeToString(callback.mutable_result_payload());
    const auto expiry = oj::util::TimeUtil::IntToDateTime(
        (std::time(nullptr) + 600LL) * 1000000);
    Check(model.ApplyJudgeResult(callback, result, expiry) ==
              oj::model::ModelSubmission::ApplyOutcome::Persisted,
          "first formal result persists");
    Check(model.ApplyJudgeResult(callback, result, expiry) ==
              oj::model::ModelSubmission::ApplyOutcome::Idempotent,
          "duplicate formal result is idempotent");
    result.set_status(oj::common::SUBMISSION_STATUS_WRONG_ANSWER);
    result.SerializeToString(callback.mutable_result_payload());
    Check(model.ApplyJudgeResult(callback, result, expiry) ==
              oj::model::ModelSubmission::ApplyOutcome::Conflict,
          "conflicting terminal result is rejected");

    oj::mq::JudgeTaskMessage custom = task;
    custom.set_message_id(custom_message);
    custom.set_custom_task_id("task-" + suffix);
    custom.set_created_at(std::time(nullptr));
    Check(model.CreateCustomOutbox(custom), "custom task creates outbox without submission");
    result.set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    oj::biz::UpdateJudgeResultRequest custom_callback;
    custom_callback.set_message_id(custom_message);
    custom_callback.set_custom_task_id(custom.custom_task_id());
    result.SerializeToString(custom_callback.mutable_result_payload());
    Check(model.ApplyJudgeResult(custom_callback, result, expiry) ==
              oj::model::ModelSubmission::ApplyOutcome::Persisted,
          "custom result receipt persists");
    Check(model.ApplyJudgeResult(custom_callback, result, expiry) ==
              oj::model::ModelSubmission::ApplyOutcome::Idempotent,
          "duplicate custom result is idempotent");

    Cleanup(database, user_id, question_id, formal_message, custom_message,
            submission.submission_id);
    oj::model::ModelBase::StopDatabase();
    std::cout << "submission_persistence_test: PASS\n";
}
