#pragma once

#include <string>
#include <map>
#include <mutex>
#include <time.h>
#include <random>
#include <unordered_map>
#include <sw/redis++/redis++.h>
#include "../../comm/comm.hpp"
#include <Logger/logstrategy.h>

namespace ns_session
{
    using namespace sw::redis;
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
        static constexpr const char* kSessionPrefix = "oj:prod:v1:session:user:";

        std::map<std::string, Session> _sessions;
        std::mutex _mtx;
        std::string _secret_key;
        Redis _redis;

        std::string BuildSessionKey(const std::string& session_id)
        {
            return std::string(kSessionPrefix) + session_id;
        }

        bool SetSessionToRedis(const Session& sess)
        {
            std::unordered_map<std::string, std::string> fields;
            fields["session_id"] = sess.session_id;
            fields["user_id"] = std::to_string(sess.user_id);
            fields["email"] = sess.email;
            fields["create_time"] = std::to_string(static_cast<long long>(sess.create_time));
            fields["last_access_time"] = std::to_string(static_cast<long long>(sess.last_access_time));

            std::string key = BuildSessionKey(sess.session_id);
            _redis.hmset(key, fields.begin(), fields.end());
            _redis.expire(key, SESSION_EXPIRE_SECONDS);
            return true;
        }

        bool GetSessionFromRedis(const std::string& session_id, Session* session)
        {
            std::string key = BuildSessionKey(session_id);
            std::unordered_map<std::string, std::string> fields;
            _redis.hgetall(key, std::inserter(fields, fields.begin()));
            if (fields.empty())
            {
                return false;
            }

            session->session_id = session_id;
            session->user_id = std::stoi(fields["user_id"]);
            session->email = fields["email"];
            session->create_time = static_cast<time_t>(std::stoll(fields["create_time"]));
            session->last_access_time = static_cast<time_t>(std::stoll(fields["last_access_time"]));

            // 滑动续期
            session->last_access_time = time(nullptr);
            _redis.hset(key, "last_access_time", std::to_string(static_cast<long long>(session->last_access_time)));
            _redis.expire(key, SESSION_EXPIRE_SECONDS);
            return true;
        }

        void DeleteSessionFromRedis(const std::string& session_id)
        {
            _redis.del(BuildSessionKey(session_id));
        }

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
        SessionManager() : _secret_key("oj_secret_key_2026"), _redis(redis_addr)
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
            
            try
            {
                SetSessionToRedis(sess);
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis session write failed, fallback to local map: " << e.what();
                _sessions[session_id] = sess;
            }
            
            logger(INFO) << "创建 Session: " << session_id << " for user: " << email;
            return session_id;
        }

        //通过 session_id 获取 session 信息，成功返回 true 并将 session 信息写入 session 参数，失败返回 false
        bool GetSession(const std::string& session_id, Session* session)
        {
            std::lock_guard<std::mutex> lock(_mtx);

            if (session == nullptr)
            {
                return false;
            }

            try
            {
                //优先尝试从 Redis 获取 session 信息，如果 Redis 出现错误则回退到本地 map 获取
                if (GetSessionFromRedis(session_id, session))
                {
                    return true;
                }
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis session read failed, fallback to local map: " << e.what();
            }

            auto it = _sessions.find(session_id);
            if (it == _sessions.end())
            {
                //本地 map 中也没有找到 session 信息，返回 false
                return false;
            }

            time_t now = time(nullptr);
            if (now - it->second.last_access_time > SESSION_EXPIRE_SECONDS)
            {
                //session 已过期，从 Redis 和本地 map 中删除 session 信息，返回 false
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
            try
            {
                DeleteSessionFromRedis(session_id);
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis session delete failed: " << e.what();
            }

            _sessions.erase(session_id); // fallback map cleanup
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

        // 解析 Cookie 头，提取 session_id
        bool ParseCookie(const std::string& cookie_header, std::string* session_id)
        {
            if (cookie_header.empty() || session_id == nullptr)
            {
                return false;
            }

            size_t start = 0;
            //示例cookie: "other_cookie=abc; oj_session_id=12345; another_cookie=xyz"
            while (start < cookie_header.size())
            {
                size_t end = cookie_header.find(';', start);
                std::string token = (end == std::string::npos)
                    ? cookie_header.substr(start)
                    : cookie_header.substr(start, end - start);

                //去掉;两边的空格
                size_t left = token.find_first_not_of(' ');
                if (left == std::string::npos)
                {
                    if (end == std::string::npos) break;
                    start = end + 1;
                    continue;
                }
                size_t right = token.find_last_not_of(' ');
                token = token.substr(left, right - left + 1);
                //解析出name=value
                size_t eq_pos = token.find('=');
                if (eq_pos != std::string::npos)
                {
                    std::string name = token.substr(0, eq_pos);
                    std::string value = token.substr(eq_pos + 1);
                    if (name == SESSION_COOKIE_NAME)
                    {
                        //找到了oj_session_id
                        *session_id = value;
                        return true;
                    }
                }

                if (end == std::string::npos) break;
                start = end + 1;
            }

            return false;
        }
    };
}
