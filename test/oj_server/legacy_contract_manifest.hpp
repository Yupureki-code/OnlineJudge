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
    {"user", "POST", "/api/auth/send_code", "UserService", "SendRegistrationCode", "SendVerificationCodeRequest", "SendVerificationCodeResponse", Auth::kPublic, "INVALID_ARGUMENT,RATE_LIMITED,UNAVAILABLE"},
    {"user", "POST", "/api/auth/verify_code", "UserService", "Register", "RegisterRequest", "AuthResponse", Auth::kPublic, "INVALID_ARGUMENT,CONFLICT,INTERNAL"},
    {"user", "POST", "/api/user/password/login", "UserService", "LoginWithPassword", "PasswordLoginRequest", "AuthResponse", Auth::kPublic, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"user", "POST", "/api/user/logout", "UserService", "Logout", "EmptyRequest", "EmptyResponse", Auth::kOptionalUser, ""},
    {"user", "GET", "/api/user/info", "UserService", "GetCurrentUser", "EmptyRequest", "GetUserProfileResponse", Auth::kOptionalUser, "UNAUTHENTICATED"},
    {"user", "POST", "/api/user/password/set", "UserService", "SetPassword", "SetPasswordRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"user", "POST", "/api/user/security/send_code", "UserService", "SendSecurityCode", "EmptyRequest", "SendVerificationCodeResponse", Auth::kUser, "UNAUTHENTICATED,RATE_LIMITED,UNAVAILABLE"},
    {"user", "POST", "/api/user/email/change", "UserService", "ChangeEmail", "ChangeEmailRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,CONFLICT"},
    {"user", "POST", "/api/user/password/change", "UserService", "ChangePassword", "ChangePasswordRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"user", "POST", "/api/user/account/delete", "UserService", "DeleteAccount", "DeleteAccountRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,INTERNAL"},
    {"user", "POST", "/api/user/name", "UserService", "UpdateProfile", "UpdateProfileRequest", "UpdateProfileResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,CONFLICT,INTERNAL"},
    {"user", "POST", "/api/user/avatar", "UserService", "UpdateAvatar", "UpdateAvatarRequest", "UpdateAvatarResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,RATE_LIMITED,INTERNAL"},
    {"user", "DELETE", "/api/user/avatar", "UserService", "DeleteAvatar", "EmptyRequest", "EmptyResponse", Auth::kUser, "UNAUTHENTICATED,INTERNAL"},
    {"user", "GET", "/api/user/stats", "UserService", "GetStatistics", "EmptyRequest", "GetUserStatisticsResponse", Auth::kUser, "UNAUTHENTICATED,INTERNAL"},
    {"question", "GET", "/all_questions?page={page}&size={size}", "QuestionService", "ListQuestions", "GetQuestionListRequest", "GetQuestionListResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,INTERNAL"},
    {"question", "GET", "/api/question/{question_id}", "QuestionService", "GetQuestion", "QuestionIdRequest", "GetQuestionDetailResponse", Auth::kPublic, "INVALID_ARGUMENT,NOT_FOUND"},
    {"question", "GET", "/api/question/{question_id}/sample_tests", "QuestionService", "ListTestCases", "ListTestCasesRequest", "ListTestCasesResponse", Auth::kPublic, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"question", "GET", "/api/question/{question_id}/pass_status", "QuestionService", "GetPassStatus", "QuestionIdRequest", "GetQuestionPassStatusResponse", Auth::kOptionalUser, "INVALID_ARGUMENT"},
    {"solution", "POST", "/api/questions/{question_id}/solutions", "SolutionService", "CreateSolution", "CreateSolutionRequest", "SolutionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND"},
    {"solution", "GET", "/api/questions/{question_id}/solutions", "SolutionService", "ListSolutions", "ListSolutionsRequest", "ListSolutionsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"solution", "GET", "/api/solutions/{solution_id}", "SolutionService", "GetSolution", "SolutionIdRequest", "GetSolutionResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND"},
    {"solution", "POST", "/api/solutions/{solution_id}/like", "SolutionService", "ToggleLike", "SolutionIdRequest", "SolutionActionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,INTERNAL"},
    {"solution", "POST", "/api/solutions/{solution_id}/favorite", "SolutionService", "ToggleFavorite", "SolutionIdRequest", "SolutionActionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,INTERNAL"},
    {"solution", "GET", "/api/solutions/{solution_id}/actions", "SolutionService", "GetActionState", "SolutionIdRequest", "SolutionActionResponse", Auth::kOptionalUser, "INVALID_ARGUMENT"},
    {"comment", "GET", "/api/solutions/{solution_id}/comments", "CommentService", "ListComments", "ListCommentsRequest", "ListCommentsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"comment", "POST", "/api/solutions/{solution_id}/comments", "CommentService", "CreateComment", "CreateCommentRequest", "CommentResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND"},
    {"comment", "PUT", "/api/comments/{comment_id}", "CommentService", "UpdateComment", "UpdateCommentRequest", "CommentResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,PERMISSION_DENIED"},
    {"comment", "DELETE", "/api/comments/{comment_id}", "CommentService", "DeleteComment", "CommentIdRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,PERMISSION_DENIED"},
    {"comment", "GET", "/api/comments/{comment_id}/replies", "CommentService", "ListReplies", "ListRepliesRequest", "ListCommentsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"comment", "POST", "/api/comments/{comment_id}/like", "CommentService", "ToggleLike", "CommentIdRequest", "CommentActionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,INTERNAL"},
    {"comment", "GET", "/api/comments/actions?ids={comment_ids}", "CommentService", "GetActionStates", "GetCommentActionsRequest", "GetCommentActionsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,INTERNAL"},
    {"submission", "POST", "/judge/{question_id}", "SubmissionService", "CreateSubmission", "CreateSubmissionRequest", "CreateSubmissionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,UNAVAILABLE"},
    {"submission", "POST", "/api/question/{question_id}/test", "SubmissionService", "CreateCustomTest", "CreateCustomTestRequest", "CreateCustomTestResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND,UNAVAILABLE"},
    {"submission", "GET", "/api/question/{question_id}/submits", "SubmissionService", "ListSubmissions", "ListSubmissionsRequest", "ListSubmissionsResponse", Auth::kOptionalUser, "INVALID_ARGUMENT,INTERNAL"},
    {"submission", "GET", "/judge_result.html?submission_id={submission_id}", "SubmissionService", "GetSubmission", "GetSubmissionRequest", "GetSubmissionResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,NOT_FOUND"},
    {"static", "POST", "/api/static/cache/invalidate", "StaticFileService", "InvalidateCache", "InvalidateStaticCacheRequest", "EmptyResponse", Auth::kUser, "INVALID_ARGUMENT,UNAUTHENTICATED,PERMISSION_DENIED"},
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
    {"admin-auth", "GET", "/api/admin/version", "AdminService", "GetVersion", "EmptyRequest", "GetVersionResponse", Auth::kPublic, ""},
    {"admin-auth", "POST", "/api/admin/auth/login", "AdminService", "Login", "AdminLoginRequest", "AdminLoginResponse", Auth::kPublic, "INVALID_ARGUMENT,UNAUTHENTICATED"},
    {"admin-auth", "POST", "/api/admin/auth/logout", "AdminService", "Logout", "EmptyRequest", "EmptyResponse", Auth::kOptionalAdmin, ""},
    {"admin", "GET", "/api/admin/overview", "AdminService", "GetOverview", "EmptyRequest", "AdminOverviewResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin", "GET", "/api/admin/cache/metrics", "AdminService", "GetCacheMetrics", "EmptyRequest", "GetCacheMetricsResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-user", "GET", "/api/admin/users", "AdminService", "ListUsers", "AdminListUsersRequest", "AdminListUsersResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-question", "GET", "/api/admin/questions", "AdminService", "ListQuestions", "GetQuestionListRequest", "GetQuestionListResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-question", "GET", "/api/admin/questions/{question_id}", "AdminService", "GetQuestion", "QuestionIdRequest", "GetQuestionDetailResponse", Auth::kAdmin, "NOT_FOUND"},
    {"admin-question", "POST", "/api/admin/questions", "AdminService", "CreateQuestion", "SaveQuestionRequest", "QuestionResponse", Auth::kAdmin, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"admin-question", "PUT", "/api/admin/questions/{question_id}", "AdminService", "UpdateQuestion", "SaveQuestionRequest", "QuestionResponse", Auth::kAdmin, "INVALID_ARGUMENT,NOT_FOUND,INTERNAL"},
    {"admin-question", "DELETE", "/api/admin/questions/{question_id}", "AdminService", "DeleteQuestion", "QuestionIdRequest", "EmptyResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-test", "GET", "/api/admin/questions/{question_id}/tests", "AdminService", "ListTestCases", "ListTestCasesRequest", "ListTestCasesResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-test", "POST", "/api/admin/questions/{question_id}/tests", "AdminService", "CreateTestCase", "CreateTestCaseRequest", "TestCaseResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-test", "PUT", "/api/admin/tests/{test_id}", "AdminService", "UpdateTestCase", "UpdateTestCaseRequest", "TestCaseResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-test", "DELETE", "/api/admin/tests/{test_id}", "AdminService", "DeleteTestCase", "TestCaseIdRequest", "EmptyResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-question", "POST", "/api/admin/questions/cache/invalidate", "AdminService", "InvalidateQuestionCache", "InvalidateQuestionCacheRequest", "EmptyResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
    {"admin-account", "GET", "/api/admin/accounts", "AdminService", "ListAdminAccounts", "ListAdminAccountsRequest", "ListAdminAccountsResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED,INTERNAL"},
    {"admin-account", "GET", "/api/admin/accounts/{admin_id}", "AdminService", "GetAdminAccount", "AdminAccountIdRequest", "AdminAccountResponse", Auth::kSuperAdmin, "NOT_FOUND,PERMISSION_DENIED"},
    {"admin-account", "POST", "/api/admin/accounts", "AdminService", "CreateAdminAccount", "SaveAdminAccountRequest", "AdminAccountResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,CONFLICT,PERMISSION_DENIED,INTERNAL"},
    {"admin-account", "PUT", "/api/admin/accounts/{admin_id}", "AdminService", "UpdateAdminAccount", "SaveAdminAccountRequest", "AdminAccountResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,CONFLICT,PERMISSION_DENIED,INTERNAL"},
    {"admin-account", "DELETE", "/api/admin/accounts/{admin_id}", "AdminService", "DeleteAdminAccount", "AdminAccountIdRequest", "EmptyResponse", Auth::kSuperAdmin, "INVALID_ARGUMENT,NOT_FOUND,CONFLICT,PERMISSION_DENIED,INTERNAL"},
    {"admin-audit", "GET", "/api/admin/audit/logs", "AdminService", "GetAuditLogs", "GetAuditLogsRequest", "GetAuditLogsResponse", Auth::kAdmin, "NOT_FOUND,INTERNAL"},
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
    {"QuestionService.ListQuestions", 1, 5, 1000000, 100},
    {"SolutionService.ListSolutions", 1, 10, 100, 50},
    {"CommentService.ListComments", 1, 20, 1000000, 1000},
    {"CommentService.ListReplies", 1, 20, 1000000, 1000},
}};

inline constexpr std::array<PagingContract, 4> kAdminPaging{{
    {"AdminService.ListUsers", 1, 20, 1000000, 200},
    {"AdminService.ListQuestions", 1, 20, 1000000, 200},
    {"AdminService.ListAdminAccounts", 1, 20, 1000000, 200},
    {"AdminService.GetAuditLogs", 1, 20, 1000000, 200},
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
