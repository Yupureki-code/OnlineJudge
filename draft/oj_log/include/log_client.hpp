#pragma once

#include <Logger/logstrategy.h>

#include "log_contract.hpp"

#include <cstdint>
#include <httplib.h>
#include <sstream>
#include <string>
#include <vector>

namespace ns_oj_log {

class LogClient
{
public:
    struct DeliveryResult
    {
        bool acknowledged = false;
        bool health_checked = false;
        bool health_ok = false;
        bool permanent_failure = false;
        int status_code = 0;
        std::string error_code;
        std::string error_message;
        std::vector<std::string> failed_required_sinks;
    };

    LogClient(const std::string& host, uint16_t port, bool verify_health = true)
        : _host(host), _port(port)
    {
        if (verify_health)
        {
            httplib::Client cli(_host, _port);
            auto res = cli.Get("/healthz");
            if (!res || res->status != 200)
            {
                throw std::runtime_error("oj_log health check failed");
            }
        }
    }

    bool Healthy() const
    {
        httplib::Client cli(_host, _port);
        cli.set_connection_timeout(0, 200000);
        cli.set_read_timeout(0, 200000);
        auto res = cli.Get("/healthz");
        return res && res->status == 200;
    }

    DeliveryResult Deliver(LogStruct log_struct, bool probe_health_on_no_response)
    {
        DeliveryResult result;
        if (log_struct.event_id.empty())
        {
            log_struct.event_id = GenerateEventId();
        }
        if (log_struct.ts_ms <= 0)
        {
            log_struct.ts_ms = NowMs();
        }
        log_struct.level = NormalizeLevelString(log_struct.level);

        std::string err;
        if (!log_struct.Validate(&err))
        {
            logger(ns_log::ERROR) << "oj_log client validate failed: " << err;
            result.status_code = -1;
            return result;
        }

        httplib::Client cli(_host, _port);
        cli.set_connection_timeout(0, 300000);
        cli.set_read_timeout(0, 500000);
        auto res = cli.Post("/api/logs/ingest", log_struct.GetJsonString(), "application/json;charset=utf-8");
        if (!res)
        {
            logger(ns_log::ERROR) << "oj_log client post failed host=" << _host << " port=" << _port;
            result.status_code = -2;
            if (probe_health_on_no_response)
            {
                result.health_checked = true;
                result.health_ok = Healthy();
            }
            return result;
        }

        auto fill_response_details = [&result](const Json::Value& body) {
            if (!body.isObject())
            {
                return;
            }
            if (body.isMember("error_code") && body["error_code"].isString())
            {
                result.error_code = body["error_code"].asString();
            }
            if (body.isMember("error_message") && body["error_message"].isString())
            {
                result.error_message = body["error_message"].asString();
            }
            result.permanent_failure = result.error_code == "invalid_json"
                || result.error_code == "invalid_payload"
                || result.error_code == "invalid_protocol";
            result.failed_required_sinks.clear();
            if (body.isMember("failed_required_sinks") && body["failed_required_sinks"].isArray())
            {
                const Json::Value& failed = body["failed_required_sinks"];
                for (Json::ArrayIndex i = 0; i < failed.size(); ++i)
                {
                    if (failed[i].isString())
                    {
                        result.failed_required_sinks.emplace_back(failed[i].asString());
                    }
                }
            }
        };

        Json::Value response_body;
        bool has_response_body = false;
        if (!res->body.empty())
        {
            Json::CharReaderBuilder builder;
            std::string errs;
            std::istringstream ss(res->body);
            has_response_body = Json::parseFromStream(builder, ss, &response_body, &errs) && response_body.isObject();
            if (!has_response_body && (res->status == 200 || res->status == 202 || res->status == 204))
            {
                logger(ns_log::ERROR) << "oj_log client ack parse failed status=" << res->status;
                result.status_code = -3;
                if (probe_health_on_no_response)
                {
                    result.health_checked = true;
                    result.health_ok = Healthy();
                }
                return result;
            }
            if (has_response_body)
            {
                fill_response_details(response_body);
            }
        }

        result.status_code = res->status;
        if (res->status == 200 || res->status == 202 || res->status == 204)
        {
            if (!has_response_body)
            {
                logger(ns_log::ERROR) << "oj_log client ack parse failed status=" << res->status;
                result.status_code = -3;
                if (probe_health_on_no_response)
                {
                    result.health_checked = true;
                    result.health_ok = Healthy();
                }
                return result;
            }

            bool ack_success = response_body.isMember("success") && response_body["success"].asBool();
            std::string ack_event_id = response_body.isMember("event_id") && response_body["event_id"].isString()
                ? response_body["event_id"].asString()
                : "";
            if (ack_success && ack_event_id == log_struct.event_id)
            {
                result.acknowledged = true;
                result.health_ok = true;
                return result;
            }

            logger(ns_log::ERROR) << "oj_log client ack mismatch status=" << res->status
                                  << " event_id=" << log_struct.event_id
                                  << " ack_event_id=" << ack_event_id
                                  << " error_code=" << result.error_code;
            return result;
        }

        logger(ns_log::ERROR) << "oj_log client post rejected status=" << res->status
                              << " error_code=" << result.error_code;
        return result;
    }
private:
    std::string _host;
    uint16_t _port;
};
} // namespace ns_oj_log