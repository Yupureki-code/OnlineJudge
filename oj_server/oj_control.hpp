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
#include <httplib.h>
#include <jsoncpp/json/json.h>

namespace ns_control
{
    using namespace ns_model;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_view;

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
        std::string _ip;
        uint16_t _port;
        uint32_t _load = 0;
        std::shared_ptr<std::mutex> _mtx;
    };

    const std::string machine_conf = "./conf/service_machine.conf";

    class CentralConsole
    {
    public:
        CentralConsole()
        {
            assert(LoadConf(machine_conf));
            logger(ns_log::INFO)<<"加载 "<<machine_conf<<" 成功";
        }
        bool LoadConf(const std::string& conf)
        {
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
                if(cur < load)
                {
                    load = cur;
                    *id = it;
                    *m = &_machines[*id];
                }
            }
            _mtx.unlock();
            return true;
        }
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
        void OnlineMachine()
        {
            _mtx.lock();
            _online.insert(_online.end(), _offline.begin(), _offline.end());
            _offline.erase(_offline.begin(), _offline.end());
            _mtx.unlock();
            logger(INFO) << "所有的主机都上线啦!" << "\n";
        }
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
        std::vector<Machine> _machines;
        std::vector<int> _online;
        std::vector<int> _offline;
        std::mutex _mtx;
    };
    
    class Control
    {
    public:
        bool AllQuestions(std::string *html)
        {
            bool ret = true;
            std::vector<struct Question> all;
            if (_model.GetAllQuestions(&all))
            {
                sort(all.begin(), all.end(), [](const struct Question &q1, const struct Question &q2){
                    return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
                });
                _view.AllExpandHtml(all, html);
            }
            else
            {
                *html = "获取题目失败, 形成题目列表失败";
                ret = false;
            }
            return ret;
        }
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
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.memory_limit;
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
    private:
        Model _model;
        View _view;
        CentralConsole _console;
    };
};

