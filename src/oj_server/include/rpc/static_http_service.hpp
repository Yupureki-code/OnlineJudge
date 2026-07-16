#pragma once

#include <memory>
#include <string>

#include "static_http.pb.h"

namespace oj::admin
{
class AdminSessionStore;
}

namespace oj::control
{
class Control;
}

namespace oj::rpc
{

enum class StaticHttpMode
{
    Main,
    Admin,
};

struct StaticHttpConfig
{
    StaticHttpMode mode = StaticHttpMode::Main;
    std::string root;
    std::shared_ptr<oj::admin::AdminSessionStore> admin_sessions;
};

class StaticHttpService final : public oj::transport::StaticHttpService
{
public:
    StaticHttpService(std::shared_ptr<oj::control::Control> control,
                      StaticHttpMode mode,
                      std::string root = {});
    StaticHttpService(std::shared_ptr<oj::control::Control> control, StaticHttpConfig config);
    ~StaticHttpService() override;

    void default_method(::google::protobuf::RpcController* controller,
                        const ::oj::common::EmptyRequest* request,
                        ::oj::common::EmptyResponse* response,
                        ::google::protobuf::Closure* done) override;

private:
    std::shared_ptr<oj::control::Control> _control;
    StaticHttpConfig config_;
};

} // namespace oj::rpc
