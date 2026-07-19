#include "model/model_comment.hpp"

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
                        "latency CSV missed a ModelComment label");
        }

    private:
        std::string process_name_;
        std::filesystem::path directory_;
        std::filesystem::path file_;
    };

    void StartModelDatabase(const TestConfig& config)
    {
        config.ApplyModelEnvironment();
        Require(oj::model::ModelBase::StartDatabase("model_comment_odb_test"),
                "failed to start ModelComment database context");
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
            oj::model::ModelBase::StopDatabase();
            try { Cleanup(); }
            catch (const std::exception& error) {
                std::cerr << "fixture cleanup failed: " << error.what() << '\n';
            }
        }

        void CreatePrerequisites(const std::string& token)
        {
            comment_ids.reserve(2);
            auto database = config_.Connect();
            {
                odb::transaction transaction(database->begin());
                auto owner = MakeUser("ca" + token.substr(token.size() > 16 ? token.size() - 16 : 0),
                                      "ca_" + token + "@example.test");
                auto other = MakeUser("cb" + token.substr(token.size() > 16 ? token.size() - 16 : 0),
                                      "cb_" + token + "@example.test");
                database->persist(owner);
                database->persist(other);
                transaction.commit();
                owner_id = owner.uid;
                other_id = other.uid;
            }

            for (unsigned int attempt = 0; attempt < 1000; ++attempt)
            {
                const auto value = (std::chrono::steady_clock::now().time_since_epoch().count() +
                                    ::getpid() + attempt) % 90000 + 10000;
                oj::db::Question question{};
                question.id = std::to_string(value);
                question.title = "ODB comment fixture";
                question.desc = "owned by model_comment_odb_test";
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
                    break;
                } catch (const odb::object_already_persistent&) {
                }
            }
            if (question_id.empty())
                throw std::runtime_error("unable to allocate a unique question fixture");

            odb::transaction transaction(database->begin());
            oj::db::Solution solution{};
            solution.question_id = question_id;
            solution.user_id = owner_id;
            solution.title = "comment fixture solution";
            solution.content_md = "owned by model_comment_odb_test";
            solution.like_count = 0;
            solution.favorite_count = 0;
            solution.comment_count = 0;
            solution.status = "approved";
            solution.created_at = oj::util::TimeUtil::IntToDateTime(
                oj::util::TimeUtil::GetTimeStamp());
            solution.updated_at = solution.created_at;
            database->persist(solution);
            transaction.commit();
            solution_id = solution.id;
        }

        void Cleanup()
        {
            if (cleaned_) return;
            auto database = config_.Connect();
            odb::transaction transaction(database->begin());
            for (uint32_t id : comment_ids)
                database->erase_query<oj::db::CommentAction>(
                    odb::query<oj::db::CommentAction>::comment_id == id);
            for (auto it = comment_ids.rbegin(); it != comment_ids.rend(); ++it)
                database->erase_query<oj::db::Comment>(
                    odb::query<oj::db::Comment>::id == *it);
            if (solution_id != 0)
            {
                database->erase_query<oj::db::SolutionAction>(
                    odb::query<oj::db::SolutionAction>::solution_id == solution_id);
                database->erase_query<oj::db::Solution>(
                    odb::query<oj::db::Solution>::id == solution_id);
            }
            if (!question_id.empty())
            {
                database->erase_query<oj::db::TestCase>(
                    odb::query<oj::db::TestCase>::question_id == question_id);
                database->erase_query<oj::db::Question>(
                    odb::query<oj::db::Question>::id == question_id);
            }
            for (uint32_t id : {owner_id, other_id})
            {
                if (id != 0)
                    database->erase_query<oj::db::User>(odb::query<oj::db::User>::uid == id);
            }
            transaction.commit();
            cleaned_ = true;
        }

        uint32_t owner_id = 0;
        uint32_t other_id = 0;
        uint32_t solution_id = 0;
        std::string question_id;
        std::vector<uint32_t> comment_ids;

    private:
        const TestConfig& config_;
        bool cleaned_ = false;
    };

    uint32_t SolutionCommentCount(const TestConfig& config, uint32_t solution_id)
    {
        auto database = config.Connect();
        odb::transaction transaction(database->begin());
        std::unique_ptr<oj::db::Solution> solution(database->find<oj::db::Solution>(solution_id));
        Require(static_cast<bool>(solution), "fixture solution is missing");
        const uint32_t count = solution->comment_count;
        transaction.commit();
        return count;
    }
}

int main()
{
    using Model = oj::model::ModelComment;
    static_assert(std::is_same_v<decltype(&Model::GetCommentsBySolutionId),
        bool (Model::*)(unsigned long long, int, int, std::vector<oj::db::Comment>*, int*)>);
    static_assert(std::is_same_v<decltype(&Model::CreateComment),
        bool (Model::*)(const oj::db::Comment&, unsigned long long*)>);
    static_assert(std::is_same_v<decltype(&Model::UpdateComment),
        bool (Model::*)(unsigned long long, int, const std::string&)>);
    static_assert(std::is_same_v<decltype(&Model::DeleteComment),
        bool (Model::*)(unsigned long long, int, bool)>);
    static_assert(std::is_same_v<decltype(&Model::GetCommentById),
        bool (Model::*)(unsigned long long, oj::db::Comment*)>);
    static_assert(std::is_same_v<decltype(&Model::GetCommentReplies),
        bool (Model::*)(unsigned long long, int, int, std::vector<oj::db::Comment>*, int*)>);
    static_assert(std::is_same_v<decltype(&Model::GetDirectChildReplyCounts),
        bool (Model::*)(const std::vector<unsigned long long>&,
                        std::map<unsigned long long, int>*)>);
    static_assert(std::is_same_v<decltype(&Model::GetCommentActions),
        bool (Model::*)(const std::vector<unsigned long long>&, int,
            std::map<unsigned long long, std::map<std::string, bool>>*)>);
    static_assert(std::is_same_v<decltype(&Model::ToggleCommentAction),
        bool (Model::*)(unsigned long long, int, const std::string&,
                        bool*, unsigned int*)>);

    try
    {
        const TestConfig config = TestConfig::Load();
        const std::string token = std::to_string(::getpid()) + "_" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        LatencyLog latency("model_comment_odb_test", token);
        Fixture fixture(config);
        fixture.CreatePrerequisites(token);
        StartModelDatabase(config);
        Model model;

        oj::db::Comment top{};
        top.solution_id = fixture.solution_id;
        top.user_id = static_cast<int>(fixture.owner_id);
        top.content = "model_comment_odb_test top " + token;
        unsigned long long top_id = 0;
        Require(model.CreateComment(top, &top_id) && top_id != 0, "top-level CreateComment failed");
        fixture.comment_ids.push_back(static_cast<uint32_t>(top_id));
        Require(SolutionCommentCount(config, fixture.solution_id) == 1,
                "top-level comment did not increment comment_count");

        oj::db::Comment loaded{};
        Require(model.GetCommentById(top_id, &loaded) &&
                 loaded.user_id == fixture.owner_id,
                 "GetCommentById returned incompatible fields");

        oj::db::Comment reply{};
        reply.solution_id = fixture.solution_id;
        reply.user_id = static_cast<int>(fixture.other_id);
        reply.parent_id = top_id;
        reply.reply_to_user_id = static_cast<int>(fixture.owner_id);
        reply.content = "model_comment_odb_test reply " + token;
        unsigned long long reply_id = 0;
        Require(model.CreateComment(reply, &reply_id) && reply_id != 0, "reply CreateComment failed");
        fixture.comment_ids.push_back(static_cast<uint32_t>(reply_id));
        oj::db::Comment second_reply = reply;
        second_reply.user_id = static_cast<int>(fixture.owner_id);
        second_reply.reply_to_user_id = static_cast<int>(fixture.other_id);
        second_reply.content = "model_comment_odb_test second reply " + token;
        unsigned long long second_reply_id = 0;
        Require(model.CreateComment(second_reply, &second_reply_id) && second_reply_id != 0,
                "second reply CreateComment failed");
        fixture.comment_ids.push_back(static_cast<uint32_t>(second_reply_id));
        Require(SolutionCommentCount(config, fixture.solution_id) == 1,
                "reply changed top-level comment_count");

        std::vector<oj::db::Comment> comments;
        int total = 0;
        Require(model.GetCommentsBySolutionId(fixture.solution_id, 1, 100, &comments, &total) &&
                total == 1 && comments.size() == 1 && comments.front().id == top_id,
                "GetCommentsBySolutionId changed top-level behavior");
        std::vector<oj::db::Comment> replies;
        Require(model.GetCommentReplies(top_id, 1, 100, &replies, &total) &&
                 total == 2 && replies.size() == 2 && replies.front().id == reply_id &&
                 !replies.front().reply_to_user_id.null() &&
                 replies.front().reply_to_user_id.get() == fixture.owner_id,
                "GetCommentReplies changed reply mapping");

        std::map<unsigned long long, int> reply_counts;
        Require(model.GetDirectChildReplyCounts({top_id, reply_id}, &reply_counts) &&
                reply_counts[top_id] == 2 && reply_counts.count(reply_id) == 0,
                "GetDirectChildReplyCounts failed");
        Require(model.GetDirectChildReplyCounts({}, &reply_counts) && reply_counts.empty(),
                "empty reply-count batch changed behavior");

        bool active = false;
        unsigned int action_count = 0;
        Require(model.ToggleCommentAction(top_id, fixture.owner_id, "like", &active, &action_count) &&
                active && action_count == 1,
                "comment like activation failed");
        Require(model.ToggleCommentAction(reply_id, fixture.owner_id, "favorite", &active, &action_count) &&
                active && action_count == 1,
                "reply favorite activation failed");
        std::map<unsigned long long, std::map<std::string, bool>> actions;
        Require(model.GetCommentActions({top_id, reply_id, second_reply_id}, fixture.owner_id, &actions) &&
                actions[top_id]["like"] && actions[reply_id]["favorite"] &&
                actions.count(second_reply_id) == 0,
                "GetCommentActions changed batched action behavior");
        Require(model.ToggleCommentAction(top_id, fixture.owner_id, "like", &active, &action_count) &&
                !active && action_count == 0,
                "comment like deactivation failed");

        Require(!model.UpdateComment(top_id, fixture.other_id, "not owned"),
                "UpdateComment bypassed ownership");
        Require(model.UpdateComment(top_id, fixture.owner_id, "edited fixture comment"),
                "owned UpdateComment failed");
        Require(model.GetCommentById(top_id, &loaded) && loaded.content == "edited fixture comment" &&
                loaded.is_edited,
                "UpdateComment did not persist edited state");
        Require(!model.DeleteComment(reply_id, fixture.owner_id, false),
                "DeleteComment bypassed ownership");
        Require(model.DeleteComment(reply_id, 0, true),
                "admin DeleteComment override failed");
        Require(SolutionCommentCount(config, fixture.solution_id) == 1,
                "reply deletion changed top-level comment_count");
        Require(model.DeleteComment(top_id, fixture.owner_id, false),
                "owned top-level DeleteComment failed");
        Require(SolutionCommentCount(config, fixture.solution_id) == 0,
                "top-level deletion did not restore comment_count");

        fixture.Cleanup();
        Model::StopDatabase();
        latency.Verify({"ModelComment.GetDirectChildReplyCounts.ODBQueryCounts",
                        "ModelComment.GetCommentActions.ODBQueryActions"});
        std::cout << "model_comment_odb_test passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        Model::StopDatabase();
        std::cerr << "model_comment_odb_test failed: " << error.what() << '\n';
        return 1;
    }
}
