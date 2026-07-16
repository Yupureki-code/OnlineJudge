#pragma once

#include "comm.hpp"
#include "model_base.hpp"
#include "../../../comm/models/solution.hxx"
#include "../../../comm/models/model_counts.hxx"
#include "../../../comm/models/gen/solution-odb.hxx"
#include "../../../comm/models/gen/model_counts-odb.hxx"
#include "../../../comm/models/solution_action.hxx"
#include "../../../comm/models/gen/solution_action-odb.hxx"
#include <limits>

namespace oj::model
{
    using namespace oj::db;
    class ModelSolution : public ModelBase
    {
    private:
        template <typename Query>
        static Query BuildSolutionQuery(const std::string& question_id,
                                        const std::string& status_filter)
        {
            Query filter(Query::question_id == question_id);
            if (!status_filter.empty())
                filter = filter && Query::status == status_filter;
            return filter;
        }

    public:
        std::string SolutionStatusToDbString(SolutionStatus status)
        {
            switch (status)
            {
                case SolutionStatus::pending:
                    return "pending";
                case SolutionStatus::approved:
                    return "approved";
                case SolutionStatus::rejected:
                    return "rejected";
                default:
                    return "approved";
            }
        }

        SolutionStatus DbStringToSolutionStatus(const std::string& s)
        {
            if (s == "pending") return SolutionStatus::pending;
            if (s == "rejected") return SolutionStatus::rejected;
            return SolutionStatus::approved;
        }

        //创建题解
        bool CreateSolution(const Solution& input, unsigned long long* solution_id = nullptr)
        {
            //参数校验
            if (input.question_id.empty() || input.user_id <= 0)
            {
                return false;
            }
            std::string trimmed_title = TrimCopy(input.title);
            std::string trimmed_content = TrimCopy(input.content_md);
            if (trimmed_title.empty() || trimmed_content.empty())
            {
                return false;
            }

            auto begin = std::chrono::steady_clock::now();
            std::unique_ptr<ns_odb::ScopedDB> database;
            std::unique_ptr<odb::transaction> transaction;
            try
            {
                database = std::make_unique<ns_odb::ScopedDB>(AcquireDatabase());
                if (database->Get() == nullptr)
                    return false;

                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.CreateSolution.ODBBegin");
                    transaction = std::make_unique<odb::transaction>((*database)->begin());
                }

                const auto now = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                oj::db::Solution record{};
                record.question_id = input.question_id;
                record.user_id = static_cast<uint32_t>(input.user_id);
                record.title = trimmed_title;
                record.content_md = trimmed_content;
                record.like_count = 0;
                record.favorite_count = 0;
                record.comment_count = 0;
                record.status = input.status;
                record.created_at = now;
                record.updated_at = now;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.CreateSolution.ODBPersist");
                    (*database)->persist(record);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.CreateSolution.ODBCommit");
                    transaction->commit();
                }
                if (solution_id != nullptr)
                    *solution_id = record.id;
            }
            catch (const std::exception& error)
            {
                if (transaction && !transaction->finalized())
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.CreateSolution.ODBRollback");
                        transaction->rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB创建题解失败: ", error.what());
                return false;
            }

            InvalidateSolutionListCaches(input.question_id);
            LOG_INFO("{}{}", "Invalidated solution list cache for question ", input.question_id);

            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Solution, false, true, cost_ms);
            return true;
        }
        //获取分页的题解列表
        bool GetSolutionsByPage(const std::string& question_id,
                                const std::string& status_filter,
                                const std::string& sort_order,
                                int page,
                                int size,
                                std::vector<Solution>* solutions,
                                int* total_count,
                                int* total_pages)
        {
            if (solutions == nullptr || total_count == nullptr || total_pages == nullptr)
            {
                return false;
            }
            auto _metrics_begin = std::chrono::steady_clock::now();

            solutions->clear();
            *total_count = 0;
            *total_pages = 0;
            if (!status_filter.empty() && status_filter != "pending" &&
                status_filter != "approved" && status_filter != "rejected")
                return false;
            if (!sort_order.empty() && sort_order != "latest" && sort_order != "hot")
                return false;

            const int safe_page = std::max(1, page);
            const int safe_size = std::max(1, std::min(size, 50));
            SolutionStatus cache_status = SolutionStatus::approved;
            if (status_filter == "pending")
                cache_status = SolutionStatus::pending;
            else if (status_filter == "rejected")
                cache_status = SolutionStatus::rejected;
            const SolutionSort cache_sort = sort_order == "hot"
                ? SolutionSort::hot : SolutionSort::latest;
            const std::string list_version = _cache.GetSolutionListVersion(question_id);
            auto cache_key = _cache.BuildSolutionCacheKey(
                question_id, page, size, list_version, Cache::CacheKey::PageType::kList,
                cache_status, cache_sort);

            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), &cached_json))
            {
                Json::CharReaderBuilder builder;
                Json::Value root;
                std::istringstream stream(cached_json);
                if (Json::parseFromStream(builder, stream, &root, nullptr))
                {
                    if (root.isMember("total"))
                        *total_count = root["total"].asInt();
                    if (root.isMember("total_pages"))
                        *total_pages = root["total_pages"].asInt();
                    if (root.isMember("solutions") && root["solutions"].isArray())
                    {
                        for (const auto& item : root["solutions"])
                        {
                            Solution solution;
                            solution.id = item["id"].asUInt64();
                            solution.question_id = item["question_id"].asString();
                            solution.user_id = item["user_id"].asInt();
                            solution.title = item["title"].asString();
                            solution.content_md = item["content_md"].asString();
                            solution.like_count = item["like_count"].asUInt();
                            solution.favorite_count = item["favorite_count"].asUInt();
                            solution.comment_count = item["comment_count"].asUInt();
                            solution.status = item["status"].asString();
                            solution.created_at = oj::util::TimeUtil::IntToDateTime(item["created_at"].asInt64());
                            solution.updated_at = oj::util::TimeUtil::IntToDateTime(item["updated_at"].asInt64());
                            solutions->push_back(std::move(solution));
                        }
                    }
                    LOG_INFO("{}{}", "Cache hit for solution list ",
                             cache_key->GetCacheKeyString(&_cache));
                    const long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - _metrics_begin).count();
                    RecordCacheMetrics(RecordActionType::Solution, true, false, cost_ms);
                    return true;
                }
            }

            std::unique_ptr<ns_odb::ScopedDB> database;
            std::unique_ptr<odb::transaction> transaction;
            try
            {
                database = std::make_unique<ns_odb::ScopedDB>(AcquireDatabase());
                if (database->Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionsByPage.ODBBegin");
                    transaction = std::make_unique<odb::transaction>((*database)->begin());
                }

                using CountQuery = odb::query<oj::db::SolutionCount>;
                std::unique_ptr<oj::db::SolutionCount> count;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionsByPage.ODBCount");
                    count.reset((*database)->query_one<oj::db::SolutionCount>(
                        BuildSolutionQuery<CountQuery>(question_id, status_filter)));
                }
                if (!count || count->value > static_cast<uint64_t>(std::numeric_limits<int>::max()))
                    throw std::overflow_error("solution count exceeds legacy int range");
                *total_count = static_cast<int>(count->value);
                *total_pages = *total_count == 0
                    ? 0 : (*total_count - 1) / safe_size + 1;
                const int effective_page = *total_pages > 0
                    ? std::min(safe_page, *total_pages) : safe_page;
                const uint64_t offset = static_cast<uint64_t>(effective_page - 1) *
                                        static_cast<uint64_t>(safe_size);

                using Query = odb::query<oj::db::Solution>;
                Query page_query = BuildSolutionQuery<Query>(question_id, status_filter);
                if (sort_order == "hot")
                    page_query += " ORDER BY like_count DESC, id ASC LIMIT ";
                else
                    page_query += " ORDER BY id ASC LIMIT ";
                page_query += std::to_string(safe_size) + " OFFSET " + std::to_string(offset);

                odb::result<oj::db::Solution> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionsByPage.ODBQuery");
                    result = (*database)->query<oj::db::Solution>(page_query, false);
                }
                solutions->reserve(static_cast<size_t>(safe_size));
                {
                    for (const auto& record : result)
                        solutions->push_back(record);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionsByPage.ODBCommit");
                    transaction->commit();
                }
            }
            catch (const std::exception& error)
            {
                if (transaction && !transaction->finalized())
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionsByPage.ODBRollback");
                        transaction->rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB题解列表查询失败: ", error.what());
                return false;
            }

            Json::Value root(Json::objectValue);
            root["total"] = *total_count;
            root["total_pages"] = *total_pages;
            Json::Value array(Json::arrayValue);
            for (const auto& solution : *solutions)
            {
                Json::Value item;
                item["id"] = static_cast<Json::UInt64>(solution.id);
                item["question_id"] = solution.question_id;
                item["user_id"] = solution.user_id;
                item["title"] = solution.title;
                item["content_md"] = solution.content_md;
                item["like_count"] = solution.like_count;
                item["favorite_count"] = solution.favorite_count;
                item["comment_count"] = solution.comment_count;
                item["status"] = solution.status;
                item["created_at"] = oj::util::TimeUtil::DateTimeToInt(solution.created_at);
                item["updated_at"] = oj::util::TimeUtil::DateTimeToInt(solution.updated_at);
                array.append(item);
            }
            root["solutions"] = array;
            Json::FastWriter writer;
            _cache.SetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), writer.write(root),
                                     _cache.BuildJitteredTtl(600, 120));
            LOG_INFO("{}{}", "Cache miss for solution list, written to cache ",
                     cache_key->GetCacheKeyString(&_cache));

            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _metrics_begin).count();
            RecordCacheMetrics(RecordActionType::Solution, false, true, cost_ms);

            return true;
        }

        bool GetSolutionById(long long solution_id, Solution* solution)
        {
            if (solution == nullptr || solution_id <= 0 ||
                static_cast<unsigned long long>(solution_id) > std::numeric_limits<uint32_t>::max())
            {
                return false;
            }
            auto _metrics_begin = std::chrono::steady_clock::now();

            //构造cache key
            auto cache_key = _cache.BuildSolutionDetailCacheKey(solution_id);

            // Try to read from cache
            std::string cached_json;
            if (_cache.GetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), &cached_json))
            {
                Json::CharReaderBuilder builder;
                Json::Value json_value;
                std::istringstream ss(cached_json);
                if (Json::parseFromStream(builder, ss, &json_value, nullptr))
                {
                    solution->id = json_value["id"].asUInt64();
                    solution->question_id = json_value["question_id"].asString();
                    solution->user_id = json_value["user_id"].asInt();
                    solution->title = json_value["title"].asString();
                    solution->content_md = json_value["content_md"].asString();
                    solution->like_count = json_value["like_count"].asUInt();
                    solution->favorite_count = json_value["favorite_count"].asUInt();
                    solution->comment_count = json_value["comment_count"].asUInt();
                    solution->status = json_value["status"].asString();
                    solution->created_at = oj::util::TimeUtil::IntToDateTime(json_value["created_at"].asInt64());
                    solution->updated_at = oj::util::TimeUtil::IntToDateTime(json_value["updated_at"].asInt64());
                    LOG_INFO("{}{}", "Cache hit for solution detail ", cache_key->GetCacheKeyString(&_cache));
                    long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - _metrics_begin).count();
                    RecordCacheMetrics(RecordActionType::Solution, true, false, cost_ms);
                    return true;
                }
            }

            std::unique_ptr<ns_odb::ScopedDB> database;
            std::unique_ptr<odb::transaction> transaction;
            try
            {
                database = std::make_unique<ns_odb::ScopedDB>(AcquireDatabase());
                if (database->Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionById.ODBBegin");
                    transaction = std::make_unique<odb::transaction>((*database)->begin());
                }
                std::unique_ptr<oj::db::Solution> record;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionById.ODBQuery");
                    record.reset((*database)->query_one<oj::db::Solution>(
                        odb::query<oj::db::Solution>::id == static_cast<uint32_t>(solution_id)));
                }
                if (!record)
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionById.ODBCommit");
                    transaction->commit();
                    return false;
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionById.ODBCommit");
                    transaction->commit();
                }
            }
            catch (const std::exception& error)
            {
                if (transaction && !transaction->finalized())
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetSolutionById.ODBRollback");
                        transaction->rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB题解详情查询失败: ", error.what());
                return false;
            }

            // Write to cache
            Json::Value json_value;
            json_value["id"] = static_cast<Json::UInt64>(solution->id);
            json_value["question_id"] = solution->question_id;
            json_value["user_id"] = solution->user_id;
            json_value["title"] = solution->title;
            json_value["content_md"] = solution->content_md;
            json_value["like_count"] = solution->like_count;
            json_value["favorite_count"] = solution->favorite_count;
            json_value["comment_count"] = solution->comment_count;
            json_value["status"] = solution->status;
            json_value["created_at"] = oj::util::TimeUtil::DateTimeToInt(solution->created_at);
            json_value["updated_at"] = oj::util::TimeUtil::DateTimeToInt(solution->updated_at);
            Json::FastWriter writer;
            std::string json_str = writer.write(json_value);
            _cache.SetStringByAnyKey(cache_key->GetCacheKeyString(&_cache), json_str,
                                     _cache.BuildJitteredTtl(600, 120));
            LOG_INFO("{}{}", "Cache miss for solution detail, written to cache ", cache_key->GetCacheKeyString(&_cache));
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - _metrics_begin).count();
            RecordCacheMetrics(RecordActionType::Solution, false, true, cost_ms);

            return true;
        }

        bool ToggleSolutionAction(long long solution_id, int user_id, const std::string& action_type,
                                   bool* now_active, unsigned int* new_count)
        {
            if (now_active == nullptr || new_count == nullptr)
            {
                return false;
            }

            if (action_type != "like" && action_type != "favorite")
                return false;
            if (solution_id <= 0 || user_id <= 0 ||
                static_cast<unsigned long long>(solution_id) > std::numeric_limits<uint32_t>::max())
                return false;

            auto begin = std::chrono::steady_clock::now();
            std::string question_id;
            std::unique_ptr<ns_odb::ScopedDB> database;
            std::unique_ptr<odb::transaction> transaction;
            try
            {
                database = std::make_unique<ns_odb::ScopedDB>(AcquireDatabase());
                if (database->Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBBegin");
                    transaction = std::make_unique<odb::transaction>((*database)->begin());
                }

                using SolutionQuery = odb::query<oj::db::Solution>;
                SolutionQuery solution_query(
                    SolutionQuery::id == static_cast<uint32_t>(solution_id));
                solution_query += " FOR UPDATE";
                std::unique_ptr<oj::db::Solution> solution;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBLockQuery");
                    solution.reset((*database)->query_one<oj::db::Solution>(solution_query));
                }
                if (!solution)
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBRollback");
                    transaction->rollback();
                    return false;
                }
                question_id = solution->question_id;

                using ActionQuery = odb::query<oj::db::SolutionAction>;
                const ActionQuery action_query(
                    ActionQuery::solution_id == static_cast<uint32_t>(solution_id) &&
                    ActionQuery::user_id == static_cast<uint32_t>(user_id) &&
                    ActionQuery::action_type == action_type);
                std::unique_ptr<oj::db::SolutionAction> action;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBActionQuery");
                    action.reset((*database)->query_one<oj::db::SolutionAction>(action_query));
                }

                uint32_t& count = action_type == "like"
                    ? solution->like_count : solution->favorite_count;
                if (action)
                {
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBErase");
                        (*database)->erase(*action);
                    }
                    if (count > 0)
                        --count;
                    *now_active = false;
                }
                else
                {
                    if (count == std::numeric_limits<uint32_t>::max())
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBRollback");
                        transaction->rollback();
                        return false;
                    }
                    oj::db::SolutionAction new_action{};
                    new_action.solution_id = static_cast<uint32_t>(solution_id);
                    new_action.user_id = static_cast<uint32_t>(user_id);
                    new_action.action_type = action_type;
                    new_action.created_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBPersist");
                        (*database)->persist(new_action);
                    }
                    ++count;
                    *now_active = true;
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBUpdate");
                    (*database)->update(*solution);
                }
                *new_count = count;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBCommit");
                    transaction->commit();
                }
            }
            catch (const std::exception& error)
            {
                if (transaction && !transaction->finalized())
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.ToggleSolutionAction.ODBRollback");
                        transaction->rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB切换题解交互失败: ", error.what());
                return false;
            }

            auto detail_key = _cache.BuildSolutionDetailCacheKey(solution_id);
            _cache.DeleteStringByAnyKey(detail_key->GetCacheKeyString(&_cache));
            InvalidateSolutionListCaches(question_id);

            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Solution, false, true, cost_ms);
                return true;
        }

        bool GetUserActionsForSolutions(int user_id, const std::vector<long long>& solution_ids,
                                        std::map<long long, std::map<std::string, bool>>& actions)
        {
            if (solution_ids.empty())
            {
                actions.clear();
                return true;
            }

            if (user_id < 0)
                return false;

            actions.clear();

            auto begin = std::chrono::steady_clock::now();
            std::vector<uint32_t> ids;
            ids.reserve(solution_ids.size());
            for (long long id : solution_ids)
            {
                if (id >= 0 && static_cast<unsigned long long>(id) <= std::numeric_limits<uint32_t>::max())
                    ids.push_back(static_cast<uint32_t>(id));
            }
            if (ids.empty())
                return true;

            std::unique_ptr<ns_odb::ScopedDB> database;
            std::unique_ptr<odb::transaction> transaction;
            try
            {
                database = std::make_unique<ns_odb::ScopedDB>(AcquireDatabase());
                if (database->Get() == nullptr)
                    return false;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetUserActionsForSolutions.ODBBegin");
                    transaction = std::make_unique<odb::transaction>((*database)->begin());
                }
                using Query = odb::query<oj::db::SolutionAction>;
                const Query query(
                    Query::user_id == static_cast<uint32_t>(user_id) &&
                    Query::solution_id.in_range(ids.begin(), ids.end()));
                odb::result<oj::db::SolutionAction> result;
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetUserActionsForSolutions.ODBQuery");
                    result = (*database)->query<oj::db::SolutionAction>(query, false);
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetUserActionsForSolutions.ODBLazyIteration");
                    for (const auto& action : result)
                        actions[action.solution_id][action.action_type] = true;
                }
                {
                    latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetUserActionsForSolutions.ODBCommit");
                    transaction->commit();
                }
            }
            catch (const std::exception& error)
            {
                if (transaction && !transaction->finalized())
                {
                    try
                    {
                        latecyMonitor::Timer timer(Monitor(), "ModelSolution.GetUserActionsForSolutions.ODBRollback");
                        transaction->rollback();
                    }
                    catch (...) {}
                }
                LOG_ERROR("{}{}", "ODB查询用户题解交互失败: ", error.what());
                return false;
            }
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count();
            RecordCacheMetrics(RecordActionType::Solution, false, true, cost_ms);
            return true;
        }

    };

}
