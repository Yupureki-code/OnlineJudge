#pragma once

#include "model_solution.hpp"

namespace ns_model
{
    class ModelUser : public ModelSolution
    {
    private:
        bool QueryUsers(const std::string& sql, std::vector<User>* users)
        {
            if (users == nullptr)
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql用户列表查询错误");
            if (!res) return false;

            int rows = mysql_num_rows(res);
            users->clear();
            users->reserve(rows);
            for (int i = 0; i < rows; ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr)
                {
                    continue;
                }
                User item;
                item.uid = row[0] == nullptr ? 0 : std::atoi(row[0]);
                item.name = row[1] == nullptr ? "" : row[1];
                item.email = row[2] == nullptr ? "" : row[2];
                item.avatar_path = row[3] == nullptr ? "" : row[3];
                item.create_time = row[4] == nullptr ? "" : row[4];
                item.last_login = row[5] == nullptr ? "" : row[5];
                users->push_back(item);
            }

            mysql_free_result(res);
            return true;
        }

        std::string BuildUserWhereClause(const std::shared_ptr<QueryStruct>& query_hash, MYSQL* my)
        {
            if (!query_hash)
            {
                return "";
            }
            auto uq = std::dynamic_pointer_cast<UserQuery>(query_hash);
            if (!uq)
            {
                return "";
            }

            std::vector<std::string> clauses;
            if (uq->id > 0)
            {
                clauses.push_back("uid=" + std::to_string(uq->id));
            }
            if (!uq->name.empty())
            {
                std::string safe_name = EscapeSqlString(uq->name, my);
                clauses.push_back("name like '%" + safe_name + "%'");
            }
            if (!uq->email.empty())
            {
                std::string safe_email = EscapeSqlString(uq->email, my);
                clauses.push_back("email like '%" + safe_email + "%'");
            }

            if (clauses.empty())
            {
                return "";
            }

            std::ostringstream where;
            where << " where ";
            for (size_t i = 0; i < clauses.size(); ++i)
            {
                if (i != 0)
                {
                    where << " or ";
                }
                where << clauses[i];
            }
            return where.str();
        }

    public:
        //用户:检查用户是否存在
        bool CheckUser(const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select * from " + oj_users + " where email='" + safe_email + "'";

            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql查询错误");
            if (!res) return false;
            int rows = mysql_num_rows(res);
            mysql_free_result(res);
            return rows > 0;
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_name = EscapeSqlString(name, my.get());
            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "insert into " + oj_users +
                              " (name, password_hash, email, password_algo) values ('" +
                              safe_name + "', '', '" + safe_email + "', 'none')";
            return ExecuteSql(sql);
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select uid,name,email,avatar_path,create_time,last_login,password_algo from " + oj_users + " where email='" + safe_email + "'";
            return QueryUser(sql, user);
        }

        //用户:更新最后登录时间
        bool UpdateLastLogin(const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "update " + oj_users + " set last_login=NOW() where email='" + safe_email + "'";
            return ExecuteSql(sql);
        }

        //用户:通过ID获取用户信息 (with Redis read cache)
        bool GetUserById(int uid, User* user)
        {
            if (user == nullptr) return false;

            // Try cache first
            std::string cache_key = "user:id:" + std::to_string(uid);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                Json::Value val;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, &val, nullptr) && val.isMember("uid"))
                {
                    user->uid = val["uid"].asInt();
                    user->name = val["name"].asString();
                    user->email = val["email"].asString();
                    user->avatar_path = val["avatar_path"].asString();
                    user->create_time = val["create_time"].asString();
                    user->last_login = val["last_login"].asString();
                    return true;
                }
            }

            // Cache miss — query MySQL
            std::string sql = "select uid,name,email,avatar_path,create_time,last_login,password_algo from " + oj_users + " where uid=" + std::to_string(uid);
            bool ok = QueryUser(sql, user);
            if (ok) {
                // Write to cache
                Json::Value val;
                val["uid"] = user->uid;
                val["name"] = user->name;
                val["email"] = user->email;
                val["avatar_path"] = user->avatar_path;
                val["create_time"] = user->create_time;
                val["last_login"] = user->last_login;
                Json::FastWriter writer;
                _cache.SetStringByAnyKey(cache_key, writer.write(val), _cache.BuildJitteredTtl(3600, 600));
            }
            return ok;
        }

        bool GetUserCount(int* count)
        {
            if(count == nullptr)
            {
                return false;
            }
            std::string key = "user_counts";
            std::string value;
            if(_cache.GetStringByAnyKey(key, &value))
            {
                *count = std::atoi(value.c_str());
                logger(ns_log::INFO) << "Cache hit for user count";
                return true;
            }
            std::string sql = "select count(*) from " + oj_users;
            if(!QueryCount(sql, count))
            {
                return false;
             }
             _cache.SetStringByAnyKey(key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
             return true;
        }

        bool GetUsersPaged(std::shared_ptr<Cache::CacheListKey> key,std::vector<User>* users, int* total_count,int * total_pages)
        {
            auto begin = std::chrono::steady_clock::now();
            if (users == nullptr || total_count == nullptr || total_pages == nullptr)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }

            if (_cache.GetUsersByPage(key, *users, total_count, total_pages))
            {
                logger(ns_log::INFO) << "Cache hit for user list page " << key->GetCacheKeyString(&_cache);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return true;
            }

            auto my = CreateConnection();
            if (!my)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }

            std::string where_clause = BuildUserWhereClause(key->GetQueryHash(), my.get());

            std::string count_sql = "select count(*) from " + oj_users + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }

            if (*total_count <= 0)
            {
                *total_pages = 0;
                users->clear();
                _cache.SetUsersByPage(key, *users, *total_count, *total_pages);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return true;
            }

            int size = key->GetSize();
            int page = key->GetPage();
            *total_pages = (*total_count + size - 1) / size;
            int safe_page = std::min(page, *total_pages);
            int offset = (safe_page - 1) * size;

            std::ostringstream page_sql;
            page_sql << "select uid,name,email,avatar_path,create_time,last_login from " << oj_users
                     << where_clause
                     << " order by uid desc limit " << size << " offset " << offset;

            if (!QueryUsers(page_sql.str(), users))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }

            auto write_key = _cache.BuildListCacheKey(key->GetQueryHash(), safe_page, size, key->GetListVersion(), Cache::CacheKey::PageType::kData, key->GetListType());
            _cache.SetUsersByPage(write_key, *users, *total_count, *total_pages);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            return true;
        }
        //设置用户密码到MySQL中
        bool SetUserPassword(const std::string& email, const std::string& password_hash, const std::string& password_algo)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string safe_hash = EscapeSqlString(password_hash, my.get());
            std::string safe_algo = EscapeSqlString(password_algo, my.get());
            std::string sql = "update " + oj_users +
                              " set password_hash='" + safe_hash +
                              "', password_algo='" + safe_algo +
                              "', password_update_at=NOW() where email='" + safe_email + "'";
            return ExecuteSql(sql);
        }

        bool GetUserPasswordAuth(const std::string& email, std::string* password_hash, std::string* password_algo)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select password_hash,password_algo from " + oj_users + " where email='" + safe_email + "'";
            return QueryUserPasswordAuth(sql, password_hash, password_algo);
        }

        bool UpdateUserEmail(const std::string& old_email, const std::string& new_email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_old_email = EscapeSqlString(old_email, my.get());
            std::string safe_new_email = EscapeSqlString(new_email, my.get());
            std::string sql = "update " + oj_users +
                              " set email='" + safe_new_email +
                              "' where email='" + safe_old_email + "'";
            return ExecuteSql(sql);
        }

        bool DeleteUserByEmail(const std::string& email)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "delete from " + oj_users + " where email='" + safe_email + "'";
            return ExecuteSql(sql);
        }

        bool UpdateUserAvatar(int uid, const std::string& avatar_path)
        {
            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_path = EscapeSqlString(avatar_path, my.get());
            std::ostringstream sql;
            sql << "update " << oj_users << " set avatar_path='" << safe_path
                << "' where uid=" << uid;
            bool ok = ExecuteSql(sql.str());
            if (ok) {
                // Invalidate user cache so next GetUserById refetches from DB
                _cache.DeleteStringByAnyKey("user:id:" + std::to_string(uid));
            }
            return ok;
        }

        bool GetUserByName(const std::string& name, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_name = EscapeSqlString(name, my.get());
            std::string sql = "select uid,name,email,avatar_path,create_time,last_login,password_algo from " + oj_users + " where name='" + safe_name + "'";
            return QueryUser(sql, user);
        }

        bool RecordUserSubmit(int user_id, const std::string& question_id, const std::string& result_json, bool is_pass)
        {
            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string safe_json = EscapeSqlString(result_json, my.get());

            std::ostringstream sql;
            sql << "insert into user_submits (user_id, question_id, result_json, is_pass, action_time) values ("
                << user_id << ", '" << safe_qid << "', '" << safe_json << "', " << (is_pass ? 1 : 0) << ", NOW())";
            if (!ExecuteSql(sql.str()))
                return false;

            _cache.DeleteStringByAnyKey("submit:user:" + std::to_string(user_id) + ":q:" + question_id);
            _cache.DeleteStringByAnyKey("stats:user:" + std::to_string(user_id));
            return true;
        }
        //检查该用户是否通过该题目
        bool HasUserPassedQuestion(int user_id, const std::string& question_id)
        {
            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::ostringstream sql;
            sql << "select count(*) from user_submits where user_id=" << user_id
                << " and question_id='" << safe_qid << "' and is_pass=1";

            int count = 0;
            if (!QueryCount(sql.str(), &count))
                return false;
            return count > 0;
        }

        // Feature 2.4: Get user submissions for a specific question
        bool GetUserSubmitsByQuestion(int user_id, const std::string& question_id, Json::Value* submits)
        {
            if (submits == nullptr)
                return false;

            // 先查缓存
            std::string cache_key = "submit:user:" + std::to_string(user_id) + ":q:" + question_id;
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                Json::Value cached_val;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, &cached_val, nullptr) && cached_val.isArray())
                {
                    *submits = cached_val;
                    logger(ns_log::INFO) << "Cache hit for user submits user=" << user_id << " q=" << question_id;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::ostringstream sql;
            sql << "select result_json, is_pass, action_time from user_submits"
                << " where user_id=" << user_id
                << " and question_id='" << safe_qid << "'"
                << " order by action_time desc";

            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "MySql查询提交记录错误");
            if (!res) return false;

            submits->clear();
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["result_json"] = row[0] ? row[0] : "";
                item["is_pass"] = (row[1] != nullptr && row[1][0] != 0);
                item["action_time"] = row[2] ? row[2] : "";
                submits->append(item);
            }
            mysql_free_result(res);

            // 回写缓存
            Json::FastWriter writer;
            std::string json_str = writer.write(*submits);
            _cache.SetStringByAnyKey(cache_key, json_str, _cache.BuildJitteredTtl(600, 120));

            return true;
        }

        // Feature 2.6: Get user statistics
        bool GetUserStats(int user_id, Json::Value* stats)
        {
            if (stats == nullptr)
                return false;

            std::string cache_key = "stats:user:" + std::to_string(user_id);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, stats, nullptr))
                {
                    logger(ns_log::INFO) << "Cache hit for user stats user=" << user_id;
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            std::ostringstream count_sql;
            count_sql << "select count(*) from user_submits where user_id=" << user_id;
            int total_submits = 0;
            if (!QueryCount(count_sql.str(), &total_submits))
                return false;

            std::ostringstream pass_sql;
            pass_sql << "select count(distinct question_id) from user_submits where user_id=" << user_id << " and is_pass=1";
            int passed_questions = 0;
            if (!QueryCount(pass_sql.str(), &passed_questions))
                return false;

            std::ostringstream passed_submits_sql;
            passed_submits_sql << "select count(*) from user_submits where user_id=" << user_id << " and is_pass=1";
            int passed_submits = 0;
            if (!QueryCount(passed_submits_sql.str(), &passed_submits))
                return false;

            double accuracy = total_submits > 0 ? static_cast<double>(passed_submits) / static_cast<double>(total_submits) : 0.0;

            (*stats)["total_submits"] = total_submits;
            (*stats)["passed_questions"] = passed_questions;
            (*stats)["passed_submits"] = passed_submits;

            Json::Value accuracy_val(accuracy);
            (*stats)["accuracy"] = accuracy_val;

            std::ostringstream recent_sql;
            recent_sql << "select us.question_id, us.is_pass, us.action_time, us.result_json, q.title, q.star"
                       << " from user_submits us"
                       << " left join " << oj_questions << " q on us.question_id = q.id"
                       << " where us.user_id=" << user_id
                       << " order by us.action_time desc limit 20";

            MYSQL_RES* res = QueryMySql(my.get(), recent_sql.str(), "MySql查询最近提交错误");
            if (!res) return false;

            Json::Value recent_arr(Json::arrayValue);
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["question_id"] = row[0] ? row[0] : "";
                item["is_pass"] = (row[1] != nullptr && row[1][0] != 0);
                item["action_time"] = row[2] ? row[2] : "";
                item["result_json"] = row[3] ? row[3] : "";
                item["title"] = row[4] ? row[4] : "";
                item["star"] = row[5] ? row[5] : "";
                recent_arr.append(item);
            }
            mysql_free_result(res);
            (*stats)["recent_submits"] = recent_arr;

            Json::FastWriter writer;
            // Only cache if there's actual data to prevent stale empty results from persisting
            if (total_submits > 0)
            {
                _cache.SetStringByAnyKey(cache_key, writer.write(*stats), _cache.BuildJitteredTtl(180, 60));
            }

            return true;
        }
    };

}
