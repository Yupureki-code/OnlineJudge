#pragma once

#include "model_base.hpp"
#include <chrono>

namespace ns_model
{
    class ModelUser : public ModelBase
    {
    private:
        //查询一批用户数据
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
                item.create_time = row[3] == nullptr ? "" : row[3];
                item.last_login = row[4] == nullptr ? "" : row[4];
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
            auto begin = std::chrono::steady_clock::now();
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select * from " + oj_users + " where email='" + safe_email + "'";

            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql查询错误");
            if (!res)
            {
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
                return false;
            }
            int rows = mysql_num_rows(res);
            mysql_free_result(res);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return rows > 0;
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email)
        {
            auto begin = std::chrono::steady_clock::now();
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_name = EscapeSqlString(name, my.get());
            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "insert into " + oj_users +
                              " (name, password_hash, email, password_algo) values ('" +
                              safe_name + "', '', '" + safe_email + "', 'none')";
            auto ok = ExecuteSql(sql);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            if (ok)
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, User* user)
        {
            auto begin = std::chrono::steady_clock::now();
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where email='" + safe_email + "'";
            auto ok = QueryUser(sql, user);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        //用户:更新最后登录时间
        bool UpdateLastLogin(const std::string& email)
        {
            auto begin = std::chrono::steady_clock::now();
            auto my = CreateConnection();
            if(!my)
                return false;

            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "update " + oj_users + " set last_login=NOW() where email='" + safe_email + "'";
            auto ok = ExecuteSql(sql);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            if (ok)
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        //用户:通过ID获取用户信息 (with Redis read cache)
        bool GetUserById(int uid, User* user)
        {
            if (user == nullptr) return false;
            auto _metrics_begin = std::chrono::steady_clock::now();

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
                    user->create_time = val["create_time"].asString();
                    user->last_login = val["last_login"].asString();
                    long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - _metrics_begin).count();
                    RecordCacheMetrics(RecordActionType::User, true, false, cost_ms);
                    return true;
                }
            }

            // Cache miss — query MySQL
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where uid=" + std::to_string(uid);
            bool ok = QueryUser(sql, user);
            if (ok) {
                // Write to cache
                Json::Value val;
                val["uid"] = user->uid;
                val["name"] = user->name;
                val["email"] = user->email;
                val["create_time"] = user->create_time;
                val["last_login"] = user->last_login;
                Json::FastWriter writer;
                _cache.SetStringByAnyKey(cache_key, writer.write(val), _cache.BuildJitteredTtl(3600, 600));
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - _metrics_begin).count();
                RecordCacheMetrics(RecordActionType::User, false, true, cost_ms);
            }
            return ok;
        }

        bool UpdateUserName(int uid, const std::string& new_name)
        {
            auto my = CreateConnection();
            if (!my) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_name = EscapeSqlString(new_name, my.get());
            std::ostringstream sql;
            sql << "update " << oj_users << " set name='" << safe_name
                << "' where uid=" << uid;
            bool ok = ExecuteSql(sql.str());
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            if (ok) {
                _cache.DeleteStringByAnyKey("user:id:" + std::to_string(uid));
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            }
            return ok;
        }


        bool GetUserByName(const std::string& name, User* user)
        {
            auto my = CreateConnection();
            if(!my)
                return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_name = EscapeSqlString(name, my.get());
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where name='" + safe_name + "'";
            bool ok = QueryUser(sql, user);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        bool GetUserCount(int* count)
        {
            if (count == nullptr) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string sql = "select count(*) from " + oj_users;
            auto ok = QueryCount(sql, count);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        bool GetUsersPaged(std::shared_ptr<ns_cache::Cache::CacheListKey> key, std::vector<User>* users, int* total_count, int* total_pages)
        {
            if (!users || !total_count || !total_pages) return false;
            auto my = CreateConnection(); if (!my) return false;
            std::ostringstream count_sql;
            count_sql << "select count(*) from " << oj_users;
            auto begin = std::chrono::steady_clock::now();
            if (!QueryCount(count_sql.str(), total_count)) return false;
            *total_pages = (*total_count + key->GetSize() - 1) / key->GetSize();
            if (*total_count <= 0) { users->clear(); return true; }
            int offset = (key->GetPage() - 1) * key->GetSize();
            std::ostringstream sql;
            sql << "select uid,name,email,create_time,last_login from " << oj_users
                << " order by uid desc limit " << key->GetSize() << " offset " << offset;
            MYSQL_RES* res = QueryMySql(my.get(), sql.str(), "分页用户查询错误");
            if (!res)
            {
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
                return false;
            }
            users->clear();
            for (int r = 0; r < mysql_num_rows(res); ++r) {
                MYSQL_ROW row = mysql_fetch_row(res); if (!row) continue;
                User u; u.uid = atoi(row[0]); u.name = row[1] ? row[1] : "";
                u.email = row[2] ? row[2] : "";
                u.create_time = row[3] ? row[3] : ""; u.last_login = row[4] ? row[4] : "";
                users->push_back(u);
            }
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            mysql_free_result(res); return true;
        }

        bool RecordUserSubmit(int user_id, const std::string& question_id, const std::string& result_json, bool is_pass)
        {
            auto my = CreateConnection();
            if (!my)
                return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string safe_json = EscapeSqlString(result_json, my.get());

            std::ostringstream sql;
            sql << "insert into user_submits (user_id, question_id, result_json, is_pass, action_time) values ("
                << user_id << ", '" << safe_qid << "', '" << safe_json << "', " << (is_pass ? 1 : 0) << ", NOW())";
            if (!ExecuteSql(sql.str()))
            {
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
                return false;
            }

            _cache.DeleteStringByAnyKey("submit:user:" + std::to_string(user_id) + ":q:" + question_id);
            _cache.DeleteStringByAnyKey("stats:user:" + std::to_string(user_id));
            _cache.DeleteStringByAnyKey("pass:user:" + std::to_string(user_id) + ":q:" + question_id);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return true;
        }
        //检查该用户是否通过该题目
        //检查该用户是否通过该题目（带Redis缓存）
        bool HasUserPassedQuestion(int user_id, const std::string& question_id)
        {
            std::string cache_key = "pass:user:" + std::to_string(user_id) + ":q:" + question_id;
            std::string cached;
            auto begin = std::chrono::steady_clock::now();
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(ModelBase::RecordActionType::User, true, false, cost_ms);
                return cached == "1";
            }

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
            bool passed = count > 0;
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            // 缓存结果，长期有效（通过的记录不会改变）
            _cache.SetStringByAnyKey(cache_key, passed ? "1" : "0", 
                                     _cache.BuildJitteredTtl(1800, 300));
            return passed;
        }

        // Feature 2.4: Get user submissions for a specific question
        bool GetUserSubmitsByQuestion(int user_id, const std::string& question_id, Json::Value* submits)
        {
            if (submits == nullptr)
                return false;
            auto begin = std::chrono::steady_clock::now();
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
                    long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
                    RecordCacheMetrics(ModelBase::RecordActionType::User, true, false, cost_ms);
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
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return true;
        }

        // Feature 2.6: Get user statistics
        bool GetUserStats(int user_id, Json::Value* stats)
        {
            if (stats == nullptr)
                return false;
            auto begin = std::chrono::steady_clock::now();
            std::string cache_key = "stats:user:" + std::to_string(user_id);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                std::istringstream ss(cached);
                if (Json::parseFromStream(builder, ss, stats, nullptr))
                {
                    logger(ns_log::INFO) << "Cache hit for user stats user=" << user_id;
                    long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
                    RecordCacheMetrics(ModelBase::RecordActionType::User, true, false, cost_ms);
                    return true;
                }
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            // 合并3次 COUNT 为1次查询
            std::ostringstream stats_sql;
            stats_sql << "select count(*) as total_submits, "
                      << "count(distinct case when is_pass=1 then question_id end) as passed_questions, "
                      << "sum(case when is_pass=1 then 1 else 0 end) as passed_submits "
                      << "from user_submits where user_id=" << user_id;
            MYSQL_RES* stats_res = QueryMySql(my.get(), stats_sql.str(), "MySql统计查询错误");
            if (!stats_res) return false;

            MYSQL_ROW stats_row = mysql_fetch_row(stats_res);
            int total_submits = (stats_row && stats_row[0]) ? std::atoi(stats_row[0]) : 0;
            int passed_questions = (stats_row && stats_row[1]) ? std::atoi(stats_row[1]) : 0;
            int passed_submits = (stats_row && stats_row[2]) ? std::atoi(stats_row[2]) : 0;
            mysql_free_result(stats_res);

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
            if (!res)
            {
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
                return false;
            }
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
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
            if (total_submits > 0)
            {
                _cache.SetStringByAnyKey(cache_key, writer.write(*stats), _cache.BuildJitteredTtl(180, 60));
            }
            
            return true;
        }

        bool SetUserPassword(const std::string& email, const std::string& password_hash, const std::string& password_algo)
        {
            auto my = CreateConnection();
            if (!my) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_email = EscapeSqlString(email, my.get());
            std::string safe_hash = EscapeSqlString(password_hash, my.get());
            std::string safe_algo = EscapeSqlString(password_algo, my.get());
            std::string sql = "update " + oj_users + " set password_hash='" + safe_hash +
                              "', password_algo='" + safe_algo +
                              "', password_update_at=NOW() where email='" + safe_email + "'";
            auto ok = ExecuteSql(sql);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        bool GetUserPasswordAuth(const std::string& email, std::string* password_hash, std::string* password_algo)
        {
            auto my = CreateConnection();
            if (!my) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "select password_hash,password_algo from " + oj_users + " where email='" + safe_email + "'";
            auto ok = QueryUserPasswordAuth(sql, password_hash, password_algo);
             long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        bool UpdateUserEmail(const std::string& old_email, const std::string& new_email)
        {
            auto my = CreateConnection();
            if (!my) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_old = EscapeSqlString(old_email, my.get());
            std::string safe_new = EscapeSqlString(new_email, my.get());
            std::string sql = "update " + oj_users + " set email='" + safe_new +
                              "' where email='" + safe_old + "'";
                              
            bool ok = ExecuteSql(sql);
            if (ok) {
                User user;
                if (GetUser(new_email, &user))
                    _cache.DeleteStringByAnyKey("user:id:" + std::to_string(user.uid));
            }
             long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        bool DeleteUserByEmail(const std::string& email)
        {
            auto my = CreateConnection();
            if (!my) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string safe_email = EscapeSqlString(email, my.get());
            std::string sql = "delete from " + oj_users + " where email='" + safe_email + "'";
            auto ok = ExecuteSql(sql);
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            return ok;
        }

        bool GetUsersByIds(const std::set<int>& user_ids, std::map<int, User>* users)
        {
            if (users == nullptr || user_ids.empty()) return false;
            users->clear();
            std::ostringstream idlist;
            int i = 0;
            for (int uid : user_ids) {
                if (i++ > 0) idlist << ",";
                idlist << uid;
            }
            auto my = CreateConnection();
            if (!my) return false;
            auto begin = std::chrono::steady_clock::now();
            std::string sql = "select uid,name,email,create_time,last_login,password_algo from " + oj_users + " where uid in (" + idlist.str() + ")";
            MYSQL_RES* res = QueryMySql(my.get(), sql, "MySql批量用户查询错误");
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(ModelBase::RecordActionType::User, false, true, cost_ms);
            if (!res) return false;
            for (int r = 0; r < mysql_num_rows(res); ++r) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (!row) continue;
                User user;
                user.uid = atoi(row[0]);
                user.name = row[1] ? row[1] : "";
                user.email = row[2] ? row[2] : "";
                user.create_time = row[3] ? row[3] : "";
                user.last_login = row[4] ? row[4] : "";
                (*users)[user.uid] = user;
            }
            mysql_free_result(res);
            return true;
        }
    };

}
