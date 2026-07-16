#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <brpc/controller.h>

#include "rpc/oj_service_impl.hpp"

namespace
{

using namespace std::chrono_literals;

void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

class WaitingClosure final : public google::protobuf::Closure
{
public:
    void Run() override
    {
        count_.fetch_add(1, std::memory_order_release);
        condition_.notify_all();
    }

    void Wait()
    {
        std::unique_lock lock(mutex_);
        Check(condition_.wait_for(lock, 2s, [&] {
            return count_.load(std::memory_order_acquire) == 1;
        }), "RPC completed");
    }

    int Count() const { return count_.load(std::memory_order_acquire); }

private:
    std::atomic_int count_{0};
    std::mutex mutex_;
    std::condition_variable condition_;
};

class TestController final : public brpc::Controller
{
public:
    bool IsCanceled() const override { return false; }
};

class Gate
{
public:
    void Block()
    {
        std::unique_lock lock(mutex_);
        entered_ = true;
        condition_.notify_all();
        condition_.wait(lock, [&] { return open_; });
    }

    void WaitUntilEntered()
    {
        std::unique_lock lock(mutex_);
        Check(condition_.wait_for(lock, 2s, [&] { return entered_; }), "worker enters gate");
    }

    void Open()
    {
        std::lock_guard lock(mutex_);
        open_ = true;
        condition_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool entered_ = false;
    bool open_ = false;
};

void CheckValidationAndUnauthenticated()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 8});
    Check(executor.Start(), "executor starts");
    oj::rpc::OJServiceImpl service(nullptr, executor);
    TestController controller;

    oj::biz::SendVerificationCodeRequest send_request;
    send_request.set_email("not-an-email");
    oj::biz::SendVerificationCodeResponse send_response;
    WaitingClosure send_done;
    service.SendRegistrationCode(&controller, &send_request, &send_response, &send_done);
    send_done.Wait();
    if (send_response.status().code() != 400)
        std::cerr << "SendRegistrationCode status: " << send_response.status().code()
                  << " " << send_response.status().message() << '\n';
    Check(send_response.status().code() == 400 &&
              send_response.status().message() == "INVALID_EMAIL",
          "registration-code validation is stable");

    oj::biz::RegisterRequest register_request;
    register_request.set_email("user@example.test");
    register_request.set_verification_code("12x456");
    register_request.set_name("User");
    register_request.set_password("password1");
    oj::biz::AuthResponse register_response;
    WaitingClosure register_done;
    service.Register(&controller, &register_request, &register_response, &register_done);
    register_done.Wait();
    Check(register_response.status().code() == 400 &&
              register_response.status().message() == "INVALID_VERIFICATION_CODE",
          "registration code format is validated before business access");

    oj::common::EmptyRequest security_request;
    oj::biz::SendVerificationCodeResponse security_response;
    WaitingClosure security_done;
    service.SendSecurityCode(&controller, &security_request, &security_response, &security_done);
    security_done.Wait();
    Check(security_response.status().code() == 401,
          "security code is session-bound and distinct from registration code");

    oj::biz::CreateSolutionRequest solution_request;
    solution_request.set_question_id("1");
    solution_request.set_title("No request actor field");
    solution_request.set_content_md("content");
    oj::biz::SolutionResponse solution_response;
    WaitingClosure solution_done;
    service.CreateSolution(&controller, &solution_request, &solution_response, &solution_done);
    solution_done.Wait();
    Check(solution_response.status().code() == 401 &&
              solution_response.status().message() == "UNAUTHORIZED",
          "solution actor is derived from the session cookie");

    oj::biz::CreateCommentRequest comment_request;
    comment_request.set_solution_id(4);
    comment_request.set_content("content");
    oj::biz::CommentResponse comment_response;
    WaitingClosure comment_done;
    service.CreateComment(&controller, &comment_request, &comment_response, &comment_done);
    comment_done.Wait();
    Check(comment_response.status().code() == 401, "comment actor is not accepted from the request");

    oj::biz::GetCommentActionsRequest actions_request;
    actions_request.add_comment_ids(9);
    actions_request.add_comment_ids(11);
    oj::biz::GetCommentActionsResponse actions_response;
    WaitingClosure actions_done;
    service.GetActionStates(&controller, &actions_request, &actions_response, &actions_done);
    actions_done.Wait();
    Check(actions_response.status().success() && actions_response.actions_size() == 2 &&
              actions_response.actions(0).comment_id() == 9 &&
              actions_response.actions(1).comment_id() == 11 &&
              !actions_response.actions(0).liked() && !actions_response.actions(0).favorited(),
          "protobuf comment IDs convert to guest action states");
    Check(send_done.Count() == 1 && register_done.Count() == 1 &&
              security_done.Count() == 1 && solution_done.Count() == 1 &&
              comment_done.Count() == 1 && actions_done.Count() == 1,
          "supported overrides complete exactly once");
    executor.Stop();
}

void CheckNotImplementedAndLegacyFallback()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 4});
    Check(executor.Start(), "501 executor starts");
    oj::rpc::OJServiceImpl service(nullptr, executor);
    TestController controller;

    oj::biz::UpdateSolutionRequest request;
    request.set_solution_id(7);
    oj::biz::SolutionResponse response;
    WaitingClosure done;
    service.UpdateSolution(&controller, &request, &response, &done);
    done.Wait();
    Check(response.status().code() == 501 && response.status().message() == "NOT_IMPLEMENTED",
          "unsupported method returns stable 501");

    oj_judge::JudgeFinishedRequest legacy_request;
    oj_judge::NullRsp legacy_response;
    WaitingClosure legacy_done;
    service.LegacyUpdateJudgeResult(&controller, &legacy_request, &legacy_response, &legacy_done);
    legacy_done.Wait();
    Check(controller.Failed() && controller.ErrorText() == "NOT_IMPLEMENTED",
          "legacy callback uses controller failure");
    Check(done.Count() == 1 && legacy_done.Count() == 1, "unsupported overrides complete exactly once");
    executor.Stop();
}

void CheckQueueRejection()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 1});
    Check(executor.Start(), "queue executor starts");
    Gate gate;
    Check(executor.Submit([&] { gate.Block(); }) == oj::runtime::SubmitResult::Accepted,
          "blocking work accepted");
    gate.WaitUntilEntered();
    Check(executor.Submit([] {}) == oj::runtime::SubmitResult::Accepted, "queue is filled");
    oj::rpc::OJServiceImpl service(nullptr, executor);
    TestController controller;
    oj::biz::UpdateSolutionRequest request;
    oj::biz::SolutionResponse response;
    WaitingClosure done;
    service.UpdateSolution(&controller, &request, &response, &done);
    done.Wait();
    Check(response.status().code() == 503 &&
              response.status().message() == "BUSINESS_QUEUE_FULL" &&
              response.status().retryable(),
          "queue rejection is surfaced by AsyncDispatch");
    Check(done.Count() == 1, "rejected override completes exactly once");
    gate.Open();
    executor.Stop();
}

} // namespace

int main()
{
    CheckValidationAndUnauthenticated();
    CheckNotImplementedAndLegacyFallback();
    CheckQueueRejection();
    std::cout << "oj_service_rpc_test: PASS\n";
    return 0;
}
