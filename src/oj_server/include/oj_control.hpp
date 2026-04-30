#pragma once

#include <Logger/logstrategy.h>
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
#include <sys/stat.h>
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
        //发送邮箱验证码
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

                ns_mail::MailSendResult send_result = _mail.SendAuthCodeMail(email, code);
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
                         const std::string& optional_password,
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
                    std::string trimmed_name = TrimSpace(optional_name);
                    if (trimmed_name.empty())
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "REGISTER_NAME_REQUIRED";
                        }
                        return false;
                    }

                    if (!IsPasswordStrongEnough(optional_password))
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "WEAK_PASSWORD";
                        }
                        return false;
                    }

                    std::string name = BuildDefaultUserName(email, trimmed_name);
                    if (!_model.CreateUser(name, email))
                    {
                        if (err_code != nullptr)
                        {
                            *err_code = "CREATE_USER_FAILED";
                        }
                        return false;
                    }

                    std::string password_hash = BuildPasswordHash(optional_password);
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
                          std::shared_ptr<QueryStruct> query_hash = nullptr,
                          const std::string& list_version = "1")
        {
            //list_version参数目前没有实际作用，后续可以用来实现当题库发生变化时自动更新缓存的功能
            //目前先直接使用_model.GetQuestionsListVersion()获取版本号，保证缓存一致性
            (void)list_version;
            std::string effective_list_version = _model.GetQuestionsListVersion();
            auto html_key = _model.BuildListCacheKey(query_hash, page, size, effective_list_version, Cache::CacheKey::PageType::kHtml);
            //先在cache中找静态HTML页面，如果找到直接返回
            //如果没有找到再查询数据并生成HTML页面，最后将生成的HTML页面写入cache供下次访问使用
            if(_model.GetHtmlPage(html, html_key))
            {
                _model.RecordListHtmlCacheMetrics(true);
                logger(ns_log::INFO)<<"[html_cache][all_questions] hit=1 page=" << page << " size=" << size;
                return true;
            }
            _model.RecordListHtmlCacheMetrics(false);
            logger(INFO) << "[html_cache][all_questions] hit=0 page=" << page << " size=" << size;
            bool ret = true;
            std::vector<struct Question> page_questions;
            int total_count = 0;
            int total_pages = 0;
            auto data_key = _model.BuildListCacheKey(query_hash, page, size, effective_list_version, Cache::CacheKey::PageType::kData);
            //先在cache中找数据，如果找到直接用数据生成HTML页面返回
            if (_model.GetQuestionsByPage(data_key, page_questions, &total_count, &total_pages))
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
            bool ret = true;
            Question q;
            if (_model.GetOneQuestion(number, q))
            {
                _view.OneExpandHtml(q, html);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在!";
                ret = false;
            }
            return ret;
        }
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

        bool VerifyEmailAuthCode(const std::string& email,
                                 const std::string& code,
                                 std::string* err_code)
        {
            constexpr int kCodeTtlSeconds = 300;
            constexpr int kMaxAttempts = 5;

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

        bool ChangeEmailWithCode(const User& current_user,
                                 const std::string& new_email,
                                 const std::string& code,
                                 User* updated_user,
                                 std::string* err_code)
        {
            if (new_email == current_user.email)
            {
                if (err_code != nullptr)
                {
                    *err_code = "SAME_EMAIL";
                }
                return false;
            }

            std::string verify_err;
            if (!VerifyEmailAuthCode(current_user.email, code, &verify_err))
            {
                if (err_code != nullptr)
                {
                    *err_code = verify_err;
                }
                return false;
            }

            if (_model.CheckUser(new_email))
            {
                if (err_code != nullptr)
                {
                    *err_code = "EMAIL_ALREADY_EXISTS";
                }
                return false;
            }

            if (!_model.UpdateUserEmail(current_user.email, new_email))
            {
                if (err_code != nullptr)
                {
                    *err_code = "UPDATE_EMAIL_FAILED";
                }
                return false;
            }

            if (updated_user != nullptr)
            {
                if (!_model.GetUser(new_email, updated_user))
                {
                    if (err_code != nullptr)
                    {
                        *err_code = "LOAD_USER_FAILED";
                    }
                    return false;
                }
            }
            return true;
        }

        bool ChangePasswordWithCode(const std::string& email,
                                    const std::string& code,
                                    const std::string& new_password,
                                    std::string* err_code)
        {
            std::string verify_err;
            if (!VerifyEmailAuthCode(email, code, &verify_err))
            {
                if (err_code != nullptr)
                {
                    *err_code = verify_err;
                }
                return false;
            }

            return SetPasswordForUser(email, new_password, err_code);
        }

        bool DeleteAccountWithCode(const std::string& email,
                                   const std::string& code,
                                   std::string* err_code)
        {
            std::string verify_err;
            if (!VerifyEmailAuthCode(email, code, &verify_err))
            {
                if (err_code != nullptr)
                {
                    *err_code = verify_err;
                }
                return false;
            }

            if (!_model.DeleteUserByEmail(email))
            {
                if (err_code != nullptr)
                {
                    *err_code = "DELETE_USER_FAILED";
                }
                return false;
            }
            return true;
        }

        bool LoginWithPassword(const std::string& email_or_username,
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
            std::string resolved_email;
            bool found = false;

            // 先作为用户名查找
            User user_by_name;
            if (_model.GetUserByName(email_or_username, &user_by_name))
            {
                if (_model.GetUserPasswordAuth(user_by_name.email, &password_hash, &password_algo))
                {
                    resolved_email = user_by_name.email;
                    found = true;
                }
            }

            // 如果用户名方式没找到或密码为空，尝试作为邮箱查找
            if (!found)
            {
                password_hash.clear();
                password_algo.clear();
                if (_model.GetUserPasswordAuth(email_or_username, &password_hash, &password_algo))
                {
                    resolved_email = email_or_username;
                    found = true;
                }
            }

            if (!found || password_hash.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "USER_NOT_FOUND";
                }
                return false;
            }

            if (password_algo.empty() || password_algo == "none")
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

            _model.UpdateLastLogin(resolved_email);
            if (!_model.GetUser(resolved_email, user))
            {
                if (err_code != nullptr)
                {
                    *err_code = "LOAD_USER_FAILED";
                }
                return false;
            }
            return true;
        }

        bool LoginAdminWithIdAndPassword(int admin_id,
                                         const std::string& password,
                                         AdminAccount* admin,
                                         User* user,
                                         std::string* err_code)
        {
            if (admin == nullptr || user == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (admin_id <= 0 || password.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (!_model.GetAdminByAdminId(admin_id, admin))
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_CREDENTIALS";
                }
                return false;
            }

            if (!VerifyPasswordHash(password, admin->password_hash, PasswordAlgoTag()))
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_CREDENTIALS";
                }
                return false;
            }

            if (!_model.GetUserById(admin->uid, user))
            {
                if (err_code != nullptr)
                {
                    *err_code = "LOAD_USER_FAILED";
                }
                return false;
            }

            _model.UpdateLastLogin(user->email);
            return true;
        }

        bool BuildSecurePasswordHash(const std::string& password,
                                     std::string* password_hash,
                                     std::string* err_code)
        {
            if (password_hash == nullptr)
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_ARGUMENT";
                }
                return false;
            }

            if (!IsPasswordStrongEnough(password))
            {
                if (err_code != nullptr)
                {
                    *err_code = "WEAK_PASSWORD";
                }
                return false;
            }

            *password_hash = BuildPasswordHash(password);
            if (password_hash->empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "PASSWORD_HASH_FAILED";
                }
                return false;
            }
            return true;
        }

        bool SaveQuestion(const Question& q)
        {
            return _model.SaveQuestion(q);
        }

        bool PublishSolution(const std::string& question_id,
                              const User& current_user,
                              const std::string& title,
                              const std::string& content_md,
                              unsigned long long* solution_id,
                              std::string* err_code)
        {
            if (current_user.uid <= 0)
            {
                if (err_code != nullptr)
                {
                    *err_code = "UNAUTHORIZED";
                }
                return false;
            }

            std::string trimmed_question_id = TrimSpace(question_id);
            if (trimmed_question_id.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_QUESTION_ID";
                }
                return false;
            }

            Question question;
            if (!_model.GetOneQuestion(trimmed_question_id, question))
            {
                if (err_code != nullptr)
                {
                    *err_code = "QUESTION_NOT_FOUND";
                }
                return false;
            }

            if (!_model.HasUserPassedQuestion(current_user.uid, trimmed_question_id))
            {
                if (err_code != nullptr)
                {
                    *err_code = "NOT_PASSED";
                }
                return false;
            }

            std::string trimmed_title = TrimSpace(title);
            std::string trimmed_content = TrimSpace(content_md);
            if (trimmed_title.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "TITLE_REQUIRED";
                }
                return false;
            }

            if (trimmed_title.size() > 30)
            {
                if (err_code != nullptr)
                {
                    *err_code = "TITLE_TOO_LONG";
                }
                return false;
            }

            if (trimmed_content.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "CONTENT_REQUIRED";
                }
                return false;
            }

            Solution solution;
            solution.question_id = trimmed_question_id;
            solution.user_id = current_user.uid;
            solution.title = trimmed_title;
            solution.content_md = trimmed_content;
            solution.status = SolutionStatus::approved;

            if (!_model.CreateSolution(solution, solution_id))
            {
                if (err_code != nullptr)
                {
                    *err_code = "CREATE_SOLUTION_FAILED";
                }
                return false;
            }

            return true;
        }

        // Comments: create a new comment for a solution (supports nesting via parent_id)
        bool PostComment(unsigned long long solution_id,
                         const User& current_user,
                         const std::string& content,
                         Json::Value* result,
                         std::string* err_code,
                         unsigned long long parent_id = 0)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            // Authorization check
            if (current_user.uid <= 0)
            {
                *err_code = "UNAUTHORIZED";
                return false;
            }

            std::string trimmed = TrimSpace(content);
            if (trimmed.empty())
            {
                *err_code = "CONTENT_REQUIRED";
                return false;
            }
            if (trimmed.size() > 1000)
            {
                *err_code = "CONTENT_TOO_LONG";
                return false;
            }

            // Check solution exists
            Solution dummy;
            if (!_model.GetSolutionById(solution_id, &dummy))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            Comment c;
            c.solution_id = solution_id;
            c.user_id = current_user.uid;
            c.content = trimmed;
            c.is_edited = false;
            // nesting support
            c.parent_id = parent_id;
            unsigned long long reply_to_user_id = 0;
            if (parent_id > 0)
            {
                // fetch parent to fill reply_to_user_id and validate same solution
                Comment parent;
                if (_model.GetCommentById(parent_id, &parent))
                {
                    reply_to_user_id = parent.user_id;
                    // ensure the parent belongs to the same solution
                    if (parent.solution_id != solution_id)
                    {
                        *err_code = "INVALID_PARENT_SOLUTION";
                        return false;
                    }
                }
            }
            c.reply_to_user_id = static_cast<int>(reply_to_user_id);

            unsigned long long new_id = 0;
            if (!_model.CreateComment(c, &new_id))
            {
                *err_code = "CREATE_FAILED";
                return false;
            }

            // fetch comment to fill response details
            Comment created;
            if (_model.GetCommentById(new_id, &created))
            {
                result->clear();
                (*result) ["success"] = true;
                (*result)["id"] = Json::UInt64(created.id);
                (*result)["solution_id"] = Json::UInt64(created.solution_id);
                (*result)["user_id"] = created.user_id;
                (*result)["content"] = created.content;
                (*result)["is_edited"] = created.is_edited;
                (*result)["created_at"] = created.created_at;
                // author name
                User author;
                if (_model.GetUserById(created.user_id, &author))
                {
                    (*result)["author_name"] = author.name;
                (*result)["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    (*result)["author_name"] = "";
                    (*result)["author_avatar"] = "/pictures/head.jpg";
                }
            }
            else
            {
                // Fallback minimal success payload
                result->clear();
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(new_id);
                (*result)["solution_id"] = Json::UInt64(solution_id);
                (*result)["content"] = trimmed;
            }
            return true;
        }

        // Comments: get comments for a solution with pagination
        bool GetComments(unsigned long long solution_id, int page, int size,
                         Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> comments;
            int total_count = 0;
            if (!_model.GetCommentsBySolutionId(solution_id, page, size, &comments, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            int total_pages = (size > 0) ? ((total_count + size - 1) / size) : 0;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = std::max(1, page);
            (*result)["size"] = size;

            Json::Value list(Json::arrayValue);
            for (const auto& c : comments)
            {
                Json::Value item;
                item["id"] = Json::UInt64(c.id);
                item["solution_id"] = Json::UInt64(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["parent_id"] = Json::UInt64(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["reply_to_user_name"] = c.reply_to_user_name;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                User author;
                if (_model.GetUserById(c.user_id, &author))
                {
                    item["author_name"] = author.name;
                    item["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    item["author_name"] = "";
                    item["author_avatar"] = "/pictures/head.jpg";
                }
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        // Comments: get top-level comments for a solution with reply counts
        bool GetTopLevelComments(unsigned long long solution_id, int page, int size,
                                 Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> comments;
            int total_count = 0;
            // fetch top-level comments only
            if (!_model.GetCommentsBySolutionId(solution_id, page, size, &comments, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            int total_pages = (size > 0) ? ((total_count + size - 1) / size) : 0;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = std::max(1, page);
            (*result)["size"] = size;

            Json::Value list(Json::arrayValue);
            for (const auto& c : comments)
            {
                Json::Value item;
                item["id"] = Json::UInt64(c.id);
                item["solution_id"] = Json::UInt64(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["parent_id"] = Json::UInt64(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["reply_to_user_name"] = c.reply_to_user_name;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                // author info
                User author;
                if (_model.GetUserById(c.user_id, &author))
                {
                    item["author_name"] = author.name;
                    item["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    item["author_name"] = "";
                    item["author_avatar"] = "/pictures/head.jpg";
                }
                // reply count for this top-level comment
                std::vector<Comment> tmp_replies;
                int tmp_total = 0;
                if (_model.GetCommentReplies(c.id, 1, 1000000, &tmp_replies, &tmp_total))
                {
                    item["reply_count"] = tmp_total;
                }
                else
                {
                    item["reply_count"] = 0;
                }
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        // Comments: get replies for a given top-level comment (child comments)
        bool GetCommentReplies(unsigned long long parent_id, int page, int size,
                               Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> replies;
            int total_count = 0;
            if (!_model.GetCommentReplies(parent_id, page, size, &replies, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            int total_pages = (size > 0) ? ((total_count + size - 1) / size) : 0;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = std::max(1, page);
            (*result)["size"] = size;

            Json::Value list(Json::arrayValue);
            for (const auto& r : replies)
            {
                Json::Value item;
                item["id"] = Json::UInt64(r.id);
                item["solution_id"] = Json::UInt64(r.solution_id);
                item["user_id"] = r.user_id;
                item["content"] = r.content;
                item["is_edited"] = r.is_edited;
                item["created_at"] = r.created_at;
                item["updated_at"] = r.updated_at;
                item["parent_id"] = Json::UInt64(r.parent_id);
                item["reply_to_user_id"] = r.reply_to_user_id;
                item["reply_to_user_name"] = r.reply_to_user_name;
                item["like_count"] = r.like_count;
                item["favorite_count"] = r.favorite_count;
                User author;
                if (_model.GetUserById(r.user_id, &author))
                {
                    item["author_name"] = author.name;
                    item["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    item["author_name"] = "";
                    item["author_avatar"] = "/pictures/head.jpg";
                }
                // reply count for this reply (supports nested reply-to-reply)
                std::vector<Comment> nested;
                int nested_total = 0;
                if (_model.GetCommentReplies(r.id, 1, 1000000, &nested, &nested_total))
                {
                    item["reply_count"] = nested_total;
                }
                else
                {
                    item["reply_count"] = 0;
                }
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        // Comments: edit a comment (owner only)
        bool EditComment(unsigned long long comment_id, const User& current_user,
                         const std::string& content, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::string trimmed = TrimSpace(content);
            if (trimmed.empty())
            {
                *err_code = "CONTENT_REQUIRED";
                return false;
            }
            if (trimmed.size() > 1000)
            {
                *err_code = "CONTENT_TOO_LONG";
                return false;
            }

            Comment c;
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "NOT_FOUND";
                return false;
            }
            if (c.user_id != current_user.uid)
            {
                *err_code = "FORBIDDEN";
                return false;
            }

            if (!_model.UpdateComment(comment_id, current_user.uid, trimmed))
            {
                *err_code = "UPDATE_FAILED";
                return false;
            }

            Comment updated;
            if (_model.GetCommentById(comment_id, &updated))
            {
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(updated.id);
                (*result)["content"] = updated.content;
                (*result)["is_edited"] = updated.is_edited;
                (*result)["updated_at"] = updated.updated_at;
            }
            else
            {
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(comment_id);
            }
            return true;
        }

        // Comments: delete a comment (owner or admin)
        bool DeleteComment(unsigned long long comment_id, const User& current_user,
                           Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            Comment c;
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "NOT_FOUND";
                return false;
            }
            bool is_admin = _model.GetAdminByUid(current_user.uid);
            if (c.user_id != current_user.uid && !is_admin)
            {
                *err_code = "FORBIDDEN";
                return false;
            }
            if (!_model.DeleteComment(comment_id, current_user.uid, is_admin))
            {
                *err_code = "DELETE_FAILED";
                return false;
            }
            (*result)["success"] = true;
            return true;
        }

        bool HasUserPassedQuestion(int user_id, const std::string& question_id)
        {
            return _model.HasUserPassedQuestion(user_id, question_id);
        }

        bool DeleteQuestion(const std::string& number)
        {
            return _model.DeleteQuestion(number);
        }

        bool GetSolutionList(const std::string& question_id,
                             const std::string& status,
                             const std::string& sort,
                             int page, int size,
                             Json::Value* result,
                             std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            Question question;
            if (!_model.GetOneQuestion(question_id, question))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }

            std::string status_filter;
            if (status == "pending" || status == "approved" || status == "rejected")
            {
                status_filter = status;
            }
            else
            {
                status_filter = "approved";
            }

            std::string sort_order = sort;
            if (sort_order != "hot" && sort_order != "latest")
            {
                sort_order = "latest";
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 50));

            std::vector<Solution> solutions;
            int total_count = 0;
            int total_pages = 0;
            if (!_model.GetSolutionsByPage(question_id, status_filter, sort_order,
                                           safe_page, safe_size,
                                           &solutions, &total_count, &total_pages))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = safe_page;
            (*result)["size"] = safe_size;

            Json::Value items(Json::arrayValue);
            for (const auto& s : solutions)
            {
                Json::Value item;
                item["id"] = Json::UInt64(s.id);
                item["question_id"] = s.question_id;
                item["user_id"] = s.user_id;
                item["title"] = s.title;
                item["content_md"] = s.content_md;
                item["like_count"] = s.like_count;
                item["favorite_count"] = s.favorite_count;
                item["comment_count"] = s.comment_count;
                item["status"] = _model.SolutionStatusToDbString(s.status);
                item["created_at"] = s.created_at;

                User author;
                if (_model.GetUserById(s.user_id, &author))
                {
                    item["author_name"] = author.name;
                    item["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    item["author_name"] = "";
                    item["author_avatar"] = "/pictures/head.jpg";
                }

                items.append(item);
            }
            (*result)["solutions"] = items;
            return true;
        }

        bool GetSolutionDetail(long long solution_id,
                               Json::Value* result,
                               std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            Solution solution;
            if (!_model.GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            (*result)["success"] = true;
            (*result)["id"] = Json::UInt64(solution.id);
            (*result)["question_id"] = solution.question_id;
            (*result)["user_id"] = solution.user_id;
            (*result)["title"] = solution.title;
            (*result)["content_md"] = solution.content_md;
            (*result)["like_count"] = solution.like_count;
            (*result)["favorite_count"] = solution.favorite_count;
            (*result)["comment_count"] = solution.comment_count;
            (*result)["status"] = _model.SolutionStatusToDbString(solution.status);
            (*result)["created_at"] = solution.created_at;
            (*result)["updated_at"] = solution.updated_at;

            User author;
            if (_model.GetUserById(solution.user_id, &author))
            {
                (*result)["author_name"] = author.name;
                (*result)["author_avatar"] = GetEffectiveAvatarUrl(author);
            }
            else
            {
                (*result)["author_name"] = "";
                (*result)["author_avatar"] = "/pictures/head.jpg";
            }

            return true;
        }

        bool ToggleLike(long long solution_id,
                        const User& current_user,
                        Json::Value* result,
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
            if (!_model.GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleSolutionAction(solution_id, current_user.uid, "like", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["liked"] = now_active;
            (*result)["like_count"] = new_count;
            return true;
        }

        bool ToggleFavorite(long long solution_id,
                            const User& current_user,
                            Json::Value* result,
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
            if (!_model.GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleSolutionAction(solution_id, current_user.uid, "favorite", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["favorited"] = now_active;
            (*result)["favorite_count"] = new_count;
            return true;
        }
        
        bool GetUserSolutionActions(long long solution_id,
                                    int user_id,
                                    Json::Value* result)
        {
            if (result == nullptr)
            {
                return false;
            }

            std::vector<long long> ids = {solution_id};
            std::map<long long, std::map<std::string, bool>> actions;
            if (!_model.GetUserActionsForSolutions(user_id, ids, actions))
            {
                return false;
            }

            (*result)["liked"] = actions.count(solution_id) > 0 && actions[solution_id].count("like") > 0;
            (*result)["favorited"] = actions.count(solution_id) > 0 && actions[solution_id].count("favorite") > 0;
            return true;
        }

        // Comments: toggle like on a comment
        bool ToggleCommentLike(unsigned long long comment_id,
                               const User& current_user,
                               Json::Value* result,
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

            Comment c;
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "COMMENT_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleCommentAction(comment_id, current_user.uid, "like", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["liked"] = now_active;
            (*result)["like_count"] = new_count;
            return true;
        }

        // Comments: toggle favorite on a comment
        bool ToggleCommentFavorite(unsigned long long comment_id,
                                   const User& current_user,
                                   Json::Value* result,
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

            Comment c;
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "COMMENT_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleCommentAction(comment_id, current_user.uid, "favorite", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["favorited"] = now_active;
            (*result)["favorite_count"] = new_count;
            return true;
        }

        // Comments: get actions for multiple comments for a user
        bool GetCommentActions(const std::vector<unsigned long long>& comment_ids,
                               int user_id,
                               Json::Value* result)
        {
            if (result == nullptr)
            {
                return false;
            }
            std::map<unsigned long long, std::map<std::string, bool>> actions;
            if (!_model.GetCommentActions(comment_ids, user_id, &actions))
            {
                return false;
            }

            (*result)["success"] = true;
            Json::Value actions_json(Json::objectValue);
            for (const auto& kv : actions)
            {
                Json::Value item(Json::objectValue);
                auto cid = kv.first;
                const auto& map = kv.second;
                auto it_like = map.find("like");
                auto it_fav = map.find("favorite");
                item["like"] = (it_like != map.end()) ? it_like->second : false;
                item["favorite"] = (it_fav != map.end()) ? it_fav->second : false;
                // use string key to avoid JSONCPP index type issues
                actions_json[std::to_string(cid)] = item;
            }
            (*result)["actions"] = actions_json;
            return true;
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
        //构建验证码发送冷却的redis key，格式为oj:auth:cooldown:{email}
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

        std::string TrimSpace(const std::string& input) const
        {
            std::string out = input;
            out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char ch){ return !std::isspace(ch); }));
            out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), out.end());
            return out;
        }

        std::string BuildDefaultUserName(const std::string& email, const std::string& optional_name) const
        {
            std::string name = TrimSpace(optional_name);
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
        ns_mail::Mail _mail;

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

        bool UploadAvatar(const User& current_user, const std::string& file_content, const std::string& content_type, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            static const size_t kMaxAvatarSize = 2 * 1024 * 1024;
            static const std::vector<std::string> kAllowedTypes = {"image/jpeg", "image/png", "image/gif", "image/webp"};

            bool type_allowed = false;
            for (const auto& t : kAllowedTypes)
            {
                if (content_type == t)
                {
                    type_allowed = true;
                    break;
                }
            }
            if (!type_allowed)
            {
                *err_code = "INVALID_TYPE";
                return false;
            }

            if (file_content.size() > kMaxAvatarSize)
            {
                *err_code = "FILE_TOO_LARGE";
                return false;
            }

            std::string ext;
            if (content_type == "image/jpeg") ext = "jpg";
            else if (content_type == "image/png") ext = "png";
            else if (content_type == "image/gif") ext = "gif";
            else ext = "webp";

            // 用户头像按 uid 命名，每个用户只保存一份
            std::string filename = std::to_string(current_user.uid) + "." + ext;
            std::string relative_path = std::string("/pictures/avatars/") + filename;
            std::string absolute_dir = std::string(HTML_PATH) + "pictures/avatars/";

            struct stat st;
            if (stat(absolute_dir.c_str(), &st) != 0)
            {
                mkdir(absolute_dir.c_str(), 0755);
            }

            // 删除旧头像文件（防止换扩展名后旧文件残留）
            if (!current_user.avatar_path.empty()
                && current_user.avatar_path.find("../") == std::string::npos
                && current_user.avatar_path.find("/pictures/avatars/") == 0
                && current_user.avatar_path != relative_path)
            {
                std::string old_path = std::string(HTML_PATH) + "pictures/avatars/" + std::string(current_user.avatar_path.substr(current_user.avatar_path.rfind('/') + 1));
                unlink(old_path.c_str());
            }

            std::string absolute_path = absolute_dir + filename;
            if (!ns_util::FileUtil::WriteFile(absolute_path, file_content))
            {
                *err_code = "SAVE_FAILED";
                return false;
            }

            if (!_model.UpdateUserAvatar(current_user.uid, relative_path))
            {
                unlink(absolute_path.c_str());
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["avatar_url"] = relative_path;
            _model.SetCachedStringByAnyKey("avatar:user:" + std::to_string(current_user.uid), relative_path, 3600);
            return true;
        }

        bool DeleteAvatar(const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            // 删除磁盘上的旧头像文件（仅限 avatars 目录下的文件，防止路径穿越）
            if (!current_user.avatar_path.empty())
            {
                std::string rel = current_user.avatar_path;
                if (rel.find("../") == std::string::npos && rel.find("/pictures/avatars/") == 0)
                {
                    std::string absolute_path = std::string(HTML_PATH) + "pictures/avatars/" + std::string(rel.substr(rel.rfind('/') + 1));
                    unlink(absolute_path.c_str());
                }
            }

            if (!_model.UpdateUserAvatar(current_user.uid, ""))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            _model.SetCachedStringByAnyKey("avatar:user:" + std::to_string(current_user.uid), "", 3600);
            return true;
        }

        bool GetUserSubmits(const std::string& question_id, const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Json::Value submits(Json::arrayValue);
            if (!_model.GetUserSubmitsByQuestion(current_user.uid, question_id, &submits))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["submits"] = submits;
            return true;
        }

        bool GetSampleTests(const std::string& question_id, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Question q;
            if (!_model.GetOneQuestion(question_id, q))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }

            Json::Value tests(Json::arrayValue);
            if (!_model.GetSampleTestsByQuestionId(question_id, &tests))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["tests"] = tests;
            return true;
        }

        bool RunSingleTest(const std::string& question_id, const std::string& code,
                          int test_case_id, const std::string& test_type,
                          const std::string& custom_input,
                          const User& current_user,
                          Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            std::string test_input;
            std::string test_output;

            if (test_type == "sample")
            {
                if (!_model.GetTestById(test_case_id, question_id, &test_input, &test_output))
                {
                    *err_code = "TEST_NOT_FOUND";
                    return false;
                }
            }
            else if (test_type == "custom")
            {
                test_input = custom_input;
                test_output = "";
            }
            else
            {
                *err_code = "INVALID_TEST_TYPE";
                return false;
            }

            Question q;
            if (!_model.GetOneQuestion(question_id, q))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }

            Json::Value compile_value;
            compile_value["id"] = question_id;
            compile_value["code"] = code;
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);

            std::string out_json;
            // Forward custom_input and custom_output to judge path
            Judge(question_id, compile_string, &out_json, test_input, test_output);

            Json::Value judge_result;
            Json::Reader reader;
            if (!reader.parse(out_json, judge_result))
            {
                *err_code = "JUDGE_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["status"] = judge_result.isMember("status") ? judge_result["status"].asString() : "UNKNOWN";
            (*result)["desc"] = judge_result.isMember("desc") ? judge_result["desc"].asString() : "";
            if (judge_result.isMember("stdout"))
            {
                (*result)["stdout"] = judge_result["stdout"].asString();
            }
            if (judge_result.isMember("stderr"))
            {
                (*result)["stderr"] = judge_result["stderr"].asString();
            }
            (*result)["input"] = test_input;
            (*result)["expected_output"] = test_output;
            return true;
        }

        bool GetUserStats(const User& current_user, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
                return false;

            Json::Value stats;
            if (!_model.GetUserStats(current_user.uid, &stats))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["stats"] = stats;
            return true;
        }

    private:
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
    };
};
