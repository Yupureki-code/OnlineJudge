#pragma once

#include "model_base.hpp"

namespace ns_model
{

    class ModelComment : public ModelBase
    {
    public:
        //获取顶级评论(带分页)
        bool GetCommentsBySolutionId(unsigned long long solution_id, int page, int size,
                                     std::vector<Comment>* comments, int* total_count)
        {
            if (comments == nullptr || total_count == nullptr)
            {
                return false;
            }
            //构建cache_key
            std::string cache_key = "comment:list:sid:" + std::to_string(solution_id)
                                  + ":page:" + std::to_string(page)
                                  + ":size:" + std::to_string(size);
            //访问缓存
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key, &cached_json))
            {
                //缓存命中:解析JSON
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    if (json_value.isMember("total"))
                        *total_count = json_value["total"].asInt();

                    if (json_value.isMember("comments") && json_value["comments"].isArray())
                    {
                        for (const auto& item : json_value["comments"])
                        {
                            Comment c;
                            c.id = item["id"].asUInt64();
                            c.solution_id = item["solution_id"].asUInt64();
                            c.user_id = item["user_id"].asInt();
                            c.content = item["content"].asString();
                            c.is_edited = item["is_edited"].asBool();
                            c.parent_id = item["parent_id"].asUInt64();
                            c.reply_to_user_id = item["reply_to_user_id"].asInt();
                            c.like_count = item["like_count"].asUInt();
                            c.favorite_count = item["favorite_count"].asUInt();
                            c.created_at = item["created_at"].asString();
                            c.updated_at = item["updated_at"].asString();
                            c.reply_to_user_name = item["reply_to_user_name"].asString();
                            c.author_name = item.get("author_name", "").asString();
                            comments->push_back(c);
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for comment list " << cache_key;
                    return true;
                }
            }
            //缓存未命中->回源MySQL
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }
            //检查page和size
            int safe_page = std::max(1, page);
            int safe_size = std::max(1, size);
            int offset = (safe_page - 1) * safe_size;
            //检查MySQL表
            std::string count_sql = "select count(*) from solution_comments where solution_id=" + std::to_string(solution_id) + " and (parent_id IS NULL OR parent_id = 0)";
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }
            if (*total_count <= 0)
            {
                comments->clear();
                return true;
            }

            std::ostringstream sql;
            //查询MySQL
            sql << "select c.id, c.solution_id, c.user_id, c.content, c.is_edited, c.parent_id, c.reply_to_user_id, c.like_count, c.favorite_count, c.created_at, c.updated_at, u.name AS author_name, ru.name AS reply_to_user_name "
                << "from solution_comments c LEFT JOIN users u ON c.user_id = u.uid LEFT JOIN users ru ON c.reply_to_user_id = ru.uid "
                << "where c.solution_id=" << solution_id
                << " and (c.parent_id IS NULL OR c.parent_id = 0)"
                << " ORDER BY c.created_at ASC LIMIT " << safe_size << " OFFSET " << offset;

            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql查询题解评论错误");
            if (!res) return false;
            //逐行解析
            int rows = mysql_num_rows(res);
            comments->clear();
            comments->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr)
                {
                    continue;
                }
                Comment c;
                try
                {
                    c.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                    c.solution_id = (row[1] != nullptr) ? std::stoull(row[1]) : 0;
                }
                catch (const std::exception&) { c.id = 0; c.solution_id = 0; }
                c.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                c.content = (row[3] != nullptr) ? row[3] : "";
                c.is_edited = (row[4] != nullptr) ? (std::atoi(row[4]) != 0) : false;
                c.parent_id = (row[5] != nullptr) ? static_cast<unsigned long long>(std::stoull(row[5])) : 0;
                c.reply_to_user_id = (row[6] != nullptr) ? std::atoi(row[6]) : 0;
                c.like_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
                c.favorite_count = (row[8] != nullptr) ? static_cast<unsigned int>(std::atoi(row[8])) : 0;
                c.created_at = (row[9] != nullptr) ? row[9] : "";
                c.updated_at = (row[10] != nullptr) ? row[10] : "";
                c.reply_to_user_name = (row[12] != nullptr) ? row[12] : "";
                c.author_name = (row[11] != nullptr) ? row[11] : "";
                comments->push_back(c);
            }
            //获取顶级评论的回复数量（嵌套 <= 2 级）
            std::vector<unsigned long long> top_level_ids;
            top_level_ids.reserve(comments->size());
            for (const auto& cc : *comments) {
                if (cc.parent_id == 0) top_level_ids.push_back(cc.id);
            }
            if (!top_level_ids.empty()) {
                std::ostringstream idlist;
                for (size_t i = 0; i < top_level_ids.size(); ++i) {
                    if (i) idlist << ",";
                    idlist << top_level_ids[i];
                }
            }
            mysql_free_result(res);

            //写回Redis
            Json::Value root(Json::objectValue);
            root["total"] = *total_count;
            Json::Value array_value(Json::arrayValue);
            for (const auto& c : *comments)
            {
                Json::Value item;
                item["id"] = static_cast<Json::UInt64>(c.id);
                item["solution_id"] = static_cast<Json::UInt64>(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["parent_id"] = static_cast<Json::UInt64>(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["reply_to_user_name"] = c.reply_to_user_name;
                item["author_name"] = c.author_name;
                array_value.append(item);
            }
            root["comments"] = array_value;
            Json::FastWriter writer;
            std::string json_str = writer.write(root);
            _cache.SetStringByAnyKey(cache_key, json_str, _cache.BuildJitteredTtl(300, 60));
            logger(ns_log::INFO) << "Cache miss for comment list, written to cache " << cache_key;

            return true;
        }

        // Comments: create a new comment
        bool CreateComment(const Comment& comment, unsigned long long* comment_id = nullptr)
        {
            if (comment.solution_id == 0 || comment.user_id <= 0)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_content = EscapeSqlString(comment.content, my.get());
            // if replying to a comment, ensure parent exists and is part of the same solution
            if (comment.parent_id > 0)
            {
                unsigned long long parent_solution_id = 0;
                std::string get_sql = "select solution_id from solution_comments where id=" + std::to_string(comment.parent_id);
                MYSQL_RES* res_parent = QueryMySql(my.get(), get_sql, "MySql查询父评论所属题解错误");
                if (!res_parent) return false;
                MYSQL_ROW row_parent = mysql_fetch_row(res_parent);
                if (row_parent == nullptr || row_parent[0] == nullptr)
                {
                    mysql_free_result(res_parent);
                    return false;
                }
                try
                {
                    parent_solution_id = std::stoull(row_parent[0]);
                }
                catch (const std::exception&){
                    mysql_free_result(res_parent);
                    return false;
                }
                mysql_free_result(res_parent);
                if (parent_solution_id != comment.solution_id)
                {
                    // parent belongs to different solution
                    return false;
                }
            }
            std::ostringstream sql;
            // Insert with optional parent and reply_to fields
            sql << "insert into solution_comments (solution_id, user_id, content, is_edited, parent_id, reply_to_user_id, like_count, favorite_count, created_at, updated_at) values (" 
                << comment.solution_id << ", " << comment.user_id << ", '" << safe_content << "', 0, "
                << comment.parent_id << ", " << comment.reply_to_user_id << ", " << comment.like_count << ", " << comment.favorite_count << ", NOW(), NOW())";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql执行错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }

            if (comment_id != nullptr)
            {
                *comment_id = static_cast<unsigned long long>(mysql_insert_id(my.get()));
            }

            // Get the question_id for cache invalidation
            std::string question_id;
            {
                std::string q_sql = "select question_id from " + SolutionsTable() + " where id=" + std::to_string(comment.solution_id);
                if (mysql_query(my.get(), q_sql.c_str()) == 0)
                {
                    MYSQL_RES* q_res = mysql_store_result(my.get());
                    if (q_res != nullptr)
                    {
                        MYSQL_ROW q_row = mysql_fetch_row(q_res);
                        if (q_row != nullptr && q_row[0] != nullptr)
                        {
                            question_id = q_row[0];
                        }
                        mysql_free_result(q_res);
                    }
                }
            }

            // update comment count on the related solution
            if (comment.parent_id == 0)
            {
                std::ostringstream upd;
                upd << "update " << SolutionsTable() << " set comment_count = comment_count + 1 where id = "
                    << comment.solution_id;
                if (!ExecuteSql(upd.str()))
                {
                    return false;
                }

                // Invalidate solution list cache since comment_count changed
                if (!question_id.empty())
                {
                    auto list_key = _cache.BuildSolutionCacheKey(
                        question_id, 1, 10, "1",
                        Cache::CacheKey::PageType::kList,
                        SolutionStatus::approved, SolutionSort::latest);
                    _cache.DeleteStringByAnyKey(list_key->GetCacheKeyString(&_cache));
                    logger(ns_log::INFO) << "Invalidated solution list cache after comment creation for question " << question_id;
                }
            }

            // Invalidate reply list cache for the parent comment (all common page sizes)
            if (comment.parent_id > 0)
            {
                std::string reply_base = "reply:list:pid:" + std::to_string(comment.parent_id);
                _cache.DeleteStringByAnyKey(reply_base + ":page:1:size:50");
                _cache.DeleteStringByAnyKey(reply_base + ":page:1:size:20");
                _cache.DeleteStringByAnyKey(reply_base + ":page:1:size:1000000");
                logger(ns_log::INFO) << "Invalidated reply list cache for parent " << comment.parent_id;
            }

            // Invalidate comment list cache for this solution
            std::string comment_list_key_prefix = "comment:list:sid:" + std::to_string(comment.solution_id);
            _cache.DeleteStringByAnyKey(comment_list_key_prefix + ":page:1:size:10");
            _cache.DeleteStringByAnyKey(comment_list_key_prefix + ":page:1:size:20");
            _cache.DeleteStringByAnyKey(comment_list_key_prefix + ":page:1:size:50");
            logger(ns_log::INFO) << "Invalidated comment list cache for solution " << comment.solution_id;

            // Pre-warm: try to read and update the comment list cache with the new comment
            if (comment.parent_id == 0)
            {
                std::string warm_key = "comment:list:sid:" + std::to_string(comment.solution_id) + ":page:1:size:20";
                std::string warm_json;
                if (_cache.GetStringByAnyKey(warm_key, &warm_json))
                {
                    Json::CharReaderBuilder builder;
                    Json::Value warm_value;
                    std::istringstream ss(warm_json);
                    if (Json::parseFromStream(builder, ss, &warm_value, nullptr))
                    {
                        Json::Value new_comment;
                        new_comment["id"] = Json::UInt64(comment.id);
                        new_comment["solution_id"] = Json::UInt64(comment.solution_id);
                        new_comment["user_id"] = comment.user_id;
                        new_comment["content"] = comment.content;
                        new_comment["is_edited"] = comment.is_edited;
                        new_comment["parent_id"] = Json::UInt64(comment.parent_id);
                        new_comment["reply_to_user_id"] = comment.reply_to_user_id;
                        new_comment["like_count"] = comment.like_count;
                        new_comment["favorite_count"] = comment.favorite_count;
                        new_comment["created_at"] = comment.created_at;
                        new_comment["updated_at"] = comment.updated_at;
                        new_comment["reply_to_user_name"] = "";
                        if (warm_value.isMember("comments") && warm_value["comments"].isArray())
                        {
                            Json::Value updated_arr(Json::arrayValue);
                            updated_arr.append(new_comment);
                            for (const auto& item : warm_value["comments"])
                            {
                                updated_arr.append(item);
                            }
                            warm_value["comments"] = updated_arr;
                            if (warm_value.isMember("total"))
                            {
                                warm_value["total"] = warm_value["total"].asInt() + 1;
                            }
                            Json::FastWriter writer;
                            _cache.SetStringByAnyKey(warm_key, writer.write(warm_value),
                                                     _cache.BuildJitteredTtl(300, 60));
                            logger(ns_log::INFO) << "Pre-warmed comment list cache for solution " << comment.solution_id;
                        }
                    }
                }
            }

            return true;
        }

        //获取评论
        /// 根据ID获取评论详情（带Redis缓存）
        bool GetCommentById(unsigned long long comment_id, Comment* comment)
        {
            if (comment == nullptr)
            {
                return false;
            }

            // 先查Redis缓存
            std::string cache_key = "comment:detail:" + std::to_string(comment_id);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                Json::Value val;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, &val, nullptr) && val.isMember("id"))
                {
                    comment->id = val["id"].asUInt64();
                    comment->solution_id = val["solution_id"].asUInt64();
                    comment->user_id = val["user_id"].asInt();
                    comment->content = val["content"].asString();
                    comment->is_edited = val["is_edited"].asBool();
                    comment->parent_id = val["parent_id"].asUInt64();
                    comment->reply_to_user_id = val["reply_to_user_id"].asInt();
                    comment->like_count = val["like_count"].asUInt();
                    comment->favorite_count = val["favorite_count"].asUInt();
                    comment->created_at = val["created_at"].asString();
                    comment->updated_at = val["updated_at"].asString();
                    comment->reply_to_user_name = val["reply_to_user_name"].asString();
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::ostringstream sql;
            sql << "select c.id, c.solution_id, c.user_id, c.content, c.is_edited, c.parent_id, c.reply_to_user_id, c.like_count, c.favorite_count, c.created_at, c.updated_at, u.name AS author_name, ru.name AS reply_to_user_name from solution_comments c LEFT JOIN users u ON c.user_id = u.uid LEFT JOIN users ru ON c.reply_to_user_id = ru.uid where c.id="
                << comment_id;

            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql评论详情查询错误");
            if (!res) return false;

            int rows = mysql_num_rows(res);
            if (rows != 1)
            {
                mysql_free_result(res);
                return false;
            }

            MYSQL_ROW row = mysql_fetch_row(res);
            if (row == nullptr)
            {
                mysql_free_result(res);
                return false;
            }
            comment->id = 0;
            comment->solution_id = 0;
            try
            {
                if (row[0] != nullptr) comment->id = std::stoull(row[0]);
                if (row[1] != nullptr) comment->solution_id = std::stoull(row[1]);
            }
            catch (const std::exception&) { comment->id = 0; comment->solution_id = 0; }
            comment->user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
            comment->content = (row[3] != nullptr) ? row[3] : "";
            comment->is_edited = (row[4] != nullptr) ? (std::atoi(row[4]) != 0) : false;
            comment->parent_id = (row[5] != nullptr) ? static_cast<unsigned long long>(std::stoull(row[5])) : 0;
            comment->reply_to_user_id = (row[6] != nullptr) ? std::atoi(row[6]) : 0;
            comment->like_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
            comment->favorite_count = (row[8] != nullptr) ? static_cast<unsigned int>(std::atoi(row[8])) : 0;
            comment->created_at = (row[9] != nullptr) ? row[9] : "";
            comment->updated_at = (row[10] != nullptr) ? row[10] : "";
            comment->reply_to_user_name = (row[12] != nullptr) ? row[12] : "";
            // row[11] is author_name; not stored in Comment struct
            mysql_free_result(res);

            // 写入Redis缓存
            Json::Value cache_val;
            cache_val["id"] = static_cast<Json::UInt64>(comment->id);
            cache_val["solution_id"] = static_cast<Json::UInt64>(comment->solution_id);
            cache_val["user_id"] = comment->user_id;
            cache_val["content"] = comment->content;
            cache_val["is_edited"] = comment->is_edited;
            cache_val["parent_id"] = static_cast<Json::UInt64>(comment->parent_id);
            cache_val["reply_to_user_id"] = comment->reply_to_user_id;
            cache_val["like_count"] = comment->like_count;
            cache_val["favorite_count"] = comment->favorite_count;
            cache_val["created_at"] = comment->created_at;
            cache_val["updated_at"] = comment->updated_at;
            cache_val["reply_to_user_name"] = comment->reply_to_user_name;
            Json::FastWriter writer;
            _cache.SetStringByAnyKey(cache_key, writer.write(cache_val), _cache.BuildJitteredTtl(300, 60));
            return true;
        }

        //获取回复列表
        bool GetCommentReplies(unsigned long long parent_id, int page, int size,
                               std::vector<Comment>* replies, int* total_count)
        {
            if (replies == nullptr || total_count == nullptr)
            {
                return false;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, size);
            int offset = (safe_page - 1) * safe_size;

            // 尝试从缓存获取回复列表
            std::string reply_cache_key = "reply:list:pid:" + std::to_string(parent_id)
                + ":page:" + std::to_string(safe_page) + ":size:" + std::to_string(safe_size);
            std::string cached_reply_json;
            if (_cache.GetStringByAnyKey(reply_cache_key, &cached_reply_json))
            {
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_reply_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    if (json_value.isMember("total"))
                        *total_count = json_value["total"].asInt();
                    if (json_value.isMember("comments") && json_value["comments"].isArray())
                    {
                        for (const auto& item : json_value["comments"])
                        {
                            Comment c;
                            c.id = item["id"].asUInt64();
                            c.solution_id = item["solution_id"].asUInt64();
                            c.user_id = item["user_id"].asInt();
                            c.content = item["content"].asString();
                            c.is_edited = item["is_edited"].asBool();
                            c.parent_id = item["parent_id"].asUInt64();
                            c.reply_to_user_id = item["reply_to_user_id"].asInt();
                            c.like_count = item["like_count"].asUInt();
                            c.favorite_count = item["favorite_count"].asUInt();
                            c.created_at = item["created_at"].asString();
                            c.updated_at = item["updated_at"].asString();
                            c.reply_to_user_name = item["reply_to_user_name"].asString();
                            c.author_name = item.get("author_name", "").asString();
                            replies->push_back(c);
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for reply list " << reply_cache_key;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            // total count for this parent comment
            std::string count_sql = "select count(*) from solution_comments where parent_id=" + std::to_string(parent_id);
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }
            if (*total_count <= 0)
            {
                replies->clear();
                return true;
            }

            std::ostringstream sql;
            sql << "select c.id, c.solution_id, c.user_id, c.content, c.is_edited, c.parent_id, c.reply_to_user_id, c.like_count, c.favorite_count, c.created_at, c.updated_at, u.name AS author_name, ru.name AS reply_to_user_name "
                << "from solution_comments c LEFT JOIN users u ON c.user_id = u.uid LEFT JOIN users ru ON c.reply_to_user_id = ru.uid "
                << "where c.parent_id=" << parent_id
                << " order by c.created_at ASC LIMIT " << safe_size << " OFFSET " << offset;

            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql查询题解评论回复错误");
            if (!res) return false;
            int rows = mysql_num_rows(res);
            replies->clear();
            replies->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Comment c;
                try
                {
                    c.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                    c.solution_id = (row[1] != nullptr) ? std::stoull(row[1]) : 0;
                }
                catch (const std::exception&) { c.id = 0; c.solution_id = 0; }
                c.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                c.content = (row[3] != nullptr) ? row[3] : "";
                c.is_edited = (row[4] != nullptr) ? (std::atoi(row[4]) != 0) : false;
                c.parent_id = (row[5] != nullptr) ? static_cast<unsigned long long>(std::stoull(row[5])) : 0;
                c.reply_to_user_id = (row[6] != nullptr) ? std::atoi(row[6]) : 0;
                c.like_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
                c.favorite_count = (row[8] != nullptr) ? static_cast<unsigned int>(std::atoi(row[8])) : 0;
                c.created_at = (row[9] != nullptr) ? row[9] : "";
                c.updated_at = (row[10] != nullptr) ? row[10] : "";
                c.reply_to_user_name = (row[12] != nullptr) ? row[12] : "";
                c.author_name = (row[11] != nullptr) ? row[11] : "";
                replies->push_back(c);
            }
            mysql_free_result(res);

            // 序列化回复列表并写入缓存
            Json::Value cache_value;
            cache_value["total"] = *total_count;
            Json::Value reply_arr(Json::arrayValue);
            for (const auto& c : *replies)
            {
                Json::Value item;
                item["id"] = Json::UInt64(c.id);
                item["solution_id"] = Json::UInt64(c.solution_id);
                item["user_id"] = c.user_id;
                item["content"] = c.content;
                item["is_edited"] = c.is_edited;
                item["parent_id"] = Json::UInt64(c.parent_id);
                item["reply_to_user_id"] = c.reply_to_user_id;
                item["like_count"] = c.like_count;
                item["favorite_count"] = c.favorite_count;
                item["created_at"] = c.created_at;
                item["updated_at"] = c.updated_at;
                item["reply_to_user_name"] = c.reply_to_user_name;
                item["author_name"] = c.author_name;
                reply_arr.append(item);
            }
            cache_value["comments"] = reply_arr;
            Json::FastWriter writer;
            _cache.SetStringByAnyKey(reply_cache_key, writer.write(cache_value),
                                     _cache.BuildJitteredTtl(120, 30));
            logger(ns_log::INFO) << "Cache miss for reply list, written to cache " << reply_cache_key;

            return true;
        }

        // Comments: update a comment (ownership check included)
        bool UpdateComment(unsigned long long comment_id, int user_id, const std::string& content)
        {
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_content = EscapeSqlString(content, my.get());
            std::ostringstream sql;
            sql << "update solution_comments set content='" << safe_content
                << "', is_edited = 1, updated_at = NOW() where id = " << comment_id
                << " and user_id = " << user_id;

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql更新评论错误!";
                return false;
            }
            if (mysql_affected_rows(my.get()) == 0)
            {
                return false;
            }
            // 清除评论缓存
            _cache.DeleteStringByAnyKey("comment:detail:" + std::to_string(comment_id));
            return true;
        }

        // Comments: delete a comment (admin override supported) with cascade delete for replies
        bool DeleteComment(unsigned long long comment_id, int user_id, bool is_admin = false)
        {
            // first fetch the related solution_id
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            unsigned long long solution_id = 0;
            unsigned long long deleted_parent_id = 0;
            {
                std::string get_sql = "select solution_id,parent_id from solution_comments where id=" + std::to_string(comment_id);
                MYSQL_RES* res = QueryMySql(my.get(), get_sql, "MySql查询评论所属题解错误");
                if (!res) return false;
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr) {
                    if (row[0] != nullptr) {
                        try { solution_id = std::stoull(row[0]); }
                        catch (const std::exception& e) {
                            logger(ns_log::ERROR) << "stoull failed for solution_id: " << e.what();
                            mysql_free_result(res);
                            return false;
                        }
                    }
                    if (row[1] != nullptr) {
                        try { deleted_parent_id = std::stoull(row[1]); }
                        catch (...) { deleted_parent_id = 0; }
                    }
                }
                mysql_free_result(res);
            }
            if (solution_id == 0)
            {
                return false;
            }

            // count children for cascade delete
            int child_count = 0;
            {
                std::string cnt_sql = "select count(*) from solution_comments where parent_id=" + std::to_string(comment_id);
                if (!QueryCount(cnt_sql, &child_count))
                {
                    return false;
                }
            }

            // delete children first
            std::ostringstream del_children;
            del_children << "delete from solution_comments where parent_id=" << comment_id;
            if (mysql_query(my.get(), del_children.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql删除子评论错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }
            // delete main comment
            std::ostringstream del_sql;
            if (is_admin)
            {
                del_sql << "delete from solution_comments where id=" << comment_id;
            }
            else
            {
                del_sql << "delete from solution_comments where id=" << comment_id << " and user_id=" << user_id;
            }
            if (mysql_query(my.get(), del_sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql删除评论错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }
            // Properly consume the DELETE result to keep the connection clean
            {
                MYSQL_RES* discard = mysql_store_result(my.get());
                if (discard != nullptr)
                {
                    mysql_free_result(discard);
                }
            }

            // Get the question_id for cache invalidation
            std::string question_id;
            {
                std::string q_sql = "select question_id from " + SolutionsTable() + " where id=" + std::to_string(solution_id);
                if (mysql_query(my.get(), q_sql.c_str()) == 0)
                {
                    MYSQL_RES* q_res = mysql_store_result(my.get());
                    if (q_res != nullptr)
                    {
                        MYSQL_ROW q_row = mysql_fetch_row(q_res);
                        if (q_row != nullptr && q_row[0] != nullptr)
                        {
                            question_id = q_row[0];
                        }
                        mysql_free_result(q_res);
                    }
                }
            }

            // Only decrement for top-level comments (replies are not counted)
            if (deleted_parent_id == 0) {
                std::ostringstream upd_sql;
                upd_sql << "update " << SolutionsTable() << " set comment_count = GREATEST(comment_count - 1, 0) where id = "
                        << solution_id;
                if (!ExecuteSql(upd_sql.str()))
                {
                    return false;
                }
            }

            // Invalidate solution list cache since comment_count changed
            if (!question_id.empty())
            {
                auto list_key = _cache.BuildSolutionCacheKey(
                    question_id, 1, 10, "1",
                    Cache::CacheKey::PageType::kList,
                    SolutionStatus::approved, SolutionSort::latest);
                _cache.DeleteStringByAnyKey(list_key->GetCacheKeyString(&_cache));
                logger(ns_log::INFO) << "Invalidated solution list cache after comment deletion for question " << question_id;
            }

            // Invalidate reply cache for the deleted comment's own replies
            _cache.DeleteStringByAnyKey("reply:list:pid:" + std::to_string(comment_id) + ":page:1:size:50");

            return true;
        }

        // Comments: batch get user actions on comments
        bool GetCommentActions(const std::vector<unsigned long long>& comment_ids,
                               int user_id,
                               std::map<unsigned long long, std::map<std::string, bool>>* actions)
        {
            if (actions == nullptr)
            {
                return false;
            }
            if (comment_ids.empty())
            {
                return true;
            }
            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }
            std::ostringstream ids_stream;
            for (size_t i = 0; i < comment_ids.size(); ++i)
            {
                if (i > 0) ids_stream << ",";
                ids_stream << comment_ids[i];
            }
            std::ostringstream sql;
            sql << "select comment_id, action_type from comment_actions where user_id=" << user_id
                << " and comment_id in (" << ids_stream.str() << ")";
            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql查询评论用户交互错误");
            if (!res) return false;
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                unsigned long long cid = 0;
                std::string atype = row[1] ? row[1] : "";
                try {
                    cid = std::stoull(row[0]);
                } catch (...) {
                    cid = 0;
                }
                if (cid != 0) {
                    (*actions)[cid][atype] = true;
                }
            }
            mysql_free_result(res);
            return true;
        }

        // Comments: toggle action on a comment (like/favorite)
        bool ToggleCommentAction(unsigned long long comment_id, int user_id, const std::string& action_type,
                                   bool* now_active, unsigned int* new_count)
        {
            if (now_active == nullptr || new_count == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_action = EscapeSqlString(action_type, my.get());

            std::string count_column;
            if (action_type == "like")
            {
                count_column = "like_count";
            }
            else if (action_type == "favorite")
            {
                count_column = "favorite_count";
            }
            else
            {
                return false;
            }

            // 尝试从缓存获取用户交互状态
            std::string action_cache_key = "action:user:" + std::to_string(user_id) + ":comment:" + std::to_string(comment_id);
            std::string cached_action_json;
            bool cached_exists = false;
            bool cached_found = false;
            if (_cache.GetStringByAnyKey(action_cache_key, &cached_action_json))
            {
                Json::CharReaderBuilder reader;
                Json::Value cv;
                std::istringstream css(cached_action_json);
                if (Json::parseFromStream(reader, css, &cv, nullptr))
                {
                    if (action_type == "like" && cv.isMember("like"))
                    {
                        cached_exists = cv["like"].asBool();
                        cached_found = true;
                    }
                    else if (action_type == "favorite" && cv.isMember("favorite"))
                    {
                        cached_exists = cv["favorite"].asBool();
                        cached_found = true;
                    }
                }
            }

            int exist_rows = 0;
            if (cached_found)
            {
                exist_rows = cached_exists ? 1 : 0;
            }
            else
            {
                std::ostringstream check_sql;
                check_sql << "select id from comment_actions where comment_id=" << comment_id
                          << " and user_id=" << user_id
                          << " and action_type='" << safe_action << "'";

                MYSQL_RES* res = QueryMySql(my.get(), check_sql.str(), "MySql查询评论交互记录错误");
                if (!res) return false;
                exist_rows = mysql_num_rows(res);
                mysql_free_result(res);
            }

            if (exist_rows > 0)
            {
                std::ostringstream del_sql;
                del_sql << "delete from comment_actions where comment_id=" << comment_id
                        << " and user_id=" << user_id
                        << " and action_type='" << safe_action << "'";
                if (mysql_query(my.get(), del_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql删除评论交互记录错误!";
                    return false;
                }

                std::ostringstream dec_sql;
                dec_sql << "update solution_comments set " << count_column << " = " << count_column << " - 1"
                        << " where id = " << comment_id;
                if (mysql_query(my.get(), dec_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新评论计数错误!";
                    return false;
                }

                *now_active = false;
            }
            else
            {
                std::ostringstream ins_sql;
                ins_sql << "insert into comment_actions (comment_id, user_id, action_type, created_at) values ("
                        << comment_id << ", " << user_id << ", '" << safe_action << "', NOW())";
                if (mysql_query(my.get(), ins_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql插入评论交互记录错误!";
                    return false;
                }

                std::ostringstream inc_sql;
                inc_sql << "update solution_comments set " << count_column << " = " << count_column << " + 1"
                        << " where id = " << comment_id;
                if (mysql_query(my.get(), inc_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新评论计数错误!";
                    return false;
                }

                *now_active = true;
            }

            // 更新交互状态缓存
            Json::Value action_state;
            if (!cached_action_json.empty())
            {
                Json::CharReaderBuilder reader;
                std::istringstream css(cached_action_json);
                Json::parseFromStream(reader, css, &action_state, nullptr);
            }
            action_state[action_type] = !*now_active ? false : true;
            Json::FastWriter action_writer;
            _cache.SetStringByAnyKey(action_cache_key, action_writer.write(action_state),
                                     _cache.BuildJitteredTtl(300, 60));

            std::ostringstream cnt_sql;
            cnt_sql << "select " << count_column << " from solution_comments where id = " << comment_id;
            MYSQL_RES* cnt_res = QueryMySql(my.get(), cnt_sql.str(), "MySql查询评论计数错误");
            if (!cnt_res) return false;
            MYSQL_ROW cnt_row = mysql_fetch_row(cnt_res);
            if (cnt_row == nullptr || cnt_row[0] == nullptr)
            {
                mysql_free_result(cnt_res);
                return false;
            }
            *new_count = static_cast<unsigned int>(std::atoi(cnt_row[0]));
            mysql_free_result(cnt_res);
            return true;
        }
    };

}
