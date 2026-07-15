// src/judge_service/main.cpp
// Judge Service 入口
//
// 职责：
// 1. brpc Server：DockerWorkDone（容器回调）
// 2. 协程事件循环：管理协程挂起/恢复
// 3. MQ Consumer：从 judge_task_queue 消费判题任务 → 创建协程
// 4. Docker Sandbox：编译容器（新建销毁）+ 运行容器（预热池复用）
// 5. Result Reporter：判题完成后通过 brpc RPC 回传结果给 Business Service

#include <brpc/server.h>
#include <string>
#include <memory>
#include <thread>
#include <cstdlib>
#include "include/judger.hpp"
#include "include/mq_consumer.hpp"
#include "include/result_reporter.hpp"
#include "include/docker_sandbox.hpp"
#include "include/event_loop.hpp"
#include "../comm/mq_client.hpp"
#include "../comm/logger.hpp"
#include "../comm/proto/judge_service.pb.h"

namespace oj_judge 
{

class JudgeServiceImpl : public JudgeService 
{
public:
    explicit JudgeServiceImpl(std::shared_ptr<JudgeEventLoop> loop)
        : _loop(std::move(loop)) {}

    void DockerWorkDone(::google::protobuf::RpcController* cntl_base,
                        const oj_judge::DockerWorkDoneRequest* req,
                        oj_judge::NullRsp* resp,
                        ::google::protobuf::Closure* done) override 
    {
        brpc::ClosureGuard done_guard(done);

        std::string task_id = req->id();
        std::string status = req->status();
        int exit_code = req->exit_code();

        LOG_INFO("DockerWorkDone task_id={} status={} exit_code={}", task_id, status, exit_code);

        oj_judge::DockerTaskResult result;
        result.status = status;
        result.exit_code = exit_code;
        result.timed_out = (status == "TIMEOUT");

        _loop->PushDockerDoneTask(task_id, result);
    }

private:
    std::shared_ptr<JudgeEventLoop> _loop;
};

} // namespace oj_judge

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (!ns_logger::InitLogger("judge_service", LOG_PATH + "judge_service.log", spdlog::level::info))
        LOG_ERROR("Failed to initialize judge_service file logger");

    auto event_loop = std::make_shared<oj_judge::JudgeEventLoop>();
    event_loop->Run(4);
    LOG_INFO("Event loop started with {} worker threads", 4);

    const char* biz_addr = std::getenv("OJ_BUSINESS_ADDR");
    if (!biz_addr) biz_addr = "127.0.0.1:8081";
    auto reporter = std::make_shared<oj_judge::ResultReporter>();
    try {
        reporter->Init(biz_addr);
        LOG_INFO("Result reporter initialized to {}", biz_addr);
    } catch (const std::exception& e) {
        LOG_WARNING("Result reporter unavailable: {}; judge_service will operate without result reporting", e.what());
        reporter = nullptr;  // allow service to run without Business Service
    }

    oj_sandbox::SandboxConfig runner_config;
    runner_config.image = "oj-runner:latest";
    runner_config.memory_limit_mb = 256;
    runner_config.cpu_limit_ms = 2000;
    runner_config.timeout_ms = 5000;
    runner_config.network_disabled = false;
    runner_config.read_only_root = false;
    auto runner_pool = std::make_unique<oj_sandbox::SandboxPool>(3, runner_config);
    LOG_INFO("Runner sandbox pool initialized with {} containers", 3);

    auto consumer = std::make_shared<oj_judge::JudgeConsumer>();

    ns_mq::MQConfig mq_config;
    const char* mq_host = std::getenv("OJ_MQ_HOST");
    mq_config.host = mq_host ? mq_host : "localhost";
    const char* mq_user = std::getenv("OJ_MQ_USER");
    mq_config.user = mq_user ? mq_user : "oj";
    const char* mq_pass = std::getenv("OJ_MQ_PASS");
    mq_config.password = mq_pass ? mq_pass : "ojpassword";
    mq_config.queue_name = "oj.judge.task";

    consumer->Init(mq_config, reporter, event_loop, runner_pool.get());
    consumer->Start();
    LOG_INFO("MQ consumer started for queue {}", mq_config.queue_name);

    brpc::Server server;
    oj_judge::JudgeServiceImpl judge_service(event_loop);

    if (server.AddService(&judge_service, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG_ERROR("Failed to add judge service");
        return -1;
    }

    brpc::ServerOptions options;
    int port = 8082;
    if (argc > 1) port = std::atoi(argv[1]);

    if (server.Start(port, &options) != 0) {
        LOG_ERROR("Failed to start judge service on port {}", port);
        return -1;
    }
    LOG_INFO("Judge Service brpc server running on port {}", port);

    server.RunUntilAskedToQuit();

    LOG_INFO("Shutting down Judge Service");
    consumer->Stop();
    runner_pool->Shutdown();
    ns_logger::ShutdownLogger();

    return 0;
}
