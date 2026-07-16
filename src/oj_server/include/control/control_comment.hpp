#pragma once

#include "comm.hpp"
#include "control_base.hpp"

namespace oj::control
{

    class ControlComment : public ControlBase
    {
    public:
        //发表评论
        bool PostComment(unsigned long long solution_id,
                         const User& current_user,
                         const std::string& content,
                         Json::Value* result,
                         std::string* err_code,
                         unsigned long long parent_id = 0)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            // Authorization check
            if (current_user.uid <= 0)
            {
                *err_code = "UNAUTHORIZED";
                return false;
            }

            std::string trimmed = oj::util::StringUtil::TrimSpace(content);
            if (trimmed.empty())
            {
                *err_code = "CONTENT_REQUIRED";
                return false;
            }
            if (trimmed.size() > 1000)
            {
                *err_code = "CONTENT_TOO_LONG";
                return false;
            }

            //检查题解是否存在
            Solution dummy;
            if (!_model.Solution().GetSolutionById(solution_id, &dummy))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            Comment c;
            c.solution_id = solution_id;
            c.user_id = current_user.uid;
            c.content = trimmed;
            c.is_edited = false;
            //评论最多只有两层:一级评论和嵌套评论
            c.parent_id = parent_id;
            unsigned long long reply_to_user_id = 0;
            if (parent_id > 0)
            {
                Comment parent;
                //获取父级评论
                if (!_model.Comment().GetCommentById(parent_id, &parent))
                {
                    *err_code = "PARENT_NOT_FOUND";
                    return false;
                }
                reply_to_user_id = parent.user_id;
                if (parent.solution_id != solution_id)
                {
                    *err_code = "INVALID_PARENT_SOLUTION";
                    return false;
                }
                // If replying to a reply, elevate to top-level parent
                if (parent.parent_id.get() > 0)
                {
                    c.parent_id = parent.parent_id;
                }
            }
            c.reply_to_user_id = static_cast<int>(reply_to_user_id);
            //创建评论
            unsigned long long new_id = 0;
            if (!_model.Comment().CreateComment(c, &new_id))
            {
                *err_code = "CREATE_FAILED";
                return false;
            }

            //填充评论
            Comment created;
            if (_model.Comment().GetCommentById(new_id, &created))
            {
                result->clear();
                (*result)["success"] = true;
                CommentToJson(created, *result);
            }
            else
            {
                // Fallback minimal success payload
                result->clear();
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(new_id);
                (*result)["solution_id"] = Json::UInt64(solution_id);
                (*result)["content"] = trimmed;
            }
            return true;
        }

        // Comments: get comments for a solution with pagination
        bool GetComments(unsigned long long solution_id, int page, int size,
                         Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> comments;
            int total_count = 0;
            if (!_model.Comment().GetCommentsBySolutionId(solution_id, page, size, &comments, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            const int safe_page = std::max(1, page);
            const int safe_size = std::max(1, size);
            result->clear();
            SetPaginationResult(result, total_count, safe_page, safe_size);

            Json::Value list(Json::arrayValue);
            for (const auto& c : comments)
            {
                Json::Value item;
                CommentToJson(c, item);
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }
        //获取顶级评论列表
        bool GetTopLevelComments(unsigned long long solution_id, int page, int size,
                                 Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> comments;
            int total_count = 0;
            //获取顶级评论列表
            if (!_model.Comment().GetCommentsBySolutionId(solution_id, page, size, &comments, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            const int safe_page = std::max(1, page);
            const int safe_size = std::max(1, size);
            result->clear();
            SetPaginationResult(result, total_count, safe_page, safe_size);

            // Batch fetch reply counts for all top-level comments
            std::map<unsigned long long, int> reply_counts;
            if (!comments.empty())
            {
                std::vector<unsigned long long> comment_ids;
                comment_ids.reserve(comments.size());
                for (const auto& comment : comments)
                {
                    comment_ids.push_back(comment.id);
                }
                if (!_model.Comment().GetDirectChildReplyCounts(comment_ids, &reply_counts))
                {
                    *err_code = "DB_ERROR";
                    return false;
                }
            }
            //写入JSON
            Json::Value list(Json::arrayValue);
            for (const auto& c : comments)
            {
                Json::Value item;
                CommentToJson(c, item);
                item["reply_count"] = reply_counts.count(c.id) ? reply_counts[c.id] : 0;
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        //获取回复列表
        bool GetCommentReplies(unsigned long long parent_id, int page, int size,
                               Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> replies;
            int total_count = 0;
            if (!_model.Comment().GetCommentReplies(parent_id, page, size, &replies, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            const int safe_page = std::max(1, page);
            const int safe_size = std::max(1, size);
            result->clear();
            SetPaginationResult(result, total_count, safe_page, safe_size);

            // Batch fetch nested reply counts for all replies
            std::map<unsigned long long, int> reply_counts;
            if (!replies.empty())
            {
                std::vector<unsigned long long> reply_ids;
                reply_ids.reserve(replies.size());
                for (const auto& reply : replies)
                {
                    reply_ids.push_back(reply.id);
                }
                if (!_model.Comment().GetDirectChildReplyCounts(reply_ids, &reply_counts))
                {
                    *err_code = "DB_ERROR";
                    return false;
                }
            }

            Json::Value list(Json::arrayValue);
            for (const auto& r : replies)
            {
                Json::Value item;
                CommentToJson(r, item);
                item["reply_count"] = reply_counts.count(r.id) ? reply_counts[r.id] : 0;
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        //编辑评论
        bool EditComment(unsigned long long comment_id, const User& current_user,
                         const std::string& content, Json::Value* result, std::string* err_code)
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
            std::string trimmed = oj::util::StringUtil::TrimSpace(content);
            if (trimmed.empty())
            {
                *err_code = "CONTENT_REQUIRED";
                return false;
            }
            if (trimmed.size() > 1000)
            {
                *err_code = "CONTENT_TOO_LONG";
                return false;
            }

            Comment c;
            //获取评论
            if (!_model.Comment().GetCommentById(comment_id, &c))
            {
                *err_code = "NOT_FOUND";
                return false;
            }
            if (c.user_id != current_user.uid)
            {
                *err_code = "FORBIDDEN";
                return false;
            }
            //更新评论
            if (!_model.Comment().UpdateComment(comment_id, current_user.uid, trimmed))
            {
                *err_code = "UPDATE_FAILED";
                return false;
            }
            //返回更新的评论
            Comment updated;
            if (_model.Comment().GetCommentById(comment_id, &updated))
            {
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(updated.id);
                (*result)["content"] = updated.content;
                (*result)["is_edited"] = updated.is_edited.get();
                (*result)["updated_at"] = oj::util::TimeUtil::DateTimeToInt(updated.updated_at.get());;
            }
            else
            {
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(comment_id);
            }
            return true;
        }

        // Comments: delete a comment (owner or admin)
        bool DeleteComment(unsigned long long comment_id, const User& current_user,
                           Json::Value* result, std::string* err_code)
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
            Comment c;
            //获取评论
            if (!_model.Comment().GetCommentById(comment_id, &c))
            {
                *err_code = "NOT_FOUND";
                return false;
            }
            //判断是否是管理员(管理员可删评论)
            bool is_admin = _model.GetAdminByUid(current_user.uid);
            if (c.user_id != current_user.uid && !is_admin)
            {
                *err_code = "FORBIDDEN";
                return false;
            }
            //删除评论
            if (!_model.Comment().DeleteComment(comment_id, current_user.uid, is_admin))
            {
                *err_code = "DELETE_FAILED";
                return false;
            }
            (*result)["success"] = true;
            return true;
        }

        // Comments: toggle like on a comment
        bool ToggleCommentLike(unsigned long long comment_id,
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

            Comment c;
            if (!_model.Comment().GetCommentById(comment_id, &c))
            {
                *err_code = "COMMENT_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.Comment().ToggleCommentAction(comment_id, current_user.uid, "like", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["liked"] = now_active;
            (*result)["like_count"] = new_count;
            return true;
        }

        // Comments: toggle favorite on a comment
        bool ToggleCommentFavorite(unsigned long long comment_id,
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

            Comment c;
            if (!_model.Comment().GetCommentById(comment_id, &c))
            {
                *err_code = "COMMENT_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.Comment().ToggleCommentAction(comment_id, current_user.uid, "favorite", &now_active, &new_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["favorited"] = now_active;
            (*result)["favorite_count"] = new_count;
            return true;
        }

        // Comments: get actions for multiple comments for a user
        bool GetCommentActions(const std::vector<unsigned long long>& comment_ids,
                               int user_id,
                               Json::Value* result)
        {
            if (result == nullptr)
            {
                return false;
            }
            std::map<unsigned long long, std::map<std::string, bool>> actions;
            if (!_model.Comment().GetCommentActions(comment_ids, user_id, &actions))
            {
                return false;
            }

            (*result)["success"] = true;
            Json::Value actions_json(Json::objectValue);
            for (const auto& kv : actions)
            {
                Json::Value item(Json::objectValue);
                auto cid = kv.first;
                const auto& map = kv.second;
                auto it_like = map.find("like");
                auto it_fav = map.find("favorite");
                item["like"] = (it_like != map.end()) ? it_like->second : false;
                item["favorite"] = (it_fav != map.end()) ? it_fav->second : false;
                // use string key to avoid JSONCPP index type issues
                actions_json[std::to_string(cid)] = item;
            }
            (*result)["actions"] = actions_json;
            return true;
        }
    };

}
