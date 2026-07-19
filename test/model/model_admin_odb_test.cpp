#include "model/oj_model.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <unistd.h>

namespace
{
    const char* RequiredEnv(const char* name)
    {
        const char* value = std::getenv(name);
        if (value == nullptr || *value == '\0')
            throw std::runtime_error(std::string("missing required environment variable: ") + name);
        return value;
    }

    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }

    struct TestConfig
    {
        std::string host;
        std::string user;
        std::string password;
        std::string database;
        unsigned int port;

        static TestConfig Load()
        {
            TestConfig config{
                RequiredEnv("OJ_ODB_TEST_HOST"), RequiredEnv("OJ_ODB_TEST_USER"),
                RequiredEnv("OJ_ODB_TEST_PASSWORD"), RequiredEnv("OJ_ODB_TEST_DATABASE"), 0
            };
            const unsigned long parsed = std::stoul(RequiredEnv("OJ_ODB_TEST_PORT"));
            if (parsed == 0 || parsed > 65535)
                throw std::runtime_error("invalid OJ_ODB_TEST_PORT");
            config.port = static_cast<unsigned int>(parsed);
            return config;
        }

        void ApplyModelEnvironment() const
        {
            const std::string port_value = std::to_string(port);
            if (::setenv("OJ_DB_HOST", host.c_str(), 1) != 0 ||
                ::setenv("OJ_DB_USER", user.c_str(), 1) != 0 ||
                ::setenv("OJ_DB_PASS", password.c_str(), 1) != 0 ||
                ::setenv("OJ_DB_NAME", database.c_str(), 1) != 0 ||
                ::setenv("OJ_DB_PORT", port_value.c_str(), 1) != 0)
                throw std::runtime_error("failed to set OJ_DB_* environment");
        }

        std::unique_ptr<odb::mysql::database> Connect() const
        {
            return std::make_unique<odb::mysql::database>(
                user, password, database, host, port);
        }
    };

    class LatencyLog
    {
    public:
        LatencyLog(std::string process_name, const std::string& token)
            : process_name_(std::move(process_name)),
              directory_(std::filesystem::temp_directory_path() /
                         (process_name_ + "_latency_" + token)),
              file_(directory_ / (process_name_ + "_latency.csv"))
        {
            const std::string path = directory_.string();
            if (::setenv("OJ_LATENCY_PATH", path.c_str(), 1) != 0)
                throw std::runtime_error("failed to set OJ_LATENCY_PATH");
        }

        ~LatencyLog()
        {
            std::error_code error;
            std::filesystem::remove(file_, error);
            error.clear();
            std::filesystem::remove(directory_, error);
        }

        void Verify(const std::vector<std::string>& labels) const
        {
            std::ifstream input(file_);
            Require(input.is_open(), "latency CSV was not created");
            const std::string contents((std::istreambuf_iterator<char>(input)),
                                       std::istreambuf_iterator<char>());
            Require(contents.find("DBPool.Acquire") != std::string::npos,
                    "latency CSV missed DBPool.Acquire");
            for (const auto& label : labels)
                Require(contents.find(label) != std::string::npos,
                        "latency CSV missed a Model admin label");
        }

    private:
        std::string process_name_;
        std::filesystem::path directory_;
        std::filesystem::path file_;
    };

    void StartModelDatabase(const TestConfig& config)
    {
        config.ApplyModelEnvironment();
        Require(oj::model::ModelBase::StartDatabase("model_admin_odb_test"),
                "failed to start Model admin database context");
    }

    oj::db::User MakeUser(const std::string& name, const std::string& email)
    {
        oj::db::User user{};
        user.name = name;
        user.password_hash = "fixture-hash";
        user.email = email;
        user.create_time = oj::util::TimeUtil::IntToDateTime(
            oj::util::TimeUtil::GetTimeStamp());
        user.last_login = user.create_time;
        user.password_algo = "fixture";
        user.password_update_at = user.create_time;
        return user;
    }

    class Fixture
    {
    public:
        Fixture(const TestConfig& config, std::string token)
            : config_(config), token(std::move(token)),
              first_request_id("admin-a-" + this->token),
              second_request_id("admin-b-" + this->token),
              audit_action("odb.admin." + this->token),
              metric_total(9000000000000ULL +
                  static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count() % 1000000000))
        {
        }

        ~Fixture()
        {
            oj::model::ModelBase::StopDatabase();
            try { Cleanup(); }
            catch (const std::exception& error) {
                std::cerr << "fixture cleanup failed: " << error.what() << '\n';
            }
        }

        void CreateUsers()
        {
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            const std::string suffix = token.substr(token.size() > 16 ? token.size() - 16 : 0);
            auto first = MakeUser("aa" + suffix, "aa_" + token + "@example.test");
            auto second = MakeUser("ab" + suffix, "ab_" + token + "@example.test");
            database->persist(first);
            database->persist(second);
            transaction.commit();
            first_user_id = first.uid;
            second_user_id = second.uid;
        }

        void CreateOldMetric()
        {
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            oj::db::CacheMetricsSnapshot metric{};
            metric.action_type = 1;
            metric.total_requests = metric_total + 3;
            metric.cache_hits = 13;
            metric.db_fallbacks = 9;
            metric.total_ms = 103;
            metric.created_at = oj::db::DateTime{2000, 1, 1, 0, 0, 0};
            database->persist(metric);
            transaction.commit();
            old_metric_id = metric.id;
            metric_ids.insert(metric.id);
        }

        std::set<uint64_t> FindMetricIds() const
        {
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            using Query = odb::query<oj::db::CacheMetricsSnapshot>;
            auto rows = database->query<oj::db::CacheMetricsSnapshot>(
                Query::action_type == static_cast<uint8_t>(4) &&
                Query::total_requests == metric_total && Query::cache_hits == 11 &&
                Query::db_fallbacks == 7 && Query::total_ms == 101);
            std::set<uint64_t> ids;
            for (const auto& row : rows) ids.insert(row.id);
            transaction.commit();
            return ids;
        }

        std::set<uint64_t> FindMetricIds(uint64_t total) const
        {
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            auto rows = database->query<oj::db::CacheMetricsSnapshot>(
                odb::query<oj::db::CacheMetricsSnapshot>::total_requests == total);
            std::set<uint64_t> ids;
            for (const auto& row : rows) ids.insert(row.id);
            transaction.commit();
            return ids;
        }

        bool MetricExists(uint64_t id) const
        {
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            const bool exists = database->find<oj::db::CacheMetricsSnapshot>(id) != nullptr;
            transaction.commit();
            return exists;
        }

        void RemoveAuditLogs()
        {
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            using Query = odb::query<oj::db::AdminAuditLog>;
            database->erase_query<oj::db::AdminAuditLog>(
                Query::request_id == first_request_id || Query::request_id == second_request_id);
            transaction.commit();
        }

        void Cleanup()
        {
            if (cleaned_) return;
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            using AuditQuery = odb::query<oj::db::AdminAuditLog>;
            database->erase_query<oj::db::AdminAuditLog>(
                AuditQuery::request_id == first_request_id ||
                AuditQuery::request_id == second_request_id);
            for (uint32_t uid : {first_user_id, second_user_id})
            {
                if (uid != 0)
                    database->erase_query<oj::db::AdminAccount>(
                        odb::query<oj::db::AdminAccount>::uid == uid);
            }
            for (uint64_t id : metric_ids)
                database->erase_query<oj::db::CacheMetricsSnapshot>(
                    odb::query<oj::db::CacheMetricsSnapshot>::id == id);
            for (uint32_t uid : {first_user_id, second_user_id})
            {
                if (uid != 0)
                    database->erase_query<oj::db::User>(odb::query<oj::db::User>::uid == uid);
            }
            transaction.commit();
            cleaned_ = true;
        }

        const std::string token;
        const std::string first_request_id;
        const std::string second_request_id;
        const std::string audit_action;
        const uint64_t metric_total;
        uint32_t first_user_id = 0;
        uint32_t second_user_id = 0;
        uint64_t old_metric_id = 0;
        std::set<uint64_t> metric_ids;

    private:
        const TestConfig& config_;
        bool cleaned_ = false;
    };

    long long MetricTotal(const std::vector<oj::model::Model::CacheMetricsAggregate>& metrics,
                          int action_type)
    {
        const auto found = std::find_if(metrics.begin(), metrics.end(), [&](const auto& metric) {
            return metric.action_type == action_type;
        });
        return found == metrics.end() ? 0 : found->total_requests;
    }
}

int main()
{
    using Model = oj::model::Model;
    static_assert(std::is_same_v<decltype(&Model::GetAdminCount),
                  bool (Model::*)(int*)>);
    static_assert(std::is_same_v<decltype(&Model::GetRoleCount),
                  bool (Model::*)(const std::string&, int*)>);
    static_assert(std::is_same_v<decltype(&Model::InsertCacheMetricSnapshot),
                  void (Model::*)(int, long long, long long, long long, long long)>);
    static_assert(std::is_same_v<decltype(&Model::FlushCacheMetrics), void (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::CleanupOldMetrics), void (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::StartMetricsFlushWorker), void (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::StopMetricsFlushWorker), void (Model::*)()>);

    try
    {
        const TestConfig config = TestConfig::Load();
        const std::string token = std::to_string(::getpid()) + "_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        LatencyLog latency("model_admin_odb_test", token);
        Fixture fixture(config, token);
        fixture.CreateUsers();
        fixture.CreateOldMetric();
        StartModelDatabase(config);
        Model model;

        Require(model.CreateAdminAccount(static_cast<int>(fixture.first_user_id), "hash-one", "admin") &&
                model.CreateAdminAccount(static_cast<int>(fixture.second_user_id), "hash-two", "admin"),
                "CreateAdminAccount failed");
        int admin_count = 0;
        int role_count = 0;
        Require(model.GetAdminCount(&admin_count) && admin_count >= 2,
                "GetAdminCount missed fixture admins");
        Require(model.GetRoleCount("admin", &role_count) && role_count >= 2,
                "GetRoleCount missed fixture admin roles");

        oj::db::AdminAccount first{};
        Require(model.GetAdminByUid(static_cast<int>(fixture.first_user_id)) &&
                model.GetAdminByUid(static_cast<int>(fixture.first_user_id), &first) &&
                first.uid == static_cast<int>(fixture.first_user_id) &&
                first.password_hash == "hash-one" && first.role == "admin",
                "GetAdminByUid returned incompatible fields");
        oj::db::AdminAccount by_id{};
        Require(model.GetAdminByAdminId(first.admin_id, &by_id) && by_id.uid == first.uid,
                "GetAdminByAdminId failed");
        Require(model.UpdateAdminRole(first.admin_id, "super_admin") &&
                model.GetAdminByAdminId(first.admin_id, &by_id) && by_id.role == "super_admin",
                "UpdateAdminRole failed");
        Require(model.GetRoleCount("super_admin", &role_count) && role_count >= 1,
                "GetRoleCount missed the updated role");

        std::vector<oj::db::AdminAccount> admins;
        int total_count = 0;
        Require(model.ListAdminsPaged(1, 200, std::to_string(fixture.first_user_id),
                                      &admins, &total_count) &&
                total_count == 1 && admins.size() == 1 && admins.front().admin_id == first.admin_id,
                "ListAdminsPaged UID filtering changed");

        oj::db::AdminAuditLog first_log{};
        first_log.request_id = fixture.first_request_id;
        first_log.operator_admin_id = first.admin_id;
        first_log.operator_uid = static_cast<int>(fixture.first_user_id);
        first_log.operator_role = "super_admin";
        first_log.action = fixture.audit_action;
        first_log.resource_type = "fixture";
        first_log.result = "success";
        first_log.payload_text = "{\"fixture\":1}";
        oj::db::AdminAuditLog second_log = first_log;
        second_log.request_id = fixture.second_request_id;
        second_log.result = "failed";
        Require(model.InsertAdminAuditLog(first_log) && model.InsertAdminAuditLog(second_log),
                "InsertAdminAuditLog failed");

        std::vector<oj::db::AdminAuditLog> logs;
        Require(model.ListAdminAuditLogsPaged(1, 20, fixture.audit_action,
                                               static_cast<int>(fixture.first_user_id), "",
                                               0, 0,
                                               &logs, &total_count) &&
                total_count == 2 && logs.size() == 2 &&
                logs.front().request_id == fixture.second_request_id,
                "audit filtering or ordering changed");
        Require(model.ListAdminAuditLogsPaged(1, 20, fixture.audit_action,
                                               static_cast<int>(fixture.first_user_id), "success",
                                               0, 0,
                                               &logs, &total_count) &&
                total_count == 1 && logs.front().request_id == fixture.first_request_id,
                "audit result filtering changed");

        std::vector<oj::model::Model::CacheMetricsAggregate> before;
        std::vector<oj::model::Model::CacheMetricsAggregate> after;
        Require(model.GetCacheMetricsOneDay(&before), "GetCacheMetricsOneDay baseline failed");
        const auto metric_ids_before = fixture.FindMetricIds();
        model.InsertCacheMetricSnapshot(4, static_cast<long long>(fixture.metric_total), 11, 7, 101);
        const std::set<uint64_t> metric_ids_after = fixture.FindMetricIds();
        std::set<uint64_t> inserted_metric_ids;
        std::set_difference(metric_ids_after.begin(), metric_ids_after.end(),
                            metric_ids_before.begin(), metric_ids_before.end(),
                            std::inserter(inserted_metric_ids, inserted_metric_ids.end()));
        Require(inserted_metric_ids.size() == 1,
                "InsertCacheMetricSnapshot did not create one fixture row");
        fixture.metric_ids.insert(inserted_metric_ids.begin(), inserted_metric_ids.end());
        Require(model.GetCacheMetricsOneDay(&after) &&
                MetricTotal(after, 4) >= MetricTotal(before, 4) +
                                         static_cast<long long>(fixture.metric_total),
                "GetCacheMetricsOneDay missed the inserted snapshot");

        auto& flush_metric = Model::Metrics().actions[3];
        flush_metric.total_requests.store(static_cast<long long>(fixture.metric_total + 1));
        flush_metric.cache_hits.store(12);
        flush_metric.db_fallbacks.store(8);
        flush_metric.total_ms.store(102);
        model.FlushCacheMetrics();
        const auto flushed_ids = fixture.FindMetricIds(fixture.metric_total + 1);
        Require(flushed_ids.size() == 1, "FlushCacheMetrics did not persist the fixture counters");
        fixture.metric_ids.insert(flushed_ids.begin(), flushed_ids.end());

        auto& worker_metric = Model::Metrics().actions[2];
        worker_metric.total_requests.store(static_cast<long long>(fixture.metric_total + 2));
        worker_metric.cache_hits.store(13);
        worker_metric.db_fallbacks.store(9);
        worker_metric.total_ms.store(103);
        model.StartMetricsFlushWorker();
        model.StartMetricsFlushWorker();
        model.StopMetricsFlushWorker();
        model.StopMetricsFlushWorker();
        const auto worker_ids = fixture.FindMetricIds(fixture.metric_total + 2);
        Require(worker_ids.size() == 1,
                "idempotent metrics worker lifecycle did not flush exactly once");
        fixture.metric_ids.insert(worker_ids.begin(), worker_ids.end());
        model.StartMetricsFlushWorker();
        model.StopMetricsFlushWorker();

        model.CleanupOldMetrics();
        Require(!fixture.MetricExists(fixture.old_metric_id),
                "CleanupOldMetrics retained the old fixture snapshot");

        fixture.RemoveAuditLogs();
        Require(model.DeleteAdminAccount(first.admin_id), "first DeleteAdminAccount failed");
        oj::db::AdminAccount second{};
        Require(model.GetAdminByUid(static_cast<int>(fixture.second_user_id), &second) &&
                model.DeleteAdminAccount(second.admin_id),
                "second DeleteAdminAccount failed");
        Require(!model.GetAdminByUid(static_cast<int>(fixture.first_user_id)),
                "deleted admin remained visible");

        fixture.Cleanup();
        oj::model::ModelBase::StopDatabase();
        latency.Verify({"Model.GetAdminCount.ODBQuery",
                        "Model.InsertCacheMetricSnapshot.ODBPersist",
                        "Model.CleanupOldMetrics.ODBErase"});
        std::cout << "model_admin_odb_test passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        oj::model::ModelBase::StopDatabase();
        std::cerr << "model_admin_odb_test failed: " << error.what() << '\n';
        return 1;
    }
}
