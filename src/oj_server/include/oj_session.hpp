#pragma once

#include <string>
#include <map>
#include <mutex>
#include <time.h>
#include <random>
#include "../../comm/comm.hpp"
#include "../../comm/logstrategy.hpp"

namespace ns_session
{
    using namespace ns_log;
    using namespace ns_util;

    const std::string SESSION_COOKIE_NAME = "oj_session_id";
    const int SESSION_EXPIRE_SECONDS = 7 * 24 * 60 * 60;

    struct Session
    {
        std::string session_id;
        int user_id;
        std::string email;
        time_t create_time;
        time_t last_access_time;
    };

    class SessionManager
    {
    private:
        std::map<std::string, Session> _sessions;
        std::mutex _mtx;
        std::string _secret_key;

        std::string GenerateSessionId()
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 15);
            
            std::string session_id;
            const char* hex = "0123456789abcdef";
            for (int i = 0; i < 32; i++) {
                session_id += hex[dis(gen)];
            }
            return session_id;
        }

    public:
        SessionManager() : _secret_key("oj_secret_key_2026")
        {
        }

        std::string CreateSession(int user_id, const std::string& email)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            
            std::string session_id = GenerateSessionId();
            Session sess;
            sess.session_id = session_id;
            sess.user_id = user_id;
            sess.email = email;
            sess.create_time = time(nullptr);
            sess.last_access_time = time(nullptr);
            
            _sessions[session_id] = sess;
            
            logger(INFO) << "创建 Session: " << session_id << " for user: " << email;
            return session_id;
        }

        bool GetSession(const std::string& session_id, Session* session)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            
            auto it = _sessions.find(session_id);
            if (it == _sessions.end())
            {
                return false;
            }

            time_t now = time(nullptr);
            if (now - it->second.last_access_time > SESSION_EXPIRE_SECONDS)
            {
                _sessions.erase(it);
                logger(INFO) << "Session 过期: " << session_id;
                return false;
            }

            it->second.last_access_time = now;
            *session = it->second;
            return true;
        }

        void DestroySession(const std::string& session_id)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            _sessions.erase(session_id);
            logger(INFO) << "销毁 Session: " << session_id;
        }

        std::string GetCookieHeader(const std::string& session_id)
        {
            time_t expire = time(nullptr) + SESSION_EXPIRE_SECONDS;
            char expire_str[64];
            struct tm* tm_info = gmtime(&expire);
            strftime(expire_str, sizeof(expire_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
            
            return SESSION_COOKIE_NAME + "=" + session_id + 
                   "; Expires=" + expire_str + 
                   "; Path=/" + 
                   "; HttpOnly" +
                   "; SameSite=Lax";
        }

        std::string GetClearCookieHeader()
        {
            return SESSION_COOKIE_NAME + "=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/";
        }

        bool ParseCookie(const std::string& cookie_header, std::string* session_id)
        {
            if (cookie_header.empty())
                return false;

            std::string cookies = cookie_header;
            size_t pos = 0;
            
            while ((pos = cookies.find(";")) != std::string::npos)
            {
                std::string cookie = cookies.substr(0, pos);
                size_t eq_pos = cookie.find("=");
                if (eq_pos != std::string::npos)
                {
                    std::string name = cookie.substr(0, eq_pos);
                    std::string value = cookie.substr(eq_pos + 1);
                    
                    while (name.size() > 0 && name[0] == ' ')
                        name = name.substr(1);
                    
                    if (name == SESSION_COOKIE_NAME)
                    {
                        *session_id = value;
                        return true;
                    }
                }
                
                if (pos + 2 < cookies.size())
                    cookies = cookies.substr(pos + 2);
                else
                    break;
            }

            size_t eq_pos = cookies.find("=");
            if (eq_pos != std::string::npos)
            {
                std::string name = cookies.substr(0, eq_pos);
                std::string value = cookies.substr(eq_pos + 1);
                
                while (name.size() > 0 && name[0] == ' ')
                    name = name.substr(1);
                
                if (name == SESSION_COOKIE_NAME)
                {
                    *session_id = value;
                    return true;
                }
            }
            
            return false;
        }
    };
}
