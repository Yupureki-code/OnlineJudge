#include <httplib.h>
#include "../include/oj_control.hpp"
#include <string>
#include <fstream>
#include "../../comm/logstrategy.hpp"

using namespace httplib;
using namespace ns_log;
// URL编码函数
std::string url_encode(const std::string &value) 
{
    std::string result;
    result.reserve(value.size());
    for (char c : value) 
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') 
        {
            result += c;
        } 
        else 
        {
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
    //ns_log::Logger::GetInstance().enable_file_log_strategy(LOG_PATH,"oj_server.log");
    Server svr;
    ns_control::Control ctl;
    //注册方法:主页面
    svr.Get("/", [](const Request& req,Response& rep){
        std::ifstream in( HTML_PATH +  std::string("/index.html"));
        std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        rep.set_content(html, "text/html;charset=utf-8");
    });
    //注册方法:题库
    svr.Get("/all_questions", [&ctl](const Request& req,Response& rep){
        int page = 1;
        //分页设计
        if (req.has_param("page")) 
        {
            page = std::stoi(req.get_param_value("page", 0));
            if (page < 1) page = 1;
        }
        std::string html;
        ctl.AllQuestions(&html, page);
        rep.set_content(html,"text/html;charset=utf-8");
    });
    //注册方法:关于
    svr.Get("/about", [](const Request& req, Response& rep){
        std::ifstream in(HTML_PATH +  std::string("/about.html"));
        std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        rep.set_content(html, "text/html;charset=utf-8");
    });
    //注册方法:用户首页
    svr.Get("/user/profile", [](const Request& req, Response& rep){
        std::ifstream in(HTML_PATH +  std::string("/user/profile.html"));
        std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        rep.set_content(html, "text/html;charset=utf-8");
    });
    //注册方法:单个题目
    svr.Get(R"(/questions/(\d+))", [&ctl](const Request& req,Response& rep){
        std::string number = req.matches[1];
        std::string html;
        ctl.OneQuestion(number, &html);
        rep.set_content(html,"text/html;charset=utf-8");
    }); 
    //注册方法:判题
    svr.Post(R"(/judge/(\d+))", [&ctl](const Request& req,Response& rep){
       std::string number = req.matches[1];
        std::string result_json;
        
        // 检查请求头，判断数据格式
        std::string content_type = req.get_header_value("Content-Type");
        std::string in_json;
        
        if (content_type.find("application/json") != std::string::npos) {
            // 如果是JSON格式，直接使用req.body
            in_json = req.body;
        } else 
        {
            // 如果是传统表单提交，解析code参数
            auto it = req.params.find("code");
            if (it != req.params.end()) 
            {
                // 构建JSON格式
                Json::Value root;
                root["code"] = it->second;
                Json::FastWriter writer;
                in_json = writer.write(root);
            } 
            else 
            {
                // 如果没有code参数，返回错误
                rep.set_content("{\"status\": 4, \"desc\": \"代码不能为空\", \"stderr\": \"\", \"stdout\": \"\"}", "application/json;charset=utf-8");
                return;
            }
        }
        
        ctl.Judge(number, in_json, &result_json);
        // 重定向到judge_result.html页面，并将JSON结果和题目ID作为参数传递
        std::string encoded_result = url_encode(result_json);
        rep.set_redirect("/judge_result.html?result=" + encoded_result + "&id=" + number);
    }); 
    // 提供静态文件访问
    svr.set_mount_point("/", HTML_PATH);
    svr.set_mount_point("/pictures", HTML_PATH + std::string("/pictures"));

    // 为每个API接口添加CORS头
    auto addCORSHeaders = [](Response& rep) {
        rep.set_header("Access-Control-Allow-Origin", "*");
        rep.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        rep.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    // 用户相关API
    // 检查用户是否存在
    svr.Post("/api/user/check", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Reader reader;
        Json::Value in_value;
        reader.parse(req.body, in_value);
        std::string email = in_value["email"].asString();
        ns_log::logger(INFO)<<"用户登陆 email:"<<email;
        Json::Value response;
        ctl.CheckUser(email, response);
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    // 创建新用户
    svr.Post("/api/user/create", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Reader reader;
        Json::Value in_value;
        reader.parse(req.body, in_value);
        std::string name = in_value["name"].asString();
        std::string email = in_value["email"].asString();
        ns_log::logger(ns_log::INFO)<<"创建新用户 name:"<<name<<" email:"<<email;
        Json::Value response;
        ctl.CreateUser(name, email, response);
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    // 获取用户信息
    svr.Post("/api/user/get", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Reader reader;
        Json::Value in_value;
        reader.parse(req.body, in_value);
        std::string email = in_value["email"].asString();
        Json::Value response;
        ctl.GetUser(email, response);
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    // 处理OPTIONS请求
    svr.Options("/api/*", [&addCORSHeaders](const Request& req, Response& rep) {
        addCORSHeaders(rep);
        rep.status = 200;
    });

    //设置守护进程
    //Daemon(false, false);
    svr.listen("0.0.0.0", 8080);
    return 0;
}