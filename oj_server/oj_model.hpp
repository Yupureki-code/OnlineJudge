#pragma once

#include <cstdio>
#include <assert.h>
#include <unordered_map>
#include <vector>
#include <fstream>
#include "../comm/comm.hpp"
#include "../comm/logstrategy.hpp"

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    struct Question
    {
        std::string number;
        std::string title;
        std::string star;
        int cpu_limit;
        int memory_limit;
        std::string desc;
    };

    const std::string default_questions_path = "./questions/";
    const std::string default_questions_list_name = "questions_list";
    const std::string space = " ";

    class Model
    {
    private:
        bool LoadQuestionList(const std::string& question_list)
        {
            std::ifstream in(question_list);
            if(!in.is_open())
                return false;
            std::string line;
            while(std::getline(in,line))
            {
                std::vector<std::string> v;
                StringUtil::SplitString(line, space, &v);
                if(v.size() != 5)
                {
                    logger(ns_log::WARNING)<<"题目属性不全";
                    continue;
                }
                Question q;
                q.number = v[0];
                q.title = v[1];
                q.star = v[2];
                q.cpu_limit = stoi(v[3]);
                q.memory_limit = stoi(v[4]);
                FileUtil::ReadFile(default_questions_path + q.number + "_desc.txt", &q.desc,true);
                _questions[q.number] = q;
            }
            logger(ns_log::INFO)<<"加载题库成功";
            in.close();
            return true;
        }
    public:
        Model()
        {
            assert(LoadQuestionList(default_questions_path + default_questions_list_name));
        }
        bool GetAllQuestions(std::vector<Question>* qs)
        {
            if(_questions.empty())
                return false;
            for(auto & it : _questions)
            {
                qs->push_back(it.second);
            }
            return true;
        }
        bool GetOneQuestion(const std::string& number,Question* q)
        {
            auto it = _questions.find(number);
            if(it == _questions.end())
            {
                logger(ns_log::INFO)<<"未找到该题目: "<<number;
                return false;
            }
            *q = it->second;
            return true;
        }
    private:
        std::unordered_map<std::string, Question> _questions;
    };
};