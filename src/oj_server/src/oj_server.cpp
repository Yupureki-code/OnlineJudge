#include <httplib.h>
#include "../include/oj_control.hpp"
#include <string>
#include <fstream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <chrono>

using namespace httplib;
using namespace ns_log;

namespace {

std::string Trim(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    {
        ++start;
    }

    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
    {
        --end;
    }

    return s.substr(start, end - start);
}

bool IsValidEmail(const std::string& email)
{
    static const std::regex kEmailPattern(R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(email, kEmailPattern);
}

bool ParseJsonBody(const Request& req, Json::Value* out)
{
    if (out == nullptr || req.body.empty())
    {
        return false;
    }

    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream ss(req.body);
    return Json::parseFromStream(builder, ss, out, &errs);
}

bool IsSafeStaticPageName(const std::string& page_name)
{
    if (page_name.empty())
    {
        return false;
    }
    if (page_name.find("..") != std::string::npos)
    {
        return false;
    }
    if (page_name.front() == '/' || page_name.front() == '\\')
    {
        return false;
    }
    return true;
}

bool IsValidAuthCode(const std::string& code)
{
    if (code.size() != 6)
    {
        return false;
    }
    return std::all_of(code.begin(), code.end(), [](unsigned char ch){ return std::isdigit(ch); });
}

void ReplyBadRequest(Response& rep, const std::string& message)
{
    Json::Value response;
    response["success"] = false;
    response["error"] = message;
    Json::FastWriter writer;
    rep.status = 400;
    rep.set_content(writer.write(response), "application/json;charset=utf-8");
}

bool ExtractAndValidateEmail(const Json::Value& in_value, std::string* email)
{
    if (email == nullptr || !in_value.isMember("email") || !in_value["email"].isString())
    {
        return false;
    }

    *email = Trim(in_value["email"].asString());
    if (email->empty() || !IsValidEmail(*email))
    {
        return false;
    }
    return true;
}

bool ExtractAndValidatePassword(const Json::Value& in_value, std::string* password)
{
    if (password == nullptr || !in_value.isMember("password") || !in_value["password"].isString())
    {
        return false;
    }

    *password = in_value["password"].asString();
    if (password->empty())
    {
        return false;
    }
    return true;
}

int ParsePositiveIntParam(const Request& req, const std::string& key, int default_value, int min_value, int max_value)
{
    if (!req.has_param(key))
    {
        return default_value;
    }

    try
    {
        int value = std::stoi(req.get_param_value(key));
        if (value < min_value) return min_value;
        if (value > max_value) return max_value;
        return value;
    }
    catch (...)
    {
        return default_value;
    }
}

ns_cache::QueryStruct ParseQuestionQuery(const Request& req)
{
    ns_cache::QueryStruct query;
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
                    query.id = Trim(parsed["id"].asString());
                }
                if (parsed.isMember("title") && parsed["title"].isString())
                {
                    query.title = Trim(parsed["title"].asString());
                }
                if (parsed.isMember("difficulty") && parsed["difficulty"].isString())
                {
                    query.difficulty = Trim(parsed["difficulty"].asString());
                }
                if (parsed.isMember("query_mode") && parsed["query_mode"].isString())
                {
                    query_mode = Trim(parsed["query_mode"].asString());
                }
                if (parsed.isMember("keyword") && parsed["keyword"].isString())
                {
                    keyword = Trim(parsed["keyword"].asString());
                }
            }
            else
            {
                query.title = Trim(query_hash);
            }
        }
    }

    if (req.has_param("query_mode"))
    {
        query_mode = Trim(req.get_param_value("query_mode"));
    }

    if (req.has_param("q"))
    {
        keyword = Trim(req.get_param_value("q"));
    }

    if (req.has_param("id"))
    {
        query.id = Trim(req.get_param_value("id"));
    }
    if (req.has_param("title"))
    {
        query.title = Trim(req.get_param_value("title"));
    }
    if (req.has_param("difficulty"))
    {
        query.difficulty = Trim(req.get_param_value("difficulty"));
    }

    if (query_mode.empty())
    {
        query_mode = "title";
    }

    if (!keyword.empty())
    {
        if (query_mode == "id")
        {
            query.id = keyword;
            query.title.clear();
        }
        else if (query_mode == "both")
        {
            query.title = keyword;
            if (std::all_of(keyword.begin(), keyword.end(), [](unsigned char ch){ return std::isdigit(ch); }))
            {
                query.id = keyword;
            }
            else
            {
                query.id.clear();
            }
        }
        else
        {
            query.title = keyword;
            query.id.clear();
        }
    }

    return query;
}

} // namespace

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
        rep.set_header("Access-Control-Allow-Methods", "POST, GET, DELETE, OPTIONS");
        rep.set_header("Access-Control-Allow-Headers", "Content-Type");
    };

    auto getCurrentUser = [&ctl](const Request& req, ns_control::User* user) -> bool {
        std::string cookie_header = req.get_header_value("Cookie");
        return ctl.GetSessionUser(cookie_header, user);
    };

    svr.set_mount_point("/pictures", HTML_PATH + std::string("/pictures"));
    svr.set_mount_point("/css", HTML_PATH + std::string("/css"));
    svr.set_mount_point("/spa", HTML_PATH + std::string("/spa"));

    svr.Get("/js/(.*)", [](const Request& req, Response& rep){
        std::string file = req.matches[1];
        std::string content = ReadHtmlFile(HTML_PATH + std::string("/js/") + file);
        rep.set_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        rep.set_header("Pragma", "no-cache");
        rep.set_header("Expires", "0");
        rep.set_content(content, "application/javascript;charset=utf-8");
    });

    svr.Get("/", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("index.html", &html);
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get("/all_questions", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        auto begin = std::chrono::steady_clock::now();
        int page = ParsePositiveIntParam(req, "page", 1, 1, 1000000);
        int size = ParsePositiveIntParam(req, "size", 5, 1, 100);
        ns_cache::QueryStruct query_hash = ParseQuestionQuery(req);

        std::string html;
        ctl.AllQuestions(&html, page, size, query_hash);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html,"text/html;charset=utf-8");
        auto end = std::chrono::steady_clock::now();
        long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        logger(INFO) << "[metrics][route] GET /all_questions page=" << page
                     << " size=" << size
                     << " cost_ms=" << cost_ms;
    });

    svr.Get("/about", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("about.html" , &html);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get("/user/profile", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("user/profile.html", &html);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get("/user/settings", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("user/settings.html", &html);

        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);

        html = InjectUserInfo(html, &user, isLoggedIn);

        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get(R"(/questions/(\d+)$)", [&ctl, &getCurrentUser](const Request& req,Response& rep){
        auto begin = std::chrono::steady_clock::now();
        std::string number = req.matches[1];
        std::string html;
        ctl.OneQuestion(number, &html);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html,"text/html;charset=utf-8");
        auto end = std::chrono::steady_clock::now();
        long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        logger(INFO) << "[metrics][route] GET /questions/" << number << " cost_ms=" << cost_ms;
    }); 

    svr.Get(R"(/judge_result\.html)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("judge_result.html", &html);
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        html = InjectUserInfo(html, &user, isLoggedIn);
        
        rep.set_content(html, "text/html;charset=utf-8");
    });

    svr.Get(R"(/one_question\.html)", [&ctl, &getCurrentUser](const Request& req, Response& rep){
        std::string html;
        ctl.GetStaticHtml("one_question.html", &html);
        
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

    svr.Post("/api/auth/send_code", [&ctl, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value in_value;
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码不能为空");
            return;
        }

        std::string code = Trim(in_value["code"].asString());
        if (!IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string name;
        if (in_value.isMember("name") && in_value["name"].isString())
        {
            name = Trim(in_value["name"].asString());
        }

        std::string password;
        if (in_value.isMember("password") && in_value["password"].isString())
        {
            password = in_value["password"].asString();
        }

        ns_control::User user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string name;
        if (!in_value.isMember("name") || !in_value["name"].isString())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "用户名不能为空");
            return;
        }
        name = Trim(in_value["name"].asString());
        if (name.empty() || name.size() > 64)
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "用户名长度应为1-64个字符");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        logger(INFO)<<"创建新用户 name:"<<name<<" email:"<<email;
        Json::Value response;
        response["success"] = true;
        ctl.CreateUser(name, email, response);
        
        if (response["created"].asBool())
        {
            ns_control::User user;
            if (ctl.GetUser(email, static_cast<ns_control::User*>(&user)))
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "无效的JSON请求体");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }
        
        Json::Value response;
        response["success"] = true;
        bool found = ctl.CheckUser(email, response);
        
        if (found && response["exists"].asBool())
        {
            ns_control::User user;
            if (ctl.GetUser(email, static_cast<ns_control::User*>(&user)))
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
        ns_control::User user;
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
        ns_control::User current_user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string password;
        if (!ExtractAndValidatePassword(in_value, &password))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "密码不能为空");
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::string email;
        if (!ExtractAndValidateEmail(in_value, &email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "邮箱格式不合法");
            return;
        }

        std::string password;
        if (!ExtractAndValidatePassword(in_value, &password))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "密码不能为空");
            return;
        }

        ns_control::User user;
        std::string err_code;
        bool ok = ctl.LoginWithPassword(email, password, &user, &err_code);

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
        ns_control::User current_user;
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
        ns_control::User current_user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码不能为空");
            return;
        }

        std::string code = Trim(in_value["code"].asString());
        if (!IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string new_email;
        if (!ExtractAndValidateEmail(in_value, &new_email))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "新邮箱格式不合法");
            return;
        }

        ns_control::User updated_user;
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
        ns_control::User current_user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码不能为空");
            return;
        }
        std::string code = Trim(in_value["code"].asString());
        if (!IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码格式不合法");
            return;
        }

        std::string new_password;
        if (!in_value.isMember("new_password") || !in_value["new_password"].isString())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "新密码不能为空");
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
        ns_control::User current_user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        if (!in_value.isMember("code") || !in_value["code"].isString())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码不能为空");
            return;
        }

        std::string code = Trim(in_value["code"].asString());
        if (!IsValidAuthCode(code))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "验证码格式不合法");
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
        ns_model::Question q;
        
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

    svr.Post("/api/questions", [&ctl, &getCurrentUser, &addCORSHeaders](const Request& req, Response& rep) {
        Json::Value response;
        ns_control::User user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
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
            ReplyBadRequest(rep, "缺少必要字段(number/title/desc/star/cpu_limit/memory_limit)");
            return;
        }

        ns_model::Question q;
        q.number = Trim(in_value["number"].asString());
        q.title = Trim(in_value["title"].asString());
        q.desc = in_value["desc"].asString();
        q.star = Trim(in_value["star"].asString());
        q.cpu_limit = in_value["cpu_limit"].asInt();
        q.memory_limit = in_value["memory_limit"].asInt();

        if (q.number.empty() || q.title.empty())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "number/title 不能为空");
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
        ns_control::User user;
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

        ns_control::User user;
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

        ns_control::User user;
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
        if (!ParseJsonBody(req, &in_value))
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "请求体必须是 JSON");
            return;
        }

        std::vector<std::string> pages;
        if (in_value.isMember("page") && in_value["page"].isString())
        {
            pages.push_back(Trim(in_value["page"].asString()));
        }
        if (in_value.isMember("pages") && in_value["pages"].isArray())
        {
            for (const auto& item : in_value["pages"])
            {
                if (item.isString())
                {
                    pages.push_back(Trim(item.asString()));
                }
            }
        }

        if (pages.empty())
        {
            addCORSHeaders(rep);
            ReplyBadRequest(rep, "缺少 page 或 pages 字段");
            return;
        }

        Json::Value invalidated(Json::arrayValue);
        for (const auto& page : pages)
        {
            if (!IsSafeStaticPageName(page))
            {
                addCORSHeaders(rep);
                ReplyBadRequest(rep, "页面路径不合法");
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
        
        ns_control::User user;
        bool isLoggedIn = getCurrentUser(req, &user);
        
        if (isLoggedIn) {
            response["success"] = true;
            response["user"]["uid"] = user.uid;
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
