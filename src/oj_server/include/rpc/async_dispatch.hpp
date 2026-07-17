#pragma once

#include <atomic>
#include <algorithm>
#include <cctype>
#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <brpc/controller.h>
#include <google/protobuf/service.h>

#include "../../../comm/proto/common.pb.h"
#include "../runtime/business_executor.hpp"

namespace oj::rpc
{

    inline constexpr int kDispatchQueueFull = 503;
    inline constexpr int kDispatchStopped = 503;
    inline constexpr int kDispatchCancelled = 499;
    inline constexpr int kDispatchException = 500;

    namespace detail
    {

        template <typename Response>
        concept HasMutableStatus = requires(Response* response) {
            { response->mutable_status() } -> std::convertible_to<oj::common::StatusResponse*>;
        };

        template <typename Response>
        void SetDispatchFailure(google::protobuf::RpcController* controller,
                                Response* response,
                                int code,
                                const char* message,
                                bool retryable) noexcept
        {
            try {
                if constexpr (HasMutableStatus<Response>) {
                    if (response != nullptr) {
                        auto* status = response->mutable_status();
                        status->set_success(false);
                        status->set_code(code);
                        status->set_message(message);
                        status->set_retryable(retryable);
                        return;
                    }
                }
                if (controller != nullptr)
                    controller->SetFailed(message);
            } catch (...) {
                if (controller != nullptr) {
                    try {
                        controller->SetFailed("RPC_DISPATCH_FAILURE");
                    } catch (...) {
                    }
                }
            }
        }

        template <typename Response>
        class DispatchCompletion
        {
        public:
            DispatchCompletion(google::protobuf::RpcController* controller,
                            Response* response,
                            google::protobuf::Closure* done)
                : controller_(controller), response_(response), done_(done)
            {
            }

            ~DispatchCompletion()
            {
                Fail(kDispatchStopped, "BUSINESS_EXECUTOR_STOPPED", true);
            }

            void Finish() noexcept
            {
                RunDoneOnce();
            }

            void Fail(int code, const char* message, bool retryable) noexcept
            {
                if (completed_.exchange(true, std::memory_order_acq_rel))
                    return;
                SetDispatchFailure(controller_, response_, code, message, retryable);
                RunDone();
            }

        private:
            void RunDoneOnce() noexcept
            {
                if (!completed_.exchange(true, std::memory_order_acq_rel))
                    RunDone();
            }

            void RunDone() noexcept
            {
                if (done_ != nullptr) {
                    try {
                        done_->Run();
                    } catch (...) {
                    }
                }
            }

            google::protobuf::RpcController* controller_;
            Response* response_;
            google::protobuf::Closure* done_;
            std::atomic_bool completed_{false};
        };

        template <typename Callable, typename Request, typename Response>
        void Invoke(Callable& callable,
                    google::protobuf::RpcController* controller,
                    const Request* request,
                    Response* response)
        {
            if constexpr (std::invocable<Callable&, google::protobuf::RpcController*, const Request*, Response*>)
                std::invoke(callable, controller, request, response);
            else if constexpr (std::invocable<Callable&, brpc::Controller*, const Request*, Response*>)
                std::invoke(callable, static_cast<brpc::Controller*>(controller), request, response);
            else if constexpr (std::invocable<Callable&, const Request&, Response*>)
                std::invoke(callable, *request, response);
            else if constexpr (std::invocable<Callable&, const Request&, Response&>)
                std::invoke(callable, *request, *response);
            else if constexpr (std::invocable<Callable&, Response*>)
                std::invoke(callable, response);
            else
                std::invoke(callable);
        }

    } // namespace detail

    template <typename Request, typename Response, typename Callable>
    oj::runtime::SubmitResult AsyncDispatch(oj::runtime::BusinessExecutor& executor,
                                            google::protobuf::RpcController* controller,
                                            const Request* request,
                                            Response* response,
                                            google::protobuf::Closure* done,
                                            Callable&& callable)
    {
        using Completion = detail::DispatchCompletion<Response>;
        using Work = std::decay_t<Callable>;
        auto completion = std::make_shared<Completion>(controller, response, done);

        auto task = [completion, controller, request, response,
                    work = Work(std::forward<Callable>(callable))]() mutable {
            if (controller != nullptr) {
                auto* brpc_controller = dynamic_cast<brpc::Controller*>(controller);
                if (brpc_controller != nullptr && brpc_controller->has_http_request()) {
                    if (brpc_controller->http_request().method() != brpc::HTTP_METHOD_POST) {
                        brpc_controller->http_response().set_status_code(405);
                        brpc_controller->http_response().SetHeader("Allow", "POST");
                        completion->Fail(405, "METHOD_NOT_ALLOWED", false);
                        return;
                    }
                    std::string content_type = brpc_controller->http_request().content_type();
                    const std::size_t separator = content_type.find(';');
                    if (separator != std::string::npos) content_type.resize(separator);
                    std::transform(content_type.begin(), content_type.end(), content_type.begin(),
                        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (content_type != "application/proto" &&
                        content_type != "application/protobuf" &&
                        content_type != "application/octet-stream") {
                        brpc_controller->http_response().set_status_code(415);
                        completion->Fail(415, "PROTOBUF_CONTENT_TYPE_REQUIRED", false);
                        return;
                    }
                }
            }
            if (controller != nullptr && controller->IsCanceled()) {
                completion->Fail(kDispatchCancelled, "REQUEST_CANCELLED", false);
                return;
            }
            try {
                detail::Invoke(work, controller, request, response);
                completion->Finish();
            } catch (...) {
                completion->Fail(kDispatchException, "INTERNAL_ERROR", false);
            }
        };

        const auto result = executor.Submit(std::move(task));
        if (result == oj::runtime::SubmitResult::QueueFull)
            completion->Fail(kDispatchQueueFull, "BUSINESS_QUEUE_FULL", true);
        else if (result == oj::runtime::SubmitResult::Stopped)
            completion->Fail(kDispatchStopped, "BUSINESS_EXECUTOR_STOPPED", true);
        return result;
    }

} // namespace oj::rpc
