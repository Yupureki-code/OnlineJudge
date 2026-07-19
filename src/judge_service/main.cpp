// src/judge_service/main.cpp
// Judge Service 入口
//
// 职责：
// 1. brpc Server：Judge API（容器不回调服务）
// 2. 协程事件循环：管理协程挂起/恢复和宿主 Docker I/O
// 3. MQ Consumer：从 judge_task_queue 消费判题任务 → 创建协程
// 4. Docker Sandbox：编译和运行容器均按任务新建销毁
// 5. Result Reporter：判题完成后通过 brpc RPC 回传结果给 Business Service

#include <brpc/server.h>
#include <string>
#include <memory>
#include <thread>
#include <cstdlib>
#include <stdexcept>
#include "include/judger.hpp"
#include "include/mq_consumer.hpp"
#include "include/result_reporter.hpp"
#include "include/docker_sandbox.hpp"
#include "include/event_loop.hpp"
#include "../comm/mq_client.hpp"
#include "../comm/logger.hpp"
#include "../comm/proto/judge_service.pb.h"

using namespace oj::logger;

namespace
{

int ConcurrencyFromEnv(const char* name, int fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 1 || parsed > 64)
        throw std::invalid_argument(std::string(name) + " must be between 1 and 64");
    return static_cast<int>(parsed);
}

}

namespace oj::judge
{

class JudgeServiceImpl : public oj_judge::JudgeService
{
public:
    void DockerWorkDone(::google::protobuf::RpcController* cntl_base,
                        const oj_judge::DockerWorkDoneRequest* req,
                        oj_judge::NullRsp* resp,
                        ::google::protobuf::Closure* done) override 
    {
        brpc::ClosureGuard done_guard(done);

        (void)req;
        (void)resp;
        cntl_base->SetFailed("Container callbacks are disabled");
        LOG_WARNING("Rejected deprecated DockerWorkDone callback");
    }
};

} // namespace oj_judge

int main(int argc, char* argv[])
{
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (!oj::logger::InitLogger("judge_service", LOG_PATH + "judge_service.log", spdlog::level::info))
        LOG_ERROR("Failed to initialize judge_service file logger");

    const char* mq_user = std::getenv("OJ_MQ_USER");
    const char* mq_pass = std::getenv("OJ_MQ_PASS");
    if (mq_user == nullptr || *mq_user == '\0' || mq_pass == nullptr || *mq_pass == '\0') {
        LOG_CRITICAL("OJ_MQ_USER and OJ_MQ_PASS are required");
        oj::logger::ShutdownLogger();
        return 1;
    }

    int worker_count = 0;
    int prefetch_count = 0;
    try {
        worker_count = ConcurrencyFromEnv("OJ_JUDGE_WORKERS", 4);
        prefetch_count = ConcurrencyFromEnv("OJ_JUDGE_PREFETCH", 4);
    } catch (const std::exception& e) {
        LOG_CRITICAL("Invalid Judge concurrency configuration: {}", e.what());
        oj::logger::ShutdownLogger();
        return 1;
    }

    auto event_loop = std::make_shared<oj::judge::JudgeEventLoop>();
    event_loop->Run(worker_count);
    LOG_INFO("Event loop started with {} worker threads", worker_count);

    latecyMonitor::LatencyMonitor latency_monitor;
    try {
        if (!latency_monitor.configureFromEnv("judge_service") || !latency_monitor.start())
            throw std::runtime_error("failed to start judge latency monitor");
        latency_monitor.enable(true);
    } catch (const std::exception& e) {
        LOG_CRITICAL("Judge latency monitor initialization failed: {}", e.what());
        oj::logger::ShutdownLogger();
        return 1;
    }

    const char* biz_addr = std::getenv("OJ_SERVER_RPC_ADDR");
    if (!biz_addr) biz_addr = "127.0.0.1:8080";
    auto reporter = std::make_shared<oj::judge::ResultReporter>();
    try {
        reporter->Init(biz_addr);
        LOG_INFO("Result reporter initialized to {}", biz_addr);
    } catch (const std::exception& e) {
        LOG_CRITICAL("Result reporter initialization failed: {}", e.what());
        oj::logger::ShutdownLogger();
        return 1;
    }

    oj_sandbox::SandboxConfig runner_config;
    runner_config.image = "oj-runner:latest";
    runner_config.memory_limit_mb = 256;
    runner_config.cpu_limit_ms = 2000;
    runner_config.timeout_ms = 5000;
    runner_config.network_disabled = true;
    runner_config.read_only_root = true;
    auto runner_pool = std::make_unique<oj_sandbox::SandboxPool>(3, runner_config);
    LOG_INFO("Runner sandbox pool initialized with {} containers", 3);

    auto consumer = std::make_shared<oj::judge::JudgeConsumer>();

    ns_mq::MQConfig mq_config;
    const char* mq_host = std::getenv("OJ_MQ_HOST");
    mq_config.host = mq_host ? mq_host : "localhost";
    mq_config.user = mq_user;
    mq_config.password = mq_pass;
    mq_config.queue_name = "oj.judge.task";
    mq_config.prefetch_count = prefetch_count;
    LOG_INFO("Judge MQ prefetch configured to {}", mq_config.prefetch_count);

    try {
        consumer->Init(mq_config, reporter, event_loop, runner_pool.get(), &latency_monitor);
        consumer->Start();
    } catch (const std::exception& e) {
        LOG_CRITICAL("MQ consumer initialization failed: {}", e.what());
        runner_pool->Shutdown();
        latency_monitor.stop();
        oj::logger::ShutdownLogger();
        return 1;
    }
    LOG_INFO("MQ consumer started for queue {}", mq_config.queue_name);

    brpc::Server server;
    oj::judge::JudgeServiceImpl judge_service;

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
    event_loop->Stop();
    runner_pool->Shutdown();
    latency_monitor.stop();
    oj::logger::ShutdownLogger();

    return 0;
}
