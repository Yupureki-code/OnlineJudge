#pragma once

#include <cstdio>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include "../comm/comm.hpp"
#include "../comm/logstrategy.hpp"
#include "COP_hanlder.hpp"

namespace ns_runner
{
    using namespace ns_hanlder;
    using namespace ns_log;
    using namespace ns_util;

    class HandlerRunner : public HandlerProgram
    {
    private:
        void SetProcLimit(const std::string& in_json)
        {
            Json::Value value;
            Json::Reader reader;
            reader.parse(in_json,value);
            int cpu_limit = value["cpu_limit"].asInt();
            int memory_limit = value["memory_limit"].asInt();
            struct rlimit r;
            r.rlim_cur = cpu_limit;
            r.rlim_max = RLIM_INFINITY;
            setrlimit(RLIMIT_CPU, &r);
            r.rlim_cur = memory_limit;
            setrlimit(RLIMIT_AS, &r);
        }
    public:
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            if(_is_enable)
            {
                std::string _run_file = PathUtil::Exe(file_name);
                std::string _stdin = PathUtil::Stdin(file_name);
                std::string _stdout = PathUtil::Stdout(file_name);
                std::string _stderr = PathUtil::Stderr(file_name);
                umask(0);
                int _stdin_fd  = open(_stdin.c_str(),O_CREAT | O_RDONLY,0644);
                int _stdout_fd = open(_stdout.c_str(),O_CREAT | O_WRONLY,0644);
                int _stderr_fd = open(_stderr.c_str(),O_CREAT | O_WRONLY,0644);
                if(_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0)
                {
                    logger(ns_log::FATAL)<<"打开_stdin _stdout _stderr文件失败";
                    return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
                }
                pid_t pid = fork();
                if(pid < 0)
                {
                    logger(ns_log::FATAL)<<"创建子进程失败";
                    return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
                }
                else if(pid == 0)
                {
                    dup2(_stdin_fd,0);
                    dup2(_stdout_fd,1);
                    dup2(_stderr_fd,2);
                    SetProcLimit(in_json);
                    execl(_run_file.c_str(), _run_file.c_str(),nullptr);
                    logger(ns_log::FATAL)<<"子进程运行 "<<_run_file<<" 失败";
                    return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
                }
                else 
                {
                    close(_stdin_fd);
                    close(_stdout_fd);
                    close(_stderr_fd);
                    int status = 0;
                    waitpid(pid,&status,0);
                    logger(ns_log::INFO)<<"运行完毕,ExitCode: "<<(status & 0x7F);
                    // 根据退出状态码返回相应的错误信息
                    if(status != 0)
                        return HandlerProgramEnd(ExitCodeToSatusCode(status & 0x7F), file_name);
                }
            }
            if(_next)
            {
                return _next->Execute(file_name,in_json);
            }
            else 
            {
                //责任链不应该在这结束
                ns_log::logger(ns_log::INFO)<<"责任链结束";
                return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
            }
        }
    };
};