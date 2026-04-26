#pragma once

#include <Logger/logstrategy.h>
#include <atomic>
#include <httplib.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <memory>
#include <mutex>
#include <spdlog/common.h>
#include <string>
#include <unordered_map>

namespace spdlog {
class logger;
}

namespace ns_oj_log {
class LogServer {
public:
  enum LogCategory { query = 0, db_hit, audit, module };

  struct LogStruct 
  {
    std::string schema_version;          // 协议版本
    long long ts_ms;                     // 时间戳，单位毫秒
    std::string service;                 // 日志来源的服务名称
    spdlog::level::level_enum log_level; // 日志级别
    LogCategory category;                // 日志分类
    std::string request_id;              // 请求ID
    int pid;
    int tid;
    int uid;
    int admin_id;
    std::string action;
    std::string resource_type;
    Json::Value payload; // 日志载荷
    std::string GetJsonString()
    {
        Json::Value value;
        value["schema_version"] = schema_version;
        value["ts_ms"] = std::to_string(ts_ms);
        value["service"] = service;
        value["log_level"] = log_level;
        value["category"] = category;
        value["request_id"] = request_id;
        value["pid"] = pid;
        value["tid"] = tid;
        value["uid"] = uid;
        value["admin_id"] = admin_id;
        value["action"] = action;
        value["resource_type"] = resource_type;
        value["payload"] = payload;

        Json::FastWriter writer;
        return writer.write(value);
    }
    void PraseJsonString(const std::string& json_str)
    {
        Json::CharReaderBuilder builder;
        Json::Value value;
        std::string errs;
        std::istringstream ss(json_str);
        if (Json::parseFromStream(builder, ss, &value, &errs))
        {
            schema_version = value["schema_version"].asString();
            ts_ms = std::stoll(value["ts_ms"].asString());
            service = value["service"].asString();
            log_level = static_cast<spdlog::level::level_enum>(value["log_level"].asInt());
            category = static_cast<LogCategory>(value["category"].asInt());
            request_id = value["request_id"].asString();
            pid = value["pid"].asInt();
            tid = value["tid"].asInt();
            uid = value["uid"].asInt();
            admin_id = value["admin_id"].asInt();
            action = value["action"].asString();
            resource_type = value["resource_type"].asString();
            payload = value["payload"];
        }
    }
  };
  LogServer();
  void RegisterRoutes(httplib::Server &svr);
  bool Start(const std::string &host, int port);

private:
  bool ParseJsonBody(const httplib::Request &req, Json::Value *out) const;
  ns_log::LogLevel ParseLogLevel(const Json::Value &event) const;
  bool PersistEvent(const Json::Value &enriched);
  bool PersistServiceLog(const Json::Value &enriched, ns_log::LogLevel level);
  std::string BuildServiceLogLine(const Json::Value &enriched,
                                  ns_log::LogLevel level) const;
  std::string PickServiceLogPath(const Json::Value &enriched) const;
  std::shared_ptr<spdlog::logger>
  GetOrCreateServiceLogger(const std::string &service_log_path);
  void RotateStoreIfNeededLocked();
  std::uint64_t GetFileSizeBytes(const std::string &file_path) const;
  std::string BuildEventId();
  long long NowMs() const;

private:
  std::atomic<unsigned long long> _seq{0};
  std::atomic<unsigned long long> _ingest_ok{0};
  std::atomic<unsigned long long> _ingest_bad_json{0};
  std::atomic<unsigned long long> _ingest_persist_fail{0};
  std::string _store_path;
  std::string _service_log_root = "./log/";
  std::uint64_t _max_store_bytes = 10 * 1024 * 1024; // 10MB
  int _rotate_keep = 3;
  std::mutex _store_mtx;
  std::mutex _service_log_mtx;
  std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>
      _service_loggers;
};
class LogClient
{
private:
public:
    LogClient(const std::string& host, uint16_t port)
        : _host(host), _port(port)
    {
        httplib::Client cli(_host, _port);
        auto res = cli.Get("/api/logs/health");
        if (!res || res->status != 200)        
        {
            throw std::runtime_error("oj_log health check failed");
        }
    }
    int SendLog(ns_oj_log::LogServer::LogStruct log_struct)
    {
        
    }
private:
    std::string _host;
    uint16_t _port;
};
} // namespace ns_oj_log
