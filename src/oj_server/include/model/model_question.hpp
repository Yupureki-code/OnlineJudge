#pragma once

#include "model_base.hpp"

namespace ns_model
{
    class ModelQuestion : public ModelBase
    {
    private:
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

        //查询Mysql
        bool QueryMySql(const std::string& sql,std::vector<Question>& questions)
        {
            auto my = CreateConnection();
            if(!my)
                return false;
            if(mysql_query(my.get(), sql.c_str()) != 0)
            {
                logger(ns_log::FATAL)<<"MySql查询错误: "<<mysql_error(my.get());
                return false;
            }
            MYSQL_RES* res = mysql_store_result(my.get());
            if (res == nullptr)
            {
                logger(ns_log::FATAL) << "MySql结果集为空!";
                return false;
            }
            int rows = mysql_num_rows(res);
            //获取题目属性
            for(int i = 0;i<rows;i++)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                Question q;
                q.number = row[0];
                q.title = row[1];
                q.desc = row[2];
                q.star = row[3];
                q.create_time = row[4];
                q.update_time = row[5];
                q.cpu_limit = atoi(row[6]);
                q.memory_limit = atoi(row[7]);
                questions.push_back(q);
            }
            mysql_free_result(res);
            return true;
        }

        //构建题目列表查询的WHERE子句
        std::string BuildQuestionWhereClause(const std::shared_ptr<QueryStruct>& query_hash, MYSQL* my)
        {
            auto q = std::dynamic_pointer_cast<QuestionQuery>(query_hash);
            if (!q) return "";
            //条件列表
            std::vector<std::string> clauses;

            // both 模式映射后会产生 id/title 同值，且 id 为纯数字，此时保持 OR 语义。
            if (!q->id.empty() &&
                !q->title.empty() &&
                q->id == q->title &&
                IsAllDigits(q->id))
            {
                //id title内容相同，并且还都是纯数字，说明用户可能在同时搜索id和title，这时保持OR关系
                std::string safe_title = EscapeSqlString(q->title, my);
                clauses.push_back("(id=" + q->id + " or title like '%" + safe_title + "%')");
            }
            else
            {
                //id和title不同时存在，或者虽然同时存在但内容不同，或者虽然同时存在且内容相同但不是纯数字，这三种情况都保持AND关系
                if (!q->id.empty())
                {
                    if (IsAllDigits(q->id))
                    {
                        //纯数字才允许进入SQL
                        clauses.push_back("id=" + q->id);
                    }
                    else
                    {
                        //否则给一个永远不可能成立的条件，让SQL返回空结果
                        clauses.push_back("1=0");
                    }
                }

                if (!q->title.empty())
                {
                    // title使用模糊匹配
                    std::string safe_title = EscapeSqlString(q->title, my);
                    clauses.push_back("title like '%" + safe_title + "%'");
                }
            }

            //如果difficulty不为空,会把英文难度映射成中文：
            if (!q->star.empty())
            {
                std::string difficulty = q->star;
                if (difficulty == "simple") difficulty = "简单";
                else if (difficulty == "medium") difficulty = "中等";
                else if (difficulty == "hard") difficulty = "困难";

                if (difficulty != "all" && difficulty != "ALL" && difficulty != "全部")
                {
                    std::string safe_star = EscapeSqlString(difficulty, my);
                    clauses.push_back("star='" + safe_star + "'");
                }
            }

            if (clauses.empty())
            {
                return "";
            }

            std::ostringstream where;
            where << " where ";
            for (size_t i = 0; i < clauses.size(); ++i)
            {
                if (i != 0)
                {
                    where << " and ";
                }
                where << clauses[i];
            }

            return where.str();
        }

    public:
        //题库:得到一页内全部的题目
        bool GetQuestionsByPage(std::shared_ptr<Cache::CacheListKey> key,
                             std::vector<Question>& questions,
                             int* total_count,
                             int* total_pages)
        {
            auto begin = std::chrono::steady_clock::now();
            // 参数校验，这一步顺便记录一次列表查询指标，表示这次请求没有成功完成
            if (total_count == nullptr || total_pages == nullptr)
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            // 先查缓存:
            //  调用 _cache.GetAllQuestions(...) 尝试从 Redis 读取这一页的数据。
            //  缓存命中时，函数直接返回 true。
            //  这时 questions、total_count、total_pages 都会被缓存结果填好。
            //  同时记录一次"缓存命中"的指标。
            if(_cache.GetQuestionsByPage(key,  questions,total_count, total_pages))
            {
                logger(ns_log::INFO) << "Cache hit for question list page " << key->GetCacheKeyString(&_cache);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return true;
            }
            // 缓存没命中就查数据库
            // 先创建 MySQL 连接。
            auto my = CreateConnection();
            if (!my)
            {
                // 如果连接失败，直接返回 false，并记录这次是回源失败。
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            // 然后根据查询条件生成 where_clause，拼出统计总数的 SQL。
            std::string where_clause = BuildQuestionWhereClause(key->GetQueryHash(), my.get());
            //先执行 select count(*) ... 得到满足条件的题目总数，写入 total_count。
            std::string count_sql = "select count(*) from " + oj_questions + where_clause;
            if (!QueryCount(count_sql, total_count))
            {
                //如果统计失败，直接返回 false。
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            // 如果 total_count <= 0，说明没有任何结果：
            // total_pages 设为 0
            // questions.clear()
            // 仍然把这个空结果写入缓存
            // 返回 true
            if (*total_count <= 0)
            {
                *total_pages = 0;
                questions.clear();
                _cache.SetQuestionsByPage(key, questions,*total_count, *total_pages);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return true;
            }
            //计算分页并查当前页数据
            int size = key->GetSize();
            int page = key->GetPage();
            *total_pages = (*total_count + size - 1) / size;
            int safe_page = std::min(page, *total_pages);
            int offset = (safe_page - 1) * size;

            std::ostringstream page_sql;
            page_sql << "select * from " << oj_questions
                     << where_clause
                     << " order by cast(id as unsigned) asc limit " << size << " offset " << offset;

            if (!QueryMySql(page_sql.str(), questions))
            {
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            // 回填缓存
            // 数据查出来后，会调用 _cache.SetQuestionsByPage(...) 写回 Redis。
            // 缓存键里包含：
            // 查询条件
            // 页码
            // 每页大小
            // 列表版本号
            // 这样下次同样条件的请求就可以直接命中缓存。
            auto write_key = _cache.BuildListCacheKey(key->GetQueryHash(), safe_page, size, key->GetListVersion(), Cache::CacheKey::PageType::kData, key->GetListType());
            _cache.SetQuestionsByPage(write_key, questions, *total_count, *total_pages);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            return true;
            // 记录指标并返回
            // 无论是缓存命中还是数据库回源成功，都会调用 RecordListMetrics(...)。
            // 这里的统计不是为了业务逻辑本身，而是为了监控缓存效果和接口耗时。
        }
        //题目:获得单个题目
        bool GetOneQuestion(const std::string& number,Question& q)
        {
            auto begin = std::chrono::steady_clock::now();
            if (!IsAllDigits(number))
            {
                // number参数不合法，直接返回 false，并记录这次请求没有成功完成。
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            auto detail_key = _cache.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kData);
            // 先查缓存，缓存命中直接返回
            if(_cache.GetDetailedQuestion(detail_key, q))
            {
                logger(ns_log::INFO) << "Cache hit for question " << number;
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return true;
            }
            // 缓存没命中就查数据库，先创建 MySQL 连接
            std::string sql = "select * from ";
            sql += oj_questions;
            sql += " where id=" + number;
            std::vector<Question> v;
            if (!QueryMySql(sql, v))
            {
                //查询数据库失败，直接返回 false，并记录这次是回源失败。
                _cache.SetQuestionNotFound(detail_key, number);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            // 数据库查询成功但没有找到这个题目，说明这个题目确实不存在。
            // 为了避免缓存穿透攻击，应该把这个"空结果"也写入缓存（比如写一个特殊的值表示这个题目不存在
            // 这样下次同样的请求就可以直接命中缓存，避免每次都打到数据库上来。
            if(v.size() != 1)
            {
                _cache.SetQuestionNotFound(detail_key, number);
                auto end = std::chrono::steady_clock::now();
                long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                return false;
            }
            q = v[0];
            _cache.SetDetailedQuestion(detail_key, q);
            auto end = std::chrono::steady_clock::now();
            long long cost_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            return true;
        }

        //题目写接口:新增或修改题目
        bool SaveQuestion(const Question& input)
        {
            if (!IsAllDigits(input.number))
            {
                return false;
            }

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_id = input.number;
            std::string safe_title = EscapeSqlString(input.title, my.get());
            std::string safe_desc = EscapeSqlString(input.desc, my.get());
            std::string safe_star = EscapeSqlString(input.star, my.get());

            std::ostringstream sql;
            sql << "insert into " << oj_questions
                << " (id, title, `desc`, star, cpu_limit, memory_limit, create_time, update_time) values ("
                << safe_id << ", '" << safe_title << "', '" << safe_desc << "', '" << safe_star << "', "
                << input.cpu_limit << ", " << input.memory_limit << ", NOW(), NOW()) "
                << "on duplicate key update "
                << "title=values(title), `desc`=values(`desc`), star=values(star), "
                << "cpu_limit=values(cpu_limit), memory_limit=values(memory_limit), update_time=NOW()";

            if (!ExecuteSql(sql.str()))
            {
                return false;
            }

            Question cached = input;
            auto detail_key = _cache.BuildDetailCacheKey(input.number, Cache::CacheKey::PageType::kData);
            _cache.SetDetailedQuestion(detail_key, cached);
            auto detail_html_key = _cache.BuildDetailCacheKey(input.number, Cache::CacheKey::PageType::kHtml);
            _cache.InvalidatePage(detail_html_key);
            TouchQuestionListVersion();
            return true;
        }

        bool DeleteQuestion(const std::string& number)
        {
            if (!IsAllDigits(number))
            {
                return false;
            }

            std::string sql = "delete from " + oj_questions + " where id=" + number;
            if (!ExecuteSql(sql))
            {
                return false;
            }

            auto detail_key = _cache.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kData);
            _cache.SetQuestionNotFound(detail_key, number);
            auto detail_html_key = _cache.BuildDetailCacheKey(number, Cache::CacheKey::PageType::kHtml);
            _cache.InvalidatePage(detail_html_key);
            TouchQuestionListVersion();
            return true;
        }

        bool GetQuestionCount(int* count)
        {
            if(count == nullptr)
            {
                return false;
            }
            std::string key = "question_counts";
            std::string value;
            if(_cache.GetStringByAnyKey(key, &value))
            {
                *count = std::atoi(value.c_str());
                logger(ns_log::INFO) << "Cache hit for question count";
                return true;
            }
            std::string sql = "select count(*) from " + oj_questions;
            if(!QueryCount(sql, count))
            {
                return false;
             }
             _cache.SetStringByAnyKey(key, std::to_string(*count), _cache.BuildJitteredTtl(6 * 60 * 60, 30 * 60));
             return true;
        }

        // 题目写路径完成后调用该接口，统一触发列表版本递增。
        std::string TouchQuestionListVersion()
        {   
            std::string version = _cache.BumpListVersion(ListType::Questions);
            logger(ns_log::INFO) << "Question list version bumped to " << version;
            return version;
        }
        std::string GetQuestionsListVersion()
        {
            return _cache.GetListVersion(ListType::Questions);
        }
        std::shared_ptr<Cache::CacheListKey> BuildListCacheKey(std::shared_ptr<QueryStruct>& query_hash, int page, int size, const std::string& list_version, Cache::CacheKey::PageType page_type)
        {
            auto normalized_query = NormalizeQuestionQuery(query_hash);
            return _cache.BuildListCacheKey(normalized_query, page, size, list_version, page_type, ListType::Questions);
        }
        std::shared_ptr<Cache::CacheDetailKey> BuildDetailCacheKey(const std::string& number, Cache::CacheKey::PageType page_type)
        {
            return _cache.BuildDetailCacheKey(number, page_type);
        }
        std::shared_ptr<Cache::CacheStaticKey> BuildHtmlCacheKey(const std::string& page_name, Cache::CacheKey::PageType page_type)
        {
            return _cache.BuildStaticCacheKey(page_name, page_type);
        }

        // Feature 2.5a: Get sample tests for a question
        bool GetSampleTestsByQuestionId(const std::string& question_id, Json::Value* tests)
        {
            if (tests == nullptr)
                return false;

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string sql = "select id, question_id, `in`, `out`, is_sample from " + oj_tests +
                              " where question_id='" + safe_qid + "' and is_sample=1 order by id asc";

            MYSQL_RES* res = ModelBase::QueryMySql(my.get(), sql, "MySql查询测试用例错误");
            if (!res) return false;
            tests->clear();
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["id"] = row[0] ? std::atoi(row[0]) : 0;
                item["question_id"] = row[1] ? row[1] : "";
                item["input"] = row[2] ? row[2] : "";
                item["output"] = row[3] ? row[3] : "";
                item["is_sample"] = (row[4] && std::atoi(row[4]) != 0) ? true : false;
                tests->append(item);
            }
            mysql_free_result(res);
            return true;
        }

        // Admin: Get ALL tests (not just samples) for a question
        bool GetTestsByQuestionId(const std::string& question_id, Json::Value* result)
        {
            if (result == nullptr)
                return false;

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::string sql = "select id, question_id, `in`, `out`, is_sample from " + oj_tests +
                              " where question_id='" + safe_qid + "' order by id asc";

            MYSQL_RES* res = ModelBase::QueryMySql(my.get(), sql, "MySql查询测试用例错误");
            if (!res) return false;

            Json::Value test_array(Json::arrayValue);
            for (int i = 0; i < mysql_num_rows(res); ++i)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row == nullptr) continue;
                Json::Value item;
                item["id"] = row[0] ? std::atoi(row[0]) : 0;
                item["question_id"] = row[1] ? row[1] : "";
                item["in"] = row[2] ? row[2] : "";
                item["out"] = row[3] ? row[3] : "";
                item["is_sample"] = (row[4] && std::atoi(row[4]) != 0) ? true : false;
                test_array.append(item);
            }
            mysql_free_result(res);
            (*result)["tests"] = test_array;
            return true;
        }

        // Feature 2.5: Get a single test case by ID (for custom test execution)
        bool GetTestById(int test_id, const std::string& question_id, std::string* test_input, std::string* test_output)
        {
            if (test_input == nullptr || test_output == nullptr)
                return false;

            auto my = CreateConnection();
            if (!my)
                return false;

            std::string safe_qid = EscapeSqlString(question_id, my.get());
            std::ostringstream sql;
            sql << "select `in`, `out` from " << oj_tests
                << " where id=" << test_id
                << " and question_id='" << safe_qid << "'";

            MYSQL_RES* res = ModelBase::QueryMySql(my.get(), sql.str(), "MySql查询测试用例错误");
            if (!res) return false;
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row == nullptr || row[0] == nullptr || row[1] == nullptr)
            {
                mysql_free_result(res);
                return false;
            }

            *test_input = row[0];
            *test_output = row[1];
            mysql_free_result(res);
            return true;
        }
    };

}
