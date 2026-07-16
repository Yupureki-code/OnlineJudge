#include "rpc/oj_admin_service_impl.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <brpc/controller.h>
#include <jsoncpp/json/json.h>

#include "comm.hpp"
#include "control/oj_control.hpp"
#include "oj_admin.hpp"
#include "rpc/admin_audit_worker.hpp"
#include "rpc/async_dispatch.hpp"
#include "rpc/proto_adapter.hpp"
#include "rpc/session_adapter.hpp"
#include "runtime/business_executor.hpp"

namespace oj::rpc
{
namespace
{

constexpr uint32_t kMaxTestOrdinal = 127;
constexpr std::size_t kMaxEncodedQuestionIdLength = 5;

bool IsDigits(const std::string& value)
{
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

uint32_t Page(const oj::common::PageRequest& page)
{
    return page.page() == 0 ? 1 : page.page();
}

uint32_t PageSize(const oj::common::PageRequest& page)
{
    return std::clamp(page.page_size() == 0 ? 20u : page.page_size(), 1u, 200u);
}

void SetError(oj::common::StatusResponse* status, int code, const char* message,
              bool retryable = false)
{
    ProtoAdapter::SetError(status, code, message, retryable);
}

void ToProto(const AdminAccount& input, oj::common::AdminAccount* output)
{
    output->set_admin_id(input.admin_id < 0 ? 0 : static_cast<uint32_t>(input.admin_id));
    output->set_uid(input.uid < 0 ? 0 : static_cast<uint32_t>(input.uid));
    output->set_role(input.role);
    output->set_created_at(oj::util::TimeUtil::DateTimeToInt(input.created_at));
}

bool ToLegacy(const oj::common::Question& input, Question* output)
{
    if (output == nullptr || !IsDigits(input.id()) || input.title().empty() ||
        input.difficulty().empty() || input.time_limit_ms() == 0 || input.memory_limit_mb() == 0)
        return false;
    const uint64_t seconds = (static_cast<uint64_t>(input.time_limit_ms()) + 999) / 1000;
    const uint64_t bytes = static_cast<uint64_t>(input.memory_limit_mb()) * 1024 * 1024;
    if (seconds > static_cast<uint64_t>(std::numeric_limits<int8_t>::max()) ||
        bytes > static_cast<uint64_t>(std::numeric_limits<int>::max()))
        return false;
    output->id = input.id();
    output->title = input.title();
    output->desc = input.description();
    output->star = input.difficulty();
    output->cpu_limit = static_cast<int>(seconds);
    output->memory_limit = static_cast<int>(bytes);
    return true;
}

uint64_t EncodeTestId(const std::string& question_id, uint32_t ordinal)
{
    if (!IsDigits(question_id) || question_id.size() > kMaxEncodedQuestionIdLength ||
        ordinal == 0 || ordinal > kMaxTestOrdinal)
        return 0;
    uint64_t encoded_question = 0;
    for (unsigned char ch : question_id)
        encoded_question = (encoded_question << 8) | ch;
    return (static_cast<uint64_t>(ordinal) << 48) |
           (static_cast<uint64_t>(question_id.size()) << 40) | encoded_question;
}

bool DecodeTestId(uint64_t encoded, std::string* question_id, int* ordinal)
{
    if (question_id == nullptr || ordinal == nullptr || encoded == 0)
        return false;
    const uint64_t value = encoded >> 48;
    const std::size_t length = static_cast<std::size_t>((encoded >> 40) & 0xff);
    if (value == 0 || value > kMaxTestOrdinal || length == 0 || length > kMaxEncodedQuestionIdLength)
        return false;
    question_id->assign(length, '0');
    uint64_t question = encoded & ((uint64_t{1} << 40) - 1);
    for (std::size_t index = length; index > 0; --index) {
        const char ch = static_cast<char>(question & 0xff);
        if (ch < '0' || ch > '9') return false;
        (*question_id)[index - 1] = ch;
        question >>= 8;
    }
    if (question != 0) return false;
    *ordinal = static_cast<int>(value);
    return true;
}

void FillTestCase(const Json::Value& input, oj::common::TestCase* output)
{
    ProtoAdapter::ToProto(input, output);
    const uint32_t ordinal = output->ordinal();
    output->set_test_case_id(EncodeTestId(output->question_id(), ordinal));
}

template <typename Response>
void SetNotImplemented(Response* response)
{
    SetError(response->mutable_status(), 501, "NOT_IMPLEMENTED");
}

} // namespace

OJAdminServiceImpl::OJAdminServiceImpl(oj::control::Control& control,
                                       oj::admin::AdminSessionStore& sessions,
                                       oj::runtime::BusinessExecutor& executor,
                                       AdminAuditWorker* audit_worker)
    : _control(control), sessions_(sessions), _executor(executor), audit_worker_(audit_worker)
{
}

void OJAdminServiceImpl::WriteAudit(brpc::Controller* controller, const AdminAccount& admin,
                                    const char* action, const char* resource_type,
                                    const std::string& resource_id)
{
    if (audit_worker_ == nullptr) return;
    std::string request_id;
    if (controller != nullptr && controller->has_http_request()) {
        const std::string* header = controller->http_request().GetHeader("X-Request-Id");
        if (header != nullptr && !header->empty() && header->size() <= 64) request_id = *header;
    }
    if (request_id.empty()) {
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        request_id = "req_" + std::to_string(milliseconds) + "_" +
                     std::to_string(audit_sequence_.fetch_add(1, std::memory_order_relaxed) + 1);
    }
    Json::Value payload;
    payload["resource_id"] = resource_id;
    if (controller != nullptr) payload["remote_ip"] = SessionAdapter::RemoteIp(*controller);
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    if (!audit_worker_->Enqueue(std::move(request_id), admin, action, resource_type,
                                "success", Json::writeString(builder, payload))) {
        LOG_WARNING("admin audit queue unavailable or full, action={}", action);
    }
}

bool OJAdminServiceImpl::IsAdminRole(const std::string& role)
{
    return role == "admin" || role == "super_admin";
}

bool OJAdminServiceImpl::IsSuperAdminRole(const std::string& role)
{
    return role == "super_admin";
}

bool OJAdminServiceImpl::RequireAdmin(brpc::Controller* controller, bool super_only,
                                      AdminAccount* current, oj::common::StatusResponse* status) const
{
    if (controller == nullptr) {
        SetError(status, 400, "BRPC_CONTROLLER_REQUIRED");
        return false;
    }
    oj::admin::AdminSession session;
    if (!AdminSessionAdapter::GetSession(*controller, sessions_, &session)) {
        SetError(status, 401, "UNAUTHORIZED");
        return false;
    }
    AdminAccount by_id;
    AdminAccount by_uid;
    auto* model = _control.GetModel();
    if (session.admin_id <= 0 || session.uid <= 0 ||
        !model->GetAdminByAdminId(session.admin_id, &by_id) ||
        !model->GetAdminByUid(session.uid, &by_uid) ||
        by_id.admin_id != by_uid.admin_id || by_id.uid != session.uid || !IsAdminRole(by_id.role)) {
        SetError(status, 401, "STALE_ADMIN_SESSION");
        return false;
    }
    if (super_only && !IsSuperAdminRole(by_id.role)) {
        SetError(status, 403, "FORBIDDEN");
        return false;
    }
    if (current != nullptr)
        *current = std::move(by_id);
    return true;
}

#define OJ_ADMIN_DISPATCH(RequestType, ResponseType, ...) \
    AsyncDispatch(_executor, controller, request, response, done, \
        [this](brpc::Controller* cntl, const RequestType* req, ResponseType* rsp) __VA_ARGS__)

void OJAdminServiceImpl::AdminGetVersion(google::protobuf::RpcController* controller,
    const oj::common::EmptyRequest* request, oj::biz::GetVersionResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::common::EmptyRequest, oj::biz::GetVersionResponse, {
        (void)cntl; (void)req;
        ProtoAdapter::SetOk(rsp->mutable_status());
        rsp->set_service("oj_admin");
        rsp->set_version(oj::admin::kAdminVersion);
        rsp->set_build_date(__DATE__);
        rsp->set_build_time(__TIME__);
    });
}

void OJAdminServiceImpl::AdminLogin(google::protobuf::RpcController* controller,
    const oj::biz::AdminLoginRequest* request, oj::biz::AdminLoginResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::AdminLoginRequest, oj::biz::AdminLoginResponse, {
        if (req->admin_id() == 0 || req->password().empty()) {
            SetError(rsp->mutable_status(), 400, "INVALID_ARGUMENT");
            return;
        }
        AdminAccount admin;
        User user;
        std::string error;
        if (!_control.Auth().LoginAdminWithIdAndPassword(
                static_cast<int>(req->admin_id()), req->password(), &admin, &user, &error)) {
            SetError(rsp->mutable_status(), 401, "INVALID_CREDENTIALS");
            return;
        }
        if (!IsAdminRole(admin.role)) {
            SetError(rsp->mutable_status(), 403, "FORBIDDEN");
            return;
        }
        if (cntl == nullptr || AdminSessionAdapter::CreateSession(*cntl, sessions_, admin, user).empty()) {
            SetError(rsp->mutable_status(), 500, "CREATE_SESSION_FAILED");
            return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        ToProto(admin, rsp->mutable_admin());
    });
}

void OJAdminServiceImpl::AdminLogout(google::protobuf::RpcController* controller,
    const oj::common::EmptyRequest* request, oj::common::EmptyResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::common::EmptyRequest, oj::common::EmptyResponse, {
        (void)req;
        if (cntl == nullptr) { SetError(rsp->mutable_status(), 400, "BRPC_CONTROLLER_REQUIRED"); return; }
        AdminSessionAdapter::DestroySession(*cntl, sessions_);
        ProtoAdapter::SetOk(rsp->mutable_status());
    });
}

void OJAdminServiceImpl::AdminGetOverview(google::protobuf::RpcController* controller,
    const oj::common::EmptyRequest* request, oj::biz::AdminOverviewResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::common::EmptyRequest, oj::biz::AdminOverviewResponse, {
        (void)req;
        if (!RequireAdmin(cntl, false, nullptr, rsp->mutable_status())) return;
        int users = 0, questions = 0;
        auto* model = _control.GetModel();
        std::string cached;
        if (model->GetCachedStringByAnyKey("admin:overview:snapshot", &cached)) {
            Json::CharReaderBuilder reader;
            Json::Value snapshot;
            std::istringstream input(cached);
            if (Json::parseFromStream(reader, input, &snapshot, nullptr) &&
                snapshot["users"].isInt() && snapshot["questions"].isInt() &&
                snapshot["solutions"].isInt()) {
                ProtoAdapter::SetOk(rsp->mutable_status());
                rsp->set_total_users(snapshot["users"].asInt());
                rsp->set_total_questions(snapshot["questions"].asInt());
                rsp->set_total_solutions(snapshot["solutions"].asInt());
                return;
            }
        }
        if (!model->User().GetUserCount(&users) || !model->Question().GetQuestionCount(&questions)) {
            SetError(rsp->mutable_status(), 500, "LOAD_OVERVIEW_FAILED", true); return;
        }
        std::vector<Solution> solutions;
        int solution_count = 0, pages = 0;
        if (!model->Solution().GetSolutionsByPage("", "", "latest", 1, 1,
                                                   &solutions, &solution_count, &pages))
            solution_count = 0;
        ProtoAdapter::SetOk(rsp->mutable_status());
        rsp->set_total_users(users); rsp->set_total_questions(questions);
        rsp->set_total_solutions(solution_count);
        Json::Value snapshot;
        snapshot["users"] = users;
        snapshot["questions"] = questions;
        snapshot["solutions"] = solution_count;
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        model->SetCachedStringByAnyKey("admin:overview:snapshot",
            Json::writeString(writer, snapshot), model->BuildCacheJitteredTtlSeconds(30, 5));
    });
}

void OJAdminServiceImpl::AdminListUsers(google::protobuf::RpcController* controller,
    const oj::biz::AdminListUsersRequest* request, oj::biz::AdminListUsersResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::AdminListUsersRequest, oj::biz::AdminListUsersResponse, {
        if (!RequireAdmin(cntl, false, nullptr, rsp->mutable_status())) return;
        auto query = std::make_shared<UserQuery>();
        query->name = req->search(); query->email = req->search();
        KeyContext context; context._query = query; context.page = Page(req->page());
        context.size = PageSize(req->page()); context.list_version = "1"; context.list_type = ListType::Users;
        auto key = std::make_shared<oj::cache::Cache::CacheListKey>(); key->Build(context);
        std::vector<User> users; int total = 0, pages = 0;
        if (!_control.GetModel()->User().GetUsersPaged(key, &users, &total, &pages)) {
            SetError(rsp->mutable_status(), 500, "LOAD_USERS_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        ProtoAdapter::SetPage(req->page(), total, rsp->mutable_page(), 20, 200);
        for (const auto& user : users) ProtoAdapter::ToProto(user, rsp->add_users());
    });
}

#define OJ_ADMIN_NOT_IMPLEMENTED(Method, RequestType, ResponseType) \
void OJAdminServiceImpl::Method(google::protobuf::RpcController* controller, \
    const RequestType* request, ResponseType* response, google::protobuf::Closure* done) \
{ OJ_ADMIN_DISPATCH(RequestType, ResponseType, { (void)cntl; (void)req; SetNotImplemented(rsp); }); }

OJ_ADMIN_NOT_IMPLEMENTED(AdminGetUser, oj::biz::AdminUserIdRequest, oj::biz::AdminUserResponse)
OJ_ADMIN_NOT_IMPLEMENTED(AdminCreateUser, oj::biz::AdminCreateUserRequest, oj::biz::AdminUserResponse)
OJ_ADMIN_NOT_IMPLEMENTED(AdminUpdateUser, oj::biz::AdminUpdateUserRequest, oj::biz::AdminUserResponse)
OJ_ADMIN_NOT_IMPLEMENTED(AdminDeleteUser, oj::biz::AdminUserIdRequest, oj::common::EmptyResponse)

void OJAdminServiceImpl::AdminListQuestions(google::protobuf::RpcController* controller,
    const oj::biz::GetQuestionListRequest* request, oj::biz::GetQuestionListResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::GetQuestionListRequest, oj::biz::GetQuestionListResponse, {
        if (!RequireAdmin(cntl, false, nullptr, rsp->mutable_status())) return;
        auto query = std::make_shared<QuestionQuery>();
        query->id = req->search(); query->title = req->search(); query->star = req->difficulty();
        auto* model = _control.GetModel();
        KeyContext context; context._query = query; context.page = Page(req->page());
        context.size = PageSize(req->page()); context.list_version = model->Question().GetQuestionsListVersion();
        context.list_type = ListType::Questions;
        auto key = std::make_shared<oj::cache::Cache::CacheListKey>(); key->Build(context);
        std::vector<Question> questions; int total = 0, pages = 0;
        if (!model->Question().GetQuestionsByPage(key, questions, &total, &pages)) {
            SetError(rsp->mutable_status(), 500, "LOAD_QUESTIONS_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        ProtoAdapter::SetPage(req->page(), total, rsp->mutable_page(), 20, 200);
        for (const auto& question : questions) ProtoAdapter::ToProto(question, rsp->add_questions());
    });
}

void OJAdminServiceImpl::AdminGetQuestion(google::protobuf::RpcController* controller,
    const oj::biz::QuestionIdRequest* request, oj::biz::GetQuestionDetailResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::QuestionIdRequest, oj::biz::GetQuestionDetailResponse, {
        if (!RequireAdmin(cntl, false, nullptr, rsp->mutable_status())) return;
        if (!IsDigits(req->question_id())) { SetError(rsp->mutable_status(), 400, "INVALID_QUESTION_ID"); return; }
        Question question;
        if (!_control.GetModel()->Question().GetOneQuestion(req->question_id(), question)) {
            SetError(rsp->mutable_status(), 404, "QUESTION_NOT_FOUND"); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status()); ProtoAdapter::ToProto(question, rsp->mutable_question());
    });
}

void OJAdminServiceImpl::AdminCreateQuestion(google::protobuf::RpcController* controller,
    const oj::biz::SaveQuestionRequest* request, oj::biz::QuestionResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, false, &current, rsp->mutable_status())) return;
        Question question;
        if (!req->has_question() || !ToLegacy(req->question(), &question)) {
            SetError(rsp->mutable_status(), 400, "INVALID_QUESTION"); return;
        }
        if (!_control.Question().SaveQuestion(question)) {
            SetError(rsp->mutable_status(), 500, "SAVE_QUESTION_FAILED", true); return;
        }
        _control.GetModel()->DeleteCachedStringByAnyKey("admin:overview:snapshot");
        WriteAudit(cntl, current, "create", "question", question.id);
        ProtoAdapter::SetOk(rsp->mutable_status()); ProtoAdapter::ToProto(question, rsp->mutable_question());
    });
}

void OJAdminServiceImpl::AdminUpdateQuestion(google::protobuf::RpcController* controller,
    const oj::biz::SaveQuestionRequest* request, oj::biz::QuestionResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, false, &current, rsp->mutable_status())) return;
        Question question;
        if (!req->has_question() || !ToLegacy(req->question(), &question)) {
            SetError(rsp->mutable_status(), 400, "INVALID_QUESTION"); return;
        }
        if (!_control.Question().SaveQuestion(question)) {
            SetError(rsp->mutable_status(), 500, "UPDATE_QUESTION_FAILED", true); return;
        }
        _control.GetModel()->DeleteCachedStringByAnyKey("admin:overview:snapshot");
        WriteAudit(cntl, current, "update", "question", question.id);
        ProtoAdapter::SetOk(rsp->mutable_status()); ProtoAdapter::ToProto(question, rsp->mutable_question());
    });
}

void OJAdminServiceImpl::AdminDeleteQuestion(google::protobuf::RpcController* controller,
    const oj::biz::QuestionIdRequest* request, oj::common::EmptyResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::QuestionIdRequest, oj::common::EmptyResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, false, &current, rsp->mutable_status())) return;
        if (!IsDigits(req->question_id())) { SetError(rsp->mutable_status(), 400, "INVALID_QUESTION_ID"); return; }
        if (!_control.Question().DeleteQuestion(req->question_id())) {
            SetError(rsp->mutable_status(), 500, "DELETE_QUESTION_FAILED", true); return;
        }
        _control.GetModel()->DeleteCachedStringByAnyKey("admin:overview:snapshot");
        WriteAudit(cntl, current, "delete", "question", req->question_id());
        ProtoAdapter::SetOk(rsp->mutable_status());
    });
}

void OJAdminServiceImpl::AdminListTestCases(google::protobuf::RpcController* controller,
    const oj::biz::ListTestCasesRequest* request, oj::biz::ListTestCasesResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::ListTestCasesRequest, oj::biz::ListTestCasesResponse, {
        if (!RequireAdmin(cntl, true, nullptr, rsp->mutable_status())) return;
        if (!IsDigits(req->question_id())) { SetError(rsp->mutable_status(), 400, "INVALID_QUESTION_ID"); return; }
        Json::Value tests;
        if (!_control.GetModel()->Question().GetTestsByQuestionId(req->question_id(), &tests)) {
            SetError(rsp->mutable_status(), 500, "LOAD_TEST_CASES_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        for (const auto& test : tests["tests"]) {
            if (!req->sample_only() || test["is_sample"].asBool()) FillTestCase(test, rsp->add_test_cases());
        }
    });
}

void OJAdminServiceImpl::AdminCreateTestCase(google::protobuf::RpcController* controller,
    const oj::biz::CreateTestCaseRequest* request, oj::biz::TestCaseResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::CreateTestCaseRequest, oj::biz::TestCaseResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, true, &current, rsp->mutable_status())) return;
        if (!IsDigits(req->question_id()) || req->ordinal() == 0 || req->ordinal() > kMaxTestOrdinal) {
            SetError(rsp->mutable_status(), 400, "INVALID_TEST_CASE_KEY"); return;
        }
        Json::Value existing;
        if (!_control.GetModel()->Question().GetTestsByQuestionId(req->question_id(), &existing)) {
            SetError(rsp->mutable_status(), 500, "LOAD_TEST_CASES_FAILED", true); return;
        }
        const uint32_t expected = existing["tests"].empty() ? 1u :
            existing["tests"][existing["tests"].size() - 1]["id"].asUInt() + 1;
        if (req->ordinal() != expected) { SetError(rsp->mutable_status(), 409, "TEST_ORDINAL_CONFLICT"); return; }
        int ordinal = 0;
        if (!_control.GetModel()->Question().CreateTestCase(req->question_id(), req->input(),
                req->expected_output(), req->is_sample(), &ordinal) || ordinal != static_cast<int>(req->ordinal())) {
            SetError(rsp->mutable_status(), 500, "CREATE_TEST_CASE_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        auto* test = rsp->mutable_test_case(); test->set_question_id(req->question_id());
        test->set_ordinal(req->ordinal()); test->set_test_case_id(EncodeTestId(req->question_id(), req->ordinal()));
        test->set_input(req->input()); test->set_expected_output(req->expected_output()); test->set_is_sample(req->is_sample());
        WriteAudit(cntl, current, "create", "test_case",
                   req->question_id() + ":" + std::to_string(req->ordinal()));
    });
}

void OJAdminServiceImpl::AdminUpdateTestCase(google::protobuf::RpcController* controller,
    const oj::biz::UpdateTestCaseRequest* request, oj::biz::TestCaseResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::UpdateTestCaseRequest, oj::biz::TestCaseResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, true, &current, rsp->mutable_status())) return;
        if (!req->has_test_case() || !IsDigits(req->test_case().question_id()) ||
            req->test_case().ordinal() == 0 || req->test_case().ordinal() > kMaxTestOrdinal) {
            SetError(rsp->mutable_status(), 400, "INVALID_TEST_CASE_KEY"); return;
        }
        const auto& test = req->test_case();
        const uint64_t encoded = EncodeTestId(test.question_id(), test.ordinal());
        if (test.test_case_id() != 0 && test.test_case_id() != encoded) {
            SetError(rsp->mutable_status(), 400, "TEST_CASE_ID_MISMATCH"); return;
        }
        if (!_control.GetModel()->Question().UpdateTestCase(static_cast<int>(test.ordinal()),
                test.question_id(), test.input(), test.expected_output(), test.is_sample())) {
            SetError(rsp->mutable_status(), 404, "TEST_CASE_NOT_FOUND"); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status()); rsp->mutable_test_case()->CopyFrom(test);
        rsp->mutable_test_case()->set_test_case_id(encoded);
        WriteAudit(cntl, current, "update", "test_case",
                   test.question_id() + ":" + std::to_string(test.ordinal()));
    });
}

void OJAdminServiceImpl::AdminDeleteTestCase(google::protobuf::RpcController* controller,
    const oj::biz::TestCaseIdRequest* request, oj::common::EmptyResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::TestCaseIdRequest, oj::common::EmptyResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, true, &current, rsp->mutable_status())) return;
        std::string question_id; int ordinal = 0;
        if (!DecodeTestId(req->test_case_id(), &question_id, &ordinal)) {
            SetError(rsp->mutable_status(), 400, "INVALID_TEST_CASE_ID"); return;
        }
        if (!_control.GetModel()->Question().DeleteTestCase(ordinal, question_id)) {
            SetError(rsp->mutable_status(), 500, "DELETE_TEST_CASE_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        WriteAudit(cntl, current, "delete", "test_case",
                   question_id + ":" + std::to_string(ordinal));
    });
}

void OJAdminServiceImpl::AdminInvalidateQuestionCache(google::protobuf::RpcController* controller,
    const oj::biz::InvalidateQuestionCacheRequest* request, oj::common::EmptyResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::InvalidateQuestionCacheRequest, oj::common::EmptyResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, false, &current, rsp->mutable_status())) return;
        if (!req->question_id().empty() && !IsDigits(req->question_id())) {
            SetError(rsp->mutable_status(), 400, "INVALID_QUESTION_ID"); return;
        }
        _control.Question().TouchQuestionListVersion(); ProtoAdapter::SetOk(rsp->mutable_status());
        WriteAudit(cntl, current, "invalidate_cache", "question", req->question_id());
    });
}

void OJAdminServiceImpl::AdminListAdminAccounts(google::protobuf::RpcController* controller,
    const oj::biz::ListAdminAccountsRequest* request, oj::biz::ListAdminAccountsResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::ListAdminAccountsRequest, oj::biz::ListAdminAccountsResponse, {
        if (!RequireAdmin(cntl, true, nullptr, rsp->mutable_status())) return;
        std::vector<AdminAccount> accounts; int total = 0;
        if (!_control.GetModel()->ListAdminsPaged(Page(req->page()), PageSize(req->page()),
                req->search(), &accounts, &total)) {
            SetError(rsp->mutable_status(), 500, "LOAD_ADMINS_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        ProtoAdapter::SetPage(req->page(), total, rsp->mutable_page(), 20, 200);
        for (const auto& account : accounts) ToProto(account, rsp->add_accounts());
    });
}

void OJAdminServiceImpl::AdminGetAdminAccount(google::protobuf::RpcController* controller,
    const oj::biz::AdminAccountIdRequest* request, oj::biz::AdminAccountResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::AdminAccountIdRequest, oj::biz::AdminAccountResponse, {
        if (!RequireAdmin(cntl, true, nullptr, rsp->mutable_status())) return;
        AdminAccount account;
        if (req->admin_id() == 0 || !_control.GetModel()->GetAdminByAdminId(req->admin_id(), &account)) {
            SetError(rsp->mutable_status(), 404, "ADMIN_NOT_FOUND"); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status()); ToProto(account, rsp->mutable_account());
    });
}

void OJAdminServiceImpl::AdminCreateAdminAccount(google::protobuf::RpcController* controller,
    const oj::biz::SaveAdminAccountRequest* request, oj::biz::AdminAccountResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::SaveAdminAccountRequest, oj::biz::AdminAccountResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, true, &current, rsp->mutable_status())) return;
        if (!req->has_account() || req->account().uid() == 0 || !IsAdminRole(req->account().role())) {
            SetError(rsp->mutable_status(), 400, "INVALID_ADMIN_ACCOUNT"); return;
        }
        User user; AdminAccount existing;
        if (!_control.GetModel()->User().GetUserById(req->account().uid(), &user)) {
            SetError(rsp->mutable_status(), 404, "USER_NOT_FOUND"); return;
        }
        if (_control.GetModel()->GetAdminByUid(req->account().uid(), &existing)) {
            SetError(rsp->mutable_status(), 409, "ADMIN_ALREADY_EXISTS"); return;
        }
        if (IsSuperAdminRole(req->account().role())) {
            int count = 0;
            if (!_control.GetModel()->GetRoleCount("super_admin", &count)) {
                SetError(rsp->mutable_status(), 500, "COUNT_SUPER_ADMIN_FAILED", true); return;
            }
            if (count > 0) { SetError(rsp->mutable_status(), 409, "ONLY_ONE_SUPER_ADMIN"); return; }
        }
        std::string hash, error;
        if (!_control.Auth().BuildSecurePasswordHash(req->password(), &hash, &error)) {
            SetError(rsp->mutable_status(), 400, error == "WEAK_PASSWORD" ? "WEAK_PASSWORD" : "PASSWORD_HASH_FAILED"); return;
        }
        if (!_control.GetModel()->CreateAdminAccount(req->account().uid(), hash, req->account().role())) {
            SetError(rsp->mutable_status(), 500, "CREATE_ADMIN_FAILED", true); return;
        }
        if (!_control.GetModel()->GetAdminByUid(req->account().uid(), &existing)) {
            SetError(rsp->mutable_status(), 500, "LOAD_CREATED_ADMIN_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status()); ToProto(existing, rsp->mutable_account());
        WriteAudit(cntl, current, "create", "admin_account", std::to_string(existing.admin_id));
    });
}

void OJAdminServiceImpl::AdminUpdateAdminAccount(google::protobuf::RpcController* controller,
    const oj::biz::SaveAdminAccountRequest* request, oj::biz::AdminAccountResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::SaveAdminAccountRequest, oj::biz::AdminAccountResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, true, &current, rsp->mutable_status())) return;
        if (!req->has_account() || req->account().admin_id() == 0 || !IsAdminRole(req->account().role())) {
            SetError(rsp->mutable_status(), 400, "INVALID_ADMIN_ACCOUNT"); return;
        }
        AdminAccount target;
        if (!_control.GetModel()->GetAdminByAdminId(req->account().admin_id(), &target)) {
            SetError(rsp->mutable_status(), 404, "ADMIN_NOT_FOUND"); return;
        }
        const std::string& role = req->account().role();
        if (current.admin_id == target.admin_id && IsSuperAdminRole(target.role) && !IsSuperAdminRole(role)) {
            SetError(rsp->mutable_status(), 400, "SUPER_ADMIN_SELF_DOWNGRADE_FORBIDDEN"); return;
        }
        int super_count = 0;
        if (target.role != role && (IsSuperAdminRole(target.role) || IsSuperAdminRole(role))) {
            if (!_control.GetModel()->GetRoleCount("super_admin", &super_count)) {
                SetError(rsp->mutable_status(), 500, "COUNT_SUPER_ADMIN_FAILED", true); return;
            }
            if (IsSuperAdminRole(target.role) && super_count <= 1) {
                SetError(rsp->mutable_status(), 409, "LAST_SUPER_ADMIN_PROTECTED"); return;
            }
            if (IsSuperAdminRole(role) && super_count > 0) {
                SetError(rsp->mutable_status(), 409, "ONLY_ONE_SUPER_ADMIN"); return;
            }
        }
        if (!_control.GetModel()->UpdateAdminRole(target.admin_id, role)) {
            SetError(rsp->mutable_status(), 500, "UPDATE_ADMIN_FAILED", true); return;
        }
        target.role = role; ProtoAdapter::SetOk(rsp->mutable_status()); ToProto(target, rsp->mutable_account());
        WriteAudit(cntl, current, "update", "admin_account", std::to_string(target.admin_id));
    });
}

void OJAdminServiceImpl::AdminDeleteAdminAccount(google::protobuf::RpcController* controller,
    const oj::biz::AdminAccountIdRequest* request, oj::common::EmptyResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::AdminAccountIdRequest, oj::common::EmptyResponse, {
        AdminAccount current;
        if (!RequireAdmin(cntl, true, &current, rsp->mutable_status())) return;
        AdminAccount target;
        if (req->admin_id() == 0 || !_control.GetModel()->GetAdminByAdminId(req->admin_id(), &target)) {
            SetError(rsp->mutable_status(), 404, "ADMIN_NOT_FOUND"); return;
        }
        if (current.admin_id == target.admin_id) {
            SetError(rsp->mutable_status(), 400, "ADMIN_SELF_DELETE_FORBIDDEN"); return;
        }
        if (IsSuperAdminRole(target.role)) {
            int count = 0;
            if (!_control.GetModel()->GetRoleCount("super_admin", &count) || count <= 1) {
                SetError(rsp->mutable_status(), 409, "LAST_SUPER_ADMIN_PROTECTED"); return;
            }
        }
        if (!_control.GetModel()->DeleteAdminAccount(target.admin_id)) {
            SetError(rsp->mutable_status(), 500, "DELETE_ADMIN_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        WriteAudit(cntl, current, "delete", "admin_account", std::to_string(target.admin_id));
    });
}

void OJAdminServiceImpl::AdminGetCacheMetrics(google::protobuf::RpcController* controller,
    const oj::common::EmptyRequest* request, oj::biz::GetCacheMetricsResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::common::EmptyRequest, oj::biz::GetCacheMetricsResponse, {
        (void)req;
        if (!RequireAdmin(cntl, true, nullptr, rsp->mutable_status())) return;
        std::vector<oj::model::Model::CacheMetricsAggregate> rows;
        if (!_control.GetModel()->GetCacheMetricsOneDay(&rows)) {
            SetError(rsp->mutable_status(), 500, "LOAD_CACHE_METRICS_FAILED", true); return;
        }
        uint64_t hits = 0, fallbacks = 0;
        for (const auto& row : rows) {
            hits += std::max(0LL, row.cache_hits);
            fallbacks += std::max(0LL, row.db_fallbacks);
        }
        ProtoAdapter::SetOk(rsp->mutable_status()); auto* metrics = rsp->mutable_metrics();
        metrics->set_hits(hits); metrics->set_misses(fallbacks);
        metrics->set_errors(0); metrics->set_sampled_at(std::time(nullptr));
    });
}

void OJAdminServiceImpl::AdminGetAuditLogs(google::protobuf::RpcController* controller,
    const oj::biz::GetAuditLogsRequest* request, oj::biz::GetAuditLogsResponse* response,
    google::protobuf::Closure* done)
{
    OJ_ADMIN_DISPATCH(oj::biz::GetAuditLogsRequest, oj::biz::GetAuditLogsResponse, {
        if (!RequireAdmin(cntl, false, nullptr, rsp->mutable_status())) return;
        if ((req->start_time() < 0 || req->end_time() < 0) ||
            (req->start_time() > 0 && req->end_time() > 0 && req->start_time() > req->end_time())) {
            SetError(rsp->mutable_status(), 400, "INVALID_AUDIT_FILTER"); return;
        }
        std::vector<AdminAuditLog> logs; int total = 0;
        if (!_control.GetModel()->ListAdminAuditLogsPaged(Page(req->page()), PageSize(req->page()),
                req->action(), 0, "", &logs, &total)) {
            SetError(rsp->mutable_status(), 500, "LOAD_AUDIT_LOGS_FAILED", true); return;
        }
        ProtoAdapter::SetOk(rsp->mutable_status());
        ProtoAdapter::SetPage(req->page(), total, rsp->mutable_page(), 20, 200);
        for (const auto& log : logs) {
            auto* out = rsp->add_logs(); out->set_request_id(log.request_id);
            out->set_operator_admin_id(log.operator_admin_id < 0 ? 0 : log.operator_admin_id);
            out->set_operator_role(log.operator_role); out->set_action(log.action);
            out->set_resource_type(log.resource_type); out->set_result(log.result); out->set_payload_text(log.payload_text);
        }
    });
}

#undef OJ_ADMIN_NOT_IMPLEMENTED
#undef OJ_ADMIN_DISPATCH

} // namespace oj::rpc
