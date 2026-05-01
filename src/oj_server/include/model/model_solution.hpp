#pragma once

#include "model_base.hpp"

namespace ns_model
{
    class ModelSolution : public ModelBase
    {
    public:
        std::string SolutionStatusToDbString(SolutionStatus status)
        {
            switch (status)
            {
                case SolutionStatus::pending:
                    return "pending";
                case SolutionStatus::approved:
                    return "approved";
                case SolutionStatus::rejected:
                    return "rejected";
                default:
                    return "approved";
            }
        }

        SolutionStatus DbStringToSolutionStatus(const std::string& s)
        {
            if (s == "pending") return SolutionStatus::pending;
            if (s == "rejected") return SolutionStatus::rejected;
            return SolutionStatus::approved;
        }

        const std::string& SolutionActionsTable() const
        {
            static const std::string table = "solution_actions";
            return table;
        }

        bool CreateSolution(const Solution& input, unsigned long long* solution_id = nullptr)
        {
            if (input.question_id.empty() || input.user_id <= 0)
            {
                return false;
            }

            std::string trimmed_title = TrimCopy(input.title);
            std::string trimmed_content = TrimCopy(input.content_md);
            if (trimmed_title.empty() || trimmed_content.empty())
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_question_id = EscapeSqlString(input.question_id, my.get());
            std::string safe_title = EscapeSqlString(trimmed_title, my.get());
            std::string safe_content = EscapeSqlString(trimmed_content, my.get());
            std::string safe_status = EscapeSqlString(SolutionStatusToDbString(input.status), my.get());

            std::ostringstream sql;
            sql << "insert into " << SolutionsTable()
                << " (question_id, user_id, title, content_md, like_count, favorite_count, comment_count, status, created_at, updated_at) values ('"
                << safe_question_id << "', "
                << input.user_id << ", '"
                << safe_title << "', '"
                << safe_content << "', 0, 0, 0, '"
                << safe_status << "', NOW(), NOW())";

            if (mysql_query(my.get(), sql.str().c_str()) != 0)
            {
                logger(ns_log::FATAL) << "MySql执行错误! errno=" << mysql_errno(my.get())
                                      << " error=" << mysql_error(my.get());
                return false;
            }

            if (solution_id != nullptr)
            {
                *solution_id = static_cast<unsigned long long>(mysql_insert_id(my.get()));
            }
            // Invalidate solution list cache for this question (multiple page/size combos)
            const int pages[] = {1, 2, 3};
            const int sizes[] = {10, 20, 50};
            const std::string sorts[] = {"1", "1"};  // version key for latest and hot
            const SolutionSort sort_types[] = {SolutionSort::latest, SolutionSort::hot};

            for (int p : pages) {
                for (int s : sizes) {
                    for (int t = 0; t < 2; ++t) {
                        auto key = _cache.BuildSolutionCacheKey(input.question_id, p, s, sorts[t],
                            Cache::CacheKey::PageType::kList, SolutionStatus::approved, sort_types[t]);
                        _cache.DeleteStringByAnyKey(key->GetCacheKeyString(&_cache));
                    }
                }
            }

            logger(ns_log::INFO) << "Invalidated solution list cache for question " << input.question_id;

            return true;
        }
        //获取分页的题解列表
        bool GetSolutionsByPage(const std::string& question_id,
                                const std::string& status_filter,
                                const std::string& sort_order,
                                int page,
                                int size,
                                std::vector<Solution>* solutions,
                                int* total_count,
                                int* total_pages)
        {
            if (solutions == nullptr || total_count == nullptr || total_pages == nullptr)
            {
                return false;
            }

            //构造cache key
            SolutionStatus status = SolutionStatus::approved;
            if (status_filter == "pending")
                status = SolutionStatus::pending;
            else if (status_filter == "rejected")
                status = SolutionStatus::rejected;

            SolutionSort sort = SolutionSort::latest;
            if (sort_order == "hot")
                sort = SolutionSort::hot;

            auto cache_key = _cache.BuildSolutionCacheKey(question_id, page, size, "1",
                                                           Cache::CacheKey::PageType::kList,
                                                           status, sort);

            // Try to read from cache
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), &cached_json))
            {
                // Cache hit: parse JSON and populate results
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    if (json_value.isMember("total"))
                        *total_count = json_value["total"].asInt();
                    if (json_value.isMember("total_pages"))
                        *total_pages = json_value["total_pages"].asInt();

                    if (json_value.isMember("solutions") && json_value["solutions"].isArray())
                    {
                        for (const auto& item : json_value["solutions"])
                        {
                            Solution s;
                            s.id = item["id"].asUInt64();
                            s.question_id = item["question_id"].asString();
                            s.user_id = item["user_id"].asInt();
                            s.title = item["title"].asString();
                            s.content_md = item["content_md"].asString();
                            s.like_count = item["like_count"].asUInt();
                            s.favorite_count = item["favorite_count"].asUInt();
                            s.comment_count = item["comment_count"].asUInt();
                            s.status = DbStringToSolutionStatus(item["status"].asString());
                            s.created_at = item["created_at"].asString();
                            s.updated_at = item["updated_at"].asString();
                            solutions->push_back(s);
                        }
                    }
                    logger(ns_log::INFO) << "Cache hit for solution list " << cache_key->GetCacheKeyString(&_cache);
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string safe_status = EscapeSqlString(status_filter, my.get());

            std::string where_clause = " where question_id='" + safe_qid + "'";
            if (!safe_status.empty())
            {
                where_clause += " and status='" + safe_status + "'";
            }

            std::string count_sql = "select count(*) from " + SolutionsTable() + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                return false;
            }

            if (*total_count <= 0)
            {
                *total_pages = 0;
                solutions->clear();
                return true;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 50));
            *total_pages = (*total_count + safe_size - 1) / safe_size;
            int safe_page_clamped = std::min(safe_page, *total_pages);
            int offset = (safe_page_clamped - 1) * safe_size;

            std::string order_clause = " order by id asc";
            if (sort_order == "hot")
            {
                order_clause = " order by like_count desc, id asc";
            }

            std::ostringstream page_sql;
            page_sql << "select id, question_id, user_id, title, content_md, like_count, favorite_count, comment_count, status, created_at, updated_at from "
                      << SolutionsTable()
                      << where_clause
                      << order_clause
                      << " limit " << safe_size << " offset " << offset;

            MYSQL_RES* res = nullptr;
            res = QueryMySql(my.get(), page_sql.str(), "MySql题解列表查询错误");
            if (!res) return false;

            int rows = mysql_num_rows(res);
            solutions->clear();
            solutions->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;

                Solution s;
                try
                {
                    s.id = (row[0] != nullptr) ? std::stoull(row[0]) : 0;
                }
                catch (const std::exception&) { s.id = 0; }
                s.question_id = (row[1] != nullptr) ? row[1] : "";
                s.user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
                s.title = (row[3] != nullptr) ? row[3] : "";
                s.content_md = (row[4] != nullptr) ? row[4] : "";
                s.like_count = (row[5] != nullptr) ? static_cast<unsigned int>(std::atoi(row[5])) : 0;
                s.favorite_count = (row[6] != nullptr) ? static_cast<unsigned int>(std::atoi(row[6])) : 0;
                s.comment_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
                s.status = (row[8] != nullptr) ? DbStringToSolutionStatus(row[8]) : SolutionStatus::approved;
                s.created_at = (row[9] != nullptr) ? row[9] : "";
                s.updated_at = (row[10] != nullptr) ? row[10] : "";
                solutions->push_back(s);
            }

            mysql_free_result(res);

            // Write to cache
            Json::Value root(Json::objectValue);
            root["total"] = *total_count;
            root["total_pages"] = *total_pages;
            Json::Value array_value(Json::arrayValue);
            for (const auto& s : *solutions)
            {
                Json::Value item;
                item["id"] = static_cast<Json::UInt64>(s.id);
                item["question_id"] = s.question_id;
                item["user_id"] = s.user_id;
                item["title"] = s.title;
                item["content_md"] = s.content_md;
                item["like_count"] = s.like_count;
                item["favorite_count"] = s.favorite_count;
                item["comment_count"] = s.comment_count;
                item["status"] = SolutionStatusToDbString(s.status);
                item["created_at"] = s.created_at;
                item["updated_at"] = s.updated_at;
                array_value.append(item);
            }
            root["solutions"] = array_value;
            Json::FastWriter writer;
            std::string json_str = writer.write(root);
            _cache.SetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), json_str,
                                     _cache.BuildJitteredTtl(600, 120));
            logger(ns_log::INFO) << "Cache miss for solution list, written to cache " << cache_key->GetCacheKeyString(&_cache);

            return true;
        }

        bool GetSolutionById(long long solution_id, Solution* solution)
        {
            if (solution == nullptr)
            {
                return false;
            }

            //构造cache key
            auto cache_key = _cache.BuildSolutionDetailCacheKey(solution_id);

            // Try to read from cache
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), &cached_json))
            {
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    solution->id = json_value["id"].asUInt64();
                    solution->question_id = json_value["question_id"].asString();
                    solution->user_id = json_value["user_id"].asInt();
                    solution->title = json_value["title"].asString();
                    solution->content_md = json_value["content_md"].asString();
                    solution->like_count = json_value["like_count"].asUInt();
                    solution->favorite_count = json_value["favorite_count"].asUInt();
                    solution->comment_count = json_value["comment_count"].asUInt();
                    solution->status = DbStringToSolutionStatus(json_value["status"].asString());
                    solution->created_at = json_value["created_at"].asString();
                    solution->updated_at = json_value["updated_at"].asString();
                    logger(ns_log::INFO) << "Cache hit for solution detail " << cache_key->GetCacheKeyString(&_cache);
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::ostringstream sql;
            sql << "select id, question_id, user_id, title, content_md, like_count, favorite_count, comment_count, status, created_at, updated_at from "
                << SolutionsTable() << " where id=" << solution_id;

            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql题解详情查询错误");
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

            solution->id = 0;
            try
            {
                if (row[0] != nullptr) solution->id = std::stoull(row[0]);
            }
            catch (const std::exception&) { solution->id = 0; }
            solution->question_id = (row[1] != nullptr) ? row[1] : "";
            solution->user_id = (row[2] != nullptr) ? std::atoi(row[2]) : 0;
            solution->title = (row[3] != nullptr) ? row[3] : "";
            solution->content_md = (row[4] != nullptr) ? row[4] : "";
            solution->like_count = (row[5] != nullptr) ? static_cast<unsigned int>(std::atoi(row[5])) : 0;
            solution->favorite_count = (row[6] != nullptr) ? static_cast<unsigned int>(std::atoi(row[6])) : 0;
            solution->comment_count = (row[7] != nullptr) ? static_cast<unsigned int>(std::atoi(row[7])) : 0;
            solution->status = (row[8] != nullptr) ? DbStringToSolutionStatus(row[8]) : SolutionStatus::approved;
            solution->created_at = (row[9] != nullptr) ? row[9] : "";
            solution->updated_at = (row[10] != nullptr) ? row[10] : "";

            mysql_free_result(res);

            // Write to cache
            Json::Value json_value;
            json_value["id"] = static_cast<Json::UInt64>(solution->id);
            json_value["question_id"] = solution->question_id;
            json_value["user_id"] = solution->user_id;
            json_value["title"] = solution->title;
            json_value["content_md"] = solution->content_md;
            json_value["like_count"] = solution->like_count;
            json_value["favorite_count"] = solution->favorite_count;
            json_value["comment_count"] = solution->comment_count;
            json_value["status"] = SolutionStatusToDbString(solution->status);
            json_value["created_at"] = solution->created_at;
            json_value["updated_at"] = solution->updated_at;
            Json::FastWriter writer;
            std::string json_str = writer.write(json_value);
            _cache.SetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), json_str,
                                     _cache.BuildJitteredTtl(600, 120));
            logger(ns_log::INFO) << "Cache miss for solution detail, written to cache " << cache_key->GetCacheKeyString(&_cache);

            return true;
        }

        bool ToggleSolutionAction(long long solution_id, int user_id, const std::string& action_type,
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

            std::ostringstream check_sql;
            check_sql << "select id from " << SolutionActionsTable()
                      << " where solution_id=" << solution_id
                      << " and user_id=" << user_id
                      << " and action_type='" << safe_action << "'";

            MYSQL_RES* res = QueryMySql(my.get(), check_sql.str(), "MySql查询交互记录错误");
            if (!res) return false;

            int exist_rows = mysql_num_rows(res);
            mysql_free_result(res);

            if (exist_rows > 0)
            {
                std::ostringstream del_sql;
                del_sql << "delete from " << SolutionActionsTable()
                        << " where solution_id=" << solution_id
                        << " and user_id=" << user_id
                        << " and action_type='" << safe_action << "'";

                if (mysql_query(my.get(), del_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql删除交互记录错误!";
                    return false;
                }

                std::ostringstream dec_sql;
                dec_sql << "update " << SolutionsTable()
                        << " set " << count_column << "=" << count_column << "-1"
                        << " where id=" << solution_id;

                if (mysql_query(my.get(), dec_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新计数错误!";
                    return false;
                }

                *now_active = false;
            }
            else
            {
                std::ostringstream ins_sql;
                ins_sql << "insert into " << SolutionActionsTable()
                        << " (solution_id, user_id, action_type, created_at) values ("
                        << solution_id << ", " << user_id << ", '" << safe_action << "', NOW())";

                if (mysql_query(my.get(), ins_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql插入交互记录错误!";
                    return false;
                }

                std::ostringstream inc_sql;
                inc_sql << "update " << SolutionsTable()
                        << " set " << count_column << "=" << count_column << "+1"
                        << " where id=" << solution_id;

                if (mysql_query(my.get(), inc_sql.str().c_str()) != 0)
                {
                    logger(ns_log::FATAL) << "MySql更新计数错误!";
                    return false;
                }

                *now_active = true;
            }

            std::ostringstream cnt_sql;
            cnt_sql << "select " << count_column << " from " << SolutionsTable() << " where id=" << solution_id;

            MYSQL_RES* cnt_res = QueryMySql(my.get(), cnt_sql.str(), "MySql查询计数错误");
            if (!cnt_res) return false;

            MYSQL_ROW cnt_row = mysql_fetch_row(cnt_res);
            if (cnt_row == nullptr || cnt_row[0] == nullptr)
            {
                mysql_free_result(cnt_res);
                return false;
            }

                *new_count = static_cast<unsigned int>(std::atoi(cnt_row[0]));
                mysql_free_result(cnt_res);

            // Invalidate solution detail cache since like/favorite count changed
            auto detail_key = _cache.BuildSolutionDetailCacheKey(solution_id);
            _cache.DeleteStringByAnyKey(detail_key->GetCacheKeyString(&_cache));
            logger(ns_log::INFO) << "Invalidated solution detail cache after action toggle for solution " << solution_id;

                return true;
        }

        bool GetUserActionsForSolutions(int user_id, const std::vector<long long>& solution_ids,
                                        std::map<long long, std::map<std::string, bool>>& actions)
        {
            if (solution_ids.empty())
            {
                return true;
            }

            auto my = CreateConnection();
            if (!my)
            {
                return false;
            }

            std::ostringstream ids_stream;
            for (size_t i = 0; i < solution_ids.size(); ++i)
            {
                if (i > 0) ids_stream << ",";
                ids_stream << solution_ids[i];
            }

            std::ostringstream sql;
            sql << "select solution_id, action_type from " << SolutionActionsTable()
                << " where user_id=" << user_id
                << " and solution_id in (" << ids_stream.str() << ")";

            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql查询用户交互记录错误");
            if (!res) return false;

            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                long long sid = 0;
                try { sid = std::stoll(row[0]); } catch (const std::exception&) { sid = 0; }
                std::string atype = row[1] ? row[1] : "";
                actions[sid][atype] = true;
            }

            mysql_free_result(res);
            return true;
        }

    };

}
