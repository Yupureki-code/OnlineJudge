#include "model/model_solution.hpp"
#include "comm/models/odb_models.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <unistd.h>

namespace
{
    using oj::model::ModelSolution;

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
                        "latency CSV missed a ModelSolution label");
        }

    private:
        std::string process_name_;
        std::filesystem::path directory_;
        std::filesystem::path file_;
    };

    void StartModelDatabase(const TestConfig& config)
    {
        config.ApplyModelEnvironment();
        Require(oj::model::ModelBase::StartDatabase("model_solution_odb_test"),
                "failed to start ModelSolution database context");
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
        explicit Fixture(const TestConfig& config) : config_(config) {}

        ~Fixture()
        {
            ModelSolution::StopDatabase();
            try { Cleanup(); }
            catch (const std::exception& error) {
                std::cerr << "fixture cleanup failed: " << error.what() << '\n';
            }
        }

        void CreatePrerequisites(const std::string& token)
        {
            solution_ids.reserve(4);
            auto database = config_.Connect();
            {
                odb::transaction transaction(database->begin());
                auto first = MakeUser("sa" + token.substr(token.size() > 16 ? token.size() - 16 : 0),
                                      "sa_" + token + "@example.test");
                auto second = MakeUser("sb" + token.substr(token.size() > 16 ? token.size() - 16 : 0),
                                       "sb_" + token + "@example.test");
                database->persist(first);
                database->persist(second);
                transaction.commit();
                first_user_id = first.uid;
                second_user_id = second.uid;
            }

            for (unsigned int attempt = 0; attempt < 1000; ++attempt)
            {
                const auto value = (std::chrono::steady_clock::now().time_since_epoch().count() +
                                    ::getpid() + attempt) % 90000 + 10000;
                oj::db::Question question{};
                question.id = std::to_string(value);
                question.title = "ODB solution fixture";
                question.desc = "owned by model_solution_odb_test";
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
            for (uint32_t id : solution_ids)
            {
                database->erase_query<oj::db::SolutionAction>(
                    odb::query<oj::db::SolutionAction>::solution_id == id);
                database->erase_query<oj::db::Solution>(
                    odb::query<oj::db::Solution>::id == id);
            }
            if (!question_id.empty())
            {
                database->erase_query<oj::db::TestCase>(
                    odb::query<oj::db::TestCase>::question_id == question_id);
                database->erase_query<oj::db::Question>(
                    odb::query<oj::db::Question>::id == question_id);
            }
            for (uint32_t id : {first_user_id, second_user_id})
            {
                if (id != 0)
                    database->erase_query<oj::db::User>(odb::query<oj::db::User>::uid == id);
            }
            transaction.commit();
            cleaned_ = true;
        }

        uint32_t first_user_id = 0;
        uint32_t second_user_id = 0;
        std::string question_id;
        std::vector<uint32_t> solution_ids;

    private:
        const TestConfig& config_;
        bool cleaned_ = false;
    };

    oj::db::Solution MakeSolution(const Fixture& fixture, const std::string& title,
                                  const std::string& status)
    {
        oj::db::Solution solution{};
        solution.question_id = fixture.question_id;
        solution.user_id = static_cast<int>(fixture.first_user_id);
        solution.title = title;
        solution.content_md = "fixture content for " + title;
        solution.status = status;
        return solution;
    }
}

int main()
{
    static_assert(std::is_same_v<decltype(&ModelSolution::CreateSolution),
                   bool (ModelSolution::*)(const oj::db::Solution&, unsigned long long*)>);
    static_assert(std::is_same_v<decltype(&ModelSolution::GetSolutionsByPage),
                  bool (ModelSolution::*)(const std::string&, const std::string&,
                                          const std::string&, int, int,
                                           std::vector<oj::db::Solution>*, int*, int*)>);
    static_assert(std::is_same_v<decltype(&ModelSolution::GetSolutionById),
                   bool (ModelSolution::*)(long long, oj::db::Solution*)>);
    static_assert(std::is_same_v<decltype(&ModelSolution::ToggleSolutionAction),
                  bool (ModelSolution::*)(long long, int, const std::string&,
                                           bool*, unsigned int*)>);
    static_assert(std::is_same_v<decltype(&ModelSolution::GetUserActionsForSolutions),
                  bool (ModelSolution::*)(int, const std::vector<long long>&,
                      std::map<long long, std::map<std::string, bool>>&)>);

    try
    {
        const TestConfig config = TestConfig::Load();
        const std::string token = std::to_string(::getpid()) + "_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        LatencyLog latency("model_solution_odb_test", token);
        Fixture fixture(config);
        fixture.CreatePrerequisites(token);
        StartModelDatabase(config);
        ModelSolution model;

        unsigned long long first_id = 0;
        unsigned long long second_id = 0;
        unsigned long long third_id = 0;
        unsigned long long pending_id = 0;
        Require(model.CreateSolution(MakeSolution(fixture, "first", "approved"), &first_id),
                "first CreateSolution failed");
        fixture.solution_ids.push_back(static_cast<uint32_t>(first_id));
        Require(model.CreateSolution(MakeSolution(fixture, "second", "approved"), &second_id),
                "second CreateSolution failed");
        fixture.solution_ids.push_back(static_cast<uint32_t>(second_id));
        Require(model.CreateSolution(MakeSolution(fixture, "third", "approved"), &third_id),
                "third CreateSolution failed");
        fixture.solution_ids.push_back(static_cast<uint32_t>(third_id));
        Require(model.CreateSolution(MakeSolution(fixture, "pending", "pending"), &pending_id),
                "pending CreateSolution failed");
        fixture.solution_ids.push_back(static_cast<uint32_t>(pending_id));

        std::vector<oj::db::Solution> page;
        int total_count = 0;
        int total_pages = 0;
        Require(model.GetSolutionsByPage(fixture.question_id, "approved", "latest", 2, 2,
                                         &page, &total_count, &total_pages) &&
                total_count == 3 && total_pages == 2 && page.size() == 1 &&
                page.front().id == first_id,
                "latest solution pagination changed");
        page.clear();
        Require(model.GetSolutionsByPage(fixture.question_id, "pending", "latest", 1, 10,
                                         &page, &total_count, &total_pages) &&
                total_count == 1 && page.size() == 1 && page.front().id == pending_id,
                "solution status filtering changed");

        oj::db::Solution detail{};
        Require(model.GetSolutionById(static_cast<long long>(first_id), &detail) &&
                 detail.title == "first" && detail.status == "approved",
                "GetSolutionById changed mapping behavior");

        bool active = false;
        unsigned int count = 0;
        Require(model.ToggleSolutionAction(first_id, fixture.second_user_id, "like", &active, &count) &&
                active && count == 1,
                "solution like activation failed");
        Require(model.ToggleSolutionAction(first_id, fixture.second_user_id, "like", &active, &count) &&
                !active && count == 0,
                "solution like deactivation failed");
        Require(model.ToggleSolutionAction(first_id, fixture.second_user_id, "like", &active, &count) && active,
                "solution like reactivation failed");
        Require(model.ToggleSolutionAction(second_id, fixture.second_user_id, "favorite", &active, &count) && active,
                "solution favorite activation failed");
        Require(model.ToggleSolutionAction(third_id, fixture.first_user_id, "like", &active, &count) && active,
                "hot-sort fixture activation failed");

        page.clear();
        Require(model.GetSolutionsByPage(fixture.question_id, "approved", "hot", 1, 2,
                                         &page, &total_count, &total_pages) &&
                page.size() == 2 && page.front().id == third_id,
                "hot solution ordering changed");

        std::map<long long, std::map<std::string, bool>> actions;
        Require(model.GetUserActionsForSolutions(
                    fixture.second_user_id,
                    {static_cast<long long>(first_id), static_cast<long long>(second_id),
                     static_cast<long long>(third_id)}, actions) &&
                actions[first_id]["like"] && actions[second_id]["favorite"] &&
                actions.find(third_id) == actions.end(),
                "GetUserActionsForSolutions changed behavior");

        fixture.Cleanup();
        oj::model::ModelBase::StopDatabase();
        latency.Verify({"ModelSolution.CreateSolution.ODBPersist",
                        "ModelSolution.GetUserActionsForSolutions.ODBQuery"});
        std::cout << "model_solution_odb_test passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        oj::model::ModelBase::StopDatabase();
        std::cerr << "model_solution_odb_test failed: " << error.what() << '\n';
        return 1;
    }
}
