#include "model/model_question.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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
                        "latency CSV missed a ModelQuestion label");
        }

    private:
        std::string process_name_;
        std::filesystem::path directory_;
        std::filesystem::path file_;
    };

    void StartModelDatabase(const TestConfig& config)
    {
        config.ApplyModelEnvironment();
        Require(oj::model::ModelBase::StartDatabase("model_question_odb_test"),
                "failed to start ModelQuestion database context");
    }

    oj::db::Question MakeQuestion(const std::string& id, const std::string& title,
                                  const std::string& difficulty)
    {
        oj::db::Question question{};
        question.id = id;
        question.title = title;
        question.desc = "owned by model_question_odb_test";
        question.star = difficulty;
        question.create_time = oj::util::TimeUtil::IntToDateTime(
            oj::util::TimeUtil::GetTimeStamp());
        question.update_time = question.create_time;
        question.cpu_limit = 2;
        question.memory_limit = 128;
        return question;
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

        void CreateQuestions(const std::string& marker)
        {
            auto database = config_.Connect();
            for (unsigned int attempt = 0; attempt < 1000; ++attempt)
            {
                const auto base = (std::chrono::steady_clock::now().time_since_epoch().count() +
                                   ::getpid() + attempt * 2) % 89999 + 10000;
                auto first = MakeQuestion(std::to_string(base), marker + " first", "简单");
                auto second = MakeQuestion(std::to_string(base + 1), marker + " " + first.id, "中等");
                try {
                    odb::transaction transaction(database->begin());
                    database->persist(first);
                    database->persist(second);
                    transaction.commit();
                    first_id = first.id;
                    second_id = second.id;
                    return;
                } catch (const odb::object_already_persistent&) {
                }
            }
            throw std::runtime_error("unable to allocate unique question fixtures");
        }

        void Cleanup()
        {
            if (cleaned_) return;
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            for (const std::string* id : {&first_id, &second_id})
            {
                if (id->empty()) continue;
                database->erase_query<oj::db::TestCase>(
                    odb::query<oj::db::TestCase>::question_id == *id);
                database->erase_query<oj::db::Question>(
                    odb::query<oj::db::Question>::id == *id);
            }
            transaction.commit();
            cleaned_ = true;
        }

        std::string first_id;
        std::string second_id;

    private:
        const TestConfig& config_;
        bool cleaned_ = false;
    };

    bool ContainsQuestion(const std::vector<oj::db::Question>& questions, const std::string& id)
    {
        return std::any_of(questions.begin(), questions.end(), [&](const oj::db::Question& question) {
            return question.id == id;
        });
    }
}

int main()
{
    using Model = oj::model::ModelQuestion;
    static_assert(std::is_same_v<decltype(&Model::GetQuestionCount),
                  bool (Model::*)(int*)>);
    static_assert(std::is_same_v<decltype(&Model::TouchQuestionListVersion),
                  std::string (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::GetQuestionsListVersion),
                  std::string (Model::*)()>);
    static_assert(std::is_same_v<decltype(&Model::BuildListCacheKey),
                   std::shared_ptr<oj::cache::Cache::CacheListKey> (Model::*)(
                       std::shared_ptr<QueryStruct>&, int, int, const std::string&,
                       oj::cache::Cache::CacheKey::PageType)>);
    static_assert(std::is_same_v<decltype(&Model::BuildDetailCacheKey),
                   std::shared_ptr<oj::cache::Cache::CacheDetailKey> (Model::*)(
                       const std::string&, oj::cache::Cache::CacheKey::PageType)>);
    static_assert(std::is_same_v<decltype(&Model::BuildHtmlCacheKey),
                   std::shared_ptr<oj::cache::Cache::CacheStaticKey> (Model::*)(
                       const std::string&, oj::cache::Cache::CacheKey::PageType)>);

    try
    {
        const TestConfig config = TestConfig::Load();
        const std::string marker = "odbq" + std::to_string(::getpid()) + "_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        LatencyLog latency("model_question_odb_test", marker);
        Fixture fixture(config);
        fixture.CreateQuestions(marker);
        StartModelDatabase(config);
        Model model;

        const std::string version = model.GetQuestionsListVersion();
        const std::string touched_version = model.TouchQuestionListVersion();
        Require(!version.empty() && !touched_version.empty(),
                "question list version helpers returned an empty version");

        oj::db::Question first{};
        first.id = fixture.first_id;
        first.title = marker + " updated";
        first.desc = "updated description";
        first.star = "简单";
        first.cpu_limit = 3;
        first.memory_limit = 256;
        Require(model.SaveQuestion(first), "SaveQuestion update failed");
        int question_count = 0;
        Require(model.GetQuestionCount(&question_count) && question_count >= 2,
                "GetQuestionCount missed fixture questions");

        oj::db::Question loaded{};
        Require(model.GetOneQuestion(fixture.first_id, loaded) &&
                loaded.title == first.title && loaded.desc == first.desc,
                "GetOneQuestion returned incompatible fields");
        Require(!model.GetOneQuestion("not-a-number", loaded),
                "GetOneQuestion accepted an invalid ID");

        std::shared_ptr<QueryStruct> title_query = std::make_shared<QuestionQuery>();
        std::dynamic_pointer_cast<QuestionQuery>(title_query)->title = "  " + marker + "  ";
        auto title_key = model.BuildListCacheKey(title_query, 99, 1, marker,
            oj::cache::Cache::CacheKey::PageType::kData);
        const auto normalized_title =
            std::dynamic_pointer_cast<QuestionQuery>(title_key->GetQueryHash());
        Require(normalized_title && normalized_title->title == marker &&
                title_key->GetPage() == 99 && title_key->GetSize() == 1 &&
                title_key->GetListVersion() == marker &&
                title_key->GetListType() == ListType::Questions &&
                 title_key->GetPageType() == oj::cache::Cache::CacheKey::PageType::kData,
                "BuildListCacheKey changed normalization or key metadata");
        auto detail_key = model.BuildDetailCacheKey(
            fixture.first_id, oj::cache::Cache::CacheKey::PageType::kHtml);
        auto html_key = model.BuildHtmlCacheKey(
            "questions", oj::cache::Cache::CacheKey::PageType::kHtml);
        Require(detail_key->GetPageName() == fixture.first_id &&
                 detail_key->GetPageType() == oj::cache::Cache::CacheKey::PageType::kHtml &&
                html_key->GetPageName() == "questions" &&
                 html_key->GetPageType() == oj::cache::Cache::CacheKey::PageType::kHtml,
                "question detail/static key helpers changed metadata");
        std::vector<oj::db::Question> page;
        int total_count = 0;
        int total_pages = 0;
        Require(model.GetQuestionsByPage(title_key, page, &total_count, &total_pages) &&
                total_count == 2 && total_pages == 2 && page.size() == 1 &&
                 page.front().id == fixture.second_id,
                "question ordering or page clamping changed");

        std::shared_ptr<QueryStruct> difficulty_query = std::make_shared<QuestionQuery>();
        auto difficulty = std::dynamic_pointer_cast<QuestionQuery>(difficulty_query);
        difficulty->title = marker;
        difficulty->star = " SIMPLE ";
        auto difficulty_key = model.BuildListCacheKey(difficulty_query, 1, 10, marker,
            oj::cache::Cache::CacheKey::PageType::kData);
        page.clear();
        Require(model.GetQuestionsByPage(difficulty_key, page, &total_count, &total_pages) &&
                 total_count == 1 && page.size() == 1 && page.front().id == fixture.first_id,
                "difficulty normalization changed");

        std::shared_ptr<QueryStruct> both_query = std::make_shared<QuestionQuery>();
        auto both = std::dynamic_pointer_cast<QuestionQuery>(both_query);
        both->id = fixture.first_id;
        both->title = fixture.first_id;
        auto both_key = model.BuildListCacheKey(both_query, 1, 10, marker,
            oj::cache::Cache::CacheKey::PageType::kData);
        page.clear();
        Require(model.GetQuestionsByPage(both_key, page, &total_count, &total_pages) &&
                total_count == 2 && ContainsQuestion(page, fixture.first_id) &&
                ContainsQuestion(page, fixture.second_id),
                "combined ID/title OR behavior changed");

        int sample_id = 0;
        int hidden_id = 0;
        Require(model.CreateTestCase(fixture.first_id, "sample input", "sample output", true, &sample_id) &&
                model.CreateTestCase(fixture.first_id, "hidden input", "hidden output", false, &hidden_id),
                "CreateTestCase failed");
        Require(hidden_id == sample_id + 1, "test ordinals are not sequential");

        Json::Value samples(Json::arrayValue);
        Require(model.GetSampleTestsByQuestionId(fixture.first_id, &samples) && samples.size() == 1 &&
                samples[0]["id"].asInt() == sample_id &&
                samples[0]["input"].asString() == "sample input",
                "sample test JSON behavior changed");
        Json::Value all_tests;
        Require(model.GetTestsByQuestionId(fixture.first_id, &all_tests) &&
                all_tests["tests"].size() == 2,
                "GetTestsByQuestionId failed");

        std::string input;
        std::string output;
        Require(model.GetTestById(hidden_id, fixture.first_id, &input, &output) &&
                input == "hidden input" && output == "hidden output",
                "GetTestById failed");
        Require(!model.GetTestById(hidden_id, fixture.second_id, &input, &output),
                "GetTestById ignored question ownership");
        Require(model.UpdateTestCase(hidden_id, fixture.first_id, "updated input", "updated output", true) &&
                model.GetTestById(hidden_id, fixture.first_id, &input, &output) &&
                input == "updated input" && output == "updated output",
                "UpdateTestCase failed");
        Require(model.DeleteTestCase(hidden_id, fixture.first_id) &&
                !model.GetTestById(hidden_id, fixture.first_id, &input, &output),
                "DeleteTestCase failed");

        Require(model.DeleteQuestion(fixture.second_id), "DeleteQuestion failed");
        fixture.Cleanup();
        oj::model::ModelBase::StopDatabase();
        latency.Verify({"ModelQuestion.GetQuestionCount.ODBQuery",
                        "ModelQuestion.GetQuestionsByPage.ODBQuery"});
        std::cout << "model_question_odb_test passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        oj::model::ModelBase::StopDatabase();
        std::cerr << "model_question_odb_test failed: " << error.what() << '\n';
        return 1;
    }
}
