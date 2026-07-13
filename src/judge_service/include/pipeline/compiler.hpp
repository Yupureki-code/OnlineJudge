#pragma once
#include "handler_base.hpp"
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include "../async/task.hpp"
#include "../async//docker_task_awaitable.hpp"

namespace oj_judge {

/// 编译器：fork + g++ 编译用户代码
///
/// 编译成功 → 调用下一个处理器（runner）
/// 编译失败 → 直接返回 COMPILE_ERROR
class HandlerCompiler : public HandlerProgram 
{
private:
    ns_async::Task<void> RunInDocker(const std::string& file_name)
    {
        std::string src_path = TmpPath(file_name, ".cpp");
        std::string exe_path = TmpPath(file_name, ".exe");
        std::string err_path = TmpPath(file_name, ".compile_err");

        pid_t pid = fork();
        if (pid == 0) {
            // 子进程：重定向 stderr 到编译错误文件
            int fd = open(err_path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                _exit(1);
            }
            dup2(fd, 2);
            close(fd);
            execlp("g++", "g++", src_path.c_str(), "-o", exe_path.c_str(),
                   "-std=c++11", "-O2", "-w", nullptr);
            _exit(1);
        }

        // 父进程：等待编译完成
        int status = 0;
        co_await LoopAwaitable();
        waitpid(pid, &status, 0);
        struct stat st;
        if (stat(exe_path.c_str(), &st) != 0) 
        {
            // 编译失败，读取错误信息
            std::string compile_err;
            ReadFile(err_path, compile_err);
            //return HandlerEndWithCompileError(file_name, compile_err);
        }
    }
public:
    std::string Execute(const std::string& file_name, const oj_judge::SubmitRequest& request,std::coroutine_handle<> h) override 
    {
        if (!_is_enable) 
        {
            return _next ? _next->Execute(file_name, request,h) : HandlerEnd({UNKNOWN}, file_name);
        }
        auto ret = RunInDocker(file_name);
        // 编译成功，调用下一个处理器
        if (_next) {
            return _next->Execute(file_name, request,h);
        }
        return HandlerEnd({UNKNOWN}, file_name);
    }

private:
    static constexpr const char* TMP_DIR = "../tmp/";

    std::string TmpPath(const std::string& name, const std::string& suffix) const {
        return std::string(TMP_DIR) + name + suffix;
    }

    bool ReadFile(const std::string& path, std::string& content) const {
        std::ifstream in(path);
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            content += line + "\n";
        }
        return true;
    }

    std::string HandlerEnd(const std::vector<Status>& result,
                           const std::string& file_name) {
        Json::Value out;
        out["status"] = StatusToString(result.empty() ? UNKNOWN : result[0]);
        out["desc"] = StatusToDesc(result.empty() ? UNKNOWN : result[0]);
        Json::StyledWriter writer;
        return writer.write(out);
    }

    std::string HandlerEndWithCompileError(const std::string& file_name,
                                            const std::string& compile_err) {
        Json::Value out;
        out["status"] = StatusToString(COMPILE_ERROR);
        out["desc"] = StatusToDesc(COMPILE_ERROR);
        out["compile_error"] = compile_err;
        Json::StyledWriter writer;
        return writer.write(out);
    }
};

} // namespace oj_judge