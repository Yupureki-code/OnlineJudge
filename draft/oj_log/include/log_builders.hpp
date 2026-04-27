// #pragma once

// #include "log_contract.hpp"

// #include <jsoncpp/json/json.h>

// #include <functional>
// #include <jsoncpp/json/value.h>
// #include <string>
// #include <thread>
// #include <unistd.h>
// #define BuildQueryJsonString(context,cache_hit) ns_oj_log::Builders::BuildQueryJsonString(context.page,context.size,context._query->GetJsonString(),cache_hit,__LINE__,__FILE__)
// #define BuildDBHitJsonString(redis_key,mysql_key,cache_hit,mysql_hit,cache_ms,mysql_ms) ns_oj_log::Builders::BuildDBHitJsonString(redis_key,mysql_key,cache_hit,mysql_hit,cache_ms,mysql_ms,__LINE__,__FILE__)
// #define BuildModuleJsonString(module_name, func_name, message) \
//             ns_oj_log::Builders::BuildModuleJsonString(module_name, func_name, message, __LINE__, __FILE__)

// class RemoteLogManager
// {
// private:
//     void WorkerLoop()
//     {
//         auto requeue_front = [this](LogStruct&& log) {
//             _oj_log_healthy.store(false, std::memory_order_relaxed);
//             {
//                 std::lock_guard<std::mutex> lock(_queue_mtx);
//                 _queue.emplace_front(std::move(log));
//             }
//             _queue_cv.notify_one();
//         };

//         while (_running.load(std::memory_order_relaxed))
//         {
//             LogStruct item;
//             {
//                 std::unique_lock<std::mutex> lock(_queue_mtx);
//                 _queue_cv.wait(lock, [this]() {
//                     return !_running.load(std::memory_order_relaxed)
//                         || (!_queue.empty() && _oj_log_healthy.load(std::memory_order_relaxed));
//                 });

//                 if (!_running.load(std::memory_order_relaxed))
//                 {
//                     break;
//                 }

//                 item = std::move(_queue.front());
//                 _queue.pop_front();
//             }

//             LogClient::DeliveryResult result;
//             try
//             {
//                 result = _client.Deliver(item, true);
//             }
//             catch (...)
//             {
//                 requeue_front(std::move(item));
//                 continue;
//             }

//             if (result.acknowledged)
//             {
//                 continue;
//             }

//             if (!result.error_code.empty())
//             {
//                 logger(ns_log::ERROR) << "remote log send rejected status=" << result.status_code
//                                       << " error_code=" << result.error_code
//                                       << " message=" << result.error_message;
//             }

//             if (result.permanent_failure)
//             {
//                 logger(ns_log::ERROR) << "remote log permanent rejection, drop event_id=" << item.event_id;
//                 continue;
//             }

//             requeue_front(std::move(item));
//         }
//     }

//     void HealthLoop()
//     {
//         while (_running.load(std::memory_order_relaxed))
//         {
//             bool ok = _client.Healthy();
//             bool previous = _oj_log_healthy.exchange(ok, std::memory_order_relaxed);
//             if (previous != ok)
//             {
//                 _queue_cv.notify_all();
//             }

//             for (int i = 0; i < 50; ++i)
//             {
//                 if (!_running.load(std::memory_order_relaxed))
//                 {
//                     return;
//                 }
//                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
//             }
//         }
//     }

// public:
//     RemoteLogManager()
//         : _client(ns_runtime_cfg::GetEnvOrDefault("OJ_LOG_HOST", "127.0.0.1"),
//                   static_cast<uint16_t>(ns_runtime_cfg::GetEnvIntOrDefault("OJ_LOG_PORT", 8100)),
//                   false)
//     {
//         _worker = std::thread([this]() { WorkerLoop(); });
//         _health_worker = std::thread([this]() { HealthLoop(); });
//     }

//     ~RemoteLogManager()
//     {
//         _running.store(false, std::memory_order_relaxed);
//         _queue_cv.notify_all();
//         if (_worker.joinable())
//         {
//             _worker.join();
//         }
//         if (_health_worker.joinable())
//         {
//             _health_worker.join();
//         }
//     }

//     RemoteLogManager(const RemoteLogManager&) = delete;
//     RemoteLogManager& operator=(const RemoteLogManager&) = delete;

//     static RemoteLogManager& Instance()
//     {
//         static RemoteLogManager inst;
//         return inst;
//     }

//     bool Submit(LogStruct log)
//     {
//         if (log.event_id.empty())
//         {
//             log.event_id = GenerateEventId();
//         }
//         if (log.ts_ms <= 0)
//         {
//             log.ts_ms = NowMs();
//         }
//         log.level = NormalizeLevelString(log.level);
//         if (log.pid <= 0)
//         {
//             log.pid = static_cast<int>(::getpid());
//         }
//         if (log.tid <= 0)
//         {
//             log.tid = static_cast<int>(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0x7fffffff);
//         }

//         std::string err;
//         if (!log.Validate(&err))
//         {
//             logger(ns_log::ERROR) << "remote log validate failed: " << err;
//             return false;
//         }

//         {
//             std::lock_guard<std::mutex> lock(_queue_mtx);
//             _queue.emplace_back(std::move(log));
//         }
//         _queue_cv.notify_one();
//         return true;
//     }

// private:
//     LogClient _client;
//     std::atomic<bool> _running{true};
//     std::atomic<bool> _oj_log_healthy{true};
//     std::deque<LogStruct> _queue;
//     std::mutex _queue_mtx;
//     std::condition_variable _queue_cv;
//     std::thread _worker;
//     std::thread _health_worker;
// };



// #define RemoteLogger(log) RemoteLogManager::Instance().Submit(log)


// namespace ns_oj_log {
// namespace Builders {

// inline Json::Value ParseJsonPayload(const std::string& payload_text)
// {
//     if (payload_text.empty())
//     {
//         return Json::Value(Json::objectValue);
//     }

//     Json::CharReaderBuilder builder;
//     Json::Value value;
//     std::string errs;
//     std::istringstream ss(payload_text);
//     if (Json::parseFromStream(builder, ss, &value, &errs))
//     {
//         return value;
//     }

//     Json::Value fallback(Json::objectValue);
//     fallback["text"] = payload_text;
//     return fallback;
// }

// inline LogStruct MakeBase(const std::string& service,
//                           ns_log::LogLevel level,
//                           LogCategory category,
//                           const std::string& action)
// {
//     LogStruct log;
//     log.event_id = GenerateEventId();
//     log.ts_ms = NowMs();
//     log.service = service;
//     log.level = LevelToString(level);
//     log.category = category;
//     log.pid = static_cast<int>(::getpid());
//     log.tid = static_cast<int>(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0x7fffffff);
//     log.action = action;
//     return log;
// }

// inline LogStruct Audit(const std::string& service,
//                        const std::string& request_id,
//                        int admin_id,
//                        int uid,
//                        const std::string& action,
//                        const std::string& resource_type,
//                        const Json::Value& payload,
//                        ns_log::LogLevel level = ns_log::INFO)
// {
//     LogStruct log = MakeBase(service, level, LogCategory::audit, action);
//     log.request_id = request_id;
//     log.admin_id = admin_id;
//     log.uid = uid;
//     log.resource_type = resource_type;
//     log.payload = payload;
//     return log;
// }

// inline LogStruct Audit(const std::string& service,
//                        const std::string& request_id,
//                        int admin_id,
//                        int uid,
//                        const std::string& action,
//                        const std::string& resource_type,
//                        const std::string& payload_text,
//                        ns_log::LogLevel level = ns_log::INFO)
// {
//     return Audit(service, request_id, admin_id, uid, action, resource_type, ParseJsonPayload(payload_text), level);
// }

// inline LogStruct Access(const std::string& service,
//                         const std::string& request_id,
//                         const std::string& route,
//                         const std::string& method,
//                         int status,
//                         int latency_ms,
//                         const std::string& message = "",
//                         const Json::Value& payload = Json::Value(Json::objectValue),
//                         ns_log::LogLevel level = ns_log::INFO)
// {
//     LogStruct log = MakeBase(service, level, LogCategory::access, "http.access");
//     log.request_id = request_id;
//     log.route = route;
//     log.method = method;
//     log.status = status;
//     log.latency_ms = latency_ms;
//     log.message = message;
//     log.payload = payload;
//     return log;
// }

// inline LogStruct Query(const std::string& service,
//                        const std::string& request_id,
//                        const std::string& action,
//                        const std::string& resource_type,
//                        const Json::Value& payload,
//                        int uid = 0,
//                        ns_log::LogLevel level = ns_log::INFO)
// {
//     LogStruct log = MakeBase(service, level, LogCategory::query, action);
//     log.request_id = request_id;
//     log.resource_type = resource_type;
//     log.uid = uid;
//     log.payload = payload;
//     return log;
// }

// inline LogStruct Cache(const std::string& service,
//                        const std::string& request_id,
//                        const std::string& action,
//                        const Json::Value& payload,
//                        ns_log::LogLevel level = ns_log::INFO)
// {
//     LogStruct log = MakeBase(service, level, LogCategory::cache, action);
//     log.request_id = request_id;
//     log.payload = payload;
//     return log;
// }

// inline LogStruct Module(const std::string& service,
//                         const std::string& action,
//                         ns_log::LogLevel level,
//                         const std::string& message,
//                         const Json::Value& payload = Json::Value(Json::objectValue))
// {
//     LogStruct log = MakeBase(service, level, LogCategory::module, action);
//     log.message = message;
//     log.payload = payload;
//     return log;
// }

// inline LogStruct Security(const std::string& service,
//                           const std::string& request_id,
//                           const std::string& action,
//                           const Json::Value& payload,
//                           int uid = 0,
//                           ns_log::LogLevel level = ns_log::WARNING)
// {
//     LogStruct log = MakeBase(service, level, LogCategory::security, action);
//     log.request_id = request_id;
//     log.uid = uid;
//     log.payload = payload;
//     return log;
// }

// inline Json::Value BuildQueryJsonString(int page,int size,const std::string& query,const int line,const std::string& file_name)
// {
//     Json::Value value;
//     value["page"] = page;
//     value["size"] = size;
//     value["filters"] = query;
//     return value;
// }
// inline Json::Value BuildDBHitJsonString(const std::string& redis_key,const std::string & mysql_key,bool cache_hit,bool mysql_hit,int cache_ms,int mysql_ms,const int line,const std::string& file_name)
// {
//     Json::Value value;
//     value["redis_key"] = redis_key;
//     value["mysql_key"] = mysql_key;
//     value["cache_hit"] = cache_hit;
//     value["mysql_hit"] = mysql_hit;
//     value["cache_ms"] = cache_ms;
//     value["mysql_ms"] = mysql_ms;
//     return value;
// }
// inline Json::Value BuildModuleJsonString(const std::string& module_name,const std::string& func_name,const std::string& message,const int line,const std::string& file_name)
// {
//     Json::Value value;
//     value["module_name"] = module_name;
//     value["func_name"] = func_name;
//     value["message"] = message;
//     value["line"] = line;
//     value["file_name"] = file_name;
//     return value;
// }
// } // namespace Builders
// } // namespace ns_oj_log