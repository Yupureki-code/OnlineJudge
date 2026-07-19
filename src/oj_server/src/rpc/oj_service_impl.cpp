#include "../../include/rpc/oj_service_impl.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <future>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <brpc/controller.h>
#include <jsoncpp/json/json.h>
#include <openssl/rand.h>
#include <google/protobuf/util/json_util.h>

#include "../../include/control/oj_control.hpp"
#include "../../include/rpc/async_dispatch.hpp"
#include "../../include/rpc/proto_adapter.hpp"
#include "../../include/rpc/session_adapter.hpp"
#include "../../include/judge/outbox_publisher.hpp"
#include "../../../comm/models/user.hxx"
#include "../../../comm/models/question.hxx"
#include "../../../comm/proto/mq_message.pb.h"
#include "../../../comm/judge_callback_auth.hpp"
#include "comm.hpp"
#include "logger.hpp"

namespace oj::rpc
{
    using namespace oj::db;
    namespace
    {

    constexpr int kNotImplemented = 501;

    int ErrorCode(const std::string& error)
    {
        if (error == "UNAUTHORIZED" || error == "INVALID_CREDENTIALS") return 401;
        if (error == "FORBIDDEN") return 403;
        if (error == "QUESTION_NOT_FOUND" || error == "SOLUTION_NOT_FOUND" ||
            error == "COMMENT_NOT_FOUND" || error == "NOT_FOUND" || error == "PARENT_NOT_FOUND") return 404;
        if (error == "NAME_TAKEN" || error == "EMAIL_ALREADY_EXISTS") return 409;
        if (error == "TOO_MANY_REQUESTS" || error == "EMAIL_DAILY_LIMIT" ||
            error == "IP_DAILY_LIMIT" || error == "ATTEMPTS_EXCEEDED") return 429;
        if (error == "REDIS_ERROR") return 503;
        if (error == "DB_ERROR" || error == "MAIL_SEND_FAILED" || error == "CSPRNG_FAILED" ||
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
        return !id.empty() && id.size() <= 5 &&
               std::all_of(id.begin(), id.end(), [](unsigned char ch) {
                   return std::isalnum(ch) != 0;
               });
    }
    //判断验证码是否合法
    bool ValidCode(const std::string& code)
    {
        return oj::util::StringUtil::IsValidAuthCode(oj::util::StringUtil::Trim(code));
    }

    std::string RandomHex(std::size_t bytes)
    {
        std::string random(bytes, '\0');
        if (bytes == 0 || RAND_bytes(reinterpret_cast<unsigned char*>(random.data()), bytes) != 1)
            return {};
        static constexpr char hex[] = "0123456789abcdef";
        std::string output(bytes * 2, '0');
        for (std::size_t index = 0; index < bytes; ++index) {
            const unsigned char value = static_cast<unsigned char>(random[index]);
            output[index * 2] = hex[value >> 4];
            output[index * 2 + 1] = hex[value & 0x0f];
        }
        return output;
    }

    uint64_t RandomGuestSubmissionId()
    {
        uint64_t value = 0;
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) return 0;
        return value | (uint64_t{1} << 63);
    }

    bool PublishTransientTask(oj::judge::ConfirmedMessagePublisher* publisher,
                              const oj::mq::JudgeTaskMessage& task)
    {
        if (publisher == nullptr) return false;
        std::string payload;
        if (!task.SerializeToString(&payload)) return false;
        auto completed = std::make_shared<std::promise<bool>>();
        auto future = completed->get_future();
        if (!publisher->Publish(task.message_id(), std::move(payload),
                [completed](bool success, const std::string&) {
                    try { completed->set_value(success); } catch (...) {}
                })) return false;
        return future.wait_for(std::chrono::seconds(5)) == std::future_status::ready && future.get();
    }

    bool SupportedLanguage(const std::string& language)
    {
        static const std::set<std::string> supported{"cpp", "cpp17", "cpp20", "c++"};
        return supported.contains(language);
    }

    bool VerifyCallbackHmac(const oj::biz::UpdateJudgeResultRequest& request)
    {
        const char* configured_id = std::getenv("OJ_JUDGE_CALLBACK_KEY_ID");
        const char* configured_secret = std::getenv("OJ_JUDGE_CALLBACK_SECRET");
        if (configured_id == nullptr || configured_secret == nullptr || *configured_secret == '\0' ||
            request.service_auth().key_id() != configured_id ||
            request.service_auth().hmac_sha256().size() != 32) return false;
        return oj::security::VerifyJudgeCallbackHmac(request, configured_secret);
    }

    bool ValidateTaskPayload(const oj::db::JudgeOutbox& outbox,
                             const oj::biz::UpdateJudgeResultRequest& request,
                             oj::mq::JudgeTaskMessage* task)
    {
        const std::string payload(outbox.payload.begin(), outbox.payload.end());
        if (task == nullptr || !task->ParseFromString(payload) || task->message_id() != request.message_id())
            return false;
        return (request.has_submission_id() && task->has_submission_id() &&
                task->submission_id() == request.submission_id()) ||
               (request.has_custom_task_id() && task->has_custom_task_id() &&
                task->custom_task_id() == request.custom_task_id());
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
    void FillSolutionActions(const oj::control::ActionState& result, oj::biz::SolutionActionResponse* response)
    {
        response->mutable_actions()->set_liked(result.liked);
        response->mutable_actions()->set_favorited(result.favorited);
        response->set_like_count(result.like_count);
        response->set_favorite_count(result.favorite_count);
    }
    //填充评论操作Response
    void FillCommentAction(uint64_t id, const oj::control::ActionState& result,
                           oj::common::CommentActionState* output)
    {
        output->set_comment_id(id);
        output->set_liked(result.liked);
        output->set_favorited(result.favorited);
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
                                 oj::runtime::BusinessExecutor& executor,
                                 oj::judge::OutboxPublisher* outbox_publisher,
                                 oj::judge::ConfirmedMessagePublisher* transient_publisher)
        : _control(std::move(control)), _executor(executor), _outbox_publisher(outbox_publisher),
          _transient_publisher(transient_publisher)
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
                if (!control->Auth().LoginWithPassword(
                        login, input->password(), cntl == nullptr ? std::string{} : SessionAdapter::RemoteIp(*cntl),
                        &user, &error))
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
                if (cntl != nullptr && !SessionAdapter::DestroySession(*cntl, *control))
                    return Fail(output, 503, "SESSION_REVOCATION_FAILED");
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
                                        const oj::biz::SendSecurityCodeRequest* request,
                                        oj::biz::SendVerificationCodeResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                std::string error;
                int retry = 0;
                const std::string client_ip = cntl == nullptr ? std::string{} : SessionAdapter::RemoteIp(*cntl);
                bool ok = false;
                if (input->purpose().empty() || input->purpose() == "current_account") {
                    ok = control->Auth().SendEmailAuthCode(user.email, client_ip, &error, &retry);
                } else if (input->purpose() == "change_email") {
                    const std::string new_email = oj::util::StringUtil::Trim(input->new_email());
                    if (!oj::util::StringUtil::IsValidEmail(new_email)) return Fail(output, 400, "INVALID_EMAIL");
                    ok = control->Auth().SendEmailChangeCode(user, new_email, client_ip, &error, &retry);
                } else {
                    return Fail(output, 400, "INVALID_SECURITY_CODE_PURPOSE");
                }
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
                    if (!SessionAdapter::DestroySession(*cntl, *control))
                        return Fail(output, 503, "SESSION_REVOCATION_FAILED");
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
                if (cntl != nullptr && !SessionAdapter::DestroySession(*cntl, *control))
                    return Fail(output, 503, "SESSION_REVOCATION_FAILED");
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
                if (cntl == nullptr || !SessionAdapter::GetSessionUser(*cntl, *control, &user)) {
                    ProtoAdapter::SetOk(output->mutable_status());
                    return;
                }
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
                std::vector<Solution> solutions;
                int total = 0;
                std::string error;
                if (!control->Solution().GetSolutionList(input.question_id(), "approved", input.sort_by(),
                                                        page, size, &solutions, &total, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto& item : solutions) ProtoAdapter::ToProto(item, output->add_solutions());
                ProtoAdapter::SetPage(input.page(), total, output->mutable_page(), 10, 50);
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
                Solution result{};
                User author{};
                std::string error;
                if (!control->Solution().GetSolutionDetail(input->solution_id(), &result, &author, &error))
                    return Fail(output, ErrorCode(error), error);
                ProtoAdapter::ToProto(result, output->mutable_solution());
                output->mutable_author()->set_uid(result.user_id);
                output->mutable_author()->set_name(author.name);
                User user{};
                oj::control::ActionState actions;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user) &&
                    control->Solution().GetUserSolutionActions(input->solution_id(), user.uid, &actions, &error)) {
                    output->mutable_current_user_actions()->set_liked(actions.liked);
                    output->mutable_current_user_actions()->set_favorited(actions.favorited);
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
                oj::control::ActionState result; std::string error;
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
                oj::control::ActionState result; std::string error;
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
                User user{}; oj::control::ActionState result; std::string error;
                if (cntl != nullptr) SessionAdapter::GetSessionUser(*cntl, *control, &user);
                if (!control->Solution().GetUserSolutionActions(
                        input->solution_id(), user.uid, &result, &error))
                    return Fail(output, ErrorCode(error), error);
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
                const int size = input->page().page_size() == 0 ? 20 : static_cast<int>(std::min(input->page().page_size(), 100u));
                std::vector<Comment> comments;
                int total = 0;
                std::string error;
                if (!control->Comment().GetTopLevelComments(
                        input->solution_id(), page, size, &comments, &total, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto& item : comments) ProtoAdapter::ToProto(item, output->add_comments());
                ProtoAdapter::SetPage(input->page(), total, output->mutable_page(), 20, 100);
                User user{};
                std::map<unsigned long long, oj::control::ActionState> actions;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user)) {
                    std::vector<unsigned long long> ids;
                    for (const auto& item : comments) ids.push_back(item.id);
                    if (control->Comment().GetCommentActions(ids, user.uid, &actions, &error)) {
                        for (const auto id : ids)
                            FillCommentAction(id, actions[id], output->add_current_user_actions());
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
                const int size = input->page().page_size() == 0 ? 20 : static_cast<int>(std::min(input->page().page_size(), 100u));
                std::vector<Comment> comments;
                int total = 0;
                std::string error;
                if (!control->Comment().GetCommentReplies(
                        input->parent_comment_id(), page, size, &comments, &total, &error))
                    return Fail(output, ErrorCode(error), error);
                std::vector<unsigned long long> ids;
                for (const auto& item : comments) {
                    ProtoAdapter::ToProto(item, output->add_comments()); ids.push_back(item.id);
                }
                ProtoAdapter::SetPage(input->page(), total, output->mutable_page(), 20, 100);
                User user{};
                std::map<unsigned long long, oj::control::ActionState> actions;
                if (cntl != nullptr && SessionAdapter::GetSessionUser(*cntl, *control, &user) &&
                    control->Comment().GetCommentActions(ids, user.uid, &actions, &error))
                    for (const auto id : ids)
                        FillCommentAction(id, actions[id], output->add_current_user_actions());
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
                Comment result{}; std::string error;
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
                Comment result{}; std::string error;
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
                std::string error;
                control->Comment().DeleteComment(input->comment_id(), user, &error)
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
                oj::control::ActionState result; std::string error;
                if (!control->Comment().ToggleCommentLike(input->comment_id(), user, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                FillCommentAction(input->comment_id(), result, output->mutable_actions());
                output->set_like_count(result.like_count);
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
                oj::control::ActionState result; std::string error;
                if (!control->Comment().ToggleCommentFavorite(input->comment_id(), user, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                FillCommentAction(input->comment_id(), result, output->mutable_actions());
                output->set_favorite_count(result.favorite_count);
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
                User user{};
                std::map<unsigned long long, oj::control::ActionState> result;
                std::string error;
                if (cntl != nullptr) SessionAdapter::GetSessionUser(*cntl, *control, &user);
                if (!control->Comment().GetCommentActions(ids, user.uid, &result, &error))
                    return Fail(output, ErrorCode(error), error);
                for (const auto id : ids) FillCommentAction(id, result[id], output->add_actions());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    void OJServiceImpl::CreateSubmission(google::protobuf::RpcController* controller,
                                         const oj::biz::CreateSubmissionRequest* request,
                                         oj::biz::CreateSubmissionResponse* response,
                                         google::protobuf::Closure* done)
    {
        auto control = _control;
        auto* publisher = _outbox_publisher;
        auto* transient_publisher = _transient_publisher;
        AsyncDispatch(_executor, controller, request, response, done,
            [control, publisher, transient_publisher](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                const bool authenticated = cntl != nullptr &&
                    SessionAdapter::GetSessionUser(*cntl, *control, &user);
                const std::string question_id = oj::util::StringUtil::Trim(input->question_id());
                const std::string language = oj::util::StringUtil::Trim(input->language());
                if (!ValidQuestionId(question_id)) return Fail(output, 400, "INVALID_QUESTION_ID");
                if (input->code().empty() || input->code().size() > 64 * 1024)
                    return Fail(output, 400, "INVALID_CODE");
                if (!SupportedLanguage(language)) return Fail(output, 400, "UNSUPPORTED_LANGUAGE");
                const int64_t window = std::time(nullptr) / 2;
                const std::string rate_identity = authenticated
                    ? "user:" + std::to_string(user.uid)
                    : "guest:" + (cntl == nullptr ? std::string("unknown") : SessionAdapter::RemoteIp(*cntl));
                const int reserved = control->GetModel()->ReserveCachedStringByAnyKey(
                    "judge:submit-rate:" + rate_identity + ":" + std::to_string(window), "1", 3);
                if (reserved == 0) return Fail(output, 429, "SUBMISSION_RATE_LIMITED");
                if (reserved < 0) return Fail(output, 503, "REDIS_ERROR");

                Question question{};
                if (!control->Question().GetOneQuestion(question_id, question))
                    return Fail(output, 404, "QUESTION_NOT_FOUND");
                if (!question.visible) return Fail(output, 404, "QUESTION_NOT_FOUND");
                Json::Value tests;
                if (!control->GetModel()->Question().GetTestsByQuestionId(question_id, &tests))
                    return Fail(output, 500, "LOAD_TEST_CASES_FAILED");
                oj::mq::JudgeTaskMessage task;
                task.set_message_id(RandomHex(16));
                task.set_user_id(authenticated ? user.uid : 0);
                task.set_question_id(question_id);
                task.set_code(input->code());
                task.set_language(language);
                task.set_time_limit_ms(std::max(1, static_cast<int>(question.cpu_limit)) * 1000);
                task.set_memory_limit_mb(static_cast<uint32_t>(std::max(1, question.memory_limit)));
                task.set_created_at(std::time(nullptr));
                for (const auto& item : tests["tests"]) {
                    auto* test = task.add_test_cases();
                    test->set_test_case_id(item.get("test_case_id", item.get("id", 0)).asUInt64());
                    test->set_index(item.get("id", 0).asUInt());
                    test->set_input(item.get("in", "").asString());
                    test->set_expected_output(item.get("out", "").asString());
                }
                if (task.message_id().empty() || task.test_cases().empty())
                    return Fail(output, 500, "BUILD_JUDGE_TASK_FAILED");
                if (!authenticated) {
                    const uint64_t submission_id = RandomGuestSubmissionId();
                    if (submission_id == 0) return Fail(output, 500, "CSPRNG_FAILED");
                    task.set_custom_task_id("guest-" + std::to_string(submission_id));
                    Json::Value state;
                    state["task_id"] = task.custom_task_id();
                    state["message_id"] = task.message_id();
                    state["user_id"] = 0;
                    state["submission_id"] = Json::UInt64(submission_id);
                    state["question_id"] = question_id;
                    state["language"] = language;
                    state["status"] = "QUEUED";
                    state["transient"] = true;
                    state["created_at"] = Json::Int64(task.created_at());
                    state["expires_at"] = Json::Int64(task.created_at() + 600);
                    Json::StreamWriterBuilder writer;
                    writer["indentation"] = "";
                    const std::string cache_key = "judge:custom:" + task.custom_task_id();
                    if (!control->GetModel()->SetCachedStringByAnyKey(
                            cache_key, Json::writeString(writer, state), 600))
                        return Fail(output, 503, "REDIS_ERROR");
                    if (!PublishTransientTask(transient_publisher, task)) {
                        control->GetModel()->DeleteCachedStringByAnyKey(cache_key);
                        return Fail(output, 503, "PUBLISH_JUDGE_TASK_FAILED");
                    }
                    auto* created = output->mutable_submission();
                    created->set_submission_id(submission_id);
                    created->set_question_id(question_id);
                    created->set_language(language);
                    created->set_status(oj::common::SUBMISSION_STATUS_QUEUED);
                    created->set_created_at(task.created_at());
                    ProtoAdapter::SetOk(output->mutable_status());
                    return;
                }
                oj::db::Submission created{};
                if (!control->GetModel()->Submission().CreateSubmissionWithOutbox(
                        user.uid, question_id, input->code(), language, task, &created))
                    return Fail(output, 500, "CREATE_SUBMISSION_FAILED");
                ProtoAdapter::ToProto(created, output->mutable_submission());
                ProtoAdapter::SetOk(output->mutable_status());
                if (publisher) publisher->Notify();
            });
    }

    void OJServiceImpl::CreateCustomTest(google::protobuf::RpcController* controller,
                                         const oj::biz::CreateCustomTestRequest* request,
                                         oj::biz::CreateCustomTestResponse* response,
                                         google::protobuf::Closure* done)
    {
        auto control = _control;
        auto* publisher = _outbox_publisher;
        AsyncDispatch(_executor, controller, request, response, done,
            [control, publisher](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                const std::string question_id = oj::util::StringUtil::Trim(input->question_id());
                const std::string language = oj::util::StringUtil::Trim(input->language());
                if (!ValidQuestionId(question_id)) return Fail(output, 400, "INVALID_QUESTION_ID");
                if (input->code().empty() || input->code().size() > 64 * 1024)
                    return Fail(output, 400, "INVALID_CODE");
                if (input->custom_input().size() > 64 * 1024)
                    return Fail(output, 400, "CUSTOM_INPUT_TOO_LARGE");
                if (!SupportedLanguage(language)) return Fail(output, 400, "UNSUPPORTED_LANGUAGE");
                const int64_t window = std::time(nullptr) / 2;
                const int reserved = control->GetModel()->ReserveCachedStringByAnyKey(
                    "judge:submit-rate:" + std::to_string(user.uid) + ":" + std::to_string(window), "1", 3);
                if (reserved == 0) return Fail(output, 429, "SUBMISSION_RATE_LIMITED");
                if (reserved < 0) return Fail(output, 503, "REDIS_ERROR");
                Question question{};
                if (!control->Question().GetOneQuestion(question_id, question))
                    return Fail(output, 404, "QUESTION_NOT_FOUND");
                if (!question.visible) return Fail(output, 404, "QUESTION_NOT_FOUND");
                oj::mq::JudgeTaskMessage task;
                task.set_message_id(RandomHex(16));
                task.set_custom_task_id(RandomHex(24));
                task.set_user_id(user.uid);
                task.set_question_id(question_id);
                task.set_code(input->code());
                task.set_language(language);
                task.set_time_limit_ms(std::max(1, static_cast<int>(question.cpu_limit)) * 1000);
                task.set_memory_limit_mb(static_cast<uint32_t>(std::max(1, question.memory_limit)));
                task.set_custom_input(input->custom_input());
                task.set_created_at(std::time(nullptr));
                auto* test = task.add_test_cases();
                test->set_index(1);
                test->set_input(input->custom_input());
                if (task.message_id().empty() || task.custom_task_id().empty() ||
                    !control->GetModel()->Submission().CreateCustomOutbox(task))
                    return Fail(output, 500, "CREATE_CUSTOM_TASK_FAILED");
                Json::Value state;
                state["task_id"] = task.custom_task_id();
                state["message_id"] = task.message_id();
                state["user_id"] = user.uid;
                state["status"] = "QUEUED";
                state["expires_at"] = Json::Int64(task.created_at() + 600);
                Json::StreamWriterBuilder writer;
                writer["indentation"] = "";
                output->set_task_id(task.custom_task_id());
                output->set_submission_status(oj::common::SUBMISSION_STATUS_QUEUED);
                if (!control->GetModel()->SetCachedStringByAnyKey(
                        "judge:custom:" + task.custom_task_id(), Json::writeString(writer, state), 600))
                    LOG_ERROR("Initial custom task cache write failed task_id={}", task.custom_task_id());
                ProtoAdapter::SetOk(output->mutable_status());
                if (publisher) publisher->Notify();
            });
    }

    void OJServiceImpl::GetSubmission(google::protobuf::RpcController* controller,
                                      const oj::biz::GetSubmissionRequest* request,
                                      oj::biz::GetSubmissionResponse* response,
                                      google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                if (input->submission_id() == 0) return Fail(output, 400, "INVALID_SUBMISSION_ID");
                const std::string transient_task_id = "guest-" + std::to_string(input->submission_id());
                std::string encoded;
                if (control->GetModel()->GetCachedStringByAnyKey(
                        "judge:custom:" + transient_task_id, &encoded)) {
                    Json::CharReaderBuilder reader;
                    Json::Value state;
                    std::istringstream stream(encoded);
                    if (!Json::parseFromStream(reader, stream, &state, nullptr) ||
                        !state.get("transient", false).asBool() ||
                        state.get("submission_id", Json::UInt64(0)).asUInt64() != input->submission_id())
                        return Fail(output, 500, "TRANSIENT_SUBMISSION_STATE_INVALID");
                    auto* submission = output->mutable_submission();
                    submission->set_submission_id(input->submission_id());
                    submission->set_question_id(state.get("question_id", "").asString());
                    submission->set_language(state.get("language", "").asString());
                    submission->set_status(ProtoAdapter::SubmissionStatus(state.get("status", "").asString()));
                    submission->set_created_at(state.get("created_at", 0).asInt64());
                    if (state.isMember("result_json"))
                        google::protobuf::util::JsonStringToMessage(
                            state["result_json"].asString(), submission->mutable_result());
                    ProtoAdapter::SetOk(output->mutable_status());
                    return;
                }
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                oj::db::Submission submission{};
                if (!control->GetModel()->Submission().GetSubmission(input->submission_id(), &submission))
                    return Fail(output, 404, "SUBMISSION_NOT_FOUND");
                if (submission.user_id != static_cast<uint32_t>(user.uid))
                    return Fail(output, 403, "FORBIDDEN");
                ProtoAdapter::ToProto(submission, output->mutable_submission());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    void OJServiceImpl::GetCustomTest(google::protobuf::RpcController* controller,
                                      const oj::biz::GetCustomTestRequest* request,
                                      oj::biz::GetCustomTestResponse* response,
                                      google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (!RequireUser(cntl, control, &user, output->mutable_status())) return;
                if (input->task_id().empty()) return Fail(output, 400, "INVALID_TASK_ID");
                std::string encoded;
                if (!control->GetModel()->GetCachedStringByAnyKey("judge:custom:" + input->task_id(), &encoded))
                    return Fail(output, 404, "CUSTOM_TASK_NOT_FOUND");
                Json::CharReaderBuilder reader;
                Json::Value state;
                std::istringstream stream(encoded);
                if (!Json::parseFromStream(reader, stream, &state, nullptr))
                    return Fail(output, 500, "CUSTOM_TASK_STATE_INVALID");
                if (state.get("user_id", 0).asInt() != user.uid) return Fail(output, 403, "FORBIDDEN");
                output->set_task_id(input->task_id());
                output->set_submission_status(ProtoAdapter::SubmissionStatus(state.get("status", "").asString()));
                if (state.isMember("result_json"))
                    google::protobuf::util::JsonStringToMessage(state["result_json"].asString(), output->mutable_result());
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    void OJServiceImpl::ListSubmissions(google::protobuf::RpcController* controller,
                                        const oj::biz::ListSubmissionsRequest* request,
                                        oj::biz::ListSubmissionsResponse* response,
                                        google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](brpc::Controller* cntl, const auto* input, auto* output) {
                User user{};
                if (cntl == nullptr || !SessionAdapter::GetSessionUser(*cntl, *control, &user)) {
                    output->set_guest(true);
                    output->set_message("您还未登录，快去登录吧~");
                    ProtoAdapter::SetPage(input->page(), 0, output->mutable_page(), 20, 100);
                    ProtoAdapter::SetOk(output->mutable_status());
                    return;
                }
                if (!input->question_id().empty() && !ValidQuestionId(input->question_id()))
                    return Fail(output, 400, "INVALID_QUESTION_ID");
                const uint32_t page = input->page().page() == 0 ? 1 : input->page().page();
                const uint32_t size = std::clamp(input->page().page_size() == 0 ? 20u : input->page().page_size(), 1u, 100u);
                std::string status;
                if (input->submission_status() != oj::common::SUBMISSION_STATUS_UNSPECIFIED) {
                    const auto* value = oj::common::SubmissionStatus_descriptor()->FindValueByNumber(input->submission_status());
                    if (!value) return Fail(output, 400, "INVALID_SUBMISSION_STATUS");
                    status = value->name().substr(std::string("SUBMISSION_STATUS_").size());
                }
                std::vector<oj::db::Submission> submissions;
                uint64_t total = 0;
                if (!control->GetModel()->Submission().ListSubmissions(
                        user.uid, input->question_id(), status, page, size, &submissions, &total))
                    return Fail(output, 500, "DB_ERROR");
                for (const auto& submission : submissions)
                    ProtoAdapter::ToProto(submission, output->add_submissions());
                ProtoAdapter::SetPage(input->page(), total, output->mutable_page(), 20, 100);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }

    void OJServiceImpl::UpdateJudgeResult(google::protobuf::RpcController* controller,
                                          const oj::biz::UpdateJudgeResultRequest* request,
                                          oj::biz::UpdateJudgeResultResponse* response,
                                          google::protobuf::Closure* done)
    {
        auto control = _control;
        AsyncDispatch(_executor, controller, request, response, done,
            [control](const auto& input, auto* output) {
                if (input.message_id().empty() || oj::security::JudgeCallbackTarget(input).empty() ||
                    input.result_payload().empty() || input.service_auth().nonce().size() < 16)
                    return Fail(output, 400, "INVALID_CALLBACK");
                const int64_t now = std::time(nullptr);
                if (std::llabs(now - input.service_auth().timestamp()) > 300 || !VerifyCallbackHmac(input)) {
                    output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED);
                    return Fail(output, 401, "INVALID_SERVICE_AUTH");
                }
                const int nonce = control->GetModel()->ReserveCachedStringByAnyKey(
                    "judge:callback-nonce:" + input.service_auth().key_id() + ":" + input.service_auth().nonce(),
                    "1", 600);
                if (nonce == 0) {
                    output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED);
                    return Fail(output, 409, "CALLBACK_REPLAYED");
                }
                if (nonce < 0) {
                    output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE);
                    output->set_retry_after_ms(1000);
                    return Fail(output, 503, "REDIS_ERROR");
                }
                oj::db::JudgeOutbox outbox{};
                oj::mq::JudgeTaskMessage task;
                Json::Value custom_state;
                bool transient = false;
                if (input.has_custom_task_id() && input.custom_task_id().starts_with("guest-")) {
                    std::string encoded;
                    Json::CharReaderBuilder reader;
                    std::istringstream stream;
                    if (control->GetModel()->GetCachedStringByAnyKey(
                            "judge:custom:" + input.custom_task_id(), &encoded)) {
                        stream.str(encoded);
                        transient = Json::parseFromStream(reader, stream, &custom_state, nullptr) &&
                            custom_state.get("transient", false).asBool() &&
                            custom_state.get("message_id", "").asString() == input.message_id();
                    }
                    if (transient) {
                        task.set_message_id(input.message_id());
                        task.set_custom_task_id(input.custom_task_id());
                        task.set_user_id(0);
                        task.set_question_id(custom_state.get("question_id", "").asString());
                        task.set_created_at(custom_state.get("created_at", 0).asInt64());
                    }
                }
                if (!transient &&
                    (!control->GetModel()->Submission().GetOutbox(input.message_id(), &outbox) ||
                     !ValidateTaskPayload(outbox, input, &task))) {
                        output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED);
                        return Fail(output, 404, "JUDGE_TASK_NOT_FOUND");
                }
                oj::common::JudgeResult result;
                if (!result.ParseFromString(input.result_payload()) ||
                    result.status() == oj::common::SUBMISSION_STATUS_UNSPECIFIED ||
                    result.status() == oj::common::SUBMISSION_STATUS_PENDING ||
                    result.status() == oj::common::SUBMISSION_STATUS_QUEUED ||
                    result.status() == oj::common::SUBMISSION_STATUS_JUDGING) {
                    output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED);
                    return Fail(output, 400, "INVALID_JUDGE_RESULT");
                }
                const int64_t expires_at = task.created_at() * 1000000LL + 600LL * 1000000;
                if (input.has_custom_task_id()) {
                    const int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    if (expires_at <= now_us) {
                        output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED);
                        return Fail(output, 410, "CUSTOM_TASK_EXPIRED");
                    }
                }
                auto outcome = oj::model::ModelSubmission::ApplyOutcome::Persisted;
                if (!transient) {
                    outcome = control->GetModel()->Submission().ApplyJudgeResult(
                        input, result, oj::util::TimeUtil::IntToDateTime(expires_at));
                }
                if (outcome == oj::model::ModelSubmission::ApplyOutcome::Conflict ||
                    outcome == oj::model::ModelSubmission::ApplyOutcome::NotFound) {
                    if (outcome == oj::model::ModelSubmission::ApplyOutcome::Conflict)
                        LOG_CRITICAL("Conflicting judge result message_id={}", input.message_id());
                    output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_REJECTED);
                    return Fail(output, 409, "CONFLICTING_JUDGE_RESULT");
                }
                if (outcome == oj::model::ModelSubmission::ApplyOutcome::RetryableFailure) {
                    output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE);
                    output->set_retry_after_ms(1000);
                    return Fail(output, 503, "PERSIST_RESULT_FAILED");
                }
                if (outcome == oj::model::ModelSubmission::ApplyOutcome::Persisted &&
                    input.has_submission_id()) {
                    control->GetModel()->User().InvalidateSubmissionCaches(
                        static_cast<int>(task.user_id()), task.question_id());
                }
                if (input.has_custom_task_id()) {
                    const int ttl = static_cast<int>((expires_at -
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count()) / 1000000);
                    std::string result_json;
                    if (!google::protobuf::util::MessageToJsonString(result, &result_json).ok()) {
                        output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE);
                        return Fail(output, 500, "ENCODE_RESULT_FAILED");
                    }
                    Json::Value state = transient ? custom_state : Json::Value{};
                    state["task_id"] = input.custom_task_id();
                    state["message_id"] = input.message_id();
                    state["user_id"] = task.user_id();
                    const auto* value = oj::common::SubmissionStatus_descriptor()->FindValueByNumber(result.status());
                    state["status"] = value ? value->name().substr(std::string("SUBMISSION_STATUS_").size()) : "SYSTEM_ERROR";
                    state["expires_at"] = Json::Int64(expires_at / 1000000);
                    state["result_json"] = result_json;
                    Json::StreamWriterBuilder writer;
                    writer["indentation"] = "";
                    if (!control->GetModel()->SetCachedStringByAnyKey(
                            "judge:custom:" + input.custom_task_id(), Json::writeString(writer, state), ttl)) {
                        output->set_outcome(oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_RETRYABLE_FAILURE);
                        output->set_retry_after_ms(1000);
                        return Fail(output, 503, "REDIS_ERROR");
                    }
                }
                output->set_outcome(outcome == oj::model::ModelSubmission::ApplyOutcome::Idempotent
                    ? oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_IDEMPOTENT
                    : oj::biz::JUDGE_RESULT_PERSISTENCE_OUTCOME_PERSISTED);
                ProtoAdapter::SetOk(output->mutable_status());
            });
    }
    
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
