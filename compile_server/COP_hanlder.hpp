#pragma once

#include <cstdio>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "../comm/logstrategy.hpp"
#include "../comm/comm.hpp"

//责任链模式:preprocesser->compiler->runner->judge

namespace ns_hanlder
{
    using namespace ns_log;
    using namespace ns_util;

    //题目状态码
    enum Status
    {
        AC = 0,         //通过
        WA,             //错误
        RUNTIME_ERROR,  //运行超时
        MEMORY_LIMIT,   //超过内存大小限制
        COMPILE_ERROR,  //编译错误
        FPE_ERROR,      //浮点数异常
        SEGV_ERROR,     //段错误
        UNKNOWN         //未知错误
    };

    //模板类
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
        //退出码转状态码
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
        //状态码转描述
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
                    return "未知错误";
            }
        }
        //状态码转字符串
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
                    return "UNKNOWN";
            }
        }
        //责任链结束
        std::string HandlerProgramEnd(const std::vector<Status>& result,const std::string& file_name,const int line,const std::string filename)
        {
            logger(ns_log::DEBUG)<<"在 "<<filename<<" 的 "<<line<<" 行结束责任链";
            Json::Value out_value;
            std::string result_string;
            bool has_wa = false;
            bool has_error = false;
            
            for(auto & it : result)
            {
                result_string += StatusToString(it) + " ";
                if(it == WA)
                    has_wa = true;
                if(it != AC && it != WA)
                    has_error = true;
            }
            out_value["status"] = result_string;
            
            // 判断最终结果
            if(has_error)
            {
                // 有运行时错误、内存错误等
                out_value["desc"] = StatusToDesc(result[0]);
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
                out_value["stdout"] = _stdout;
                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stderr(file_name), &_stderr, true);
                out_value["stderr"] = _stderr;
            }
            else if(has_wa)
            {
                // 有WA
                out_value["desc"] = StatusToDesc(WA);
            }
            else
            {
                // 全部AC
                out_value["desc"] = StatusToDesc(AC);
            }
            
            Json::StyledWriter writer;
            std::string out_json = writer.write(out_value);
            FileUtil::RemoveTmpFiles(file_name);
            logger(DEBUG)<<"out_json:"<<out_json;
            return out_json;
        }
    protected:
        std::shared_ptr<HandlerProgram> _next;
        bool _is_enable = true;
    };
    #define HandlerProgramEnd(a,b) (HandlerProgramEnd(a,b,__LINE__,__FILE__))
};