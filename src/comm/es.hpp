#pragma once
#include <elasticlient/client.h>
#include <cpr/response.h>
#include <json/json.h>
#include <json/value.h>
#include <json/writer.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include "config.h"
#include "logger.hpp"

namespace oj::es
{
    struct Response 
    {
        bool status = true;  // 0 表示成功，负值表示错误
        std::string errmsg;  // 错误信息
        
        Response() : status(true) {}
        Response(int s, const std::string& msg = "") : status(s), errmsg(msg) {}
    };

    using namespace oj::logger;
    inline bool Serialize(const Json::Value &val, std::string* dst)
    {
        Json::StreamWriterBuilder swb;
        swb.settings_["emitUTF8"] = true;
        std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());
        std::stringstream ss;
        int ret = sw->write(val, &ss);
        if (ret != 0) 
        {
            LOG_ERROR("Json序列化失败!");
            return false;
        }
        *dst = ss.str();
        return true;
    }
    inline bool UnSerialize(const std::string &src, Json::Value *val)
    {
        Json::CharReaderBuilder crb;
        std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
        std::string err;
        bool ret = cr->parse(src.c_str(), src.c_str() + src.size(), val, &err);
        if (ret == false) 
        {
            LOG_ERROR("json反序列化失败:{}!",err);
            return false;
        }
        return true;
    }

    inline std::shared_ptr<elasticlient::Client> es_client;
    inline bool InitESClient(const std::string& host = es_host)
    {
        es_client = std::make_shared<elasticlient::Client>(std::vector<std::string>{host});
        return true;
    }

    class ESIndex
    {
    public:
        ESIndex(std::shared_ptr<elasticlient::Client> client = es_client)
        :_client(client)
        {
            _settings["number_of_shards"] = 1;
            _settings["number_of_replicas"] = 0;
            _settings["analysis"]["analyzer"]["ik"]["tokenizer"] = "ik_max_word";
            _mappings["dynamic"] = true;
        }
        void setSettings(const std::string& analysis,int number_of_shards = 1,int number_of_replicas = 0)
        {
            _settings["analysis"]["analyzer"]["ik"]["tokenizer"] = analysis;
            _settings["number_of_shards"] = number_of_shards;
            _settings["number_of_replicas"] = number_of_replicas;
        }
        void setProperties(const std::string& name,const std::string& type,const std::string& analyzer = "ik_max_word")
        {
            _properties[name]["type"] = type;
            if(!analyzer.empty())
            {
                _properties[name]["analyzer"] = analyzer;
            }
        }
        Response create(const std::string& name)
        {
            Response rep;
            if(!_properties.isNull())
            {
                _mappings["properties"] = _properties;
            }
            Json::Value json_out;
            json_out["settings"] = _settings;
            json_out["mappings"] = _mappings;
            std::string out;
            if(!Serialize(json_out,&out))
            {
                rep.status = false;
                rep.errmsg = "Json序列化失败";
                return rep;
            }
            try 
            {
                if (!_client) { rep.status = false; rep.errmsg = "ES client未初始化"; return rep; }
                auto rsp = _client->index(name, "", "", out);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ERROR("创建ES索引 {} 失败，响应状态码异常: {}", name, rsp.status_code);
                    rep.status = false;
                    rep.errmsg = "响应状态码异常: " + std::to_string(rsp.status_code);
                    return rep;
                }
            } catch(std::exception &e) 
            {
                LOG_ERROR("创建ES索引 {} 失败: {}", name, e.what());
                rep.status = false;
                rep.errmsg = e.what();
                return rep;
            }
            rep.status = true;
            return rep;
        }
    private:
        std::shared_ptr<elasticlient::Client> _client;
        Json::Value _settings;
        Json::Value _mappings;
        Json::Value _properties;
    };

    class ESInsert
    {
    public:
        ESInsert(std::shared_ptr<elasticlient::Client> client = es_client)
        :_client(client)
        { }
        template<class T>
        ESInsert& add(const std::string& key,const T& value)
        {
            _insert[key] = value;
            return *this;
        }
        template<class T>
        ESInsert& operator+(const std::pair<std::string, T>& kv)
        {
            _insert[kv.first] = kv.second;
            return *this;
        }
        Response insert(const std::string& name,const std::string& type,const std::string& id)
        {
            Response rep;
            if (!_client) { rep.status = false; rep.errmsg = "ES client未初始化"; return rep; }
            std::string out;
            if(!Serialize(_insert,&out))
            {
                rep.status = false;
                rep.errmsg = "Json序列化失败";
                return rep;
            }
            try 
            {
                auto rsp = _client->index(name, type, id, out);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ERROR("ES插入 {} 失败，响应状态码异常: {}", out, rsp.status_code);
                    rep.status = false;
                    rep.errmsg = "响应状态码异常: " + std::to_string(rsp.status_code);
                    return rep;
                }
            } catch(std::exception &e) 
            {
                LOG_ERROR("ES插入 {} 失败: {}", out, e.what());
                rep.status = false;
                rep.errmsg = e.what();
                return rep;
            }
            rep.status = true;
            return rep;
        }
        Response batchInsert(const std::string& str)
        {
            Response rep;
            if (!_client) { rep.status = false; rep.errmsg = "ES client未初始化"; return rep; }
            try 
            {
                auto rsp = _client->performRequest(elasticlient::Client::HTTPMethod::POST,"_bulk", str);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ERROR("ES批量插入 {} 失败，响应状态码异常: {}", str, rsp.status_code);
                    rep.status = false;
                    rep.errmsg = "响应状态码异常: " + std::to_string(rsp.status_code);
                    return rep;
                }
            } catch(std::exception &e) 
            {
                LOG_ERROR("ES批量插入 {} 失败: {}", str, e.what());
                rep.status = false;
                rep.errmsg = e.what();
                return rep;
            }
            rep.status = true;
            return rep;
        }
    private:
        std::shared_ptr<elasticlient::Client> _client;
        Json::Value _insert;
    };

    class ESRemove
    {
    public:
        ESRemove(std::shared_ptr<elasticlient::Client> client = es_client)
        :_client(client)
        { }
        Response remove(const std::string& name,const std::string& type,const std::string& id)
        {
            Response rep;
            if (!_client) { rep.status = false; rep.errmsg = "ES client未初始化"; return rep; }
            try 
            {
                auto rsp = _client->remove(name, type, id);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ERROR("删除数据 {} 失败，响应状态码异常: {}", id, rsp.status_code);
                    rep.status = false;
                    rep.errmsg = "响应状态码异常: " + std::to_string(rsp.status_code);
                    return rep;
                }
            } 
            catch(std::exception &e) 
            {
                LOG_ERROR("删除数据 {} 失败: {}", id, e.what());
                rep.status = false;
                rep.errmsg = e.what();
                return rep;
            }
            rep.status = true;
            return rep;
        }
    private:
        std::shared_ptr<elasticlient::Client> _client;
    };

    class ESQuery
    {
    public:
        ESQuery(std::shared_ptr<elasticlient::Client> client = es_client)
        :_client(client)
        { }
        void addMust(const std::string& key,const std::string& value)
        {
            Json::Value match;
            match["match"][key] = value;
            _must.append(match);
        }
        void addMustNot(const std::string& key,const std::vector<std::string>& value)
        {
            Json::Value terms;
            for(auto & it : value)
            {
                terms["terms"][key].append(it);
            }
            _must_not.append(terms);
        }
        void addShould(const std::string& key,const std::string& value)
        {
            Json::Value match;
            match["match"][key] = value;
            _should.append(match);
        }
        void addFilter(const std::string& key,const std::vector<std::string>& value)
        {
            Json::Value terms;
            for(auto & it : value)
            {
                terms["terms"][key].append(it);
            }
            _filter.append(terms);
        }
        void addFilterTimeRange(const std::string& key,uint64_t start,uint64_t  end)
        {
            _filter["range"][key]["gte"] = start;
            _filter["range"][key]["lte"] = end;
        }
        Response query(const std::string& name,const std::string& type,Json::Value* value)
        {
            Response rep;
            if (!_client) { rep.status = false; rep.errmsg = "ES client未初始化"; return rep; }
            Json::Value body;
            if(!_must.empty()) body["must"] = _must;
            if(!_must_not.empty()) body["must_not"] = _must_not;
            if(!_should.empty()) body["should"] = _should;
            if(!_filter.empty()) body["filter"] = _filter;
            Json::Value json_out;
            json_out["query"]["bool"] = body;
            std::string out;
            if(!Serialize(json_out,&out))
            {
                rep.status = false;
                rep.errmsg = "Json序列化失败";
                return rep;
            }
            std::string in;
            try 
            {
                auto rsp = _client->search(name, type, out);
                if (rsp.status_code < 200 || rsp.status_code >= 300) 
                {
                    LOG_ERROR("检索数据 {} 失败，响应状态码异常: {}", out, rsp.status_code);
                    rep.status = false;
                    rep.errmsg = "响应状态码异常: " + std::to_string(rsp.status_code);
                    return rep;
                }
                in = rsp.text;
            } 
            catch(std::exception &e) 
            {
                LOG_ERROR("检索数据 {} 失败: {}", out, e.what());
                rep.status = false;
                rep.errmsg = e.what();
                return rep;
            }
            if(!UnSerialize(in,value))
            {
                rep.status = false;
                rep.errmsg = "Json反序列化失败";
                return rep;
            }
            rep.status = true;
            return rep;
        }
    private:
        std::shared_ptr<elasticlient::Client> _client;
        Json::Value _must;
        Json::Value _must_not;
        Json::Value _should;
        Json::Value _filter;
    };
}
