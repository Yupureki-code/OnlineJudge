#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <set>
#include <string>

#include <brpc/controller.h>

#include "rpc/oj_service_impl.hpp"

namespace
{

using namespace std::chrono_literals;

[[noreturn]] void Fail(const std::string& message)
{
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

void Check(bool condition, const std::string& message)
{
    if (!condition) Fail(message);
}

class WaitingClosure final : public google::protobuf::Closure
{
public:
    void Run() override
    {
        count_.fetch_add(1, std::memory_order_release);
        condition_.notify_all();
    }

    void Wait(const std::string& method)
    {
        std::unique_lock lock(mutex_);
        Check(condition_.wait_for(lock, 2s, [&] {
            return count_.load(std::memory_order_acquire) >= 1;
        }), method + " did not complete");
        Check(count_.load(std::memory_order_acquire) == 1,
              method + " completed more than once");
    }

private:
    std::atomic_int count_{0};
    std::mutex mutex_;
    std::condition_variable condition_;
};

class TestController : public brpc::Controller
{
public:
    bool IsCanceled() const override { return false; }
};

class CancelledController final : public TestController
{
public:
    bool IsCanceled() const override { return true; }
};

template <typename Response>
Response ExpectedStatus(int code, const std::string& message = {}, bool retryable = false)
{
    Response response;
    auto* status = response.mutable_status();
    status->set_success(code == 200);
    status->set_code(code);
    status->set_message(message);
    status->set_retryable(retryable);
    return response;
}

class RpcSuite
{
public:
    RpcSuite()
        : executor_({.worker_count = 1, .queue_capacity = 16}), service_(nullptr, executor_)
    {
        Check(executor_.Start(), "business executor starts");
    }

    ~RpcSuite()
    {
        executor_.Stop();
    }

    template <typename Request, typename Response, typename Setup>
    void Expect(const std::string& name,
                void (oj::rpc::OJServiceImpl::*method)(google::protobuf::RpcController*,
                                                       const Request*, Response*,
                                                       google::protobuf::Closure*),
                google::protobuf::RpcController* controller,
                Setup setup,
                const Response& expected)
    {
        Check(tested_.insert(name).second, "duplicate test for " + name);
        Request request;
        setup(request);
        Response actual;
        WaitingClosure done;
        (service_.*method)(controller, &request, &actual, &done);
        done.Wait(name);
        if (actual.SerializeAsString() != expected.SerializeAsString()) {
            std::cerr << "FAIL: " << name << " response mismatch\n"
                      << "Expected:\n" << expected.DebugString()
                      << "Actual:\n" << actual.DebugString();
            std::exit(1);
        }
    }

    template <typename Request, typename Response>
    void ExpectStatus(const std::string& name,
                      void (oj::rpc::OJServiceImpl::*method)(google::protobuf::RpcController*,
                                                             const Request*, Response*,
                                                             google::protobuf::Closure*),
                      google::protobuf::RpcController* controller,
                      int code,
                      const std::string& message = {},
                      bool retryable = false)
    {
        Expect<Request, Response>(name, method, controller, [](Request&) {},
                                  ExpectedStatus<Response>(code, message, retryable));
    }

    void ExpectLegacyFailure(TestController* controller)
    {
        const std::string name = "LegacyUpdateJudgeResult";
        Check(tested_.insert(name).second, "duplicate test for " + name);
        oj_judge::JudgeFinishedRequest request;
        oj_judge::NullRsp response;
        WaitingClosure done;
        service_.LegacyUpdateJudgeResult(controller, &request, &response, &done);
        done.Wait(name);
        Check(controller->Failed() && controller->ErrorText() == "NOT_IMPLEMENTED",
              name + " controller failure mismatch");
        Check(response.SerializeAsString().empty(), name + " response must remain empty");
    }

    void VerifyDescriptorCoverage() const
    {
        const auto* descriptor = oj::biz::OJService::descriptor();
        Check(descriptor != nullptr, "OJService descriptor exists");
        Check(static_cast<int>(tested_.size()) == descriptor->method_count(),
              "tested RPC count does not match OJService descriptor");
        for (int index = 0; index < descriptor->method_count(); ++index) {
            const std::string& name = descriptor->method(index)->name();
            Check(tested_.contains(name), "missing RPC test for " + name);
        }
    }

private:
    oj::runtime::BusinessExecutor executor_;
    oj::rpc::OJServiceImpl service_;
    std::set<std::string> tested_;
};

void CheckAuthAndUserRpcs(RpcSuite& suite, TestController* controller)
{
    auto invalid_email = ExpectedStatus<oj::biz::SendVerificationCodeResponse>(400, "INVALID_EMAIL");
    suite.Expect<oj::biz::SendVerificationCodeRequest>(
        "SendRegistrationCode", &oj::rpc::OJServiceImpl::SendRegistrationCode, controller,
        [](auto& request) { request.set_email("invalid"); }, invalid_email);

    suite.Expect<oj::biz::RegisterRequest>(
        "Register", &oj::rpc::OJServiceImpl::Register, controller,
        [](auto& request) { request.set_email("invalid"); },
        ExpectedStatus<oj::biz::AuthResponse>(400, "INVALID_EMAIL"));

    suite.Expect<oj::biz::VerificationCodeLoginRequest>(
        "LoginWithVerificationCode", &oj::rpc::OJServiceImpl::LoginWithVerificationCode, controller,
        [](auto& request) { request.set_email("invalid"); },
        ExpectedStatus<oj::biz::AuthResponse>(400, "INVALID_EMAIL"));

    suite.ExpectStatus<oj::biz::PasswordLoginRequest, oj::biz::AuthResponse>(
        "LoginWithPassword", &oj::rpc::OJServiceImpl::LoginWithPassword, controller,
        400, "LOGIN_ID_REQUIRED");
    suite.ExpectStatus<oj::common::EmptyRequest, oj::common::EmptyResponse>(
        "Logout", &oj::rpc::OJServiceImpl::Logout, nullptr, 200);
    suite.ExpectStatus<oj::biz::SetPasswordRequest, oj::common::EmptyResponse>(
        "SetPassword", &oj::rpc::OJServiceImpl::SetPassword, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::SendSecurityCodeRequest, oj::biz::SendVerificationCodeResponse>(
        "SendSecurityCode", &oj::rpc::OJServiceImpl::SendSecurityCode, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::ChangePasswordRequest, oj::common::EmptyResponse>(
        "ChangePassword", &oj::rpc::OJServiceImpl::ChangePassword, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::ChangeEmailRequest, oj::common::EmptyResponse>(
        "ChangeEmail", &oj::rpc::OJServiceImpl::ChangeEmail, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::DeleteAccountRequest, oj::common::EmptyResponse>(
        "DeleteAccount", &oj::rpc::OJServiceImpl::DeleteAccount, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::common::EmptyRequest, oj::biz::GetUserProfileResponse>(
        "GetCurrentUser", &oj::rpc::OJServiceImpl::GetCurrentUser, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::GetUserProfileRequest, oj::biz::GetUserProfileResponse>(
        "GetUserProfile", &oj::rpc::OJServiceImpl::GetUserProfile, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::UpdateProfileRequest, oj::biz::UpdateProfileResponse>(
        "UpdateProfile", &oj::rpc::OJServiceImpl::UpdateProfile, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::UpdateAvatarRequest, oj::biz::UpdateAvatarResponse>(
        "UpdateAvatar", &oj::rpc::OJServiceImpl::UpdateAvatar, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::common::EmptyRequest, oj::common::EmptyResponse>(
        "DeleteAvatar", &oj::rpc::OJServiceImpl::DeleteAvatar, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::common::EmptyRequest, oj::biz::GetUserStatisticsResponse>(
        "GetStatistics", &oj::rpc::OJServiceImpl::GetStatistics, controller, 401, "UNAUTHORIZED");
}

void CheckQuestionRpcs(RpcSuite& suite, TestController* controller)
{
    CancelledController cancelled;
    suite.ExpectStatus<oj::biz::GetQuestionListRequest, oj::biz::GetQuestionListResponse>(
        "ListQuestions", &oj::rpc::OJServiceImpl::ListQuestions, &cancelled, 499, "REQUEST_CANCELLED");
    suite.ExpectStatus<oj::biz::QuestionIdRequest, oj::biz::GetQuestionDetailResponse>(
        "GetQuestion", &oj::rpc::OJServiceImpl::GetQuestion, controller, 400, "INVALID_QUESTION_ID");

    auto pass_status = ExpectedStatus<oj::biz::GetQuestionPassStatusResponse>(200);
    pass_status.set_passed(false);
    suite.Expect<oj::biz::QuestionIdRequest>(
        "GetPassStatus", &oj::rpc::OJServiceImpl::GetPassStatus, nullptr,
        [](auto& request) { request.set_question_id("1"); }, pass_status);

    suite.ExpectStatus<oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse>(
        "CreateQuestion", &oj::rpc::OJServiceImpl::CreateQuestion, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse>(
        "UpdateQuestion", &oj::rpc::OJServiceImpl::UpdateQuestion, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::QuestionIdRequest, oj::common::EmptyResponse>(
        "DeleteQuestion", &oj::rpc::OJServiceImpl::DeleteQuestion, controller, 501, "NOT_IMPLEMENTED");

    suite.Expect<oj::biz::ListTestCasesRequest>(
        "ListTestCases", &oj::rpc::OJServiceImpl::ListTestCases, controller,
        [](auto& request) { request.set_question_id("1"); },
        ExpectedStatus<oj::biz::ListTestCasesResponse>(501, "NOT_IMPLEMENTED"));
    suite.ExpectStatus<oj::biz::CreateTestCaseRequest, oj::biz::TestCaseResponse>(
        "CreateTestCase", &oj::rpc::OJServiceImpl::CreateTestCase, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::UpdateTestCaseRequest, oj::biz::TestCaseResponse>(
        "UpdateTestCase", &oj::rpc::OJServiceImpl::UpdateTestCase, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::TestCaseIdRequest, oj::common::EmptyResponse>(
        "DeleteTestCase", &oj::rpc::OJServiceImpl::DeleteTestCase, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::InvalidateQuestionCacheRequest, oj::common::EmptyResponse>(
        "InvalidateCache", &oj::rpc::OJServiceImpl::InvalidateCache, controller, 501, "NOT_IMPLEMENTED");
}

void CheckSolutionRpcs(RpcSuite& suite, TestController* controller)
{
    suite.ExpectStatus<oj::biz::ListSolutionsRequest, oj::biz::ListSolutionsResponse>(
        "ListSolutions", &oj::rpc::OJServiceImpl::ListSolutions, controller, 400, "INVALID_QUESTION_ID");
    suite.ExpectStatus<oj::biz::SolutionIdRequest, oj::biz::GetSolutionResponse>(
        "GetSolution", &oj::rpc::OJServiceImpl::GetSolution, controller, 400, "INVALID_ID");
    suite.ExpectStatus<oj::biz::CreateSolutionRequest, oj::biz::SolutionResponse>(
        "CreateSolution", &oj::rpc::OJServiceImpl::CreateSolution, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::UpdateSolutionRequest, oj::biz::SolutionResponse>(
        "UpdateSolution", &oj::rpc::OJServiceImpl::UpdateSolution, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::SolutionIdRequest, oj::common::EmptyResponse>(
        "DeleteSolution", &oj::rpc::OJServiceImpl::DeleteSolution, controller, 501, "NOT_IMPLEMENTED");
    suite.ExpectStatus<oj::biz::SolutionIdRequest, oj::biz::SolutionActionResponse>(
        "ToggleSolutionLike", &oj::rpc::OJServiceImpl::ToggleSolutionLike, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::SolutionIdRequest, oj::biz::SolutionActionResponse>(
        "ToggleSolutionFavorite", &oj::rpc::OJServiceImpl::ToggleSolutionFavorite, controller,
        401, "UNAUTHORIZED");

    suite.Expect<oj::biz::SolutionIdRequest>(
        "GetActionState", &oj::rpc::OJServiceImpl::GetActionState, nullptr,
        [](auto& request) { request.set_solution_id(uint64_t{1} << 32); },
        ExpectedStatus<oj::biz::SolutionActionResponse>(404, "SOLUTION_NOT_FOUND"));
}

void CheckCommentRpcs(RpcSuite& suite, TestController* controller)
{
    suite.ExpectStatus<oj::biz::ListCommentsRequest, oj::biz::ListCommentsResponse>(
        "ListComments", &oj::rpc::OJServiceImpl::ListComments, controller, 400, "INVALID_ID");
    suite.ExpectStatus<oj::biz::ListRepliesRequest, oj::biz::ListCommentsResponse>(
        "ListReplies", &oj::rpc::OJServiceImpl::ListReplies, controller, 400, "INVALID_ID");
    suite.ExpectStatus<oj::biz::CreateCommentRequest, oj::biz::CommentResponse>(
        "CreateComment", &oj::rpc::OJServiceImpl::CreateComment, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::UpdateCommentRequest, oj::biz::CommentResponse>(
        "UpdateComment", &oj::rpc::OJServiceImpl::UpdateComment, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::CommentIdRequest, oj::common::EmptyResponse>(
        "DeleteComment", &oj::rpc::OJServiceImpl::DeleteComment, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::CommentIdRequest, oj::biz::CommentActionResponse>(
        "ToggleCommentLike", &oj::rpc::OJServiceImpl::ToggleCommentLike, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::CommentIdRequest, oj::biz::CommentActionResponse>(
        "ToggleCommentFavorite", &oj::rpc::OJServiceImpl::ToggleCommentFavorite, controller,
        401, "UNAUTHORIZED");

    suite.Expect<oj::biz::GetCommentActionsRequest>(
        "GetActionStates", &oj::rpc::OJServiceImpl::GetActionStates, nullptr,
        [](auto& request) {
            request.add_comment_ids(uint64_t{1} << 32);
        }, ExpectedStatus<oj::biz::GetCommentActionsResponse>(404, "COMMENT_NOT_FOUND"));
}

void CheckSubmissionAndCacheRpcs(RpcSuite& suite, TestController* controller)
{
    suite.ExpectStatus<oj::biz::CreateSubmissionRequest, oj::biz::CreateSubmissionResponse>(
        "CreateSubmission", &oj::rpc::OJServiceImpl::CreateSubmission, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::CreateCustomTestRequest, oj::biz::CreateCustomTestResponse>(
        "CreateCustomTest", &oj::rpc::OJServiceImpl::CreateCustomTest, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::GetSubmissionRequest, oj::biz::GetSubmissionResponse>(
        "GetSubmission", &oj::rpc::OJServiceImpl::GetSubmission, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::GetCustomTestRequest, oj::biz::GetCustomTestResponse>(
        "GetCustomTest", &oj::rpc::OJServiceImpl::GetCustomTest, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::ListSubmissionsRequest, oj::biz::ListSubmissionsResponse>(
        "ListSubmissions", &oj::rpc::OJServiceImpl::ListSubmissions, controller, 401, "UNAUTHORIZED");
    suite.ExpectStatus<oj::biz::UpdateJudgeResultRequest, oj::biz::UpdateJudgeResultResponse>(
        "UpdateJudgeResult", &oj::rpc::OJServiceImpl::UpdateJudgeResult, controller, 400, "INVALID_CALLBACK");
    suite.ExpectLegacyFailure(controller);
    suite.ExpectStatus<oj::biz::InvalidateStaticCacheRequest, oj::common::EmptyResponse>(
        "InvalidateStaticCache", &oj::rpc::OJServiceImpl::InvalidateStaticCache, controller,
        401, "UNAUTHORIZED");
}

} // namespace

int main()
{
    RpcSuite suite;
    TestController controller;
    CheckAuthAndUserRpcs(suite, &controller);
    CheckQuestionRpcs(suite, &controller);
    CheckSolutionRpcs(suite, &controller);
    CheckCommentRpcs(suite, &controller);
    CheckSubmissionAndCacheRpcs(suite, &controller);
    suite.VerifyDescriptorCoverage();
    std::cout << "oj_service_rpc_test: PASS (51/51 OJService RPCs)\n";
    return 0;
}
