#pragma once

#include "comm.hpp"
#include "model_base.hpp"

#include <algorithm>
#include <chrono>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>
#include "../../../comm/models/user.hxx"
#include "../../../comm/models/model_counts.hxx"
#include "../../../comm/models/gen/user-odb.hxx"
#include "../../../comm/models/gen/model_counts-odb.hxx"

namespace oj::model
{
    class ModelUser : public ModelBase
    {
    private:
        class DatabaseHandle
        {
        public:
            DatabaseHandle& operator=(ns_odb::ScopedDB&& database)
            {
                _database = std::make_unique<ns_odb::ScopedDB>(std::move(database));
                return *this;
            }

            odb::database* operator->() { return _database->Get(); }
            odb::database& operator*() { return **_database; }
            odb::database* Get() { return _database ? _database->Get() : nullptr; }

        private:
            std::unique_ptr<ns_odb::ScopedDB> _database;
        };

        static long long ElapsedMs(const std::chrono::steady_clock::time_point& begin)
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
        }

        static bool DateTimeLess(const oj::db::DateTime& lhs, const oj::db::DateTime& rhs)
        {
            return std::tie(lhs.year, lhs.month, lhs.day, lhs.hour, lhs.minute, lhs.second) <
                   std::tie(rhs.year, rhs.month, rhs.day, rhs.hour, rhs.minute, rhs.second);
        }

        static odb::query<oj::db::User> BuildUserQuery(
            const std::shared_ptr<QueryStruct>& query_hash, bool* filtered)
        {
            using Query = odb::query<oj::db::User>;
            Query query;
            *filtered = false;
            const auto user_query = std::dynamic_pointer_cast<UserQuery>(query_hash);
            if (!user_query)
                return query;

            const auto add_clause = [&](const odb::mysql::query_base& clause) {
                query = *filtered ? Query(query || clause) : Query(clause);
                *filtered = true;
            };
            if (user_query->id > 0)
                add_clause(Query::uid == static_cast<uint32_t>(user_query->id));
            if (!user_query->name.empty())
                add_clause(Query::name.like("%" + user_query->name + "%"));
            if (!user_query->email.empty())
                add_clause(Query::email.like("%" + user_query->email + "%"));
            return query;
        }

    public:
        //用户:检查用户是否存在
        bool CheckUser(const std::string& email)
        {
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.CheckUser.ODBBegin");
                    transaction.reset(database->begin());
                }
                bool found = false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.CheckUser.ODBQuery");
                    found = database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == email) != nullptr;
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.CheckUser.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return found;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.CheckUser.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::CheckUser rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::CheckUser ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        //用户:创建新用户
        bool CreateUser(const std::string& name, const std::string& email)
        {
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.CreateUser.ODBBegin");
                    transaction.reset(database->begin());
                }
                const auto now = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                oj::db::User user{};
                user.name = name;
                user.password_hash = "";
                user.email = email;
                user.create_time = now;
                user.last_login = now;
                user.password_algo = "none";
                user.password_update_at = now;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.CreateUser.ODBPersist");
                    database->persist(user);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.CreateUser.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.CreateUser.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::CreateUser rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::CreateUser ODB failure: {}", e.what());
                return false;
            }
        }

        //用户:获取用户信息
        bool GetUser(const std::string& email, User* user)
        {
            if (user == nullptr) return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUser.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUser.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == email));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUser.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                if (!found) return false;
                *user = *found;
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUser.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUser rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUser ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        //用户:更新最后登录时间
        bool UpdateLastLogin(const std::string& email)
        {
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateLastLogin.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateLastLogin.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == email));
                }
                if (found) 
                {
                    found->last_login = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateLastLogin.ODBUpdate");
                        database->update(*found);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateLastLogin.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateLastLogin.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::UpdateLastLogin rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::UpdateLastLogin ODB failure: {}", e.what());
                return false;
            }
        }

        //用户:通过ID获取用户信息 (with Redis read cache)
        bool GetUserById(int uid, User* user)
        {
            if (user == nullptr || uid <= 0) return false;
            const auto begin = std::chrono::steady_clock::now();
            const std::string cache_key = "user:id:" + std::to_string(uid);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached)) {
                Json::CharReaderBuilder builder;
                Json::Value value;
                std::istringstream stream(cached);
                if (Json::parseFromStream(builder, stream, &value, nullptr) && value.isMember("uid")) {
                    user->uid = value["uid"].asInt();
                    user->name = value["name"].asString();
                    user->email = value["email"].asString();
                    user->create_time = oj::util::TimeUtil::IntToDateTime(value["create_time"].asInt64());
                    user->last_login = oj::util::TimeUtil::IntToDateTime(value["last_login"].asInt64());
                    RecordCacheMetrics(RecordActionType::User, true, false, ElapsedMs(begin));
                    return true;
                }
            }

            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserById.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserById.ODBLoad");
                    found.reset(database->find<oj::db::User>(static_cast<uint32_t>(uid)));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserById.ODBCommit");
                    transaction.commit();
                }
                if (!found) return false;
                *user = *found;
                Json::Value value;
                value["uid"] = user->uid;
                value["name"] = user->name;
                value["email"] = user->email;
                value["create_time"] = oj::util::TimeUtil::DateTimeToInt(user->create_time);
                value["last_login"] = oj::util::TimeUtil::DateTimeToInt(user->last_login);
                Json::FastWriter writer;
                _cache.SetStringByAnyKey(cache_key, writer.write(value),
                                         _cache.BuildJitteredTtl(3600, 600));
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserById.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUserById rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUserById ODB failure: {}", e.what());
                return false;
            }
        }

        bool UpdateUserName(int uid, const std::string& new_name)
        {
            if (uid <= 0) return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserName.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserName.ODBLoad");
                    found.reset(database->find<oj::db::User>(static_cast<uint32_t>(uid)));
                }
                if (found) {
                    found->name = new_name;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserName.ODBUpdate");
                        database->update(*found);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserName.ODBCommit");
                    transaction.commit();
                }
                _cache.DeleteStringByAnyKey("user:id:" + std::to_string(uid));
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserName.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::UpdateUserName rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::UpdateUserName ODB failure: {}", e.what());
                return false;
            }
        }

        bool GetUserByName(const std::string& name, User* user)
        {
            if (user == nullptr) return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserByName.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserByName.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::name == name));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserByName.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                if (!found) return false;
                *user = *found;
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserByName.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUserByName rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUserByName ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool GetUserCount(int* count)
        {
            if (count == nullptr) return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserCount.ODBBegin");
                    transaction.reset(database->begin());
                }
                oj::db::UserCount result{};
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserCount.ODBQuery");
                    const odb::query<oj::db::User> query;
                    result = database->query_value<oj::db::UserCount>(query);
                }
                if (result.value > static_cast<uint64_t>(std::numeric_limits<int>::max()))
                    throw std::overflow_error("user count exceeds legacy int range");
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserCount.ODBCommit");
                    transaction.commit();
                }
                *count = static_cast<int>(result.value);
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserCount.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUserCount rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUserCount ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool GetUsersPaged(std::shared_ptr<oj::cache::Cache::CacheListKey> key,
                           std::vector<User>* users, int* total_count, int* total_pages)
        {
            if (!key || !users || !total_count || !total_pages || key->GetSize() <= 0)
                return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersPaged.ODBBegin");
                    transaction.reset(database->begin());
                }
                bool filtered = false;
                const auto query = BuildUserQuery(key->GetQueryHash(), &filtered);
                oj::db::UserCount count_result{};
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersPaged.ODBQueryCount");
                    count_result = database->query_value<oj::db::UserCount>(query);
                }
                if (count_result.value > static_cast<uint64_t>(std::numeric_limits<int>::max()))
                    throw std::overflow_error("user count exceeds legacy int range");

                const int result_count = static_cast<int>(count_result.value);
                const int page_size = key->GetSize();
                const int page = std::max(1, key->GetPage());
                const int result_pages = result_count > 0
                    ? (result_count - 1) / page_size + 1
                    : 0;
                std::vector<User> matched;
                if (page <= result_pages) {
                    const uint64_t offset = static_cast<uint64_t>(page - 1) *
                                            static_cast<uint64_t>(page_size);
                    odb::query<oj::db::User> page_query(query);
                    page_query += " ORDER BY `uid` DESC LIMIT " + std::to_string(page_size) +
                                  " OFFSET " + std::to_string(offset);
                    odb::result<oj::db::User> result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersPaged.ODBQuery");
                        result = database->query<oj::db::User>(page_query, false);
                    }
                    {
                        for (const auto& item : result)
                            matched.push_back(item);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersPaged.ODBCommit");
                    transaction.commit();
                }

                *total_count = result_count;
                *total_pages = result_pages;
                users->clear();
                users->swap(matched);
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersPaged.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUsersPaged rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUsersPaged ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool RecordUserSubmit(int user_id, const std::string& question_id,
                              const std::string& result_json, bool is_pass)
        {
            if (user_id <= 0) return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try 
            {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.RecordUserSubmit.ODBBegin");
                    transaction.reset(database->begin());
                }
                oj::db::LegacySubmission submission{};
                submission.user_id = static_cast<uint32_t>(user_id);
                submission.question_id = question_id;
                submission.result_json = result_json;
                submission.is_pass = is_pass;
                submission.action_time = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.RecordUserSubmit.ODBPersist");
                    database->persist(submission);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.RecordUserSubmit.ODBCommit");
                    transaction.commit();
                }
                _cache.DeleteStringByAnyKey("submit:user:" + std::to_string(user_id) + ":q:" + question_id);
                _cache.DeleteStringByAnyKey("stats:user:" + std::to_string(user_id));
                _cache.DeleteStringByAnyKey("pass:user:" + std::to_string(user_id) + ":q:" + question_id);
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } 
            catch (const std::exception& e) 
            {
                if (!transaction.finalized()) 
                {
                    try 
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.RecordUserSubmit.ODBRollback");
                        transaction.rollback();
                    } 
                    catch (const std::exception& rollback_error) 
                    {
                        LOG_ERROR("ModelUser::RecordUserSubmit rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::RecordUserSubmit ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        //检查该用户是否通过该题目（带Redis缓存）
        bool HasUserPassedQuestion(int user_id, const std::string& question_id)
        {
            if (user_id <= 0) return false;
            const std::string cache_key = "pass:user:" + std::to_string(user_id) + ":q:" + question_id;
            std::string cached;
            const auto begin = std::chrono::steady_clock::now();
            if (_cache.GetStringByAnyKey(cache_key, &cached)) {
                RecordCacheMetrics(RecordActionType::User, true, false, ElapsedMs(begin));
                return cached == "1";
            }

            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.HasUserPassedQuestion.ODBBegin");
                    transaction.reset(database->begin());
                }
                odb::result<oj::db::LegacySubmission> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.HasUserPassedQuestion.ODBQuery");
                    result = database->query<oj::db::LegacySubmission>(
                        odb::query<oj::db::LegacySubmission>::user_id == static_cast<uint32_t>(user_id) &&
                        odb::query<oj::db::LegacySubmission>::question_id == question_id &&
                        odb::query<oj::db::LegacySubmission>::is_pass == true, false);
                }
                bool passed = false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.HasUserPassedQuestion.ODBLazyIteration");
                    for (const auto& ignored : result) {
                        (void)ignored;
                        passed = true;
                        break;
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.HasUserPassedQuestion.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                _cache.SetStringByAnyKey(cache_key, passed ? "1" : "0",
                                         _cache.BuildJitteredTtl(1800, 300));
                return passed;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.HasUserPassedQuestion.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::HasUserPassedQuestion rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::HasUserPassedQuestion ODB failure: {}", e.what());
                return false;
            }
        }

        bool GetUserSubmitsByQuestion(int user_id, const std::string& question_id,
                                      Json::Value* submits)
        {
            if (submits == nullptr || user_id <= 0) return false;
            const auto begin = std::chrono::steady_clock::now();
            const std::string cache_key = "submit:user:" + std::to_string(user_id) + ":q:" + question_id;
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached)) {
                Json::CharReaderBuilder builder;
                Json::Value cached_value;
                std::istringstream stream(cached);
                if (Json::parseFromStream(builder, stream, &cached_value, nullptr) && cached_value.isArray()) {
                    *submits = cached_value;
                    RecordCacheMetrics(RecordActionType::User, true, false, ElapsedMs(begin));
                    LOG_INFO("Cache hit for user submits user={} q={}", user_id, question_id);
                    return true;
                }
            }

            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserSubmitsByQuestion.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::vector<oj::db::LegacySubmission> records;
                odb::result<oj::db::LegacySubmission> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserSubmitsByQuestion.ODBQuery");
                    result = database->query<oj::db::LegacySubmission>(
                        odb::query<oj::db::LegacySubmission>::user_id == static_cast<uint32_t>(user_id) &&
                        odb::query<oj::db::LegacySubmission>::question_id == question_id, false);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserSubmitsByQuestion.ODBLazyIteration");
                    for (const auto& item : result)
                        records.push_back(item);
                }
                std::stable_sort(records.begin(), records.end(), [](const auto& lhs, const auto& rhs) {
                    return DateTimeLess(rhs.action_time, lhs.action_time);
                });
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserSubmitsByQuestion.ODBCommit");
                    transaction.commit();
                }
                submits->clear();
                for (const auto& record : records) {
                    Json::Value item;
                    item["result_json"] = record.result_json;
                    item["is_pass"] = record.is_pass;
                    item["action_time"] = oj::util::TimeUtil::DateTimeToInt(record.action_time);
                    submits->append(item);
                }
                Json::FastWriter writer;
                _cache.SetStringByAnyKey(cache_key, writer.write(*submits),
                                         _cache.BuildJitteredTtl(600, 120));
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserSubmitsByQuestion.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUserSubmitsByQuestion rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUserSubmitsByQuestion ODB failure: {}", e.what());
                return false;
            }
        }
        //获取用户历史统计数据
        bool GetUserStats(int user_id, Json::Value* stats)
        {
            if (stats == nullptr || user_id <= 0) return false;
            const auto begin = std::chrono::steady_clock::now();
            const std::string cache_key = "stats:user:" + std::to_string(user_id);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached)) {
                Json::CharReaderBuilder builder;
                std::istringstream stream(cached);
                if (Json::parseFromStream(builder, stream, stats, nullptr)) {
                    LOG_INFO("Cache hit for user stats user={}", user_id);
                    RecordCacheMetrics(RecordActionType::User, true, false, ElapsedMs(begin));
                    return true;
                }
            }

            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBBegin");
                    transaction.reset(database->begin());
                }
                using SubmissionQuery = odb::query<oj::db::LegacySubmission>;
                const SubmissionQuery submission_query =
                    SubmissionQuery::user_id == static_cast<uint32_t>(user_id);
                oj::db::LegacySubmissionStats aggregate{};
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBQuerySubmissions");
                    aggregate = database->query_value<oj::db::LegacySubmissionStats>(submission_query);
                }

                std::vector<oj::db::LegacySubmission> records;
                odb::result<oj::db::LegacySubmission> submission_result;
                SubmissionQuery recent_query(submission_query);
                recent_query += " ORDER BY `action_time` DESC, `id` DESC LIMIT 20";
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBQueryRecentSubmissions");
                    submission_result = database->query<oj::db::LegacySubmission>(recent_query, false);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBLazyIterationSubmissions");
                    for (const auto& item : submission_result)
                        records.push_back(item);
                }

                std::set<std::string> recent_question_ids;
                for (const auto& record : records)
                    recent_question_ids.insert(record.question_id);
                std::map<std::string, oj::db::Question> questions;
                if (!recent_question_ids.empty()) {
                    using Query = odb::query<oj::db::Question>;
                    Query question_query;
                    bool first = true;
                    for (const auto& id : recent_question_ids) {
                        const Query clause = Query::id == id;
                        question_query = first ? Query(clause) : Query(question_query || clause);
                        first = false;
                    }
                    odb::result<oj::db::Question> question_result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBQueryQuestions");
                        question_result = database->query<oj::db::Question>(question_query, false);
                    }
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBLazyIterationQuestions");
                        for (const auto& question : question_result)
                            questions.emplace(question.id, question);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBCommit");
                    transaction.commit();
                }

                if (aggregate.total_submits > static_cast<uint64_t>(std::numeric_limits<int>::max()) ||
                    aggregate.passed_submits > static_cast<uint64_t>(std::numeric_limits<int>::max()) ||
                    aggregate.passed_questions > static_cast<uint64_t>(std::numeric_limits<int>::max()))
                    throw std::overflow_error("user submission statistics exceed legacy int range");
                const int total_submits = static_cast<int>(aggregate.total_submits);
                const int passed_submit_count = static_cast<int>(aggregate.passed_submits);
                (*stats)["total_submits"] = total_submits;
                (*stats)["passed_questions"] = static_cast<int>(aggregate.passed_questions);
                (*stats)["passed_submits"] = passed_submit_count;
                (*stats)["accuracy"] = total_submits > 0
                    ? static_cast<double>(passed_submit_count) / static_cast<double>(total_submits)
                    : 0.0;

                Json::Value recent(Json::arrayValue);
                for (const auto& record : records) {
                    Json::Value item;
                    item["question_id"] = record.question_id;
                    item["is_pass"] = record.is_pass;
                    item["action_time"] = oj::util::TimeUtil::DateTimeToInt(record.action_time);
                    item["result_json"] = record.result_json;
                    const auto question = questions.find(record.question_id);
                    item["title"] = question == questions.end() ? "" : question->second.title;
                    item["star"] = question == questions.end() ? "" : question->second.star;
                    recent.append(item);
                }
                (*stats)["recent_submits"] = recent;
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                if (total_submits > 0) {
                    Json::FastWriter writer;
                    _cache.SetStringByAnyKey(cache_key, writer.write(*stats),
                                             _cache.BuildJitteredTtl(180, 60));
                }
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserStats.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUserStats rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUserStats ODB failure: {}", e.what());
                return false;
            }
        }

        bool SetUserPassword(const std::string& email, const std::string& password_hash,
                             const std::string& password_algo)
        {
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.SetUserPassword.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.SetUserPassword.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == email));
                }
                if (found) {
                    found->password_hash = password_hash;
                    found->password_algo = password_algo;
                    found->password_update_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.SetUserPassword.ODBUpdate");
                        database->update(*found);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.SetUserPassword.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.SetUserPassword.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::SetUserPassword rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::SetUserPassword ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool GetUserPasswordAuth(const std::string& email, std::string* password_hash,
                                 std::string* password_algo)
        {
            if (password_hash == nullptr || password_algo == nullptr) return false;
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserPasswordAuth.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserPasswordAuth.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == email));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserPasswordAuth.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                if (!found) return false;
                *password_hash = found->password_hash;
                *password_algo = found->password_algo;
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUserPasswordAuth.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUserPasswordAuth rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUserPasswordAuth ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool UpdateUserEmail(const std::string& old_email, const std::string& new_email)
        {
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserEmail.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserEmail.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == old_email));
                }
                uint32_t updated_uid = 0;
                if (found) {
                    updated_uid = found->uid;
                    found->email = new_email;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserEmail.ODBUpdate");
                        database->update(*found);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserEmail.ODBCommit");
                    transaction.commit();
                }
                if (updated_uid != 0)
                    _cache.DeleteStringByAnyKey("user:id:" + std::to_string(updated_uid));
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.UpdateUserEmail.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::UpdateUserEmail rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::UpdateUserEmail ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool DeleteUserByEmail(const std::string& email)
        {
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.DeleteUserByEmail.ODBBegin");
                    transaction.reset(database->begin());
                }
                std::unique_ptr<oj::db::User> found;
                uint32_t deleted_uid = 0;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.DeleteUserByEmail.ODBQuery");
                    found.reset(database->query_one<oj::db::User>(
                        odb::query<oj::db::User>::email == email));
                }
                if (found) {
                    deleted_uid = found->uid;
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.DeleteUserByEmail.ODBErase");
                    database->erase<oj::db::User>(found->uid);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.DeleteUserByEmail.ODBCommit");
                    transaction.commit();
                }
                if (deleted_uid != 0)
                    _cache.DeleteStringByAnyKey("user:id:" + std::to_string(deleted_uid));
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.DeleteUserByEmail.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::DeleteUserByEmail rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::DeleteUserByEmail ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }

        bool GetUsersByIds(const std::set<int>& user_ids, std::map<int, User>* users)
        {
            if (users == nullptr || user_ids.empty()) return false;
            users->clear();
            const auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            odb::transaction transaction;
            try {
                database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersByIds.ODBBegin");
                    transaction.reset(database->begin());
                }
                using Query = odb::query<oj::db::User>;
                Query query;
                bool filtered = false;
                for (const int uid : user_ids) {
                    if (uid <= 0) continue;
                    const Query clause = Query::uid == static_cast<uint32_t>(uid);
                    query = filtered ? Query(query || clause) : clause;
                    filtered = true;
                }
                if (!filtered) {
                    transaction.commit();
                    return true;
                }
                odb::result<oj::db::User> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersByIds.ODBQuery");
                    result = database->query<oj::db::User>(query, false);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersByIds.ODBLazyIteration");
                    for (const auto& item : result) {
                        (*users)[static_cast<int>(item.uid)] = item;
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersByIds.ODBCommit");
                    transaction.commit();
                }
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return true;
            } catch (const std::exception& e) {
                if (!transaction.finalized()) {
                    try {
                        latecyMonitor::Timer timer(Monitor(), "ModelUser.GetUsersByIds.ODBRollback");
                        transaction.rollback();
                    } catch (const std::exception& rollback_error) {
                        LOG_ERROR("ModelUser::GetUsersByIds rollback failure: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("ModelUser::GetUsersByIds ODB failure: {}", e.what());
                RecordCacheMetrics(RecordActionType::User, false, true, ElapsedMs(begin));
                return false;
            }
        }
    };
}
