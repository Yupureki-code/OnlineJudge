#pragma once

#include "control_comment.hpp"

namespace ns_control
{

    class ControlSolution : public ControlComment
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
            if (!_model.GetOneQuestion(trimmed_question_id, question))
            {
                if (err_code != nullptr)
                {
                    *err_code = "QUESTION_NOT_FOUND";
                }
                return false;
            }
            //查看该用户是否通过该题目
            if (!_model.HasUserPassedQuestion(current_user.uid, trimmed_question_id))
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
            if (!_model.CreateSolution(solution, solution_id))
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
            if (!_model.GetOneQuestion(question_id, question))
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
            if (!_model.GetSolutionsByPage(question_id, status_filter, sort_order,
                                           safe_page, safe_size,
                                           &solutions, &total_count, &total_pages))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = safe_page;
            (*result)["size"] = safe_size;

            Json::Value items(Json::arrayValue);
            for (const auto& s : solutions)
            {
                Json::Value item;
                item["id"] = Json::UInt64(s.id);
                item["question_id"] = s.question_id;
                item["user_id"] = s.user_id;
                item["title"] = s.title;
                item["content_md"] = s.content_md;
                item["like_count"] = s.like_count;
                item["favorite_count"] = s.favorite_count;
                item["comment_count"] = s.comment_count;
                item["status"] = _model.SolutionStatusToDbString(s.status);
                item["created_at"] = s.created_at;

                User author;
                if (_model.GetUserById(s.user_id, &author))
                {
                    item["author_name"] = author.name;
                    item["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    item["author_name"] = "";
                    item["author_avatar"] = "/pictures/head.jpg";
                }

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
            if (!_model.GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            (*result)["success"] = true;
            (*result)["id"] = Json::UInt64(solution.id);
            (*result)["question_id"] = solution.question_id;
            (*result)["user_id"] = solution.user_id;
            (*result)["title"] = solution.title;
            (*result)["content_md"] = solution.content_md;
            (*result)["like_count"] = solution.like_count;
            (*result)["favorite_count"] = solution.favorite_count;
            (*result)["comment_count"] = solution.comment_count;
            (*result)["status"] = _model.SolutionStatusToDbString(solution.status);
            (*result)["created_at"] = solution.created_at;
            (*result)["updated_at"] = solution.updated_at;

            User author;
            if (_model.GetUserById(solution.user_id, &author))
            {
                (*result)["author_name"] = author.name;
                (*result)["author_avatar"] = GetEffectiveAvatarUrl(author);
            }
            else
            {
                (*result)["author_name"] = "";
                (*result)["author_avatar"] = "/pictures/head.jpg";
            }

            return true;
        }

        bool ToggleLike(long long solution_id,
                        const User& current_user,
                        Json::Value* result,
                        std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            if (current_user.uid <= 0)
            {
                *err_code = "UNAUTHORIZED";
                return false;
            }

            Solution solution;
            if (!_model.GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleSolutionAction(solution_id, current_user.uid, "like", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["liked"] = now_active;
            (*result)["like_count"] = new_count;
            return true;
        }

        bool ToggleFavorite(long long solution_id,
                            const User& current_user,
                            Json::Value* result,
                            std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }

            if (current_user.uid <= 0)
            {
                *err_code = "UNAUTHORIZED";
                return false;
            }

            Solution solution;
            if (!_model.GetSolutionById(solution_id, &solution))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleSolutionAction(solution_id, current_user.uid, "favorite", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["favorited"] = now_active;
            (*result)["favorite_count"] = new_count;
            return true;
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
            if (!_model.GetUserActionsForSolutions(user_id, ids, actions))
            {
                return false;
            }

            (*result)["liked"] = actions.count(solution_id) > 0 && actions[solution_id].count("like") > 0;
            (*result)["favorited"] = actions.count(solution_id) > 0 && actions[solution_id].count("favorite") > 0;
            return true;
        }
    };

}
