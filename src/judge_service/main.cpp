// src/judge_service/main.cpp
// Judge Service 入口
//
// 职责：
// 1. brpc Server：DockerWorkDone（容器回调）、Submit、GetStatus
// 2. libevent 事件循环：管理协程挂起/恢复
// 3. MQ Consumer：从 judge_task_queue 消费判题任务 → 创建协程
// 4. Docker Sandbox：编译容器（新建销毁）+ 运行容器（预热池复用）
// 5. Result Reporter：判题完成后通过 brpc RPC 回传结果给 Business Service

#include <brpc/server.h>
#include <butil/logging.h>
#include <jsoncpp/json/json.h>
#include <string>
#include <memory>
#include <thread>
#include <cstdlib>

#include "include/pipeline/entry.hpp"
#include "include/mq_consumer.hpp"
#include "include/result_reporter.hpp"
#include "include/sandbox/docker_sandbox.hpp"
#include "include/judge_worker.hpp"
#include "include/docker_work_done_service.hpp"
#include "include/async/event_loop.hpp"
#include "include/async/pending_task_manager.hpp"
#include "../comm/mq_client.hpp"
#include "../comm/proto/judge_service.pb.h"
#include "../comm/proto/mq_message.pb.h"

namespace oj_judge
{
    class JudgeServer
    {
    public:
    private:
        std::shared_ptr<JudgeConsumer> _mq;
        std::shared_ptr<HandlerEntry> _entry;
        std::shared_ptr<JudgeEventLoop> _loop;
    };
}

// ==================== main ====================

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // ① 初始化事件循环
    ns_async::JudgeEventLoop event_loop;

    // ② 初始化结果回传器
    const char* biz_addr = std::getenv("OJ_BUSINESS_ADDR");
    if (!biz_addr) biz_addr = "127.0.0.1:8081";
    auto reporter = std::make_shared<oj_judge::ResultReporter>();
    try {
        reporter->Init(biz_addr);
        LOG(INFO) << "Result reporter initialized to " << biz_addr;
    } catch (const std::exception& e) {
        LOG(ERROR) << "Failed to init result reporter: " << e.what();
    }

    // ③ 初始化运行容器预热池
    oj_sandbox::SandboxConfig runner_config;
    runner_config.image = "oj-runner:latest";
    runner_config.memory_limit_mb = 256;
    runner_config.cpu_limit_ms = 2000;
    runner_config.timeout_ms = 5000;
    auto runner_pool = std::make_unique<oj_sandbox::SandboxPool>(3, runner_config);
    LOG(INFO) << "Runner sandbox pool initialized (3 containers)";

    // ④ 初始化 JudgeWorker（协程调度器）
    oj_judge::JudgeWorker judge_worker(&event_loop, runner_pool.get(), reporter);

    // ⑤ 初始化 MQ Consumer
    ns_mq::MQConfig mq_config;
    const char* mq_host = std::getenv("OJ_MQ_HOST");
    mq_config.host = mq_host ? mq_host : "localhost";
    const char* mq_user = std::getenv("OJ_MQ_USER");
    mq_config.user = mq_user ? mq_user : "oj";
    const char* mq_pass = std::getenv("OJ_MQ_PASS");
    mq_config.password = mq_pass ? mq_pass : "ojpassword";
    mq_config.queue_name = "oj.judge.task";

    ns_mq::MQManager::Instance().InitConsumer(mq_config);
    ns_mq::MQManager::Instance().GetConsumer().SetMessageHandler(
        [&judge_worker](const std::string& body, uint64_t tag, bool redelivered) {
            judge_worker.OnMQMessage(body, tag, redelivered);
        });

    // ⑥ 启动 MQ 消费线程
    std::thread mq_thread([&mq_config]() {
        ns_mq::MQManager::Instance().GetConsumer().StartConsuming();
    });
    LOG(INFO) << "MQ consumer started for queue " << mq_config.queue_name;

    // ⑦ 启动 brpc Server（DockerWorkDone + Submit + GetStatus）
    brpc::Server server;
    ns_judge::DockerWorkDoneServiceImpl judge_service(&event_loop);

    if (server.AddService(&judge_service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Failed to add judge service";
        return -1;
    }

    brpc::ServerOptions options;
    int port = 8082;
    if (argc > 1) port = std::atoi(argv[1]);

    if (server.Start(port, &options) != 0) {
        LOG(ERROR) << "Failed to start judge service on port " << port;
        return -1;
    }
    LOG(INFO) << "Judge Service brpc server running on port " << port;

    // ⑧ 启动事件循环（阻塞主线程）
    LOG(INFO) << "Starting event loop...";
    event_loop.Run();

    // ⑨ 清理
    event_loop.Stop();
    server.Stop(0);
    ns_mq::MQManager::Instance().Shutdown();
    if (mq_thread.joinable()) mq_thread.join();
    runner_pool->Shutdown();

    return 0;
}