#pragma once

#include <Logger/logstrategy.h>
#include <jsoncpp/json/json.h>

#include <atomic>
#include <chrono>
#include <cctype>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <unistd.h>

namespace ns_oj_log {

inline long long NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

enum class LogCategory
{
    access = 0,
    audit,
    query,
    cache,
    module,
    security,
    unknown
};

inline const char* CategoryToString(LogCategory category)
{
    switch (category)
    {
        case LogCategory::access: return "access";
        case LogCategory::audit: return "audit";
        case LogCategory::query: return "query";
        case LogCategory::cache: return "cache";
        case LogCategory::module: return "module";
        case LogCategory::security: return "security";
        default: return "unknown";
    }
}

inline LogCategory CategoryFromString(const std::string& category)
{
    if (category == "access") return LogCategory::access;
    if (category == "audit") return LogCategory::audit;
    if (category == "query") return LogCategory::query;
    if (category == "cache") return LogCategory::cache;
    if (category == "module") return LogCategory::module;
    if (category == "security") return LogCategory::security;
    return LogCategory::unknown;
}

inline bool IsAsciiToken(const std::string& value)
{
    if (value.empty())
    {
        return false;
    }
    for (size_t i = 0; i < value.size(); ++i)
    {
        unsigned char ch = static_cast<unsigned char>(value[i]);
        if (!(std::isalnum(ch) || ch == '_' || ch == '.' || ch == '-' || ch == '/'))
        {
            return false;
        }
    }
    return true;
}

inline std::string NormalizeLevelString(const std::string& level)
{
    std::string out = level;
    for (size_t i = 0; i < out.size(); ++i)
    {
        out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    }
    return out;
}

inline std::string LevelToString(ns_log::LogLevel level)
{
    return NormalizeLevelString(ns_log::LogLevelToString(level));
}

inline bool IsSupportedLevelString(const std::string& level)
{
    std::string normalized = NormalizeLevelString(level);
    return normalized == "DEBUG"
        || normalized == "INFO"
        || normalized == "WARNING"
        || normalized == "ERROR"
        || normalized == "FATAL";
}

inline bool IsOptionalAsciiToken(const std::string& value, size_t max_len)
{
    return value.empty() || (value.size() <= max_len && IsAsciiToken(value));
}

inline bool IsRequiredAsciiToken(const std::string& value, size_t max_len)
{
    return !value.empty() && value.size() <= max_len && IsAsciiToken(value);
}

inline std::string GenerateEventId()
{
    static std::atomic<unsigned long long> seq(0);
    unsigned long long next = seq.fetch_add(1, std::memory_order_relaxed) + 1;
    std::size_t tid_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::ostringstream oss;
    oss << "elog_" << NowMs() << "_" << static_cast<long long>(::getpid())
        << "_" << next << "_" << tid_hash;
    return oss.str();
}

struct LogStruct
{
    std::string schema_version = "v1";
    std::string event_id;
    long long ts_ms = 0;
    std::string service;
    std::string instance;
    std::string level = "INFO";
    LogCategory category = LogCategory::unknown;
    std::string request_id;
    std::string trace_id;
    int pid = 0;
    int tid = 0;
    int uid = 0;
    int admin_id = 0;
    std::string action;
    std::string resource_type;
    std::string route;
    std::string method;
    int status = 0;
    int latency_ms = 0;
    std::string message;
    Json::Value tags = Json::objectValue;
    Json::Value payload = Json::objectValue;

    Json::Value ToJsonValue() const
    {
        Json::Value value(Json::objectValue);
        value["schema_version"] = schema_version;
        value["event_id"] = event_id;
        value["ts_ms"] = Json::Int64(ts_ms);
        value["service"] = service;
        value["instance"] = instance;
        value["level"] = NormalizeLevelString(level);
        value["category"] = CategoryToString(category);
        value["request_id"] = request_id;
        value["trace_id"] = trace_id;
        value["pid"] = pid;
        value["tid"] = tid;
        value["uid"] = uid;
        value["admin_id"] = admin_id;
        value["action"] = action;
        value["resource_type"] = resource_type;
        value["route"] = route;
        value["method"] = method;
        value["status"] = status;
        value["latency_ms"] = latency_ms;
        value["message"] = message;
        value["tags"] = tags;
        value["payload"] = payload;
        return value;
    }

    std::string GetJsonString() const
    {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        return Json::writeString(builder, ToJsonValue());
    }

    bool ParseJsonValue(const Json::Value& value)
    {
        if (!value.isObject())
        {
            return false;
        }

        schema_version = value.get("schema_version", "v1").asString();
        event_id = value.get("event_id", "").asString();
        ts_ms = value.get("ts_ms", Json::Int64(0)).asInt64();
        service = value.get("service", "").asString();
        instance = value.get("instance", "").asString();
        level = NormalizeLevelString(value.get("level", "INFO").asString());
        category = CategoryFromString(value.get("category", "unknown").asString());
        request_id = value.get("request_id", "").asString();
        trace_id = value.get("trace_id", "").asString();
        pid = value.get("pid", 0).asInt();
        tid = value.get("tid", 0).asInt();
        uid = value.get("uid", 0).asInt();
        admin_id = value.get("admin_id", 0).asInt();
        action = value.get("action", "").asString();
        resource_type = value.get("resource_type", "").asString();
        route = value.get("route", "").asString();
        method = value.get("method", "").asString();
        status = value.get("status", 0).asInt();
        latency_ms = value.get("latency_ms", 0).asInt();
        message = value.get("message", "").asString();
        tags = value.isMember("tags") ? value["tags"] : Json::Value(Json::objectValue);
        payload = value.isMember("payload") ? value["payload"] : Json::Value(Json::objectValue);
        return true;
    }

    bool ParseJsonString(const std::string& json_str)
    {
        Json::CharReaderBuilder builder;
        Json::Value value;
        std::string errs;
        std::istringstream ss(json_str);
        if (!Json::parseFromStream(builder, ss, &value, &errs))
        {
            return false;
        }
        return ParseJsonValue(value);
    }

    bool Validate(std::string* err) const
    {
        if (schema_version != "v1")
        {
            if (err != nullptr) *err = "schema_version must be v1";
            return false;
        }
        if (ts_ms <= 0)
        {
            if (err != nullptr) *err = "ts_ms must be positive";
            return false;
        }
        if (service.empty() || service.size() > 32 || !IsAsciiToken(service))
        {
            if (err != nullptr) *err = "service must be a non-empty token up to 32 chars";
            return false;
        }
        if (!instance.empty() && (instance.size() > 64 || !IsAsciiToken(instance)))
        {
            if (err != nullptr) *err = "instance must be a token up to 64 chars";
            return false;
        }
        if (!IsSupportedLevelString(level))
        {
            if (err != nullptr) *err = "level is unsupported";
            return false;
        }
        if (category == LogCategory::unknown)
        {
            if (err != nullptr) *err = "category is unsupported";
            return false;
        }
        if (action.empty() || action.size() > 64 || !IsAsciiToken(action))
        {
            if (err != nullptr) *err = "action must be a non-empty token up to 64 chars";
            return false;
        }
        if (request_id.size() > 64)
        {
            if (err != nullptr) *err = "request_id too long";
            return false;
        }
        if (trace_id.size() > 64)
        {
            if (err != nullptr) *err = "trace_id too long";
            return false;
        }
        if (resource_type.size() > 32)
        {
            if (err != nullptr) *err = "resource_type too long";
            return false;
        }
        if (route.size() > 128)
        {
            if (err != nullptr) *err = "route too long";
            return false;
        }
        if (method.size() > 16)
        {
            if (err != nullptr) *err = "method too long";
            return false;
        }
        if (message.size() > 1024)
        {
            if (err != nullptr) *err = "message too long";
            return false;
        }
        if (!tags.isNull() && !tags.isObject())
        {
            if (err != nullptr) *err = "tags must be an object";
            return false;
        }
        if (payload.isNull())
        {
            if (category == LogCategory::audit)
            {
                if (err != nullptr) *err = "audit payload must be an object";
                return false;
            }
            return true;
        }
        if (category == LogCategory::audit)
        {
            if (request_id.empty())
            {
                if (err != nullptr) *err = "audit request_id is required";
                return false;
            }
            if (admin_id <= 0)
            {
                if (err != nullptr) *err = "audit admin_id must be positive";
                return false;
            }
            if (!IsRequiredAsciiToken(resource_type, 32))
            {
                if (err != nullptr) *err = "audit resource_type must be a non-empty token up to 32 chars";
                return false;
            }
            if (!payload.isObject())
            {
                if (err != nullptr) *err = "audit payload must be an object";
                return false;
            }
            const Json::Value& result = payload["result"];
            if (!result.isString() || !IsRequiredAsciiToken(result.asString(), 32))
            {
                if (err != nullptr) *err = "audit payload.result must be a non-empty token up to 32 chars";
                return false;
            }
            const Json::Value& operator_role = payload["operator_role"];
            if (!operator_role.isString() || !IsRequiredAsciiToken(operator_role.asString(), 32))
            {
                if (err != nullptr) *err = "audit payload.operator_role must be a non-empty token up to 32 chars";
                return false;
            }
            if (payload.isMember("detail_text"))
            {
                const Json::Value& detail_text = payload["detail_text"];
                if (!detail_text.isString() || detail_text.asString().size() > 8 * 1024)
                {
                    if (err != nullptr) *err = "audit payload.detail_text must be a string up to 8KB";
                    return false;
                }
            }
        }
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        if (Json::writeString(builder, payload).size() > 16 * 1024)
        {
            if (err != nullptr) *err = "payload too large";
            return false;
        }
        return true;
    }
};

} // namespace ns_oj_log