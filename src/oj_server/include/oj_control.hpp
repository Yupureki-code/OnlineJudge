#pragma once

#include <cstdio>
#include <assert.h>
#include <string>
#include <vector>
#include <mutex>
#include <assert.h>
#include <memory>
#include "oj_model.hpp"
#include "oj_view.hpp"
#include "oj_session.hpp"
#include <httplib.h>
#include <jsoncpp/json/json.h>
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
        Model* GetModel() { return &_model; }
        
        //加载一页内的全部题目
        bool AllQuestions(std::string *html, int page = 1)
        {
            bool ret = true;
            std::vector<struct Question> all;
            if (_model.GetAllQuestions(&all))
            {
                //按照题目编号排序
                sort(all.begin(), all.end(), [](const struct Question &q1, const struct Question &q2){
                    return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
                });
                
                const int questionsPerPage = 5;
                int totalPages = (all.size() + questionsPerPage - 1) / questionsPerPage;
                if (page < 1) page = 1;
                if (page > totalPages) page = totalPages;
                
                int startIndex = (page - 1) * questionsPerPage;
                int endIndex = std::min(startIndex + questionsPerPage, (int)all.size());
                
                std::vector<struct Question> pageQuestions;
                for (int i = startIndex; i < endIndex; i++) {
                    pageQuestions.push_back(all[i]);
                }
                
                _view.AllExpandHtml(pageQuestions, html, page, totalPages);
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
            struct Question q;
            if (_model.GetOneQuestion(number, &q))
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
        void Judge(const std::string& number,const std::string& in_json,std::string* out_json)
        {
            struct Question q;
            _model.GetOneQuestion(number, &q);
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
    private:
        Model _model;
        View _view;
        CentralConsole _console;
        SessionManager _session_manager;

    public:
        std::string CreateSession(int user_id, const std::string& email)
        {
            return _session_manager.CreateSession(user_id, email);
        }

        void DestroySession(const std::string& session_id)
        {
            _session_manager.DestroySession(session_id);
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

