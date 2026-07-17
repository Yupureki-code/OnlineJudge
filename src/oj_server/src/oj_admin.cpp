#include <cerrno>
#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include <brpc/server.h>
#include <gflags/gflags.h>

#include "comm.hpp"
#include "control/oj_control.hpp"
#include "model/model_base.hpp"
#include "oj_admin.hpp"
#include "rpc/admin_audit_worker.hpp"
#include "rpc/oj_admin_service_impl.hpp"
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
    if (!oj::logger::InitLogger("oj_admin", LOG_PATH + "oj_admin.log", spdlog::level::info))
        return 1;
    const std::string max_body_size = std::to_string(
        BoundedEnv("OJ_ADMIN_MAX_BODY_SIZE", 8 * 1024 * 1024, 1024, 64 * 1024 * 1024));
    GFLAGS_NAMESPACE::SetCommandLineOption("max_body_size", max_body_size.c_str());
    if (!oj::model::ModelBase::StartDatabase("oj_admin")) {
        LOG_CRITICAL("Failed to initialize oj_admin ODB runtime");
        oj::logger::ShutdownLogger();
        return 1;
    }

    auto control = std::make_shared<oj::control::Control>();
    auto sessions = std::make_shared<oj::admin::AdminSessionStore>();
    oj::runtime::BusinessExecutor executor({
        .worker_count = static_cast<std::size_t>(BoundedEnv("OJ_ADMIN_BUSINESS_THREADS", 4, 1, 256)),
        .queue_capacity = static_cast<std::size_t>(BoundedEnv("OJ_ADMIN_BUSINESS_QUEUE_CAPACITY", 256, 1, 65536)),
    });
    if (!executor.Start()) {
        LOG_CRITICAL("Failed to start oj_admin business executor");
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }

    oj::rpc::AdminAuditWorker audit_worker(*control->GetModel());
    if (!audit_worker.Start()) {
        LOG_CRITICAL("Failed to start admin audit worker");
        executor.Stop();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }
    oj::rpc::OJAdminServiceImpl service(*control, *sessions, executor, &audit_worker);
    oj::rpc::StaticHttpConfig static_config{
        .mode = oj::rpc::StaticHttpMode::Admin,
        .root = HTML_PATH,
        .admin_sessions = sessions,
    };
    oj::rpc::StaticHttpService static_service(control, std::move(static_config));
    brpc::Server server;
    brpc::ServiceOptions service_options;
    service_options.ownership = brpc::SERVER_DOESNT_OWN_SERVICE;
    service_options.allow_http_body_to_pb = true;
    if (server.AddService(&service, service_options) != 0) {
        LOG_CRITICAL("Failed to add OJAdminService");
        executor.Stop();
        audit_worker.Stop();
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
        LOG_CRITICAL("Failed to add admin static HTTP service");
        executor.Stop();
        audit_worker.Stop();
        oj::model::ModelBase::StopDatabase();
        oj::logger::ShutdownLogger();
        return 1;
    }

    brpc::ServerOptions options;
    options.num_threads = BoundedEnv("OJ_ADMIN_SERVER_THREADS", 4, 1, 256);
    options.max_concurrency = BoundedEnv("OJ_ADMIN_SERVER_MAX_CONCURRENCY", 512, 1, 65536);
    options.has_builtin_services = false;
    const int default_port = admin_port >= 1 && admin_port <= 65535 ? admin_port : 8090;
    const int port = BoundedEnv("OJ_ADMIN_PORT", default_port, 1, 65535);
    const std::string endpoint = admin_host + ":" + std::to_string(port);
    const bool started = server.Start(endpoint.c_str(), &options) == 0;
    if (started) {
        LOG_INFO("oj_admin brpc server running at {}", endpoint);
        server.RunUntilAskedToQuit();
    } else {
        LOG_CRITICAL("Failed to start oj_admin at {}", endpoint);
    }

    executor.Stop();
    audit_worker.Stop();
    oj::model::ModelBase::StopDatabase();
    oj::logger::ShutdownLogger();
    return started ? 0 : 1;
}
