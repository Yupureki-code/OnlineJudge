#pragma once

#include <cstdio>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../comm/logstrategy.hpp"
#include "../comm/comm.hpp"
#include "COP_hanlder.hpp"

namespace ns_preprocesser
{
    using namespace ns_util;
    using namespace ns_log;
    using namespace ns_hanlder;

    class HandlerPreProcesser : public HandlerProgram
    {
    public:
        std::string Execute(const std::string& file,const std::string& in_json)override
        {
            std::string file_name;
            if(_is_enable)
            {
                Json::Value in_value;
                Json::Reader reader;
                reader.parse(in_json,in_value);
                file_name = FileUtil::GetUniqueFileName();
                logger(ns_log::INFO)<<"文件名称: "<<file_name;
                std::string code = in_value["code"].asString();
                if(!FileUtil::WriteFile(PathUtil::Src(file_name), code))
                {
                    logger(ns_log::FATAL)<<"打开文件 "<<PathUtil::Src(file_name)<<"失败";
                    return HandlerProgramEnd({UNKNOWN}, file_name);
                }
            }
            if(_next)
            {
                return _next->Execute(file_name, in_json);
            }
            else 
            {
                //责任链不应该在这结束
                ns_log::logger(ns_log::INFO)<<"责任链错误结束";
                return HandlerProgramEnd({UNKNOWN}, file_name);
            }
        }
    };
};