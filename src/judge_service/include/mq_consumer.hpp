#pragma once
#include "../../comm/mq_client.hpp"
#include "../../comm/proto/judge_service.pb.h"
#include "judger.hpp"
#include "result_reporter.hpp"
#include "judge_delivery_policy.hpp"
#include "judge_inbox.hpp"
#include "docker_sandbox.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <limits>
#include "event_loop.hpp"

namespace oj::judge
{

    /// 判题任务消费者：从 RabbitMQ 消费判题任务，执行判题，回传结果
    class JudgeConsumer 
    {
    private:
        /// 处理单条判题消息
        void RetryOrDeadLetter(const std::string& body, ns_mq::DeliveryContext delivery,
                               uint32_t retry_count,
                               uint32_t retry_after_ms)
        {
            if (ResolveDeliveryAction(ReportDecision::Retry, retry_count) == DeliveryAction::DeadLetter) {
                LOG_ERROR("Judge result delivery still unavailable after retries tag={}; retaining task",
                          delivery.delivery_tag);
                _consumer->Retry(delivery, body, std::max(retry_after_ms, 30000u), retry_count + 1);
                return;
            }
            _consumer->Retry(delivery, body, retry_after_ms, retry_count + 1);
        }

        void FinishDelivery(const std::string& body, ns_mq::DeliveryContext delivery,
                            const oj::mq::JudgeTaskMessage& request,
                            const oj::common::JudgeResult& result, uint32_t retry_count)
        {
            const ReportResult reported = _reporter ? _reporter->Report(request, result) : ReportResult{};
            const DeliveryAction action = ResolveDeliveryAction(reported.decision, retry_count);
            if (action == DeliveryAction::Ack) {
                _consumer->Ack(delivery);
            } else if (action == DeliveryAction::DeadLetter &&
                       reported.decision == ReportDecision::Reject) {
                _consumer->DeadLetter(delivery, body, retry_count);
            } else {
                RetryOrDeadLetter(body, delivery, retry_count, reported.retry_after_ms);
            }
        }

        void HandleMessage(const std::string& body, ns_mq::DeliveryContext delivery, bool redelivered,
                            uint32_t retry_count)
        {
            (void)redelivered;
            try 
            {
                const std::string fingerprint = JudgeInbox::PayloadFingerprint(body);
                // 反序列化 Protobuf 二进制消息
                oj::mq::JudgeTaskMessage request;
                const bool parsed = request.ParseFromString(body);
                if (!parsed || request.message_id().size() > 256 || !ValidJudgeTask(request)) {
                    LOG_ERROR("Rejecting malformed judge task tag={}", delivery.delivery_tag);
                    if (parsed && !request.message_id().empty() && request.message_id().size() <= 256 &&
                        (request.has_submission_id() || request.has_custom_task_id())) {
                        oj::common::JudgeResult terminal;
                        terminal.set_status(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
                        terminal.set_compile_error("invalid judge task payload");
                        terminal.set_completed_at(std::time(nullptr));
                        oj::common::JudgeResult existing;
                        std::string claim_token;
                        uint32_t retry_after_ms = 1000;
                        const auto claim = _inbox.Claim(request.message_id(), fingerprint, 60000,
                                                        &existing, &claim_token, &retry_after_ms);
                        if (claim == JudgeInbox::ClaimResult::Completed) terminal = std::move(existing);
                        else if (claim == JudgeInbox::ClaimResult::Processing) {
                            RetryOrDeadLetter(body, delivery, retry_count, retry_after_ms);
                            return;
                        } else if (claim == JudgeInbox::ClaimResult::Conflict) {
                            _consumer->DeadLetter(delivery, body, retry_count);
                            return;
                        } else if (claim != JudgeInbox::ClaimResult::Started ||
                                   _inbox.Complete(request.message_id(), fingerprint, claim_token,
                                                   terminal) != JudgeInbox::CompleteResult::Completed) {
                            RetryOrDeadLetter(body, delivery, retry_count, 1000);
                            return;
                        }
                        FinishDelivery(body, delivery, request, terminal, retry_count);
                        return;
                    }
                    _consumer->DeadLetter(delivery, body, retry_count);
                    return;
                }

                const int64_t calculated_timeout = 15000LL + request.test_cases_size() *
                    (static_cast<int64_t>(request.time_limit_ms()) + 6000);
                oj::common::JudgeResult completed;
                std::string claim_token;
                uint32_t retry_after_ms = 1000;
                const auto claim = _inbox.Claim(request.message_id(), fingerprint,
                    static_cast<uint32_t>(std::min<int64_t>(calculated_timeout + 30000, 3600000)),
                    &completed, &claim_token, &retry_after_ms);
                if (claim == JudgeInbox::ClaimResult::Completed) {
                    FinishDelivery(body, delivery, request, completed, retry_count);
                    return;
                }
                if (claim == JudgeInbox::ClaimResult::Processing) {
                    RetryOrDeadLetter(body, delivery, retry_count, retry_after_ms);
                    return;
                }
                if (claim == JudgeInbox::ClaimResult::Conflict) {
                    LOG_ERROR("Judge inbox fingerprint conflict message_id={}", request.message_id());
                    _consumer->DeadLetter(delivery, body, retry_count);
                    return;
                }
                if (claim == JudgeInbox::ClaimResult::Error) {
                    RetryOrDeadLetter(body, delivery, retry_count, 1000);
                    return;
                }

                // 创建 Judger 并注入运行容器预热池
                auto judger = std::make_shared<Judger>(_runner_pool);
                std::vector<TestCase> tests;
                tests.reserve(request.test_cases_size());
                for (const auto& item : request.test_cases()) {
                    tests.push_back({
                        .test_case_id = item.test_case_id(),
                        .index = item.index(),
                        .input = item.input(),
                        .output = item.expected_output(),
                        .cpu_limit = std::max(1, static_cast<int>(request.time_limit_ms() / 1000)),
                        .mem_limit = std::max(1, static_cast<int>(request.memory_limit_mb())),
                    });
                }
                judger->SetTestCases(tests);
                
                // 启动判题协程
                auto task = judger->Run(request);
                judger->SetTask(task);
                
                auto completion = [this, delivery, body, request, retry_count, fingerprint,
                                   claim_token, judger](
                                 const oj::common::JudgeResult& result) {
                    const auto completed = _inbox.Complete(request.message_id(), fingerprint,
                                                           claim_token, result);
                    if (completed != JudgeInbox::CompleteResult::Completed) {
                        LOG_ERROR("Failed to persist judge inbox completion message_id={}",
                                  request.message_id());
                        if (completed == JudgeInbox::CompleteResult::Conflict) {
                            _consumer->DeadLetter(delivery, body, retry_count);
                            return;
                        }
                        RetryOrDeadLetter(body, delivery, retry_count, 1000);
                        return;
                    }
                    FinishDelivery(body, delivery, request, result, retry_count);
                };
                _loop->PushNewTask(std::move(task),
                    static_cast<int>(std::min<int64_t>(calculated_timeout, 3600000)),
                    std::move(completion));
            } 
            catch (const std::exception& e) 
            {
                LOG_ERROR("HandleMessage failed: {}", e.what());
                RetryOrDeadLetter(body, delivery, retry_count, 1000);
            }
        }
    public:
        /// 初始化
        /// @param mq_config MQ 配置
        /// @param reporter 结果回传器
        /// @param loop 事件循环
        /// @param runner_pool 运行容器预热池
        void Init(const ns_mq::MQConfig& mq_config,
                  std::shared_ptr<ResultReporter> reporter,
                  std::shared_ptr<JudgeEventLoop> loop,
                  oj_sandbox::SandboxPool* runner_pool) 
        {
            _reporter = std::move(reporter);
            _loop = std::move(loop);
            _runner_pool = runner_pool;
            const char* redis_uri = std::getenv("OJ_REDIS_ADDR");
            if (redis_uri == nullptr || *redis_uri == '\0')
                throw std::runtime_error("OJ_REDIS_ADDR is required for the judge inbox");
            const char* prefix = std::getenv("OJ_JUDGE_INBOX_KEY_PREFIX");
            const char* completed_ttl = std::getenv("OJ_JUDGE_INBOX_COMPLETED_TTL_SECONDS");
            uint64_t completed_ttl_ms = 7ULL * 24 * 60 * 60 * 1000;
            if (completed_ttl && *completed_ttl) {
                const auto seconds = std::stoull(completed_ttl);
                if (seconds == 0 || seconds > std::numeric_limits<uint64_t>::max() / 1000)
                    throw std::runtime_error("invalid OJ_JUDGE_INBOX_COMPLETED_TTL_SECONDS");
                completed_ttl_ms = seconds * 1000;
            }
            _inbox.Open(redis_uri, prefix && *prefix ? prefix : "oj:prod:v1:judge:inbox:",
                        completed_ttl_ms);
            _consumer = std::make_unique<ns_mq::MQConsumer>();
            _consumer->Init(mq_config);

            _consumer->SetMessageHandler(
                [this](const std::string& body, ns_mq::DeliveryContext delivery,
                       bool redelivered, uint32_t retry_count) {
                    this->HandleMessage(body, delivery, redelivered, retry_count);
                });
        }

        /// 启动消费（阻塞当前线程）
        void Start() {
            _running.store(true);
            _consumer_thread = std::make_unique<std::thread>([this]() {
                _consumer->StartConsuming();
            });
        }

        /// 停止消费
        void Stop() {
            _running.store(false);
            _consumer->Close();
            if (_consumer_thread && _consumer_thread->joinable()) {
                _consumer_thread->join();
            }
        }

    private:
        std::unique_ptr<ns_mq::MQConsumer> _consumer;
        std::shared_ptr<ResultReporter> _reporter;
        std::shared_ptr<JudgeEventLoop> _loop;
        oj_sandbox::SandboxPool* _runner_pool;
        std::unique_ptr<std::thread> _consumer_thread;
        std::atomic<bool> _running{false};
        JudgeInbox _inbox;
    };

} // namespace oj::judge
