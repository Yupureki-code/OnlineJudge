#pragma once

#include "control_auth.hpp"

namespace ns_control
{

    class ControlComment : public ControlAuth
    {
    public:
        // Comments: create a new comment for a solution (supports nesting via parent_id)
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

            std::string trimmed = TrimSpace(content);
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

            // Check solution exists
            Solution dummy;
            if (!_model.GetSolutionById(solution_id, &dummy))
            {
                *err_code = "SOLUTION_NOT_FOUND";
                return false;
            }

            Comment c;
            c.solution_id = solution_id;
            c.user_id = current_user.uid;
            c.content = trimmed;
            c.is_edited = false;
            // nesting support — only 2 levels: top-level + direct replies
            c.parent_id = parent_id;
            unsigned long long reply_to_user_id = 0;
            if (parent_id > 0)
            {
                Comment parent;
                if (_model.GetCommentById(parent_id, &parent))
                {
                    reply_to_user_id = parent.user_id;
                    if (parent.solution_id != solution_id)
                    {
                        *err_code = "INVALID_PARENT_SOLUTION";
                        return false;
                    }
                    // If replying to a reply, elevate to top-level parent
                    if (parent.parent_id > 0)
                    {
                        c.parent_id = parent.parent_id;
                    }
                }
            }
            c.reply_to_user_id = static_cast<int>(reply_to_user_id);

            unsigned long long new_id = 0;
            if (!_model.CreateComment(c, &new_id))
            {
                *err_code = "CREATE_FAILED";
                return false;
            }

            // fetch comment to fill response details
            Comment created;
            if (_model.GetCommentById(new_id, &created))
            {
                result->clear();
                (*result) ["success"] = true;
                (*result)["id"] = Json::UInt64(created.id);
                (*result)["solution_id"] = Json::UInt64(created.solution_id);
                (*result)["user_id"] = created.user_id;
                (*result)["content"] = created.content;
                (*result)["is_edited"] = created.is_edited;
                (*result)["created_at"] = created.created_at;
                // author name
                User author;
                if (_model.GetUserById(created.user_id, &author))
                {
                    (*result)["author_name"] = author.name;
                (*result)["author_avatar"] = GetEffectiveAvatarUrl(author);
                }
                else
                {
                    (*result)["author_name"] = "";
                    (*result)["author_avatar"] = "/pictures/head.jpg";
                }
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
            if (!_model.GetCommentsBySolutionId(solution_id, page, size, &comments, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            int total_pages = (size > 0) ? ((total_count + size - 1) / size) : 0;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = std::max(1, page);
            (*result)["size"] = size;

            Json::Value list(Json::arrayValue);
            for (const auto& c : comments)
            {
                Json::Value item;
                item["id"] = Json::UInt64(c.id);
                item["solution_id"] = Json::UInt64(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["parent_id"] = Json::UInt64(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["reply_to_user_name"] = c.reply_to_user_name;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                item["author_name"] = c.author_name;
                item["author_avatar"] = "/pictures/head.jpg";
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        // Comments: get top-level comments for a solution with reply counts
        bool GetTopLevelComments(unsigned long long solution_id, int page, int size,
                                 Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> comments;
            int total_count = 0;
            // fetch top-level comments only
            if (!_model.GetCommentsBySolutionId(solution_id, page, size, &comments, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            int total_pages = (size > 0) ? ((total_count + size - 1) / size) : 0;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = std::max(1, page);
            (*result)["size"] = size;

            // Batch fetch reply counts for all top-level comments
            std::map<unsigned long long, int> reply_counts;
            if (!comments.empty())
            {
                auto rep_my = _model.CreateConnection();
                if (rep_my)
                {
                    std::ostringstream idlist;
                    for (size_t i = 0; i < comments.size(); ++i)
                    {
                        if (i > 0) idlist << ",";
                        idlist << comments[i].id;
                    }
                    std::string batch_sql = "select parent_id, count(*) as cnt from solution_comments where parent_id in (" + idlist.str() + ") group by parent_id";
                    if (mysql_query(rep_my.get(), batch_sql.c_str()) == 0)
                    {
                        MYSQL_RES* rep_res = mysql_store_result(rep_my.get());
                        if (rep_res)
                        {
                            for (int ri = 0; ri < mysql_num_rows(rep_res); ++ri)
                            {
                                MYSQL_ROW rep_row = mysql_fetch_row(rep_res);
                                if (rep_row && rep_row[0] && rep_row[1])
                                    reply_counts[std::stoull(rep_row[0])] = std::atoi(rep_row[1]);
                            }
                            mysql_free_result(rep_res);
                        }
                    }
                }
            }

            Json::Value list(Json::arrayValue);
            for (const auto& c : comments)
            {
                Json::Value item;
                item["id"] = Json::UInt64(c.id);
                item["solution_id"] = Json::UInt64(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["parent_id"] = Json::UInt64(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["reply_to_user_name"] = c.reply_to_user_name;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                // author info (from JOIN, no extra DB query)
                item["author_name"] = c.author_name;
                item["author_avatar"] = "/pictures/head.jpg";
                item["reply_count"] = reply_counts.count(c.id) ? reply_counts[c.id] : 0;
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        // Comments: get replies for a given top-level comment (child comments)
        bool GetCommentReplies(unsigned long long parent_id, int page, int size,
                               Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::vector<Comment> replies;
            int total_count = 0;
            if (!_model.GetCommentReplies(parent_id, page, size, &replies, &total_count))
            {
                *err_code = "DB_ERROR";
                return false;
            }

            (*result)["success"] = true;
            (*result)["total"] = total_count;
            int total_pages = (size > 0) ? ((total_count + size - 1) / size) : 0;
            (*result)["total_pages"] = total_pages;
            (*result)["page"] = std::max(1, page);
            (*result)["size"] = size;

            // Batch fetch nested reply counts for all replies
            std::map<unsigned long long, int> reply_counts;
            if (!replies.empty())
            {
                auto rep_my = _model.CreateConnection();
                if (rep_my)
                {
                    std::ostringstream idlist;
                    for (size_t i = 0; i < replies.size(); ++i)
                    {
                        if (i > 0) idlist << ",";
                        idlist << replies[i].id;
                    }
                    std::string batch_sql = "select parent_id, count(*) as cnt from solution_comments where parent_id in (" + idlist.str() + ") group by parent_id";
                    if (mysql_query(rep_my.get(), batch_sql.c_str()) == 0)
                    {
                        MYSQL_RES* rep_res = mysql_store_result(rep_my.get());
                        if (rep_res)
                        {
                            for (int ri = 0; ri < mysql_num_rows(rep_res); ++ri)
                            {
                                MYSQL_ROW rep_row = mysql_fetch_row(rep_res);
                                if (rep_row && rep_row[0] && rep_row[1])
                                    reply_counts[std::stoull(rep_row[0])] = std::atoi(rep_row[1]);
                            }
                            mysql_free_result(rep_res);
                        }
                    }
                }
            }

            Json::Value list(Json::arrayValue);
            for (const auto& r : replies)
            {
                Json::Value item;
                item["id"] = Json::UInt64(r.id);
                item["solution_id"] = Json::UInt64(r.solution_id);
                item["user_id"] = r.user_id;
                item["content"] = r.content;
                item["is_edited"] = r.is_edited;
                item["created_at"] = r.created_at;
                item["updated_at"] = r.updated_at;
                item["parent_id"] = Json::UInt64(r.parent_id);
                item["reply_to_user_id"] = r.reply_to_user_id;
                item["reply_to_user_name"] = r.reply_to_user_name;
                item["like_count"] = r.like_count;
                item["favorite_count"] = r.favorite_count;
                item["author_name"] = r.author_name;
                item["author_avatar"] = "/pictures/head.jpg";
                item["reply_count"] = reply_counts.count(r.id) ? reply_counts[r.id] : 0;
                list.append(item);
            }
            (*result)["comments"] = list;
            return true;
        }

        // Comments: edit a comment (owner only)
        bool EditComment(unsigned long long comment_id, const User& current_user,
                         const std::string& content, Json::Value* result, std::string* err_code)
        {
            if (result == nullptr || err_code == nullptr)
            {
                return false;
            }
            std::string trimmed = TrimSpace(content);
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
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "NOT_FOUND";
                return false;
            }
            if (c.user_id != current_user.uid)
            {
                *err_code = "FORBIDDEN";
                return false;
            }

            if (!_model.UpdateComment(comment_id, current_user.uid, trimmed))
            {
                *err_code = "UPDATE_FAILED";
                return false;
            }

            Comment updated;
            if (_model.GetCommentById(comment_id, &updated))
            {
                (*result)["success"] = true;
                (*result)["id"] = Json::UInt64(updated.id);
                (*result)["content"] = updated.content;
                (*result)["is_edited"] = updated.is_edited;
                (*result)["updated_at"] = updated.updated_at;
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
            Comment c;
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "NOT_FOUND";
                return false;
            }
            bool is_admin = _model.GetAdminByUid(current_user.uid);
            if (c.user_id != current_user.uid && !is_admin)
            {
                *err_code = "FORBIDDEN";
                return false;
            }
            if (!_model.DeleteComment(comment_id, current_user.uid, is_admin))
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
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "COMMENT_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleCommentAction(comment_id, current_user.uid, "like", &now_active, &new_count))
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
            if (!_model.GetCommentById(comment_id, &c))
            {
                *err_code = "COMMENT_NOT_FOUND";
                return false;
            }

            bool now_active = false;
            unsigned int new_count = 0;
            if (!_model.ToggleCommentAction(comment_id, current_user.uid, "favorite", &now_active, &new_count))
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
            if (!_model.GetCommentActions(comment_ids, user_id, &actions))
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
