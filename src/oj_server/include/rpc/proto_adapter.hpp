#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

#include <jsoncpp/json/json.h>
#include <google/protobuf/util/json_util.h>

#include "../../../comm/proto/common.pb.h"
#include "../../../comm/comm.hpp"
#include "../../../comm/models/user.hxx"
#include "../../../comm/models/question.hxx"
#include "../../../comm/models/solution.hxx"
#include "../../../comm/models/comment.hxx"
#include "../../../comm/models/submission.hxx"

namespace oj::rpc
{
    using namespace oj::db;
    class ProtoAdapter
    {
    public:
        static void SetStatus(oj::common::StatusResponse* output,
                            bool success,
                            int code,
                            std::string message = {},
                            bool retryable = false)
        {
            if (output == nullptr)
                return;
            output->set_success(success);
            output->set_code(code);
            output->set_message(std::move(message));
            output->set_retryable(retryable);
        }

        static void SetOk(oj::common::StatusResponse* output)
        {
            SetStatus(output, true, 200);
        }

        static void SetError(oj::common::StatusResponse* output,
                            int code,
                            std::string message,
                            bool retryable = false)
        {
            SetStatus(output, false, code, std::move(message), retryable);
        }

        static void SetPage(const oj::common::PageRequest& request,
                            uint64_t total_items,
                            oj::common::PageResponse* output,
                            uint32_t default_page_size = 20,
                            uint32_t max_page_size = 100)
        {
            if (output == nullptr)
                return;
            const uint32_t page = request.page() == 0 ? 1 : request.page();
            const uint32_t requested_size = request.page_size() == 0 ? default_page_size : request.page_size();
            const uint32_t page_size = std::clamp(requested_size, 1u, std::max(1u, max_page_size));
            output->set_total_items(total_items);
            output->set_page(page);
            output->set_page_size(page_size);
            output->set_total_pages(total_items == 0 ? 0 : static_cast<uint32_t>(
                std::min<uint64_t>((total_items + page_size - 1) / page_size,
                                std::numeric_limits<uint32_t>::max())));
        }

        static oj::common::SubmissionStatus SubmissionStatus(std::string_view status)
        {
            std::string normalized(status);
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
                return static_cast<char>(std::toupper(ch));
            });
            if (normalized == "PENDING") return oj::common::SUBMISSION_STATUS_PENDING;
            if (normalized == "QUEUED") return oj::common::SUBMISSION_STATUS_QUEUED;
            if (normalized == "JUDGING") return oj::common::SUBMISSION_STATUS_JUDGING;
            if (normalized == "AC" || normalized == "ACCEPTED") return oj::common::SUBMISSION_STATUS_ACCEPTED;
            if (normalized == "WA" || normalized == "WRONG_ANSWER") return oj::common::SUBMISSION_STATUS_WRONG_ANSWER;
            if (normalized == "CE" || normalized == "COMPILE_ERROR") return oj::common::SUBMISSION_STATUS_COMPILE_ERROR;
            if (normalized == "MLE" || normalized == "MEMORY_LIMIT_EXCEEDED") return oj::common::SUBMISSION_STATUS_MEMORY_LIMIT_EXCEEDED;
            if (normalized == "TLE" || normalized == "TIME_LIMIT_EXCEEDED") return oj::common::SUBMISSION_STATUS_TIME_LIMIT_EXCEEDED;
            if (normalized == "RTE" || normalized == "RUNTIME_ERROR") return oj::common::SUBMISSION_STATUS_RUNTIME_ERROR;
            if (normalized == "SYSTEM_ERROR") return oj::common::SUBMISSION_STATUS_SYSTEM_ERROR;
            if (normalized == "CANCELLED") return oj::common::SUBMISSION_STATUS_CANCELLED;
            return oj::common::SUBMISSION_STATUS_UNSPECIFIED;
        }

        static void ToProto(const oj::db::User& input, oj::common::User* output)
        {
            if (output == nullptr) return;
            output->set_uid(input.uid);
            output->set_name(input.name);
            output->set_email(input.email);
            output->set_create_time(oj::util::TimeUtil::DateTimeToInt(input.create_time));
            output->set_last_login(oj::util::TimeUtil::DateTimeToInt(input.last_login));
            output->set_role(0);
            output->set_disabled(false);
        }

        static void ToProto(const oj::db::Question& input, oj::common::Question* output)
        {
            if (output == nullptr) return;
            output->set_id(input.id);
            output->set_title(input.title);
            output->set_description(input.desc);
            output->set_difficulty(input.star);
            output->set_time_limit_ms(LegacyTimeLimitMs(input.cpu_limit));
            output->set_memory_limit_mb(LegacyMemoryLimitMb(input.memory_limit));
            output->set_create_time(oj::util::TimeUtil::DateTimeToInt(input.create_time));
            output->set_update_time(oj::util::TimeUtil::DateTimeToInt(input.update_time));
            output->set_visible(input.visible);
        }

        static void ToProto(const oj::db::Submission& input, oj::common::Submission* output)
        {
            if (output == nullptr) return;
            output->set_submission_id(input.submission_id);
            output->set_user_id(input.user_id);
            output->set_question_id(input.question_id);
            if (input.code) output->set_code(input.code.get());
            if (input.language) output->set_language(input.language.get());
            output->set_status(SubmissionStatus(input.status));
            if (input.result_json) {
                google::protobuf::util::JsonStringToMessage(input.result_json.get(), output->mutable_result());
            }
            output->set_created_at(oj::util::TimeUtil::DateTimeToInt(input.created_at) / 1000000);
            if (input.completed_at)
                output->set_completed_at(oj::util::TimeUtil::DateTimeToInt(input.completed_at.get()) / 1000000);
            output->set_result_version(input.result_version);
        }

        static void ToProto(const oj::db::Solution& input, oj::common::Solution* output)
        {
            if (output == nullptr) return;
            output->set_id(input.id);
            output->set_question_id(input.question_id);
            output->set_user_id(input.user_id);
            output->set_title(input.title);
            output->set_content_md(input.content_md);
            output->set_like_count(input.like_count);
            output->set_favorite_count(input.favorite_count);
            output->set_comment_count(input.comment_count);
            output->set_moderation_status(input.status);
            output->set_created_at(oj::util::TimeUtil::DateTimeToInt(input.created_at));
            output->set_updated_at(oj::util::TimeUtil::DateTimeToInt(input.updated_at));
        }

        static void ToProto(const oj::db::Comment& input, oj::common::Comment* output)
        {
            if (output == nullptr) return;
            output->Clear();
            output->set_id(input.id);
            output->set_solution_id(input.solution_id);
            output->set_user_id(input.user_id);
            if (!input.parent_id.null()) output->set_parent_id(input.parent_id.get());
            if (!input.reply_to_user_id.null()) output->set_reply_to_user_id(input.reply_to_user_id.get());
            output->set_content(input.content);
            output->set_like_count(input.like_count);
            output->set_favorite_count(input.favorite_count);
            if (!input.is_edited.null()) output->set_is_edited(input.is_edited.get());
            if (!input.created_at.null())
                output->set_created_at(oj::util::TimeUtil::DateTimeToInt(input.created_at.get()));
            if (!input.updated_at.null())
                output->set_updated_at(oj::util::TimeUtil::DateTimeToInt(input.updated_at.get()));
        }
        
        static void ToProto(const Json::Value& input, oj::common::User* output)
        {
            if (output == nullptr) return;
            output->set_uid(UInt(input, "uid"));
            output->set_name(String(input, "name"));
            output->set_email(String(input, "email"));
            output->set_create_time(Time(input, "create_time"));
            output->set_last_login(Time(input, "last_login"));
            output->set_role(Int(input, "role"));
            output->set_disabled(Bool(input, "disabled"));
        }

        static void ToProto(const Json::Value& input, oj::common::Question* output)
        {
            if (output == nullptr) return;
            output->set_id(String(input, input.isMember("id") ? "id" : "number"));
            output->set_title(String(input, "title"));
            output->set_description(String(input, input.isMember("description") ? "description" : "desc"));
            output->set_difficulty(String(input, input.isMember("difficulty") ? "difficulty" : "star"));
            output->set_time_limit_ms(input.isMember("time_limit_ms") ? UInt(input, "time_limit_ms") :
                                                                        LegacyTimeLimitMs(Int(input, "cpu_limit")));
            output->set_memory_limit_mb(input.isMember("memory_limit_mb") ? UInt(input, "memory_limit_mb") :
                                                                            LegacyMemoryLimitMb(Int(input, "memory_limit")));
            output->set_create_time(Time(input, "create_time"));
            output->set_update_time(Time(input, "update_time"));
            output->set_visible(!input.isMember("visible") || Bool(input, "visible"));
        }

        static void ToProto(const Json::Value& input, oj::common::TestCase* output)
        {
            if (output == nullptr) return;
            output->set_test_case_id(UInt64(input, input.isMember("test_case_id") ? "test_case_id" : "id"));
            output->set_question_id(String(input, "question_id"));
            output->set_ordinal(UInt(input, input.isMember("ordinal") ? "ordinal" : "id"));
            output->set_input(String(input, input.isMember("input") ? "input" : "in"));
            output->set_expected_output(String(input, input.isMember("expected_output") ? "expected_output" :
                                                    (input.isMember("output") ? "output" : "out")));
            output->set_is_sample(Bool(input, "is_sample"));
        }

    private:
        static uint32_t Unsigned(int value)
        {
            return value < 0 ? 0 : static_cast<uint32_t>(value);
        }

        static uint32_t LegacyTimeLimitMs(int seconds)
        {
            if (seconds <= 0) return 0;
            return static_cast<uint32_t>(std::min<uint64_t>(
                static_cast<uint64_t>(seconds) * 1000, std::numeric_limits<uint32_t>::max()));
        }

        static uint32_t LegacyMemoryLimitMb(int megabytes)
        {
            return megabytes <= 0 ? 0 : static_cast<uint32_t>(megabytes);
        }

        static std::string SolutionStatusString(::SolutionStatus status)
        {
            switch (status) {
            case ::SolutionStatus::pending: return "pending";
            case ::SolutionStatus::approved: return "approved";
            case ::SolutionStatus::rejected: return "rejected";
            }
            return "pending";
        }

        static std::string String(const Json::Value& value, const char* key)
        {
            return value.isMember(key) ? value[key].asString() : std::string{};
        }

        static uint64_t UInt64(const Json::Value& value, const char* key)
        {
            return value.isMember(key) ? value[key].asUInt64() : 0;
        }

        static uint32_t UInt(const Json::Value& value, const char* key)
        {
            return value.isMember(key) ? value[key].asUInt() : 0;
        }

        static int Int(const Json::Value& value, const char* key)
        {
            return value.isMember(key) ? value[key].asInt() : 0;
        }

        static bool Bool(const Json::Value& value, const char* key)
        {
            return value.isMember(key) && value[key].asBool();
        }

        static int64_t Time(const Json::Value& value, const char* key)
        {
            if (!value.isMember(key)) return 0;
            if (value[key].isInt64() || value[key].isUInt64()) return value[key].asInt64();
            return 0;
        }
    };

} // namespace oj::rpc
