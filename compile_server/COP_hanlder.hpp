#pragma once

#include "../comm/comm.hpp"
#include <jsoncpp/json/value.h>
#include <memory>
#include <string>
#include <jsoncpp/json/json.h>

namespace ns_hanlder
{
    using namespace ns_util;

    const std::string default_questions_path = "./questions/";
    const std::string space = ":";

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
                case 8:
                    return FPE_ERROR;
                case 11:
                    return SEGV_ERROR;
                case 6:
                    return MEMORY_LIMIT;
                case 24:
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
        std::string HandlerProgramEnd(enum Status status_code,const std::string& file_name)
        {
            Json::Value out_value;
            out_value["status"] = status_code;
            out_value["desc"] = StatusToDesc(status_code);
            // 读取stdout和stderr，无论程序是否正常结束
            std::string _stdout;
            FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
            out_value["stdout"] = _stdout;
            std::string _stderr;
            FileUtil::ReadFile(PathUtil::Stderr(file_name), &_stderr, true);
            out_value["stderr"] = _stderr;
            Json::StreamWriterBuilder builder;
            builder["emitUTF8"] = true;  // 输出UTF-8编码的中文，不进行Unicode转义
            std::string out_json = Json::writeString(builder, out_value);
            FileUtil::RemoveTmpFiles(file_name);
            return out_json;
        }
    protected:
        bool _is_enable = true;
        std::shared_ptr<HandlerProgram> _next;
    };
};