#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/fmt.h>

#include "../include/oj_log.hpp"

#include <Logger/logstrategy.h>
#include "../../comm/config.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <cstdio>
#include <cctype>

using namespace httplib;
using namespace ns_log;

namespace ns_oj_log
{
namespace
{
int GetEnvIntOrDefault(const char* name, int default_value)
{
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0')
    {
        return default_value;
    }
    char* end = nullptr;
    long parsed = std::strtol(value, &end, 10);
    if (end == value)
    {
        return default_value;
    }
    if (parsed > 2147483647L)
    {
        return 2147483647;
    }
    if (parsed < -2147483647L)
    {
        return -2147483647;
    }
    return static_cast<int>(parsed);
}

std::string SanitizeLoggerName(const std::string& in)
{
    std::string out = in;
    for (char& c : out)
    {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-'))
        {
            c = '_';
        }
    }
    return out;
}

spdlog::level::level_enum ToSpdLevel(ns_log::LogLevel level)
{
    switch (level)
    {
        case ns_log::DEBUG: return spdlog::level::debug;
        case ns_log::INFO: return spdlog::level::info;
        case ns_log::WARNING: return spdlog::level::warn;
        case ns_log::ERROR: return spdlog::level::err;
        case ns_log::FATAL: return spdlog::level::critical;
        default: return spdlog::level::info;
    }
}
}

LogServer::LogServer()
    : _store_path("./log/oj_log_events.jsonl")
{
    _service_log_root = LOG_PATH;

    const char* env_store = std::getenv("OJ_LOG_STORE_PATH");
    if (env_store != nullptr)
    {
        std::string p(env_store);
        if (!p.empty())
        {
            _store_path = p;
        }
    }

    _max_store_bytes = static_cast<std::uint64_t>(GetEnvIntOrDefault("OJ_LOG_MAX_STORE_MB", 10)) * 1024ULL * 1024ULL;
    if (_max_store_bytes < 1024ULL * 1024ULL)
    {
        _max_store_bytes = 1024ULL * 1024ULL; // at least 1MB
    }

    _rotate_keep = GetEnvIntOrDefault("OJ_LOG_ROTATE_KEEP", 3);
    if (_rotate_keep < 1)
    {
        _rotate_keep = 1;
    }
}

bool LogServer::ParseJsonBody(const Request& req, Json::Value* out) const
{
    if (out == nullptr)
    {
        return false;
    }

    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream ss(req.body);
    return Json::parseFromStream(builder, ss, out, &errs);
}

ns_log::LogLevel LogServer::ParseLogLevel(const Json::Value& event) const
{
    if (!event.isObject() || !event.isMember("level") || !event["level"].isString())
    {
        return ns_log::INFO;
    }

    std::string level = event["level"].asString();
    if (level == "DEBUG" || level == "debug")
    {
        return ns_log::DEBUG;
    }
    if (level == "WARNING" || level == "warning" || level == "WARN" || level == "warn")
    {
        return ns_log::WARNING;
    }
    if (level == "ERROR" || level == "error")
    {
        return ns_log::ERROR;
    }
    if (level == "FATAL" || level == "fatal")
    {
        return ns_log::FATAL;
    }
    return ns_log::INFO;
}

bool LogServer::PersistEvent(const Json::Value& enriched)
{
    Json::FastWriter writer;
    std::string line = writer.write(enriched);

    std::lock_guard<std::mutex> lock(_store_mtx);
    RotateStoreIfNeededLocked();
    std::ofstream fout(_store_path.c_str(), std::ios::app);
    if (!fout.is_open())
    {
        logger(ERROR) << "[oj_log][persist] failed to open store path: " << _store_path;
        return false;
    }
    fout << line;
    return true;
}

std::string LogServer::BuildServiceLogLine(const Json::Value& enriched, ns_log::LogLevel level) const
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::string payload = Json::writeString(builder, enriched);

    const std::string service =
        (enriched.isObject() && enriched.isMember("service") && enriched["service"].isString())
            ? enriched["service"].asString()
            : "unknown";

    return spdlog::fmt_lib::format(
        "[recv_ts_ms={}][service={}][level={}] {}",
        NowMs(),
        service,
        ns_log::LogLevelToString(level),
        payload);
}

std::string LogServer::PickServiceLogPath(const Json::Value& enriched) const
{
    const std::string fallback = _service_log_root + "oj_server.log";
    if (!enriched.isObject() || !enriched.isMember("service") || !enriched["service"].isString())
    {
        return fallback;
    }

    const std::string service = enriched["service"].asString();
    if (service == "oj_admin")
    {
        return _service_log_root + "oj_admin.log";
    }
    return fallback;
}

std::shared_ptr<spdlog::logger> LogServer::GetOrCreateServiceLogger(const std::string& service_log_path)
{
    std::lock_guard<std::mutex> lock(_service_log_mtx);
    auto it = _service_loggers.find(service_log_path);
    if (it != _service_loggers.end())
    {
        return it->second;
    }

    const std::string logger_name = "oj_log_router_" + SanitizeLoggerName(service_log_path);
    auto existing = spdlog::get(logger_name);
    if (existing)
    {
        _service_loggers[service_log_path] = existing;
        return existing;
    }

    auto created = spdlog::basic_logger_mt(logger_name, service_log_path, true);
    created->set_pattern("%v");
    created->set_level(spdlog::level::debug);
    created->flush_on(spdlog::level::err);
    _service_loggers[service_log_path] = created;
    return created;
}

bool LogServer::PersistServiceLog(const Json::Value& enriched, ns_log::LogLevel level)
{
    const std::string service_log_path = PickServiceLogPath(enriched);
    const std::string line = BuildServiceLogLine(enriched, level);

    try
    {
        auto lg = GetOrCreateServiceLogger(service_log_path);
        if (!lg)
        {
            logger(ERROR) << "[oj_log][service_log] logger creation failed path=" << service_log_path;
            return false;
        }
        lg->log(ToSpdLevel(level), line);
        return true;
    }
    catch (const std::exception& ex)
    {
        logger(ERROR) << "[oj_log][service_log] write failed path=" << service_log_path << " what=" << ex.what();
        return false;
    }
    catch (...)
    {
        logger(ERROR) << "[oj_log][service_log] write failed path=" << service_log_path << " unknown exception";
        return false;
    }
}

std::uint64_t LogServer::GetFileSizeBytes(const std::string& file_path) const
{
    std::ifstream fin(file_path.c_str(), std::ios::binary | std::ios::ate);
    if (!fin.is_open())
    {
        return 0;
    }
    std::streampos end = fin.tellg();
    if (end < 0)
    {
        return 0;
    }
    return static_cast<std::uint64_t>(end);
}

void LogServer::RotateStoreIfNeededLocked()
{
    if (GetFileSizeBytes(_store_path) < _max_store_bytes)
    {
        return;
    }

    // Delete the oldest rotated file if exists.
    std::string oldest = _store_path + "." + std::to_string(_rotate_keep);
    std::remove(oldest.c_str());

    // Shift .(n-1) -> .n
    for (int i = _rotate_keep - 1; i >= 1; --i)
    {
        std::string src = _store_path + "." + std::to_string(i);
        std::string dst = _store_path + "." + std::to_string(i + 1);
        std::rename(src.c_str(), dst.c_str());
    }

    // Move current base file to .1
    std::string first = _store_path + ".1";
    std::rename(_store_path.c_str(), first.c_str());
}

std::string LogServer::BuildEventId()
{
    long long ms = NowMs();
    unsigned long long seq = _seq.fetch_add(1, std::memory_order_relaxed) + 1;
    return "elog_" + std::to_string(ms) + "_" + std::to_string(seq);
}

long long LogServer::NowMs() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

void LogServer::RegisterRoutes(Server& svr)
{
    svr.Get("/healthz", [](const Request& req, Response& rep) {
        (void)req;
        rep.set_content("ok", "text/plain;charset=utf-8");
    });

    svr.Get("/metrics", [this](const Request& req, Response& rep) {
        (void)req;
        std::string body;
        body += "oj_log_ingest_ok " + std::to_string(_ingest_ok.load(std::memory_order_relaxed)) + "\n";
        body += "oj_log_ingest_bad_json " + std::to_string(_ingest_bad_json.load(std::memory_order_relaxed)) + "\n";
        body += "oj_log_ingest_persist_fail " + std::to_string(_ingest_persist_fail.load(std::memory_order_relaxed)) + "\n";
        body += "oj_log_store_file_bytes " + std::to_string(GetFileSizeBytes(_store_path)) + "\n";
        rep.set_content(body, "text/plain;charset=utf-8");
    });

    // Minimal ingest route for V1 skeleton. It only validates payload,
    // enriches metadata, and prints structured event summary to logger.
    svr.Post("/api/logs/ingest", [this](const Request& req, Response& rep) {
        Json::Value in;
        if (!ParseJsonBody(req, &in) || !in.isObject())
        {
            _ingest_bad_json.fetch_add(1, std::memory_order_relaxed);
            rep.status = 400;
            rep.set_content("invalid_json", "text/plain;charset=utf-8");
            return;
        }

        Json::Value enriched = in;
        const ns_log::LogLevel inbound_level = ParseLogLevel(in);
        enriched["event_id"] = BuildEventId();
        enriched["recv_ts_ms"] = Json::Int64(NowMs());
        enriched["peer_ip"] = req.remote_addr;
        enriched["peer_port"] = req.remote_port;

        Json::FastWriter writer;
        logger(inbound_level) << "[oj_log][ingest] " << writer.write(enriched);
        if (!PersistServiceLog(enriched, inbound_level))
        {
            _ingest_persist_fail.fetch_add(1, std::memory_order_relaxed);
            rep.status = 500;
            rep.set_content("service_log_persist_failed", "text/plain;charset=utf-8");
            return;
        }
        if (!PersistEvent(enriched))
        {
            _ingest_persist_fail.fetch_add(1, std::memory_order_relaxed);
            rep.status = 500;
            rep.set_content("persist_failed", "text/plain;charset=utf-8");
            return;
        }
        _ingest_ok.fetch_add(1, std::memory_order_relaxed);
        rep.status = 202;
    });
}

bool LogServer::Start(const std::string& host, int port)
{
    Server svr;
    RegisterRoutes(svr);
    logger(INFO) << "oj_log start at " << host << ":" << port;
    return svr.listen(host.c_str(), port);
}
}

int main(int argc, char* argv[])
{
    std::string host = "0.0.0.0";
    int port = 8100;

    const char* env_port = std::getenv("OJ_LOG_PORT");
    if (env_port != nullptr)
    {
        try
        {
            port = std::stoi(env_port);
        }
        catch (...)
        {
            port = 8100;
        }
    }

    if (argc >= 2)
    {
        try
        {
            port = std::stoi(argv[1]);
        }
        catch (...)
        {
            logger(WARNING) << "invalid cli port, fallback to " << port;
        }
    }

    ns_oj_log::LogServer server;
    server.Start(host, port);
    return 0;
}
