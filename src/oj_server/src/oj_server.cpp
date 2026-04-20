#include <httplib.h>
#include "../include/oj_control.hpp"
#include <string>
#include <fstream>
#include <algorithm>
#include "../../comm/logstrategy.hpp"

using namespace httplib;
using namespace ns_log;

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

std::string ReadHtmlFile(const std::string& path)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        return "";
    }
    std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    return html;
}

std::string InjectUserInfo(const std::string& html, ns_control::User* user, bool isLoggedIn)
{
    std::string result = html;
    
    std::string userInfoScript = R"(<script>)";
    userInfoScript += "var SERVER_USER_INFO = {";
    userInfoScript += "isLoggedIn: " + std::string(isLoggedIn ? "true" : "false");
    if (isLoggedIn && user)
    {
        userInfoScript += ", name: \"" + user->name + "\"";
        userInfoScript += ", email: \"" + user->email + "\"";
        userInfoScript += ", uid: " + std::to_string(user->uid);
    }
    userInfoScript += "};";
    
    if (isLoggedIn) {
        userInfoScript += R"(document.addEventListener('DOMContentLoaded', function() {
        var ua = document.getElementById('user-auth');
        var up = document.getElementById('user-profile');
        if (ua && up) { ua.style.display = 'none'; up.style.display = 'flex'; }
    });)";
    }
    
    userInfoScript += "</script>";
    
    size_t bodyEndPos = result.find("</body>");
    if (bodyEndPos != std::string::npos)
    {
        result.insert(bodyEndPos, userInfoScript);
    }
    else
    {
        result += userInfoScript;
    }
    
    return result;
}

int main()
{
    Server svr;
    ns_control::Control ctl;

    auto addCORSHeaders = [](Response& rep) {
        rep.set_header("Access-Control-Allow-Origin", "*");
        rep.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        rep.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    auto getCurrentUser = [&ctl](const Request& req, ns_control::User* user) -> bool {
        std::string cookie_header = req.get_header_value("Cookie");
        return ctl.GetSessionUser(cookie_header, user);
    };

    svr.set_mount_point("/pictures", HTML_PATH + std::string("/pictures"));
    svr.set_mount_point("/js", HTML_PATH + std::string("/js"));
    svr.set_mount_point("/css", HTML_PATH + std::string("/css"));
    svr.set_mount_point("/spa", HTML_PATH + std::string("/spa"));

    svr.Get("/js/(.*)", [](const Request& req, Response& rep){
        std::string file = req.matches[1];
        std::string content = ReadHtmlFile(HTML_PATH + std::string("/js/") + file);
        rep.set_content(content, "application/javascript;charset=utf-8");
    });

    svr.Get("/", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html = ReadHtmlFile(HTML_PATH + "/spa/app.html");
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get("/all_questions", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        int page = 1;
        if (req.has_param("page")) 
        {
            page = std::stoi(req.get_param_value("page", 0));
            if (page < 1) page = 1;
        }
        std::string html;
        ctl.AllQuestions(&html, page);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html,"text/html;charset=utf-8");
    });

    svr.Get("/about", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html = ReadHtmlFile(HTML_PATH + "/about.html");
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get("/user/profile", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html = ReadHtmlFile(HTML_PATH + "/user/profile.html");
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get(R"(/questions/(\d+)$)", [&ctl, &getCurrentUser](const Request& req,Response& rep){
        std::string number = req.matches[1];
        std::string html;
        ctl.OneQuestion(number, &html);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html,"text/html;charset=utf-8");
    }); 

    svr.Get(R"(/judge_result\.html)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html = ReadHtmlFile(HTML_PATH + "/judge_result.html");
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get(R"(/one_question\.html)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html = ReadHtmlFile(HTML_PATH + "/one_question.html");
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Post(R"(/judge/(\d+)$)", [&ctl](const Request& req,Response& rep){
       std::string number = req.matches[1];
        std::string result_json;
        
        std::string content_type = req.get_header_value("Content-Type");
        std::string in_json;
        
        if (content_type.find("application/json") != std::string::npos) {
            in_json = req.body;
        } else 
        {
            auto it = req.params.find("code");
            if (it != req.params.end()) 
            {
                Json::Value root;
                root["code"] = it->second;
                Json::FastWriter writer;
                in_json = writer.write(root);
            } 
            else 
            {
                rep.set_content("{\"status\": 4, \"desc\": \"代码不能为空\", \"stderr\": \"\", \"stdout\": \"\"}", "application/json;charset=utf-8");
                return;
            }
        }
        
        ctl.Judge(number, in_json, &result_json);
        std::string encoded_result = url_encode(result_json);
        rep.set_redirect("/judge_result.html?result=" + encoded_result + "&id=" + number);
    }); 

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

    svr.Post("/api/user/create", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Reader reader;
        Json::Value in_value;
        reader.parse(req.body, in_value);
        std::string name = in_value["name"].asString();
        std::string email = in_value["email"].asString();
        ns_log::logger(ns_log::INFO)<<"创建新用户 name:"<<name<<" email:"<<email;
        Json::Value response;
        ctl.CreateUser(name, email, response);
        
        if (response["created"].asBool())
        {
            ns_control::User user;
            if (ctl.GetUser(email, static_cast<ns_control::User*>(&user)))
            {
                std::string session_id = ctl.CreateSession(user.uid, email);
                std::string cookie = ctl.GetSetCookieHeader(session_id);
                rep.set_header("Set-Cookie", cookie);
                ns_log::logger(ns_log::INFO) << "用户登录成功，设置 Session: " << email;
            }
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

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

    svr.Post("/api/user/login", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Reader reader;
        Json::Value in_value;
        reader.parse(req.body, in_value);
        std::string email = in_value["email"].asString();
        
        Json::Value response;
        bool found = ctl.CheckUser(email, response);
        
        if (found && response["exists"].asBool())
        {
            ns_control::User user;
            if (ctl.GetUser(email, static_cast<ns_control::User*>(&user)))
            {
                std::string session_id = ctl.CreateSession(user.uid, email);
                std::string cookie = ctl.GetSetCookieHeader(session_id);
                rep.set_header("Set-Cookie", cookie);
                ns_log::logger(ns_log::INFO) << "用户登录成功，设置 Session: " << email;
            }
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Post("/api/user/logout", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        std::string cookie_header = req.get_header_value("Cookie");
        ns_control::User user;
        if (ctl.GetSessionUser(cookie_header, &user))
        {
            ctl.DestroySession(cookie_header);
            ns_log::logger(ns_log::INFO) << "用户退出登录: " << user.email;
        }
        
        std::string cookie = ctl.GetClearCookieHeader();
        rep.set_header("Set-Cookie", cookie);
        
        Json::Value response;
        response["success"] = true;
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Get("/api/questions", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        std::vector<ns_model::Question> all;
        
        if (ctl.GetModel()->GetAllQuestions(&all)) {
            sort(all.begin(), all.end(), [](const ns_model::Question &q1, const ns_model::Question &q2){
                return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
            });
            
            Json::Value questions(Json::arrayValue);
            for (const auto& q : all) {
                Json::Value question;
                question["number"] = q.number;
                question["title"] = q.title;
                question["star"] = q.star;
                questions.append(question);
            }
            response["questions"] = questions;
            response["success"] = true;
        } else {
            response["success"] = false;
            response["questions"] = Json::Value(Json::arrayValue);
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Get("/api/user/info", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        if (isLoggedIn) {
            response["success"] = true;
            response["user"]["name"] = user.name;
            response["user"]["email"] = user.email;
            response["user"]["create_time"] = user.create_time;
        } else {
            response["success"] = false;
            response["message"] = "未登录";
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Options("/api/*", [&addCORSHeaders](const Request& req, Response& rep) {
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.listen("0.0.0.0", 8080);
    return 0;
}
