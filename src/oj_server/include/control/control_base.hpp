#pragma once

#include <cstdio>
#include <assert.h>
#include <string>
#include <vector>
#include <mutex>
#include <assert.h>
#include <memory>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include "../model/oj_model.hpp"
#include "../oj_view.hpp"
#include "../oj_session.hpp"
#include "../oj_mail.hpp"
#include <jsoncpp/json/json.h>
#include <sw/redis++/redis++.h>
#include <openssl/sha.h>
#include "../../../comm/comm.hpp"
#include "../../../comm/models/user.hxx"

// ControlBase: 基础设施，所有controller层的公共基类 — 共享成员、静态页面、session管理
namespace oj::control
{
    using namespace oj::model;
    using namespace oj::util;
    using namespace oj::view;
    using namespace oj::session;
    using oj::db::User;

    struct ActionState
    {
        bool liked = false;
        bool favorited = false;
        uint64_t like_count = 0;
        uint64_t favorite_count = 0;
    };

    class ControlBase
    {
    public:
        ControlBase() : _auth_redis(redis_addr)
        {}

        Model* GetModel() { return &_model; }

        //获取静态页面，优先级：redis缓存 > view内存缓存 > 磁盘文件
        bool GetStaticHtml(const std::string& path, std::string* html)
        {
            //用户个性主页不应该缓存；后台壳页可缓存，用户信息在注入阶段单独动态拼接。
            const bool disable_cache = (path == "user/profile.html" || path == "user/settings.html");
            if (disable_cache)
            {
                bool view_cache_hit = false;
                bool ok = _view.GetStaticHtml(path, html, &view_cache_hit, true);
                if (ok)
                {
                    LOG_INFO("{}{}{}", "[html_cache][static] bypass=1 page=", path, " source=disk");
                }
                else 
                {
                    LOG_CRITICAL("{}{}{}", "[html_cache][static] bypass=0 page=", path, " source=disk");
                }
                return ok;
            }
            //其他页面正常使用缓存
            auto html_key = _model.Question().BuildHtmlCacheKey(path, Cache::CacheKey::PageType::kHtml);
            if (_model.GetHtmlPage(html, html_key))
            {
                _model.RecordCacheMetrics(ModelBase::RecordActionType::Question, true, false, 0);
                LOG_INFO("{}{}{}", "[html_cache][static] hit=1 page=", path, " source=redis");
                return true;
            }

            _model.RecordCacheMetrics(ModelBase::RecordActionType::Question, false, false, 0);
            bool view_cache_hit = false;
            bool ok = _view.GetStaticHtml(path, html, &view_cache_hit);
            if (ok)
            {
                _model.SetHtmlPage(html, html_key);
                LOG_INFO("{}{}{}{}", "[html_cache][static] hit=0 page=", path, " source=", (view_cache_hit ? "view_mem" : "disk"));
            }
            return ok;
        }

        bool InvalidateStaticHtmlCache(const std::string& path)
        {
            if (path.empty())
            {
                return false;
            }
            auto html_key = _model.Question().BuildHtmlCacheKey(path, Cache::CacheKey::PageType::kHtml);
            _model.InvalidatePage(html_key);
            LOG_INFO("{}{}", "[cache] static html invalidated page=", path);
            return true;
        }

        std::string CreateSession(int user_id, const std::string& email)
        {
            return _session_manager.CreateSession(user_id, email);
        }

        bool DestroySession(const std::string& session_id)
        {
            return _session_manager.DestroySession(session_id);
        }

        bool DestroySessionByCookieHeader(const std::string& cookie_header)
        {
            std::string session_id;
            if (_session_manager.ParseCookie(cookie_header, &session_id))
            {
                return _session_manager.DestroySession(session_id);
            }
            return true;
        }

        //通过session获取用户信息，失败返回false，成功返回true并将用户信息写入user参数
        bool GetSessionUser(const std::string& cookie_header, User* user)
        {
            std::string session_id;
            //从cookie_header中解析出session_id，如果解析失败返回false
            if (!_session_manager.ParseCookie(cookie_header, &session_id))
            {
                return false;
            }

            Session session;
            //通过session_id获取session信息，如果获取失败返回false
            if (!_session_manager.GetSession(session_id, &session))
            {
                return false;
            }
            //通过session中的user_id在mysql中获取用户信息，如果获取失败返回false，成功返回true并将用户信息写入user参数
            return _model.User().GetUserById(session.user_id, user);
        }

        std::string GetSetCookieHeader(const std::string& session_id)
        {
            return _session_manager.GetCookieHeader(session_id);
        }

        std::string GetClearCookieHeader()
        {
            return _session_manager.GetClearCookieHeader();
        }

        // 按用户ID在avatars目录下查找头像文件，仅依赖文件系统，不依赖DB的avatar_path
        std::string GetEffectiveAvatarUrl(const User& author) const
        {
            static const char* kExts[] = {".jpg", ".png", ".gif", ".webp"};
            for (const char* ext : kExts)
            {
                std::string path = std::string(HTML_PATH) + "pictures/avatars/" + std::to_string(author.uid) + ext;
                struct stat st;
                if (stat(path.c_str(), &st) == 0)
                    return std::string("/pictures/avatars/") + std::to_string(author.uid) + ext;
            }
            return "/pictures/head.jpg";
        }

    protected:
        bool GetPublicSolution(long long solution_id, Solution* solution, std::string* err_code)
        {
            if (solution == nullptr || err_code == nullptr)
            {
                return false;
            }

            if (!_model.Solution().GetSolutionById(solution_id, solution) ||
                solution->status != "approved")
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            Question question;
            if (!_model.Question().GetOneQuestion(solution->question_id, question))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }
            return true;
        }

        bool ToggleSolutionAction(long long solution_id,
                                  const User& current_user,
                                  const std::string& action_type,
                                  ActionState* result,
                                  std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            if (current_user.uid <= 0)
            {
                *err_code = "UNAUTHORIZED";
                return false;
            }

            Solution solution;
            if (!GetPublicSolution(solution_id, &solution, err_code)) return false;

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.Solution().ToggleSolutionAction(solution_id, current_user.uid, action_type, &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            if (action_type == "like")
            {
                result->liked = now_active;
                result->like_count = new_count;
            }
            else
            {
                result->favorited = now_active;
                result->favorite_count = new_count;
            }
            return true;
        }

        Model _model;
        View _view;
        SessionManager _session_manager;
        sw::redis::Redis _auth_redis;
        oj::mail::Mail _mail;
    };
}
