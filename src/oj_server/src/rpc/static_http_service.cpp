#include "rpc/static_http_service.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>

#include <brpc/closure_guard.h>
#include <brpc/controller.h>
#include <brpc/http_status_code.h>
#include <brpc/progressive_attachment.h>
#include <jsoncpp/json/json.h>

#include "control/oj_control.hpp"
#include "oj_admin.hpp"
#include "rpc/session_adapter.hpp"
#include "config.h"
#include "../../../comm/models/user.hxx"

namespace oj::rpc
{
    using namespace oj::db;
    namespace
    {

        namespace fs = std::filesystem;

        constexpr std::string_view kNoStore = "no-store, no-cache, must-revalidate, max-age=0";
        constexpr std::string_view kAssetCache = "public, max-age=3600";

        std::optional<std::string> DecodePath(std::string_view encoded)
        {
            auto hex = [](char value) -> int {
                if (value >= '0' && value <= '9') return value - '0';
                if (value >= 'a' && value <= 'f') return value - 'a' + 10;
                if (value >= 'A' && value <= 'F') return value - 'A' + 10;
                return -1;
            };

            std::string decoded;
            decoded.reserve(encoded.size());
            for (std::size_t i = 0; i < encoded.size(); ++i) {
                if (encoded[i] != '%') {
                    decoded.push_back(encoded[i]);
                    continue;
                }
                if (i + 2 >= encoded.size()) return std::nullopt;
                const int high = hex(encoded[i + 1]);
                const int low = hex(encoded[i + 2]);
                if (high < 0 || low < 0) return std::nullopt;
                decoded.push_back(static_cast<char>((high << 4) | low));
                i += 2;
            }
            if (decoded.find('\0') != std::string::npos ||
                decoded.find('\\') != std::string::npos ||
                decoded.find("..") != std::string::npos) {
                return std::nullopt;
            }
            return decoded;
        }

        bool IsWithin(const fs::path& root, const fs::path& candidate)
        {
            auto root_it = root.begin();
            auto candidate_it = candidate.begin();
            for (; root_it != root.end(); ++root_it, ++candidate_it) {
                if (candidate_it == candidate.end() || *root_it != *candidate_it) return false;
            }
            return true;
        }

        std::optional<fs::path> ResolveFile(const std::string& root_text, std::string_view request_path)
        {
            std::error_code error;
            const fs::path root = fs::weakly_canonical(root_text, error);
            if (error || root.empty()) return std::nullopt;

            std::string relative(request_path);
            while (!relative.empty() && relative.front() == '/') relative.erase(relative.begin());
            const fs::path candidate = fs::weakly_canonical(root / relative, error);
            if (error || !IsWithin(root, candidate) || !fs::is_regular_file(candidate, error) || error) {
                return std::nullopt;
            }
            return candidate;
        }

        std::string MimeType(const fs::path& path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            static const std::unordered_map<std::string, std::string> types = {
                {".html", "text/html;charset=utf-8"},
                {".css", "text/css;charset=utf-8"},
                {".js", "application/javascript;charset=utf-8"},
                {".json", "application/json;charset=utf-8"},
                {".txt", "text/plain;charset=utf-8"},
                {".svg", "image/svg+xml"}, {".png", "image/png"}, {".jpg", "image/jpeg"},
                {".jpeg", "image/jpeg"}, {".gif", "image/gif"}, {".webp", "image/webp"},
                {".ico", "image/x-icon"}, {".mp4", "video/mp4"}, {".webm", "video/webm"},
                {".woff", "font/woff"}, {".woff2", "font/woff2"}, {".ttf", "font/ttf"},
                {".map", "application/json"}, {".pdf", "application/pdf"},
            };
            const auto found = types.find(extension);
            return found == types.end() ? "application/octet-stream" : found->second;
        }

        std::optional<std::string> ReadBinary(const fs::path& path)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input) return std::nullopt;
            std::ostringstream contents;
            contents << input.rdbuf();
            if (input.bad()) return std::nullopt;
            return contents.str();
        }

        std::string ScriptSafeJson(const Json::Value& value)
        {
            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string json = Json::writeString(builder, value);
            std::string escaped;
            escaped.reserve(json.size());
            for (const unsigned char ch : json) {
                if (ch == '<') escaped += "\\u003c";
                else if (ch == '>') escaped += "\\u003e";
                else if (ch == '&') escaped += "\\u0026";
                else escaped.push_back(static_cast<char>(ch));
            }
            return escaped;
        }

        std::string InjectScript(std::string html, const Json::Value& user_info)
        {
            const std::string script = "<script>var SERVER_USER_INFO = " + ScriptSafeJson(user_info) + ";</script>";
            const std::size_t body = html.find("</body>");
            if (body == std::string::npos) html += script;
            else html.insert(body, script);
            return html;
        }

        void WriteResponse(brpc::Controller* controller,
                        int status,
                        std::string_view content_type,
                        std::string_view body,
                        bool head,
                        bool cache_asset = false)
        {
            auto& response = controller->http_response();
            response.set_status_code(status);
            response.set_content_type(std::string(content_type));
            response.SetHeader("Content-Length", std::to_string(body.size()));
            response.SetHeader("Cache-Control", std::string(cache_asset ? kAssetCache : kNoStore));
            if (!cache_asset) {
                response.SetHeader("Pragma", "no-cache");
                response.SetHeader("Expires", "0");
            }
            if (!head) controller->response_attachment().append(body.data(), body.size());
        }

        bool WriteFileResponse(brpc::Controller* controller, const fs::path& path,
                            bool head, bool cache_asset)
        {
            std::error_code error;
            const std::uintmax_t file_size = fs::file_size(path, error);
            if (error || file_size > std::numeric_limits<std::size_t>::max()) return false;

            auto& response = controller->http_response();
            response.set_status_code(brpc::HTTP_STATUS_OK);
            response.set_content_type(MimeType(path));
            response.SetHeader("Cache-Control", std::string(cache_asset ? kAssetCache : kNoStore));
            if (!cache_asset) {
                response.SetHeader("Pragma", "no-cache");
                response.SetHeader("Expires", "0");
            }
            constexpr std::uintmax_t kProgressiveThreshold = 1024 * 1024;
            if (head || file_size <= kProgressiveThreshold)
                response.SetHeader("Content-Length", std::to_string(file_size));
            if (head) return true;

            std::ifstream input(path, std::ios::binary);
            if (!input) return false;
            constexpr std::size_t kChunkSize = 64 * 1024;
            char buffer[kChunkSize];
            butil::intrusive_ptr<brpc::ProgressiveAttachment> writer;
            if (file_size > kProgressiveThreshold) {
                writer = controller->CreateProgressiveAttachment();
                if (!writer) return false;
            }
            while (input) {
                input.read(buffer, sizeof(buffer));
                const std::streamsize count = input.gcount();
                if (count > 0) {
                    const int result = writer ? writer->Write(buffer, static_cast<std::size_t>(count))
                                            : controller->response_attachment().append(
                                                    buffer, static_cast<std::size_t>(count));
                    if (result != 0) return false;
                }
            }
            if (input.bad()) {
                controller->response_attachment().clear();
                return false;
            }
            return true;
        }

        void NotFound(brpc::Controller* controller, bool head)
        {
            WriteResponse(controller, brpc::HTTP_STATUS_NOT_FOUND, "text/plain;charset=utf-8", "Not Found", head);
        }

        int PositiveQuery(const brpc::URI& uri, const char* name, int fallback, int maximum)
        {
            const std::string* value = uri.GetQuery(name);
            if (value == nullptr || value->empty() ||
                !std::all_of(value->begin(), value->end(), [](unsigned char ch) { return std::isdigit(ch); })) {
                return fallback;
            }
            try {
                const unsigned long long parsed = std::stoull(*value);
                if (parsed == 0 || parsed > static_cast<unsigned long long>(maximum)) return fallback;
                return static_cast<int>(parsed);
            } catch (const std::exception&) {
                return fallback;
            }
        }

        std::shared_ptr<QueryStruct> QuestionQueryFrom(const brpc::URI& uri)
        {
            auto query = std::make_shared<QuestionQuery>();
            auto set = [&uri](const char* key, std::string* output) {
                if (const std::string* value = uri.GetQuery(key)) *output = *value;
            };
            set("id", &query->id);
            set("title", &query->title);
            set("difficulty", &query->star);
            if (const std::string* encoded = uri.GetQuery("query_hash"); encoded != nullptr && !encoded->empty()) {
                Json::CharReaderBuilder builder;
                Json::Value parsed;
                std::string errors;
                std::istringstream input(*encoded);
                if (Json::parseFromStream(builder, input, &parsed, &errors) && parsed.isObject()) {
                    if (parsed["id"].isString()) query->id = parsed["id"].asString();
                    if (parsed["title"].isString()) query->title = parsed["title"].asString();
                    if (parsed["difficulty"].isString()) query->star = parsed["difficulty"].asString();
                } else {
                    query->title = *encoded;
                }
            }
            std::string mode = "title";
            std::string keyword;
            set("query_mode", &mode);
            set("q", &keyword);
            if (!keyword.empty()) {
                if (mode == "id") query->id = keyword;
                else if (mode == "both") {
                    query->title = keyword;
                    if (std::all_of(keyword.begin(), keyword.end(), [](unsigned char ch) { return std::isdigit(ch); }))
                        query->id = keyword;
                } else query->title = keyword;
            }
            return query;
        }

        bool IsDigits(std::string_view text)
        {
            return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char ch) { return std::isdigit(ch); });
        }

    } // namespace

    StaticHttpService::StaticHttpService(std::shared_ptr<oj::control::Control> control,
                                        StaticHttpMode mode,
                                        std::string root)
        : StaticHttpService(std::move(control), StaticHttpConfig{mode, std::move(root), {}})
    {}

    StaticHttpService::StaticHttpService(std::shared_ptr<oj::control::Control> control, StaticHttpConfig config)
        : _control(std::move(control)), config_(std::move(config))
    {
        if (config_.root.empty()) config_.root = HTML_PATH;
        if (config_.mode == StaticHttpMode::Admin && !config_.admin_sessions)
            config_.admin_sessions = std::make_shared<oj::admin::AdminSessionStore>();
    }

    StaticHttpService::~StaticHttpService() = default;

    void StaticHttpService::default_method(::google::protobuf::RpcController* controller_base,
                                        const ::oj::common::EmptyRequest*,
                                        ::oj::common::EmptyResponse*,
                                        ::google::protobuf::Closure* done)
    {
        brpc::ClosureGuard done_guard(done);
        auto* controller = static_cast<brpc::Controller*>(controller_base);
        const brpc::HttpMethod method = controller->http_request().method();
        const bool head = method == brpc::HTTP_METHOD_HEAD;
        if (method != brpc::HTTP_METHOD_GET && !head) {
            controller->http_response().SetHeader("Allow", "GET, HEAD");
            WriteResponse(controller, brpc::HTTP_STATUS_METHOD_NOT_ALLOWED,
                        "text/plain;charset=utf-8", "Method Not Allowed", false);
            return;
        }

        const auto decoded = DecodePath(controller->http_request().uri().path());
        if (!decoded || decoded->empty() || decoded->front() != '/' || decoded->rfind("/oj.biz.", 0) == 0) {
            NotFound(controller, head);
            return;
        }
        const std::string& path = *decoded;

        if (path == "/healthz") {
            WriteResponse(controller, brpc::HTTP_STATUS_OK, "text/plain;charset=utf-8", "ok", head);
            return;
        }

        if (config_.mode == StaticHttpMode::Admin) {
            if (path == "/") {
                controller->http_response().set_status_code(brpc::HTTP_STATUS_FOUND);
                controller->http_response().SetHeader("Location", "/admin/login");
                controller->http_response().SetHeader("Content-Length", "0");
                controller->http_response().SetHeader("Cache-Control", std::string(kNoStore));
                return;
            }
            if (path == "/admin/login") {
                const auto file = ResolveFile(config_.root, "/admin/login.html");
                const auto body = file ? ReadBinary(*file) : std::nullopt;
                if (!body) NotFound(controller, head);
                else WriteResponse(controller, brpc::HTTP_STATUS_OK, "text/html;charset=utf-8", *body, head);
                return;
            }

            bool shell = path == "/admin" || path == "/admin/users" || path == "/admin/questions" ||
                        path == "/admin/questions/new";
            constexpr std::string_view prefix = "/admin/questions/";
            if (!shell && path.rfind(prefix, 0) == 0) shell = IsDigits(std::string_view(path).substr(prefix.size()));
            if (shell) {
                oj::admin::AdminSession session;
                const bool authenticated = config_.admin_sessions &&
                    oj::rpc::AdminSessionAdapter::GetSession(*controller, *config_.admin_sessions, &session) &&
                    session.admin_id > 0 && session.uid > 0 &&
                    (session.role == "admin" || session.role == "super_admin");
                if (!authenticated) {
                    NotFound(controller, head);
                    return;
                }
                std::string html;
                if (!_control || !_control->GetStaticHtml("admin/index.html", &html) || html.empty()) {
                    NotFound(controller, head);
                    return;
                }
                Json::Value info;
                info["isLoggedIn"] = true;
                info["name"] = session.name;
                info["email"] = session.email;
                info["uid"] = session.uid;
                info["admin_id"] = session.admin_id;
                info["role"] = session.role;
                WriteResponse(controller, brpc::HTTP_STATUS_OK, "text/html;charset=utf-8",
                            InjectScript(std::move(html), info), head);
                return;
            }
        } else {
            std::string template_name;
            if (path == "/") template_name = "index.html";
            else if (path == "/about") template_name = "about.html";
            else if (path == "/user/profile") template_name = "user/profile.html";
            else if (path == "/user/settings") template_name = "user/settings.html";
            else if (path == "/judge_result.html") template_name = "judge_result.html";
            else if (path.rfind("/solutions/", 0) == 0 && IsDigits(std::string_view(path).substr(11)))
                template_name = "solutions/detail.html";

            constexpr std::string_view questions = "/questions/";
            if (path.rfind(questions, 0) == 0) {
                const std::string_view suffix = std::string_view(path).substr(questions.size());
                const std::size_t slash = suffix.find('/');
                if (slash == std::string_view::npos && IsDigits(suffix)) {
                    std::string html;
                    if (!_control || !_control->Question().OneQuestion(std::string(suffix), &html)) {
                        NotFound(controller, head);
                        return;
                    }
                    template_name.clear();
                    User user{};
                    const bool logged_in = _control->GetSessionUser(SessionAdapter::Cookie(*controller), &user);
                    Json::Value info;
                    info["isLoggedIn"] = logged_in;
                    if (logged_in) {
                        info["name"] = user.name; info["email"] = user.email; info["uid"] = user.uid;
                        info["avatar_url"] = _control->GetEffectiveAvatarUrl(user);
                    }
                    WriteResponse(controller, brpc::HTTP_STATUS_OK, "text/html;charset=utf-8",
                                InjectScript(std::move(html), info), head);
                    return;
                }
                if (slash != std::string_view::npos && IsDigits(suffix.substr(0, slash))) {
                    const std::string_view rest = suffix.substr(slash);
                    if (rest == "/solutions/new") template_name = "solutions/new.html";
                    else if (rest.rfind("/solutions/", 0) == 0 && IsDigits(rest.substr(11))) {
                        const std::string location = "/questions/" + std::string(suffix.substr(0, slash));
                        controller->http_response().set_status_code(brpc::HTTP_STATUS_FOUND);
                        controller->http_response().SetHeader("Location", location);
                        controller->http_response().SetHeader("Content-Length", "0");
                        controller->http_response().SetHeader("Cache-Control", std::string(kNoStore));
                        return;
                    }
                }
            }

            if (path == "/all_questions") {
                std::string html;
                const brpc::URI& uri = controller->http_request().uri();
                if (!_control || !_control->Question().AllQuestions(
                                    &html, PositiveQuery(uri, "page", 1, 1000000),
                                    PositiveQuery(uri, "size", 5, 100), QuestionQueryFrom(uri))) {
                    NotFound(controller, head);
                    return;
                }
                User user{};
                const bool logged_in = _control->GetSessionUser(SessionAdapter::Cookie(*controller), &user);
                Json::Value info;
                info["isLoggedIn"] = logged_in;
                if (logged_in) {
                    info["name"] = user.name; info["email"] = user.email; info["uid"] = user.uid;
                    info["avatar_url"] = _control->GetEffectiveAvatarUrl(user);
                }
                WriteResponse(controller, brpc::HTTP_STATUS_OK, "text/html;charset=utf-8",
                            InjectScript(std::move(html), info), head);
                return;
            }

            if (!template_name.empty()) {
                std::string html;
                if (!_control || !_control->GetStaticHtml(template_name, &html) || html.empty()) {
                    NotFound(controller, head);
                    return;
                }
                User user{};
                const bool logged_in = _control->GetSessionUser(SessionAdapter::Cookie(*controller), &user);
                Json::Value info;
                info["isLoggedIn"] = logged_in;
                if (logged_in) {
                    info["name"] = user.name; info["email"] = user.email; info["uid"] = user.uid;
                    info["avatar_url"] = _control->GetEffectiveAvatarUrl(user);
                }
                WriteResponse(controller, brpc::HTTP_STATUS_OK, "text/html;charset=utf-8",
                            InjectScript(std::move(html), info), head);
                return;
            }
        }

        std::string asset_path = path;
        if (config_.mode == StaticHttpMode::Admin) {
            if (path.rfind("/admin/assets/", 0) != 0) {
                NotFound(controller, head);
                return;
            }
            asset_path = "/admin/" + path.substr(std::string("/admin/assets/").size());
        }
        const auto file = ResolveFile(config_.root, asset_path);
        if (!file || !WriteFileResponse(controller, *file, head,
                                        file->extension() != ".html" && file->extension() != ".htm")) {
            NotFound(controller, head);
            return;
        }
    }

} // namespace oj::rpc
