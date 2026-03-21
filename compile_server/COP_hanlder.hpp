#pragma once

#include "../comm/comm.hpp"
#include <jsoncpp/json/value.h>
#include <memory>
#include "../comm/logstrategy.hpp"
#include <string>
#include <jsoncpp/json/json.h>

namespace ns_hanlder
{
    using namespace ns_util;
    using namespace ns_log;

    enum Status
    {
        AC = 0,
        WA,
        RUNTIME_ERROR,
        MEMORY_LIMIT,
        COMPILE_ERROR,
        FPE_ERROR,
        SEGV_ERROR,
        UNKNOWN
    };

    class HandlerProgram
    {
    public:
        virtual std::string Execute(const std::string&,const std::string&) = 0;
        void Set_next(std::shared_ptr<HandlerProgram>& next)
        {
            _next = next;
        }
        void Enable()
        {
            _is_enable = true;
        }
        void Unable()
        {
            _is_enable = false;
        }
        enum Status ExitCodeToSatusCode(int code)
        {
            switch (code) 
            {
                case 0:
                    return AC;
                case -1:
                    return WA;
                case 8: // SIGFPE
                case 136: // exit code for SIGFPE
                    return FPE_ERROR;
                case 11: // SIGSEGV
                case 139: // exit code for SIGSEGV
                    return SEGV_ERROR;
                case 6: // SIGABRT
                case 134: // exit code for SIGABRT
                    return MEMORY_LIMIT;
                case 24: // SIGXCPU
                case 152: // exit code for SIGXCPU
                    return RUNTIME_ERROR;
                default:
                    return UNKNOWN;
            }
        }
        std::string StatusToDesc(enum Status status)
        {
            switch(status)
            {
                case AC:
                    return "恭喜你!已通过此题";
                case WA:
                    return "部分解答错误";
                case RUNTIME_ERROR:
                    return "运行时间超时";
                case MEMORY_LIMIT:
                    return "内存申请过多";
                case COMPILE_ERROR:
                    return "编译错误";
                case FPE_ERROR:
                    return "浮点数异常";
                case SEGV_ERROR:
                    return "段错误";
                case UNKNOWN:
                    return "服务器繁忙，请稍后重试";
                default:
                    return "服务器繁忙，请稍后重试";
            }
        }
        std::string StatusToString(enum Status status)
        {
            switch(status)
            {
                case AC:
                    return "AC";
                case WA:
                    return "WA";
                case RUNTIME_ERROR:
                    return "RUNTIME_ERROR";
                case MEMORY_LIMIT:
                    return "MEMORY_LIMIT";
                case COMPILE_ERROR:
                    return "COMPILE_ERROR";
                case FPE_ERROR:
                    return "FPE_ERROR";
                case SEGV_ERROR:
                    return "SEGV_ERROR";
                case UNKNOWN:
                    return "UNKNOWN";
                default:
                    return "WA";
            }
        }
        std::string HandlerProgramEnd(const std::vector<Status>& result,const std::string& file_name,const int line,const std::string filename)
        {
            logger(ns_log::DEBUG)<<"在 "<<filename<<" 的 "<<line<<" 行结束责任链";
            Json::Value out_value;
            std::string result_string;
            int final_result = 0;
            for(auto & it : result)
            {
                final_result |= it;
                result_string += StatusToString(it) + " ";
            }
            out_value["status"] = result_string;
            if(result.size() == 1 && result[0] != AC && result[0] != WA)
            {
                out_value["desc"] = StatusToDesc(result[0]);
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
                out_value["stdout"] = _stdout;
                std::string _stderr;
                // 如果是编译错误，读取编译错误文件
                if(result[0] == COMPILE_ERROR)
                {
                    FileUtil::ReadFile(PathUtil::Compile_err(file_name), &_stderr, true);
                }
                else
                {
                    FileUtil::ReadFile(PathUtil::Stderr(file_name), &_stderr, true);
                }
                out_value["stderr"] = _stderr;
            }
            else
            {
                if(final_result == 0) out_value["desc"] = StatusToDesc(AC);
                else out_value["desc"] = StatusToDesc(WA);
            }
            Json::StreamWriterBuilder builder;
            builder["emitUTF8"] = true;
            std::string out_json = Json::writeString(builder, out_value);
            FileUtil::RemoveTmpFiles(file_name);
            logger(DEBUG)<<"out_json:"<<out_json;
            return out_json;
        }
    protected:
        bool _is_enable = true;
        std::shared_ptr<HandlerProgram> _next;
    };
    #define HandlerProgramEnd(a,b) (HandlerProgramEnd(a,b,__LINE__,__FILE__))
};
