#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#include <brpc/details/controller_private_accessor.h>

#include "biz_service.pb.h"
#include "common.pb.h"
#include "rpc/async_dispatch.hpp"
#include "rpc/proto_adapter.hpp"
#include "rpc/session_adapter.hpp"

namespace
{

using namespace std::chrono_literals;

void Check(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

class CountingClosure final : public google::protobuf::Closure
{
public:
    void Run() override
    {
        count_.fetch_add(1, std::memory_order_release);
        condition_.notify_all();
    }

    bool WaitFor(int expected, std::chrono::milliseconds timeout = 2s)
    {
        std::unique_lock lock(mutex_);
        return condition_.wait_for(lock, timeout, [&] { return count_.load(std::memory_order_acquire) >= expected; });
    }

    int Count() const { return count_.load(std::memory_order_acquire); }

private:
    std::atomic_int count_{0};
    std::mutex mutex_;
    std::condition_variable condition_;
};

class FakeRpcController final : public google::protobuf::RpcController
{
public:
    void Reset() override
    {
        failed_ = false;
        cancelled_ = false;
        error_.clear();
    }
    bool Failed() const override { return failed_; }
    std::string ErrorText() const override { return error_; }
    void StartCancel() override { cancelled_ = true; }
    void SetFailed(const std::string& reason) override
    {
        failed_ = true;
        error_ = reason;
    }
    bool IsCanceled() const override { return cancelled_; }
    void NotifyOnCancel(google::protobuf::Closure*) override {}

private:
    bool failed_ = false;
    bool cancelled_ = false;
    std::string error_;
};

class Gate
{
public:
    void EnterAndWait()
    {
        std::unique_lock lock(mutex_);
        entered_ = true;
        condition_.notify_all();
        condition_.wait(lock, [&] { return open_; });
    }

    void WaitUntilEntered()
    {
        std::unique_lock lock(mutex_);
        Check(condition_.wait_for(lock, 2s, [&] { return entered_; }), "worker entered gate");
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

struct FakeUser
{
    int uid = 0;
};

struct FakeControl
{
    bool GetSessionUser(const std::string& cookie, FakeUser* user)
    {
        seen_cookie = cookie;
        user->uid = 7;
        return true;
    }

    std::string CreateSession(int uid, const std::string& email)
    {
        created_uid = uid;
        created_email = email;
        return "new-session";
    }

    bool DestroySession(const std::string& session_id)
    {
        destroyed_session = session_id;
        return destroy_succeeds;
    }
    std::string GetSetCookieHeader(const std::string& id) { return "oj_session_id=" + id + "; Path=/"; }
    std::string GetClearCookieHeader() { return "oj_session_id=; Max-Age=0; Path=/"; }

    std::string seen_cookie;
    int created_uid = 0;
    std::string created_email;
    std::string destroyed_session;
    bool destroy_succeeds = true;
};

void CheckAcceptedAndException()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 4});
    Check(executor.Start(), "executor starts");

    oj::common::EmptyRequest request;
    oj::biz::AuthResponse response;
    FakeRpcController controller;
    CountingClosure done;
    auto result = oj::rpc::AsyncDispatch(
        executor, &controller, &request, &response, &done,
        [](const oj::common::EmptyRequest&, oj::biz::AuthResponse* output) {
            output->mutable_status()->set_success(true);
        });
    Check(result == oj::runtime::SubmitResult::Accepted, "dispatch accepted");
    Check(done.WaitFor(1), "accepted dispatch completes");
    Check(done.Count() == 1 && response.status().success(), "accepted dispatch done once");

    oj::biz::AuthResponse failed_response;
    CountingClosure failed_done;
    result = oj::rpc::AsyncDispatch(
        executor, &controller, &request, &failed_response, &failed_done,
        [] { throw std::runtime_error("unstable detail"); });
    Check(result == oj::runtime::SubmitResult::Accepted, "throwing dispatch accepted");
    Check(failed_done.WaitFor(1), "throwing dispatch completes");
    Check(failed_done.Count() == 1, "throwing dispatch done once");
    Check(failed_response.status().code() == oj::rpc::kDispatchException &&
              failed_response.status().message() == "INTERNAL_ERROR",
          "exception gets stable protobuf status");
    executor.Stop();
}

void CheckRejectedAndStopped()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 1});
    Check(executor.Start(), "bounded executor starts");
    Gate gate;
    Check(executor.Submit([&gate] { gate.EnterAndWait(); }) == oj::runtime::SubmitResult::Accepted,
          "blocking task accepted");
    gate.WaitUntilEntered();
    Check(executor.Submit([] {}) == oj::runtime::SubmitResult::Accepted, "queue filled");

    oj::common::EmptyRequest request;
    oj::biz::AuthResponse response;
    FakeRpcController controller;
    CountingClosure done;
    const auto result = oj::rpc::AsyncDispatch(executor, &controller, &request, &response, &done, [] {});
    Check(result == oj::runtime::SubmitResult::QueueFull, "full queue rejects dispatch");
    Check(done.WaitFor(1) && done.Count() == 1, "rejected dispatch done once");
    Check(response.status().message() == "BUSINESS_QUEUE_FULL" && response.status().retryable(),
          "queue rejection gets stable retryable status");
    gate.Open();
    executor.Stop();

    oj::biz::AuthResponse stopped_response;
    CountingClosure stopped_done;
    Check(oj::rpc::AsyncDispatch(executor, &controller, &request, &stopped_response, &stopped_done, [] {}) ==
              oj::runtime::SubmitResult::Stopped,
          "stopped executor rejects dispatch");
    Check(stopped_done.WaitFor(1) && stopped_done.Count() == 1, "stopped dispatch done once");
    Check(stopped_response.status().message() == "BUSINESS_EXECUTOR_STOPPED", "stopped status stable");
}

void CheckCancelPending()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 2});
    Check(executor.Start(), "cancel executor starts");
    Gate gate;
    Check(executor.Submit([&gate] { gate.EnterAndWait(); }) == oj::runtime::SubmitResult::Accepted,
          "cancel blocker accepted");
    gate.WaitUntilEntered();

    oj::common::EmptyRequest request;
    oj::biz::AuthResponse response;
    FakeRpcController controller;
    CountingClosure done;
    Check(oj::rpc::AsyncDispatch(executor, &controller, &request, &response, &done, [] {}) ==
              oj::runtime::SubmitResult::Accepted,
          "pending dispatch accepted before cancellation");
    std::thread stopper([&] { executor.Stop(oj::runtime::StopPolicy::CancelPending); });
    Check(done.WaitFor(1), "dropped pending dispatch completes");
    Check(done.Count() == 1 && response.status().message() == "BUSINESS_EXECUTOR_STOPPED",
          "dropped pending dispatch done once with stopped status");
    gate.Open();
    stopper.join();
}

void CheckCancelledBeforeRun()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 2});
    Check(executor.Start(), "pre-run cancel executor starts");
    Gate gate;
    Check(executor.Submit([&gate] { gate.EnterAndWait(); }) == oj::runtime::SubmitResult::Accepted,
          "pre-run cancel blocker accepted");
    gate.WaitUntilEntered();

    oj::common::EmptyRequest request;
    oj::biz::AuthResponse response;
    FakeRpcController controller;
    CountingClosure done;
    bool called = false;
    Check(oj::rpc::AsyncDispatch(executor, &controller, &request, &response, &done, [&called] { called = true; }) ==
              oj::runtime::SubmitResult::Accepted,
          "pre-run canceled dispatch initially accepted");
    controller.StartCancel();
    gate.Open();
    Check(done.WaitFor(1), "pre-run canceled dispatch completes");
    Check(done.Count() == 1 && !called && response.status().message() == "REQUEST_CANCELLED",
          "pre-run cancellation skips callable and runs done once");
    executor.Stop();
}

void CheckControllerFallback()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 1});
    oj::common::EmptyRequest request;
    oj::common::User response_without_status;
    FakeRpcController controller;
    CountingClosure done;
    Check(oj::rpc::AsyncDispatch(executor, &controller, &request, &response_without_status, &done, [] {}) ==
              oj::runtime::SubmitResult::Stopped,
          "fallback dispatch rejected");
    Check(done.WaitFor(1) && controller.Failed(), "response without status fails controller");
    Check(controller.ErrorText() == "BUSINESS_EXECUTOR_STOPPED", "controller failure is stable");
}

void CheckHttpTransportPolicy()
{
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 2});
    Check(executor.Start(), "transport executor starts");
    oj::common::EmptyRequest request;
    oj::biz::AuthResponse response;
    brpc::Controller controller;
    controller.http_request().set_method(brpc::HTTP_METHOD_POST);
    controller.http_request().set_content_type("application/json");
    CountingClosure done;
    bool called = false;
    Check(oj::rpc::AsyncDispatch(executor, &controller, &request, &response, &done,
                                 [&called] { called = true; }) ==
              oj::runtime::SubmitResult::Accepted,
          "invalid transport is accepted for bounded rejection");
    Check(done.WaitFor(1) && !called, "JSON transport skips business handler");
    Check(response.status().message() == "PROTOBUF_CONTENT_TYPE_REQUIRED",
          "JSON transport receives stable protobuf status, got " + response.status().message());
    Check(controller.http_response().status_code() == 415,
          "JSON transport receives HTTP 415, got " +
              std::to_string(controller.http_response().status_code()));
    executor.Stop();
}

void CheckSessions()
{
    brpc::Controller controller;
    controller.http_request().SetHeader("Cookie", "theme=dark; oj_session_id=abc123; x=y");
    butil::EndPoint endpoint;
    Check(butil::str2endpoint("192.0.2.10", 4321, &endpoint) == 0, "test endpoint parses");
    brpc::ControllerPrivateAccessor(&controller).set_remote_side(endpoint);

    FakeControl control;
    FakeUser user;
    Check(oj::rpc::SessionAdapter::GetSessionUser(controller, control, &user), "session user resolves");
    Check(user.uid == 7 && control.seen_cookie.find("oj_session_id=abc123") != std::string::npos,
          "cookie header passed to control");
    Check(oj::rpc::SessionAdapter::RemoteIp(controller) == "192.0.2.10", "remote IP excludes port");

    setenv("OJ_GATEWAY_AUTH_TOKEN", "test-gateway-token", 1);
    controller.http_request().SetHeader("X-OJ-Client-IP", "198.51.100.7");
    controller.http_request().SetHeader("X-OJ-Gateway-Token", "wrong-token");
    Check(oj::rpc::SessionAdapter::RemoteIp(controller) == "192.0.2.10",
          "untrusted client IP header is ignored");
    controller.http_request().SetHeader("X-OJ-Gateway-Token", "test-gateway-token");
    Check(oj::rpc::SessionAdapter::RemoteIp(controller) == "198.51.100.7",
          "gateway-authenticated client IP header is accepted");
    unsetenv("OJ_GATEWAY_AUTH_TOKEN");

    Check(oj::rpc::SessionAdapter::CreateSession(controller, control, 8, "u@example.test") == "new-session",
          "session created");
    Check(*controller.http_response().GetHeader("Set-Cookie") == "oj_session_id=new-session; Path=/",
          "set-cookie response header written");
    oj::rpc::SessionAdapter::DestroySession(controller, control);
    Check(control.destroyed_session == "abc123", "session ID parsed for destroy");
    Check(controller.http_response().GetHeader("Set-Cookie")->find("Max-Age=0") != std::string::npos,
          "clear-cookie response header written");

    brpc::Controller failed_controller;
    failed_controller.http_request().SetHeader("Cookie", "oj_session_id=retry-session");
    control.destroy_succeeds = false;
    Check(!oj::rpc::SessionAdapter::DestroySession(failed_controller, control),
          "unconfirmed revocation is retryable");
    Check(failed_controller.http_response().GetHeader("Set-Cookie") == nullptr,
          "failed revocation keeps cookie for retry");
}

void CheckProtoConversions()
{
    Check(oj::util::TimeUtil::GetTimeString(0) == "1970-01-01 08:00:00",
          "display timestamps use China Standard Time");
    Check(oj::util::TimeUtil::GetTimeString(123456) == "1970-01-01 08:00:00.123456",
          "display timestamps preserve microseconds");
    Check(oj::util::TimeUtil::GetTimeString(16LL * 60 * 60 * 1000000) ==
              "1970-01-02 00:00:00",
          "China Standard Time conversion handles day rollover");

    oj::db::User user{};
    user.uid = 42;
    user.name = "Ada";
    user.email = "ada@example.test";
    user.create_time = oj::util::TimeUtil::IntToDateTime(86400LL * 1000000);
    user.last_login = oj::util::TimeUtil::IntToDateTime(86401LL * 1000000);
    user.password_algo = "argon2id";
    oj::common::User proto_user;
    oj::rpc::ProtoAdapter::ToProto(user, &proto_user);
    Check(proto_user.uid() == 42 && proto_user.create_time() == 86400LL * 1000000 &&
              proto_user.last_login() == 86401LL * 1000000,
          "ODB user and datetimes convert");

    oj::db::Question question{};
    question.id = "10001";
    question.title = "Limits";
    question.star = "easy";
    question.cpu_limit = 2;
    question.memory_limit = 128;
    question.visible = true;
    oj::common::Question proto_question;
    oj::rpc::ProtoAdapter::ToProto(question, &proto_question);
    Check(proto_question.time_limit_ms() == 2000 && proto_question.memory_limit_mb() == 128,
          "question limits preserve database seconds and megabytes");

    oj::db::Solution solution{};
    solution.id = 9;
    solution.question_id = "10001";
    solution.user_id = 42;
    solution.title = "Typed path";
    solution.content_md = "body";
    solution.status = "approved";
    solution.created_at = oj::util::TimeUtil::IntToDateTime(1000000);
    solution.updated_at = oj::util::TimeUtil::IntToDateTime(2000000);
    oj::common::Solution proto_solution;
    oj::rpc::ProtoAdapter::ToProto(solution, &proto_solution);
    Check(proto_solution.id() == 9 && proto_solution.user_id() == 42 &&
              proto_solution.moderation_status() == "approved" && proto_solution.created_at() == 1000000,
          "ODB solution converts without a JSON intermediate");

    oj::db::Comment comment{};
    comment.id = 11;
    comment.solution_id = 9;
    comment.user_id = 42;
    comment.content = "top level";
    oj::common::Comment proto_comment;
    oj::rpc::ProtoAdapter::ToProto(comment, &proto_comment);
    Check(proto_comment.id() == 11 && proto_comment.parent_id() == 0 &&
              proto_comment.reply_to_user_id() == 0 && !proto_comment.is_edited() &&
              proto_comment.created_at() == 0 && proto_comment.updated_at() == 0,
          "nullable comment fields remain unset defaults");
    comment.parent_id = 7;
    comment.reply_to_user_id = 8;
    comment.is_edited = true;
    comment.created_at = oj::util::TimeUtil::IntToDateTime(3000000);
    oj::rpc::ProtoAdapter::ToProto(comment, &proto_comment);
    Check(proto_comment.parent_id() == 7 && proto_comment.reply_to_user_id() == 8 &&
              proto_comment.is_edited() && proto_comment.created_at() == 3000000,
          "present nullable comment fields convert directly");
    comment.parent_id.reset();
    comment.reply_to_user_id.reset();
    comment.is_edited.reset();
    comment.created_at.reset();
    oj::rpc::ProtoAdapter::ToProto(comment, &proto_comment);
    Check(proto_comment.parent_id() == 0 && proto_comment.reply_to_user_id() == 0 &&
              !proto_comment.is_edited() && proto_comment.created_at() == 0,
          "absent nullable fields clear reused protobuf output");

    Json::Value test;
    test["id"] = Json::UInt64(17);
    test["question_id"] = "00001";
    test["in"] = "1 2\n";
    test["out"] = "3\n";
    test["is_sample"] = true;
    oj::common::TestCase proto_test;
    oj::rpc::ProtoAdapter::ToProto(test, &proto_test);
    Check(proto_test.test_case_id() == 17 && proto_test.ordinal() == 17 &&
              proto_test.input() == "1 2\n" && proto_test.expected_output() == "3\n" && proto_test.is_sample(),
          "legacy test JSON aliases convert");

    oj::common::PageRequest page_request;
    page_request.set_page_size(1000);
    oj::common::PageResponse page_response;
    oj::rpc::ProtoAdapter::SetPage(page_request, 201, &page_response);
    Check(page_response.page() == 1 && page_response.page_size() == 100 && page_response.total_pages() == 3,
          "paging defaults and bounds are stable");
    Check(oj::rpc::ProtoAdapter::SubmissionStatus("WA") == oj::common::SUBMISSION_STATUS_WRONG_ANSWER,
          "legacy submission status converts");

    oj::common::StatusResponse status;
    oj::rpc::ProtoAdapter::SetError(&status, 429, "TOO_MANY_REQUESTS", true);
    Check(!status.success() && status.code() == 429 && status.retryable(), "status helper fills contract");
}

} // namespace

int main()
{
    CheckAcceptedAndException();
    CheckRejectedAndStopped();
    CheckCancelPending();
    CheckCancelledBeforeRun();
    CheckControllerFallback();
    CheckHttpTransportPolicy();
    CheckSessions();
    CheckProtoConversions();
    std::cout << "brpc_adapter_test: PASS\n";
    return 0;
}
