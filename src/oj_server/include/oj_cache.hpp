#include <Logger/logstrategy.h>
#include <httplib.h>
#include <jsoncpp/json/value.h>
#include <string>
#include <sstream>
#include <ctime>
#include <random>
#include <sw/redis++/redis++.h>
#include "../../comm/comm.hpp"

using namespace sw::redis;
using namespace ns_log;

namespace ns_cache
{
    struct Question
    {
        std::string number; //题号
        std::string title;  //标题
        std::string star;   //难度
        std::string desc;   //描述
        int cpu_limit;      //时间限制
        int memory_limit;   //内存限制
        std::string create_time; //创建时间
        std::string update_time; //更新时间
    };

    struct QueryStruct
    {
        std::string id;
        std::string title;
        std::string difficulty;

        QueryStruct() = default;

        std::string ToString() const
        {
            std::ostringstream oss;
            oss << "id=" << id
                << ";title=" << title
                << ";difficulty=" << difficulty;
            return oss.str();
        }

        std::string ToJsonString() const
        {
            Json::Value json_value;
            json_value["id"] = id;
            json_value["title"] = title;
            json_value["difficulty"] = difficulty;
            Json::FastWriter writer;
            return writer.write(json_value);
        }
    };
    class Cache
    {
    public:
        Cache(const std::string& redis = redis_addr,
              const std::string& business = "oj",
              const std::string& env = "prod",
              const std::string& version = "v1")
        :_redis(Redis(redis)), _business(business), _env(env), _version(version)
        {}

        bool GetQuestion(const std::string& id,Question& question)
        {
            std::string key = _business + ":" + _env + ":" + _version + ":question:detail:" + id;
            try 
            {
                if (_redis.exists(key))
                {
                    OptionalString result = _redis.get(key);
                    if (result)
                    {
                        std::string json_str = result.value();
                        if (json_str == "__NULL__")
                        {
                            logger(ns_log::INFO) << "Null cache hit for question " << id;
                            return false;
                        }

                        Json::CharReaderBuilder builder;
                        Json::Value json_value;
                        std::istringstream ss(json_str);
                        if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                        {
                            question.number = json_value["number"].asString();
                            question.title = json_value["title"].asString();
                            question.star = json_value["star"].asString();
                            question.desc = json_value["desc"].asString();
                            question.cpu_limit = json_value["cpu_limit"].asInt();
                            question.memory_limit = json_value["memory_limit"].asInt();
                            question.create_time = json_value["create_time"].asString();
                            question.update_time = json_value["update_time"].asString();

                            logger(ns_log::INFO) << "Cache hit for question " << id;
                            return true;
                        }
                    }

                    logger(ns_log::WARNING) << "Invalid cache payload for question " << id;
                    return false;
                }
                else
                {
                    logger(ns_log::INFO) << "Cache miss for question " << id;
                    return false;
                }
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
                return false;
            
            }
        }

        void SetQuestion(Question& question)
        {
            std::string key = _business + ":" + _env + ":" + _version + ":question:detail:" + question.number;
            Json::Value json_value;
            json_value["number"] = question.number;
            json_value["title"] = question.title;
            json_value["star"] = question.star;
            json_value["desc"] = question.desc;
            json_value["cpu_limit"] = question.cpu_limit;
            json_value["memory_limit"] = question.memory_limit;
            json_value["create_time"] = question.create_time;
            json_value["update_time"] = question.update_time;
            Json::FastWriter writer;
            std::string json_str = writer.write(json_value);
            try 
            {
                _redis.setex(key, BuildJitteredTtl(3600, 300), json_str);
                logger(ns_log::INFO) << "Question " << question.number << " cached successfully";
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }

        void SetQuestionNotFound(const std::string& id)
        {
            std::string key = _business + ":" + _env + ":" + _version + ":question:detail:" + id;
            try
            {
                _redis.setex(key, BuildJitteredTtl(60, 30), "__NULL__");
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }

        bool GetAllQuestions(int page,
                             int size,
                             const std::string& list_version,
                             int* total_count,
                             int* total_pages,
                             std::vector<Question>& questions,
                             const QueryStruct& query_hash)
        {
            std::string key = _business + ":" + _env + ":" + _version + ":question:list:" + query_hash.ToString() + ":" + std::to_string(page) + ":" + std::to_string(size) + ":" + list_version;
            try 
            {
                if (_redis.exists(key))
                {
                    OptionalString result = _redis.get(key);
                    if (result)
                    {
                        std::string json_str = result.value();
                        Json::CharReaderBuilder builder;
                        Json::Value json_value;
                        std::istringstream ss(json_str);
                        if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                        {
                            if (total_count != nullptr && json_value.isMember("total_count"))
                            {
                                *total_count = json_value["total_count"].asInt();
                            }

                            if (total_pages != nullptr && json_value.isMember("total_pages"))
                            {
                                *total_pages = json_value["total_pages"].asInt();
                            }

                            if(json_value.isObject() && json_value.isMember("questions") && json_value["questions"].isArray())
                            {
                                for (const auto& item : json_value["questions"])
                                {
                                    Question q;
                                    q.number = item["number"].asString();
                                    q.title = item["title"].asString();
                                    q.star = item["star"].asString();
                                    questions.push_back(q);
                                }
                            }
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for question list " << key;
                    return true;
                }
                else
                {
                    logger(ns_log::INFO) << "Cache miss for question list " << key;
                    return false;
                }
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
                return false;
            }
        }

        void SetAllQuestions(int page,
                             int size,
                             const std::string& list_version,
                             int total_count,
                             int total_pages,
                             std::vector<Question>& questions,
                             const QueryStruct& query_hash = QueryStruct())
        {
            std::string key = _business + ":" + _env + ":" + _version + ":question:list:" + query_hash.ToString() + ":" + std::to_string(page) + ":" + std::to_string(size) + ":" + list_version;
            Json::Value array_value(Json::arrayValue);
            for (const auto& q : questions)
            {
                Json::Value item;
                item["number"] = q.number;
                item["title"] = q.title;
                item["star"] = q.star;
                array_value.append(item);
            }
            Json::Value root(Json::objectValue);
            root["query"] = query_hash.ToJsonString();
            root["page"] = page;
            root["size"] = size;
            root["total_count"] = total_count;
            root["total_pages"] = total_pages;
            root["questions"] = array_value;
            root["list_version"] = list_version;
            Json::FastWriter writer;
            std::string json_str = writer.write(root);
            try 
            {
                int ttl = total_count <= 0 ? BuildJitteredTtl(60, 30) : BuildJitteredTtl(600, 120);
                _redis.setex(key, ttl, json_str);
                logger(ns_log::INFO) << "Question list " << key << " cached successfully";
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }

        std::string GetListVersion()
        {
            const std::string key = _business + ":" + _env + ":" + _version + ":question:list:version";
            try
            {
                OptionalString value = _redis.get(key);
                if (value)
                {
                    return value.value();
                }
                _redis.set(key, "1");
                return "1";
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
                return "1";
            }
        }

        std::string BumpListVersion()
        {
            const std::string key = _business + ":" + _env + ":" + _version + ":question:list:version";
            try
            {
                long long cur = _redis.incr(key);
                return std::to_string(cur);
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
                return "1";
            }
        }

    private:
        int BuildJitteredTtl(int base_ttl, int jitter)
        {
            if (base_ttl <= 1)
                return 1;
            if (jitter <= 0)
                return base_ttl;

            static thread_local std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
            std::uniform_int_distribution<int> dist(0, jitter);
            return base_ttl + dist(rng);
        }

        Redis _redis;
        std::string _business;
        std::string _env;
        std::string _version;
    };
}

