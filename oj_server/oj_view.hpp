#pragma once

#include <string>
#include <vector>
#include <ctemplate/template.h>
#include "oj_model.hpp"

namespace ns_view
{
    using namespace ns_model;

    const std::string default_template_path = "./template_html/";

    class View
    {
    public:
        void AllExpandHtml(const std::vector<struct Question> &questions, std::string *html)
        {
            std::string src_html = default_template_path + "all_questions.html";
            ctemplate::TemplateDictionary root("all_questions");
            for (const auto& q : questions)
            {
                ctemplate::TemplateDictionary *sub = root.AddSectionDictionary("question_list");
                sub->SetValue("number", q.number);
                sub->SetValue("title", q.title);
                sub->SetValue("star", q.star);
            }
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);
            tpl->Expand(html, &root);
        }
        void OneExpandHtml(const struct Question &q, std::string *html)
        {
            std::string src_html = default_template_path + "one_question.html";
            ctemplate::TemplateDictionary root("one_question");
            root.SetValue("number", q.number);
            root.SetValue("title", q.title);
            root.SetValue("star", q.star);
            root.SetValue("desc", q.desc);
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html, ctemplate::DO_NOT_STRIP);
            tpl->Expand(html, &root);
        }
    };
};