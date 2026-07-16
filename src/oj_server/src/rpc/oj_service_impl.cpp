#include "../../include/rpc/oj_service_impl.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <brpc/controller.h>
#include <jsoncpp/json/json.h>

#include "../../include/control/oj_control.hpp"
#include "../../include/rpc/async_dispatch.hpp"
#include "../../include/rpc/proto_adapter.hpp"
#include "../../include/rpc/session_adapter.hpp"
#include "../../../comm/models/user.hxx"
#include "../../../comm/models/question.hxx"

namespace oj::rpc
{
    using namespace oj::db;
    namespace
    {

    constexpr int kNotImplemented = 501;

    int ErrorCode(const std::string& error)
    {
        if (error == "UNAUTHORIZED") return 401;
        if (error == "FORBIDDEN") return 403;
        if (error == "QUESTION_NOT_FOUND" || error == "SOLUTION_NOT_FOUND" ||
            error == "COMMENT_NOT_FOUND" || error == "NOT_FOUND" || error == "PARENT_NOT_FOUND") return 404;
        if (error == "NAME_TAKEN" || error == "EMAIL_ALREADY_EXISTS") return 409;
        if (error == "TOO_MANY_REQUESTS" || error == "EMAIL_DAILY_LIMIT" ||
            error == "IP_DAILY_LIMIT" || error == "ATTEMPTS_EXCEEDED") return 429;
        if (error == "DB_ERROR" || error == "REDIS_ERROR" || error == "MAIL_SEND_FAILED" ||
            error == "SAVE_FAILED" || error == "CREATE_FAILED" || error == "UPDATE_FAILED" ||
            error == "DELETE_FAILED" || error == "CREATE_SOLUTION_FAILED") return 500;
        return 400;
    }

    template <typename Response>
    void Fail(Response* response, int code, const std::string& message)
    {
        ProtoAdapter::SetError(response->mutable_status(), code, message, code == 429 || code == 503);
    }

    template <typename Response>
    void NotImplemented(Response* response)
    {
        Fail(response, kNotImplemented, "NOT_IMPLEMENTED");
    }
    //判断题目ID是否合法
    bool ValidQuestionId(const std::string& id)
    {
        return !id.empty() && std::all_of(id.begin(), id.end(), [](unsigned char ch) {
            return std::isdigit(ch) != 0;
        });
    }
    //判断验证码是否合法
    bool ValidCode(const std::string& code)
    {
        return oj::util::StringUtil::IsValidAuthCode(oj::util::StringUtil::Trim(code));
    }
    //通过session获取用户信息
    bool RequireUser(brpc::Controller* controller, const std::shared_ptr<oj::control::Control>& control,
                    User* user, oj::common::StatusResponse* status)
    {
        if (controller != nullptr && control != nullptr &&
            SessionAdapter::GetSessionUser(*controller, *control, user))
            return true;
        ProtoAdapter::SetError(status, 401, "UNAUTHORIZED");
        return false;
    }
    //填充历史统计信息Response
    void FillStatistics(const Json::Value& result, oj::common::UserStatistics* output)
    {
        const Json::Value& stats = result.isMember("stats") ? result["stats"] : result;
        output->set_total_submissions(stats.get("total_submits", 0).asUInt64());
        output->set_accepted_submissions(stats.get("passed_submits", 0).asUInt64());
        output->set_accepted_questions(stats.get("passed_questions", 0).asUInt64());
    }
    //填充题解操作Response
    void FillSolutionActions(const Json::Value& result, oj::biz::SolutionActionResponse* response)
    {
        response->mutable_actions()->set_liked(result.get("liked", false).asBool());
        response->mutable_actions()->set_favorited(result.get("favorited", false).asBool());
        response->set_like_count(result.get("like_count", 0).asUInt64());
        response->set_favorite_count(result.get("favorite_count", 0).asUInt64());
    }
    //填充评论操作Response
    void FillCommentAction(uint64_t id, const Json::Value& result, oj::common::CommentActionState* output)
    {
        output->set_comment_id(id);
        output->set_liked(result.get("liked", result.get("like", false)).asBool());
        output->set_favorited(result.get("favorited", result.get("favorite", false)).asBool());
    }

    template <typename Request, typename Response>
    void Dispatch501(oj::runtime::BusinessExecutor& executor,
                    google::protobuf::RpcController* controller, const Request* request,
                    Response* response, google::protobuf::Closure* done)
    {
        AsyncDispatch(executor, controller, request, response, done,
                    [](const Request&, Response* output) { NotImplemented(output); });
    }

    } // namespace

    OJServiceImpl::OJServiceImpl(std::shared_ptr<oj::control::Control> control,
                                oj::runtime::BusinessExecutor& executor)
        : _control(std::move(control)), _executor(executor)
    {
    }

    //发送邮箱验证码
    void OJServiceImpl::SendRegistrationCode(google::protobuf::RpcController* controller,
                                            const oj::biz::SendVerificationCodeRequest* request,
                                            oj::biz::SendVerificationCodeResponse* response,
                                            google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                const std::string email = oj::util::StringUtil::Trim(input->email());
                if (!oj::util::StringUtil::IsValidEmail(email)) return Fail(output, 400, "INVALID_EMAIL");
                std::string error;
                int retry = 0;
                const bool ok = control->Auth().SendEmailAuthCode(
                    email, cntl == nullptr ? std::string{} : SessionAdapter::RemoteIp(*cntl), &error, &retry);
                output->set_retry_after_seconds(std::max(0, retry));
                ok ? ProtoAdapter::SetOk(output->mutable_status()) : Fail(output, ErrorCode(error), error);
            });
    }
    //用户注册
    void OJServiceImpl::Register(google::protobuf::RpcController* controller,
                                const oj::biz::RegisterRequest* request,
                                oj::biz::AuthResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                const std::string email = oj::util::StringUtil::Trim(input->email());
                const std::string name = oj::util::StringUtil::Trim(input->name());
                const std::string code = oj::util::StringUtil::Trim(input->verification_code());
                if (!oj::util::StringUtil::IsValidEmail(email)) return Fail(output, 400, "INVALID_EMAIL");
                if (!ValidCode(code)) return Fail(output, 400, "INVALID_VERIFICATION_CODE");
                if (name.empty() || name.size() > 64) return Fail(output, 400, "INVALID_NAME");
                User user{};
                bool created = false;
                std::string error;
                if (!control->Auth().VerifyEmailAuthCodeAndLogin(
                        email, code, name, input->password(), &user, &created, &error))
                    return Fail(output, ErrorCode(error), error);
                if (cntl == nullptr || SessionAdapter::CreateSession(*cntl, *control, user.uid, user.email).empty())
                    return Fail(output, 500, "SESSION_CREATE_FAILED");
                ProtoAdapter::ToProto(user, output->mutable_user());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //邮箱验证码登陆
    void OJServiceImpl::LoginWithVerificationCode(google::protobuf::RpcController* controller,
                                                const oj::biz::VerificationCodeLoginRequest* request,
                                                oj::biz::AuthResponse* response,
                                                google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                const std::string email = oj::util::StringUtil::Trim(input->email());
                const std::string code = oj::util::StringUtil::Trim(input->verification_code());
                if (!oj::util::StringUtil::IsValidEmail(email)) return Fail(output, 400, "INVALID_EMAIL");
                if (!ValidCode(code)) return Fail(output, 400, "INVALID_VERIFICATION_CODE");
                User user{};
                bool created = false;
                std::string error;
                if (!control->Auth().VerifyEmailAuthCodeAndLogin(email, code, {}, {}, &user, &created, &error))
                    return Fail(output, ErrorCode(error), error);
                if (cntl == nullptr || SessionAdapter::CreateSession(*cntl, *control, user.uid, user.email).empty())
                    return Fail(output, 500, "SESSION_CREATE_FAILED");
                ProtoAdapter::ToProto(user, output->mutable_user());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //账户密码登陆
    void OJServiceImpl::LoginWithPassword(google::protobuf::RpcController* controller,
                                        const oj::biz::PasswordLoginRequest* request,
                                        oj::biz::AuthResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                const std::string login = oj::util::StringUtil::Trim(input->email_or_name());
                if (login.empty()) return Fail(output, 400, "LOGIN_ID_REQUIRED");
                if (input->password().empty()) return Fail(output, 400, "PASSWORD_REQUIRED");
                User user{};
                std::string error;
                if (!control->Auth().LoginWithPassword(login, input->password(), &user, &error))
                    return Fail(output, ErrorCode(error), error);
                if (cntl == nullptr || SessionAdapter::CreateSession(*cntl, *control, user.uid, user.email).empty())
                    return Fail(output, 500, "SESSION_CREATE_FAILED");
                ProtoAdapter::ToProto(user, output->mutable_user());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //账户登出
    void OJServiceImpl::Logout(google::protobuf::RpcController* controller,
                            const oj::common::EmptyRequest* request,
                            oj::common::EmptyResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto*, auto* output) {
                if (cntl != nullptr) SessionAdapter::DestroySession(*cntl, *control);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //设置密码
    void OJServiceImpl::SetPassword(google::protobuf::RpcController* controller,
                                    const oj::biz::SetPasswordRequest* request,
                                    oj::common::EmptyResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->password().empty()) return Fail(output, 400, "PASSWORD_REQUIRED");
                std::string error;
                control->Auth().SetPasswordForUser(user.email, input->password(), &error)
                    ? ProtoAdapter::SetOk(output->mutable_status()) : Fail(output, ErrorCode(error), error);
            });
    }
    //验证用户信息并发送邮箱验证码
    void OJServiceImpl::SendSecurityCode(google::protobuf::RpcController* controller,
                                        const oj::common::EmptyRequest* request,
                                        oj::biz::SendVerificationCodeResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto*, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                std::string error;
                int retry = 0;
                const bool ok = control->Auth().SendEmailAuthCode(
                    user.email, cntl == nullptr ? std::string{} : SessionAdapter::RemoteIp(*cntl), &error, &retry);
                output->set_retry_after_seconds(std::max(0, retry));
                ok ? ProtoAdapter::SetOk(output->mutable_status()) : Fail(output, ErrorCode(error), error);
            });
    }
    //修改密码
    void OJServiceImpl::ChangePassword(google::protobuf::RpcController* controller,
                                    const oj::biz::ChangePasswordRequest* request,
                                    oj::common::EmptyResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (!ValidCode(input->verification_code())) return Fail(output, 400, "INVALID_VERIFICATION_CODE");
                if (input->new_password().empty()) return Fail(output, 400, "PASSWORD_REQUIRED");
                std::string error;
                control->Auth().ChangePasswordWithCode(user.email, oj::util::StringUtil::Trim(input->verification_code()),
                                                        input->new_password(), &error)
                    ? ProtoAdapter::SetOk(output->mutable_status()) : Fail(output, ErrorCode(error), error);
            });
    }
    //修改邮箱
    void OJServiceImpl::ChangeEmail(google::protobuf::RpcController* controller,
                                    const oj::biz::ChangeEmailRequest* request,
                                    oj::common::EmptyResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                const std::string email = oj::util::StringUtil::Trim(input->new_email());
                const std::string code = oj::util::StringUtil::Trim(input->verification_code());
                if (!oj::util::StringUtil::IsValidEmail(email)) return Fail(output, 400, "INVALID_EMAIL");
                if (!ValidCode(code)) return Fail(output, 400, "INVALID_VERIFICATION_CODE");
                User updated{};
                std::string error;
                if (!control->Auth().ChangeEmailWithCode(user, email, code, &updated, &error))
                    return Fail(output, ErrorCode(error), error);
                if (cntl != nullptr) {
                    SessionAdapter::DestroySession(*cntl, *control);
                    if (SessionAdapter::CreateSession(*cntl, *control, updated.uid, updated.email).empty())
                        return Fail(output, 500, "SESSION_CREATE_FAILED");
                }
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //注销账户
    void OJServiceImpl::DeleteAccount(google::protobuf::RpcController* controller,
                                    const oj::biz::DeleteAccountRequest* request,
                                    oj::common::EmptyResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                const std::string code = oj::util::StringUtil::Trim(input->verification_code());
                if (!ValidCode(code)) return Fail(output, 400, "INVALID_VERIFICATION_CODE");
                std::string error;
                if (!control->Auth().DeleteAccountWithCode(user.email, code, &error))
                    return Fail(output, ErrorCode(error), error);
                if (cntl != nullptr) SessionAdapter::DestroySession(*cntl, *control);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取一批用户
    void OJServiceImpl::GetCurrentUser(google::protobuf::RpcController* controller,
                                    const oj::common::EmptyRequest* request,
                                    oj::biz::GetUserProfileResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto*, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                ProtoAdapter::ToProto(user, output->mutable_user());
                output->mutable_user()->mutable_avatar()->set_url(control->GetEffectiveAvatarUrl(user));
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取用户账户信息
    void OJServiceImpl::GetUserProfile(google::protobuf::RpcController* controller,
                                    const oj::biz::GetUserProfileRequest* request,
                                    oj::biz::GetUserProfileResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->uid() != 0 && input->uid() != static_cast<uint32_t>(user.uid))
                    return Fail(output, 403, "FORBIDDEN");
                ProtoAdapter::ToProto(user, output->mutable_user());
                output->mutable_user()->mutable_avatar()->set_url(control->GetEffectiveAvatarUrl(user));
                Json::Value stats;
                std::string error;
                if (control->User().GetUserStats(user, &stats, &error)) FillStatistics(stats, output->mutable_statistics());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //更新用户账户信息
    void OJServiceImpl::UpdateProfile(google::protobuf::RpcController* controller,
                                    const oj::biz::UpdateProfileRequest* request,
                                    oj::biz::UpdateProfileResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                const std::string name = oj::util::StringUtil::Trim(input->name());
                if (name.empty() || name.size() > 20) return Fail(output, 400, "INVALID_NAME");
                User existing{};
                if (control->User().GetUserByName(name, &existing) && existing.uid != user.uid)
                    return Fail(output, 409, "NAME_TAKEN");
                if (!control->User().UpdateUserName(user.uid, name)) return Fail(output, 500, "DB_ERROR");
                user.name = name;
                ProtoAdapter::ToProto(user, output->mutable_user());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //更新用户头像
    void OJServiceImpl::UpdateAvatar(google::protobuf::RpcController* controller,
                                    const oj::biz::UpdateAvatarRequest* request,
                                    oj::biz::UpdateAvatarResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                Json::Value result;
                std::string error;
                if (!control->User().UploadAvatar(user, input->content(), input->content_type(), &result, &error))
                    return Fail(output, ErrorCode(error), error);
                output->mutable_avatar()->set_url(result.get("avatar_url", "").asString());
                output->mutable_avatar()->set_content_type(input->content_type());
                output->mutable_avatar()->set_size_bytes(input->content().size());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //删除用户头像
    void OJServiceImpl::DeleteAvatar(google::protobuf::RpcController* controller,
                                    const oj::common::EmptyRequest* request,
                                    oj::common::EmptyResponse* response, google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto*, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                Json::Value result;
                std::string error;
                control->User().DeleteAvatar(user, &result, &error)
                    ? ProtoAdapter::SetOk(output->mutable_status()) : Fail(output, 500, error);
            });
    }
    //获取用户历史统计数据
    void OJServiceImpl::GetStatistics(google::protobuf::RpcController* controller,
                                    const oj::common::EmptyRequest* request,
                                    oj::biz::GetUserStatisticsResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto*, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                Json::Value result;
                std::string error;
                if (!control->User().GetUserStats(user, &result, &error)) return Fail(output, 500, error);
                FillStatistics(result, output->mutable_statistics());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取题目列表
    void OJServiceImpl::ListQuestions(google::protobuf::RpcController* controller,
                                    const oj::biz::GetQuestionListRequest* request,
                                    oj::biz::GetQuestionListResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](const auto& input, auto* output) {
                const int page = input.page().page() == 0 ? 1 :
                    static_cast<int>(std::min(input.page().page(), 1000000u));
                const int size = input.page().page_size() == 0 ? 20 : static_cast<int>(std::min(input.page().page_size(), 100u));
                auto query = std::make_shared<QuestionQuery>();
                query->title = oj::util::StringUtil::Trim(input.search());
                query->star = oj::util::StringUtil::Trim(input.difficulty());
                std::shared_ptr<QueryStruct> raw = query;
                const std::string version = control->Question().GetQuestionsListVersion();
                auto key = control->Question().BuildListCacheKey(
                    raw, page, size, version, oj::cache::Cache::CacheKey::PageType::kData);
                std::vector<Question> questions;
                int total = 0;
                int pages = 0;
                if (!control->Question().GetQuestionsByPage(key, questions, &total, &pages))
                    return Fail(output, 500, "DB_ERROR");
                for (const auto& question : questions) ProtoAdapter::ToProto(question, output->add_questions());
                ProtoAdapter::SetPage(input.page(), total, output->mutable_page(), 20, 100);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取题目详情
    void OJServiceImpl::GetQuestion(google::protobuf::RpcController* controller,
                                    const oj::biz::QuestionIdRequest* request,
                                    oj::biz::GetQuestionDetailResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (!ValidQuestionId(input->question_id())) return Fail(output, 400, "INVALID_QUESTION_ID");
                Question question;
                if (!control->Question().GetOneQuestion(input->question_id(), question))
                    return Fail(output, 404, "QUESTION_NOT_FOUND");
                ProtoAdapter::ToProto(question, output->mutable_question());
                Json::Value samples;
                std::string error;
                if (!control->Question().GetSampleTests(input->question_id(), &samples, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto& item : samples["tests"]) ProtoAdapter::ToProto(item, output->add_sample_tests());
                User user{};
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user))
                    output->set_passed_by_current_user(control->Question().HasUserPassedQuestion(user.uid, input->question_id()));
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取题目通过状态
    void OJServiceImpl::GetPassStatus(google::protobuf::RpcController* controller,
                                    const oj::biz::QuestionIdRequest* request,
                                    oj::biz::GetQuestionPassStatusResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (!ValidQuestionId(input->question_id())) return Fail(output, 400, "INVALID_QUESTION_ID");
                User user{};
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user))
                    output->set_passed(control->Question().HasUserPassedQuestion(user.uid, input->question_id()));
                else
                    output->set_passed(false);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    #define OJ_501(method, request_type, response_type) \
    void OJServiceImpl::method(google::protobuf::RpcController* controller, const request_type* request, \
                            response_type* response, google::protobuf::Closure* done) \
    { Dispatch501(_executor, controller, request, response, done); }

    OJ_501(CreateQuestion, oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse)
    OJ_501(UpdateQuestion, oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse)
    OJ_501(DeleteQuestion, oj::biz::QuestionIdRequest, oj::common::EmptyResponse)
    //测试用例
    void OJServiceImpl::ListTestCases(google::protobuf::RpcController* controller,
                                    const oj::biz::ListTestCasesRequest* request,
                                    oj::biz::ListTestCasesResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](const auto& input, auto* output) {
                if (!ValidQuestionId(input.question_id())) return Fail(output, 400, "INVALID_QUESTION_ID");
                if (!input.sample_only()) return Fail(output, 501, "NOT_IMPLEMENTED");
                Json::Value result;
                std::string error;
                if (!control->Question().GetSampleTests(input.question_id(), &result, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto& item : result["tests"]) ProtoAdapter::ToProto(item, output->add_test_cases());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    OJ_501(CreateTestCase, oj::biz::CreateTestCaseRequest, oj::biz::TestCaseResponse)
    OJ_501(UpdateTestCase, oj::biz::UpdateTestCaseRequest, oj::biz::TestCaseResponse)
    OJ_501(DeleteTestCase, oj::biz::TestCaseIdRequest, oj::common::EmptyResponse)
    OJ_501(InvalidateCache, oj::biz::InvalidateQuestionCacheRequest, oj::common::EmptyResponse)
    //获取题解列表
    void OJServiceImpl::ListSolutions(google::protobuf::RpcController* controller,
                                    const oj::biz::ListSolutionsRequest* request,
                                    oj::biz::ListSolutionsResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](const auto& input, auto* output) {
                if (!ValidQuestionId(input.question_id())) return Fail(output, 400, "INVALID_QUESTION_ID");
                if (input.author_uid() != 0) return Fail(output, 501, "NOT_IMPLEMENTED");
                const int page = input.page().page() == 0 ? 1 :
                    static_cast<int>(std::min(input.page().page(), 100u));
                const int size = input.page().page_size() == 0 ? 10 : static_cast<int>(std::min(input.page().page_size(), 50u));
                Json::Value result;
                std::string error;
                if (!control->Solution().GetSolutionList(input.question_id(), "approved", input.sort_by(),
                                                        page, size, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto& item : result["solutions"]) ProtoAdapter::ToProto(item, output->add_solutions());
                ProtoAdapter::SetPage(input.page(), result.get("total", 0).asUInt64(), output->mutable_page(), 10, 50);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取题解详情
    void OJServiceImpl::GetSolution(google::protobuf::RpcController* controller,
                                    const oj::biz::SolutionIdRequest* request,
                                    oj::biz::GetSolutionResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (input->solution_id() == 0) return Fail(output, 400, "INVALID_ID");
                Json::Value result;
                std::string error;
                if (!control->Solution().GetSolutionDetail(input->solution_id(), &result, &error))
                    return Fail(output, ErrorCode(error), error);
                ProtoAdapter::ToProto(result, output->mutable_solution());
                output->mutable_author()->set_uid(output->solution().user_id());
                output->mutable_author()->set_name(result.get("author_name", "").asString());
                User user{};
                Json::Value actions;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user) &&
                    control->Solution().GetUserSolutionActions(input->solution_id(), user.uid, &actions)) {
                    output->mutable_current_user_actions()->set_liked(actions.get("liked", false).asBool());
                    output->mutable_current_user_actions()->set_favorited(actions.get("favorited", false).asBool());
                }
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //创建题解
    void OJServiceImpl::CreateSolution(google::protobuf::RpcController* controller,
                                    const oj::biz::CreateSolutionRequest* request,
                                    oj::biz::SolutionResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (!ValidQuestionId(input->question_id())) return Fail(output, 400, "INVALID_QUESTION_ID");
                if (input->title().empty()) return Fail(output, 400, "TITLE_REQUIRED");
                if (input->content_md().empty()) return Fail(output, 400, "CONTENT_REQUIRED");
                unsigned long long id = 0;
                std::string error;
                if (!control->Solution().PublishSolution(input->question_id(), user, input->title(),
                                                        input->content_md(), &id, &error))
                    return Fail(output, ErrorCode(error), error);
                auto* solution = output->mutable_solution();
                solution->set_id(id);
                solution->set_question_id(input->question_id());
                solution->set_user_id(user.uid);
                solution->set_title(oj::util::StringUtil::Trim(input->title()));
                solution->set_content_md(oj::util::StringUtil::Trim(input->content_md()));
                solution->set_moderation_status("approved");
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    OJ_501(UpdateSolution, oj::biz::UpdateSolutionRequest, oj::biz::SolutionResponse)
    OJ_501(DeleteSolution, oj::biz::SolutionIdRequest, oj::common::EmptyResponse)
    //题解点赞
    void OJServiceImpl::ToggleSolutionLike(google::protobuf::RpcController* controller,
                                        const oj::biz::SolutionIdRequest* request,
                                        oj::biz::SolutionActionResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->solution_id() == 0) return Fail(output, 400, "INVALID_ID");
                Json::Value result; std::string error;
                if (!control->Solution().ToggleLike(input->solution_id(), user, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                FillSolutionActions(result, output); ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //题解收藏
    void OJServiceImpl::ToggleSolutionFavorite(google::protobuf::RpcController* controller,
                                            const oj::biz::SolutionIdRequest* request,
                                            oj::biz::SolutionActionResponse* response,
                                            google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->solution_id() == 0) return Fail(output, 400, "INVALID_ID");
                Json::Value result; std::string error;
                if (!control->Solution().ToggleFavorite(input->solution_id(), user, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                FillSolutionActions(result, output); ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取操作状态
    void OJServiceImpl::GetActionState(google::protobuf::RpcController* controller,
                                    const oj::biz::SolutionIdRequest* request,
                                    oj::biz::SolutionActionResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (input->solution_id() == 0) return Fail(output, 400, "INVALID_ID");
                User user{}; Json::Value result;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user) &&
                    !control->Solution().GetUserSolutionActions(input->solution_id(), user.uid, &result))
                    return Fail(output, 500, "DB_ERROR");
                FillSolutionActions(result, output); ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取评论列表
    void OJServiceImpl::ListComments(google::protobuf::RpcController* controller,
                                    const oj::biz::ListCommentsRequest* request,
                                    oj::biz::ListCommentsResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (input->solution_id() == 0) return Fail(output, 400, "INVALID_ID");
                const int page = input->page().page() == 0 ? 1 :
                    static_cast<int>(std::min(input->page().page(), 1000000u));
                const int size = input->page().page_size() == 0 ? 20 : static_cast<int>(std::min(input->page().page_size(), 1000u));
                Json::Value result; std::string error;
                if (!control->Comment().GetTopLevelComments(input->solution_id(), page, size, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto& item : result["comments"]) ProtoAdapter::ToProto(item, output->add_comments());
                ProtoAdapter::SetPage(input->page(), result.get("total", 0).asUInt64(), output->mutable_page(), 20, 1000);
                User user{}; Json::Value actions;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user)) {
                    std::vector<unsigned long long> ids;
                    for (const auto& item : result["comments"]) ids.push_back(item.get("id", 0).asUInt64());
                    if (control->Comment().GetCommentActions(ids, user.uid, &actions)) {
                        for (const auto id : ids) FillCommentAction(id, actions["actions"][std::to_string(id)], output->add_current_user_actions());
                    }
                }
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取回复列表
    void OJServiceImpl::ListReplies(google::protobuf::RpcController* controller,
                                    const oj::biz::ListRepliesRequest* request,
                                    oj::biz::ListCommentsResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (input->parent_comment_id() == 0) return Fail(output, 400, "INVALID_ID");
                const int page = input->page().page() == 0 ? 1 :
                    static_cast<int>(std::min(input->page().page(), 1000000u));
                const int size = input->page().page_size() == 0 ? 20 : static_cast<int>(std::min(input->page().page_size(), 1000u));
                Json::Value result; std::string error;
                if (!control->Comment().GetCommentReplies(input->parent_comment_id(), page, size, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                std::vector<unsigned long long> ids;
                for (const auto& item : result["comments"]) {
                    ProtoAdapter::ToProto(item, output->add_comments()); ids.push_back(item.get("id", 0).asUInt64());
                }
                ProtoAdapter::SetPage(input->page(), result.get("total", 0).asUInt64(), output->mutable_page(), 20, 1000);
                User user{}; Json::Value actions;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user) &&
                    control->Comment().GetCommentActions(ids, user.uid, &actions))
                    for (const auto id : ids) FillCommentAction(id, actions["actions"][std::to_string(id)], output->add_current_user_actions());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //发表评论
    void OJServiceImpl::CreateComment(google::protobuf::RpcController* controller,
                                    const oj::biz::CreateCommentRequest* request,
                                    oj::biz::CommentResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->solution_id() == 0) return Fail(output, 400, "INVALID_ID");
                if (input->content().empty()) return Fail(output, 400, "CONTENT_REQUIRED");
                Json::Value result; std::string error;
                if (!control->Comment().PostComment(input->solution_id(), user, input->content(), &result,
                                                    &error, input->parent_comment_id()))
                    return Fail(output, ErrorCode(error), error);
                ProtoAdapter::ToProto(result, output->mutable_comment());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //更新评论
    void OJServiceImpl::UpdateComment(google::protobuf::RpcController* controller,
                                    const oj::biz::UpdateCommentRequest* request,
                                    oj::biz::CommentResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->comment_id() == 0) return Fail(output, 400, "INVALID_ID");
                if (input->content().empty()) return Fail(output, 400, "CONTENT_REQUIRED");
                Json::Value result; std::string error;
                if (!control->Comment().EditComment(input->comment_id(), user, input->content(), &result, &error))
                    return Fail(output, ErrorCode(error), error);
                ProtoAdapter::ToProto(result, output->mutable_comment());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //删除评论
    void OJServiceImpl::DeleteComment(google::protobuf::RpcController* controller,
                                    const oj::biz::CommentIdRequest* request,
                                    oj::common::EmptyResponse* response,
                                    google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->comment_id() == 0) return Fail(output, 400, "INVALID_ID");
                Json::Value result; std::string error;
                control->Comment().DeleteComment(input->comment_id(), user, &result, &error)
                    ? ProtoAdapter::SetOk(output->mutable_status()) : Fail(output, ErrorCode(error), error);
            });
    }
    //评论点赞
    void OJServiceImpl::ToggleCommentLike(google::protobuf::RpcController* controller,
                                        const oj::biz::CommentIdRequest* request,
                                        oj::biz::CommentActionResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->comment_id() == 0) return Fail(output, 400, "INVALID_ID");
                Json::Value result; std::string error;
                if (!control->Comment().ToggleCommentLike(input->comment_id(), user, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                FillCommentAction(input->comment_id(), result, output->mutable_actions());
                output->set_like_count(result.get("like_count", 0).asUInt64());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //评论收藏
    void OJServiceImpl::ToggleCommentFavorite(google::protobuf::RpcController* controller,
                                            const oj::biz::CommentIdRequest* request,
                                            oj::biz::CommentActionResponse* response,
                                            google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->comment_id() == 0) return Fail(output, 400, "INVALID_ID");
                Json::Value result; std::string error;
                if (!control->Comment().ToggleCommentFavorite(input->comment_id(), user, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                FillCommentAction(input->comment_id(), result, output->mutable_actions());
                output->set_favorite_count(result.get("favorite_count", 0).asUInt64());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    //获取
    void OJServiceImpl::GetActionStates(google::protobuf::RpcController* controller,
                                        const oj::biz::GetCommentActionsRequest* request,
                                        oj::biz::GetCommentActionsResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                std::vector<unsigned long long> ids(input->comment_ids().begin(), input->comment_ids().end());
                User user{}; Json::Value result;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user) &&
                    !control->Comment().GetCommentActions(ids, user.uid, &result))
                    return Fail(output, 500, "DB_ERROR");
                for (const auto id : ids) FillCommentAction(id, result["actions"][std::to_string(id)], output->add_actions());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    OJ_501(CreateSubmission, oj::biz::CreateSubmissionRequest, oj::biz::CreateSubmissionResponse)
    OJ_501(CreateCustomTest, oj::biz::CreateCustomTestRequest, oj::biz::CreateCustomTestResponse)
    OJ_501(GetSubmission, oj::biz::GetSubmissionRequest, oj::biz::GetSubmissionResponse)
    OJ_501(GetCustomTest, oj::biz::GetCustomTestRequest, oj::biz::GetCustomTestResponse)
    OJ_501(ListSubmissions, oj::biz::ListSubmissionsRequest, oj::biz::ListSubmissionsResponse)
    OJ_501(UpdateJudgeResult, oj::biz::UpdateJudgeResultRequest, oj::biz::UpdateJudgeResultResponse)
    
    void OJServiceImpl::LegacyUpdateJudgeResult(google::protobuf::RpcController* controller,
                                                const oj_judge::JudgeFinishedRequest* request,
                                                oj_judge::NullRsp* response,
                                                google::protobuf::Closure* done)
    {
        AsyncDispatch(_executor, controller, request, response, done,
            [](google::protobuf::RpcController* cntl, const auto*, auto*) {
                if (cntl != nullptr) cntl->SetFailed("NOT_IMPLEMENTED");
            });
    }

    void OJServiceImpl::InvalidateStaticCache(google::protobuf::RpcController* controller,
                                            const oj::biz::InvalidateStaticCacheRequest* request,
                                            oj::common::EmptyResponse* response,
                                            google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                const std::string path = oj::util::StringUtil::Trim(input->path());
                if (!oj::util::StringUtil::IsSafeStaticPageName(path)) return Fail(output, 400, "INVALID_PATH");
                if (!control->InvalidateStaticHtmlCache(path)) return Fail(output, 500, "INVALIDATE_FAILED");
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    #undef OJ_501

} // namespace oj::rpc
