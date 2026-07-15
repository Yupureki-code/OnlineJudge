#pragma once
#include "../../comm/comm.hpp"
#include "../../comm/logger.hpp"
#include "../../comm/filesystem.hpp"
#include <memory>
#include "event_loop.hpp"
#include "docker_sandbox.hpp"

namespace oj_judge 
{

/// 判题链入口：组装 preprocesser → compiler → runner → judge
struct TestCase 
{
    std::string input;
    std::string output;
    int cpu_limit;    // 秒
    int mem_limit;    // MB
};

class Judger
{
public:
    Judger() : _runner_pool(nullptr), _handle(nullptr) {}
    explicit Judger(oj_sandbox::SandboxPool* pool) : _runner_pool(pool), _handle(nullptr) {}

    /// 运行判题链
    /// @param request 判题请求（包含 code, question_id, time_limit 等）
    /// @return 判题结果
    CoTask Run(const oj_judge::SubmitRequest& request) 
    {
        LOG_INFO("Judger started submission_id={}", request.submission_id());
        // ① 生成唯一文件名，写入源代码到宿主机临时目录
        std::string file_name = oj_util::StringUtil::GetUniqueName();
        std::string src_path = oj_util::PathUtil::Src(file_name);
        std::string exe_path = oj_util::PathUtil::Exe(file_name);
        std::string err_path = oj_util::PathUtil::Compile_err(file_name);
        _file_system.write(src_path, request.code());
        LOG_DEBUG("Submission source staged");

        // ② 创建编译容器（每次新建，编译完销毁）
        oj_sandbox::DockerSandbox compile_sandbox;
        oj_sandbox::SandboxConfig compile_config;
        compile_config.image = "oj-compiler:latest";
        compile_config.network_disabled = false;  // 编译容器需要网络通知 JudgeService
        compile_config.read_only_root = false;
        compile_config.work_dir = "/home/judge";
        std::string compile_container_id = compile_sandbox.Create(compile_config);
        LOG_DEBUG("Compile container created id={}", compile_container_id);
        if (compile_container_id.empty()) {
            LOG_ERROR("Failed to create compile container");
            auto resp = std::make_shared<JudgeFinishedRequest>();
            resp->set_submission_id(request.submission_id());
            resp->set_question_id(request.question_id());
            resp->set_user_id(request.user_id());
            auto* status = resp->add_status_list();
            status->set_result(UNKNOWN);
            co_return resp;
        }
        compile_sandbox.Start();
        LOG_DEBUG("Compile container started");

        // ③ 写入源代码到编译容器
        compile_sandbox.WriteFile("/home/judge/source.cpp", request.code());

        // ④ 启动编译容器脚本，协程挂起等待容器 curl 通知
        DockerTaskAwaitable compile_awaitable;
        compile_awaitable.container_id = compile_container_id;
        compile_awaitable.script = "/judge/compile.sh";
        compile_awaitable.timeout_ms = 10000;  // 编译超时 10 秒
        compile_awaitable.home = _handle.promise().home;  // 返回 Work 协程
        compile_awaitable.task_id = _handle.promise().task_id;  // Docker 回调标识
        *_handle.promise().status = Blocking;  // 标记为等待 Docker
        compile_awaitable.start_container = [this, &compile_sandbox](
            const std::string& cid, const std::string& script, const std::string& task_id) {
            std::string judge_host = std::getenv("OJ_JUDGE_HOST") ? std::getenv("OJ_JUDGE_HOST") : "host.docker.internal:8082";
            std::string cmd = "export TASK_ID=" + task_id + 
                             " && export JUDGE_HOST=" + judge_host + 
                             " && " + script;
            LOG_DEBUG("Starting detached compile task");
            compile_sandbox.ExecDetached(cmd);  // non-blocking — awaits DockerWorkDone callback
            LOG_DEBUG("Detached compile task started");
        };
        LOG_DEBUG("Waiting for compile task");

        co_await compile_awaitable;
        LOG_DEBUG("Compile task resumed status={}", _handle.promise().result.status);

        // ⑤ 协程恢复，检查编译结果
        auto& compile_result = _handle.promise().result;
        if (compile_result.status != "OK") {
            // 编译失败，直接返回 CE
            auto resp = std::make_shared<JudgeFinishedRequest>();
            resp->set_submission_id(request.submission_id());
            resp->set_question_id(request.question_id());
            resp->set_user_id(request.user_id());
            auto* status = resp->add_status_list();
            status->set_result(COMPILE_ERROR);
            status->set_time_cost(0);
            status->set_memory_cost(0);
            // 读取编译错误信息
            std::string compile_err = compile_sandbox.ReadFile("/home/judge/compile_err");
            compile_sandbox.Destroy();
            co_return resp;
        }

        // ⑥ 从编译容器拷贝可执行文件到宿主机
        std::string exe_content = compile_sandbox.ReadFile("/home/judge/main");
        _file_system.write(exe_path, exe_content);
        compile_sandbox.Destroy();  // 编译容器销毁

        // ⑦ 从预热池获取运行容器
        auto runner_sandbox = _runner_pool->Acquire();
        if (!runner_sandbox || runner_sandbox->GetContainerId().empty()) {
            auto resp = std::make_shared<JudgeFinishedRequest>();
            resp->set_submission_id(request.submission_id());
            resp->set_question_id(request.question_id());
            resp->set_user_id(request.user_id());
            auto* status = resp->add_status_list();
            status->set_result(UNKNOWN);
            co_return resp;
        }

        // ⑧ 拷贝可执行文件到运行容器
        runner_sandbox->WriteFile("/home/judge/main", exe_content);
        // 设置可执行权限
        runner_sandbox->Exec("chmod +x /home/judge/main", 5000);

        // ⑨ 逐个测试用例运行
        auto resp = std::make_shared<JudgeFinishedRequest>();
        resp->set_submission_id(request.submission_id());
        resp->set_question_id(request.question_id());
        resp->set_user_id(request.user_id());

        for (size_t i = 0; i < _test_cases.size(); ++i) {
            const auto& test = _test_cases[i];

            // 写入输入文件到运行容器
            runner_sandbox->WriteFile("/home/judge/input.txt", test.input);

            // 启动运行容器脚本，协程挂起等待容器 curl 通知
            DockerTaskAwaitable run_awaitable;
            run_awaitable.container_id = runner_sandbox->GetContainerId();
            run_awaitable.script = "/judge/run.sh";
            run_awaitable.timeout_ms = request.time_limit() + 1000;
            run_awaitable.home = _handle.promise().home;  // 返回 Work 协程
            run_awaitable.task_id = _handle.promise().task_id;  // Docker 回调标识
            *_handle.promise().status = Blocking;  // 标记为等待 Docker
            run_awaitable.start_container = [this, &runner_sandbox, &test, &request](
                const std::string& cid, const std::string& script, const std::string& task_id) {
                std::string judge_host = std::getenv("OJ_JUDGE_HOST") ? std::getenv("OJ_JUDGE_HOST") : "host.docker.internal:8082";
                std::string cmd = "export TASK_ID=" + task_id +
                                 " && export JUDGE_HOST=" + judge_host +
                                 " && export TIME_LIMIT=" + std::to_string(test.cpu_limit) +
                                 " && export MEM_LIMIT=" + std::to_string(test.mem_limit * 1024) +
                                 " && " + script;
                runner_sandbox->Exec(cmd, request.time_limit() + 5000);
            };

            co_await run_awaitable;

            // ⑩ 协程恢复，解析运行结果
            auto& run_result = _handle.promise().result;
            auto* status = resp->add_status_list();

            if (run_result.timed_out) {
                status->set_result(TIME_LIMIT);
                status->set_time_cost(request.time_limit());
                status->set_memory_cost(0);
            } else if (run_result.status != "OK") {
                // 运行时错误
                SubmitResult result = MapStatusToSubmitResult(run_result.status);
                status->set_result(result);
                status->set_time_cost(0);
                status->set_memory_cost(0);
            } else {
                // 运行成功，比较输出
                std::string user_output = runner_sandbox->ReadFile("/home/judge/output.txt");
                LOG_DEBUG("User program completed");
                std::string expected = test.output;
                if (RemoveAllWhitespace(user_output) == RemoveAllWhitespace(expected)) {
                    status->set_result(AC);
                } else {
                    status->set_result(WA);
                }
                status->set_time_cost(0);
                status->set_memory_cost(0);
            }
        }

        // ⑪ 归还运行容器到预热池
        _runner_pool->Release(std::move(runner_sandbox));

        // ⑫ 清理宿主机临时文件
        _file_system.remove(src_path);
        _file_system.remove(exe_path);

        co_return resp;
    }

    /// 设置测试用例
    void SetTestCases(const std::vector<TestCase>& tests) 
    {
        _test_cases = tests;
    }

    void SetTask(CoTask& task)
    {
        _handle = task.GetHandle();
    }

    /// 设置运行容器预热池
    void SetSandboxPool(oj_sandbox::SandboxPool* pool)
    {
        _runner_pool = pool;
    }

private:
    fileUtil::FileSystem _file_system;
    oj_sandbox::SandboxPool* _runner_pool;
    CoTask::handle_type _handle;
    std::vector<TestCase> _test_cases;

    /// 去除所有空白字符
    static std::string RemoveAllWhitespace(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                result += c;
            }
        }
        return result;
    }

    /// Docker 脚本状态映射到 SubmitResult 枚举
    static SubmitResult MapStatusToSubmitResult(const std::string& status) {
        if (status == "MEMORY_LIMIT") return MEMORY_LIMIT;
        if (status == "SEGV")         return SEGV_ERROR;
        if (status == "FPE")          return FPE_ERROR;
        if (status == "TIME_LIMIT")   return TIME_LIMIT;
        if (status == "COMPILE_ERROR") return COMPILE_ERROR;
        return RUNTIME_ERROR;
    }
};

} // namespace oj_judge
