#include "control/oj_control.hpp"
#include "model/model_submission.hpp"
#include "../../src/comm/models/user.hxx"
#include "user-odb.hxx"

#include <odb/mysql/database.hxx>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

oj::db::DateTime Now()
{
    return oj::util::TimeUtil::IntToDateTime(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

void EraseFixture(odb::mysql::database& database, const std::string& visible_id,
                  const std::string& hidden_id)
{
    odb::transaction transaction(database.begin());
    using CommentQuery = odb::query<oj::db::Comment>;
    using SolutionQuery = odb::query<oj::db::Solution>;
    using SubmissionQuery = odb::query<oj::db::Submission>;
    std::vector<uint32_t> solution_ids;
    for (const auto& solution : database.query<oj::db::Solution>(
             SolutionQuery::question_id == visible_id || SolutionQuery::question_id == hidden_id))
        solution_ids.push_back(solution.id);
    if (!solution_ids.empty())
        database.erase_query<oj::db::Comment>(
            CommentQuery::solution_id.in_range(solution_ids.begin(), solution_ids.end()));
    database.erase_query<oj::db::Submission>(
        SubmissionQuery::question_id == visible_id || SubmissionQuery::question_id == hidden_id);
    database.erase_query<oj::db::Solution>(
        SolutionQuery::question_id == visible_id || SolutionQuery::question_id == hidden_id);
    database.erase_query<oj::db::Question>(
        odb::query<oj::db::Question>::id == visible_id ||
        odb::query<oj::db::Question>::id == hidden_id);
    transaction.commit();
}
}

int main()
{
    if (std::getenv("OJ_DB_HOST") == nullptr) setenv("OJ_DB_HOST", "127.0.0.1", 0);
    if (std::getenv("OJ_DB_PORT") == nullptr) setenv("OJ_DB_PORT", "3306", 0);
    if (std::getenv("OJ_DB_USER") == nullptr) setenv("OJ_DB_USER", "oj_server", 0);
    if (std::getenv("OJ_DB_PASS") == nullptr) setenv("OJ_DB_PASS", "password", 0);
    if (std::getenv("OJ_DB_NAME") == nullptr) setenv("OJ_DB_NAME", "myoj", 0);
    if (std::getenv("OJ_REDIS_ADDR") == nullptr)
        setenv("OJ_REDIS_ADDR", "tcp://127.0.0.1:6379", 0);

    const auto suffix = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::string visible_id = "98" + suffix.substr(suffix.size() - 3);
    const std::string hidden_id = "99" + suffix.substr(suffix.size() - 3);
    odb::mysql::database database(
        std::getenv("OJ_DB_USER"), std::getenv("OJ_DB_PASS"), std::getenv("OJ_DB_NAME"),
        std::getenv("OJ_DB_HOST"), static_cast<unsigned int>(std::stoul(std::getenv("OJ_DB_PORT"))));
    EraseFixture(database, visible_id, hidden_id);

    uint32_t visible_solution_id = 0;
    uint32_t hidden_solution_id = 0;
    uint32_t pending_solution_id = 0;
    uint32_t top_comment_id = 0;
    uint32_t hidden_comment_id = 0;
    uint32_t pending_comment_id = 0;
    uint32_t fixture_user_id = 0;
    {
        odb::transaction transaction(database.begin());
        const auto now = Now();
        oj::db::User fixture_user{};
        fixture_user.name = "query" + suffix.substr(suffix.size() - 8);
        fixture_user.email = fixture_user.name + "@example.test";
        fixture_user.password_hash = "test";
        fixture_user.password_algo = "test";
        fixture_user.create_time = now;
        fixture_user.last_login = now;
        fixture_user.password_update_at = now;
        database.persist(fixture_user);
        fixture_user_id = fixture_user.uid;
        for (const auto& entry : std::vector<std::pair<std::string, bool>>{
                 {visible_id, true}, {hidden_id, false}}) {
            oj::db::Question question{};
            question.id = entry.first;
            question.title = entry.second ? "visible regression" : "hidden regression";
            question.desc = "query regression";
            question.star = "easy";
            question.create_time = now;
            question.update_time = now;
            question.cpu_limit = 1;
            question.memory_limit = 64;
            question.visible = entry.second;
            database.persist(question);
        }
        for (int index = 0; index < 3; ++index) {
            oj::db::Solution solution{};
            solution.question_id = visible_id;
            solution.user_id = fixture_user_id;
            solution.title = "visible " + std::to_string(index);
            solution.content_md = "content";
            solution.status = "approved";
            solution.created_at = now;
            solution.updated_at = now;
            database.persist(solution);
            if (index == 0) visible_solution_id = solution.id;
        }
        oj::db::Solution hidden{};
        hidden.question_id = hidden_id;
        hidden.user_id = fixture_user_id;
        hidden.title = "hidden";
        hidden.content_md = "content";
        hidden.status = "approved";
        hidden.created_at = now;
        hidden.updated_at = now;
        database.persist(hidden);
        hidden_solution_id = hidden.id;

        oj::db::Solution pending{};
        pending.question_id = visible_id;
        pending.user_id = fixture_user_id;
        pending.title = "pending";
        pending.content_md = "content";
        pending.status = "pending";
        pending.created_at = now;
        pending.updated_at = now;
        database.persist(pending);
        pending_solution_id = pending.id;

        oj::db::Comment top{};
        top.solution_id = visible_solution_id;
        top.user_id = fixture_user_id;
        top.content = "nullable top level";
        database.persist(top);
        top_comment_id = top.id;

        oj::db::Comment hidden_comment{};
        hidden_comment.solution_id = hidden_solution_id;
        hidden_comment.user_id = fixture_user_id;
        hidden_comment.content = "hidden comment";
        database.persist(hidden_comment);
        hidden_comment_id = hidden_comment.id;

        oj::db::Comment pending_comment{};
        pending_comment.solution_id = pending_solution_id;
        pending_comment.user_id = fixture_user_id;
        pending_comment.content = "pending comment";
        database.persist(pending_comment);
        pending_comment_id = pending_comment.id;

        for (int index = 0; index < 5; ++index) {
            oj::db::Submission submission{};
            submission.user_id = fixture_user_id;
            submission.question_id = visible_id;
            submission.status = index % 2 == 0 ? "ACCEPTED" : "WRONG_ANSWER";
            submission.created_at = now;
            database.persist(submission);
        }
        transaction.commit();
    }

    Check(oj::model::ModelBase::StartDatabase("data_query_regression_test"),
          "model database starts");
    oj::control::Control control;

    oj::db::Question question{};
    Check(control.Question().GetOneQuestion(visible_id, question) &&
              question.title == "visible regression",
          "question cache miss returns the database row");
    Check(!control.Question().GetOneQuestion(hidden_id, question),
          "public question detail hides invisible rows");
    Check(control.GetModel()->Question().GetOneQuestion(hidden_id, question, true) && !question.visible,
          "admin question detail includes invisible rows");

    auto hidden_query = std::make_shared<QuestionQuery>();
    hidden_query->title = "hidden regression";
    std::shared_ptr<QueryStruct> raw_hidden_query = hidden_query;
    auto public_key = control.Question().BuildListCacheKey(
        raw_hidden_query, 1, 20, control.Question().GetQuestionsListVersion() + suffix,
        oj::cache::Cache::CacheKey::PageType::kData);
    std::vector<oj::db::Question> questions;
    int question_total = 0;
    int question_pages = 0;
    Check(control.Question().GetQuestionsByPage(
              public_key, questions, &question_total, &question_pages) && question_total == 0,
          "public question list count excludes invisible rows");
    auto admin_key = control.GetModel()->Question().BuildListCacheKey(
        raw_hidden_query, 1, 20, suffix, oj::cache::Cache::CacheKey::PageType::kData);
    Check(control.GetModel()->Question().GetQuestionsByPage(
              admin_key, questions, &question_total, &question_pages, true) &&
              question_total == 1 && questions.size() == 1 && !questions[0].visible,
          "admin question list includes invisible rows");

    std::vector<oj::db::Solution> solution_list;
    int solution_total = 0;
    std::string error;
    Check(!control.Solution().GetSolutionList(hidden_id, "approved", "latest", 1, 20,
                                               &solution_list, &solution_total, &error) &&
              error == "QUESTION_NOT_FOUND",
          "public solution list hides solutions for invisible questions");
    oj::db::Solution detail{};
    oj::db::User author{};
    error.clear();
    Check(!control.Solution().GetSolutionDetail(hidden_solution_id, &detail, &author, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "public solution detail hides solutions for invisible questions");
    error.clear();
    Check(!control.Solution().GetSolutionDetail(pending_solution_id, &detail, &author, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "public solution detail hides unapproved solutions");

    std::vector<oj::db::Solution> pending_list;
    error.clear();
    Check(control.Solution().GetSolutionList(visible_id, "pending", "latest", 1, 20,
                                              &pending_list, &solution_total, &error) &&
              solution_total == 3,
          "public solution list cannot request unapproved status");

    std::vector<oj::db::Solution> solutions;
    int total = 0;
    int pages = 0;
    Check(control.GetModel()->Solution().GetSolutionsByPage(
               visible_id, "approved", "latest", 1, 10, &solutions, &total, &pages) &&
              solutions.size() == 3 && solutions[0].id > solutions[1].id,
          "latest solutions are ordered descending");
    int all_solution_count = 0;
    Check(control.GetModel()->Solution().GetSolutionCount("", &all_solution_count) &&
              all_solution_count >= 4,
          "admin solution overview uses an unfiltered count");
    oj::db::Solution loaded{};
    Check(control.GetModel()->Solution().GetSolutionById(visible_solution_id, &loaded) &&
              loaded.title == "visible 0",
          "solution cache miss returns the database row");
    Check(control.GetModel()->Solution().GetSolutionById(pending_solution_id, &loaded) &&
              loaded.status == "pending",
          "admin model path explicitly retains unapproved solution access");

    std::vector<oj::db::Comment> comments;
    int comment_total = 0;
    error.clear();
    Check(control.Comment().GetTopLevelComments(visible_solution_id, 1, 1000,
                                                &comments, &comment_total, &error) &&
              comment_total == 1 && comments.size() == 1 &&
              comments[0].parent_id.null() && comments[0].created_at.null(),
          "nullable comments remain absent in typed control results");
    oj::db::Comment loaded_comment{};
    Check(control.GetModel()->Comment().GetCommentById(top_comment_id, &loaded_comment) &&
              loaded_comment.parent_id.null() && loaded_comment.created_at.null(),
          "nullable comment detail survives cold cache serialization");
    loaded_comment.parent_id = 123;
    loaded_comment.created_at = Now();
    Check(control.GetModel()->Comment().GetCommentById(top_comment_id, &loaded_comment) &&
              loaded_comment.parent_id.null() && loaded_comment.created_at.null(),
          "nullable comment detail survives hot cache serialization");

    oj::db::User user{};
    user.uid = static_cast<int>(fixture_user_id);
    oj::control::ActionState action;
    error.clear();
    Check(!control.Solution().ToggleLike(hidden_solution_id, user, &action, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "solution like rejects hidden question content");
    error.clear();
    Check(!control.Solution().ToggleFavorite(pending_solution_id, user, &action, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "solution favorite rejects unapproved content");
    error.clear();
    Check(!control.Solution().GetUserSolutionActions(hidden_solution_id, user.uid, &action, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "solution action state rejects hidden question content");

    error.clear();
    Check(!control.Comment().GetTopLevelComments(
              hidden_solution_id, 1, 20, &comments, &comment_total, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment list rejects hidden question content");
    error.clear();
    Check(!control.Comment().GetCommentReplies(
              hidden_comment_id, 1, 20, &comments, &comment_total, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment replies reject hidden question content");
    error.clear();
    oj::db::Comment comment_result{};
    Check(!control.Comment().PostComment(hidden_solution_id, user, "blocked", &comment_result, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment creation rejects hidden question content");
    error.clear();
    Check(!control.Comment().EditComment(pending_comment_id, user, "blocked", &comment_result, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment update rejects unapproved solution content");
    oj::db::User non_admin{};
    non_admin.uid = std::numeric_limits<int32_t>::max();
    error.clear();
    Check(!control.Comment().DeleteComment(hidden_comment_id, non_admin, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment deletion rejects hidden content before ownership checks");
    error.clear();
    Check(!control.Comment().ToggleCommentLike(hidden_comment_id, user, &action, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment like rejects hidden question content");
    error.clear();
    Check(!control.Comment().ToggleCommentFavorite(pending_comment_id, user, &action, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment favorite rejects unapproved solution content");
    error.clear();
    std::map<unsigned long long, oj::control::ActionState> comment_actions;
    Check(!control.Comment().GetCommentActions(
              {hidden_comment_id}, user.uid, &comment_actions, &error) &&
              error == "SOLUTION_NOT_FOUND",
          "comment action state rejects hidden question content");

    Json::Value cached_question(Json::objectValue);
    cached_question["id"] = visible_id;
    cached_question["title"] = "cached";
    cached_question["star"] = "easy";
    cached_question["desc"] = "cached";
    cached_question["cpu_limit"] = 1;
    cached_question["memory_limit"] = 1024;
    cached_question["visible"] = true;
    cached_question["create_time"] = Json::Int64(1);
    cached_question["update_time"] = Json::Int64(2);
    Json::FastWriter writer;
    Check(oj::cache::Cache::ParseDetailedQuestion(
              writer.write(cached_question), visible_id, &question),
          "valid question cache payload is accepted");
    cached_question.removeMember("visible");
    Check(!oj::cache::Cache::ParseDetailedQuestion(
              writer.write(cached_question), visible_id, &question),
          "question cache payload missing visible is a miss");
    cached_question["visible"] = "true";
    Check(!oj::cache::Cache::ParseDetailedQuestion(
              writer.write(cached_question), visible_id, &question),
          "question cache payload with invalid visible type is a miss");
    cached_question["visible"] = true;
    cached_question["id"] = hidden_id;
    Check(!oj::cache::Cache::ParseDetailedQuestion(
              writer.write(cached_question), visible_id, &question),
          "question cache payload with mismatched id is a miss");

    std::vector<oj::db::Submission> submissions;
    uint64_t submission_total = 0;
    Check(control.GetModel()->Submission().ListSubmissions(
              fixture_user_id, visible_id, "", 2, 2, &submissions, &submission_total) &&
              submission_total == 5 && submissions.size() == 2 &&
              submissions[0].submission_id > submissions[1].submission_id,
          "submission count and ordered database pagination are stable");

    EraseFixture(database, visible_id, hidden_id);
    {
        odb::transaction transaction(database.begin());
        database.erase<oj::db::User>(fixture_user_id);
        transaction.commit();
    }
    oj::model::ModelBase::StopDatabase();
    std::cout << "data_query_regression_test: PASS\n";
    return 0;
}
