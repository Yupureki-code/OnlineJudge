#include <Logger/logstrategy.h>
#include <httplib.h>
#include <jsoncpp/json/value.h>
#include <memory>
#include <mysql/mysql.h>
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
    struct KeyContext
    {
        QueryStruct _query_hash;
        int page;
        int size;
        std::string list_version;
        std::string _page_name;
    };
    class Cache
    {
    public:
        class CacheKey
        {
        public:
            enum class PageType{
                kData,
                kHtml
            };
            virtual void Build(const KeyContext& context) = 0;
            virtual std::string GetCacheKeyString(Cache* cache) const = 0;
            void SetPageType(PageType page_type)
            {
                _page_type = page_type;
            }
            std::string PageTypeToString() const
            {
                switch (_page_type)
                {
                    case PageType::kData: return "data";
                    case PageType::kHtml: return "html";
                    default: return "unknown";
                }
            }
            PageType GetPageType() const
            {
                return _page_type;
            }
        private:
            PageType _page_type;
        };

        class CacheStaticKey : public CacheKey
        {
        public:
            void Build(const KeyContext& context)override
            {
                _page_name = context._page_name;
            }
            std::string GetCacheKeyString(Cache* cache) const override
            {
                return cache->_business + ":" + cache->_env + ":" + cache->_version + ":static:" + PageTypeToString() + ":" + _page_name;
            }
            std::string GetPageName() const
            {
                return _page_name;
            }
        private:
            std::string _page_name;
        };

        class CacheListKey : public CacheKey
        {
        public:
            void Build(const KeyContext& context)override
            {   
                _query_hash = context._query_hash;
                _page = context.page;
                _size = context.size;
                _list_version = context.list_version;
            }
            std::string GetCacheKeyString(Cache* cache) const override
            {                
                return cache->_business + ":" + cache->_env + ":" + cache->_version + ":list:" + PageTypeToString() + ":" + _query_hash.ToString() + ":" + std::to_string(_page) + ":" + std::to_string(_size) + ":" + _list_version;
            }
            const QueryStruct& GetQueryHash() const
            {
                return _query_hash;
            }
            int GetPage() const
            {                
                return _page;
            }
            int GetSize() const
            {                
                return _size;
            }
            std::string GetListVersion() const
            {                
                return _list_version;
            }
        private:
            QueryStruct _query_hash;
            int _page;
            int _size;
            std::string _list_version;
        };

        class CacheDetailKey : public CacheKey
        {
        public:
            void Build(const KeyContext& context)override
            {
                _page_name = context._page_name;
            }
            std::string GetCacheKeyString(Cache* cache) const override
            {
                return cache->_business + ":" + cache->_env + ":" + cache->_version + ":detail:" + PageTypeToString() + ":" + _page_name;
            }
            std::string GetPageName() const
            {
                return _page_name;
            }
        private:
            std::string _page_name;
        };
        Cache(const std::string& redis = redis_addr,
              const std::string& business = "oj",
              const std::string& env = "prod",
              const std::string& version = "v1")
        :_redis(Redis(redis)), _business(business), _env(env), _version(version)
        {}

        bool GetQuestion(const std::shared_ptr<CacheDetailKey>& key, Question& question)
        {
            std::string key_str = key->GetCacheKeyString(this);
            try 
            {
                if (_redis.exists(key_str))
                {
                    OptionalString result = _redis.get(key_str);
                    if (result)
                    {
                        std::string json_str = result.value();
                        if (json_str == "__NULL__")
                        {
                            logger(ns_log::INFO) << "Null cache hit for question " << key_str;
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

                            logger(ns_log::INFO) << "Cache hit for question " << key_str;
                            return true;
                        }
                    }

                    logger(ns_log::WARNING) << "Invalid cache payload for question " << key_str;
                    return false;
                }
                else
                {
                    logger(ns_log::INFO) << "Cache miss for question " << key_str;
                    return false;
                }
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
                return false;
            
            }
        }

        void SetQuestion(const std::shared_ptr<CacheDetailKey>& key, Question& question)
        {
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
                _redis.setex(key->GetCacheKeyString(this), BuildJitteredTtl(3600, 300), json_str);
                logger(ns_log::INFO) << "Question " << question.number << " cached successfully";
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }

        // 设置题目不存在的缓存，避免缓存穿透
        void SetQuestionNotFound(const std::shared_ptr<CacheDetailKey>& key, const std::string& id)
        {
            try
            {
                _redis.setex(key->GetCacheKeyString(this), BuildJitteredTtl(60, 30), "__NULL__");
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }

        bool GetAllQuestions(const std::shared_ptr<CacheListKey>& key, std::vector<Question>& questions, int * total_count, int* total_pages)
        {
            std::string key_str = key->GetCacheKeyString(this);
            try 
            {
                if (_redis.exists(key_str))
                {
                    OptionalString result = _redis.get(key_str);
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
                    logger(ns_log::INFO) << "Cache hit for question list " << key->GetCacheKeyString(this);
                    return true;
                }
                else
                {
                    logger(ns_log::INFO) << "Cache miss for question list " << key->GetCacheKeyString(this)  ;
                    return false;
                }
            } 
            catch (const sw::redis::Error &e) 
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
                return false;
            }
        }

        void SetAllQuestions(const std::shared_ptr<CacheListKey>& key,
                             std::vector<Question>& questions,
                             int total_count,
                             int total_pages)
        {
            std::string key_str = key->GetCacheKeyString(this);
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
            root["query"] = key->GetQueryHash().ToJsonString();
            root["page"] = key->GetPage();
            root["size"] = key->GetSize();
            root["total_count"] = total_count;
            root["total_pages"] = total_pages;
            root["questions"] = array_value;
            root["list_version"] = key->GetListVersion();
            Json::FastWriter writer;
            std::string json_str = writer.write(root);
            try 
            {
                int ttl = total_count <= 0 ? BuildJitteredTtl(60, 30) : BuildJitteredTtl(600, 120);
                _redis.setex(key->GetCacheKeyString(this), ttl, json_str);
                logger(ns_log::INFO) << "Question list " << key->GetCacheKeyString(this) << " cached successfully";
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
        bool GetHtmlPage(std::string *html, const std::shared_ptr<CacheKey>& key)
        {
            if(key->GetPageType() != CacheKey::PageType::kHtml)
            {
                logger(ns_log::ERROR) << "Invalid cache key type for GetHtmlPage: " << key->PageTypeToString();
                return false;
            }
            std::string key_str = key->GetCacheKeyString(this);
             try
             {
                OptionalString result = _redis.get(key_str);
                if (result)
                {
                    *html = result.value();
                    logger(ns_log::INFO) << "Html page " << key_str << " cache hit";
                    return true;
                }
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
             logger(ns_log::INFO) << "Html page " << key_str << " cache miss";
             return false;
        }
        void SetHtmlPage(std::string *html, const std::shared_ptr<CacheKey>& key)
        {
            if(key->GetPageType() != CacheKey::PageType::kHtml)
            {
                logger(ns_log::ERROR) << "Invalid cache key type for SetHtmlPage: " << key->PageTypeToString();
                return;
            }
            try
            {
                _redis.setex(key->GetCacheKeyString(this), BuildJitteredTtl(21600, 3600), *html);
                logger(ns_log::INFO) << "Html page " << key->GetCacheKeyString(this) << " cached successfully";
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }
        void InvalidatePage(const std::shared_ptr<CacheKey>& key)
        {
            if (!key)
            {
                return;
            }
            try
            {
                std::string key_str = key->GetCacheKeyString(this);
                long long deleted = _redis.del(key_str);
                logger(ns_log::INFO) << "Invalidate cache key=" << key_str << " deleted=" << deleted;
            }
            catch (const sw::redis::Error &e)
            {
                logger(ns_log::ERROR) << "Redis error: " << e.what();
            }
        }
        std::shared_ptr<CacheListKey> BuildListCacheKey(const QueryStruct& query_struct, int page, int size, const std::string& list_version, CacheKey::PageType page_type)
        {
            std::shared_ptr<CacheListKey> key = std::make_shared<CacheListKey>();
            key->SetPageType(page_type);
            KeyContext context;
            context._query_hash = query_struct;
            context.page = page;
            context.size = size;
            context.list_version = list_version;
            key->Build(context);
            return key;
        }
        std::shared_ptr<CacheDetailKey> BuildDetailCacheKey(const std::string& _page_name, CacheKey::PageType page_type)
        {
            std::shared_ptr<CacheDetailKey> key = std::make_shared<CacheDetailKey>();
            key->SetPageType(page_type);
            KeyContext context;             
            context._page_name = _page_name;
            key->Build(context);
            return key;
        }
        std::shared_ptr<CacheStaticKey> BuildStaticCacheKey(const std::string& _page_name, CacheKey::PageType page_type)
        {            
            std::shared_ptr<CacheStaticKey> key = std::make_shared<CacheStaticKey>();
            key->SetPageType(page_type);
            KeyContext context;             
            context._page_name = _page_name;
            key->Build(context);
            return key;
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

