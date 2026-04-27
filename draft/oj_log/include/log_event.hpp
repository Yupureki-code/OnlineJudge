#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <jsoncpp/json/value.h>
#include <Logger/logstrategy.h>

namespace ns_log_event
{
    enum class LogCategory
    {
        access = 0,
        db_hit,
        audit,
        app
    };

    inline std::string CategoryToString(LogCategory c)
    {
        switch (c)
        {
            case LogCategory::access: return "access";
            case LogCategory::db_hit: return "db_hit";
            case LogCategory::audit: return "audit";
            case LogCategory::app: return "app";
            default: return "unknown";
        }
    }

    inline LogCategory CategoryFromString(const std::string& s)
    {
        if (s == "access") return LogCategory::access;
        if (s == "db_hit") return LogCategory::db_hit;
        if (s == "audit") return LogCategory::audit;
        return LogCategory::app;
    }

    inline long long NowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    struct LogEvent
    {
        // protocol envelope (business side)
        std::string schema_version = "v1";
        long long ts_ms = 0;
        std::string service;

        // classification
        LogCategory category = LogCategory::app;
        ns_log::LogLevel level = ns_log::INFO;

        // request context
        std::string request_id;
        std::string route;
        std::string method;
        int status = 0;
        int latency_ms = 0;
        std::string client_ip;

        // identity
        int uid = 0;
        int admin_id = 0;

        // semantics
        std::string action;
        std::string resource_type;
        Json::Value payload = Json::objectValue;
    };

    inline Json::Value ToJson(const LogEvent& e)
    {
        Json::Value root(Json::objectValue);
        root["schema_version"] = e.schema_version;
        root["ts_ms"] = Json::Int64(e.ts_ms);
        root["service"] = e.service;
        root["category"] = CategoryToString(e.category);
        root["level"] = ns_log::LogLevelToString(e.level);
        root["request_id"] = e.request_id;
        root["route"] = e.route;
        root["method"] = e.method;
        root["status"] = e.status;
        root["latency_ms"] = e.latency_ms;
        root["client_ip"] = e.client_ip;
        root["uid"] = e.uid;
        root["admin_id"] = e.admin_id;
        root["action"] = e.action;
        root["resource_type"] = e.resource_type;
        root["payload"] = e.payload;
        return root;
    }
}
