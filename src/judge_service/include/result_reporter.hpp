#pragma once

#include <brpc/channel.h>
#include <brpc/controller.h>
#include <openssl/rand.h>

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../comm/logger.hpp"
#include "../../comm/judge_callback_auth.hpp"
#include "../../comm/proto/biz_service.pb.h"
#include "../../comm/proto/mq_message.pb.h"
#include "report_decision.hpp"

namespace oj::judge
{

struct ReportResult
{
    ReportDecision decision = ReportDecision::Retry;
    uint32_t retry_after_ms = 1000;
};

class ResultReporter
{
public:
    void Init(const std::string& business_service_addr)
    {
        const char* key_id = std::getenv("OJ_JUDGE_CALLBACK_KEY_ID");
        const char* secret = std::getenv("OJ_JUDGE_CALLBACK_SECRET");
        if (key_id == nullptr || *key_id == '\0' || secret == nullptr || *secret == '\0')
            throw std::runtime_error("judge callback signing credentials are not configured");
        key_id_ = key_id;
        secret_ = secret;
        brpc::ChannelOptions options;
        options.timeout_ms = 5000;
        options.max_retry = 0;
        channel_ = std::make_unique<brpc::Channel>();
        if (channel_->Init(business_service_addr.c_str(), "", &options) != 0)
            throw std::runtime_error("failed to initialize Business Service channel");
        stub_ = std::make_unique<oj::biz::OJService::Stub>(channel_.get());
    }

    ReportResult Report(const oj::mq::JudgeTaskMessage& task, const oj::common::JudgeResult& result)
    {
        if (!stub_) return {};
        oj::biz::UpdateJudgeResultRequest request;
        request.set_message_id(task.message_id());
        if (task.has_submission_id()) request.set_submission_id(task.submission_id());
        else if (task.has_custom_task_id()) request.set_custom_task_id(task.custom_task_id());
        else return {ReportDecision::Reject, 0};
        if (!result.SerializeToString(request.mutable_result_payload()))
            return {ReportDecision::Reject, 0};
        auto* auth = request.mutable_service_auth();
        auth->set_key_id(key_id_);
        auth->set_timestamp(std::time(nullptr));
        auth->set_nonce(RandomHex(16));
        if (auth->nonce().empty()) return {};
        auth->set_hmac_sha256(oj::security::JudgeCallbackHmac(request, secret_));

        oj::biz::UpdateJudgeResultResponse response;
        brpc::Controller controller;
        controller.set_timeout_ms(5000);
        stub_->UpdateJudgeResult(&controller, &request, &response, nullptr);
        if (controller.Failed()) {
            LOG_ERROR("UpdateJudgeResult transport failure message_id={} error={}",
                      task.message_id(), controller.ErrorText());
            return {};
        }
        switch (response.outcome()) {
            case oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_PERSISTED:
            case oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_IDEMPOTENT:
                report_count_.fetch_add(1, std::memory_order_relaxed);
                return {ReportDecision::Ack, 0};
            case oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED:
                return {ReportDecision::Reject, 0};
            case oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE:
            default:
                return {ReportDecision::Retry,
                        response.retry_after_ms() == 0 ? 1000u : response.retry_after_ms()};
        }
    }

    uint64_t GetReportCount() const { return report_count_.load(std::memory_order_relaxed); }

private:
    static std::string RandomHex(std::size_t bytes)
    {
        std::string random(bytes, '\0');
        if (RAND_bytes(reinterpret_cast<unsigned char*>(random.data()), bytes) != 1) return {};
        static constexpr char hex[] = "0123456789abcdef";
        std::string output(bytes * 2, '0');
        for (std::size_t index = 0; index < bytes; ++index) {
            const auto value = static_cast<unsigned char>(random[index]);
            output[index * 2] = hex[value >> 4];
            output[index * 2 + 1] = hex[value & 0x0f];
        }
        return output;
    }

    std::string key_id_;
    std::string secret_;
    std::unique_ptr<brpc::Channel> channel_;
    std::unique_ptr<oj::biz::OJService::Stub> stub_;
    std::atomic<uint64_t> report_count_{0};
};

} // namespace oj::judge
