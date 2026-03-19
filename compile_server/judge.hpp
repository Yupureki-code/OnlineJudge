#pragma once

#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "COP_hanlder.hpp"
#include <unordered_map>
#include "../comm/logstrategy.hpp"

namespace ns_judge
{
    using namespace ns_hanlder;
    using namespace ns_util;
    using namespace ns_log;

    class HandlerJudge : public HandlerProgram
    {
    private:
        bool LoadQuestionTests()
        {
            std::ifstream lists(default_questions_path + "test_lists.txt");
            if(!lists.is_open())
            {
                logger(ns_log::FATAL)<<"未找到测试列表 : "<<default_questions_path + "test_lists.txt";
                return false;
            }
            std::string test_id;
            while(std::getline(lists,test_id))
            {
                if(test_id.empty()) continue;
                std::string list = default_questions_path + test_id + "/";
                std::string in,out;
                if(!FileUtil::ReadFile(list + "in.txt", &in))
                {
                    logger(ns_log::WARNING)<<"未找到 "<<list + "in.txt";
                    continue;
                }
                if(!FileUtil::ReadFile(list + "out.txt", &out))
                {
                    logger(ns_log::WARNING)<<"未找到 "<<list + "out.txt";
                    continue;
                }
                _tests[test_id] = {in,out};
            }
            return true;
        }
    public:
        HandlerJudge()
        {
            assert(LoadQuestionTests());
        }
        std::string Execute(const std::string& file_name,const std::string& in_json)override
        {
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json,in_value);
            std::string id = in_value["id"].asString();
            std::string right_ans;
            FileUtil::ReadFile(default_questions_path + id + "/out.txt", &right_ans);
            std::string user_ans;
            FileUtil::ReadFile(PathUtil::Stdout(file_name), &user_ans);
            if(user_ans == right_ans)
                return HandlerProgramEnd(AC, file_name);
            else
                return HandlerProgramEnd(WA, file_name);
        }
    private:
        std::unordered_map<std::string,std::pair<std::string,std::string>> _tests;
    };
};