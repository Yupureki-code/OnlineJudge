local routes = {}

local function add(verb, path, service, method, request_type, response_type, auth, csrf, bindings)
    bindings = bindings or {}
    if bindings.body == nil then bindings.body = verb == "GET" and false or "*" end
    routes[#routes + 1] = {
        verb = verb,
        path = path,
        service = service,
        method = method,
        request_type = request_type,
        response_type = response_type,
        auth = auth,
        csrf = csrf,
        bindings = bindings,
    }
end

local function main(verb, path, method, request_type, response_type, auth, csrf, bindings)
    add(verb, path, "oj.biz.OJService", method, request_type, response_type, auth, csrf, bindings)
end

local function admin(verb, path, method, request_type, response_type, auth, csrf, bindings)
    add(verb, path, "oj.biz.OJAdminService", method, request_type, response_type, auth, csrf, bindings)
end

routes[#routes + 1] = { verb = "GET", path = "/api/csrf", local_handler = "csrf", auth = "public", csrf = false }
routes[#routes + 1] = { verb = "GET", path = "/admin/api/csrf", local_handler = "csrf", auth = "public", csrf = false, csrf_path = "/admin" }

main("POST", "/api/auth/registration-code", "SendRegistrationCode", "oj.biz.SendVerificationCodeRequest", "oj.biz.SendVerificationCodeResponse", "public", true)
main("POST", "/api/auth/register", "Register", "oj.biz.RegisterRequest", "oj.biz.AuthResponse", "public", true)
main("POST", "/api/auth/login/code", "LoginWithVerificationCode", "oj.biz.VerificationCodeLoginRequest", "oj.biz.AuthResponse", "public", true)
main("POST", "/api/auth/login/password", "LoginWithPassword", "oj.biz.PasswordLoginRequest", "oj.biz.AuthResponse", "public", true)
main("POST", "/api/auth/logout", "Logout", "oj.common.EmptyRequest", "oj.common.EmptyResponse", "optional_user", true)
main("PUT", "/api/user/password", "SetPassword", "oj.biz.SetPasswordRequest", "oj.common.EmptyResponse", "user", true)
main("POST", "/api/user/security-code", "SendSecurityCode", "oj.biz.SendSecurityCodeRequest", "oj.biz.SendVerificationCodeResponse", "user", true)
main("PUT", "/api/user/password/change", "ChangePassword", "oj.biz.ChangePasswordRequest", "oj.common.EmptyResponse", "user", true)
main("PUT", "/api/user/email", "ChangeEmail", "oj.biz.ChangeEmailRequest", "oj.common.EmptyResponse", "user", true)
main("DELETE", "/api/user", "DeleteAccount", "oj.biz.DeleteAccountRequest", "oj.common.EmptyResponse", "user", true)
main("GET", "/api/user", "GetCurrentUser", "oj.common.EmptyRequest", "oj.biz.GetUserProfileResponse", "optional_user", false)
main("GET", "/api/users/:uid", "GetUserProfile", "oj.biz.GetUserProfileRequest", "oj.biz.GetUserProfileResponse", "optional_user", false, { path = { uid = "uid" } })
main("PATCH", "/api/user/profile", "UpdateProfile", "oj.biz.UpdateProfileRequest", "oj.biz.UpdateProfileResponse", "user", true)
main("PUT", "/api/user/avatar", "UpdateAvatar", "oj.biz.UpdateAvatarRequest", "oj.biz.UpdateAvatarResponse", "user", true)
main("DELETE", "/api/user/avatar", "DeleteAvatar", "oj.common.EmptyRequest", "oj.common.EmptyResponse", "user", true)
main("GET", "/api/user/statistics", "GetStatistics", "oj.common.EmptyRequest", "oj.biz.GetUserStatisticsResponse", "user", false)

main("GET", "/api/questions", "ListQuestions", "oj.biz.GetQuestionListRequest", "oj.biz.GetQuestionListResponse", "optional_user", false,
     { query = { page = "page.page", page_size = "page.page_size", search = "search", difficulty = "difficulty" } })
main("GET", "/api/questions/:question_id", "GetQuestion", "oj.biz.QuestionIdRequest", "oj.biz.GetQuestionDetailResponse", "public", false, { path = { question_id = "question_id" } })
main("GET", "/api/questions/:question_id/pass-status", "GetPassStatus", "oj.biz.QuestionIdRequest", "oj.biz.GetQuestionPassStatusResponse", "optional_user", false, { path = { question_id = "question_id" } })
main("GET", "/api/questions/:question_id/test-cases", "ListTestCases", "oj.biz.ListTestCasesRequest", "oj.biz.ListTestCasesResponse", "public", false,
     { path = { question_id = "question_id" }, query = { sample_only = "sample_only" } })

main("GET", "/api/questions/:question_id/solutions", "ListSolutions", "oj.biz.ListSolutionsRequest", "oj.biz.ListSolutionsResponse", "optional_user", false,
     { path = { question_id = "question_id" }, query = { page = "page.page", page_size = "page.page_size", sort_by = "sort_by", author_uid = "author_uid" } })
main("GET", "/api/solutions/:solution_id", "GetSolution", "oj.biz.SolutionIdRequest", "oj.biz.GetSolutionResponse", "optional_user", false, { path = { solution_id = "solution_id" } })
main("POST", "/api/questions/:question_id/solutions", "CreateSolution", "oj.biz.CreateSolutionRequest", "oj.biz.SolutionResponse", "user", true, { path = { question_id = "question_id" } })
main("PUT", "/api/solutions/:solution_id", "UpdateSolution", "oj.biz.UpdateSolutionRequest", "oj.biz.SolutionResponse", "user", true, { path = { solution_id = "solution_id" } })
main("DELETE", "/api/solutions/:solution_id", "DeleteSolution", "oj.biz.SolutionIdRequest", "oj.common.EmptyResponse", "user", true, { path = { solution_id = "solution_id" } })
main("POST", "/api/solutions/:solution_id/like", "ToggleSolutionLike", "oj.biz.SolutionIdRequest", "oj.biz.SolutionActionResponse", "user", true, { path = { solution_id = "solution_id" } })
main("POST", "/api/solutions/:solution_id/favorite", "ToggleSolutionFavorite", "oj.biz.SolutionIdRequest", "oj.biz.SolutionActionResponse", "user", true, { path = { solution_id = "solution_id" } })
main("GET", "/api/solutions/:solution_id/actions", "GetActionState", "oj.biz.SolutionIdRequest", "oj.biz.SolutionActionResponse", "optional_user", false, { path = { solution_id = "solution_id" } })

main("GET", "/api/solutions/:solution_id/comments", "ListComments", "oj.biz.ListCommentsRequest", "oj.biz.ListCommentsResponse", "optional_user", false,
     { path = { solution_id = "solution_id" }, query = { page = "page.page", page_size = "page.page_size" } })
main("GET", "/api/comments/:comment_id/replies", "ListReplies", "oj.biz.ListRepliesRequest", "oj.biz.ListCommentsResponse", "optional_user", false,
     { path = { comment_id = "parent_comment_id" }, query = { page = "page.page", page_size = "page.page_size" } })
main("POST", "/api/solutions/:solution_id/comments", "CreateComment", "oj.biz.CreateCommentRequest", "oj.biz.CommentResponse", "user", true, { path = { solution_id = "solution_id" } })
main("PUT", "/api/comments/:comment_id", "UpdateComment", "oj.biz.UpdateCommentRequest", "oj.biz.CommentResponse", "user", true, { path = { comment_id = "comment_id" } })
main("DELETE", "/api/comments/:comment_id", "DeleteComment", "oj.biz.CommentIdRequest", "oj.common.EmptyResponse", "user", true, { path = { comment_id = "comment_id" } })
main("POST", "/api/comments/:comment_id/like", "ToggleCommentLike", "oj.biz.CommentIdRequest", "oj.biz.CommentActionResponse", "user", true, { path = { comment_id = "comment_id" } })
main("POST", "/api/comments/:comment_id/favorite", "ToggleCommentFavorite", "oj.biz.CommentIdRequest", "oj.biz.CommentActionResponse", "user", true, { path = { comment_id = "comment_id" } })
main("GET", "/api/comments/actions", "GetActionStates", "oj.biz.GetCommentActionsRequest", "oj.biz.GetCommentActionsResponse", "optional_user", false,
     { query = { comment_ids = "comment_ids" } })

main("POST", "/api/questions/:question_id/submissions", "CreateSubmission", "oj.biz.CreateSubmissionRequest", "oj.biz.CreateSubmissionResponse", "optional_user", true, { path = { question_id = "question_id" } })
main("POST", "/api/questions/:question_id/custom-tests", "CreateCustomTest", "oj.biz.CreateCustomTestRequest", "oj.biz.CreateCustomTestResponse", "user", true, { path = { question_id = "question_id" } })
main("GET", "/api/submissions/:submission_id", "GetSubmission", "oj.biz.GetSubmissionRequest", "oj.biz.GetSubmissionResponse", "optional_user", false, { path = { submission_id = "submission_id" } })
main("GET", "/api/custom-tests/:task_id", "GetCustomTest", "oj.biz.GetCustomTestRequest", "oj.biz.GetCustomTestResponse", "user", false, { path = { task_id = "task_id" } })
main("GET", "/api/submissions", "ListSubmissions", "oj.biz.ListSubmissionsRequest", "oj.biz.ListSubmissionsResponse", "optional_user", false,
     { query = { page = "page.page", page_size = "page.page_size", question_id = "question_id", submission_status = "submission_status" } })

admin("GET", "/admin/api/version", "AdminGetVersion", "oj.common.EmptyRequest", "oj.biz.GetVersionResponse", "public", false)
admin("POST", "/admin/api/auth/login", "AdminLogin", "oj.biz.AdminLoginRequest", "oj.biz.AdminLoginResponse", "public", true)
admin("POST", "/admin/api/auth/logout", "AdminLogout", "oj.common.EmptyRequest", "oj.common.EmptyResponse", "optional_admin", true)
admin("GET", "/admin/api/overview", "AdminGetOverview", "oj.common.EmptyRequest", "oj.biz.AdminOverviewResponse", "admin", false)
admin("GET", "/admin/api/users", "AdminListUsers", "oj.biz.AdminListUsersRequest", "oj.biz.AdminListUsersResponse", "admin", false,
      { query = { page = "page.page", page_size = "page.page_size", search = "search" } })
admin("GET", "/admin/api/users/:uid", "AdminGetUser", "oj.biz.AdminUserIdRequest", "oj.biz.AdminUserResponse", "admin", false, { path = { uid = "uid" } })
admin("POST", "/admin/api/users", "AdminCreateUser", "oj.biz.AdminCreateUserRequest", "oj.biz.AdminUserResponse", "admin", true)
admin("PUT", "/admin/api/users/:uid", "AdminUpdateUser", "oj.biz.AdminUpdateUserRequest", "oj.biz.AdminUserResponse", "admin", true, { path = { uid = "user.uid" } })
admin("DELETE", "/admin/api/users/:uid", "AdminDeleteUser", "oj.biz.AdminUserIdRequest", "oj.common.EmptyResponse", "admin", true, { path = { uid = "uid" } })
admin("GET", "/admin/api/questions", "AdminListQuestions", "oj.biz.GetQuestionListRequest", "oj.biz.GetQuestionListResponse", "admin", false,
      { query = { page = "page.page", page_size = "page.page_size", search = "search", difficulty = "difficulty", include_hidden = "include_hidden" } })
admin("GET", "/admin/api/questions/:question_id", "AdminGetQuestion", "oj.biz.QuestionIdRequest", "oj.biz.GetQuestionDetailResponse", "admin", false, { path = { question_id = "question_id" } })
admin("POST", "/admin/api/questions", "AdminCreateQuestion", "oj.biz.SaveQuestionRequest", "oj.biz.QuestionResponse", "admin", true)
admin("PUT", "/admin/api/questions/:question_id", "AdminUpdateQuestion", "oj.biz.SaveQuestionRequest", "oj.biz.QuestionResponse", "admin", true, { path = { question_id = "question.id" } })
admin("DELETE", "/admin/api/questions/:question_id", "AdminDeleteQuestion", "oj.biz.QuestionIdRequest", "oj.common.EmptyResponse", "admin", true, { path = { question_id = "question_id" } })
admin("GET", "/admin/api/questions/:question_id/test-cases", "AdminListTestCases", "oj.biz.ListTestCasesRequest", "oj.biz.ListTestCasesResponse", "super_admin", false, { path = { question_id = "question_id" } })
admin("POST", "/admin/api/questions/:question_id/test-cases", "AdminCreateTestCase", "oj.biz.CreateTestCaseRequest", "oj.biz.TestCaseResponse", "super_admin", true, { path = { question_id = "question_id" } })
admin("PUT", "/admin/api/test-cases/:test_case_id", "AdminUpdateTestCase", "oj.biz.UpdateTestCaseRequest", "oj.biz.TestCaseResponse", "super_admin", true, { path = { test_case_id = "test_case.test_case_id" } })
admin("DELETE", "/admin/api/test-cases/:test_case_id", "AdminDeleteTestCase", "oj.biz.TestCaseIdRequest", "oj.common.EmptyResponse", "super_admin", true, { path = { test_case_id = "test_case_id" } })
admin("POST", "/admin/api/questions/cache/invalidate", "AdminInvalidateQuestionCache", "oj.biz.InvalidateQuestionCacheRequest", "oj.common.EmptyResponse", "admin", true)
admin("GET", "/admin/api/accounts", "AdminListAdminAccounts", "oj.biz.ListAdminAccountsRequest", "oj.biz.ListAdminAccountsResponse", "super_admin", false,
      { query = { page = "page.page", page_size = "page.page_size" } })
admin("GET", "/admin/api/accounts/:admin_id", "AdminGetAdminAccount", "oj.biz.AdminAccountIdRequest", "oj.biz.AdminAccountResponse", "super_admin", false, { path = { admin_id = "admin_id" } })
admin("POST", "/admin/api/accounts", "AdminCreateAdminAccount", "oj.biz.SaveAdminAccountRequest", "oj.biz.AdminAccountResponse", "super_admin", true)
admin("PUT", "/admin/api/accounts/:admin_id", "AdminUpdateAdminAccount", "oj.biz.SaveAdminAccountRequest", "oj.biz.AdminAccountResponse", "super_admin", true, { path = { admin_id = "account.admin_id" } })
admin("DELETE", "/admin/api/accounts/:admin_id", "AdminDeleteAdminAccount", "oj.biz.AdminAccountIdRequest", "oj.common.EmptyResponse", "super_admin", true, { path = { admin_id = "admin_id" } })
admin("GET", "/admin/api/cache/metrics", "AdminGetCacheMetrics", "oj.common.EmptyRequest", "oj.biz.GetCacheMetricsResponse", "super_admin", false)
admin("GET", "/admin/api/audit-logs", "AdminGetAuditLogs", "oj.biz.GetAuditLogsRequest", "oj.biz.GetAuditLogsResponse", "admin", false,
      { query = { page = "page.page", page_size = "page.page_size", action = "action", start_time = "start_time", end_time = "end_time" } })
admin("GET", "/admin/api/diagnostics/files", "AdminListDiagnosticFiles", "oj.biz.DiagnosticFilesRequest", "oj.biz.DiagnosticFilesResponse", "admin", false,
      { query = { kind = "kind" } })
admin("GET", "/admin/api/diagnostics/content", "AdminReadDiagnosticFile", "oj.biz.DiagnosticContentRequest", "oj.biz.DiagnosticContentResponse", "admin", false,
      { query = { kind = "kind", file = "file" } })

routes.denied = {
    ["oj.biz.OJService.CreateQuestion"] = "admin-only operation",
    ["oj.biz.OJService.UpdateQuestion"] = "admin-only operation",
    ["oj.biz.OJService.DeleteQuestion"] = "admin-only operation",
    ["oj.biz.OJService.CreateTestCase"] = "admin-only operation",
    ["oj.biz.OJService.UpdateTestCase"] = "admin-only operation",
    ["oj.biz.OJService.DeleteTestCase"] = "admin-only operation",
    ["oj.biz.OJService.InvalidateCache"] = "admin-only operation",
    ["oj.biz.OJService.UpdateJudgeResult"] = "internal Judge callback",
    ["oj.biz.OJService.LegacyUpdateJudgeResult"] = "retired internal callback",
    ["oj.biz.OJService.InvalidateStaticCache"] = "internal cache operation",
}

return routes
