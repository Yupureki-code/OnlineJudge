#pragma once

#include <assert.h>
#include <cstdlib>
#include <vector>
#include <memory>
#include <algorithm>
#include <cctype>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <limits>
#include "../../../comm/comm.hpp"
#include "../oj_cache.hpp"
#include "model_base.hpp"
#include "model_comment.hpp"
#include "model_solution.hpp"
#include "model_submission.hpp"
#include "model_user.hpp"
#include "model_question.hpp"
#include <thread>

// model: 主要用来和数据进行交互，对外提供访问数据的接口

namespace oj::model
{
    using namespace oj::cache;
    //用户属性
    class Model : public ModelBase
    {
    private:
        class DeferredScopedDB
        {
        public:
            DeferredScopedDB& operator=(ns_odb::ScopedDB&& database)
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

        using ODBAdminAccount = oj::db::AdminAccount;
        using ODBAdminAccountQuery = odb::query<ODBAdminAccount>;
        using ODBAdminAccountCount = oj::db::AdminAccountCount;
        using ODBAdminAccountCountQuery = odb::query<ODBAdminAccountCount>;
        using ODBAdminAuditLog = oj::db::AdminAuditLog;
        using ODBAdminAuditLogQuery = odb::query<ODBAdminAuditLog>;
        using ODBAdminAuditLogCount = oj::db::AdminAuditLogCount;
        using ODBAdminAuditLogCountQuery = odb::query<ODBAdminAuditLogCount>;
        using ODBCacheMetricsSnapshot = oj::db::CacheMetricsSnapshot;
        using ODBCacheMetricsSnapshotQuery = odb::query<ODBCacheMetricsSnapshot>;
        using ODBCacheMetricsAggregate = oj::db::CacheMetricsAggregateView;

    public:
        struct CacheMetricsAggregate
        {
            int action_type = 0;
            long long total_requests = 0;
            long long cache_hits = 0;
            long long db_fallbacks = 0;
            long long total_ms = 0;
        };

        ModelComment  _comment;
        ModelSolution _solution;
        ModelSubmission _submission;
        ModelUser     _user;
        ModelQuestion _question;

        ~Model()
        {
            StopMetricsFlushWorker();
        }
        
        ModelComment&  Comment()  { return _comment; }
        ModelSolution& Solution() { return _solution; }
        ModelSubmission& Submission() { return _submission; }
        ModelUser&     User()     { return _user; }
        ModelQuestion& Question() { return _question; }

        const std::string& AdminAccountsTable() const
        {
            static const std::string table = "admin_accounts";
            return table;
        }

        const std::string& AdminAuditLogsTable() const
        {
            static const std::string table = "admin_audit_logs";
            return table;
        }

        // 在cache中获取静态HTML页面
        bool GetHtmlPage(std::string *html,
                         std::shared_ptr<Cache::CacheKey> key)
        {
            return _cache.GetHtmlPage(html, key);
        }
        void SetHtmlPage(std::string *html,
                         std::shared_ptr<Cache::CacheKey> key)
        {
            _cache.SetHtmlPage(html, key);
        }

        bool GetCachedStringByAnyKey(const std::string& key, std::string* value)
        {
            if (value == nullptr)
            {
                return false;
            }
            return _cache.GetStringByAnyKey(key, value);
        }

        bool SetCachedStringByAnyKey(const std::string& key, const std::string& value, int expire_seconds)
        {
            if (expire_seconds <= 0)
            {
                return false;
            }
            return _cache.SetStringByAnyKey(key, value, expire_seconds);
        }

        int ReserveCachedStringByAnyKey(const std::string& key, const std::string& value, int expire_seconds)
        {
            if (key.empty() || expire_seconds <= 0) return -1;
            return _cache.ReserveStringByAnyKey(key, value, expire_seconds);
        }

        bool DeleteCachedStringByAnyKey(const std::string& key)
        {
            if (key.empty())
            {
                return false;
            }
            return _cache.DeleteStringByAnyKey(key);
        }

        int BuildCacheJitteredTtlSeconds(int base_seconds, int jitter_seconds)
        {
            return _cache.BuildJitteredTtl(base_seconds, jitter_seconds);
        }
        void InvalidatePage(std::shared_ptr<Cache::CacheKey> key)
        {
            _cache.InvalidatePage(key);
        }


        // Admin: check if a uid belongs to an admin account
        bool GetAdminByUid(int uid)
        {
            AdminAccount admin;
            return GetAdminByUid(uid, &admin);
        }

        bool GetAdminByUid(int uid, AdminAccount* admin)
        {
            if (admin == nullptr || uid < 0)
                return false;
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByUid.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByUid.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                odb::result<ODBAdminAccount> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByUid.ODBQuery");
                    result = database->query<ODBAdminAccount>(ODBAdminAccountQuery::uid == static_cast<uint32_t>(uid));
                }
                std::vector<ODBAdminAccount> matched;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByUid.ODBLazyIteration");
                    for (const auto& item : result)
                        matched.push_back(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByUid.ODBCommit");
                    transaction->Commit();
                }
                if (matched.size() != 1)
                    return false;
                *admin = matched.front();
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByUid.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin uid query failed: ", e.what());
                return false;
            }
        }

        bool GetAdminByAdminId(int admin_id, AdminAccount* admin)
        {
            if (admin == nullptr || admin_id < 0)
                return false;
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByAdminId.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByAdminId.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                std::unique_ptr<ODBAdminAccount> item;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByAdminId.ODBLoad");
                    item.reset(database->find<ODBAdminAccount>(static_cast<uint32_t>(admin_id)));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByAdminId.ODBCommit");
                    transaction->Commit();
                }
                if (!item)
                    return false;
                *admin = *item;
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetAdminByAdminId.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin id query failed: ", e.what());
                return false;
            }
        }

        bool GetAdminCount(int* count)
        {
            if(count == nullptr)
            {
                return false;
            }
            std::lock_guard<std::mutex> mutation_lock(_admin_mutation_mutex);
            std::string key = "admin_counts";
            std::string value;
            if(_cache.GetStringByAnyKey(key, &value))
            {
                *count = std::atoi(value.c_str());
                LOG_INFO("{}", "Cache hit for admin count");
                return true;
            }
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminCount.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminCount.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                int found = 0;
                {
                    odb::result<ODBAdminAccountCount> result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetAdminCount.ODBQuery");
                        result = database->query<ODBAdminAccountCount>();
                    }
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetAdminCount.ODBLazyIteration");
                        for (const auto& item : result)
                            found = static_cast<int>(item.value);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetAdminCount.ODBCommit");
                    transaction->Commit();
                }
                *count = found;
                _cache.SetStringByAnyKey(key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetAdminCount.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin count failed: ", e.what());
                return false;
            }
        }

        bool GetRoleCount(const std::string& role, int* count)
        {
            if (count == nullptr)
            {
                return false;
            }
            std::lock_guard<std::mutex> mutation_lock(_admin_mutation_mutex);
            const std::string cache_key = "admin_role_count:" + role;
            std::string value;
            if(_cache.GetStringByAnyKey(cache_key, &value))
            {
                *count = std::atoi(value.c_str());
                LOG_INFO("{}{}", "Cache hit for role count of role ", role);
                return true;
            }
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetRoleCount.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetRoleCount.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                int found = 0;
                {
                    odb::result<ODBAdminAccountCount> result_set;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetRoleCount.ODBQuery");
                        result_set = database->query<ODBAdminAccountCount>(ODBAdminAccountCountQuery::role == role);
                    }
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetRoleCount.ODBLazyIteration");
                        for (const auto& item : result_set)
                            found = static_cast<int>(item.value);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetRoleCount.ODBCommit");
                    transaction->Commit();
                }
                *count = found;
                _cache.SetStringByAnyKey(cache_key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetRoleCount.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin role count failed: ", e.what());
                return false;
            }
        }

        bool ListAdminsPaged(int page, int size, const std::string& keyword, std::vector<AdminAccount>* admins, int* total_count)
        {
            if (admins == nullptr || total_count == nullptr)
            {
                return false;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 200));
            long long offset = static_cast<long long>(safe_page - 1) * safe_size;
            std::string trimmed_keyword = TrimCopy(keyword);
            ODBAdminAccountQuery condition(true);
            ODBAdminAccountCountQuery count_condition(true);
            if (!trimmed_keyword.empty() && IsAllDigits(trimmed_keyword))
            {
                char* end = nullptr;
                unsigned long long parsed = std::strtoull(trimmed_keyword.c_str(), &end, 10);
                if (end != nullptr && *end == '\0' && parsed <= std::numeric_limits<uint32_t>::max())
                {
                    condition = ODBAdminAccountQuery::uid == static_cast<uint32_t>(parsed) ||
                                ODBAdminAccountQuery::role.like("%" + trimmed_keyword + "%");
                    count_condition = ODBAdminAccountCountQuery::uid == static_cast<uint32_t>(parsed) ||
                                      ODBAdminAccountCountQuery::role.like("%" + trimmed_keyword + "%");
                }
                else
                {
                    condition = ODBAdminAccountQuery::role.like("%" + trimmed_keyword + "%");
                    count_condition = ODBAdminAccountCountQuery::role.like("%" + trimmed_keyword + "%");
                }
            }
            else if (!trimmed_keyword.empty())
            {
                condition = ODBAdminAccountQuery::role.like("%" + trimmed_keyword + "%");
                count_condition = ODBAdminAccountCountQuery::role.like("%" + trimmed_keyword + "%");
            }
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                {
                    odb::result<ODBAdminAccountCount> count_result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBCountQuery");
                        count_result = database->query<ODBAdminAccountCount>(count_condition);
                    }
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBCountLazyIteration");
                        *total_count = 0;
                        for (const auto& item : count_result)
                            *total_count = static_cast<int>(item.value);
                    }
                }

                condition += "ORDER BY" + ODBAdminAccountQuery::admin_id + "DESC LIMIT" +
                             ODBAdminAccountQuery::_val(static_cast<uint64_t>(safe_size)) + "OFFSET" +
                             ODBAdminAccountQuery::_val(static_cast<uint64_t>(offset));
                admins->clear();
                {
                    odb::result<ODBAdminAccount> result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBQuery");
                        result = database->query<ODBAdminAccount>(condition);
                    }
                    admins->reserve(static_cast<size_t>(safe_size));
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBLazyIteration");
                        for (const auto& item : result)
                            admins->push_back(item);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBCommit");
                    transaction->Commit();
                }
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminsPaged.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin list failed: ", e.what());
                return false;
            }
        }

        bool CreateAdminAccount(int uid, const std::string& password_hash, const std::string& role)
        {
            if (uid < 0)
                return false;
            std::lock_guard<std::mutex> mutation_lock(_admin_mutation_mutex);
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                if (role == "super_admin")
                {
                    odb::result<ODBAdminAccount> existing;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBCheckSuperAdmin");
                        existing = database->query<ODBAdminAccount>(ODBAdminAccountQuery::role == "super_admin");
                    }
                    if (existing.begin() != existing.end())
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBRollback");
                        transaction->Rollback();
                        return false;
                    }
                }
                ODBAdminAccount item{};
                item.uid = static_cast<uint32_t>(uid);
                item.password_hash = password_hash;
                item.role = role;
                item.created_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBPersist");
                    database->persist(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBCommit");
                    transaction->Commit();
                }
                _cache.DeleteStringByAnyKey("admin_counts");
                _cache.DeleteStringByAnyKey("admin_role_count:" + role);
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.CreateAdminAccount.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin create failed: ", e.what());
                return false;
            }
        }

        bool UpdateAdminRole(int admin_id, const std::string& new_role)
        {
            if (admin_id < 0)
                return false;
            std::lock_guard<std::mutex> mutation_lock(_admin_mutation_mutex);
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                std::unique_ptr<ODBAdminAccount> item;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBLoad");
                    item.reset(database->find<ODBAdminAccount>(static_cast<uint32_t>(admin_id)));
                }
                if (!item)
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBCommit");
                    transaction->Commit();
                    return true;
                }
                const std::string old_role = item->role;
                item->role = new_role;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBUpdate");
                    database->update(*item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBCommit");
                    transaction->Commit();
                }
                _cache.DeleteStringByAnyKey("admin_role_count:" + old_role);
                _cache.DeleteStringByAnyKey("admin_role_count:" + new_role);
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.UpdateAdminRole.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin role update failed: ", e.what());
                return false;
            }
        }

        bool DeleteAdminAccount(int admin_id)
        {
            if (admin_id < 0)
                return false;
            std::lock_guard<std::mutex> mutation_lock(_admin_mutation_mutex);
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                std::unique_ptr<ODBAdminAccount> item;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBLoad");
                    item.reset(database->find<ODBAdminAccount>(static_cast<uint32_t>(admin_id)));
                }
                if (!item)
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBCommit");
                    transaction->Commit();
                    return true;
                }
                const std::string old_role = item->role;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBErase");
                    database->erase(*item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBCommit");
                    transaction->Commit();
                }
                _cache.DeleteStringByAnyKey("admin_counts");
                _cache.DeleteStringByAnyKey("admin_role_count:" + old_role);
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.DeleteAdminAccount.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB admin delete failed: ", e.what());
                return false;
            }
        }

        bool InsertAdminAuditLog(const AdminAuditLog& log)
        {
            if (log.operator_admin_id < 0 || log.operator_uid < 0)
                return false;
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertAdminAuditLog.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                {
                    LOG_CRITICAL("{}", "无法连接数据库，无法记录审计日志!");
                    return false;
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertAdminAuditLog.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                ODBAdminAuditLog item{};
                item.request_id = log.request_id;
                item.operator_admin_id = static_cast<uint32_t>(log.operator_admin_id);
                item.operator_uid = static_cast<uint32_t>(log.operator_uid);
                item.operator_role = log.operator_role;
                item.action = log.action;
                item.resource_type = log.resource_type;
                item.result = log.result;
                item.created_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                item.payload_text = log.payload_text;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertAdminAuditLog.ODBPersist");
                    database->persist(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertAdminAuditLog.ODBCommit");
                    transaction->Commit();
                }
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.InsertAdminAuditLog.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB audit insert failed: ", e.what());
                return false;
            }
        }

        bool ListAdminAuditLogsPaged(int page,
                                     int size,
                                     const std::string& action,
                                     int operator_uid,
                                     const std::string& result,
                                     int64_t start_time,
                                     int64_t end_time,
                                     std::vector<AdminAuditLog>* logs,
                                     int* total_count)
        {
            if (logs == nullptr || total_count == nullptr)
            {
                return false;
            }

            int safe_page = std::max(1, page);
            int safe_size = std::max(1, std::min(size, 200));
            long long offset = static_cast<long long>(safe_page - 1) * safe_size;
            const std::string safe_action = TrimCopy(action);
            const std::string safe_result = TrimCopy(result);
            if (operator_uid < 0 || safe_action.size() > 64 || safe_result.size() > 16 ||
                start_time < 0 || end_time < 0 ||
                start_time > std::numeric_limits<int64_t>::max() / 1000000 ||
                end_time > std::numeric_limits<int64_t>::max() / 1000000 ||
                (start_time > 0 && end_time > 0 && start_time > end_time) ||
                (!safe_result.empty() && safe_result != "success" &&
                 safe_result != "denied" && safe_result != "failed"))
                return false;
            ODBAdminAuditLogQuery condition(true);
            ODBAdminAuditLogCountQuery count_condition(true);
            if (!safe_action.empty())
            {
                condition = condition && ODBAdminAuditLogQuery::action == safe_action;
                count_condition = count_condition && ODBAdminAuditLogCountQuery::action == safe_action;
            }
            if (operator_uid > 0)
            {
                condition = condition && ODBAdminAuditLogQuery::operator_uid == static_cast<uint32_t>(operator_uid);
                count_condition = count_condition &&
                                  ODBAdminAuditLogCountQuery::operator_uid == static_cast<uint32_t>(operator_uid);
            }
            if (!safe_result.empty())
            {
                condition = condition && ODBAdminAuditLogQuery::result == safe_result;
                count_condition = count_condition && ODBAdminAuditLogCountQuery::result == safe_result;
            }
            if (start_time > 0)
            {
                const auto start = oj::util::TimeUtil::IntToDateTime(start_time * 1000000);
                condition = condition && ODBAdminAuditLogQuery::created_at >= start;
                count_condition = count_condition && ODBAdminAuditLogCountQuery::created_at >= start;
            }
            if (end_time > 0)
            {
                const auto end = oj::util::TimeUtil::IntToDateTime(end_time * 1000000);
                condition = condition && ODBAdminAuditLogQuery::created_at <= end;
                count_condition = count_condition && ODBAdminAuditLogCountQuery::created_at <= end;
            }
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                {
                    odb::result<ODBAdminAuditLogCount> count_result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBCountQuery");
                        count_result = database->query<ODBAdminAuditLogCount>(count_condition);
                    }
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBCountLazyIteration");
                        *total_count = 0;
                        for (const auto& item : count_result)
                            *total_count = static_cast<int>(item.value);
                    }
                }

                condition += "ORDER BY" + ODBAdminAuditLogQuery::log_id + "DESC LIMIT" +
                             ODBAdminAuditLogQuery::_val(static_cast<uint64_t>(safe_size)) + "OFFSET" +
                             ODBAdminAuditLogQuery::_val(static_cast<uint64_t>(offset));
                logs->clear();
                {
                    odb::result<ODBAdminAuditLog> result_set;
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBQuery");
                        result_set = database->query<ODBAdminAuditLog>(condition);
                    }
                    logs->reserve(static_cast<size_t>(safe_size));
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBLazyIteration");
                        for (const auto& item : result_set)
                            logs->push_back(item);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBCommit");
                    transaction->Commit();
                }
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.ListAdminAuditLogsPaged.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB audit list failed: ", e.what());
                return false;
            }
        }

        bool GetCacheMetricsOneDay(std::vector<CacheMetricsAggregate>* metrics)
        {
            if (metrics == nullptr)
                return false;
            std::lock_guard<std::mutex> flush_lock(_metrics_flush_mutex);
            std::time_t cutoff_time = std::time(nullptr) - 24 * 60 * 60;
            std::tm cutoff_tm{};
            if (localtime_r(&cutoff_time, &cutoff_tm) == nullptr)
                return false;
            oj::db::DateTime cutoff{};
            cutoff.year = static_cast<unsigned int>(cutoff_tm.tm_year + 1900);
            cutoff.month = static_cast<unsigned int>(cutoff_tm.tm_mon + 1);
            cutoff.day = static_cast<unsigned int>(cutoff_tm.tm_mday);
            cutoff.hour = static_cast<unsigned int>(cutoff_tm.tm_hour);
            cutoff.minute = static_cast<unsigned int>(cutoff_tm.tm_min);
            cutoff.second = static_cast<unsigned int>(cutoff_tm.tm_sec);
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetCacheMetricsOneDay.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetCacheMetricsOneDay.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                ODBCacheMetricsSnapshotQuery aggregate_query(
                    ODBCacheMetricsSnapshotQuery::created_at >= cutoff);
                aggregate_query += " GROUP BY `action_type` ORDER BY `action_type`";
                odb::result<ODBCacheMetricsAggregate> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetCacheMetricsOneDay.ODBQuery");
                    result = database->query<ODBCacheMetricsAggregate>(aggregate_query);
                }
                std::map<int, CacheMetricsAggregate> aggregate;
                for (int action_type = 0; action_type < 5; ++action_type)
                    aggregate[action_type].action_type = action_type;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetCacheMetricsOneDay.ODBLazyIteration");
                    for (const auto& item : result)
                    {
                        const int action_type = static_cast<int>(item.action_type);
                        if (action_type < 0 || action_type >= 5)
                            continue;
                        auto& row = aggregate[action_type];
                        SaturatingAdd(&row.total_requests, item.total_requests);
                        SaturatingAdd(&row.cache_hits, item.cache_hits);
                        SaturatingAdd(&row.db_fallbacks, item.db_fallbacks);
                        SaturatingAdd(&row.total_ms, item.total_ms);
                    }
                }
                for (int action_type = 0; action_type < 5; ++action_type)
                {
                    const auto& live = Metrics().actions[action_type];
                    auto& row = aggregate[action_type];
                    SaturatingAdd(&row.total_requests, live.total_requests.load(std::memory_order_relaxed));
                    SaturatingAdd(&row.cache_hits, live.cache_hits.load(std::memory_order_relaxed));
                    SaturatingAdd(&row.db_fallbacks, live.db_fallbacks.load(std::memory_order_relaxed));
                    SaturatingAdd(&row.total_ms, live.total_ms.load(std::memory_order_relaxed));
                }
                metrics->clear();
                metrics->reserve(aggregate.size());
                for (const auto& entry : aggregate)
                    metrics->push_back(entry.second);
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.GetCacheMetricsOneDay.ODBCommit");
                    transaction->Commit();
                }
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.GetCacheMetricsOneDay.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB cache metrics aggregate failed: ", e.what());
                return false;
            }
        }

        //在MySQL中插入缓存记录
        void InsertCacheMetricSnapshot(int action_type, long long total, long long hits, long long falls, long long ms)
        {
            PersistCacheMetricSnapshot(action_type, total, hits, falls, ms);
        }
        //刷新缓存记录
        void FlushCacheMetrics()
        {
            std::lock_guard<std::mutex> flush_lock(_metrics_flush_mutex);
            for (int t = 0; t < 5; ++t)
            {
                auto& m = Metrics().actions[t];
                const long long hits = m.cache_hits.exchange(0, std::memory_order_acq_rel);
                const long long falls = m.db_fallbacks.exchange(0, std::memory_order_acq_rel);
                const long long ms = m.total_ms.exchange(0, std::memory_order_acq_rel);
                const long long req = m.total_requests.exchange(0, std::memory_order_acq_rel);
                if (req == 0 && hits == 0 && falls == 0 && ms == 0)
                    continue;
                if (!PersistCacheMetricSnapshot(t, req, hits, falls, ms))
                {
                    m.total_requests.fetch_add(req, std::memory_order_relaxed);
                    m.cache_hits.fetch_add(hits, std::memory_order_relaxed);
                    m.db_fallbacks.fetch_add(falls, std::memory_order_relaxed);
                    m.total_ms.fetch_add(ms, std::memory_order_relaxed);
                }
            }
        }
        //刷新缓存线程的工作区
        void StartMetricsFlushWorker()
        {
            bool expected = false;
            if (!_metrics_running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
                return;
            try
            {
                _metrics_worker = std::thread([this]() {
                    int waited_seconds = 0;
                    while (_metrics_running.load(std::memory_order_acquire))
                    {
                        std::unique_lock<std::mutex> lock(_metrics_wait_mutex);
                        _metrics_cv.wait_for(lock, std::chrono::seconds(1), [this]() {
                            return !_metrics_running.load(std::memory_order_acquire);
                        });
                        lock.unlock();
                        if (!_metrics_running.load(std::memory_order_acquire))
                            break;
                        ++waited_seconds;
                        bool need_flush = waited_seconds >= 30;
                        for (int t = 0; !need_flush && t < 5; ++t)
                            need_flush = Metrics().actions[t].total_requests.load(std::memory_order_relaxed) >= 100;
                        if (need_flush)
                        {
                            FlushCacheMetrics();
                            waited_seconds = 0;
                        }
                    }
                });
            }
            catch (...)
            {
                _metrics_running.store(false, std::memory_order_release);
                throw;
            }
        }
        //停止缓存刷新的功能
        void StopMetricsFlushWorker()
        {
            const bool was_running = _metrics_running.exchange(false, std::memory_order_acq_rel);
            _metrics_cv.notify_all();
            if (_metrics_worker.joinable())
                _metrics_worker.join();
            if (was_running)
                FlushCacheMetrics();
        }
        //清理MySQL中的旧缓存记录
        void CleanupOldMetrics()
        {
            std::time_t cutoff_time = std::time(nullptr) - 30 * 24 * 60 * 60;
            std::tm cutoff_tm{};
            if (localtime_r(&cutoff_time, &cutoff_tm) == nullptr)
                return;
            oj::db::DateTime cutoff{};
            cutoff.year = static_cast<unsigned int>(cutoff_tm.tm_year + 1900);
            cutoff.month = static_cast<unsigned int>(cutoff_tm.tm_mon + 1);
            cutoff.day = static_cast<unsigned int>(cutoff_tm.tm_mday);
            cutoff.hour = static_cast<unsigned int>(cutoff_tm.tm_hour);
            cutoff.minute = static_cast<unsigned int>(cutoff_tm.tm_min);
            cutoff.second = static_cast<unsigned int>(cutoff_tm.tm_sec);
            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CleanupOldMetrics.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CleanupOldMetrics.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CleanupOldMetrics.ODBErase");
                    database->erase_query<ODBCacheMetricsSnapshot>(ODBCacheMetricsSnapshotQuery::created_at < cutoff);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.CleanupOldMetrics.ODBCommit");
                    transaction->Commit();
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.CleanupOldMetrics.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_WARNING("{}{}", "CleanupOldMetrics failed: ", e.what());
            }
        }

    private:
        static void SaturatingAdd(long long* target, uint64_t value)
        {
            const uint64_t current = static_cast<uint64_t>(*target);
            const uint64_t maximum = static_cast<uint64_t>(std::numeric_limits<long long>::max());
            *target = value > maximum - current
                ? std::numeric_limits<long long>::max()
                : static_cast<long long>(current + value);
        }

        static void SaturatingAdd(long long* target, long long value)
        {
            if (value <= 0)
                return;
            const long long maximum = std::numeric_limits<long long>::max();
            *target = value > maximum - *target ? maximum : *target + value;
        }

        bool PersistCacheMetricSnapshot(int action_type, long long total, long long hits,
                                        long long falls, long long ms)
        {
            if (action_type < 0 || action_type >= 5 || total < 0 || hits < 0 ||
                falls < 0 || ms < 0)
                return false;

            DeferredScopedDB database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertCacheMetricSnapshot.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertCacheMetricSnapshot.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                ODBCacheMetricsSnapshot item{};
                item.action_type = static_cast<uint8_t>(action_type);
                item.total_requests = static_cast<uint64_t>(total);
                item.cache_hits = static_cast<uint64_t>(hits);
                item.db_fallbacks = static_cast<uint64_t>(falls);
                item.total_ms = static_cast<uint64_t>(ms);
                item.created_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());;
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertCacheMetricSnapshot.ODBPersist");
                    database->persist(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "Model.InsertCacheMetricSnapshot.ODBCommit");
                    transaction->Commit();
                }
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.InsertCacheMetricSnapshot.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_WARNING("{}{}", "InsertCacheMetricSnapshot failed: ", e.what());
                return false;
            }
            catch (...)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "Model.InsertCacheMetricSnapshot.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (...) {}
                }
                LOG_WARNING("{}", "InsertCacheMetricSnapshot failed: unknown exception");
                return false;
            }
        }

        std::mutex _admin_mutation_mutex;
        std::atomic<bool> _metrics_running{false};
        std::mutex _metrics_flush_mutex;
        std::mutex _metrics_wait_mutex;
        std::condition_variable _metrics_cv;
        std::thread _metrics_worker;
    };

};
