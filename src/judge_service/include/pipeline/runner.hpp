#pragma once
#include "handler_base.hpp"
#include "../sandbox/docker_sandbox.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace oj_judge {

/// 测试用例数据
struct TestCase {
    std::string input;
    std::string output;
    int cpu_limit;    // 秒
    int mem_limit;    // MB
};

/// 运行器：在 Docker 沙箱中运行编译后的程序
///
/// 两种模式：
/// 1. Docker 沙箱模式（推荐）：每次判题创建隔离容器
/// 2. fork/exec 模式（兼容）：直接 fork 运行，setrlimit 限制资源
class HandlerRunner : public HandlerProgram {
public:
    HandlerRunner() :  _sandbox_pool(nullptr) {}

    /// 设置测试用例（由外部从数据库查询后注入）
    void SetTestCases(const std::vector<TestCase>& tests) {
        _tests = tests;
    }

    /// 从数据库查询测试用例（替代外部注入）
    /// 注意：需要链接 ODB 库
    bool LoadTestsFromDB(const std::string& question_id) {
        // DB 查询功能在 Phase 2 集成 ODB 后启用
        // 当前回退到依赖 SetTestCases() 外部注入
        return false;
    }

    /// 清理临时文件
    static void CleanupTempFiles(const std::string& file_name, int test_count) {
        std::string tmp_dir = "../tmp/";
        auto unlink_file = [&](const std::string& path) {
            ::unlink((tmp_dir + path).c_str());
        };
        unlink_file(file_name + ".cpp");
        unlink_file(file_name + ".exe");
        unlink_file(file_name + ".compile_err");
        for (int i = 1; i <= test_count; ++i) {
            std::string tf = file_name + "_" + std::to_string(i);
            unlink_file(tf + ".stdin");
            unlink_file(tf + ".stdout");
            unlink_file(tf + ".stderr");
            unlink_file(tf + ".ans");
        }
    }

    std::string Execute(const std::string& file_name, const oj_judge::SubmitRequest& request,std::coroutine_handle<> h) override {
        if (!_is_enable) {
            return _next ? _next->Execute(file_name, request,h) : HandlerEnd({UNKNOWN}, file_name);
        }
        bool is_custom_test = request.is_custom_test();

        return RunInDocker(file_name, request, is_custom_test);
    }

private:
    oj_sandbox::SandboxPool* _sandbox_pool;
    std::vector<TestCase> _tests;

    static constexpr const char* TMP_DIR = "../tmp/";

    std::string TmpPath(const std::string& name, const std::string& suffix) const {
        return std::string(TMP_DIR) + name + suffix;
    }

    // ==================== Docker 沙箱模式 ====================

    std::string RunInDocker(const std::string& file_name,
                             const oj_judge::SubmitRequest& request ,
                             bool is_custom_test) {
        auto sandbox = _sandbox_pool->Acquire();
        if (!sandbox || sandbox->GetContainerId().empty()) {
            return HandlerEnd({UNKNOWN}, file_name);
        }

        std::string exe_path = TmpPath(file_name, ".exe");

        // 逐个测试用例运行
        std::vector<Status> results;
        for (size_t i = 0; i < _tests.size(); ++i) {
            const auto& test = _tests[i];

            // 写入输入文件到容器
            sandbox->WriteFile("/home/judge/input.txt", test.input);

            // 执行程序
            oj_sandbox::SandboxConfig config;
            config.cpu_limit_ms = test.cpu_limit * 1000;
            config.memory_limit_mb = test.mem_limit;
            config.timeout_ms = test.cpu_limit * 1000 + 1000;

            auto result = sandbox->Exec(
                "./main < input.txt > output.txt 2> stderr.txt",
                config.timeout_ms);

            // 判断结果
            if (result.timed_out) {
                results.push_back(TIME_LIMIT);
            } else if (result.exit_code != 0) {
                results.push_back(ExitCodeToStatusCode(result.exit_code));
            } else {
                // 读取输出并比较
                std::string user_output = sandbox->ReadFile("/home/judge/output.txt");
                std::string expected = test.output;
                if (RemoveAllWhitespace(user_output) == RemoveAllWhitespace(expected)) {
                    results.push_back(AC);
                } else {
                    results.push_back(WA);
                }
            }
        }

        _sandbox_pool->Release(std::move(sandbox));

        // if (_next) {
        //     return _next->Execute(file_name, request,h);
        // }
        return HandlerEnd(results, file_name);
    }

    // ==================== 辅助函数 ====================

    void SetProcLimit(int cpu_limit, int mem_limit) {
        struct rlimit r;
        r.rlim_cur = cpu_limit;
        r.rlim_max = RLIM_INFINITY;
        setrlimit(RLIMIT_CPU, &r);

        r.rlim_cur = mem_limit * 1024 * 1024;
        r.rlim_max = mem_limit * 1024 * 1024;
        setrlimit(RLIMIT_AS, &r);
    }

    bool WriteFile(const std::string& path, const std::string& content) {
        std::ofstream out(path);
        if (!out.is_open()) return false;
        out << content;
        return true;
    }

    std::string RemoveAllWhitespace(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
                result += c;
            }
        }
        return result;
    }

    std::string HandlerEnd(const std::vector<Status>& result,
                           const std::string& file_name) {
        Json::Value out;
        std::string status_str;
        for (auto& s : result) {
            status_str += StatusToString(s) + " ";
        }
        out["status"] = status_str;
        out["desc"] = StatusToDesc(result.empty() ? UNKNOWN : result[0]);
        Json::StyledWriter writer;
        return writer.write(out);
    }
};

} // namespace oj_judge