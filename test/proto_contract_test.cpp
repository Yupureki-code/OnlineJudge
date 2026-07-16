#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/unknown_field_set.h>

#include "biz_service.pb.h"
#include "common.pb.h"
#include "judge_service.pb.h"
#include "mq_message.pb.h"

namespace {

void Check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

template <typename Message>
Message RoundTrip(const Message& source) {
    std::string wire;
    Check(source.SerializeToString(&wire), source.GetTypeName() + " serialization");
    Message parsed;
    Check(parsed.ParseFromString(wire), source.GetTypeName() + " parsing");
    return parsed;
}

void AppendVarint(std::string* wire, uint64_t value) {
    while (value >= 0x80) {
        wire->push_back(static_cast<char>((value & 0x7f) | 0x80));
        value >>= 7;
    }
    wire->push_back(static_cast<char>(value));
}

const google::protobuf::MethodDescriptor* Method(
    const google::protobuf::ServiceDescriptor* service, const char* name) {
    Check(service != nullptr, "service descriptor exists");
    const auto* method = service->FindMethodByName(name);
    Check(method != nullptr, std::string("method exists: ") + name);
    return method;
}

template <std::size_t N>
void CheckMethods(const google::protobuf::ServiceDescriptor* service,
                  const std::array<const char*, N>& expected) {
    Check(service != nullptr, "service descriptor exists");
    Check(service->method_count() == static_cast<int>(N),
          service->full_name() + " method count");

    std::set<std::string> expected_names;
    for (const char* name : expected) {
        Check(expected_names.insert(name).second,
              service->full_name() + " expected method is unique: " + name);
        Method(service, name);
    }

    std::set<std::string> actual_names;
    for (int i = 0; i < service->method_count(); ++i) {
        actual_names.insert(service->method(i)->name());
    }
    Check(actual_names == expected_names, service->full_name() + " exact method set");
}

void TestLargeSubmissionId() {
    constexpr uint64_t kLargeId = 9007199254740993ULL;

    oj::common::Submission submission;
    submission.set_submission_id(kLargeId);
    submission.set_question_id("1000");
    submission.set_status(oj::common::SUBMISSION_STATUS_QUEUED);
    const auto parsed_submission = RoundTrip(submission);
    Check(parsed_submission.submission_id() == kLargeId,
          "submission_id preserves values above JavaScript safe integer range");

    oj::biz::GetSubmissionRequest request;
    request.set_submission_id(kLargeId);
    Check(RoundTrip(request).submission_id() == kLargeId,
          "submission lookup uses uint64 without precision loss");
}

void TestFormalAndCustomTasks() {
    constexpr uint64_t kLargeId = 18446744073709550000ULL;

    oj::mq::JudgeTaskMessage formal;
    formal.set_message_id("msg_01JZZZZZZZZZZZZZZZZZZZZZZZ");
    formal.set_submission_id(kLargeId);
    formal.set_question_id("1000");
    formal.set_code("int main() { return 0; }");
    formal.set_language("cpp");
    formal.add_test_cases()->set_expected_output("42\n");
    const auto parsed_formal = RoundTrip(formal);
    Check(parsed_formal.task_id_case() == oj::mq::JudgeTaskMessage::kSubmissionId,
          "formal task selects submission_id");
    Check(parsed_formal.submission_id() == kLargeId, "formal task preserves uint64 ID");
    Check(parsed_formal.message_id() == formal.message_id(),
          "formal task preserves outbox message_id");

    oj::mq::JudgeTaskMessage custom;
    custom.set_message_id("msg_01JYYYYYYYYYYYYYYYYYYYYYYYYY");
    custom.set_custom_task_id("task_4f9d71f47a8d4dcc94c7c894090f81d4");
    custom.set_custom_input("1 2\n");
    const auto parsed_custom = RoundTrip(custom);
    Check(parsed_custom.task_id_case() == oj::mq::JudgeTaskMessage::kCustomTaskId,
          "custom task selects custom_task_id");
    Check(parsed_custom.custom_task_id() == custom.custom_task_id(),
          "custom task preserves high-entropy string ID");

    oj::biz::UpdateJudgeResultRequest callback;
    callback.set_message_id(custom.message_id());
    callback.set_custom_task_id(custom.custom_task_id());
    oj::common::JudgeResult result;
    result.set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
    Check(result.SerializeToString(callback.mutable_result_payload()),
          "callback result payload serialization");
    callback.mutable_service_auth()->set_key_id("judge-primary");
    callback.mutable_service_auth()->set_timestamp(1721000000);
    callback.mutable_service_auth()->set_nonce("nonce-01JYYYYYYYYYYYYYYYYYYYY");
    callback.mutable_service_auth()->set_hmac_sha256(std::string(32, 'x'));
    const auto parsed_callback = RoundTrip(callback);
    Check(parsed_callback.message_id() == custom.message_id(),
          "message_id spans MQ task and Judge callback");
    Check(parsed_callback.custom_task_id() == custom.custom_task_id(),
          "callback preserves custom task identity");
    Check(parsed_callback.service_auth().hmac_sha256().size() == 32,
          "callback preserves its service authentication signature");
    oj::common::JudgeResult parsed_result;
    Check(parsed_result.ParseFromString(parsed_callback.result_payload()),
          "callback preserves exact parseable result bytes");
}

void TestAllSubmissionStatuses() {
    const auto* descriptor = oj::common::SubmissionStatus_descriptor();
    Check(descriptor->value_count() == 12, "canonical submission status count");

    for (int i = 0; i < descriptor->value_count(); ++i) {
        const int number = descriptor->value(i)->number();
        Check(oj::common::SubmissionStatus_IsValid(number), "status enum value is valid");

        oj::common::TestCaseResult result;
        result.set_index(static_cast<uint32_t>(i));
        result.set_status(static_cast<oj::common::SubmissionStatus>(number));
        const auto parsed = RoundTrip(result);
        Check(parsed.status() == number,
              "status round trip: " + descriptor->value(i)->name());
    }
}

void TestUnknownFieldCompatibility() {
    oj::common::Submission source;
    source.set_submission_id(9007199254740993ULL);
    source.set_status(oj::common::SUBMISSION_STATUS_JUDGING);

    std::string wire;
    Check(source.SerializeToString(&wire), "unknown-field source serialization");
    AppendVarint(&wire, (99U << 3U) | 0U);
    AppendVarint(&wire, 123456789U);

    oj::common::Submission parsed;
    Check(parsed.ParseFromString(wire), "message with future field parses");
    const auto& unknown = parsed.GetReflection()->GetUnknownFields(parsed);
    Check(unknown.field_count() == 1, "future field retained in unknown field set");
    Check(unknown.field(0).number() == 99, "future field number retained");

    const auto reparsed = RoundTrip(parsed);
    const auto& reparsed_unknown =
        reparsed.GetReflection()->GetUnknownFields(reparsed);
    Check(reparsed_unknown.field_count() == 1, "future field survives relay serialization");
    Check(reparsed_unknown.field(0).varint() == 123456789U,
          "future field value survives relay serialization");
}

void TestServiceContracts() {
    const auto* pool = google::protobuf::DescriptorPool::generated_pool();
    Check(pool->FindMessageTypeByName("oj.mq.JudgeTaskMessage") != nullptr,
          "canonical MQ task message is generated");
    Check(pool->FindMessageTypeByName("oj_judge.SubmitRequest") != nullptr,
          "Phase 2 Judge compatibility request remains during migration");
    Check(pool->FindMessageTypeByName("oj.mq.JudgeResultCallback") == nullptr,
          "legacy MQ result callback is not a competing result contract");

    Check(oj::biz::AuthResponse::descriptor()->FindFieldByName("jwt_token") == nullptr,
          "user auth response has no JWT field");
    Check(oj::biz::AdminLoginResponse::descriptor()->FindFieldByName("jwt_token") == nullptr,
          "admin auth response has no JWT field");
    Check(oj::biz::CreateSolutionRequest::descriptor()->FindFieldByName("user_id") == nullptr,
          "solution mutations derive actor from session");
    Check(oj::biz::CreateCommentRequest::descriptor()->FindFieldByName("user_id") == nullptr,
          "comment mutations derive actor from session");
    Check(oj::biz::CreateSubmissionRequest::descriptor()->FindFieldByName("user_id") == nullptr,
          "submission creation derives actor from session");
    Check(oj::biz::ListSubmissionsRequest::descriptor()->FindFieldByName("user_id") == nullptr,
          "submission history derives actor from session");
    Check(oj::biz::CreateCommentRequest::descriptor()->FindFieldByName("reply_to_user_id") == nullptr,
          "reply target is derived from the stored parent comment");
    Check(oj::mq::JudgeTaskMessage::descriptor()->FindFieldByName("task_kind") == nullptr,
          "MQ task identity has one source of truth");
    Check(oj::biz::UpdateJudgeResultRequest::descriptor()->FindFieldByName("task_kind") == nullptr,
          "result callback identity has one source of truth");
    Check(oj::biz::UpdateJudgeResultRequest::descriptor()->FindFieldByName("service_auth") != nullptr,
          "Judge callback carries service authentication");
    Check(oj::common::CommentActionState::descriptor()->FindFieldByName("comment_id") != nullptr,
          "batched comment actions remain correlated by comment ID");

    const auto* biz_file = oj::biz::OJService::descriptor()->file();
    Check(biz_file->service_count() == 2, "business proto defines exactly two services");
    Check(biz_file->FindServiceByName("OJService") == oj::biz::OJService::descriptor(),
          "OJService is the main-site service");
    Check(biz_file->FindServiceByName("OJAdminService") ==
              oj::biz::OJAdminService::descriptor(),
          "OJAdminService is the admin service");

    constexpr std::array<const char*, 51> kOJMethods{{
        "SendRegistrationCode", "Register", "LoginWithVerificationCode",
        "LoginWithPassword", "Logout", "SetPassword", "SendSecurityCode",
        "ChangePassword", "ChangeEmail", "DeleteAccount", "GetCurrentUser",
        "GetUserProfile", "UpdateProfile", "UpdateAvatar", "DeleteAvatar",
        "GetStatistics", "ListQuestions", "GetQuestion", "GetPassStatus",
        "CreateQuestion", "UpdateQuestion", "DeleteQuestion", "ListTestCases",
        "CreateTestCase", "UpdateTestCase", "DeleteTestCase", "InvalidateCache",
        "ListSolutions", "GetSolution", "CreateSolution", "UpdateSolution",
        "DeleteSolution", "ToggleSolutionLike", "ToggleSolutionFavorite",
        "GetActionState", "ListComments", "ListReplies", "CreateComment",
        "UpdateComment", "DeleteComment", "ToggleCommentLike",
        "ToggleCommentFavorite", "GetActionStates", "CreateSubmission",
        "CreateCustomTest", "GetSubmission", "GetCustomTest", "ListSubmissions",
        "UpdateJudgeResult", "LegacyUpdateJudgeResult", "InvalidateStaticCache",
    }};
    CheckMethods(oj::biz::OJService::descriptor(), kOJMethods);

    constexpr std::array<const char*, 26> kOJAdminMethods{{
        "AdminGetVersion", "AdminLogin", "AdminLogout", "AdminGetOverview",
        "AdminListUsers", "AdminGetUser", "AdminCreateUser", "AdminUpdateUser",
        "AdminDeleteUser", "AdminListQuestions", "AdminGetQuestion",
        "AdminCreateQuestion", "AdminUpdateQuestion", "AdminDeleteQuestion",
        "AdminListTestCases", "AdminCreateTestCase", "AdminUpdateTestCase",
        "AdminDeleteTestCase", "AdminInvalidateQuestionCache",
        "AdminListAdminAccounts", "AdminGetAdminAccount", "AdminCreateAdminAccount",
        "AdminUpdateAdminAccount", "AdminDeleteAdminAccount", "AdminGetCacheMetrics",
        "AdminGetAuditLogs",
    }};
    CheckMethods(oj::biz::OJAdminService::descriptor(), kOJAdminMethods);
    for (int i = 0; i < oj::biz::OJAdminService::descriptor()->method_count(); ++i) {
        Check(oj::biz::OJAdminService::descriptor()->method(i)->name().rfind("Admin", 0) == 0,
              "every admin RPC has the Admin prefix");
    }

    const auto* list_admins =
        Method(oj::biz::OJAdminService::descriptor(), "AdminListAdminAccounts");
    Check(list_admins->input_type()->full_name() == "oj.biz.ListAdminAccountsRequest",
           "admin account listing carries paging input");

    const auto* update = Method(oj::biz::OJService::descriptor(), "UpdateJudgeResult");
    Check(update->input_type()->full_name() == "oj.biz.UpdateJudgeResultRequest",
          "Judge callback uses canonical request");
    Check(update->output_type()->full_name() == "oj.biz.UpdateJudgeResultResponse",
          "Judge callback has explicit persistence response");
    const auto* legacy_update =
        Method(oj::biz::OJService::descriptor(), "LegacyUpdateJudgeResult");
    Check(legacy_update->input_type()->full_name() == "oj_judge.JudgeFinishedRequest",
          "completed Phase 2 Judge retains an explicit transitional callback");

    oj::biz::UpdateJudgeResultResponse response;
    response.set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_PERSISTED);
    Check(RoundTrip(response).outcome() ==
              oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_PERSISTED,
          "persisted callback outcome serializes");
    response.set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_IDEMPOTENT);
    Check(RoundTrip(response).outcome() ==
              oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_IDEMPOTENT,
          "idempotent callback outcome serializes");
    response.set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE);
    response.set_retry_after_ms(250);
    Check(RoundTrip(response).retry_after_ms() == 250,
          "retryable callback outcome carries retry delay");

    const auto* docker = Method(oj_judge::JudgeService::descriptor(), "DockerWorkDone");
    Check(docker->input_type()->full_name() == "oj_judge.DockerWorkDoneRequest",
          "existing Docker callback request remains available");
    Check(docker->output_type()->full_name() == "oj_judge.NullRsp",
          "existing Docker callback response remains available");
}

}  // namespace

int main() {
    TestLargeSubmissionId();
    TestFormalAndCustomTasks();
    TestAllSubmissionStatuses();
    TestUnknownFieldCompatibility();
    TestServiceContracts();
    std::cout << "proto_contract_test: PASS\n";
    return 0;
}
