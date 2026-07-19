#pragma once

#include <atomic>
#include <string>

#include "biz_service.pb.h"

namespace brpc { class Controller; }
namespace oj::admin { class AdminSessionStore; }
namespace oj::control { class Control; }
namespace oj::runtime { class BusinessExecutor; }
namespace oj::rpc { class AdminAuditWorker; }
namespace oj::db { struct AdminAccount; }

namespace oj::rpc
{

class OJAdminServiceImpl final : public oj::biz::OJAdminService
{
public:
    OJAdminServiceImpl(oj::control::Control& control,
                       oj::admin::AdminSessionStore& sessions,
                       oj::runtime::BusinessExecutor& executor,
                       AdminAuditWorker* audit_worker = nullptr);

    static bool IsAdminRole(const std::string& role);
    static bool IsSuperAdminRole(const std::string& role);

    void AdminGetVersion(google::protobuf::RpcController*, const oj::common::EmptyRequest*,
                         oj::biz::GetVersionResponse*, google::protobuf::Closure*) override;
    void AdminLogin(google::protobuf::RpcController*, const oj::biz::AdminLoginRequest*,
                    oj::biz::AdminLoginResponse*, google::protobuf::Closure*) override;
    void AdminLogout(google::protobuf::RpcController*, const oj::common::EmptyRequest*,
                     oj::common::EmptyResponse*, google::protobuf::Closure*) override;
    void AdminGetOverview(google::protobuf::RpcController*, const oj::common::EmptyRequest*,
                          oj::biz::AdminOverviewResponse*, google::protobuf::Closure*) override;
    void AdminListUsers(google::protobuf::RpcController*, const oj::biz::AdminListUsersRequest*,
                        oj::biz::AdminListUsersResponse*, google::protobuf::Closure*) override;
    void AdminGetUser(google::protobuf::RpcController*, const oj::biz::AdminUserIdRequest*,
                      oj::biz::AdminUserResponse*, google::protobuf::Closure*) override;
    void AdminCreateUser(google::protobuf::RpcController*, const oj::biz::AdminCreateUserRequest*,
                         oj::biz::AdminUserResponse*, google::protobuf::Closure*) override;
    void AdminUpdateUser(google::protobuf::RpcController*, const oj::biz::AdminUpdateUserRequest*,
                         oj::biz::AdminUserResponse*, google::protobuf::Closure*) override;
    void AdminDeleteUser(google::protobuf::RpcController*, const oj::biz::AdminUserIdRequest*,
                         oj::common::EmptyResponse*, google::protobuf::Closure*) override;
    void AdminListQuestions(google::protobuf::RpcController*, const oj::biz::GetQuestionListRequest*,
                            oj::biz::GetQuestionListResponse*, google::protobuf::Closure*) override;
    void AdminGetQuestion(google::protobuf::RpcController*, const oj::biz::QuestionIdRequest*,
                          oj::biz::GetQuestionDetailResponse*, google::protobuf::Closure*) override;
    void AdminCreateQuestion(google::protobuf::RpcController*, const oj::biz::SaveQuestionRequest*,
                             oj::biz::QuestionResponse*, google::protobuf::Closure*) override;
    void AdminUpdateQuestion(google::protobuf::RpcController*, const oj::biz::SaveQuestionRequest*,
                             oj::biz::QuestionResponse*, google::protobuf::Closure*) override;
    void AdminDeleteQuestion(google::protobuf::RpcController*, const oj::biz::QuestionIdRequest*,
                             oj::common::EmptyResponse*, google::protobuf::Closure*) override;
    void AdminListTestCases(google::protobuf::RpcController*, const oj::biz::ListTestCasesRequest*,
                            oj::biz::ListTestCasesResponse*, google::protobuf::Closure*) override;
    void AdminCreateTestCase(google::protobuf::RpcController*, const oj::biz::CreateTestCaseRequest*,
                             oj::biz::TestCaseResponse*, google::protobuf::Closure*) override;
    void AdminUpdateTestCase(google::protobuf::RpcController*, const oj::biz::UpdateTestCaseRequest*,
                             oj::biz::TestCaseResponse*, google::protobuf::Closure*) override;
    void AdminDeleteTestCase(google::protobuf::RpcController*, const oj::biz::TestCaseIdRequest*,
                             oj::common::EmptyResponse*, google::protobuf::Closure*) override;
    void AdminInvalidateQuestionCache(google::protobuf::RpcController*,
                                      const oj::biz::InvalidateQuestionCacheRequest*,
                                      oj::common::EmptyResponse*, google::protobuf::Closure*) override;
    void AdminListAdminAccounts(google::protobuf::RpcController*, const oj::biz::ListAdminAccountsRequest*,
                                oj::biz::ListAdminAccountsResponse*, google::protobuf::Closure*) override;
    void AdminGetAdminAccount(google::protobuf::RpcController*, const oj::biz::AdminAccountIdRequest*,
                              oj::biz::AdminAccountResponse*, google::protobuf::Closure*) override;
    void AdminCreateAdminAccount(google::protobuf::RpcController*, const oj::biz::SaveAdminAccountRequest*,
                                 oj::biz::AdminAccountResponse*, google::protobuf::Closure*) override;
    void AdminUpdateAdminAccount(google::protobuf::RpcController*, const oj::biz::SaveAdminAccountRequest*,
                                 oj::biz::AdminAccountResponse*, google::protobuf::Closure*) override;
    void AdminDeleteAdminAccount(google::protobuf::RpcController*, const oj::biz::AdminAccountIdRequest*,
                                 oj::common::EmptyResponse*, google::protobuf::Closure*) override;
    void AdminGetCacheMetrics(google::protobuf::RpcController*, const oj::common::EmptyRequest*,
                              oj::biz::GetCacheMetricsResponse*, google::protobuf::Closure*) override;
    void AdminGetAuditLogs(google::protobuf::RpcController*, const oj::biz::GetAuditLogsRequest*,
                           oj::biz::GetAuditLogsResponse*, google::protobuf::Closure*) override;
    void AdminListDiagnosticFiles(google::protobuf::RpcController*, const oj::biz::DiagnosticFilesRequest*,
                                  oj::biz::DiagnosticFilesResponse*, google::protobuf::Closure*) override;
    void AdminReadDiagnosticFile(google::protobuf::RpcController*, const oj::biz::DiagnosticContentRequest*,
                                 oj::biz::DiagnosticContentResponse*, google::protobuf::Closure*) override;

private:
    bool RequireAdmin(brpc::Controller*, bool super_only, oj::db::AdminAccount*,
                      oj::common::StatusResponse*) const;
    void WriteAudit(brpc::Controller*, const oj::db::AdminAccount&, const char* action,
                    const char* resource_type, const std::string& resource_id);

    oj::control::Control& _control;
    oj::admin::AdminSessionStore& sessions_;
    oj::runtime::BusinessExecutor& _executor;
    AdminAuditWorker* audit_worker_;
    std::atomic<unsigned long long> audit_sequence_{0};
};

} // namespace oj::rpc
