#include "model/model_user.hpp"
#include "comm/models/odb_models.hpp"

#include <chrono>
#include <cmath>
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
                RequiredEnv("OJ_ODB_TEST_HOST"),
                RequiredEnv("OJ_ODB_TEST_USER"),
                RequiredEnv("OJ_ODB_TEST_PASSWORD"),
                RequiredEnv("OJ_ODB_TEST_DATABASE"),
                0
            };
            const std::string port = RequiredEnv("OJ_ODB_TEST_PORT");
            const unsigned long parsed = std::stoul(port);
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
                        "latency CSV missed a ModelUser label");
        }

    private:
        std::string process_name_;
        std::filesystem::path directory_;
        std::filesystem::path file_;
    };

    void StartModelDatabase(const TestConfig& config)
    {
        config.ApplyModelEnvironment();
        Require(oj::model::ModelBase::StartDatabase("model_user_odb_test"),
                "failed to start ModelUser database context");
    }

    std::string UniqueToken()
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::to_string(::getpid()) + "_" + std::to_string(ticks);
    }

    class Fixture
    {
    public:
        explicit Fixture(const TestConfig& config) : config_(config) {}

        ~Fixture()
        {
            oj::model::ModelBase::StopDatabase();
            try { Cleanup(); }
            catch (const std::exception& error) {
                std::cerr << "fixture cleanup failed: " << error.what() << '\n';
            }
        }

        void CreateQuestion()
        {
            auto database = config_.Connect();
            for (unsigned int attempt = 0; attempt < 1000; ++attempt)
            {
                const auto value = (std::chrono::steady_clock::now().time_since_epoch().count() +
                                    ::getpid() + attempt) % 90000 + 10000;
                oj::db::Question question{};
                question.id = std::to_string(value);
                question.title = "ODB user fixture question";
                question.desc = "owned by model_user_odb_test";
                question.star = "简单";
                question.create_time = oj::util::TimeUtil::IntToDateTime(
                    oj::util::TimeUtil::GetTimeStamp());
                question.update_time = question.create_time;
                question.cpu_limit = 1;
                question.memory_limit = 64;
                try {
                    odb::transaction transaction(database->begin());
                    database->persist(question);
                    transaction.commit();
                    question_id = question.id;
                    return;
                } catch (const odb::object_already_persistent&) {
                }
            }
            throw std::runtime_error("unable to allocate a unique question fixture");
        }

        void Cleanup()
        {
            if (cleaned_) return;
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            uint32_t owned_user_id = user_id;
            if (owned_user_id == 0 && !user_email.empty())
            {
                std::unique_ptr<oj::db::User> user(database->query_one<oj::db::User>(
                    odb::query<oj::db::User>::email == user_email ||
                    odb::query<oj::db::User>::email == alternate_user_email));
                if (user) owned_user_id = user->uid;
            }
            if (owned_user_id != 0)
            {
                database->erase_query<oj::db::LegacySubmission>(
                    odb::query<oj::db::LegacySubmission>::user_id == owned_user_id);
                database->erase_query<oj::db::User>(
                    odb::query<oj::db::User>::uid == owned_user_id);
            }
            if (!question_id.empty())
            {
                database->erase_query<oj::db::TestCase>(
                    odb::query<oj::db::TestCase>::question_id == question_id);
                database->erase_query<oj::db::Question>(
                    odb::query<oj::db::Question>::id == question_id);
            }
            transaction.commit();
            cleaned_ = true;
        }

        uint32_t user_id = 0;
        std::string user_email;
        std::string alternate_user_email;
        std::string question_id;

    private:
        const TestConfig& config_;
        bool cleaned_ = false;
    };
}

int main()
{
    static_assert(std::is_same_v<decltype(&oj::model::ModelUser::GetUserCount),
                  bool (oj::model::ModelUser::*)(int*)>);

    try
    {
        const TestConfig config = TestConfig::Load();
        const std::string token = UniqueToken();
        LatencyLog latency("model_user_odb_test", token);
        Fixture fixture(config);
        fixture.CreateQuestion();
        const std::string first_name = "u" + token.substr(token.size() > 17 ? token.size() - 17 : 0);
        const std::string second_name = "v" + token.substr(token.size() > 17 ? token.size() - 17 : 0);
        const std::string first_email = "user_" + token + "@example.test";
        const std::string second_email = "user2_" + token + "@example.test";
        fixture.user_email = first_email;
        fixture.alternate_user_email = second_email;
        StartModelDatabase(config);
        oj::model::ModelUser model;
        Require(!model.CheckUser(first_email), "CheckUser should report an absent user");
        Require(model.CreateUser(first_name, first_email), "CreateUser failed");

        oj::db::User user{};
        Require(model.CheckUser(first_email) && model.GetUser(first_email, &user),
                "created user was not readable");
        Require(user.uid > 0 && user.name == first_name && user.email == first_email,
                "GetUser returned incompatible fields");
        fixture.user_id = static_cast<uint32_t>(user.uid);
        int user_count = 0;
        Require(model.GetUserCount(&user_count) && user_count >= 1,
                "GetUserCount missed the created user");

        oj::db::User loaded{};
        Require(model.GetUserById(user.uid, &loaded) && loaded.email == first_email,
                "GetUserById failed");
        Require(model.GetUserByName(first_name, &loaded) && loaded.uid == user.uid,
                "GetUserByName failed");

        auto query = std::make_shared<UserQuery>();
        query->email = first_email;
        KeyContext context;
        context._query = query;
        context.page = 1;
        context.size = 5;
        context.list_type = ListType::Users;
        auto page_key = std::make_shared<oj::cache::Cache::CacheListKey>();
        page_key->Build(context);
        std::vector<oj::db::User> page;
        int total_count = 0;
        int total_pages = 0;
        Require(model.GetUsersPaged(page_key, &page, &total_count, &total_pages) &&
                total_count == 1 && total_pages == 1 && page.size() == 1 &&
                page.front().uid == user.uid,
                "GetUsersPaged changed filtering or pagination behavior");

        std::map<int, oj::db::User> users;
        Require(model.GetUsersByIds({static_cast<int>(user.uid)}, &users) && users.size() == 1,
                "GetUsersByIds failed");
        Require(model.UpdateUserName(user.uid, second_name) &&
                model.GetUserByName(second_name, &loaded) && loaded.uid == user.uid,
                "UpdateUserName failed");
        Require(model.UpdateLastLogin(first_email), "UpdateLastLogin failed");

        Require(model.SetUserPassword(first_email, "fixture-hash", "fixture-algo"),
                "SetUserPassword failed");
        std::string hash;
        std::string algorithm;
        Require(model.GetUserPasswordAuth(first_email, &hash, &algorithm) &&
                hash == "fixture-hash" && algorithm == "fixture-algo",
                "password authentication fields changed");
        Require(model.UpdateUserEmail(first_email, second_email) &&
                !model.GetUser(first_email, &loaded) &&
                model.GetUser(second_email, &loaded) && loaded.uid == user.uid,
                "UpdateUserEmail failed");

        Require(!model.HasUserPassedQuestion(user.uid, fixture.question_id),
                "fixture question unexpectedly passed");
        Require(model.RecordUserSubmit(user.uid, fixture.question_id, "{\"case\":1}", false) &&
                model.RecordUserSubmit(user.uid, fixture.question_id, "{\"case\":2}", true),
                "RecordUserSubmit failed");
        Require(model.HasUserPassedQuestion(user.uid, fixture.question_id),
                "HasUserPassedQuestion missed a passing submission");

        Json::Value submissions;
        Require(model.GetUserSubmitsByQuestion(user.uid, fixture.question_id, &submissions) &&
                submissions.isArray() && submissions.size() == 2,
                "GetUserSubmitsByQuestion returned an incompatible result");
        Json::Value stats;
        Require(model.GetUserStats(user.uid, &stats) &&
                stats["total_submits"].asInt() == 2 &&
                stats["passed_questions"].asInt() == 1 &&
                stats["passed_submits"].asInt() == 1 &&
                std::fabs(stats["accuracy"].asDouble() - 0.5) < 0.000001,
                "GetUserStats aggregate behavior changed");

        {
            auto database = config.Connect();
            odb::transaction transaction(database->begin());
            database->erase_query<oj::db::LegacySubmission>(
                odb::query<oj::db::LegacySubmission>::user_id == fixture.user_id);
            transaction.commit();
        }
        Require(model.DeleteUserByEmail(second_email) && !model.CheckUser(second_email),
                "DeleteUserByEmail failed");
        fixture.user_id = 0;

        fixture.Cleanup();
        oj::model::ModelBase::StopDatabase();
        latency.Verify({"ModelUser.GetUserCount.ODBQuery"});
        std::cout << "model_user_odb_test passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        oj::model::ModelBase::StopDatabase();
        std::cerr << "model_user_odb_test failed: " << error.what() << '\n';
        return 1;
    }
}
