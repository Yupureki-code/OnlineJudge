#pragma once

#include "comm.hpp"
#include "control_base.hpp"

namespace oj::control
{

    class ControlSolution : public ControlBase
    {
    public:
        //发布题解
        bool PublishSolution(const std::string& question_id,
                              const User& current_user,
                              const std::string& title,
                              const std::string& content_md,
                              unsigned long long* solution_id,
                              std::string* err_code)
        {
            if (current_user.uid <= 0)
            {
                if (err_code != nullptr)
                {
                    *err_code = "UNAUTHORIZED";
                }
                return false;
            }

            std::string trimmed_question_id = oj::util::StringUtil::TrimSpace(question_id);
            if (trimmed_question_id.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "INVALID_QUESTION_ID";
                }
                return false;
            }
            //得到题目信息
            Question question;
            if (!_model.Question().GetOneQuestion(trimmed_question_id, question))
            {
                if (err_code != nullptr)
                {
                    *err_code = "QUESTION_NOT_FOUND";
                }
                return false;
            }
            //查看该用户是否通过该题目
            if (!_model.User().HasUserPassedQuestion(current_user.uid, trimmed_question_id))
            {
                if (err_code != nullptr)
                {
                    *err_code = "NOT_PASSED";
                }
                return false;
            }

            std::string trimmed_title = oj::util::StringUtil::TrimSpace(title);
            std::string trimmed_content = oj::util::StringUtil::TrimSpace(content_md);
            if (trimmed_title.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "TITLE_REQUIRED";
                }
                return false;
            }

            if (trimmed_title.size() > 30)
            {
                if (err_code != nullptr)
                {
                    *err_code = "TITLE_TOO_LONG";
                }
                return false;
            }

            if (trimmed_content.empty())
            {
                if (err_code != nullptr)
                {
                    *err_code = "CONTENT_REQUIRED";
                }
                return false;
            }

            Solution solution;
            solution.question_id = trimmed_question_id;
            solution.user_id = current_user.uid;
            solution.title = trimmed_title;
            solution.content_md = trimmed_content;
            solution.status = "approved";
            //保存题解
            if (!_model.Solution().CreateSolution(solution, solution_id))
            {
                if (err_code != nullptr)
                {
                    *err_code = "CREATE_SOLUTION_FAILED";
                }
                return false;
            }

            return true;
        }

        bool GetSolutionList(const std::string& question_id,
                             const std::string& status,
                             const std::string& sort,
                             int page, int size,
                             std::vector<Solution>* solutions,
                             int* total_count,
                             std::string* err_code)
        {
            if (solutions == nullptr || total_count == nullptr || err_code == nullptr)
            {
                return false;
            }
            (void)status;

            Question question;
            //获取题目
            if (!_model.Question().GetOneQuestion(question_id, question))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }

            const std::string status_filter = "approved";

            std::string sort_order = sort;
            if (sort_order != "hot" && sort_order != "latest")
            {
                sort_order = "latest";
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 50));

            int total_pages = 0;
            //获取分页的题解列表
            if (!_model.Solution().GetSolutionsByPage(question_id, status_filter, sort_order,
                                           safe_page, safe_size,
                                            solutions, total_count, &total_pages))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            return true;
        }
        //获取题解详情
        bool GetSolutionDetail(long long solution_id,
                               Solution* result,
                               User* author,
                               std::string* err_code)
        {
            if (result == nullptr || author == nullptr || err_code == nullptr)
            {
                return false;
            }

            //获取题解详情
            if (!GetPublicSolution(solution_id, result, err_code)) return false;
            *author = User{};
            author->uid = result->user_id;
            _model.User().GetUserById(result->user_id, author);

            return true;
        }

        bool ToggleLike(long long solution_id,
                        const User& current_user,
                        ActionState* result,
                        std::string* err_code)
        {
            return ToggleSolutionAction(solution_id, current_user, "like", result, err_code);
        }

        bool ToggleFavorite(long long solution_id,
                            const User& current_user,
                            ActionState* result,
                            std::string* err_code)
        {
            return ToggleSolutionAction(solution_id, current_user, "favorite", result, err_code);
        }

        bool GetUserSolutionActions(long long solution_id,
                                     int user_id,
                                     ActionState* result,
                                     std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            Solution solution;
            if (!GetPublicSolution(solution_id, &solution, err_code)) return false;

            if (user_id <= 0)
            {
                *result = ActionState{};
                return true;
            }

            std::vector<long long> ids = {solution_id};
            std::map<long long, std::map<std::string, bool>> actions;
            if (!_model.Solution().GetUserActionsForSolutions(user_id, ids, actions))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            result->liked = actions.count(solution_id) > 0 && actions[solution_id].count("like") > 0;
            result->favorited = actions.count(solution_id) > 0 && actions[solution_id].count("favorite") > 0;
            return true;
        }
    };

}
