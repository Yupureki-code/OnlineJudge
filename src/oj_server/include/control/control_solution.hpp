#pragma once

#include "control_base.hpp"

namespace ns_control
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

            std::string trimmed_question_id = TrimSpace(question_id);
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

            std::string trimmed_title = TrimSpace(title);
            std::string trimmed_content = TrimSpace(content_md);
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
            solution.status = SolutionStatus::approved;
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
                             Json::Value* result,
                             std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            Question question;
            //获取题目
            if (!_model.Question().GetOneQuestion(question_id, question))
            {
                *err_code = "QUESTION_NOT_FOUND";
                return false;
            }

            std::string status_filter;
            if (status == "pending" || status == "approved" || status == "rejected")
            {
                status_filter = status;
            }
            else
            {
                status_filter = "approved";
            }

            std::string sort_order = sort;
            if (sort_order != "hot" && sort_order != "latest")
            {
                sort_order = "latest";
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 50));

            std::vector<Solution> solutions;
            int total_count = 0;
            int total_pages = 0;
            //获取分页的题解列表
            if (!_model.Solution().GetSolutionsByPage(question_id, status_filter, sort_order,
                                           safe_page, safe_size,
                                           &solutions, &total_count, &total_pages))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            SetPaginationResult(result, total_count, safe_page, safe_size);

            // 批量获取所有作者信息（N+1优化：1次查询替代N次查询）
            std::set<int> user_ids;
            for (const auto& s : solutions) { user_ids.insert(s.user_id); }
            std::map<int, User> user_map;
            _model.User().GetUsersByIds(user_ids, &user_map);

            Json::Value items(Json::arrayValue);
            for (const auto& s : solutions)
            {
                Json::Value item;
                SolutionToJson(s, item);
                BuildUserInfoJsonBatch(s.user_id, item, user_map);
                items.append(item);
            }
            (*result)["solutions"] = items;
            return true;
        }
        //获取题解详情
        bool GetSolutionDetail(long long solution_id,
                               Json::Value* result,
                               std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            Solution solution;
            //获取题解详情
            if (!_model.Solution().GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            (*result)["success"] = true;
            SolutionToJson(solution, *result);
            BuildUserInfoJson(solution.user_id, *result);

            return true;
        }

        bool ToggleLike(long long solution_id,
                        const User& current_user,
                        Json::Value* result,
                        std::string* err_code)
        {
            return ToggleSolutionAction(solution_id, current_user, "like", result, err_code);
        }

        bool ToggleFavorite(long long solution_id,
                            const User& current_user,
                            Json::Value* result,
                            std::string* err_code)
        {
            return ToggleSolutionAction(solution_id, current_user, "favorite", result, err_code);
        }

        bool GetUserSolutionActions(long long solution_id,
                                    int user_id,
                                    Json::Value* result)
        {
            if (result == nullptr)
            {
                return false;
            }

            std::vector<long long> ids = {solution_id};
            std::map<long long, std::map<std::string, bool>> actions;
            if (!_model.Solution().GetUserActionsForSolutions(user_id, ids, actions))
            {
                return false;
            }

            (*result)["liked"] = actions.count(solution_id) > 0 && actions[solution_id].count("like") > 0;
            (*result)["favorited"] = actions.count(solution_id) > 0 && actions[solution_id].count("favorite") > 0;
            return true;
        }
    };

}
