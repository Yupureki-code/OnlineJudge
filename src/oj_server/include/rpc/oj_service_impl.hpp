#pragma once

#include <memory>

#include "../../../comm/proto/biz_service.pb.h"
#include "../runtime/business_executor.hpp"

namespace oj::control
{
class Control;
}

namespace oj::rpc
{

    class OJServiceImpl final : public oj::biz::OJService
    {
    public:
        OJServiceImpl(std::shared_ptr<oj::control::Control> control,
                    oj::runtime::BusinessExecutor& executor);

    #define OJ_MAIN_RPC(method, request_type, response_type) \
        void method(google::protobuf::RpcController* controller, \
                    const request_type* request, response_type* response, \
                    google::protobuf::Closure* done) override

        OJ_MAIN_RPC(SendRegistrationCode, oj::biz::SendVerificationCodeRequest, oj::biz::SendVerificationCodeResponse);
        OJ_MAIN_RPC(Register, oj::biz::RegisterRequest, oj::biz::AuthResponse);
        OJ_MAIN_RPC(LoginWithVerificationCode, oj::biz::VerificationCodeLoginRequest, oj::biz::AuthResponse);
        OJ_MAIN_RPC(LoginWithPassword, oj::biz::PasswordLoginRequest, oj::biz::AuthResponse);
        OJ_MAIN_RPC(Logout, oj::common::EmptyRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(SetPassword, oj::biz::SetPasswordRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(SendSecurityCode, oj::common::EmptyRequest, oj::biz::SendVerificationCodeResponse);
        OJ_MAIN_RPC(ChangePassword, oj::biz::ChangePasswordRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(ChangeEmail, oj::biz::ChangeEmailRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(DeleteAccount, oj::biz::DeleteAccountRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(GetCurrentUser, oj::common::EmptyRequest, oj::biz::GetUserProfileResponse);
        OJ_MAIN_RPC(GetUserProfile, oj::biz::GetUserProfileRequest, oj::biz::GetUserProfileResponse);
        OJ_MAIN_RPC(UpdateProfile, oj::biz::UpdateProfileRequest, oj::biz::UpdateProfileResponse);
        OJ_MAIN_RPC(UpdateAvatar, oj::biz::UpdateAvatarRequest, oj::biz::UpdateAvatarResponse);
        OJ_MAIN_RPC(DeleteAvatar, oj::common::EmptyRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(GetStatistics, oj::common::EmptyRequest, oj::biz::GetUserStatisticsResponse);
        OJ_MAIN_RPC(ListQuestions, oj::biz::GetQuestionListRequest, oj::biz::GetQuestionListResponse);
        OJ_MAIN_RPC(GetQuestion, oj::biz::QuestionIdRequest, oj::biz::GetQuestionDetailResponse);
        OJ_MAIN_RPC(GetPassStatus, oj::biz::QuestionIdRequest, oj::biz::GetQuestionPassStatusResponse);
        OJ_MAIN_RPC(CreateQuestion, oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse);
        OJ_MAIN_RPC(UpdateQuestion, oj::biz::SaveQuestionRequest, oj::biz::QuestionResponse);
        OJ_MAIN_RPC(DeleteQuestion, oj::biz::QuestionIdRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(ListTestCases, oj::biz::ListTestCasesRequest, oj::biz::ListTestCasesResponse);
        OJ_MAIN_RPC(CreateTestCase, oj::biz::CreateTestCaseRequest, oj::biz::TestCaseResponse);
        OJ_MAIN_RPC(UpdateTestCase, oj::biz::UpdateTestCaseRequest, oj::biz::TestCaseResponse);
        OJ_MAIN_RPC(DeleteTestCase, oj::biz::TestCaseIdRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(InvalidateCache, oj::biz::InvalidateQuestionCacheRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(ListSolutions, oj::biz::ListSolutionsRequest, oj::biz::ListSolutionsResponse);
        OJ_MAIN_RPC(GetSolution, oj::biz::SolutionIdRequest, oj::biz::GetSolutionResponse);
        OJ_MAIN_RPC(CreateSolution, oj::biz::CreateSolutionRequest, oj::biz::SolutionResponse);
        OJ_MAIN_RPC(UpdateSolution, oj::biz::UpdateSolutionRequest, oj::biz::SolutionResponse);
        OJ_MAIN_RPC(DeleteSolution, oj::biz::SolutionIdRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(ToggleSolutionLike, oj::biz::SolutionIdRequest, oj::biz::SolutionActionResponse);
        OJ_MAIN_RPC(ToggleSolutionFavorite, oj::biz::SolutionIdRequest, oj::biz::SolutionActionResponse);
        OJ_MAIN_RPC(GetActionState, oj::biz::SolutionIdRequest, oj::biz::SolutionActionResponse);
        OJ_MAIN_RPC(ListComments, oj::biz::ListCommentsRequest, oj::biz::ListCommentsResponse);
        OJ_MAIN_RPC(ListReplies, oj::biz::ListRepliesRequest, oj::biz::ListCommentsResponse);
        OJ_MAIN_RPC(CreateComment, oj::biz::CreateCommentRequest, oj::biz::CommentResponse);
        OJ_MAIN_RPC(UpdateComment, oj::biz::UpdateCommentRequest, oj::biz::CommentResponse);
        OJ_MAIN_RPC(DeleteComment, oj::biz::CommentIdRequest, oj::common::EmptyResponse);
        OJ_MAIN_RPC(ToggleCommentLike, oj::biz::CommentIdRequest, oj::biz::CommentActionResponse);
        OJ_MAIN_RPC(ToggleCommentFavorite, oj::biz::CommentIdRequest, oj::biz::CommentActionResponse);
        OJ_MAIN_RPC(GetActionStates, oj::biz::GetCommentActionsRequest, oj::biz::GetCommentActionsResponse);
        OJ_MAIN_RPC(CreateSubmission, oj::biz::CreateSubmissionRequest, oj::biz::CreateSubmissionResponse);
        OJ_MAIN_RPC(CreateCustomTest, oj::biz::CreateCustomTestRequest, oj::biz::CreateCustomTestResponse);
        OJ_MAIN_RPC(GetSubmission, oj::biz::GetSubmissionRequest, oj::biz::GetSubmissionResponse);
        OJ_MAIN_RPC(GetCustomTest, oj::biz::GetCustomTestRequest, oj::biz::GetCustomTestResponse);
        OJ_MAIN_RPC(ListSubmissions, oj::biz::ListSubmissionsRequest, oj::biz::ListSubmissionsResponse);
        OJ_MAIN_RPC(UpdateJudgeResult, oj::biz::UpdateJudgeResultRequest, oj::biz::UpdateJudgeResultResponse);
        OJ_MAIN_RPC(LegacyUpdateJudgeResult, oj_judge::JudgeFinishedRequest, oj_judge::NullRsp);
        OJ_MAIN_RPC(InvalidateStaticCache, oj::biz::InvalidateStaticCacheRequest, oj::common::EmptyResponse);

    #undef OJ_MAIN_RPC

    private:
        std::shared_ptr<oj::control::Control> _control;
        oj::runtime::BusinessExecutor& _executor;
    };
} // namespace oj::rpc
