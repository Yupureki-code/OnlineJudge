#pragma once

#include "comm.hpp"
#include "model_base.hpp"

#include <cstdint>
#include <cstdlib>
#include <limits>
#include <mutex>
#include <stdexcept>
#include "../../../comm/models/question.hxx"
#include "../../../comm/models/gen/question-odb.hxx"
#include "../../../comm/models/test_case.hxx"
#include "../../../comm/models/model_counts.hxx"
#include "../../../comm/models/gen/test_case-odb.hxx"
#include "../../../comm/models/gen//model_counts-odb.hxx"

namespace oj::model
{
    using namespace oj::db;
    class ModelQuestion : public ModelBase
    {
    private:
        using ODBQuestion = oj::db::Question;
        using ODBQuestionQuery = odb::query<ODBQuestion>;
        using ODBTestCase = oj::db::TestCase;
        using ODBTestCaseQuery = odb::query<ODBTestCase>;

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

        inline static std::mutex _question_mutation_mutex;

        std::shared_ptr<QuestionQuery> NormalizeQuestionQuery(const std::shared_ptr<QueryStruct>& raw_query)
        {
            auto raw = std::dynamic_pointer_cast<QuestionQuery>(raw_query);
            auto normalized = std::make_shared<QuestionQuery>();
            normalized->id = TrimCopy(raw ? raw->id : "");
            normalized->title = TrimCopy(raw ? raw->title : "");
            normalized->star = LowerAscii(TrimCopy(raw ? raw->star : ""));

            if (!normalized->id.empty() && !IsAllDigits(normalized->id))
            {
                normalized->id.clear();
            }

            return normalized;
        }

        template <typename Query>
        Query BuildQuestionQuery(const std::shared_ptr<QueryStruct>& query_hash)
        {
            auto q = std::dynamic_pointer_cast<QuestionQuery>(query_hash);
            if (!q)
                return Query(true);

            Query condition(true);
            if (!q->id.empty() && !q->title.empty() && q->id == q->title && IsAllDigits(q->id))
            {
                condition = condition &&
                    (Query::id == q->id || Query::title.like("%" + q->title + "%"));
            }
            else
            {
                if (!q->id.empty())
                    condition = condition && (Query::id == q->id);
                if (!q->title.empty())
                    condition = condition && Query::title.like("%" + q->title + "%");
            }

            if (!q->star.empty())
            {
                std::string difficulty = q->star;
                if (difficulty == "simple") difficulty = "简单";
                else if (difficulty == "medium") difficulty = "中等";
                else if (difficulty == "hard") difficulty = "困难";

                if (difficulty != "all" && difficulty != "ALL" && difficulty != "全部")
                    condition = condition && (Query::star == difficulty);
            }
            return condition;
        }

        static Json::Value SampleTestJson(const ODBTestCase& test)
        {
            Json::Value item;
            item["id"] = static_cast<int>(test.id);
            item["question_id"] = test.question_id;
            item["input"] = test.input;
            item["output"] = test.output;
            item["is_sample"] = test.is_sample;
            return item;
        }

        static Json::Value TestJson(const ODBTestCase& test)
        {
            Json::Value item;
            item["id"] = static_cast<int>(test.id);
            item["question_id"] = test.question_id;
            item["in"] = test.input;
            item["out"] = test.output;
            item["is_sample"] = test.is_sample;
            return item;
        }

    public:
        //题库:得到一页内全部的题目
        bool GetQuestionsByPage(std::shared_ptr<Cache::CacheListKey> key,
                                std::vector<Question>& questions,
                                int* total_count,
                                int* total_pages)
        {
            auto begin = std::chrono::steady_clock::now();
            if (!key || key->GetSize() <= 0 || total_count == nullptr || total_pages == nullptr)
                return false;

            if (_cache.GetQuestionsByPage(key, questions, total_count, total_pages))
            {
                LOG_INFO("{}{}", "Cache hit for question list page ", key->GetCacheKeyString(&_cache));
                const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(RecordActionType::Question, true, false, cost_ms);
                return true;
            }

            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                using CountQuery = odb::query<oj::db::QuestionCount>;
                std::unique_ptr<oj::db::QuestionCount> count;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBCount");
                    count.reset(database->query_one<oj::db::QuestionCount>(
                        BuildQuestionQuery<CountQuery>(key->GetQueryHash())));
                }
                if (!count || count->value > static_cast<uint64_t>(std::numeric_limits<int>::max()))
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBRollback");
                    transaction->Rollback();
                    return false;
                }
                *total_count = static_cast<int>(count->value);
                if (*total_count <= 0)
                {
                    *total_pages = 0;
                    questions.clear();
                }
                else
                {
                    const int size = key->GetSize();
                    *total_pages = (*total_count - 1) / size + 1;
                    const int safe_page = std::max(1, std::min(key->GetPage(), *total_pages));
                    const uint64_t offset = static_cast<uint64_t>(safe_page - 1) *
                                            static_cast<uint64_t>(size);
                    ODBQuestionQuery page_query =
                        BuildQuestionQuery<ODBQuestionQuery>(key->GetQueryHash());
                    page_query += " ORDER BY CAST(id AS UNSIGNED) ASC, id ASC LIMIT " +
                                  std::to_string(size) + " OFFSET " + std::to_string(offset);
                    odb::result<ODBQuestion> result;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBQuery");
                        result = database->query<ODBQuestion>(page_query, false);
                    }
                    questions.clear();
                    questions.reserve(static_cast<size_t>(size));
                    {
                        for (const auto& item : result)
                            questions.push_back(item);
                    }
                }

                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBCommit");
                    transaction->Commit();
                }

                if (*total_count <= 0)
                {
                    _cache.SetQuestionsByPage(key, questions, *total_count, *total_pages);
                }
                else
                {
                    const int safe_page = std::max(1, std::min(key->GetPage(), *total_pages));
                    auto write_key = _cache.BuildListCacheKey(
                        key->GetQueryHash(), safe_page, key->GetSize(), key->GetListVersion(),
                        Cache::CacheKey::PageType::kData, key->GetListType());
                    _cache.SetQuestionsByPage(write_key, questions, *total_count, *total_pages);
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionsByPage.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB question list rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB question list query failed: ", e.what());
                return false;
            }

            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        //题目:获得单个题目
        bool GetOneQuestion(const std::string& id, Question& q)
        {
            auto begin = std::chrono::steady_clock::now();
            if (!IsAllDigits(id))
                return false;

            auto detail_key = _cache.BuildDetailCacheKey(id, Cache::CacheKey::PageType::kData);
            if (_cache.GetDetailedQuestion(detail_key, q))
            {
                LOG_INFO("{}{}", "Cache hit for question ", id);
                const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(RecordActionType::Question, true, false, cost_ms);
                return true;
            }

            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetOneQuestion.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetOneQuestion.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                ODBQuestion item;
                bool found = false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetOneQuestion.ODBFind");
                    found = database->find<ODBQuestion>(id, item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetOneQuestion.ODBCommit");
                    transaction->Commit();
                }
                if (!found)
                {
                    _cache.SetQuestionNotFound(detail_key, id);
                    return false;
                }
                _cache.SetDetailedQuestion(detail_key, item);
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetOneQuestion.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB question lookup rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB question lookup failed: ", e.what());
                return false;
            }

            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        //题目写接口:新增或修改题目
        bool SaveQuestion(const Question& input)
        {
            if ( input.cpu_limit < std::numeric_limits<int8_t>::min() ||
                input.cpu_limit > std::numeric_limits<int8_t>::max())
                return false;

            std::lock_guard<std::mutex> mutation_lock(_question_mutation_mutex);
            auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            ODBQuestion stored;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                bool exists = false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBFind");
                    exists = database->find<ODBQuestion>(input.id, stored);
                }

                const auto now = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                if (exists)
                {
                    stored.title = input.title;
                    stored.desc = input.desc;
                    stored.star = input.star;
                    stored.cpu_limit = static_cast<int8_t>(input.cpu_limit);
                    stored.memory_limit = input.memory_limit;
                    stored.update_time = now;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBUpdate");
                        database->update(stored);
                    }
                }
                else
                {
                    stored.id = input.id;
                    stored.title = input.title;
                    stored.desc = input.desc;
                    stored.star = input.star;
                    stored.cpu_limit = static_cast<int8_t>(input.cpu_limit);
                    stored.memory_limit = input.memory_limit;
                    stored.create_time = now;
                    stored.update_time = now;
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBPersist");
                        database->persist(stored);
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBCommit");
                    transaction->Commit();
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.SaveQuestion.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB question save rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB question save failed: ", e.what());
                return false;
            }
            auto detail_key = _cache.BuildDetailCacheKey(input.id, Cache::CacheKey::PageType::kData);
            _cache.SetDetailedQuestion(detail_key, stored);
            auto detail_html_key = _cache.BuildDetailCacheKey(input.id, Cache::CacheKey::PageType::kHtml);
            _cache.InvalidatePage(detail_html_key);
            _cache.DeleteStringByAnyKey("question_counts");
            TouchQuestionListVersion();
            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        //删除题目
        bool DeleteQuestion(const std::string& id)
        {
            if (!IsAllDigits(id))
                return false;

            std::lock_guard<std::mutex> mutation_lock(_question_mutation_mutex);
            auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteQuestion.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteQuestion.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                ODBQuestion item;
                bool found = false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteQuestion.ODBFind");
                    found = database->find<ODBQuestion>(id, item);
                }
                if (found)
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteQuestion.ODBErase");
                    database->erase(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteQuestion.ODBCommit");
                    transaction->Commit();
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteQuestion.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB question delete rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB question delete failed: ", e.what());
                return false;
            }

            auto detail_key = _cache.BuildDetailCacheKey(id, Cache::CacheKey::PageType::kData);
            _cache.SetQuestionNotFound(detail_key, id);
            auto detail_html_key = _cache.BuildDetailCacheKey(id, Cache::CacheKey::PageType::kHtml);
            _cache.InvalidatePage(detail_html_key);
            _cache.DeleteStringByAnyKey("question_counts");
            TouchQuestionListVersion();
            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        //得到题目数量
        bool GetQuestionCount(int* count)
        {
            if (count == nullptr)
                return false;
            auto begin = std::chrono::steady_clock::now();
            std::string value;
            if (_cache.GetStringByAnyKey("question_counts", &value))
            {
                *count = std::atoi(value.c_str());
                LOG_INFO("{}", "Cache hit for question count");
                const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count();
                RecordCacheMetrics(RecordActionType::Question, true, false, cost_ms);
                return true;
            }

            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionCount.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionCount.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }
                std::unique_ptr<oj::db::QuestionCount> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionCount.ODBQuery");
                    result.reset(database->query_one<oj::db::QuestionCount>());
                }
                if (!result || result->value > static_cast<uint64_t>(std::numeric_limits<int>::max()))
                    throw std::overflow_error("question count exceeds legacy int range");
                *count = static_cast<int>(result->value);
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionCount.ODBCommit");
                    transaction->Commit();
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetQuestionCount.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB question count rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB question count failed: ", e.what());
                return false;
            }

            _cache.SetStringByAnyKey("question_counts", std::to_string(*count),
                                     _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        // 题目写路径完成后调用该接口，统一触发列表版本递增。
        std::string TouchQuestionListVersion()
        {
            std::string version = _cache.BumpListVersion(ListType::Questions);
            LOG_INFO("{}{}", "Question list version bumped to ", version);
            return version;
        }

        // 获得题目列表版本
        std::string GetQuestionsListVersion()
        {
            return _cache.GetListVersion(ListType::Questions);
        }

        std::shared_ptr<Cache::CacheListKey> BuildListCacheKey(std::shared_ptr<QueryStruct>& query_hash,
                                                               int page, int size,
                                                               const std::string& list_version,
                                                               Cache::CacheKey::PageType page_type)
        {
            auto normalized_query = NormalizeQuestionQuery(query_hash);
            return _cache.BuildListCacheKey(normalized_query, page, size, list_version,
                                            page_type, ListType::Questions);
        }

        std::shared_ptr<Cache::CacheDetailKey> BuildDetailCacheKey(
            const std::string& id, Cache::CacheKey::PageType page_type)
        {
            return _cache.BuildDetailCacheKey(id, page_type);
        }

        std::shared_ptr<Cache::CacheStaticKey> BuildHtmlCacheKey(
            const std::string& page_name, Cache::CacheKey::PageType page_type)
        {
            return _cache.BuildStaticCacheKey(page_name, page_type);
        }

        // 通过题目id获取该题目的样例
        bool GetSampleTestsByQuestionId(const std::string& question_id, Json::Value* tests)
        {
            if (tests == nullptr)
                return false;
            auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetSampleTestsByQuestionId.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetSampleTestsByQuestionId.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                std::vector<ODBTestCase> matched;
                odb::result<ODBTestCase> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetSampleTestsByQuestionId.ODBQuery");
                    result = database->query<ODBTestCase>(
                        ODBTestCaseQuery::question_id == question_id && ODBTestCaseQuery::is_sample == true);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetSampleTestsByQuestionId.ODBLazyIteration");
                    for (const auto& item : result)
                        matched.push_back(item);
                }
                std::sort(matched.begin(), matched.end(), [](const ODBTestCase& lhs, const ODBTestCase& rhs) {
                    return lhs.id < rhs.id;
                });
                tests->clear();
                for (const auto& item : matched)
                    tests->append(SampleTestJson(item));
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetSampleTestsByQuestionId.ODBCommit");
                    transaction->Commit();
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetSampleTestsByQuestionId.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB sample test rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB sample test query failed: ", e.what());
                return false;
            }
            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        // 通过题目id获取该题目的测试用例
        bool GetTestsByQuestionId(const std::string& question_id, Json::Value* result)
        {
            if (result == nullptr)
                return false;
            auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestsByQuestionId.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestsByQuestionId.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                std::vector<ODBTestCase> matched;
                odb::result<ODBTestCase> tests;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestsByQuestionId.ODBQuery");
                    tests = database->query<ODBTestCase>(ODBTestCaseQuery::question_id == question_id);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestsByQuestionId.ODBLazyIteration");
                    for (const auto& item : tests)
                        matched.push_back(item);
                }
                std::sort(matched.begin(), matched.end(), [](const ODBTestCase& lhs, const ODBTestCase& rhs) {
                    return lhs.id < rhs.id;
                });
                Json::Value test_array(Json::arrayValue);
                for (const auto& item : matched)
                    test_array.append(TestJson(item));
                (*result)["tests"] = test_array;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestsByQuestionId.ODBCommit");
                    transaction->Commit();
                }
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestsByQuestionId.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB test list rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB test list query failed: ", e.what());
                return false;
            }
            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        // Feature 2.5: Get a single test case by business ordinal.
        bool GetTestById(int test_id, const std::string& question_id,
                         std::string* test_input, std::string* test_output)
        {
            if (test_input == nullptr || test_output == nullptr ||
                !IsAllDigits(question_id) || test_id <= 0 ||
                test_id > std::numeric_limits<int8_t>::max())
                return false;
            auto begin = std::chrono::steady_clock::now();
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestById.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestById.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                bool found = false;
                ODBTestCase matched{};
                odb::result<ODBTestCase> tests;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestById.ODBQuery");
                    tests = database->query<ODBTestCase>(
                        ODBTestCaseQuery::question_id == question_id &&
                        ODBTestCaseQuery::id == static_cast<int8_t>(test_id));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestById.ODBLazyIteration");
                    for (const auto& item : tests)
                    {
                        matched = item;
                        found = true;
                        break;
                    }
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestById.ODBCommit");
                    transaction->Commit();
                }
                if (!found)
                    return false;
                *test_input = matched.input;
                *test_output = matched.output;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.GetTestById.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB test lookup rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB test lookup failed: ", e.what());
                return false;
            }
            const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Question, false, true, cost_ms);
            return true;
        }

        bool CreateTestCase(const std::string& question_id,
                            const std::string& input,
                            const std::string& output,
                            bool is_sample,
                            int* test_id = nullptr)
        {
            if (!IsAllDigits(question_id))
                return false;
            std::lock_guard<std::mutex> mutation_lock(_question_mutation_mutex);
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                int next_id = 1;
                odb::result<ODBTestCase> tests;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBQuery");
                    tests = database->query<ODBTestCase>(ODBTestCaseQuery::question_id == question_id);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBLazyIteration");
                    for (const auto& item : tests)
                        next_id = std::max(next_id, static_cast<int>(item.id) + 1);
                }
                if (next_id > std::numeric_limits<int8_t>::max())
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBCommit");
                    transaction->Commit();
                    return false;
                }

                ODBTestCase item{};
                item.id = static_cast<int8_t>(next_id);
                item.question_id = question_id;
                item.input = input;
                item.output = output;
                item.is_sample = is_sample;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBPersist");
                    database->persist(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBCommit");
                    transaction->Commit();
                }
                if (test_id != nullptr)
                    *test_id = next_id;
                return true;
            }
            catch (const std::exception& e)
            {
                if (transaction)
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.CreateTestCase.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB test create rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB test create failed: ", e.what());
                return false;
            }
        }

        bool UpdateTestCase(int test_id,
                            const std::string& question_id,
                            const std::string& input,
                            const std::string& output,
                            bool is_sample)
        {
            if (!IsAllDigits(question_id) || test_id <= 0 ||
                test_id > std::numeric_limits<int8_t>::max())
                return false;
            std::lock_guard<std::mutex> mutation_lock(_question_mutation_mutex);
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                bool found = false;
                ODBTestCase item{};
                odb::result<ODBTestCase> tests;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBQuery");
                    tests = database->query<ODBTestCase>(
                        ODBTestCaseQuery::question_id == question_id &&
                        ODBTestCaseQuery::id == static_cast<int8_t>(test_id));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBLazyIteration");
                    for (const auto& current : tests)
                    {
                        item = current;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBCommit");
                    transaction->Commit();
                    return false;
                }
                item.input = input;
                item.output = output;
                item.is_sample = is_sample;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBUpdate");
                    database->update(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBCommit");
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
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.UpdateTestCase.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB test update rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB test update failed: ", e.what());
                return false;
            }
        }

        bool DeleteTestCase(int test_id, const std::string& question_id)
        {
            if (!IsAllDigits(question_id) || test_id <= 0 ||
                test_id > std::numeric_limits<int8_t>::max())
                return false;
            std::lock_guard<std::mutex> mutation_lock(_question_mutation_mutex);
            DatabaseHandle database;
            std::unique_ptr<ns_odb::ScopedTransaction> transaction;
            try
            {
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBAcquire");
                    database = AcquireDatabase();
                }
                if (database.Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBBegin");
                    transaction = std::make_unique<ns_odb::ScopedTransaction>(*database);
                }

                bool found = false;
                ODBTestCase item{};
                odb::result<ODBTestCase> tests;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBQuery");
                    tests = database->query<ODBTestCase>(
                        ODBTestCaseQuery::question_id == question_id &&
                        ODBTestCaseQuery::id == static_cast<int8_t>(test_id));
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBLazyIteration");
                    for (const auto& current : tests)
                    {
                        item = current;
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBErase");
                    database->erase(item);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBCommit");
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
                        latecyMonitor::Timer timer(Monitor(), "ModelQuestion.DeleteTestCase.ODBRollback");
                        transaction->Rollback();
                    }
                    catch (const std::exception& rollback_error)
                    {
                        LOG_ERROR("ODB test delete rollback failed: {}", rollback_error.what());
                    }
                }
                LOG_ERROR("{}{}", "ODB test delete failed: ", e.what());
                return false;
            }
        }
    };
}
