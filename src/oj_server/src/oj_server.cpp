#include <httplib.h>
#include "../include/oj_control.hpp"
#include "../../comm/comm.hpp"
#include <mysql/mysql.h>
#include <memory>
#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>

using namespace httplib;
using namespace ns_log;
using namespace ns_util;

namespace {

//在request中提取查询信息，并注入到QueryStruct中
std::shared_ptr<QueryStruct> ParseQuestionQuery(const Request& req)
{
    std::shared_ptr<QuestionQuery> query = std::make_shared<QuestionQuery>();
    std::string query_mode = "title";
    std::string keyword;

    if (req.has_param("query_hash"))
    {
        std::string query_hash = req.get_param_value("query_hash");
        if (!query_hash.empty())
        {
            Json::CharReaderBuilder builder;
            Json::Value parsed;
            std::string errs;
            std::istringstream ss(query_hash);
            if (Json::parseFromStream(builder, ss, &parsed, &errs) && parsed.isObject())
            {
                if (parsed.isMember("id") && parsed["id"].isString())
                {
                    query->id = StringUtil::Trim(parsed["id"].asString());
                }
                if (parsed.isMember("title") && parsed["title"].isString())
                {
                    query->title = StringUtil::Trim(parsed["title"].asString());
                }
                if (parsed.isMember("difficulty") && parsed["difficulty"].isString())
                {
                    query->star = StringUtil::Trim(parsed["difficulty"].asString());
                }
                if (parsed.isMember("query_mode") && parsed["query_mode"].isString())
                {
                    query_mode = StringUtil::Trim(parsed["query_mode"].asString());
                }
                if (parsed.isMember("keyword") && parsed["keyword"].isString())
                {
                    keyword = StringUtil::Trim(parsed["keyword"].asString());
                }
            }
            else
            {
                query->title = StringUtil::Trim(query_hash);
            }
        }
    }

    if (req.has_param("query_mode"))
    {
        query_mode = StringUtil::Trim(req.get_param_value("query_mode"));
    }

    if (req.has_param("q"))
    {
        keyword = StringUtil::Trim(req.get_param_value("q"));
    }

    if (req.has_param("id"))
    {
        query->id = StringUtil::Trim(req.get_param_value("id"));
    }
    if (req.has_param("title"))
    {
        query->title = StringUtil::Trim(req.get_param_value("title"));
    }
    if (req.has_param("difficulty"))
    {
        query->star = StringUtil::Trim(req.get_param_value("difficulty"));
    }

    if (query_mode.empty())
    {
        query_mode = "title";
    }

    if (!keyword.empty())
    {
        if (query_mode == "id")
        {
            query->id = keyword;
            query->title.clear();
        }
        else if (query_mode == "both")
        {
            query->title = keyword;
            if (std::all_of(keyword.begin(), keyword.end(), [](unsigned char ch){ return std::isdigit(ch); }))
            {
                query->id = keyword;
            }
            else
            {
                query->id.clear();
            }
        }
        else
        {
            query->title = keyword;
            query->id.clear();
        }
    }

    return query;
}

} // namespace

//向html中注入用户信息的脚本，供前端使用
std::string InjectUserInfo(const std::string& html, User* user, bool isLoggedIn)
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
        userInfoScript += ", avatar_url: \"" + user->avatar_path + "\"";
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
    // Initialize MySQL client library for thread safety
    mysql_library_init(0, nullptr, nullptr);

    Server svr;
    ns_control::Control ctl;
    //添加CORS响应头的函数，供后续路由使用
    auto addCORSHeaders = [](Response& rep) {
        //添加CORS响应头，允许跨域访问
        rep.set_header("Access-Control-Allow-Origin", "*");
        //允许的HTTP方法
        rep.set_header("Access-Control-Allow-Methods", "POST, GET, DELETE, OPTIONS, PUT");
        //允许的HTTP请求头
        rep.set_header("Access-Control-Allow-Headers", "Content-Type");
    };
    //通过session获取用户信息的函数，供后续路由使用
    auto getCurrentUser = [&ctl](const Request& req, User* user) -> bool {
        std::string cookie_header = req.get_header_value("Cookie");
        return ctl.GetSessionUser(cookie_header, user);
    };
    //设置静态文件目录
    svr.set_mount_point("/pictures", HTML_PATH + std::string("/pictures"));
    svr.set_mount_point("/css", HTML_PATH + std::string("/css"));
    svr.set_mount_point("/spa", HTML_PATH + std::string("/spa"));
    //设置路由和处理函数
    svr.Get("/js/(.*)", [](const Request& req, Response& rep){
        std::string file = req.matches[1];
        std::string content;
        FileUtil::ReadFile(HTML_PATH + std::string("/js/") + file, &content, true);
        rep.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        rep.set_header("Pragma", "no-cache");
        rep.set_header("Expires", "0");
        rep.set_content(content, "application/javascript;charset=utf-8");
    });
    //根路由，返回首页HTML
    svr.Get("/", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("index.html", &html);
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        html = InjectUserInfo(html, &user, isLoggedIn);
        rep.set_content(html, "text/html;charset=utf-8");
    });
    //访问题库的路由，返回题库HTML
    svr.Get("/all_questions", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        //解析查询参数，构建查询结构体
        auto begin = std::chrono::steady_clock::now();
        int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
        int size = HttpUtil::ParsePositiveIntParam(req, "size", 5, 1, 100);
        auto query_hash = ParseQuestionQuery(req);
        std::string html;
        ctl.AllQuestions(&html, page, size, std::move(query_hash));
        
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html,"text/html;charset=utf-8");
        auto end = std::chrono::steady_clock::now();
        long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        logger(INFO) << "[metrics][route] GET /all_questions page=" << page
                     << " size=" << size
                     << " cost_ms=" << cost_ms;
    });
    //关于页面路由，返回关于页面HTML
    svr.Get("/about", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("about.html" , &html);
        
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });
    //用户个人中心路由，返回个人中心页面HTML
    svr.Get("/user/profile", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("user/profile.html", &html);
        
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });
    //用户账户设置路由，返回账户设置HTML
    svr.Get("/user/settings", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("user/settings.html", &html);

        User user;
        bool isLoggedIn = getCurrentUser(req, &user);

        html = InjectUserInfo(html, &user, isLoggedIn);

        rep.set_content(html, "text/html;charset=utf-8");
    });
    //题目详情路由，返回题目详情HTML
    svr.Get(R"(/questions/(\d+)$)", [&ctl, &getCurrentUser](const Request& req,Response& rep){
        auto begin = std::chrono::steady_clock::now();
        std::string number = req.matches[1];
        std::string html;
        ctl.OneQuestion(number, &html);
        
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html,"text/html;charset=utf-8");
        auto end = std::chrono::steady_clock::now();
        long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        logger(INFO) << "[metrics][route] GET /questions/" << number << " cost_ms=" << cost_ms;
    }); 
    // 发布题解页面路由，返回独立发布页面HTML
    svr.Get(R"(/questions/(\d+)/solutions/new$)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("solutions/new.html", &html);

        User user;
        bool isLoggedIn = getCurrentUser(req, &user);

        html = InjectUserInfo(html, &user, isLoggedIn);

        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get(R"(/solutions/(\d+)$)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("solutions/detail.html", &html);

        User user;
        bool isLoggedIn = getCurrentUser(req, &user);

        html = InjectUserInfo(html, &user, isLoggedIn);

        rep.set_content(html, "text/html;charset=utf-8");
    });

    // 判题结果页面路由，返回判题结果HTML
    svr.Get(R"(/judge_result\.html)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("judge_result.html", &html);
        
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });
    // 判题路由，接收代码和题目编号，返回判题结果后，前端调用judge_result路由
    svr.Post(R"(/judge/(\d+)$)", [&ctl, &getCurrentUser](const Request& req,Response& rep){
       std::string number = req.matches[1];
        std::string result_json;
        
        std::string content_type = req.get_header_value("Content-Type");
        std::string in_json;
        //支持两种传参方式：application/json的请求体，或者普通表单参数code
        if (content_type.find("application/json") != std::string::npos)
        {
            //请求体是JSON格式，直接使用请求体作为代码内容
            in_json = req.body;
        } 
        else 
        {
            //请求体不是JSON格式，从表单参数中提取code参数作为代码内容
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
        //调用控制层的Judge函数进行判题，获取结果后重定向到判题结果页面，并通过URL参数传递结果数据
        ctl.Judge(number, in_json, &result_json);
        
        // 记录用户提交记录
        // compile_server 返回的 status 是空格分隔的字符串 (例: "AC AC AC ")
        // is_pass = 所有测试用例都是 "AC"
        User current_user;
        if (getCurrentUser(req, &current_user))
        {
            bool is_pass = false;
            if (!result_json.empty())
            {
                Json::Value result_value;
                Json::Reader reader;
                if (reader.parse(result_json, result_value) && result_value.isMember("status"))
                {
                    std::string status_str = result_value["status"].asString();
                    // 按空格分词，检查是否全部为 "AC"
                    is_pass = !status_str.empty();
                    std::string token;
                    for (char ch : status_str)
                    {
                        if (ch == ' ')
                        {
                            if (!token.empty() && token != "AC")
                            {
                                is_pass = false;
                                break;
                            }
                            token.clear();
                        }
                        else
                        {
                            token += ch;
                        }
                    }
                    // 检查最后一个 token（处理不以空格结尾的情况）
                    if (is_pass && !token.empty() && token != "AC")
                    {
                        is_pass = false;
                    }
                }
            }
            ctl.GetModel()->RecordUserSubmit(current_user.uid, number, result_json, is_pass);
        }
        
        std::string encoded_result = HttpUtil::url_encode(result_json);
        rep.set_redirect("/judge_result.html?result=" + encoded_result + "&id=" + number);
    }); 
    //发送邮箱验证码的路由
    svr.Post("/api/auth/send_code", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        std::string err_code;
        int retry_after_seconds = 0;
        bool ok = ctl.SendEmailAuthCode(email, req.remote_addr, &err_code, &retry_after_seconds);

        Json::Value response;
        response["success"] = ok;
        if (ok)
        {
            response["retry_after_seconds"] = retry_after_seconds;
        }
        else
        {
            response["error_code"] = err_code;
            if (err_code == "TOO_MANY_REQUESTS" || err_code == "EMAIL_DAILY_LIMIT" || err_code == "IP_DAILY_LIMIT")
            {
                rep.status = 429;
            }
            else
            {
                rep.status = 400;
            }
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/auth/verify_code", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码不能为空");
            return;
        }

        std::string code = StringUtil::Trim(in_value["code"].asString());
        if (!StringUtil::IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string name;
        if (in_value.isMember("name") && in_value["name"].isString())
        {
            name = StringUtil::Trim(in_value["name"].asString());
        }

        std::string password;
        if (in_value.isMember("password") && in_value["password"].isString())
        {
            password = in_value["password"].asString();
        }

        User user;
        bool is_new_user = false;
        std::string err_code;
        bool ok = ctl.VerifyEmailAuthCodeAndLogin(email, code, name, password, &user, &is_new_user, &err_code);

        Json::Value response;
        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            if (err_code == "ATTEMPTS_EXCEEDED")
            {
                rep.status = 429;
            }
            else
            {
                rep.status = 400;
            }
        }
        else
        {
            std::string session_id = ctl.CreateSession(user.uid, user.email);
            std::string cookie = ctl.GetSetCookieHeader(session_id);
            rep.set_header("Set-Cookie", cookie);

            response["is_new_user"] = is_new_user;
            response["user"]["uid"] = user.uid;
            response["user"]["name"] = user.name;
            response["user"]["email"] = user.email;
            response["user"]["create_time"] = user.create_time;
            response["user"]["last_login"] = user.last_login;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/user/check", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        logger(INFO)<<"用户登陆 email:"<<email;
        Json::Value response;
        response["success"] = true;
        ctl.CheckUser(email, response);
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Post("/api/user/create", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string name;
        if (!in_value.isMember("name") || !in_value["name"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "用户名不能为空");
            return;
        }
        name = StringUtil::Trim(in_value["name"].asString());
        if (name.empty() || name.size() > 64)
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "用户名长度应为1-64个字符");
            return;
        }

        std::string email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        logger(INFO)<<"创建新用户 name:"<<name<<" email:"<<email;
        Json::Value response;
        response["success"] = true;
        ctl.CreateUser(name, email, response);
        
        if (response["created"].asBool())
        {
            User user;
            if (ctl.GetUser(email, static_cast<User*>(&user)))
            {
                std::string session_id = ctl.CreateSession(user.uid, email);
                std::string cookie = ctl.GetSetCookieHeader(session_id);
                rep.set_header("Set-Cookie", cookie);
                logger(INFO) << "用户登录成功，设置 Session: " << email;
            }
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Post("/api/user/get", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        Json::Value response;
        response["success"] = true;
        ctl.GetUser(email, response);
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Post("/api/user/login", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }
        
        Json::Value response;
        response["success"] = true;
        bool found = ctl.CheckUser(email, response);
        
        if (found && response["exists"].asBool())
        {
            User user;
            if (ctl.GetUser(email, static_cast<User*>(&user)))
            {
                std::string session_id = ctl.CreateSession(user.uid, email);
                std::string cookie = ctl.GetSetCookieHeader(session_id);
                rep.set_header("Set-Cookie", cookie);
                logger(INFO) << "用户登录成功，设置 Session: " << email;
            }
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Post("/api/user/logout", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        std::string cookie_header = req.get_header_value("Cookie");
        User user;
        if (ctl.GetSessionUser(cookie_header, &user))
        {
            ctl.DestroySessionByCookieHeader(cookie_header);
            logger(INFO) << "用户退出登录: " << user.email;
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

    svr.Post("/api/user/password/set", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = false;
            response["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string password;
        if (!JsonUtil::ExtractAndValidatePassword(in_value, &password))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "密码不能为空");
            return;
        }

        std::string err_code;
        bool ok = ctl.SetPasswordForUser(current_user.email, password, &err_code);
        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            rep.status = 400;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/user/password/login", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string login_id;
        if (in_value.isMember("email") && in_value["email"].isString())
        {
            login_id = StringUtil::Trim(in_value["email"].asString());
        }
        else if (in_value.isMember("username") && in_value["username"].isString())
        {
            login_id = StringUtil::Trim(in_value["username"].asString());
        }
        else
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请输入用户名或邮箱");
            return;
        }

        if (login_id.empty())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "用户名或邮箱不能为空");
            return;
        }

        std::string password;
        if (!JsonUtil::ExtractAndValidatePassword(in_value, &password))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "密码不能为空");
            return;
        }

        User user;
        std::string err_code;
        bool ok = ctl.LoginWithPassword(login_id, password, &user, &err_code);

        Json::Value response;
        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            rep.status = 400;
        }
        else
        {
            std::string session_id = ctl.CreateSession(user.uid, user.email);
            std::string cookie = ctl.GetSetCookieHeader(session_id);
            rep.set_header("Set-Cookie", cookie);

            response["user"]["uid"] = user.uid;
            response["user"]["name"] = user.name;
            response["user"]["email"] = user.email;
            response["user"]["create_time"] = user.create_time;
            response["user"]["last_login"] = user.last_login;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/user/security/send_code", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = false;
            response["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        std::string err_code;
        int retry_after_seconds = 0;
        bool ok = ctl.SendEmailAuthCode(current_user.email, req.remote_addr, &err_code, &retry_after_seconds);

        response["success"] = ok;
        if (ok)
        {
            response["retry_after_seconds"] = retry_after_seconds;
        }
        else
        {
            response["error_code"] = err_code;
            if (err_code == "TOO_MANY_REQUESTS" || err_code == "EMAIL_DAILY_LIMIT" || err_code == "IP_DAILY_LIMIT")
            {
                rep.status = 429;
            }
            else
            {
                rep.status = 400;
            }
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/user/email/change", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = false;
            response["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码不能为空");
            return;
        }

        std::string code = StringUtil::Trim(in_value["code"].asString());
        if (!StringUtil::IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string new_email;
        if (!JsonUtil::ExtractAndValidateEmail(in_value, &new_email))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "新邮箱格式不合法");
            return;
        }

        User updated_user;
        std::string err_code;
        bool ok = ctl.ChangeEmailWithCode(current_user, new_email, code, &updated_user, &err_code);

        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            rep.status = (err_code == "ATTEMPTS_EXCEEDED") ? 429 : 400;
        }
        else
        {
            std::string cookie_header = req.get_header_value("Cookie");
            ctl.DestroySessionByCookieHeader(cookie_header);
            std::string session_id = ctl.CreateSession(updated_user.uid, updated_user.email);
            rep.set_header("Set-Cookie", ctl.GetSetCookieHeader(session_id));

            response["user"]["uid"] = updated_user.uid;
            response["user"]["name"] = updated_user.name;
            response["user"]["email"] = updated_user.email;
            response["user"]["create_time"] = updated_user.create_time;
            response["user"]["last_login"] = updated_user.last_login;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/user/password/change", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = false;
            response["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码不能为空");
            return;
        }
        std::string code = StringUtil::Trim(in_value["code"].asString());
        if (!StringUtil::IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string new_password;
        if (!in_value.isMember("new_password") || !in_value["new_password"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "新密码不能为空");
            return;
        }
        new_password = in_value["new_password"].asString();

        std::string err_code;
        bool ok = ctl.ChangePasswordWithCode(current_user.email, code, new_password, &err_code);
        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            rep.status = (err_code == "ATTEMPTS_EXCEEDED") ? 429 : 400;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/user/account/delete", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = false;
            response["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::HttpUtil::ReplyBadRequest(rep, "验证码不能为空");
            return;
        }

        std::string code = StringUtil::Trim(in_value["code"].asString());
        if (!StringUtil::IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string err_code;
        bool ok = ctl.DeleteAccountWithCode(current_user.email, code, &err_code);
        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            rep.status = (err_code == "ATTEMPTS_EXCEEDED") ? 429 : 400;
        }
        else
        {
            std::string cookie_header = req.get_header_value("Cookie");
            ctl.DestroySessionByCookieHeader(cookie_header);
            rep.set_header("Set-Cookie", ctl.GetClearCookieHeader());
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    // svr.Get("/api/questions", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
    //     Json::Value response;
    //     std::vector<Question> all;
        
    //     if (ctl.GetModel()->GetAllQuestions(&all)) {
    //         sort(all.begin(), all.end(), [](const Question &q1, const Question &q2){
    //             return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
    //         });
            
    //         Json::Value questions(Json::arrayValue);
    //         for (const auto& q : all) {
    //             Json::Value question;
    //             question["number"] = q.number;
    //             question["title"] = q.title;
    //             question["star"] = q.star;
    //             questions.append(question);
    //         }
    //         response["questions"] = questions;
    //         response["success"] = true;
    //     } else {
    //         response["success"] = false;
    //         response["questions"] = Json::Value(Json::arrayValue);
    //     }
        
    //     Json::FastWriter writer;
    //     std::string response_str = writer.write(response);
    //     addCORSHeaders(rep);
    //     rep.set_content(response_str, "application/json;charset=utf-8");
    // });

    svr.Get("/api/metrics/cache", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        Json::Value response;
        auto m = ctl.GetModel()->GetCacheMetricsSnapshot();

        response["success"] = true;
        response["list_requests"] = Json::Int64(m.list_requests);
        response["list_hits"] = Json::Int64(m.list_hits);
        response["list_misses"] = Json::Int64(m.list_misses);
        response["list_db_fallbacks"] = Json::Int64(m.list_db_fallbacks);
        response["list_total_ms"] = Json::Int64(m.list_total_ms);

        response["detail_requests"] = Json::Int64(m.detail_requests);
        response["detail_hits"] = Json::Int64(m.detail_hits);
        response["detail_misses"] = Json::Int64(m.detail_misses);
        response["detail_db_fallbacks"] = Json::Int64(m.detail_db_fallbacks);
        response["detail_total_ms"] = Json::Int64(m.detail_total_ms);

        response["html_static_requests"] = Json::Int64(m.html_static_requests);
        response["html_static_hits"] = Json::Int64(m.html_static_hits);
        response["html_static_misses"] = Json::Int64(m.html_static_misses);

        response["html_list_requests"] = Json::Int64(m.html_list_requests);
        response["html_list_hits"] = Json::Int64(m.html_list_hits);
        response["html_list_misses"] = Json::Int64(m.html_list_misses);

        response["html_detail_requests"] = Json::Int64(m.html_detail_requests);
        response["html_detail_hits"] = Json::Int64(m.html_detail_hits);
        response["html_detail_misses"] = Json::Int64(m.html_detail_misses);

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Get(R"(/api/question/(\d+))", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        std::string number = req.matches[1];
        Question q;
        
        if (ctl.GetModel()->GetOneQuestion(number, q)) {
            response["success"] = true;
            response["question"]["number"] = q.number;
            response["question"]["title"] = q.title;
            response["question"]["star"] = q.star;
            response["question"]["desc"] = q.desc;
        } else {
            response["success"] = false;
            response["error"] = "题目不存在";
        }
        
        Json::FastWriter writer;
        std::string response_str = writer.write(response);
        addCORSHeaders(rep);
        rep.set_content(response_str, "application/json;charset=utf-8");
    });

    svr.Get(R"(/api/question/(\d+)/pass_status$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = true;
            response["passed"] = false;
            response["logged_in"] = false;
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        std::string question_id = req.matches[1];
        bool passed = ctl.HasUserPassedQuestion(current_user.uid, question_id);
        response["success"] = true;
        response["passed"] = passed;
        response["logged_in"] = true;

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post(R"(/api/questions/(\d+)/solutions$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;

        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            response["success"] = false;
            response["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("title") || !in_value["title"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "标题不能为空");
            return;
        }

        if (!in_value.isMember("content_md") || !in_value["content_md"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "题解内容不能为空");
            return;
        }

        const std::string question_id = req.matches[1];
        const std::string title = in_value["title"].asString();
        const std::string content_md = in_value["content_md"].asString();

        unsigned long long solution_id = 0;
        std::string err_code;
        bool ok = ctl.PublishSolution(question_id, current_user, title, content_md, &solution_id, &err_code);

        response["success"] = ok;
        if (!ok)
        {
            response["error_code"] = err_code;
            if (err_code == "QUESTION_NOT_FOUND")
            {
                rep.status = 404;
            }
            else if (err_code == "UNAUTHORIZED" || err_code == "NOT_PASSED")
            {
                rep.status = 401;
            }
            else
            {
                rep.status = 400;
            }
        }
        else
        {
            response["solution_id"] = Json::UInt64(solution_id);
            response["question_id"] = question_id;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    //  Comment system routes for solutions
    svr.Get(R"(/api/solutions/(\d+)/comments$)", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        long long solution_id = 0;
        try { solution_id = std::stoll(req.matches[1]); } catch (...) { HttpUtil::ReplyBadRequest(rep, "无效的题解ID"); return; }
        int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
        int size = HttpUtil::ParsePositiveIntParam(req, "size", 20, 1, 1000);
        Json::Value result;
        std::string err_code;
        bool ok = ctl.GetTopLevelComments((unsigned long long)solution_id, page, size, &result, &err_code);
        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            rep.status = (err_code == "SOLUTION_NOT_FOUND" ? 404 : 500);
        }
        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Post(R"(/api/solutions/(\d+)/comments$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        long long solution_id = 0;
        try { solution_id = std::stoll(req.matches[1]); } catch (...) { HttpUtil::ReplyBadRequest(rep, "无效的题解ID"); return; }
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }
        if (!in_value.isMember("content") || !in_value["content"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "内容不能为空");
            return;
        }
        std::string content = in_value["content"].asString();
        unsigned long long parent_id = 0;
        if (in_value.isMember("parent_id") && in_value["parent_id"].isIntegral())
        {
            parent_id = static_cast<unsigned long long>(in_value["parent_id"].asUInt64());
        }
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }
        Json::Value result;
        std::string err_code;
        bool ok = ctl.PostComment((unsigned long long)solution_id, current_user, content, &result, &err_code, parent_id);
        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            rep.status = (err_code == "UNAUTHORIZED" ? 401 : (err_code == "SOLUTION_NOT_FOUND" ? 404 : 400));
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Put(R"(/api/comments/(\d+)$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        long long comment_id = 0;
        try { comment_id = std::stoll(req.matches[1]); } catch (...) { HttpUtil::ReplyBadRequest(rep, "无效的评论ID"); return; }
        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }
        if (!in_value.isMember("content") || !in_value["content"].isString())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "内容不能为空");
            return;
        }
        std::string content = in_value["content"].asString();
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }
        Json::Value result;
        std::string err_code;
        bool ok = ctl.EditComment((unsigned long long)comment_id, current_user, content, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = (err_code == "NOT_FOUND" ? 404 : (err_code == "FORBIDDEN" ? 403 : 400));
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Delete(R"(/api/comments/(\d+)$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        long long comment_id = 0;
        try { comment_id = std::stoll(req.matches[1]); } catch (...) { Json::Value r; r["success"] = false; r["error_code"] = "INVALID_ID"; Json::FastWriter writer; addCORSHeaders(rep); rep.status = 400; rep.set_content(writer.write(r), "application/json;charset=utf-8"); return; }
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }
        Json::Value result; std::string err_code;
        bool ok = ctl.DeleteComment((unsigned long long)comment_id, current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = (err_code == "NOT_FOUND" ? 404 : (err_code == "FORBIDDEN" ? 403 : 400));
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    // OPTIONS for new comment routes (CORS preflight)
    svr.Options(R"(/api/solutions/(\d+)/comments$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req; addCORSHeaders(rep); rep.status = 204; });
    svr.Options(R"(/api/comments/(\d+)$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req; addCORSHeaders(rep); rep.status = 204; });

    svr.Post("/api/questions", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User user;
        if (!getCurrentUser(req, &user))
        {
            response["success"] = false;
            response["error"] = "未登录";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("number") || !in_value["number"].isString() ||
            !in_value.isMember("title") || !in_value["title"].isString() ||
            !in_value.isMember("desc") || !in_value["desc"].isString() ||
            !in_value.isMember("star") || !in_value["star"].isString() ||
            !in_value.isMember("cpu_limit") || !in_value["cpu_limit"].isInt() ||
            !in_value.isMember("memory_limit") || !in_value["memory_limit"].isInt())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "缺少必要字段(number/title/desc/star/cpu_limit/memory_limit)");
            return;
        }

        Question q;
        q.number = StringUtil::Trim(in_value["number"].asString());
        q.title = StringUtil::Trim(in_value["title"].asString());
        q.desc = in_value["desc"].asString();
        q.star = StringUtil::Trim(in_value["star"].asString());
        q.cpu_limit = in_value["cpu_limit"].asInt();
        q.memory_limit = in_value["memory_limit"].asInt();

        if (q.number.empty() || q.title.empty())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "number/title 不能为空");
            return;
        }

        bool ok = ctl.SaveQuestion(q);
        response["success"] = ok;
        if (!ok)
        {
            response["error"] = "题目写入失败";
            rep.status = 500;
        }
        else
        {
            response["question_id"] = q.number;
            logger(INFO) << "[question] upsert id=" << q.number << " by " << user.email;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Delete(R"(/api/question/(\d+))", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        User user;
        if (!getCurrentUser(req, &user))
        {
            response["success"] = false;
            response["error"] = "未登录";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        const std::string number = req.matches[1];
        bool ok = ctl.DeleteQuestion(number);
        response["success"] = ok;
        if (!ok)
        {
            response["error"] = "题目删除失败";
            rep.status = 500;
        }
        else
        {
            response["question_id"] = number;
            logger(INFO) << "[question] delete id=" << number << " by " << user.email;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/questions/cache/invalidate", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        Json::Value response;

        User user;
        if (!getCurrentUser(req, &user))
        {
            response["success"] = false;
            response["error"] = "未登录";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        std::string version = ctl.TouchQuestionListVersion();
        response["success"] = true;
        response["list_version"] = version;
        response["operator"] = user.email;

        logger(INFO) << "[cache] list version invalidated by " << user.email << ", new_version=" << version;

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Post("/api/static/cache/invalidate", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;

        User user;
        if (!getCurrentUser(req, &user))
        {
            response["success"] = false;
            response["error"] = "未登录";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(response), "application/json;charset=utf-8");
            return;
        }

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::vector<std::string> pages;
        if (in_value.isMember("page") && in_value["page"].isString())
        {
            pages.push_back(StringUtil::Trim(in_value["page"].asString()));
        }
        if (in_value.isMember("pages") && in_value["pages"].isArray())
        {
            for (const auto& item : in_value["pages"])
            {
                if (item.isString())
                {
                    pages.push_back(StringUtil::Trim(item.asString()));
                }
            }
        }

        if (pages.empty())
        {
            addCORSHeaders(rep);
            HttpUtil::ReplyBadRequest(rep, "缺少 page 或 pages 字段");
            return;
        }

        Json::Value invalidated(Json::arrayValue);
        for (const auto& page : pages)
        {
            if (!StringUtil::IsSafeStaticPageName(page))
            {
                addCORSHeaders(rep);
                HttpUtil::ReplyBadRequest(rep, "页面路径不合法");
                return;
            }
            if (ctl.InvalidateStaticHtmlCache(page))
            {
                invalidated.append(page);
            }
        }

        response["success"] = true;
        response["operator"] = user.email;
        response["invalidated_pages"] = invalidated;
        response["count"] = static_cast<Json::UInt64>(invalidated.size());

        logger(INFO) << "[cache] static html invalidated by " << user.email
                     << ", count=" << invalidated.size();

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(response), "application/json;charset=utf-8");
    });

    svr.Get("/api/user/info", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        
        User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        if (isLoggedIn) {
            response["success"] = true;
            response["user"]["uid"] = user.uid;
            response["user"]["name"] = user.name;
            response["user"]["email"] = user.email;
            response["user"]["avatar_url"] = user.avatar_path;
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

    svr.Post("/api/user/avatar", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }

        if (!req.has_file("avatar"))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "NO_FILE";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 400; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }

        const auto& file = req.get_file_value("avatar");
        std::string content_type = file.content_type;

        Json::Value result; std::string err_code;
        bool ok = ctl.UploadAvatar(current_user, file.content, content_type, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = (err_code == "INVALID_TYPE" || err_code == "FILE_TOO_LARGE") ? 400 : 500;
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Delete("/api/user/avatar", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }

        Json::Value result; std::string err_code;
        bool ok = ctl.DeleteAvatar(current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = 500;
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Get(R"(/api/question/(\d+)/sample_tests$)", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        std::string question_id = req.matches[1];

        Json::Value result; std::string err_code;
        bool ok = ctl.GetSampleTests(question_id, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = (err_code == "QUESTION_NOT_FOUND") ? 404 : 500;
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Post(R"(/api/question/(\d+)/test$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }

        std::string question_id = req.matches[1];

        Json::Value in_value;
        if (!JsonUtil::ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep); HttpUtil::ReplyBadRequest(rep, "请求体必须是 JSON"); return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep); HttpUtil::ReplyBadRequest(rep, "缺少代码"); return;
        }

        std::string code = in_value["code"].asString();
        std::string test_type = in_value.isMember("test_type") ? in_value["test_type"].asString() : "sample";
        int test_case_id = in_value.isMember("test_case_id") ? in_value["test_case_id"].asInt() : 0;
        std::string custom_input = in_value.isMember("custom_input") ? in_value["custom_input"].asString() : "";

        Json::Value result; std::string err_code;
        bool ok = ctl.RunSingleTest(question_id, code, test_case_id, test_type, custom_input, current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = (err_code == "QUESTION_NOT_FOUND" || err_code == "TEST_NOT_FOUND") ? 404 : 400;
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Get(R"(/api/question/(\d+)/submits$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }

        std::string question_id = req.matches[1];

        Json::Value result; std::string err_code;
        bool ok = ctl.GetUserSubmits(question_id, current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = 500;
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Get("/api/user/stats", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            Json::Value r; r["success"] = false; r["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer; addCORSHeaders(rep); rep.status = 401; rep.set_content(writer.write(r), "application/json;charset=utf-8");
            return;
        }

        Json::Value result; std::string err_code;
        bool ok = ctl.GetUserStats(current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false; result["error_code"] = err_code;
            rep.status = 500;
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Options("/api/questions", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/auth/send_code", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/auth/verify_code", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/user/password/login", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/user/password/set", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/user/security/send_code", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/user/email/change", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/user/password/change", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/user/account/delete", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options(R"(/api/question/(\d+))", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options(R"(/api/question/(\d+)/pass_status$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options(R"(/api/questions/(\d+)/solutions$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Get(R"(/api/questions/(\d+)/solutions$)", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        std::string question_id = req.matches[1];

        std::string status = req.has_param("status") ? req.get_param_value("status") : "approved";
        std::string sort = req.has_param("sort") ? req.get_param_value("sort") : "latest";
        int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 100);
        int size = HttpUtil::ParsePositiveIntParam(req, "size", 10, 1, 50);

        Json::Value result;
        std::string err_code;
        bool ok = ctl.GetSolutionList(question_id, status, sort, page, size, &result, &err_code);

        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            if (err_code == "QUESTION_NOT_FOUND")
            {
                rep.status = 404;
            }
            else
            {
                rep.status = 500;
            }
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
    rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    // Get replies for a specific top-level comment
    svr.Get(R"(/api/comments/(\d+)/replies$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        long long parent_id = 0;
        try { parent_id = std::stoll(req.matches[1]); } catch (...) { HttpUtil::ReplyBadRequest(rep, "无效的评论ID"); return; }
        int page = HttpUtil::ParsePositiveIntParam(req, "page", 1, 1, 1000000);
        int size = HttpUtil::ParsePositiveIntParam(req, "size", 20, 1, 1000);
        Json::Value result;
        std::string err_code;
        bool ok = ctl.GetCommentReplies((unsigned long long)parent_id, page, size, &result, &err_code);
        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            rep.status = (err_code == "NOT_FOUND" ? 404 : 500);
        }
        Json::FastWriter writer; addCORSHeaders(rep); rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    // OPTIONS preflight for /api/comments/{id}/replies
    svr.Options(R"(/api/comments/(\d+)/replies$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req; addCORSHeaders(rep); rep.status = 204; });

    svr.Get(R"(/api/solutions/(\d+)$)", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        long long solution_id = 0;
        try { solution_id = std::stoll(req.matches[1]); } catch (...) { HttpUtil::ReplyBadRequest(rep, "无效的题解ID"); return; }

        Json::Value result;
        std::string err_code;
        bool ok = ctl.GetSolutionDetail(solution_id, &result, &err_code);

        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            if (err_code == "SOLUTION_NOT_FOUND")
            {
                rep.status = 404;
            }
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Post(R"(/api/solutions/(\d+)/like$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value result;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            result["success"] = false;
            result["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(result), "application/json;charset=utf-8");
            return;
        }

        long long solution_id = 0;
        try { solution_id = std::stoll(req.matches[1]); } catch (...) { result["success"] = false; result["error_code"] = "INVALID_ID"; Json::FastWriter writer; addCORSHeaders(rep); rep.status = 400; rep.set_content(writer.write(result), "application/json;charset=utf-8"); return; }
        std::string err_code;
        bool ok = ctl.ToggleLike(solution_id, current_user, &result, &err_code);

        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            if (err_code == "SOLUTION_NOT_FOUND")
            {
                rep.status = 404;
            }
            else
            {
                rep.status = 500;
            }
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Post(R"(/api/solutions/(\d+)/favorite$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value result;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            result["success"] = false;
            result["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(result), "application/json;charset=utf-8");
            return;
        }

        long long solution_id = 0;
        try { solution_id = std::stoll(req.matches[1]); } catch (...) { result["success"] = false; result["error_code"] = "INVALID_ID"; Json::FastWriter writer; addCORSHeaders(rep); rep.status = 400; rep.set_content(writer.write(result), "application/json;charset=utf-8"); return; }
        std::string err_code;
        bool ok = ctl.ToggleFavorite(solution_id, current_user, &result, &err_code);

        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            if (err_code == "SOLUTION_NOT_FOUND")
            {
                rep.status = 404;
            }
            else
            {
                rep.status = 500;
            }
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    // Comments: like, favorite and batch actions
    svr.Post(R"(/api/comments/(\d+)/like$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value result;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            result["success"] = false;
            result["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(result), "application/json;charset=utf-8");
            return;
        }

        unsigned long long comment_id = 0;
        try { comment_id = std::stoull(req.matches[1]); } catch (...) { result["success"] = false; result["error_code"] = "INVALID_ID"; Json::FastWriter writer; addCORSHeaders(rep); rep.status = 400; rep.set_content(writer.write(result), "application/json;charset=utf-8"); return; }
        std::string err_code;
        bool ok = ctl.ToggleCommentLike(comment_id, current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            if (err_code == "COMMENT_NOT_FOUND") {
                rep.status = 404;
            } else {
                rep.status = 500;
            }
        }
        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Post(R"(/api/comments/(\d+)/favorite$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value result;
        User current_user;
        if (!getCurrentUser(req, &current_user))
        {
            result["success"] = false;
            result["error_code"] = "UNAUTHORIZED";
            Json::FastWriter writer;
            addCORSHeaders(rep);
            rep.status = 401;
            rep.set_content(writer.write(result), "application/json;charset=utf-8");
            return;
        }

        unsigned long long comment_id = 0;
        try { comment_id = std::stoull(req.matches[1]); } catch (...) { result["success"] = false; result["error_code"] = "INVALID_ID"; Json::FastWriter writer; addCORSHeaders(rep); rep.status = 400; rep.set_content(writer.write(result), "application/json;charset=utf-8"); return; }
        std::string err_code;
        bool ok = ctl.ToggleCommentFavorite(comment_id, current_user, &result, &err_code);
        if (!ok)
        {
            result["success"] = false;
            result["error_code"] = err_code;
            if (err_code == "COMMENT_NOT_FOUND") {
                rep.status = 404;
            } else {
                rep.status = 500;
            }
        }
        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Get(R"(/api/comments/actions$)", [&ctl, &addCORSHeaders, &getCurrentUser](const Request& req, Response& rep) {
        Json::Value result;
        std::string ids_param = req.has_param("ids") ? req.get_param_value("ids") : "";
        std::vector<unsigned long long> ids;
        if (!ids_param.empty()) {
            std::stringstream ss(ids_param);
            std::string token;
            while (std::getline(ss, token, ',')) {
                try {
                    unsigned long long id = std::stoull(token);
                    ids.push_back(id);
                } catch (...) {
                    // skip invalid entries
                }
            }
        }
        // Check login to decide default values
        User current_user;
        bool logged_in = getCurrentUser(req, &current_user);
        Json::Value payload;
        if (logged_in) {
            ctl.GetCommentActions(ids, current_user.uid, &payload);
        } else {
            payload["success"] = true;
            Json::Value actions_json(Json::objectValue);
            for (const auto& id : ids) {
                Json::Value item(Json::objectValue);
                item["like"] = false;
                item["favorite"] = false;
                actions_json[std::to_string(id)] = item;
            }
            payload["actions"] = actions_json;
        }
        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(payload), "application/json;charset=utf-8");
    });

    // Preflight for new comment routes
    svr.Options(R"(/api/comments/(\d+)/like$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req; addCORSHeaders(rep); rep.status = 200; });
    svr.Options(R"(/api/comments/(\d+)/favorite$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req; addCORSHeaders(rep); rep.status = 200; });

    svr.Get(R"(/api/solutions/(\d+)/actions$)", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        long long solution_id = 0;
        try { solution_id = std::stoll(req.matches[1]); } catch (...) { HttpUtil::ReplyBadRequest(rep, "无效的题解ID"); return; }

        Json::Value result;
        User current_user;
        bool logged_in = getCurrentUser(req, &current_user);

        if (logged_in)
        {
            ctl.GetUserSolutionActions(solution_id, current_user.uid, &result);
            result["success"] = true;
        }
        else
        {
            result["success"] = true;
            result["liked"] = false;
            result["favorited"] = false;
        }

        Json::FastWriter writer;
        addCORSHeaders(rep);
        rep.set_content(writer.write(result), "application/json;charset=utf-8");
    });

    svr.Options(R"(/api/solutions/(\d+)/like$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options(R"(/api/solutions/(\d+)/favorite$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options(R"(/api/solutions/(\d+)$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options(R"(/api/solutions/(\d+)/actions$)", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/questions/cache/invalidate", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/static/cache/invalidate", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.Options("/api/*", [&addCORSHeaders](const Request& req, Response& rep) {
        (void)req;
        addCORSHeaders(rep);
        rep.status = 200;
    });

    svr.listen("0.0.0.0", 8080);
    return 0;
}
