#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>

#include <brpc/controller.h>

#include "control/oj_control.hpp"
#include "oj_admin.hpp"
#include "rpc/oj_admin_service_impl.hpp"
#include "runtime/business_executor.hpp"

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

class Done final : public google::protobuf::Closure
{
public:
    void Run() override
    {
        called_.store(true, std::memory_order_release);
        condition_.notify_all();
    }

    bool Wait()
    {
        std::unique_lock lock(mutex_);
        return condition_.wait_for(lock, 2s, [this] { return called_.load(std::memory_order_acquire); });
    }

private:
    std::atomic_bool called_{false};
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
        condition_.wait(lock, [this] { return open_; });
    }

    void WaitEntered()
    {
        std::unique_lock lock(mutex_);
        Check(condition_.wait_for(lock, 2s, [this] { return entered_; }), "worker reaches gate");
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

void CheckNamingAndRoles()
{
    const auto* descriptor = oj::rpc::OJAdminServiceImpl::descriptor();
    Check(descriptor->full_name() == "oj.biz.OJAdminService", "generated service name is stable");
    Check(descriptor->FindMethodByName("AdminGetVersion") != nullptr, "version RPC naming is stable");
    Check(oj::rpc::OJAdminServiceImpl::IsAdminRole("admin"), "admin role accepted");
    Check(oj::rpc::OJAdminServiceImpl::IsAdminRole("super_admin"), "super role accepted");
    Check(!oj::rpc::OJAdminServiceImpl::IsAdminRole("user"), "user role rejected");
    Check(oj::rpc::OJAdminServiceImpl::IsSuperAdminRole("super_admin"), "super-only role accepted");
    Check(!oj::rpc::OJAdminServiceImpl::IsSuperAdminRole("admin"), "ordinary admin rejected for super-only");
}

void CheckNoDatabasePaths()
{
    oj::control::Control control;
    oj::admin::AdminSessionStore sessions(false);
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 4});
    Check(executor.Start(), "executor starts");
    oj::rpc::OJAdminServiceImpl service(control, sessions, executor);
    TestController controller;

    oj::biz::AdminLoginRequest login;
    oj::biz::AdminLoginResponse login_response;
    Done login_done;
    service.AdminLogin(&controller, &login, &login_response, &login_done);
    Check(login_done.Wait(), "invalid login completes");
    Check(login_response.status().code() == 400 && login_response.status().message() == "INVALID_ARGUMENT",
          "invalid login fails before database access");

    oj::biz::AdminUserIdRequest get_user;
    oj::biz::AdminUserResponse get_user_response;
    Done get_user_done;
    service.AdminGetUser(&controller, &get_user, &get_user_response, &get_user_done);
    Check(get_user_done.Wait(), "unimplemented user RPC completes");
    Check(get_user_response.status().code() == 501 &&
              get_user_response.status().message() == "NOT_IMPLEMENTED",
          "admin user CRUD has stable 501 status");

    executor.Stop();
}

void CheckQueueRejection()
{
    oj::control::Control control;
    oj::admin::AdminSessionStore sessions(false);
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 1});
    Check(executor.Start(), "bounded executor starts");
    oj::rpc::OJAdminServiceImpl service(control, sessions, executor);
    Gate gate;
    Check(executor.Submit([&gate] { gate.Block(); }) == oj::runtime::SubmitResult::Accepted,
          "blocking task accepted");
    gate.WaitEntered();
    Check(executor.Submit([] {}) == oj::runtime::SubmitResult::Accepted, "queue filled");

    TestController controller;
    oj::common::EmptyRequest request;
    oj::biz::GetVersionResponse response;
    Done done;
    service.AdminGetVersion(&controller, &request, &response, &done);
    Check(done.Wait(), "rejected RPC completes");
    Check(response.status().code() == 503 && response.status().message() == "BUSINESS_QUEUE_FULL" &&
              response.status().retryable(),
          "queue rejection status is stable");
    gate.Open();
    executor.Stop();
}

void CheckLogoutRevocationFailure()
{
    oj::control::Control control;
    oj::admin::AdminSessionStore sessions(true, "tcp://127.0.0.1:1");
    oj::runtime::BusinessExecutor executor({.worker_count = 1, .queue_capacity = 2});
    Check(executor.Start(), "logout executor starts");
    oj::rpc::OJAdminServiceImpl service(control, sessions, executor);
    TestController controller;
    controller.http_request().set_method(brpc::HTTP_METHOD_POST);
    controller.http_request().set_content_type("application/protobuf");
    controller.http_request().SetHeader("Cookie", "oj_admin_session_id=unconfirmed");
    oj::common::EmptyRequest request;
    oj::common::EmptyResponse response;
    Done done;
    service.AdminLogout(&controller, &request, &response, &done);
    Check(done.Wait(), "failed admin revocation completes");
    Check(response.status().code() == 503 && response.status().message() == "SESSION_REVOCATION_FAILED" &&
              response.status().retryable(),
          "failed admin revocation returns retryable status");
    Check(controller.http_response().GetHeader("Set-Cookie") == nullptr,
          "failed admin revocation does not clear retry cookie");
    executor.Stop();
}

} // namespace

int main()
{
    CheckNamingAndRoles();
    CheckNoDatabasePaths();
    CheckQueueRejection();
    CheckLogoutRevocationFailure();
    std::cout << "oj_admin_service_rpc_test: PASS\n";
    return 0;
}
