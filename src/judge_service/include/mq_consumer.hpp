#pragma once
#include "../../comm/mq_client.hpp"
#include "../../comm/proto/judge_service.pb.h"
#include "judger.hpp"
#include "result_reporter.hpp"
#include "docker_sandbox.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include "event_loop.hpp"

namespace oj_judge 
{

    /// 判题任务消费者：从 RabbitMQ 消费判题任务，执行判题，回传结果
    class JudgeConsumer 
    {
    private:
        /// 处理单条判题消息
        void HandleMessage(const std::string& body, uint64_t tag, bool redelivered) 
        {
            try 
            {
                // 反序列化 Protobuf 二进制消息
                SubmitRequest request;
                request.ParseFromString(body);

                // 创建 Judger 并注入运行容器预热池
                Judger judger(_runner_pool);
                
                // TODO: 从数据库查询测试用例并注入 judger.SetTestCases(...)
                // 当前使用空测试用例，Phase 2 集成 ODB 后补全
                
                // 启动判题协程
                auto task = judger.Run(request);
                judger.SetTask(task);
                
                // 注册到事件循环，获取 task_id
                auto& promise = task.GetHandle().promise();
                std::string task_id = _loop->PushNewTask(std::move(task));
                promise.task_id = task_id;
                
                // 设置协程完成回调：回传结果 + ACK 消息
                promise.cb = [this, tag](const JudgeFinishedRequest& request) {
                    // 回传判题结果给 Business Service（可选）
                    if (_reporter) {
                        _reporter->ReportResultWithRetry(request);
                    }
                    // ACK 消息（判题完成，从 MQ 移除）
                    _consumer->Ack(tag);
                };
            } 
            catch (const std::exception& e) 
            {
                LOG_ERROR("HandleMessage failed: {}", e.what());
                // 判题失败，NACK 并重新入队（限制重试次数）
                _consumer->Nack(tag, redelivered ? false : true);
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
        std::shared_ptr<JudgeEventLoop> _loop;
        oj_sandbox::SandboxPool* _runner_pool;
        std::unique_ptr<std::thread> _consumer_thread;
        std::atomic<bool> _running{false};
    };

} // namespace oj_judge
