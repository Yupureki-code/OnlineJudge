#pragma once

#include <array>
#include <string_view>

namespace oj::legacy_contract {

inline constexpr std::string_view kProtobufContentType = "application/x-protobuf";
inline constexpr std::string_view kUserCookie = "oj_session_id";
inline constexpr std::string_view kAdminCookie = "oj_admin_session_id";

enum class Auth {
    kPublic,
    kOptionalUser,
    kUser,
    kOptionalAdmin,
    kAdmin,
    kSuperAdmin,
};

struct RpcContract {
    std::string_view domain;
    std::string_view legacy_verb;
    std::string_view legacy_path;
    std::string_view service;
    std::string_view method;
    std::string_view request;
    std::string_view response;
    Auth auth;
    std::string_view legacy_errors;
};

// This is the migration inventory: old HTTP behavior on the left, native brpc
// method and Phase 3.2 message names on the right. Identity always comes from
// the session for authenticated methods, never from a request user_id field.
inline constexpr std::array<RpcContract, 36> kUserRpcs{{
    {"user", "POST", "/api/auth/send_code", "OJService", "SendRegistrationCode", "SendVerificationCodeRequest", "SendVerificationCodeResponse", Auth::kPublic, "INVALID_ARGUMENT,RATE_LIMITED,UNAVAILABLE"},
    {"user", "POST", "/api/auth/verify_code", "OJService", "Register", "RegisterRequest", "AuthResponse", Auth::kPublic, "INVALID_ARGUMENT,CONFLICT,INTERNAL"},
    {"user", "POST", "/api/user/password/login", "OJService", "LoginWithPassword", "PasswordLoginRequest", "AuthResponse", Auth::kPublic, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"user", "POST", "/api/user/logout", "OJService", "Logout", "EmptyRequest", "EmptyResponse", Auth::kOptionalUser, ""},
    {"user", "GET", "/api/user/info", "OJService", "GetCurrentUser", "EmptyRequest", "GetUserProfileResponse", Auth::kOptionalUser, "UNAUTHENTICATED"},
    {"user", "POST", "/api/user/password/set", "OJService", "SetPassword", "SetPasswordRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"user", "POST", "/api/user/security/send_code", "OJService", "SendSecurityCode", "EmptyRequest", "SendVerificationCodeResponse", Auth::kUser, "UNAUTHENTICATED,RATE_LIMITED,UNAVAILABLE"},
    {"user", "POST", "/api/user/email/change", "OJService", "ChangeEmail", "ChangeEmailRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,CONFLICT"},
    {"user", "POST", "/api/user/password/change", "OJService", "ChangePassword", "ChangePasswordRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"user", "POST", "/api/user/account/delete", "OJService", "DeleteAccount", "DeleteAccountRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,INTERNAL"},
    {"user", "POST", "/api/user/name", "OJService", "UpdateProfile", "UpdateProfileRequest", "UpdateProfileResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,CONFLICT,INTERNAL"},
    {"user", "POST", "/api/user/avatar", "OJService", "UpdateAvatar", "UpdateAvatarRequest", "UpdateAvatarResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,RATE_LIMITED,INTERNAL"},
    {"user", "DELETE", "/api/user/avatar", "OJService", "DeleteAvatar", "EmptyRequest", "EmptyResponse", Auth::kUser, "UNAUTHENTICATED,INTERNAL"},
    {"user", "GET", "/api/user/stats", "OJService", "GetStatistics", "EmptyRequest", "GetUserStatisticsResponse", Auth::kUser, "UNAUTHENTICATED,INTERNAL"},
    {"question", "GET", "/all_questions?page={page}&size={size}", "OJService", "ListQuestions", "GetQuestionListRequest", "GetQuestionListResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,INTERNAL"},
    {"question", "GET", "/api/question/{question_id}", "OJService", "GetQuestion", "QuestionIdRequest", "GetQuestionDetailResponse", Auth::kPublic, "INVALID_ARGUMENT,NOT_FOUND"},
    {"question", "GET", "/api/question/{question_id}/sample_tests", "OJService", "ListTestCases", "ListTestCasesRequest", "ListTestCasesResponse", Auth::kPublic, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"question", "GET", "/api/question/{question_id}/pass_status", "OJService", "GetPassStatus", "QuestionIdRequest", "GetQuestionPassStatusResponse", Auth::kOptionalUser, "INVALID_ARGUMENT"},
    {"solution", "POST", "/api/questions/{question_id}/solutions", "OJService", "CreateSolution", "CreateSolutionRequest", "SolutionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND"},
    {"solution", "GET", "/api/questions/{question_id}/solutions", "OJService", "ListSolutions", "ListSolutionsRequest", "ListSolutionsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"solution", "GET", "/api/solutions/{solution_id}", "OJService", "GetSolution", "SolutionIdRequest", "GetSolutionResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND"},
    {"solution", "POST", "/api/solutions/{solution_id}/like", "OJService", "ToggleSolutionLike", "SolutionIdRequest", "SolutionActionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,INTERNAL"},
    {"solution", "POST", "/api/solutions/{solution_id}/favorite", "OJService", "ToggleSolutionFavorite", "SolutionIdRequest", "SolutionActionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,INTERNAL"},
    {"solution", "GET", "/api/solutions/{solution_id}/actions", "OJService", "GetActionState", "SolutionIdRequest", "SolutionActionResponse", Auth::kOptionalUser, "INVALID_ARGUMENT"},
    {"comment", "GET", "/api/solutions/{solution_id}/comments", "OJService", "ListComments", "ListCommentsRequest", "ListCommentsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"comment", "POST", "/api/solutions/{solution_id}/comments", "OJService", "CreateComment", "CreateCommentRequest", "CommentResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND"},
    {"comment", "PUT", "/api/comments/{comment_id}", "OJService", "UpdateComment", "UpdateCommentRequest", "CommentResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,PERMISSION_DENIED"},
    {"comment", "DELETE", "/api/comments/{comment_id}", "OJService", "DeleteComment", "CommentIdRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,PERMISSION_DENIED"},
    {"comment", "GET", "/api/comments/{comment_id}/replies", "OJService", "ListReplies", "ListRepliesRequest", "ListCommentsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"comment", "POST", "/api/comments/{comment_id}/like", "OJService", "ToggleCommentLike", "CommentIdRequest", "CommentActionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,INTERNAL"},
    {"comment", "GET", "/api/comments/actions?ids={comment_ids}", "OJService", "GetActionStates", "GetCommentActionsRequest", "GetCommentActionsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,INTERNAL"},
    {"submission", "POST", "/judge/{question_id}", "OJService", "CreateSubmission", "CreateSubmissionRequest", "CreateSubmissionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,UNAVAILABLE"},
    {"submission", "POST", "/api/question/{question_id}/test", "OJService", "CreateCustomTest", "CreateCustomTestRequest", "CreateCustomTestResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,UNAVAILABLE"},
    {"submission", "GET", "/api/question/{question_id}/submits", "OJService", "ListSubmissions", "ListSubmissionsRequest", "ListSubmissionsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,INTERNAL"},
    {"submission", "GET", "/judge_result.html?submission_id={submission_id}", "OJService", "GetSubmission", "GetSubmissionRequest", "GetSubmissionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND"},
    {"static", "POST", "/api/static/cache/invalidate", "OJService", "InvalidateStaticCache", "InvalidateStaticCacheRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,PERMISSION_DENIED"},
}};

struct RemovedLegacyRoute {
    std::string_view verb;
    std::string_view path;
    std::string_view reason;
};

// These pre-authentication endpoints cannot be represented safely in the new
// contract and are intentionally removed at the final atomic cutover.
inline constexpr std::array<RemovedLegacyRoute, 4> kRemovedLegacyUserRoutes{{
    {"POST", "/api/user/check", "public account enumeration"},
    {"POST", "/api/user/create", "account creation without verification"},
    {"POST", "/api/user/get", "public user lookup by email"},
    {"POST", "/api/user/login", "login without proof of account possession"},
}};

inline constexpr std::array<RpcContract, 22> kAdminRpcs{{
    {"admin-auth", "GET", "/api/admin/version", "OJAdminService", "AdminGetVersion", "EmptyRequest", "GetVersionResponse", Auth::kPublic, ""},
    {"admin-auth", "POST", "/api/admin/auth/login", "OJAdminService", "AdminLogin", "AdminLoginRequest", "AdminLoginResponse", Auth::kPublic, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"admin-auth", "POST", "/api/admin/auth/logout", "OJAdminService", "AdminLogout", "EmptyRequest", "EmptyResponse", Auth::kOptionalAdmin, ""},
    {"admin", "GET", "/api/admin/overview", "OJAdminService", "AdminGetOverview", "EmptyRequest", "AdminOverviewResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin", "GET", "/api/admin/cache/metrics", "OJAdminService", "AdminGetCacheMetrics", "EmptyRequest", "GetCacheMetricsResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-user", "GET", "/api/admin/users", "OJAdminService", "AdminListUsers", "AdminListUsersRequest", "AdminListUsersResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-question", "GET", "/api/admin/questions", "OJAdminService", "AdminListQuestions", "GetQuestionListRequest", "GetQuestionListResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-question", "GET", "/api/admin/questions/{question_id}", "OJAdminService", "AdminGetQuestion", "QuestionIdRequest", "GetQuestionDetailResponse", Auth::kAdmin, "NOT_FOUND"},
    {"admin-question", "POST", "/api/admin/questions", "OJAdminService", "AdminCreateQuestion", "SaveQuestionRequest", "QuestionResponse", Auth::kAdmin, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"admin-question", "PUT", "/api/admin/questions/{question_id}", "OJAdminService", "AdminUpdateQuestion", "SaveQuestionRequest", "QuestionResponse", Auth::kAdmin, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"admin-question", "DELETE", "/api/admin/questions/{question_id}", "OJAdminService", "AdminDeleteQuestion", "QuestionIdRequest", "EmptyResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-test", "GET", "/api/admin/questions/{question_id}/tests", "OJAdminService", "AdminListTestCases", "ListTestCasesRequest", "ListTestCasesResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-test", "POST", "/api/admin/questions/{question_id}/tests", "OJAdminService", "AdminCreateTestCase", "CreateTestCaseRequest", "TestCaseResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-test", "PUT", "/api/admin/tests/{test_id}", "OJAdminService", "AdminUpdateTestCase", "UpdateTestCaseRequest", "TestCaseResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-test", "DELETE", "/api/admin/tests/{test_id}", "OJAdminService", "AdminDeleteTestCase", "TestCaseIdRequest", "EmptyResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-question", "POST", "/api/admin/questions/cache/invalidate", "OJAdminService", "AdminInvalidateQuestionCache", "InvalidateQuestionCacheRequest", "EmptyResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-account", "GET", "/api/admin/accounts", "OJAdminService", "AdminListAdminAccounts", "ListAdminAccountsRequest", "ListAdminAccountsResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-account", "GET", "/api/admin/accounts/{admin_id}", "OJAdminService", "AdminGetAdminAccount", "AdminAccountIdRequest", "AdminAccountResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED"},
    {"admin-account", "POST", "/api/admin/accounts", "OJAdminService", "AdminCreateAdminAccount", "SaveAdminAccountRequest", "AdminAccountResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,CONFLICT,PERMISSION_DENIED,INTERNAL"},
    {"admin-account", "PUT", "/api/admin/accounts/{admin_id}", "OJAdminService", "AdminUpdateAdminAccount", "SaveAdminAccountRequest", "AdminAccountResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,CONFLICT,PERMISSION_DENIED,INTERNAL"},
    {"admin-account", "DELETE", "/api/admin/accounts/{admin_id}", "OJAdminService", "AdminDeleteAdminAccount", "AdminAccountIdRequest", "EmptyResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,CONFLICT,PERMISSION_DENIED,INTERNAL"},
    {"admin-audit", "GET", "/api/admin/audit/logs", "OJAdminService", "AdminGetAuditLogs", "GetAuditLogsRequest", "GetAuditLogsResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
}};

struct PageContract {
    std::string_view path;
    int status;
    std::string_view content_type;
    Auth auth;
    std::string_view behavior;
};

inline constexpr std::array<PageContract, 15> kUserPages{{
    {"/", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "index shell; inject non-sensitive session user"},
    {"/all_questions", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "question-list shell; preserve query URL"},
    {"/about", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "about page"},
    {"/user/profile", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "profile shell"},
    {"/user/settings", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "settings shell"},
    {"/questions/{question_id}", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "question shell"},
    {"/questions/{question_id}/solutions/new", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "solution editor shell"},
    {"/solutions/{solution_id}", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "solution detail shell"},
    {"/questions/{question_id}/solutions/{solution_id}", 302, "", Auth::kPublic, "Location: /questions/{question_id}"},
    {"/judge_result.html", 200, "text/html;charset=utf-8", Auth::kOptionalUser, "submission polling shell"},
    {"/js/*", 200, "application/javascript;charset=utf-8", Auth::kPublic, "no-store"},
    {"/css/*", 200, "text/css", Auth::kPublic, "static file; HEAD supported"},
    {"/pictures/*", 200, "image/*", Auth::kPublic, "static file; HEAD supported"},
    {"/*", 200, "extension MIME", Auth::kPublic, "normalized static root; HEAD supported"},
    {"<missing-or-traversal>", 404, "", Auth::kPublic, "never escape static root"},
}};

inline constexpr std::array<PageContract, 10> kAdminPages{{
    {"/healthz", 200, "text/plain;charset=utf-8", Auth::kPublic, "body: ok"},
    {"/", 302, "", Auth::kPublic, "Location: /admin/login"},
    {"/admin/login", 200, "text/html;charset=utf-8", Auth::kPublic, "no-store"},
    {"/admin", 200, "text/html;charset=utf-8", Auth::kAdmin, "admin shell; unauthenticated is 404"},
    {"/admin/users", 200, "text/html;charset=utf-8", Auth::kAdmin, "admin shell; unauthenticated is 404"},
    {"/admin/questions", 200, "text/html;charset=utf-8", Auth::kAdmin, "admin shell; unauthenticated is 404"},
    {"/admin/questions/new", 200, "text/html;charset=utf-8", Auth::kAdmin, "admin shell; unauthenticated is 404"},
    {"/admin/questions/{question_id}", 200, "text/html;charset=utf-8", Auth::kAdmin, "admin shell; unauthenticated is 404"},
    {"/admin/assets/*", 200, "extension MIME", Auth::kPublic, "normalized static file; HEAD supported"},
    {"/pictures/*", 200, "image/*", Auth::kPublic, "normalized static file; HEAD supported"},
}};

struct PagingContract {
    std::string_view method;
    int default_page;
    int default_size;
    int max_page;
    int max_size;
};

inline constexpr std::array<PagingContract, 4> kUserPaging{{
    {"OJService.ListQuestions", 1, 5, 1000000, 100},
    {"OJService.ListSolutions", 1, 10, 100, 50},
    {"OJService.ListComments", 1, 20, 1000000, 1000},
    {"OJService.ListReplies", 1, 20, 1000000, 1000},
}};

inline constexpr std::array<PagingContract, 4> kAdminPaging{{
    {"OJAdminService.AdminListUsers", 1, 20, 1000000, 200},
    {"OJAdminService.AdminListQuestions", 1, 20, 1000000, 200},
    {"OJAdminService.AdminListAdminAccounts", 1, 20, 1000000, 200},
    {"OJAdminService.AdminGetAuditLogs", 1, 20, 1000000, 200},
}};

struct ErrorContract {
    std::string_view code;
    int http_status;
};

inline constexpr std::array<ErrorContract, 9> kErrors{{
    {"OK", 200},
    {"INVALID_ARGUMENT", 400},
    {"UNAUTHENTICATED", 401},
    {"PERMISSION_DENIED", 403},
    {"NOT_FOUND", 404},
    {"CONFLICT", 409},
    {"RATE_LIMITED", 429},
    {"INTERNAL", 500},
    {"UNAVAILABLE", 503},
}};

}  // namespace oj::legacy_contract
