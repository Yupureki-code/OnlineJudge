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
            int memory_limit = value["mem_limit"].asInt();
            
            logger(ns_log::DEBUG)<<"设置资源限制 - CPU: "<<cpu_limit<<"秒, 内存: "<<memory_limit<<"字节";
            
            struct rlimit r;
            // 设置CPU时间限制
            r.rlim_cur = cpu_limit;
            r.rlim_max = RLIM_INFINITY;
            if(setrlimit(RLIMIT_CPU, &r) < 0) {
                logger(ns_log::ERROR)<<"设置CPU限制失败";
            }
            
            // 设置内存限制 - 使用RLIMIT_AS限制地址空间
            r.rlim_cur = memory_limit;
            r.rlim_max = memory_limit;
            if(setrlimit(RLIMIT_AS, &r) < 0) {
                logger(ns_log::ERROR)<<"设置内存限制失败";
            }
        }
    public:
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            if(_is_enable)
            {   
                Json::Value in_value;
                Json::Reader reader;
                reader.parse(in_json,in_value);
                std::string id = in_value["id"].asString();
                std::string _run_file = PathUtil::Exe(file_name);
                std::string _stdin = PathUtil::Stdin(default_questions_path + id + "/in");
                logger(ns_log::DEBUG)<<"stdin path : "<<_stdin;
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
                    SetProcLimit(in_json);
                    dup2(_stdin_fd,0);
                    dup2(_stdout_fd,1);
                    dup2(_stderr_fd,2);
                    execl(_run_file.c_str(), _run_file.c_str(),nullptr);
                    _exit(1);
                }
                else 
                {
                    close(_stdin_fd);
                    close(_stdout_fd);
                    close(_stderr_fd);
                    int status = 0;
                    waitpid(pid,&status,0);
                    
                    // 检查进程是否正常退出
                    if (WIFEXITED(status)) {
                        int exit_code = WEXITSTATUS(status);
                        logger(ns_log::INFO)<<"运行完毕,ExitCode: "<<exit_code;
                        if(exit_code != 0)
                            return HandlerProgramEnd(ExitCodeToSatusCode(exit_code), file_name);
                    } else if (WIFSIGNALED(status)) {
                        // 进程被信号终止
                        int signal = WTERMSIG(status);
                        logger(ns_log::INFO)<<"运行被信号终止,Signal: "<<signal;
                        return HandlerProgramEnd(ExitCodeToSatusCode(signal), file_name);
                    } else {
                        // 其他情况
                        logger(ns_log::INFO)<<"运行状态未知,Status: "<<status;
                        return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
                    }
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