#include <cerrno>
#include <climits>
#include <cstdlib>
#include <memory>
#include <string>

#include <brpc/server.h>
#include <gflags/gflags.h>


#include "../include/control/oj_control.hpp"
#include "model/model_base.hpp"
#include "judge/outbox_publisher.hpp"
#include "rpc/oj_service_impl.hpp"
#include "rpc/static_http_service.hpp"
#include "runtime/business_executor.hpp"
#include "../../comm/mq_client.hpp"

namespace
{

int BoundedEnv(const char* name, int fallback, int minimum, int maximum)
{
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;

    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < minimum || parsed > maximum)
        return fallback;
    return static_cast<int>(parsed);
}

} // namespace

int main()
{
    if (!oj::logger::InitLogger("oj_server", LOG_PATH + "oj_server.log", spdlog::level::info))
        return 1;
    const std::string max_body_size = std::to_string(
        BoundedEnv("OJ_SERVER_MAX_BODY_SIZE", 8 * 1024 * 1024, 1024, 64 * 1024 * 1024));
    GFLAGS_NAMESPACE::SetCommandLineOption("max_body_size", max_body_size.c_str());
    if (!oj::model::ModelBase::StartDatabase("oj_server")) {
        LOG_CRITICAL("Failed to initialize oj_server ODB runtime");
        oj::logger::ShutdownLogger();
        return 1;
    }

    auto control = std::make_shared<oj::control::Control>();
    control->GetModel()->StartMetricsFlushWorker();
    ns_mq::MQConfig mq_config;
    mq_config.host = ns_runtime_cfg::GetEnvOrDefault("OJ_MQ_HOST", "localhost");
    mq_config.port = static_cast<uint16_t>(BoundedEnv("OJ_MQ_PORT", 5672, 1, 65535));
    mq_config.user = ns_runtime_cfg::GetEnvOrDefault("OJ_MQ_USER", "oj");
    mq_config.password = ns_runtime_cfg::GetEnvOrDefault("OJ_MQ_PASS", "ojpassword");
    ns_mq::MQProducer mq_producer;
    try {
        mq_producer.Init(mq_config);
    } catch (const std::exception& error) {
        LOG_CRITICAL("Failed to initialize judge MQ producer: {}", error.what());
        control->GetModel()->StopMetricsFlushWorker();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }
    oj::runtime::BusinessExecutor executor({
        .worker_count = static_cast<std::size_t>(BoundedEnv("OJ_BUSINESS_THREADS", 4, 1, 256)),
        .queue_capacity = static_cast<std::size_t>(BoundedEnv("OJ_BUSINESS_QUEUE_CAPACITY", 256, 1, 65536)),
    });
    if (!executor.Start()) {
        LOG_CRITICAL("Failed to start oj_server business executor");
        mq_producer.Close();
        control->GetModel()->StopMetricsFlushWorker();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }

    oj::judge::ModelOutboxStore outbox_store(control->GetModel()->Submission());
    oj::judge::MqConfirmedPublisher confirmed_publisher(mq_producer);
    oj::judge::OutboxPublisher outbox_publisher(outbox_store, confirmed_publisher);
    if (!control->GetModel()->Submission().RecoverCustomTaskCaches())
        LOG_ERROR("Failed to recover custom task cache state");
    if (!outbox_publisher.Start()) {
        LOG_CRITICAL("Failed to start judge outbox publisher");
        executor.Stop();
        mq_producer.Close();
        control->GetModel()->StopMetricsFlushWorker();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }
    oj::rpc::OJServiceImpl service(control, executor, &outbox_publisher);
    oj::rpc::StaticHttpService static_service(control, oj::rpc::StaticHttpMode::Main, HTML_PATH);
    brpc::Server server;
    brpc::ServiceOptions service_options;
    service_options.ownership = brpc::SERVER_DOESNT_OWN_SERVICE;
    service_options.allow_http_body_to_pb = true;
    if (server.AddService(&service, service_options) != 0) {
        LOG_CRITICAL("Failed to add OJService");
        executor.Stop();
        outbox_publisher.Stop();
        mq_producer.Close();
        control->GetModel()->StopMetricsFlushWorker();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }

    brpc::ServiceOptions static_options;
    static_options.ownership = brpc::SERVER_DOESNT_OWN_SERVICE;
    static_options.restful_mappings = "/* => default_method";
    static_options.allow_default_url = false;
    static_options.allow_http_body_to_pb = false;
    if (server.AddService(&static_service, static_options) != 0) {
        LOG_CRITICAL("Failed to add main static HTTP service");
        executor.Stop();
        outbox_publisher.Stop();
        mq_producer.Close();
        control->GetModel()->StopMetricsFlushWorker();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }

    brpc::ServerOptions options;
    options.num_threads = BoundedEnv("OJ_SERVER_THREADS", 8, 1, 256);
    options.max_concurrency = BoundedEnv("OJ_SERVER_MAX_CONCURRENCY", 1024, 1, 65536);
    options.has_builtin_services = false;
    const int port = BoundedEnv("OJ_SERVER_PORT", 8080, 1, 65535);
    const bool started = server.Start(port, &options) == 0;
    if (started) {
        LOG_INFO("oj_server brpc server running on port {}", port);
        server.RunUntilAskedToQuit();
    } else {
        LOG_CRITICAL("Failed to start oj_server on port {}", port);
    }
    executor.Stop();
    outbox_publisher.Stop();
    mq_producer.Close();
    control->GetModel()->StopMetricsFlushWorker();
    oj::model::ModelBase::StopDatabase();
    oj::logger::ShutdownLogger();
    return started ? 0 : 1;
}
