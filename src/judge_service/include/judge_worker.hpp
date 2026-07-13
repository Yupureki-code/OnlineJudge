#pragma once
#include "async/task.hpp"
#include "async/pending_task_manager.hpp"
#include "async/docker_task_awaitable.hpp"
#include "async/event_loop.hpp"
#include "pipeline/entry.hpp"
#include "sandbox/docker_sandbox.hpp"
#include "result_reporter.hpp"
#include "../../comm/mq_client.hpp"
#include "../../comm/proto/mq_message.pb.h"
#include <jsoncpp/json/json.h>
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace oj_judge {

/// JudgeWorker — 协程调度器
///
/// MQ 消息到达 → 创建协程 → 协程异步执行判题
/// 协程在 Docker 操作时挂起，容器通知后恢复
class JudgeWorker {
public:
    JudgeWorker(ns_async::JudgeEventLoop* loop,
                oj_sandbox::SandboxPool* runner_pool,
                std::shared_ptr<ResultReporter> reporter)
        : _loop(loop), _runner_pool(runner_pool), _reporter(reporter) {}

    /// MQ 消息回调
    /// @param body Protobuf 二进制消息体
    /// @param delivery_tag MQ 投递标签
    /// @param redelivered 是否重投
    void OnMQMessage(const std::string& body, uint64_t delivery_tag, bool redelivered) {
        // 解析 Protobuf 二进制消息
        oj::mq::JudgeTaskMessage task;
        if (!task.ParseFromString(body)) {
            // 解析失败，NACK 不重新入队
            return;
        }

        // 创建协程并启动
        auto coro = JudgeAsync(task, delivery_tag);
        coro.resume();  // 启动协程，遇到 co_await 时挂起

        // 保存协程句柄防止析构
        _active_coroutines.push_back(std::move(coro));
    }

private:
    ns_async::JudgeEventLoop* _loop;
    oj_sandbox::SandboxPool* _runner_pool;
    std::shared_ptr<ResultReporter> _reporter;
    std::vector<ns_async::Task<void>> _active_coroutines;

    /// 协程化判题主函数
    ns_async::Task<void> JudgeAsync(const oj::mq::JudgeTaskMessage& task,
                                      uint64_t delivery_tag) {
        // ① 写入源文件到编译容器
        // TODO: 创建编译容器，写入 source.cpp
        // 当前骨架：使用 fork 编译

        // ② 协程化编译
        ns_async::DockerTaskAwaitable compile_awaitable;
        compile_awaitable.container_id = "";  // 编译容器 ID
        compile_awaitable.script = "/judge/compile.sh";
        compile_awaitable.timeout_ms = 10000;
        compile_awaitable.start_container = [this](const std::string& cid,
                                                     const std::string& script,
                                                     const std::string& task_id) {
            // TODO: 创建编译容器并启动脚本
            // docker run -e TASK_ID=xxx -e JUDGE_HOST=xxx oj-compiler /judge/compile.sh
        };

        auto compile_result = co_await compile_awaitable;

        // ③ 检查编译结果
        if (compile_result.status == "COMPILE_ERROR") {
            // 回传编译错误
            if (_reporter) {
                _reporter->ReportResult(task.submission_id(), task.user_id(),
                                        task.question_id(), "CE", "", false);
            }
            co_return;
        }

        // ④ 获取运行容器（从预热池）
        auto sandbox = _runner_pool->Acquire();

        // ⑤ 逐个测试用例运行
        std::vector<std::string> results;
        for (int i = 0; i < task.test_cases_size(); ++i) {
            const auto& tc = task.test_cases(i);

            // 写入输入文件到运行容器
            sandbox->WriteFile("/home/judge/input.txt", tc.input());

            // 协程化运行
            ns_async::DockerTaskAwaitable run_awaitable;
            run_awaitable.container_id = sandbox->GetContainerId();
            run_awaitable.script = "/judge/run.sh";
            run_awaitable.timeout_ms = task.time_limit() + 1000;
            run_awaitable.start_container = [sandbox](const std::string& cid,
                                                       const std::string& script,
                                                       const std::string& task_id) {
                // 在容器内执行脚本
                // 设置环境变量: TASK_ID, JUDGE_HOST, TIME_LIMIT, MEM_LIMIT
                sandbox->Exec(
                    "TASK_ID=" + task_id +
                    " JUDGE_HOST=" + std::getenv("OJ_JUDGE_HOST") +
                    " TIME_LIMIT=" + std::to_string(0) +
                    " MEM_LIMIT=" + std::to_string(0) +
                    " /judge/run.sh",
                    30000);
            };

            auto run_result = co_await run_awaitable;

            // 判断结果
            if (run_result.timed_out) {
                results.push_back("TLE");
            } else if (run_result.status != "OK") {
                results.push_back(run_result.status);
            } else {
                // 读取输出并比较
                std::string user_output = sandbox->ReadFile("/home/judge/output.txt");
                if (RemoveWhitespace(user_output) == RemoveWhitespace(tc.expected_output())) {
                    results.push_back("AC");
                } else {
                    results.push_back("WA");
                }
            }
        }

        // ⑥ 归还运行容器
        _runner_pool->Release(std::move(sandbox));

        // ⑦ 汇总结果
        std::string overall_status = "AC";
        for (const auto& r : results) {
            if (r != "AC") { overall_status = r; break; }
        }

        // ⑧ 回传结果
        if (_reporter) {
            _reporter->ReportResult(task.submission_id(), task.user_id(),
                                    task.question_id(), overall_status, "", false);
        }

        co_return;
    }

    std::string RemoveWhitespace(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                result += c;
            }
        }
        return result;
    }
};

} // namespace ns_judge