#pragma once

#include <Logger/logstrategy.h>
#include <cstdio>
#include <assert.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <assert.h>
#include <memory>
#include <random>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <array>
#include <iomanip>
#include <sys/stat.h>
#include "oj_model.hpp"
#include "oj_view.hpp"
#include "oj_session.hpp"
#include "oj_mail.hpp"
#include <httplib.h>
#include <jsoncpp/json/json.h>
#include <sw/redis++/redis++.h>
#include <openssl/sha.h>
#include "../../comm/comm.hpp"

// ControlBase: 基础设施，所有controller层的公共基类 — 共享成员、Judge、静态页面、session管理
namespace ns_control
{
    using namespace ns_model;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_view;
    using namespace ns_session;

    //表示后端编译服务器的类
    class Machine
    {
    public:
        Machine(std::string ip,uint16_t port)
        :_ip(ip),_port(port),_mtx(std::make_shared<std::mutex>())
        {}
        void Inload()
        {
            _mtx->lock();
            _load++;
            _mtx->unlock();
        }
        void Deload()
        {
            _mtx->lock();
            _load--;
            _mtx->unlock();
        }
        void ResetLoad()
        {
            _mtx->lock();
            _load = 0;
            _mtx->unlock();
        }
        uint32_t Load()
        {
            int load;
            _mtx->lock();
            load = _load;
            _mtx->unlock();
            return load;
        }
        std::string Ip()
        {
            return _ip;
        }
        uint16_t Port()
        {
            return _port;
        }
        ~Machine()
        {}
    private:
        std::string _ip;  //编译服务器的ip地址
        uint16_t _port;   //编译服务器的port端口
        uint32_t _load = 0;  //编译服务器的负载
        std::shared_ptr<std::mutex> _mtx; //互斥锁
    };

    //默认配置文件
    const std::string machine_conf = CONF_PATH + std::string("service_machine.conf");

    class CentralConsole
    {
    public:
        CentralConsole()
        {
            assert(LoadConf(machine_conf));
            logger(ns_log::INFO)<<"加载 "<<machine_conf<<" 成功";
        }
        //加载配置文件:配置编译服务器
        bool LoadConf(const std::string& conf)
        {
            logger(ns_log::DEBUG)<<"conf : "<<conf;
            std::ifstream in(conf);
            if(!in.is_open())
                return false;
            std::string line;
            std::string space = ":";
            while(std::getline(in,line))
            {
                std::vector<std::string> v;
                StringUtil::SplitString(line, space, &v);
                if(v.size() != 2)
                {
                    logger(ns_log::WARNING) <<"配置服务器 "<<line<<" 失败";
                    continue;
                }
                Machine machine(v[0],std::stoi(v[1]));
                _online.push_back(_machines.size());
                _machines.push_back(machine);
                logger(ns_log::INFO)<<"配置服务器 "<<v[0]<<":"<<v[1]<<" 成功";
            }
            in.close();
            return true;
        }
        //智能选择负载最小的编译服务器
        bool SmartChoose(int* id,Machine** m)
        {
            _mtx.lock();
            if(_online.size() == 0)
            {
                logger(ns_log::FATAL)<<"服务器全挂!";
                _mtx.unlock();
                return false;
            }
            *id = _online[0];
            *m = &_machines[*id];
            int load = _machines[*id].Load();
            for(auto & it : _online)
            {
                int cur = _machines[it].Load();
                if(cur < load) //比较负载
                {
                    load = cur;
                    *id = it;
                    *m = &_machines[*id];
                }
            }
            _mtx.unlock();
            return true;
        }
        //编译服务器下线
         void OfflineMachine(int which)
        {
            _mtx.lock();
            for(auto iter = _online.begin(); iter != _online.end(); iter++)
            {
                if(*iter == which)
                {
                    _machines[which].ResetLoad();
                    _online.erase(iter);
                    _offline.push_back(which);
                    break;
                }
            }
            _mtx.unlock();
        }
        //编译服务器上线
        void OnlineMachine()
        {
            _mtx.lock();
            _online.insert(_online.end(), _offline.begin(), _offline.end());
            _offline.erase(_offline.begin(), _offline.end());
            _mtx.unlock();
            logger(INFO) << "所有的主机都上线啦!" << "\n";
        }
        //DEBUG::显示目前的主机情况
        void ShowMachines()
        {
            _mtx.lock();
            std::cout<<"当前在线主机列表:"<<std::endl;
            for(auto& it : _online)
            {
                std::cout<<_machines[it].Ip()<<":"<<_machines[it].Port()<<" load:"<<_machines[it].Load();
            }
            std::cout<<"当前离线主机列表:"<<std::endl;
            for(auto& it : _offline)
            {
                std::cout<<_machines[it].Ip()<<":"<<_machines[it].Port()<<" load:"<<_machines[it].Load();
            }
            _mtx.unlock();
        }
    private:
        std::vector<Machine> _machines;  //表示全部主机的数组，下标代表编号
        std::vector<int> _online;  //上线的主机
        std::vector<int> _offline; //下线的主机
        std::mutex _mtx; //互斥锁
    };

    class ControlBase
    {
    public:
        ControlBase() : _auth_redis(redis_addr)
        {}

        Model* GetModel() { return &_model; }

        //判断题目正确，调用主机责任链
        // Forward optional custom_input/output to the compile server
        void Judge(const std::string& number,const std::string& in_json,std::string* out_json,
                   const std::string& custom_input = "",
                   const std::string& custom_output = "")
        {
            Question q;
            _model.GetOneQuestion(number, q);
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(in_json, in_value);
            std::string code = in_value["code"].asString();
            Json::Value compile_value;
            compile_value["id"] = number;
            compile_value["code"] = code;
            // Forward custom test input/output to the compile server when provided
            if (!custom_input.empty()) {
                compile_value["custom_input"] = custom_input;
                // Mark this as a custom test so the judge can skip normal judging
                compile_value["is_custom_test"] = true;  // ADD THIS LINE
            }
            if (!custom_output.empty()) {
                compile_value["custom_output"] = custom_output;
            }
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);
            while(true)
            {
                int id = 0;
                Machine *m = nullptr;
                if(!_console.SmartChoose(&id, &m))
                {
                    break;
                }
                httplib::Client cli(m->Ip(), m->Port());
                m->Inload();
                logger(INFO) << " 选择主机成功, 主机id: " << id << " 详情: " << m->Ip() << ":" << m->Port() << " 当前主机的负载是: " << m->Load() << "\n";
                if(auto res = cli.Post("/compile_and_run", compile_string, "application/json;charset=utf-8"))
                {
                    if(res->status == 200)
                    {
                        *out_json = res->body;
                        m->Deload();
                        logger(INFO) << "请求编译和运行服务成功..." << "\n";
                        break;
                    }
                    m->Deload();
                }
                else
                {
                    logger(ERROR) << " 当前请求的主机id: " << id << " 详情: " << m->Ip()<< ":" << m->Port() << " 可能已经离线"<< "\n";
                    _console.OfflineMachine(id);
                    _console.ShowMachines();
                }
            }
        }

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
                    logger(INFO) << "[html_cache][static] bypass=1 page=" << path << " source=disk";
                }
                else 
                {
                    logger(FATAL) << "[html_cache][static] bypass=0 page=" << path << " source=disk";
                }
                return ok;
            }
            //其他页面正常使用缓存
            auto html_key = _model.BuildHtmlCacheKey(path, Cache::CacheKey::PageType::kHtml);
            if (_model.GetHtmlPage(html, html_key))
            {
                _model.RecordStaticHtmlCacheMetrics(true);
                logger(INFO) << "[html_cache][static] hit=1 page=" << path << " source=redis";
                return true;
            }

            _model.RecordStaticHtmlCacheMetrics(false);
            bool view_cache_hit = false;
            bool ok = _view.GetStaticHtml(path, html, &view_cache_hit);
            if (ok)
            {
                _model.SetHtmlPage(html, html_key);
                logger(INFO) << "[html_cache][static] hit=0 page=" << path
                             << " source=" << (view_cache_hit ? "view_mem" : "disk");
            }
            return ok;
        }

        bool InvalidateStaticHtmlCache(const std::string& path)
        {
            if (path.empty())
            {
                return false;
            }
            auto html_key = _model.BuildHtmlCacheKey(path, Cache::CacheKey::PageType::kHtml);
            _model.InvalidatePage(html_key);
            logger(INFO) << "[cache] static html invalidated page=" << path;
            return true;
        }

        std::string CreateSession(int user_id, const std::string& email)
        {
            return _session_manager.CreateSession(user_id, email);
        }

        void DestroySession(const std::string& session_id)
        {
            _session_manager.DestroySession(session_id);
        }

        void DestroySessionByCookieHeader(const std::string& cookie_header)
        {
            std::string session_id;
            if (_session_manager.ParseCookie(cookie_header, &session_id))
            {
                _session_manager.DestroySession(session_id);
            }
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
            return _model.GetUserById(session.user_id, user);
        }

        std::string GetSetCookieHeader(const std::string& session_id)
        {
            return _session_manager.GetCookieHeader(session_id);
        }

        std::string GetClearCookieHeader()
        {
            return _session_manager.GetClearCookieHeader();
        }

    protected:
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

        std::string TrimSpace(const std::string& input) const
        {
            std::string out = input;
            out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch){ return !std::isspace(ch); }));
            out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), out.end());
            return out;
        }

        Model _model;
        View _view;
        CentralConsole _console;
        SessionManager _session_manager;
        sw::redis::Redis _auth_redis;
        ns_mail::Mail _mail;
    };
}
