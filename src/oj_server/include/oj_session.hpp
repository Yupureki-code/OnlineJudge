#pragma once

#include <string>
#include <map>
#include <mutex>
#include <time.h>
#include <array>
#include <charconv>
#include <unordered_map>
#include <openssl/rand.h>
#include <sw/redis++/redis++.h>
#include "../../comm/comm.hpp"
#include "../../comm/logger.hpp"

namespace oj::session
{
    using namespace sw::redis;
    using namespace oj::util;
    using namespace oj::logger;
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

        template <typename Integer>
        static bool ParseInteger(const std::string& encoded, Integer* value)
        {
            if (value == nullptr || encoded.empty())
            {
                return false;
            }
            Integer parsed{};
            const char* begin = encoded.data();
            const char* end = begin + encoded.size();
            const auto result = std::from_chars(begin, end, parsed);
            if (result.ec != std::errc{} || result.ptr != end)
            {
                return false;
            }
            *value = parsed;
            return true;
        }

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

            const auto stored_id = fields.find("session_id");
            const auto user_id = fields.find("user_id");
            const auto email = fields.find("email");
            const auto create_time = fields.find("create_time");
            const auto last_access_time = fields.find("last_access_time");
            long long parsed_create_time = 0;
            long long parsed_last_access_time = 0;
            int parsed_user_id = 0;
            if (fields.size() != 5 || stored_id == fields.end() || user_id == fields.end() ||
                email == fields.end() || create_time == fields.end() || last_access_time == fields.end() ||
                stored_id->second != session_id || email->second.empty() || email->second.size() > 320 ||
                !ParseInteger(user_id->second, &parsed_user_id) || parsed_user_id <= 0 ||
                !ParseInteger(create_time->second, &parsed_create_time) || parsed_create_time <= 0 ||
                !ParseInteger(last_access_time->second, &parsed_last_access_time) ||
                parsed_last_access_time < parsed_create_time)
            {
                LOG_WARNING("{}", "Invalid Redis user session hash rejected");
                return false;
            }

            session->session_id = session_id;
            session->user_id = parsed_user_id;
            session->email = email->second;
            session->create_time = static_cast<time_t>(parsed_create_time);
            session->last_access_time = static_cast<time_t>(parsed_last_access_time);

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
            std::array<unsigned char, 32> bytes{};
            if (RAND_bytes(bytes.data(), bytes.size()) != 1)
            {
                LOG_ERROR("{}", "CSPRNG failed while creating user session");
                return {};
            }

            static constexpr char hex[] = "0123456789abcdef";
            std::string session_id(bytes.size() * 2, '0');
            for (size_t i = 0; i < bytes.size(); ++i)
            {
                session_id[i * 2] = hex[bytes[i] >> 4];
                session_id[i * 2 + 1] = hex[bytes[i] & 0x0f];
            }
            return session_id;
        }

    public:
        explicit SessionManager(const std::string& redis_address = redis_addr)
            : _secret_key("oj_secret_key_2026"), _redis(redis_address)
        {
        }

        std::string CreateSession(int user_id, const std::string& email)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            
            std::string session_id = GenerateSessionId();
            if (session_id.empty())
            {
                return {};
            }
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
                LOG_ERROR("{}{}", "Redis session write failed, fallback to local map: ", e.what());
                _sessions[session_id] = sess;
            }
            
            LOG_INFO("Session created for user_id={}", user_id);
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
                // Redis 正常 miss 是权威撤销；只有 Redis 不可用时才允许本地降级。
                if (GetSessionFromRedis(session_id, session))
                {
                    return true;
                }
                _sessions.erase(session_id);
                return false;
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis session read failed, fallback to local map: ", e.what());
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
                LOG_INFO("Session expired");
                return false;
            }

            it->second.last_access_time = now;
            *session = it->second;
            return true;
        }

        bool DestroySession(const std::string& session_id)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            try
            {
                DeleteSessionFromRedis(session_id);
            }
            catch (const sw::redis::Error& e)
            {
                LOG_ERROR("{}{}", "Redis session delete failed: ", e.what());
                _sessions.erase(session_id);
                return false;
            }

            _sessions.erase(session_id); // fallback map cleanup
            LOG_INFO("Session destroyed");
            return true;
        }

        std::string GetCookieHeader(const std::string& session_id)
        {
            time_t expire = time(nullptr) + SESSION_EXPIRE_SECONDS;
            char expire_str[64];
            struct tm* tm_info = gmtime(&expire);
            strftime(expire_str, sizeof(expire_str), "%a, %d %b %Y %H:%M:%S GMT", tm_info);
            
            const bool secure = std::getenv("OJ_COOKIE_SECURE") != nullptr &&
                                std::string(std::getenv("OJ_COOKIE_SECURE")) == "true";
            return SESSION_COOKIE_NAME + "=" + session_id +
                   "; Expires=" + expire_str + 
                   "; Path=/" + 
                   "; HttpOnly" +
                   "; SameSite=Lax" +
                   (secure ? "; Secure" : "");
        }

        std::string GetClearCookieHeader()
        {
            const bool secure = std::getenv("OJ_COOKIE_SECURE") != nullptr &&
                                std::string(std::getenv("OJ_COOKIE_SECURE")) == "true";
            return SESSION_COOKIE_NAME + "=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/; HttpOnly; SameSite=Lax" +
                   std::string(secure ? "; Secure" : "");
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
