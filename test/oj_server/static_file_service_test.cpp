#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <unistd.h>

#include <brpc/controller.h>

#include "control/oj_control.hpp"
#include "oj_admin.hpp"
#include "rpc/static_http_service.hpp"

namespace
{

namespace fs = std::filesystem;

void Check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Result
{
    int status;
    std::string body;
    std::string type;
    std::string length;
    std::string location;
};

Result Request(oj::rpc::StaticHttpService& service,
               const std::string& uri,
               brpc::HttpMethod method = brpc::HTTP_METHOD_GET,
               const std::string& cookie = {})
{
    brpc::Controller controller;
    controller.http_request().set_method(method);
    controller.http_request().uri() = uri;
    if (!cookie.empty()) controller.http_request().SetHeader("Cookie", cookie);
    oj::common::EmptyRequest request;
    oj::common::EmptyResponse response;
    service.default_method(&controller, &request, &response, nullptr);
    Result result{controller.http_response().status_code(), {}, controller.http_response().content_type(), {}, {}};
    controller.response_attachment().append_to(&result.body);
    if (const auto* value = controller.http_response().GetHeader("Content-Length")) result.length = *value;
    if (const auto* value = controller.http_response().GetHeader("Location")) result.location = *value;
    return result;
}

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / ("oj-static-test-" + std::to_string(::getpid()));
    const fs::path outside = root.parent_path() / (root.filename().string() + "-outside.txt");
    fs::create_directories(root / "admin");
    fs::create_directories(root / "css");
    {
        std::ofstream css(root / "css" / "site.css", std::ios::binary);
        const char content[] = "a{color:red}\0x";
        css.write(content, sizeof(content) - 1);
    }
    std::ofstream(root / "admin" / "login.html") << "<html>login</html>";
    std::ofstream(root / "admin" / "index.html") << "<html>admin</html>";
    std::ofstream(root / "admin" / "admin-app.js") << "window.app=true;";
    std::ofstream(outside) << "secret";
    fs::create_symlink(outside, root / "escape.txt");

    auto control = std::make_shared<oj::control::Control>();
    oj::rpc::StaticHttpService main_service(control, oj::rpc::StaticHttpMode::Main, root.string());

    Result result = Request(main_service, "/css/site.css");
    Check(result.status == 200, "asset status");
    Check(result.type == "text/css;charset=utf-8", "CSS MIME");
    Check(result.body.size() == 14 && result.length == "14", "binary body and length");

    result = Request(main_service, "/css/site.css", brpc::HTTP_METHOD_HEAD);
    Check(result.status == 200 && result.body.empty() && result.length == "14", "HEAD metadata without body");
    Check(Request(main_service, "/%2e%2e/outside.txt").status == 404, "encoded traversal rejected");
    Check(Request(main_service, "/escape.txt").status == 404, "symlink escape rejected");
    Check(Request(main_service, "/bad%00name").status == 404, "NUL rejected");
    Check(Request(main_service, "/bad%5cname").status == 404, "backslash rejected");
    Check(Request(main_service, "/oj.biz.UserService/GetUser").status == 404, "native RPC path not an asset");
    Check(Request(main_service, "/css/site.css", brpc::HTTP_METHOD_POST).status == 405, "non-read method rejected");
    Check(Request(main_service, "/healthz").body == "ok", "main health check");

    auto sessions = std::make_shared<oj::admin::AdminSessionStore>();
    oj::rpc::StaticHttpConfig config{oj::rpc::StaticHttpMode::Admin, root.string(), sessions};
    oj::rpc::StaticHttpService admin_service(control, config);
    Check(Request(admin_service, "/healthz").body == "ok", "admin health check");
    result = Request(admin_service, "/");
    Check(result.status == 302 && result.location == "/admin/login", "admin root redirect");
    Check(Request(admin_service, "/admin/login").status == 200, "admin login is public");
    Check(Request(admin_service, "/admin").status == 404, "admin shell requires session");
    Check(Request(admin_service, "/admin/index.html").status == 404,
          "admin shell physical path requires session");
    Check(Request(admin_service, "/admin/assets/admin-app.js").body == "window.app=true;", "admin asset mapping");

    AdminAccount admin;
    admin.admin_id = 3;
    admin.uid = 7;
    admin.role = "admin";
    User user{};
    user.uid = 7;
    user.name = "</script><script>bad()</script>";
    user.email = "admin@example.test";
    const std::string session_id = sessions->CreateSession(admin, user);
    result = Request(admin_service, "/admin", brpc::HTTP_METHOD_GET,
                     std::string(oj::admin::kAdminSessionCookieName) + "=" + session_id);
    Check(result.status == 200, "authenticated admin shell");
    Check(result.body.find("SERVER_USER_INFO") != std::string::npos, "admin user injected");
    Check(result.body.find("</script><script>bad()") == std::string::npos, "injected values are script-safe");

    fs::remove_all(root);
    fs::remove(outside);
    std::cout << "static_file_service_test: PASS\n";
    return 0;
}
