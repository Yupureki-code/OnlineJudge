#pragma once

#include <cstdio>
#include <assert.h>
#include <string>
#include <vector>
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
#include "oj_model.hpp"
#include "oj_mail.hpp"
#include "oj_view.hpp"
#include "oj_session.hpp"
#include <httplib.h>
#include <jsoncpp/json/json.h>
#include <sw/redis++/redis++.h>
#include <openssl/sha.h>
#include "../../comm/comm.hpp"

//用来控制访问和返回的数据

namespace ns_control
{
    using namespace ns_model;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_view;
    using namespace ns_session;

    //使用ns_model中的User结构体
    using User = ns_model::User;

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
    
    class Control
    {
    public:
        Control() : _auth_redis(redis_addr)
        {}

        Model* GetModel() { return &_model; }

        bool SendEmailAuthCode(const std::string& email, const std::string& client_ip, std::string* err_code, int* retry_after_seconds)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kCooldownSeconds = 60;
            constexpr int kEmailDailyLimit = 20;
            constexpr int kIpDailyLimit = 50;

            if (retry_after_seconds != nullptr)
            {
                *retry_after_seconds = 0;
            }

            try
            {
                std::string cooldown_key = BuildAuthCooldownKey(email);
                if (_auth_redis.exists(cooldown_key))
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "TOO_MANY_REQUESTS";
                    }
                    if (retry_after_seconds != nullptr)
                    {
                        *retry_after_seconds = kCooldownSeconds;
                    }
                    return false;
                }

                std::string email_daily_key = BuildAuthEmailDailyLimitKey(email);
                long long email_daily_count = _auth_redis.incr(email_daily_key);
                if (email_daily_count == 1)
                {
                    _auth_redis.expire(email_daily_key, SecondsUntilDayEnd());
                }
                if (email_daily_count > kEmailDailyLimit)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "EMAIL_DAILY_LIMIT";
                    }
                    return false;
                }

                std::string ip_daily_key = BuildAuthIpDailyLimitKey(client_ip);
                long long ip_daily_count = _auth_redis.incr(ip_daily_key);
                if (ip_daily_count == 1)
                {
                    _auth_redis.expire(ip_daily_key, SecondsUntilDayEnd());
                }
                if (ip_daily_count > kIpDailyLimit)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "IP_DAILY_LIMIT";
                    }
                    return false;
                }

                _auth_redis.setex(cooldown_key, kCooldownSeconds, "1");

                std::string code = GenerateAuthCode();
                _auth_redis.setex(BuildAuthCodeKey(email), kCodeTtlSeconds, code);
                _auth_redis.del(BuildAuthAttemptsKey(email));

                ns_mail::MailSendResult send_result = ns_mail::MailService::SendAuthCodeMail(email, code);
                if (!send_result.ok)
                {
                    _auth_redis.del(BuildAuthCodeKey(email));
                    _auth_redis.del(BuildAuthAttemptsKey(email));
                    _auth_redis.del(cooldown_key);
                    if (err_code != nullptr)
                    {
                        *err_code = "MAIL_SEND_FAILED";
                    }
                    return false;
                }

                if (retry_after_seconds != nullptr)
                {
                    *retry_after_seconds = kCooldownSeconds;
                }
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis auth send code failed: " << e.what();
                if (err_code != nullptr)
                {
                    *err_code = "REDIS_ERROR";
                }
                return false;
            }
        }

        bool VerifyEmailAuthCodeAndLogin(const std::string& email,
                                         const std::string& code,
                                         const std::string& optional_name,
                                         User* user,
                                         bool* is_new_user,
                                         std::string* err_code)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kMaxAttempts = 5;

            if (user == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (is_new_user != nullptr)
            {
                *is_new_user = false;
            }

            try
            {
                auto cached_code = _auth_redis.get(BuildAuthCodeKey(email));
                if (!cached_code)
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "CODE_EXPIRED";
                    }
                    return false;
                }

                if (cached_code.value() != code)
                {
                    std::string attempts_key = BuildAuthAttemptsKey(email);
                    long long attempts = _auth_redis.incr(attempts_key);
                    if (attempts == 1)
                    {
                        _auth_redis.expire(attempts_key, kCodeTtlSeconds);
                    }

                    if (attempts >= kMaxAttempts)
                    {
                        _auth_redis.del(BuildAuthCodeKey(email));
                        _auth_redis.del(attempts_key);
                        if (err_code != nullptr)
                        {
                            *err_code = "ATTEMPTS_EXCEEDED";
                        }
                    }
                    else
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "CODE_MISMATCH";
                        }
                    }
                    return false;
                }

                _auth_redis.del(BuildAuthCodeKey(email));
                _auth_redis.del(BuildAuthAttemptsKey(email));

                bool exists = _model.CheckUser(email);
                if (!exists)
                {
                    std::string name = BuildDefaultUserName(email, optional_name);
                    if (!_model.CreateUser(name, email))
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "CREATE_USER_FAILED";
                        }
                        return false;
                    }
                    if (is_new_user != nullptr)
                    {
                        *is_new_user = true;
                    }
                }

                _model.UpdateLastLogin(email);
                if (!_model.GetUser(email, user))
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "LOAD_USER_FAILED";
                    }
                    return false;
                }
                return true;
            }
            catch (const sw::redis::Error& e)
            {
                logger(ERROR) << "Redis auth verify failed: " << e.what();
                if (err_code != nullptr)
                {
                    *err_code = "REDIS_ERROR";
                }
                return false;
            }
        }
        
        //加载一页内的全部题目
        bool AllQuestions(std::string *html,
                          int page = 1,
                          int size = 5,
                          const QueryStruct& query_hash = QueryStruct(),
                          const std::string& list_version = "1")
        {
            (void)list_version;
            std::string effective_list_version = _model.GetListVersion();
            auto html_key = _model.BuildListCacheKey(query_hash, page, size, effective_list_version, Cache::CacheKey::PageType::kHtml);
            if(_model.GetHtmlPage(html, html_key))
            {
                _model.RecordListHtmlCacheMetrics(true);
                logger(INFO) << "[html_cache][all_questions] hit=1 page=" << page << " size=" << size;
                return true;
            }
            _model.RecordListHtmlCacheMetrics(false);
            logger(INFO) << "[html_cache][all_questions] hit=0 page=" << page << " size=" << size;
            bool ret = true;
            std::vector<struct Question> page_questions;
            int total_count = 0;
            int total_pages = 0;
            auto data_key = _model.BuildListCacheKey(query_hash, page, size, effective_list_version, Cache::CacheKey::PageType::kData);

            if (_model.GetAllQuestions(data_key, page_questions, &total_count, &total_pages))
            {
                int safe_page = page;
                if (safe_page < 1) safe_page = 1;
                if (total_pages > 0 && safe_page > total_pages) safe_page = total_pages;
                if (total_pages <= 0) safe_page = 1;

                int render_total_pages = (total_pages <= 0 ? 1 : total_pages);
                _view.AllExpandHtml(page_questions, html, safe_page, render_total_pages);
                _model.SetHtmlPage(html, html_key);
            }
            else
            {
                *html = "获取题目失败, 形成题目列表失败";
                ret = false;
            }
            return ret;
        }
        //获取单个题目
        bool OneQuestion(const std::string &number, std::string *html)
        {
            auto html_key = _model.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kHtml);
            if (_model.GetHtmlPage(html, html_key))
            {
                _model.RecordDetailHtmlCacheMetrics(true);
                logger(INFO) << "[html_cache][one_question] hit=1 qid=" << number;
                return true;
            }
            _model.RecordDetailHtmlCacheMetrics(false);
            logger(INFO) << "[html_cache][one_question] hit=0 qid=" << number;

            bool ret = true;
            Question q;
            if (_model.GetOneQuestion(number, q))
            {
                _view.OneExpandHtml(q, html);
                _model.SetHtmlPage(html, html_key);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在!";
                ret = false;
            }
            return ret;
        }
        //判断题目正确，调用主机责任链
        void Judge(const std::string& number,const std::string& in_json,std::string* out_json)
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

        //用户:检查用户是否存在
        bool CheckUser(const std::string& email, Json::Value& response)
        {
            bool exists = _model.CheckUser(email);
            response["exists"] = exists;
            if(exists)
            {
                logger(ns_log::INFO)<<"用户存在 email:"<<email;
                User user;
                if(_model.GetUser(email, &user))
                {
                    response["user"] = Json::Value();
                    response["user"]["uid"] = user.uid;
                    response["user"]["name"] = user.name;
                    response["user"]["email"] = user.email;
                    response["user"]["create_time"] = user.create_time;
                    response["user"]["last_login"] = user.last_login;
                }
            }
            return true;
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email, Json::Value& response)
        {
            bool created = _model.CreateUser(name, email);
            response["created"] = created;
            if(created)
            {
                _model.UpdateLastLogin(email);
                User user;
                if(_model.GetUser(email, &user))
                {
                    response["user"] = Json::Value();
                    response["user"]["uid"] = user.uid;
                    response["user"]["name"] = user.name;
                    response["user"]["email"] = user.email;
                    response["user"]["create_time"] = user.create_time;
                    response["user"]["last_login"] = user.last_login;
                }
            }
            return created;
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, Json::Value& response)
        {
            User user;
            bool found = _model.GetUser(email, &user);
            if(found)
            {
                _model.UpdateLastLogin(email);
                response["user"] = Json::Value();
                response["user"]["uid"] = user.uid;
                response["user"]["name"] = user.name;
                response["user"]["email"] = user.email;
                response["user"]["create_time"] = user.create_time;
                response["user"]["last_login"] = user.last_login;
            }
            response["found"] = found;
            return found;
        }

        //用户:获取用户信息(User对象)
        bool GetUser(const std::string& email, User* user)
        {
            return _model.GetUser(email, user);
        }

        bool SetPasswordForUser(const std::string& email, const std::string& password, std::string* err_code)
        {
            if (!IsPasswordStrongEnough(password))
            {
                if (err_code != nullptr)
                {
                    *err_code = "WEAK_PASSWORD";
                }
                return false;
            }

            std::string password_hash = BuildPasswordHash(password);
            if (password_hash.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_HASH_FAILED";
                }
                return false;
            }

            if (!_model.SetUserPassword(email, password_hash, PasswordAlgoTag()))
            {
                if (err_code != nullptr)
                {
                    *err_code = "UPDATE_PASSWORD_FAILED";
                }
                return false;
            }
            return true;
        }

        bool LoginWithPassword(const std::string& email,
                               const std::string& password,
                               User* user,
                               std::string* err_code)
        {
            if (user == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            std::string password_hash;
            std::string password_algo;
            if (!_model.GetUserPasswordAuth(email, &password_hash, &password_algo))
            {
                if (err_code != nullptr)
                {
                    *err_code = "USER_NOT_FOUND";
                }
                return false;
            }

            if (password_algo.empty() || password_algo == "none" || password_hash.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_NOT_SET";
                }
                return false;
            }

            if (!VerifyPasswordHash(password, password_hash, password_algo))
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_MISMATCH";
                }
                return false;
            }

            _model.UpdateLastLogin(email);
            if (!_model.GetUser(email, user))
            {
                if (err_code != nullptr)
                {
                    *err_code = "LOAD_USER_FAILED";
                }
                return false;
            }
            return true;
        }

        bool SaveQuestion(const Question& q)
        {
            return _model.SaveQuestion(q);
        }

        bool DeleteQuestion(const std::string& number)
        {
            return _model.DeleteQuestion(number);
        }

        // 题目写路径统一调用：递增列表版本，触发列表缓存失效。
        std::string TouchQuestionListVersion()
        {
            return _model.TouchQuestionListVersion();
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
        bool GetStaticHtml(const std::string& path, std::string* html)
        {
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
    private:
        std::string PasswordAlgoTag() const
        {
            return "sha256_iter_v1";
        }

        int PasswordIterations() const
        {
            return 120000;
        }

        std::string PasswordPepper() const
        {
            const char* pepper = std::getenv("OJ_PASSWORD_PEPPER");
            if (pepper == nullptr)
            {
                return "";
            }
            return std::string(pepper);
        }

        bool IsPasswordStrongEnough(const std::string& password) const
        {
            if (password.size() < 8 || password.size() > 72)
            {
                return false;
            }

            bool has_letter = false;
            bool has_digit = false;
            for (unsigned char ch : password)
            {
                if (std::isalpha(ch)) has_letter = true;
                if (std::isdigit(ch)) has_digit = true;
            }
            return has_letter && has_digit;
        }

        std::string ToHex(const unsigned char* data, size_t len) const
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (size_t i = 0; i < len; ++i)
            {
                oss << std::setw(2) << static_cast<unsigned int>(data[i]);
            }
            return oss.str();
        }

        std::string Sha256Hex(const std::string& input) const
        {
            std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
            SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest.data());
            return ToHex(digest.data(), digest.size());
        }

        std::string GenerateSaltHex(size_t bytes) const
        {
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, 255);
            std::vector<unsigned char> raw(bytes);
            for (size_t i = 0; i < bytes; ++i)
            {
                raw[i] = static_cast<unsigned char>(dist(rng));
            }
            return ToHex(raw.data(), raw.size());
        }

        std::string DerivePasswordDigest(const std::string& password, const std::string& salt, int iterations) const
        {
            std::string pepper = PasswordPepper();
            std::string digest = Sha256Hex(salt + ":" + password + ":" + pepper);
            for (int i = 1; i < iterations; ++i)
            {
                digest = Sha256Hex(digest + ":" + salt + ":" + pepper);
            }
            return digest;
        }

        std::string BuildPasswordHash(const std::string& password) const
        {
            const int iter = PasswordIterations();
            std::string salt = GenerateSaltHex(16);
            std::string digest = DerivePasswordDigest(password, salt, iter);
            if (digest.empty())
            {
                return "";
            }

            return salt + "$" + std::to_string(iter) + "$" + digest;
        }

        bool VerifyPasswordHash(const std::string& password,
                                const std::string& stored_hash,
                                const std::string& algo) const
        {
            if (algo != PasswordAlgoTag())
            {
                return false;
            }

            std::vector<std::string> parts;
            StringUtil::SplitString(stored_hash, "$", &parts);
            if (parts.size() != 3)
            {
                return false;
            }

            const std::string& salt = parts[0];
            int iter = 0;
            try
            {
                iter = std::stoi(parts[1]);
            }
            catch (...)
            {
                return false;
            }

            if (iter <= 0)
            {
                return false;
            }

            std::string expected = DerivePasswordDigest(password, salt, iter);
            return expected == parts[2];
        }

        std::string BuildAuthCodeKey(const std::string& email) const
        {
            return "oj:auth:code:" + email;
        }

        std::string BuildAuthAttemptsKey(const std::string& email) const
        {
            return "oj:auth:attempts:" + email;
        }

        std::string BuildAuthCooldownKey(const std::string& email) const
        {
            return "oj:auth:cooldown:" + email;
        }

        std::string BuildAuthEmailDailyLimitKey(const std::string& email) const
        {
            return "oj:auth:limit:email:" + email + ":" + CurrentDateTag();
        }

        std::string BuildAuthIpDailyLimitKey(const std::string& ip) const
        {
            return "oj:auth:limit:ip:" + ip + ":" + CurrentDateTag();
        }

        std::string CurrentDateTag() const
        {
            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
#if defined(_WIN32)
            localtime_s(&tm_now, &tt);
#else
            localtime_r(&tt, &tm_now);
#endif
            char buf[16] = {0};
            std::strftime(buf, sizeof(buf), "%Y%m%d", &tm_now);
            return std::string(buf);
        }

        int SecondsUntilDayEnd() const
        {
            auto now = std::chrono::system_clock::now();
            std::time_t tt = std::chrono::system_clock::to_time_t(now);
            std::tm tm_now;
#if defined(_WIN32)
            localtime_s(&tm_now, &tt);
#else
            localtime_r(&tt, &tm_now);
#endif
            int seconds = (23 - tm_now.tm_hour) * 3600 + (59 - tm_now.tm_min) * 60 + (60 - tm_now.tm_sec);
            return std::max(60, seconds);
        }

        std::string GenerateAuthCode() const
        {
            static thread_local std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(0, 999999);
            int value = dist(rng);
            std::ostringstream oss;
            oss.width(6);
            oss.fill('0');
            oss << value;
            return oss.str();
        }

        std::string BuildDefaultUserName(const std::string& email, const std::string& optional_name) const
        {
            std::string name = optional_name;
            name.erase(name.begin(), std::find_if(name.begin(), name.end(), [](unsigned char ch){ return !std::isspace(ch); }));
            name.erase(std::find_if(name.rbegin(), name.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), name.end());
            if (!name.empty())
            {
                if (name.size() > 64)
                {
                    name.resize(64);
                }
                return name;
            }

            auto pos = email.find('@');
            std::string prefix = pos == std::string::npos ? email : email.substr(0, pos);
            if (prefix.empty())
            {
                prefix = "user";
            }
            if (prefix.size() > 32)
            {
                prefix.resize(32);
            }

            auto now = std::chrono::system_clock::now().time_since_epoch();
            long long suffix = std::chrono::duration_cast<std::chrono::seconds>(now).count() % 100000;
            return prefix + "_" + std::to_string(suffix);
        }

        Model _model;
        View _view;
        CentralConsole _console;
        SessionManager _session_manager;
        sw::redis::Redis _auth_redis;

    public:
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

        bool GetSessionUser(const std::string& cookie_header, User* user)
        {
            std::string session_id;
            if (!_session_manager.ParseCookie(cookie_header, &session_id))
            {
                return false;
            }

            Session session;
            if (!_session_manager.GetSession(session_id, &session))
            {
                return false;
            }

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
    };
};

