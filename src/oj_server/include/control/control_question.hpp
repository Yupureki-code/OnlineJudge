#pragma once

#include "control_base.hpp"

namespace ns_control
{

    class ControlQuestion : public ControlBase
    {
    public:
        bool SaveQuestion(const Question& q)
        {
            return _model.Question().SaveQuestion(q);
        }
        //检查该用户是否通过该题目
        bool HasUserPassedQuestion(int user_id, const std::string& question_id)
        {
            return _model.User().HasUserPassedQuestion(user_id, question_id);
        }

        bool DeleteQuestion(const std::string& number)
        {
            return _model.Question().DeleteQuestion(number);
        }

        // 题目写路径统一调用：递增列表版本，触发列表缓存失效。
        std::string TouchQuestionListVersion()
        {
            return _model.Question().TouchQuestionListVersion();
        }

        //加载一页内的全部题目
        bool AllQuestions(std::string *html,
                          int page = 1,
                          int size = 5,
                          std::shared_ptr<QueryStruct> query_hash = nullptr,
                          const std::string& list_version = "1")
        {
            //list_version参数目前没有实际作用，后续可以用来实现当题库发生变化时自动更新缓存的功能
            //目前先直接使用_model.GetQuestionsListVersion()获取版本号，保证缓存一致性
            (void)list_version;
            std::string effective_list_version = _model.Question().GetQuestionsListVersion();
            auto html_key = _model.Question().BuildListCacheKey(query_hash, page, size, effective_list_version, Cache::CacheKey::PageType::kHtml);
            //先在cache中找静态HTML页面，如果找到直接返回
            //如果没有找到再查询数据并生成HTML页面，最后将生成的HTML页面写入cache供下次访问使用
            if(_model.GetHtmlPage(html, html_key))
            {
                _model.RecordListHtmlCacheMetrics(true);
                logger(ns_log::INFO)<<"[html_cache][all_questions] hit=1 page=" << page << " size=" << size;
                return true;
            }
            _model.RecordListHtmlCacheMetrics(false);
            logger(INFO) << "[html_cache][all_questions] hit=0 page=" << page << " size=" << size;
            bool ret = true;
            std::vector<struct Question> page_questions;
            int total_count = 0;
            int total_pages = 0;
            auto data_key = _model.Question().BuildListCacheKey(query_hash, page, size, effective_list_version, Cache::CacheKey::PageType::kData);
            //先在cache中找数据，如果找到直接用数据生成HTML页面返回
            if (_model.Question().GetQuestionsByPage(data_key, page_questions, &total_count, &total_pages))
            {
                int safe_page = page;
                if (safe_page < 1) safe_page = 1;
                if (total_pages > 0 && safe_page > total_pages) safe_page = total_pages;
                if (total_pages <= 0) safe_page = 1;

                int render_total_pages = (total_pages <= 0 ? 1 : total_pages);
                _view.AllExpandHtml(page_questions, html, safe_page, render_total_pages);
                _model.SetHtmlPage(html, html_key);
            }
            else
            {
                *html = "获取题目失败, 形成题目列表失败";
                ret = false;
            }
            return ret;
        }
        //获取单个题目
        bool OneQuestion(const std::string &number, std::string *html)
        {
            bool ret = true;
            Question q;
            if (_model.Question().GetOneQuestion(number, q))
            {
                _view.OneExpandHtml(q, html);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在!";
                ret = false;
            }
            return ret;
        }

        bool GetSampleTests(const std::string& question_id, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Question q;
            //获取题目
            if (!_model.Question().GetOneQuestion(question_id, q))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }
            //获取题目的样例
            Json::Value tests(Json::arrayValue);
            if (!_model.Question().GetSampleTestsByQuestionId(question_id, &tests))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["tests"] = tests;
            return true;
        }
        //运行测试用例
        bool RunSingleTest(const std::string& question_id, const std::string& code,
                          int test_case_id, const std::string& test_type,
                          const std::string& custom_input,
                          const User& current_user,
                          Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            std::string test_input;
            std::string test_output;

            if (test_type == "sample")
            {
                if (!_model.Question().GetTestById(test_case_id, question_id, &test_input, &test_output))
                {
                    *err_code = "TEST_NOT_FOUND";
                    return false;
                }
            }
            else if (test_type == "custom")
            {
                test_input = custom_input;
                test_output = "";
            }
            else
            {
                *err_code = "INVALID_TEST_TYPE";
                return false;
            }

            Question q;
            if (!_model.Question().GetOneQuestion(question_id, q))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }

            Json::Value compile_value;
            compile_value["id"] = question_id;
            compile_value["code"] = code;
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);

            std::string out_json;
            // Forward custom_input and custom_output to judge path
            Judge(question_id, compile_string, &out_json, test_input, test_output);

            Json::Value judge_result;
            Json::Reader reader;
            if (!reader.parse(out_json, judge_result))
            {
                *err_code = "JUDGE_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["status"] = judge_result.isMember("status") ? judge_result["status"].asString() : "UNKNOWN";
            (*result)["desc"] = judge_result.isMember("desc") ? judge_result["desc"].asString() : "";
            if (judge_result.isMember("stdout"))
            {
                (*result)["stdout"] = judge_result["stdout"].asString();
            }
            if (judge_result.isMember("stderr"))
            {
                (*result)["stderr"] = judge_result["stderr"].asString();
            }
            (*result)["input"] = test_input;
            (*result)["expected_output"] = test_output;
            return true;
        }
    };

}
