#include <httplib.h>
#include "oj_control.hpp"
#include <string>
#include <fstream>

using namespace httplib;

// URL编码函数
std::string url_encode(const std::string &value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else {
            result += '%';
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", static_cast<unsigned char>(c));
            result += hex;
        }
    }
    return result;
}

int main()
{
    Server svr;
    ns_control::Control ctl;
    svr.Get("/", [](const Request& req,Response& rep){
        std::ifstream in("template_html/index.html");
        std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        rep.set_content(html, "text/html;charset=utf-8");
    });
    svr.Get("/all_questions", [&ctl](const Request& req,Response& rep){
        std::string html;
        ctl.AllQuestions(&html);
        rep.set_content(html,"text/html;charset=utf-8");
    });
    svr.Get(R"(/questions/(\d+))", [&ctl](const Request& req,Response& rep){
        std::string number = req.matches[1];
        std::string html;
        ctl.OneQuestion(number, &html);
        rep.set_content(html,"text/html;charset=utf-8");
    }); 
    svr.Post(R"(/judge/(\d+))", [&ctl](const Request& req,Response& rep){
       std::string number = req.matches[1];
        std::string result_json;
        
        // 检查请求头，判断数据格式
        std::string content_type = req.get_header_value("Content-Type");
        std::string in_json;
        
        if (content_type.find("application/json") != std::string::npos) {
            // 如果是JSON格式，直接使用req.body
            in_json = req.body;
        } else {
            // 如果是传统表单提交，解析code参数
            auto it = req.params.find("code");
            if (it != req.params.end()) {
                // 构建JSON格式
                Json::Value root;
                root["code"] = it->second;
                Json::FastWriter writer;
                in_json = writer.write(root);
            } else {
                // 如果没有code参数，返回错误
                rep.set_content("{\"status\": 4, \"desc\": \"代码不能为空\", \"stderr\": \"\", \"stdout\": \"\"}", "application/json;charset=utf-8");
                return;
            }
        }
        
        ctl.Judge(number, in_json, &result_json);
        // 重定向到judge_result.html页面，并将JSON结果作为参数传递
        std::string encoded_result = url_encode(result_json);
        rep.set_redirect("/judge_result.html?result=" + encoded_result);
    }); 
    // 提供静态文件访问
    svr.set_mount_point("/", "./template_html");
    svr.set_mount_point("/pictures", "./pictures");
    svr.listen("0.0.0.0", 8080);
    return 0;
}