#pragma once

#include "model_base.hpp"
#include "../../../comm/models/odb_models.hpp"
#include "../../../comm/comm.hpp"
#include <odb/query.hxx>
#include <limits>

namespace oj::model
{

    class ModelComment : public ModelBase
    {
    private:
        class TimedTransaction
        {
        public:
            TimedTransaction(odb::database& database, latecyMonitor::LatencyMonitor& monitor,
                             const char* begin_metric, const char* rollback_metric)
                : monitor_(monitor), rollback_metric_(rollback_metric)
            {
                latecyMonitor::Timer timer(monitor_, begin_metric);
                transaction_ = std::make_unique<odb::transaction>(database.begin());
            }

            ~TimedTransaction()
            {
                if (transaction_->finalized()) return;
                try
                {
                    latecyMonitor::Timer timer(monitor_, rollback_metric_);
                    transaction_->rollback();
                }
                catch (const std::exception& error)
                {
                    LOG_ERROR("Timed ODB rollback failed: {}", error.what());
                }
                catch (...)
                {
                    LOG_ERROR("{}", "Timed ODB rollback failed");
                }
            }

            void commit() { transaction_->commit(); }

        private:
            std::unique_ptr<odb::transaction> transaction_;
            latecyMonitor::LatencyMonitor& monitor_;
            const char* rollback_metric_;
        };

        static bool FitsOdbId(uint64_t id)
        {
            return id <= std::numeric_limits<uint32_t>::max();
        }

        static bool FitsOdbId(odb::nullable<uint64_t> id)
        {
            return id.null() || id.get() <= std::numeric_limits<uint32_t>::max();
        }

        static Json::Value CommentJson(const Comment& comment)
        {
            Json::Value item;
            item["id"] = static_cast<Json::UInt64>(comment.id);
            item["solution_id"] = static_cast<Json::UInt64>(comment.solution_id);
            item["user_id"] = comment.user_id;
            item["content"] = comment.content;
            item["is_edited"] = comment.is_edited.null()
                ? Json::Value() : Json::Value(comment.is_edited.get());
            item["parent_id"] = comment.parent_id.null()
                ? Json::Value() : Json::Value(static_cast<Json::UInt64>(comment.parent_id.get()));
            item["reply_to_user_id"] = comment.reply_to_user_id.null()
                ? Json::Value() : Json::Value(comment.reply_to_user_id.get());
            item["like_count"] = comment.like_count;
            item["favorite_count"] = comment.favorite_count;
            item["created_at"] = comment.created_at.null() ? Json::Value() : Json::Value(
                oj::util::TimeUtil::DateTimeToInt(comment.created_at.get()));
            item["updated_at"] = comment.updated_at.null() ? Json::Value() : Json::Value(
                oj::util::TimeUtil::DateTimeToInt(comment.updated_at.get()));
            return item;
        }

        static bool CommentFromJson(const Json::Value& item, Comment* comment)
        {
            if (comment == nullptr || !item.isObject() ||
                !item["id"].isUInt64() || !item["solution_id"].isUInt64() ||
                !item["user_id"].isUInt() || !item["content"].isString() ||
                !item["like_count"].isUInt() || !item["favorite_count"].isUInt() ||
                (!item["is_edited"].isNull() && !item["is_edited"].isBool()) ||
                (!item["parent_id"].isNull() && !item["parent_id"].isUInt64()) ||
                (!item["reply_to_user_id"].isNull() && !item["reply_to_user_id"].isUInt()) ||
                (!item["created_at"].isNull() && !item["created_at"].isInt64()) ||
                (!item["updated_at"].isNull() && !item["updated_at"].isInt64()))
                return false;
            comment->id = item["id"].asUInt64();
            comment->solution_id = item["solution_id"].asUInt64();
            comment->user_id = item["user_id"].asInt();
            comment->content = item["content"].asString();
            comment->is_edited = item["is_edited"].isNull()
                ? odb::nullable<bool>() : odb::nullable<bool>(item["is_edited"].asBool());
            comment->parent_id = item["parent_id"].isNull()
                ? odb::nullable<uint64_t>() : odb::nullable<uint64_t>(item["parent_id"].asUInt64());
            comment->reply_to_user_id = item["reply_to_user_id"].isNull()
                ? odb::nullable<uint32_t>() : odb::nullable<uint32_t>(item["reply_to_user_id"].asUInt());
            comment->like_count = item["like_count"].asUInt();
            comment->favorite_count = item["favorite_count"].asUInt();
            comment->created_at = item["created_at"].isNull()
                ? odb::nullable<oj::db::DateTime>()
                : odb::nullable<oj::db::DateTime>(oj::util::TimeUtil::IntToDateTime(item["created_at"].asInt64()));
            comment->updated_at = item["updated_at"].isNull()
                ? odb::nullable<oj::db::DateTime>()
                : odb::nullable<oj::db::DateTime>(oj::util::TimeUtil::IntToDateTime(item["updated_at"].asInt64()));
            return true;
        }

        static std::string CommentDetailCacheKey(unsigned long long comment_id)
        {
            return "comment:detail:v2:" + std::to_string(comment_id);
        }

        static int CountAsInt(uint64_t count)
        {
            return static_cast<int>(std::min<uint64_t>(count, std::numeric_limits<int>::max()));
        }

        static constexpr size_t MaxBatchIds = 200;

    public:
        static constexpr int MaxPageSize = 100;

        bool GetCommentsBySolutionId(unsigned long long solution_id, int page, int size,
                                     std::vector<Comment>* comments, int* total_count)
        {
            if (comments == nullptr || total_count == nullptr || !FitsOdbId(solution_id))
                return false;

            auto metrics_begin = std::chrono::steady_clock::now();
            comments->clear();
            *total_count = 0;

            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                const int safe_page = std::max(1, page);
                const int safe_size = std::max(1, std::min(size, MaxPageSize));
                const uint64_t offset = static_cast<uint64_t>(safe_page - 1) *
                    static_cast<uint64_t>(safe_size);
                std::vector<oj::db::Comment> rows;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.GetCommentsBySolutionId.ODBBegin",
                        "ModelComment.GetCommentsBySolutionId.ODBRollback");
                    using Query = odb::query<oj::db::Comment>;
                    using CountQuery = odb::query<oj::db::CommentCount>;
                    const Query filter = Query::solution_id == static_cast<uint32_t>(solution_id) &&
                        (Query::parent_id.is_null() || Query::parent_id == static_cast<uint64_t>(0));
                    const CountQuery count_filter =
                        CountQuery::solution_id == static_cast<uint32_t>(solution_id) &&
                        (CountQuery::parent_id.is_null() ||
                         CountQuery::parent_id == static_cast<uint64_t>(0));
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.GetCommentsBySolutionId.ODBQueryCount");
                        *total_count = CountAsInt(
                            database->query_value<oj::db::CommentCount>(count_filter).value);
                    }
                    Query page_filter(filter);
                    page_filter += " ORDER BY `created_at` ASC, `id` ASC LIMIT " +
                        std::to_string(safe_size) + " OFFSET " + std::to_string(offset);
                    odb::result<oj::db::Comment> result;
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.GetCommentsBySolutionId.ODBQueryComments");
                        result = database->query<oj::db::Comment>(page_filter);
                    }
                    {
                        latecyMonitor::Timer iterate_timer(Monitor(), "ModelComment.GetCommentsBySolutionId.ODBIterateComments");
                        for (const auto& row : result) rows.push_back(row);
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.GetCommentsBySolutionId.ODBCommit");
                        transaction.commit();
                    }
                }

                for (const auto& row : rows)
                {
                    Comment comment = row;
                    comments->push_back(std::move(comment));
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentsBySolutionId failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentsBySolutionId failed: {}", error.what());
                return false;
            }

            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - metrics_begin).count());
            return true;
        }

        bool CreateComment(const Comment& comment, unsigned long long* comment_id = nullptr)
        {
            if (comment.solution_id == 0 || comment.user_id <= 0 ||
                !FitsOdbId(comment.solution_id) || !FitsOdbId(comment.parent_id) ||
                !FitsOdbId(static_cast<unsigned long long>(comment.user_id)) ||
                (!comment.reply_to_user_id.null() &&
                 !FitsOdbId(static_cast<unsigned long long>(comment.reply_to_user_id.get()))))
                return false;

            auto begin = std::chrono::steady_clock::now();
            uint32_t new_id = 0;
            std::string question_id;
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.CreateComment.ODBBegin",
                        "ModelComment.CreateComment.ODBRollback");
                    const uint64_t parent_id = comment.parent_id.null() ? 0 : comment.parent_id.get();
                    if (parent_id > 0)
                    {
                        using Query = odb::query<oj::db::Comment>;
                        Query query(Query::id == static_cast<uint32_t>(parent_id));
                        query += " FOR UPDATE";
                        std::unique_ptr<oj::db::Comment> parent;
                        {
                            latecyMonitor::Timer load_timer(Monitor(), "ModelComment.CreateComment.ODBLoadParentForUpdate");
                            parent.reset(database->query_one<oj::db::Comment>(query));
                        }
                        if (!parent || parent->solution_id != comment.solution_id) return false;
                    }

                    using SolutionQuery = odb::query<oj::db::Solution>;
                    SolutionQuery solution_query(SolutionQuery::id == static_cast<uint32_t>(comment.solution_id));
                    solution_query += " FOR UPDATE";
                    std::unique_ptr<oj::db::Solution> solution;
                    {
                        latecyMonitor::Timer load_timer(Monitor(), "ModelComment.CreateComment.ODBLoadSolutionForUpdate");
                        solution.reset(database->query_one<oj::db::Solution>(solution_query));
                    }
                    if (!solution) return false;
                    question_id = solution->question_id;
                    oj::db::Comment row{};
                    row.parent_id = parent_id == 0
                        ? odb::nullable<uint64_t>() : odb::nullable<uint64_t>(parent_id);
                    const uint32_t reply_to_user_id = comment.reply_to_user_id.null()
                        ? 0 : comment.reply_to_user_id.get();
                    row.reply_to_user_id = reply_to_user_id == 0
                        ? odb::nullable<uint32_t>()
                        : odb::nullable<uint32_t>(reply_to_user_id);
                    row.like_count = 0;
                    row.favorite_count = 0;
                    row.solution_id = static_cast<uint32_t>(comment.solution_id);
                    row.user_id = static_cast<uint32_t>(comment.user_id);
                    row.content = comment.content;
                    row.is_edited = false;
                    const auto now = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                    row.created_at = now;
                    row.updated_at = now;
                    {
                        latecyMonitor::Timer persist_timer(Monitor(), "ModelComment.CreateComment.ODBPersistComment");
                        new_id = database->persist(row);
                    }
                    if (parent_id == 0)
                    {
                        if (solution->comment_count == std::numeric_limits<uint32_t>::max())
                            return false;
                        ++solution->comment_count;
                        {
                            latecyMonitor::Timer update_timer(Monitor(), "ModelComment.CreateComment.ODBUpdateSolution");
                            database->update(*solution);
                        }
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.CreateComment.ODBCommit");
                        transaction.commit();
                    }
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.CreateComment failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.CreateComment failed: {}", error.what());
                return false;
            }

            if (comment_id != nullptr) *comment_id = new_id;
            if (comment.parent_id.null() || comment.parent_id.get() == 0)
            {
                auto key = _cache.BuildSolutionDetailCacheKey(comment.solution_id);
                _cache.DeleteStringByAnyKey(key->GetCacheKeyString(&_cache));
                InvalidateSolutionListCaches(question_id);
            }
            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count());
            return true;
        }

        bool GetCommentById(unsigned long long comment_id, Comment* comment)
        {
            if (comment == nullptr || comment_id == 0 || !FitsOdbId(comment_id)) return false;
            auto metrics_begin = std::chrono::steady_clock::now();
            const std::string cache_key = CommentDetailCacheKey(comment_id);
            std::string cached;
            if (_cache.GetStringByAnyKey(cache_key, &cached))
            {
                Json::CharReaderBuilder builder;
                Json::Value value;
                std::istringstream stream(cached);
                if (Json::parseFromStream(builder, stream, &value, nullptr) &&
                    CommentFromJson(value, comment) && comment->id == comment_id)
                {
                    RecordCacheMetrics(RecordActionType::Comment, true, false,
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - metrics_begin).count());
                    return true;
                }
            }

            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.GetCommentById.ODBBegin",
                        "ModelComment.GetCommentById.ODBRollback");
                    std::unique_ptr<oj::db::Comment> row;
                    {
                        latecyMonitor::Timer load_timer(Monitor(), "ModelComment.GetCommentById.ODBLoadComment");
                        row.reset(database->find<oj::db::Comment>(static_cast<uint32_t>(comment_id)));
                    }
                    if (!row) return false;
                    *comment = *row;
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.GetCommentById.ODBCommit");
                        transaction.commit();
                    }
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentById failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentById failed: {}", error.what());
                return false;
            }
            Json::FastWriter writer;
            _cache.SetStringByAnyKey(cache_key, writer.write(CommentJson(*comment)),
                                     _cache.BuildJitteredTtl(300, 60));
            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - metrics_begin).count());
            return true;
        }

        bool GetCommentReplies(unsigned long long parent_id, int page, int size,
                               std::vector<Comment>* replies, int* total_count)
        {
            if (replies == nullptr || total_count == nullptr || parent_id == 0 || !FitsOdbId(parent_id)) return false;
            auto metrics_begin = std::chrono::steady_clock::now();
            const int safe_page = std::max(1, page);
            const int safe_size = std::max(1, std::min(size, MaxPageSize));
            const uint64_t offset = static_cast<uint64_t>(safe_page - 1) *
                static_cast<uint64_t>(safe_size);
            replies->clear();
            *total_count = 0;

            std::vector<oj::db::Comment> rows;
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.GetCommentReplies.ODBBegin",
                        "ModelComment.GetCommentReplies.ODBRollback");
                    using Query = odb::query<oj::db::Comment>;
                    using CountQuery = odb::query<oj::db::CommentCount>;
                    const Query filter(Query::parent_id == static_cast<uint64_t>(parent_id));
                    const CountQuery count_filter(
                        CountQuery::parent_id == static_cast<uint64_t>(parent_id));
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.GetCommentReplies.ODBQueryCount");
                        *total_count = CountAsInt(
                            database->query_value<oj::db::CommentCount>(count_filter).value);
                    }
                    Query page_filter(filter);
                    page_filter += " ORDER BY `created_at` ASC, `id` ASC LIMIT " +
                        std::to_string(safe_size) + " OFFSET " + std::to_string(offset);
                    odb::result<oj::db::Comment> result;
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.GetCommentReplies.ODBQueryComments");
                        result = database->query<oj::db::Comment>(page_filter);
                    }
                    {
                        latecyMonitor::Timer iterate_timer(Monitor(), "ModelComment.GetCommentReplies.ODBIterateComments");
                        for (const auto& row : result) rows.push_back(row);
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.GetCommentReplies.ODBCommit");
                        transaction.commit();
                    }
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentReplies failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentReplies failed: {}", error.what());
                return false;
            }
            for (const auto& row : rows)
            {
                Comment reply = row;
                replies->push_back(std::move(reply));
            }
            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - metrics_begin).count());
            return true;
        }

        bool GetDirectChildReplyCounts(const std::vector<unsigned long long>& comment_ids,
                                       std::map<unsigned long long, int>* reply_counts)
        {
            if (reply_counts == nullptr) return false;
            reply_counts->clear();
            if (comment_ids.empty()) return true;
            if (comment_ids.size() > MaxBatchIds) return false;
            std::vector<uint64_t> ids;
            ids.reserve(comment_ids.size());
            for (unsigned long long id : comment_ids)
            {
                if (id == 0 || !FitsOdbId(id)) return false;
                ids.push_back(id);
            }
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.GetDirectChildReplyCounts.ODBBegin",
                        "ModelComment.GetDirectChildReplyCounts.ODBRollback");
                    using Query = odb::query<oj::db::Comment>;
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.GetDirectChildReplyCounts.ODBQueryCounts");
                        odb::result<oj::db::Comment> children = database->query<oj::db::Comment>(
                            Query::parent_id.in_range(ids.begin(), ids.end()), false);
                        for (const auto& child : children)
                        {
                            if (child.parent_id.null()) continue;
                            int& count = (*reply_counts)[child.parent_id.get()];
                            if (count < std::numeric_limits<int>::max()) ++count;
                        }
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.GetDirectChildReplyCounts.ODBCommit");
                        transaction.commit();
                    }
                }
                return true;
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.GetDirectChildReplyCounts failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.GetDirectChildReplyCounts failed: {}", error.what());
                return false;
            }
        }

        bool UpdateComment(unsigned long long comment_id, int user_id, const std::string& content)
        {
            if (comment_id == 0 || !FitsOdbId(comment_id) || user_id <= 0) return false;
            auto begin = std::chrono::steady_clock::now();
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.UpdateComment.ODBBegin",
                        "ModelComment.UpdateComment.ODBRollback");
                    using Query = odb::query<oj::db::Comment>;
                    Query query(Query::id == static_cast<uint32_t>(comment_id));
                    query += " FOR UPDATE";
                    std::unique_ptr<oj::db::Comment> row;
                    {
                        latecyMonitor::Timer load_timer(Monitor(), "ModelComment.UpdateComment.ODBLoadCommentForUpdate");
                        row.reset(database->query_one<oj::db::Comment>(query));
                    }
                    if (!row || row->user_id != static_cast<uint32_t>(user_id)) return false;
                    row->content = content;
                    row->is_edited = true;
                    row->updated_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                    {
                        latecyMonitor::Timer update_timer(Monitor(), "ModelComment.UpdateComment.ODBUpdateComment");
                        database->update(*row);
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.UpdateComment.ODBCommit");
                        transaction.commit();
                    }
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.UpdateComment failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.UpdateComment failed: {}", error.what());
                return false;
            }
            _cache.DeleteStringByAnyKey(CommentDetailCacheKey(comment_id));
            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count());
            return true;
        }

        bool DeleteComment(unsigned long long comment_id, int user_id, bool is_admin = false)
        {
            if (comment_id == 0 || !FitsOdbId(comment_id) || (!is_admin && user_id <= 0)) return false;
            auto begin = std::chrono::steady_clock::now();
            unsigned long long solution_id = 0;
            unsigned long long parent_id = 0;
            std::string question_id;
            std::vector<uint32_t> child_ids;
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.DeleteComment.ODBBegin",
                        "ModelComment.DeleteComment.ODBRollback");
                    using Query = odb::query<oj::db::Comment>;
                    Query query(Query::id == static_cast<uint32_t>(comment_id));
                    query += " FOR UPDATE";
                    std::unique_ptr<oj::db::Comment> row;
                    {
                        latecyMonitor::Timer load_timer(Monitor(), "ModelComment.DeleteComment.ODBLoadCommentForUpdate");
                        row.reset(database->query_one<oj::db::Comment>(query));
                    }
                    if (!row || (!is_admin && row->user_id != static_cast<uint32_t>(user_id))) return false;
                    solution_id = row->solution_id;
                    parent_id = row->parent_id.null() ? 0 : row->parent_id.get();

                    using SolutionQuery = odb::query<oj::db::Solution>;
                    SolutionQuery solution_query(SolutionQuery::id == static_cast<uint32_t>(solution_id));
                    solution_query += " FOR UPDATE";
                    std::unique_ptr<oj::db::Solution> solution;
                    {
                        latecyMonitor::Timer load_timer(Monitor(), "ModelComment.DeleteComment.ODBLoadSolutionForUpdate");
                        solution.reset(database->query_one<oj::db::Solution>(solution_query));
                    }
                    if (!solution) return false;
                    question_id = solution->question_id;

                    {
                        using ActionQuery = odb::query<oj::db::CommentAction>;
                        using ChildQuery = odb::query<oj::db::Comment>;
                        ChildQuery child_query(
                            ChildQuery::parent_id == static_cast<uint64_t>(comment_id));
                        child_query += " FOR UPDATE";
                        odb::result<oj::db::Comment> children;
                        {
                            latecyMonitor::Timer query_timer(Monitor(), "ModelComment.DeleteComment.ODBQueryChildren");
                            children = database->query<oj::db::Comment>(child_query);
                        }
                        {
                            latecyMonitor::Timer iterate_timer(Monitor(), "ModelComment.DeleteComment.ODBIterateChildren");
                            for (const auto& child : children) child_ids.push_back(child.id);
                        }
                        for (uint32_t child_id : child_ids)
                        {
                            latecyMonitor::Timer child_action_timer(Monitor(), "ModelComment.DeleteComment.ODBEraseChildAction");
                            database->erase_query<oj::db::CommentAction>(ActionQuery::comment_id == child_id);
                        }
                    }
                    {
                        latecyMonitor::Timer erase_timer(Monitor(), "ModelComment.DeleteComment.ODBEraseChildren");
                        database->erase_query<oj::db::Comment>(Query::parent_id == static_cast<uint64_t>(comment_id));
                    }
                    {
                        latecyMonitor::Timer erase_timer(Monitor(), "ModelComment.DeleteComment.ODBEraseActions");
                        using ActionQuery = odb::query<oj::db::CommentAction>;
                        database->erase_query<oj::db::CommentAction>(
                            ActionQuery::comment_id == static_cast<uint32_t>(comment_id));
                    }
                    {
                        latecyMonitor::Timer erase_timer(Monitor(), "ModelComment.DeleteComment.ODBEraseComment");
                        database->erase<oj::db::Comment>(static_cast<uint32_t>(comment_id));
                    }
                    if (parent_id == 0)
                    {
                        if (solution->comment_count > 0) --solution->comment_count;
                        {
                            latecyMonitor::Timer update_timer(Monitor(), "ModelComment.DeleteComment.ODBUpdateSolution");
                            database->update(*solution);
                        }
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.DeleteComment.ODBCommit");
                        transaction.commit();
                    }
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.DeleteComment failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.DeleteComment failed: {}", error.what());
                return false;
            }

            _cache.DeleteStringByAnyKey(CommentDetailCacheKey(comment_id));
            for (uint32_t child_id : child_ids)
                _cache.DeleteStringByAnyKey(CommentDetailCacheKey(child_id));
            if (parent_id == 0)
            {
                auto key = _cache.BuildSolutionDetailCacheKey(solution_id);
                _cache.DeleteStringByAnyKey(key->GetCacheKeyString(&_cache));
                InvalidateSolutionListCaches(question_id);
            }
            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count());
            return true;
        }

        bool GetCommentActions(const std::vector<unsigned long long>& comment_ids,
                               int user_id,
                               std::map<unsigned long long, std::map<std::string, bool>>* actions)
        {
            if (actions == nullptr || user_id <= 0) return false;
            actions->clear();
            if (comment_ids.empty()) return true;
            if (comment_ids.size() > MaxBatchIds) return false;
            using Query = odb::query<oj::db::CommentAction>;
            Query ids(false);
            for (unsigned long long id : comment_ids)
            {
                if (id == 0 || !FitsOdbId(id)) return false;
                ids = ids || Query::comment_id == static_cast<uint32_t>(id);
            }
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.GetCommentActions.ODBBegin",
                        "ModelComment.GetCommentActions.ODBRollback");
                    odb::result<oj::db::CommentAction> result;
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.GetCommentActions.ODBQueryActions");
                        result = database->query<oj::db::CommentAction>(
                            Query::user_id == static_cast<uint32_t>(user_id) && ids);
                    }
                    {
                        latecyMonitor::Timer iterate_timer(Monitor(), "ModelComment.GetCommentActions.ODBIterateActions");
                        for (const auto& action : result) (*actions)[action.comment_id][action.action_type] = true;
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.GetCommentActions.ODBCommit");
                        transaction.commit();
                    }
                }
                return true;
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentActions failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.GetCommentActions failed: {}", error.what());
                return false;
            }
        }

        bool ToggleCommentAction(unsigned long long comment_id, int user_id,
                                 const std::string& action_type,
                                 bool* now_active, unsigned int* new_count)
        {
            if (now_active == nullptr || new_count == nullptr || comment_id == 0 || !FitsOdbId(comment_id) ||
                user_id <= 0 || (action_type != "like" && action_type != "favorite"))
                return false;

            auto begin = std::chrono::steady_clock::now();
            try
            {
                auto database = AcquireDatabase();
                if (!database.Get()) return false;
                {
                    TimedTransaction transaction(*database, Monitor(),
                        "ModelComment.ToggleCommentAction.ODBBegin",
                        "ModelComment.ToggleCommentAction.ODBRollback");
                    using CommentQuery = odb::query<oj::db::Comment>;
                    CommentQuery comment_query(CommentQuery::id == static_cast<uint32_t>(comment_id));
                    comment_query += " FOR UPDATE";
                    std::unique_ptr<oj::db::Comment> comment;
                    {
                        latecyMonitor::Timer load_timer(Monitor(), "ModelComment.ToggleCommentAction.ODBLoadCommentForUpdate");
                        comment.reset(database->query_one<oj::db::Comment>(comment_query));
                    }
                    if (!comment) return false;
                    using ActionQuery = odb::query<oj::db::CommentAction>;
                    const ActionQuery action_query =
                        ActionQuery::comment_id == static_cast<uint32_t>(comment_id) &&
                        ActionQuery::user_id == static_cast<uint32_t>(user_id) &&
                        ActionQuery::action_type == action_type;
                    std::unique_ptr<oj::db::CommentAction> action;
                    {
                        latecyMonitor::Timer query_timer(Monitor(), "ModelComment.ToggleCommentAction.ODBQueryAction");
                        action.reset(database->query_one<oj::db::CommentAction>(action_query));
                    }
                    if (action)
                    {
                        {
                            latecyMonitor::Timer erase_timer(Monitor(), "ModelComment.ToggleCommentAction.ODBEraseAction");
                            database->erase(*action);
                        }
                        if (action_type == "like")
                        {
                            if (comment->like_count > 0) --comment->like_count;
                            *new_count = comment->like_count;
                        }
                        else
                        {
                            if (comment->favorite_count > 0) --comment->favorite_count;
                            *new_count = comment->favorite_count;
                        }
                        *now_active = false;
                    }
                    else
                    {
                        const uint32_t count = action_type == "like"
                            ? comment->like_count : comment->favorite_count;
                        if (count == std::numeric_limits<uint32_t>::max()) return false;
                        oj::db::CommentAction created{};
                        created.comment_id = static_cast<uint32_t>(comment_id);
                        created.user_id = static_cast<uint32_t>(user_id);
                        created.action_type = action_type;
                        created.created_at = oj::util::TimeUtil::IntToDateTime(oj::util::TimeUtil::GetTimeStamp());
                        {
                            latecyMonitor::Timer persist_timer(Monitor(), "ModelComment.ToggleCommentAction.ODBPersistAction");
                            database->persist(created);
                        }
                        if (action_type == "like") *new_count = ++comment->like_count;
                        else *new_count = ++comment->favorite_count;
                        *now_active = true;
                    }
                    {
                        latecyMonitor::Timer update_timer(Monitor(), "ModelComment.ToggleCommentAction.ODBUpdateComment");
                        database->update(*comment);
                    }
                    {
                        latecyMonitor::Timer commit_timer(Monitor(), "ModelComment.ToggleCommentAction.ODBCommit");
                        transaction.commit();
                    }
                }
            }
            catch (const odb::exception& error)
            {
                LOG_ERROR("ModelComment.ToggleCommentAction failed: {}", error.what());
                return false;
            }
            catch (const std::exception& error)
            {
                LOG_ERROR("ModelComment.ToggleCommentAction failed: {}", error.what());
                return false;
            }

            _cache.DeleteStringByAnyKey("action:user:" + std::to_string(user_id)
                + ":comment:" + std::to_string(comment_id));
            _cache.DeleteStringByAnyKey(CommentDetailCacheKey(comment_id));
            RecordCacheMetrics(RecordActionType::Comment, false, true,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - begin).count());
            return true;
        }
    };

}
