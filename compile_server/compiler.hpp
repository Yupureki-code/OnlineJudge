#pragma once

#include <cstdio>
#include <jsoncpp/json/value.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../comm/comm.hpp"
#include "../comm/logstrategy.hpp"
#include "COP_hanlder.hpp"

namespace ns_compile
{
    using namespace ns_hanlder;
    using namespace ns_util;
    using namespace ns_log;

    class HandlerCompiler : public HandlerProgram
    {
    public:
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            if(_is_enable)
            {
                pid_t pid = fork();
                if(pid == 0)
                {
                    int _stderr = open(PathUtil::Compile_err(file_name).c_str(),O_CREAT | O_WRONLY | O_TRUNC,0x644);
                    if(_stderr < 0)
                    {
                        logger(ns_log::FATAL)<<"打开编译错误文件失败: "<<PathUtil::Compile_err(file_name);
                        return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
                    }
                    dup2(_stderr,2);
                    // 直接写入编译命令到错误文件
                    fprintf(stderr, "编译命令: g++ %s -o %s -std=c++11\n", PathUtil::Src(file_name).c_str(), PathUtil::Exe(file_name).c_str());
                    execlp("g++","g++", PathUtil::Src(file_name).c_str(),"-o",PathUtil::Exe(file_name).c_str(),"-std=c++11",nullptr);
                    logger(ns_log::FATAL)<<"子进程启动编译失败";
                    return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
                }
                else 
                {
                    waitpid(pid,nullptr,0);
                    if(FileUtil::IsFileExist(PathUtil::Exe(file_name)))
                    {
                        logger(ns_log::INFO)<<"编译成功";
                    }
                    else 
                    {
                        logger(ns_log::INFO)<<"编译失败";
                        return HandlerProgramEnd(ns_hanlder::COMPILE_ERROR, file_name);
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
                ns_log::logger(ns_log::INFO)<<"责任链错误结束";
                return HandlerProgramEnd(ns_hanlder::UNKNOWN, file_name);
            }
        }
    };
};