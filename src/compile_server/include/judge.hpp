#pragma once

#include <cstdio>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "COP_hanlder.hpp"
#include <Logger/logstrategy.h>
// 判断类:判断运行结果
// 1.运行中错误，获取对应的错误类型
// 2.运行正常，判断答案对错。对->AC，错->WA

namespace ns_judge
{
    using namespace ns_hanlder;
    using namespace ns_util;
    using namespace ns_log;

    class HandlerJudge : public HandlerProgram
    {
    public:
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            std::vector<enum Status> result;
            if(_is_enable)
            {
                for(int i = 1;;i++)
                {
                    std::string test_file = file_name +"_" + std::to_string(i);
                    std::string out,ans,err;
                    std::string stderr_path = PathUtil::Stderr(test_file);
                    if(!FileUtil::ReadFile(stderr_path, &err))
                    {
                        if(result.size() > 0)
                            break;
                        else
                        {
                            logger(FATAL)<<"读取stderr文件失败: "<<stderr_path;
                            return HandlerProgramEnd({UNKNOWN}, file_name);
                        }
                    }
                    logger(DEBUG)<<"stderr: "<<err;
                    //判断运行是否出错
                    if(err == "运行时间超时") result.push_back(RUNTIME_ERROR);
                    else if(err == "内存申请过多") result.push_back(MEMORY_LIMIT);
                    else if(err == "浮点数异常") result.push_back(FPE_ERROR);
                    else if(err == "段错误") result.push_back(SEGV_ERROR);
                    else if(err == "服务器繁忙，请稍后重试") result.push_back(UNKNOWN);
                    else
                    {
                        //运行正常，判断答案对错
                        std::string stdout_path = PathUtil::Stdout(test_file);
                        std::string ans_path = PathUtil::Ans(test_file);
                        //out:用户答案
                        //ams:答案
                        std::string out,ans;
                        FileUtil::ReadFile(stdout_path, &out);
                        FileUtil::ReadFile(ans_path, &ans);
                        //去掉字符串内的全部空格，换行符等
                        out = StringUtil::RemoveAllWhitespace(out);
                        ans = StringUtil::RemoveAllWhitespace(ans);
                        if(out == ans)
                            result.push_back(AC);
                        else
                            result.push_back(WA);
                    }
                }
            }
            if(_next)
            {
                //不应该继续执行
                _next->Execute(file_name, in_json);
            }
            else
            {
                return HandlerProgramEnd(result, file_name);
            }
        }
    };
};