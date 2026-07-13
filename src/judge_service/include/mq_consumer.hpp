#pragma once
#include "../../comm/mq_client.hpp"
#include "../../comm/proto/judge_service.pb.h"
#include "async/task.hpp"
#include "pipeline/entry.hpp"
#include "result_reporter.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include "async/event_loop.hpp"

namespace oj_judge 
{

    /// 判题任务消费者：从 RabbitMQ 消费判题任务，执行判题，回传结果
    class JudgeConsumer 
    {
    private:
        /// 处理单条判题消息
        void HandleMessage(const std::string& body, uint64_t tag, bool redelivered) 
        {
            try {
                // 执行判题
                SubmitRequest request;
                request.ParseFromString(body);
                // TODO: 从数据库查询测试用例并注入 entry.SetTestCases(...)
                _loop->PushTask([this,request](std::coroutine_handle<> h)->ns_async::Task<void>{
                    auto result = _entry->Run(request,h);
                });

                // 解析判题结果
                Json::Value result;
                std::string status = result["status"].asString();

                // 回传结果
                // if (_reporter) {
                //     _reporter->ReportResult(submission_id, user_id, question_id,
                //                             status, result_json, is_custom_test);
                // }

                // ACK 消息
                _consumer->Ack(tag);

            } catch (const std::exception& e) {
                // 判题失败，NACK 并重新入队（限制重试次数）
                _consumer->Nack(tag, redelivered ? false : true);
            }
        }
    public:
        /// 初始化
        /// @param mq_config MQ 配置
        /// @param reporter 结果回传器
        void Init(const ns_mq::MQConfig& mq_config,
                std::shared_ptr<oj_judge::HandlerEntry> entry,
                std::shared_ptr<ResultReporter> reporter) 
        {
            _entry = entry;
            _reporter = std::move(reporter);
            _consumer = std::make_unique<ns_mq::MQConsumer>();
            _consumer->Init(mq_config);

            _consumer->SetMessageHandler(
                [this](const std::string& body, uint64_t tag, bool redelivered) {
                    this->HandleMessage(body, tag, redelivered);
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
        std::unique_ptr<std::thread> _consumer_thread;
        std::shared_ptr<oj_judge::HandlerEntry> _entry;
        std::shared_ptr<JudgeEventLoop> _loop;
        std::atomic<bool> _running{false};
    };

} // namespace ns_judge