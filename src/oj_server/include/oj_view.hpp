#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <ctemplate/template.h>
#include "oj_model.hpp"
#include "../../comm/comm.hpp"

// view: 主要用来渲染和返回html

namespace ns_view
{
    using namespace ns_model;

    // HTML转义函数
    inline std::string HtmlEscape(const std::string& input)
    {
        std::string output;
        output.reserve(input.size() * 2);
        for (char c : input)
        {
            switch (c)
            {
                case '&': output += "&amp;"; break;
                case '<': output += "&lt;"; break;
                case '>': output += "&gt;"; break;
                case '"': output += "&quot;"; break;
                case '\'': output += "&#x27;"; break;
                default: output += c; break;
            }
        }
        return output;
    }

    class View
    {
    public:
        void AllExpandHtml(const std::vector<struct Question> &questions, std::string *html, int page = 1, int totalPages = 1)
        {
            // 题目的编号 题目的标题 题目的难度
            // 推荐使用表格显示
            // 1. 形成路径
            std::string src_html = HTML_PATH +  std::string("all_questions.html");
            //形成数字典
            ctemplate::TemplateDictionary root("all_questions");
            for (const auto& q : questions)
            {
                ctemplate::TemplateDictionary *sub = root.AddSectionDictionary("question_list");
                sub->SetValue("number", q.number);
                sub->SetValue("title", q.title);
                sub->SetValue("star", q.star);
            }
            root.SetValue("current_page", std::to_string(page));
            root.SetValue("total_pages", std::to_string(totalPages));
            //获得渲染的html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);
            //开始渲染html
            tpl->Expand(html, &root);
        }
        void OneExpandHtml(const struct Question &q, std::string *html)
        {
            //形成目录
            std::string src_html = HTML_PATH + std::string("one_question.html");
            //形成数字典
            ctemplate::TemplateDictionary root("one_question");
            root.SetValue("number", q.number);
            root.SetValue("title", q.title);
            root.SetValue("star", q.star);
            root.SetValue("desc", HtmlEscape(q.desc));
            //获取渲染的html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);
            //开始渲染html
            tpl->Expand(html, &root);
        }
        bool GetStaticHtml(const std::string& name, std::string* html, bool* cache_hit = nullptr)
        {
            {
                std::lock_guard<std::mutex> lock(_html_cache_mtx);
                auto it = _html_cache.find(name);
                if (it != _html_cache.end())
                {
                    if (cache_hit != nullptr) *cache_hit = true;
                    *html = it->second;
                    return true;
                }
            }

            std::string path = HTML_PATH + "/" + name;
            std::ifstream in(path);
            if (!in.is_open())
            {
                if (cache_hit != nullptr) *cache_hit = false;
                return false;
            }
            std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            in.close();
            if (cache_hit != nullptr) *cache_hit = false;
            *html = content;
            {
                std::lock_guard<std::mutex> lock(_html_cache_mtx);
                _html_cache[name] = content;
            }
            return true;
        }
    private:
        std::unordered_map<std::string, std::string> _html_cache; //html缓存
        std::mutex _html_cache_mtx;
    };
};