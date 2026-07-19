#pragma once
#include "../../comm/comm.hpp"
#include "../../comm/logger.hpp"
#include "../../comm/latecymonitor.hpp"
#include "../../comm/filesystem.hpp"
#include "../../comm/proto/mq_message.pb.h"
#include <memory>
#include <ctime>
#include "event_loop.hpp"
#include "docker_sandbox.hpp"
#include "judge_delivery_policy.hpp"

namespace oj::judge
{

/// 判题链入口：组装 preprocesser → compiler → runner → judge
struct TestCase 
{
    uint64_t test_case_id = 0;
    uint32_t index = 0;
    std::string input;
    std::string output;
    int cpu_limit;    // 秒
    int mem_limit;    // MB
};

class Judger
{
public:
    Judger() : _runner_pool(nullptr), _latency_monitor(nullptr) {}
    explicit Judger(oj_sandbox::SandboxPool* pool, latecyMonitor::LatencyMonitor* latency_monitor = nullptr)
        : _runner_pool(pool), _latency_monitor(latency_monitor) {}

    /// 运行判题链
    /// @param request 判题请求（包含 code, question_id, time_limit 等）
    /// @return 判题结果
    CoTask Run(oj::mq::JudgeTaskMessage request)
    {
        std::unique_ptr<latecyMonitor::Timer> latency_timer;
        if (_latency_monitor != nullptr) {
            latency_timer = std::make_unique<latecyMonitor::Timer>(
                *_latency_monitor,
                "judge_service JudgerCoTask.Run at " + oj::util::TimeUtil::GetTimeString());
        }
        LOG_INFO("Judger started message_id={}", request.message_id());
        // ① 生成唯一文件名，写入源代码到宿主机临时目录
        std::string file_name = oj::util::StringUtil::GetUniqueName();
        std::string src_path = oj::util::PathUtil::Src(file_name);
        std::string exe_path = oj::util::PathUtil::Exe(file_name);
        std::string err_path = oj::util::PathUtil::Compile_err(file_name);
        struct HostFileCleanup {
            fileUtil::FileSystem& filesystem;
            std::vector<std::string> paths;
            ~HostFileCleanup() {
                for (const auto& path : paths) {
                    try { filesystem.remove(path); } catch (...) {}
                }
            }
        } cleanup{_file_system, {src_path, exe_path, err_path}};
        _file_system.write(src_path, request.code());
        LOG_DEBUG("Submission source staged");

        // ② 创建编译容器（每次新建，编译完销毁）
        oj_sandbox::DockerSandbox compile_sandbox;
        oj_sandbox::SandboxConfig compile_config;
        compile_config.image = "oj-compiler:latest";
        compile_config.network_disabled = true;
        compile_config.read_only_root = true;
        compile_config.work_dir = "/home/judge";
        auto create_result = co_await DockerTaskAwaitable{
            6000,
            [&compile_sandbox, compile_config] {
                DockerTaskResult result;
                result.stdout_str = compile_sandbox.Create(compile_config);
                result.status = result.stdout_str.empty() ? "SYSTEM_ERROR" : "OK";
                return result;
            }};
        std::string compile_container_id = std::move(create_result.stdout_str);
        LOG_DEBUG("Compile container created id={}", compile_container_id);
        if (compile_container_id.empty()) {
            LOG_ERROR("Failed to create compile container");
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }
        auto start_result = co_await DockerTaskAwaitable{
            6000,
            [&compile_sandbox] {
                const int rc = compile_sandbox.Start();
                return DockerTaskResult{rc == 0 ? "OK" : "SYSTEM_ERROR", rc};
            }};
        if (start_result.status != "OK") {
            LOG_ERROR("Failed to start compile container: {}", compile_sandbox.GetLastError());
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }
        LOG_DEBUG("Compile container started");

        // ③ 写入源代码到编译容器
        auto write_source_result = co_await DockerTaskAwaitable{
            6000,
            [&compile_sandbox, code = request.code()] {
                const int rc = compile_sandbox.WriteFile("/home/judge/source.cpp", code);
                return DockerTaskResult{rc == 0 ? "OK" : "SYSTEM_ERROR", rc};
            }};
        if (write_source_result.status != "OK") {
            LOG_ERROR("Failed to write compile input: {}", compile_sandbox.GetLastError());
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }

        // ④ 宿主执行并等待 Docker exec；容器没有网络和回调能力。
        DockerTaskAwaitable compile_awaitable;
        compile_awaitable.timeout_ms = 10000;
        compile_awaitable.work = [&compile_sandbox, standard = LanguageStandard(request.language())] {
            return ToDockerTaskResult(compile_sandbox.Exec(
                "CXX_STANDARD=" + standard + " /judge/compile.sh", 10000), true);
        };

        auto compile_result = co_await compile_awaitable;
        LOG_DEBUG("Compile task resumed status={}", compile_result.status);

        // ⑤ 协程恢复，检查编译结果
        if (compile_result.status != "OK") {
            // 编译失败，直接返回 CE
            auto resp = std::make_shared<oj::common::JudgeResult>();
            resp->set_status(oj::common::SUBMISSION_STATUS_COMPILE_ERROR);
            // 读取编译错误信息
            auto read_error_result = co_await DockerTaskAwaitable{
                6000,
                [&compile_sandbox] {
                    DockerTaskResult result;
                    result.stdout_str = compile_sandbox.ReadFile("/home/judge/compile_err");
                    result.status = "OK";
                    return result;
                }};
            resp->set_compile_error(read_error_result.stdout_str);
            resp->set_completed_at(std::time(nullptr));
            co_await DockerTaskAwaitable{
                4000,
                [&compile_sandbox] {
                    compile_sandbox.Destroy();
                    return DockerTaskResult{"OK", 0};
                }};
            co_return resp;
        }

        // ⑥ 从编译容器拷贝可执行文件到宿主机
        auto read_executable_result = co_await DockerTaskAwaitable{
            6000,
            [&compile_sandbox] {
                DockerTaskResult result;
                result.stdout_str = compile_sandbox.ReadFile("/home/judge/main");
                result.status = result.stdout_str.empty() ? "SYSTEM_ERROR" : "OK";
                return result;
            }};
        if (read_executable_result.status != "OK") {
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }
        std::string exe_content = std::move(read_executable_result.stdout_str);
        _file_system.write(exe_path, exe_content);
        co_await DockerTaskAwaitable{
            4000,
            [&compile_sandbox] {
                compile_sandbox.Destroy();
                return DockerTaskResult{"OK", 0};
            }};

        // ⑦ 从预热池获取运行容器
        std::unique_ptr<oj_sandbox::DockerSandbox> runner_sandbox;
        auto acquire_result = co_await DockerTaskAwaitable{
            12000,
            [this, &runner_sandbox] {
                runner_sandbox = _runner_pool ? _runner_pool->Acquire() : nullptr;
                const bool ok = runner_sandbox && !runner_sandbox->GetContainerId().empty();
                return DockerTaskResult{ok ? "OK" : "SYSTEM_ERROR", ok ? 0 : -1};
            }};
        if (acquire_result.status != "OK") {
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }

        // ⑧ 拷贝可执行文件到运行容器
        auto write_executable_result = co_await DockerTaskAwaitable{
            6000,
            [&runner_sandbox, exe_content] {
                const int rc = runner_sandbox->WriteFile("/home/judge/main", exe_content);
                return DockerTaskResult{rc == 0 ? "OK" : "SYSTEM_ERROR", rc};
            }};
        if (write_executable_result.status != "OK") {
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }
        // 设置可执行权限
        auto chmod_result = co_await DockerTaskAwaitable{
            6000,
            [&runner_sandbox] {
                return ToDockerTaskResult(
                    runner_sandbox->Exec("chmod +x /home/judge/main", 5000), false);
            }};
        if (chmod_result.status != "OK") {
            co_return ErrorResult(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
        }

        // ⑨ 逐个测试用例运行
        auto resp = std::make_shared<oj::common::JudgeResult>();
        resp->set_status(_test_cases.empty() ? oj::common::SUBMISSION_STATUS_SYSTEM_ERROR
                                             : oj::common::SUBMISSION_STATUS_ACCEPTED);

        for (size_t i = 0; i < _test_cases.size(); ++i) {
            const auto& test = _test_cases[i];

            // 写入输入文件到运行容器
            auto write_input_result = co_await DockerTaskAwaitable{
                6000,
                [&runner_sandbox, input = test.input] {
                    const int rc = runner_sandbox->WriteFile("/home/judge/input.txt", input);
                    return DockerTaskResult{rc == 0 ? "OK" : "SYSTEM_ERROR", rc};
                }};
            if (write_input_result.status != "OK") {
                resp->set_status(oj::common::SUBMISSION_STATUS_SYSTEM_ERROR);
                break;
            }

            // 宿主等待 exec 完成；超时由 DockerSandbox kill 一次性容器。
            DockerTaskAwaitable run_awaitable;
            run_awaitable.timeout_ms = request.time_limit_ms() + 1000;
            run_awaitable.work = [&runner_sandbox, &test, timeout = request.time_limit_ms()] {
                const auto started = std::chrono::steady_clock::now();
                std::string cmd = "TIME_LIMIT=" + std::to_string(std::max(1, test.cpu_limit)) +
                                  " MEM_LIMIT=" + std::to_string(std::max(1, test.mem_limit) * 1024) +
                                  " OUTPUT_LIMIT_KB=1024 /judge/run.sh";
                auto result = ToDockerTaskResult(runner_sandbox->Exec(cmd, timeout + 1000), false);
                result.time_used_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - started).count();
                if (result.exit_code == 137 && runner_sandbox->WasOomKilled())
                    result.status = "MEMORY_LIMIT";
                result.memory_used_bytes = runner_sandbox->MemoryPeakBytes();
                return result;
            };

            auto run_result = co_await run_awaitable;
            LOG_DEBUG("Runner completed exit_code={} status={} timed_out={} stderr={}",
                      run_result.exit_code, run_result.status, run_result.timed_out,
                      run_result.stderr_str);

            // ⑩ 协程恢复，解析运行结果
            auto* test_result = resp->add_test_case_results();
            test_result->set_index(test.index);
            test_result->set_input(test.input);
            test_result->set_expected_output(test.output);
            test_result->set_time_used_ms(run_result.time_used_ms);
            test_result->set_memory_used_bytes(run_result.memory_used_bytes);
            oj::common::SubmissionStatus test_status = oj::common::SUBMISSION_STATUS_ACCEPTED;

            if (run_result.timed_out) {
                test_status = oj::common::SUBMISSION_STATUS_TIME_LIMIT_EXCEEDED;
                test_result->set_time_used_ms(std::max<int64_t>(request.time_limit_ms(), run_result.time_used_ms));
            } else if (run_result.status != "OK") {
                // 运行时错误
                test_status = MapStatusToSubmissionStatus(run_result.status);
            } else {
                // 运行成功，比较输出
                auto read_output_result = co_await DockerTaskAwaitable{
                    6000,
                    [&runner_sandbox] {
                        DockerTaskResult result;
                        result.stdout_str = runner_sandbox->ReadFile("/home/judge/output.txt");
                        result.status = "OK";
                        return result;
                    }};
                std::string user_output = std::move(read_output_result.stdout_str);
                LOG_DEBUG("User program completed");
                test_result->set_actual_output(user_output);
                std::string expected = test.output;
                if ((!request.has_custom_task_id() || IsGuestSubmission(request)) &&
                    NormalizeOutput(user_output) != NormalizeOutput(expected)) {
                    test_status = oj::common::SUBMISSION_STATUS_WRONG_ANSWER;
                }
            }
            test_result->set_status(test_status);
            resp->set_time_used_ms(resp->time_used_ms() + test_result->time_used_ms());
            resp->set_memory_used_bytes(std::max(resp->memory_used_bytes(),
                                                 test_result->memory_used_bytes()));
            if (resp->status() == oj::common::SUBMISSION_STATUS_ACCEPTED &&
                test_status != oj::common::SUBMISSION_STATUS_ACCEPTED) {
                resp->set_status(test_status);
            }
        }

        // ⑪ 归还运行容器到预热池
        co_await DockerTaskAwaitable{
            4000,
            [&runner_sandbox] {
                runner_sandbox->Destroy();
                return DockerTaskResult{"OK", 0};
            }};
        runner_sandbox.reset();

        resp->set_completed_at(std::time(nullptr));

        co_return resp;
    }

    /// 设置测试用例
    void SetTestCases(const std::vector<TestCase>& tests) 
    {
        _test_cases = tests;
    }

    void SetTask(CoTask& task)
    {
        (void)task;
    }

    /// 设置运行容器预热池
    void SetSandboxPool(oj_sandbox::SandboxPool* pool)
    {
        _runner_pool = pool;
    }

private:
    fileUtil::FileSystem _file_system;
    oj_sandbox::SandboxPool* _runner_pool;
    latecyMonitor::LatencyMonitor* _latency_monitor;
    std::vector<TestCase> _test_cases;

    static std::shared_ptr<oj::common::JudgeResult> ErrorResult(
        oj::common::SubmissionStatus status) {
        auto result = std::make_shared<oj::common::JudgeResult>();
        result->set_status(status);
        result->set_completed_at(std::time(nullptr));
        return result;
    }

    static std::string NormalizeOutput(const std::string& output) {
        std::string normalized;
        size_t line_start = 0;
        while (line_start < output.size()) {
            size_t line_end = output.find('\n', line_start);
            if (line_end == std::string::npos) line_end = output.size();
            size_t content_end = line_end;
            while (content_end > line_start &&
                   (output[content_end - 1] == ' ' || output[content_end - 1] == '\t' ||
                    output[content_end - 1] == '\r')) --content_end;
            normalized.append(output, line_start, content_end - line_start);
            normalized.push_back('\n');
            line_start = line_end + (line_end < output.size());
        }
        while (!normalized.empty() && normalized.back() == '\n') normalized.pop_back();
        return normalized;
    }

    static std::string LanguageStandard(const std::string& language) {
        return language == "cpp20" ? "c++20" : "c++17";
    }

    static DockerTaskResult ToDockerTaskResult(const oj_sandbox::ExecResult& exec, bool compile) {
        DockerTaskResult result;
        result.exit_code = exec.exit_code;
        result.timed_out = exec.timed_out;
        result.killed = exec.killed;
        result.stdout_str = exec.stdout_str;
        result.stderr_str = exec.stderr_str;
        if (exec.timed_out) result.status = "TIME_LIMIT";
        else if (exec.exit_code == 0) result.status = "OK";
        else if (compile) result.status = "COMPILE_ERROR";
        else if (exec.exit_code == 137) result.status = "RUNTIME_ERROR";
        else if (exec.exit_code == 152) result.status = "TIME_LIMIT";
        else result.status = "RUNTIME_ERROR";
        return result;
    }

    /// Docker 脚本状态映射到 SubmitResult 枚举
    static oj::common::SubmissionStatus MapStatusToSubmissionStatus(const std::string& status) {
        if (status == "MEMORY_LIMIT") return oj::common::SUBMISSION_STATUS_MEMORY_LIMIT_EXCEEDED;
        if (status == "TIME_LIMIT") return oj::common::SUBMISSION_STATUS_TIME_LIMIT_EXCEEDED;
        return oj::common::SUBMISSION_STATUS_RUNTIME_ERROR;
    }
};

} // namespace oj_judge
