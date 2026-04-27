// #pragma once

// #include <Logger/logstrategy.h>
// #include "log_contract.hpp"
// #include <cstdlib>
// #include <cstdio>
// #include <fstream>
// #include <httplib.h>
// #include <jsoncpp/json/json.h>
// #include <jsoncpp/json/value.h>
// #include "../../comm/config.h"
// #include <mutex>
// #include <sstream>
// #include <string>

// namespace ns_oj_log {

// namespace detail {

// inline int GetEnvIntOrDefault(const char* name, int default_value)
// {
//   const char* value = std::getenv(name);
//   if (value == nullptr || *value == '\0')
//   {
//     return default_value;
//   }
//   char* end = nullptr;
//   long parsed = std::strtol(value, &end, 10);
//   if (end == value)
//   {
//     return default_value;
//   }
//   if (parsed > 2147483647L)
//   {
//     return 2147483647;
//   }
//   if (parsed < -2147483647L)
//   {
//     return -2147483647;
//   }
//   return static_cast<int>(parsed);
// }

// } // namespace detail

// class LogServer {
// public:
//   using LogCategory = ns_oj_log::LogCategory;
//   using LogStruct = ns_oj_log::LogStruct;

//   LogServer()
//     : _store_path("./log/oj_log_events.jsonl")
//   {
//     const char* env_store = std::getenv("OJ_LOG_STORE_PATH");
//     if (env_store != nullptr)
//     {
//       std::string p(env_store);
//       if (!p.empty())
//       {
//         _store_path = p;
//       }
//     }
//   }

//   void RegisterRoutes(httplib::Server& svr)
//   {
//     svr.Get("/healthz", [](const httplib::Request& req, httplib::Response& rep) {
//       (void)req;
//       rep.set_content("ok", "text/plain;charset=utf-8");
//     });

//     svr.Post("/api/logs/ingest", [this](const httplib::Request& req, httplib::Response& rep) {
//       Json::Value in;
//       if (!ParseJsonBody(req, &in) || !in.isObject())
//       {
//         Json::Value response(Json::objectValue);
//         response["success"] = false;
//         response["error_code"] = "invalid_json";
//         Json::FastWriter writer;
//         rep.status = 400;
//         rep.set_content(writer.write(response), "application/json;charset=utf-8");
//         return;
//       }

//       LogStruct log;
//       if (!log.ParseJsonValue(in))
//       {
//         Json::Value response(Json::objectValue);
//         response["success"] = false;
//         response["error_code"] = "invalid_payload";
//         response["error_message"] = "request body must decode into LogStruct";
//         Json::FastWriter writer;
//         rep.status = 400;
//         rep.set_content(writer.write(response), "application/json;charset=utf-8");
//         return;
//       }

//       if (log.event_id.empty())
//       {
//         log.event_id = GenerateEventId();
//       }
//       log.level = NormalizeLevelString(log.level);

//       Json::Value enriched = log.ToJsonValue();
//       const ns_log::LogLevel inbound_level = ParseLogLevel(log.level);
//       enriched["recv_ts_ms"] = Json::Int64(ns_oj_log::NowMs());
//       enriched["peer_ip"] = req.remote_addr;
//       enriched["peer_port"] = req.remote_port;

//       Json::FastWriter writer;
//       logger(inbound_level) << "[oj_log][ingest] " << writer.write(enriched);
//       const char* failed_sink = PersistToFiles(enriched, inbound_level);
//       if (failed_sink != nullptr)
//       {
//         Json::Value response(Json::objectValue);
//         response["success"] = false;
//         response["event_id"] = log.event_id;
//         response["error_code"] = "sink_persist_failed";
//         Json::Value failed_sinks(Json::arrayValue);
//         failed_sinks.append(failed_sink);
//         response["failed_required_sinks"] = failed_sinks;
//         Json::FastWriter writer;
//         rep.status = 500;
//         rep.set_content(writer.write(response), "application/json;charset=utf-8");
//         return;
//       }
//       Json::Value response(Json::objectValue);
//       response["success"] = true;
//       response["event_id"] = log.event_id;
//       Json::FastWriter response_writer;
//       rep.status = 202;
//       rep.set_content(response_writer.write(response), "application/json;charset=utf-8");
//     });
//   }

//   bool Start(const std::string& host, int port)
//   {
//     httplib::Server svr;
//     RegisterRoutes(svr);
//     logger(ns_log::INFO) << "oj_log start at " << host << ":" << port;
//     return svr.listen(host.c_str(), port);
//   }

// private:
//   bool ParseJsonBody(const httplib::Request& req, Json::Value* out) const
//   {
//     if (out == nullptr)
//     {
//       return false;
//     }

//     Json::CharReaderBuilder builder;
//     std::string errs;
//     std::istringstream ss(req.body);
//     return Json::parseFromStream(builder, ss, out, &errs);
//   }

//   ns_log::LogLevel ParseLogLevel(const std::string& level) const
//   {
//     std::string normalized = NormalizeLevelString(level);
//     if (normalized == "DEBUG")
//     {
//       return ns_log::DEBUG;
//     }
//     if (normalized == "WARNING" || normalized == "WARN")
//     {
//       return ns_log::WARNING;
//     }
//     if (normalized == "ERROR")
//     {
//       return ns_log::ERROR;
//     }
//     if (normalized == "FATAL")
//     {
//       return ns_log::FATAL;
//     }
//     return ns_log::INFO;
//   }

//   const char* PersistToFiles(const Json::Value& enriched, ns_log::LogLevel level)
//   {
//     if (!PersistServiceLog(enriched, level))
//     {
//       logger(ns_log::ERROR) << "[oj_log][sink] write failed sink=service_file";
//       return "service_file";
//     }

//     if (!PersistEvent(enriched))
//     {
//       logger(ns_log::ERROR) << "[oj_log][sink] write failed sink=archive_jsonl";
//       return "archive_jsonl";
//     }

//     return nullptr;
//   }

//   bool PersistEvent(const Json::Value& enriched)
//   {
//     Json::FastWriter writer;
//     std::string line = writer.write(enriched);

//     std::lock_guard<std::mutex> lock(_file_mtx);
//     std::ofstream fout(_store_path.c_str(), std::ios::app);
//     if (!fout.is_open())
//     {
//       logger(ns_log::ERROR) << "[oj_log][persist] failed to open store path: " << _store_path;
//       return false;
//     }
//     fout << line;
//     return true;
//   }

//   bool PersistServiceLog(const Json::Value& enriched, ns_log::LogLevel level)
//   {
//     const std::string service_log_path = PickServiceLogPath(enriched);
//     const std::string line = BuildServiceLogLine(enriched, level);

//     std::lock_guard<std::mutex> lock(_file_mtx);
//     std::ofstream fout(service_log_path.c_str(), std::ios::app);
//     if (!fout.is_open())
//     {
//       logger(ns_log::ERROR) << "[oj_log][service_log] write failed path=" << service_log_path << " open failed";
//       return false;
//     }
//     fout << line << '\n';
//     return true;
//   }

//   std::string BuildServiceLogLine(const Json::Value& enriched, ns_log::LogLevel level) const
//   {
//     Json::StreamWriterBuilder builder;
//     builder["indentation"] = "";
//     const std::string payload = Json::writeString(builder, enriched);

//     const std::string service =
//       (enriched.isObject() && enriched.isMember("service") && enriched["service"].isString())
//         ? enriched["service"].asString()
//         : "unknown";

//     std::ostringstream oss;
//     oss << "[recv_ts_ms=" << ns_oj_log::NowMs() << "][service=" << service << "][level=" << ns_log::LogLevelToString(level) << "] "
//       << payload;
//     return oss.str();
//   }

//   std::string PickServiceLogPath(const Json::Value& enriched) const
//   {
//     const std::string fallback = std::string(LOG_PATH) + "oj_server.log";
//     if (!enriched.isObject() || !enriched.isMember("service") || !enriched["service"].isString())
//     {
//       return fallback;
//     }

//     const std::string service = enriched["service"].asString();
//     if (service == "oj_admin")
//     {
//       return std::string(LOG_PATH) + "oj_admin.log";
//     }
//     return fallback;
//   }
//   std::string _store_path;
//   std::mutex _file_mtx;
// };

// } // namespace ns_oj_log
