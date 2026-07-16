#include <cerrno>
#include <climits>
#include <cstdlib>
#include <memory>
#include <string>

#include <brpc/server.h>
#include <gflags/gflags.h>


#include "../include/control/oj_control.hpp"
#include "model/model_base.hpp"
#include "rpc/oj_service_impl.hpp"
#include "rpc/static_http_service.hpp"
#include "runtime/business_executor.hpp"

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
        oj::logger::LOG_CRITICAL("Failed to initialize oj_server ODB runtime");
        oj::logger::ShutdownLogger();
        return 1;
    }

    auto control = std::make_shared<oj::control::Control>();
    control->GetModel()->StartMetricsFlushWorker();
    oj::runtime::BusinessExecutor executor({
        .worker_count = static_cast<std::size_t>(BoundedEnv("OJ_BUSINESS_THREADS", 4, 1, 256)),
        .queue_capacity = static_cast<std::size_t>(BoundedEnv("OJ_BUSINESS_QUEUE_CAPACITY", 256, 1, 65536)),
    });
    if (!executor.Start()) {
        oj::logger::LOG_CRITICAL("Failed to start oj_server business executor");
        control->GetModel()->StopMetricsFlushWorker();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }

    oj::rpc::OJServiceImpl service(control, executor);
    oj::rpc::StaticHttpService static_service(control, oj::rpc::StaticHttpMode::Main, HTML_PATH);
    brpc::Server server;
    brpc::ServiceOptions service_options;
    service_options.ownership = brpc::SERVER_DOESNT_OWN_SERVICE;
    service_options.allow_http_body_to_pb = false;
    if (server.AddService(&service, service_options) != 0) {
        oj::logger::LOG_CRITICAL("Failed to add OJService");
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
        oj::logger::LOG_CRITICAL("Failed to add main static HTTP service");
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
        oj::logger::LOG_INFO("oj_server brpc server running on port {}", port);
        server.RunUntilAskedToQuit();
    } else {
        oj::logger::LOG_CRITICAL("Failed to start oj_server on port {}", port);
    }
    control->GetModel()->StopMetricsFlushWorker();
    oj::model::ModelBase::StopDatabase();
    oj::logger::ShutdownLogger();
    return started ? 0 : 1;
}
