#include "odb_models.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include <mysql/mysql.h>
#include <odb/mysql/traits.hxx>
#include <odb/nullable.hxx>

namespace {

template <typename T>
using Traits = odb::access::object_traits_impl<T, odb::id_mysql>;

template <typename T>
void CheckTable(const char* expected, std::size_t expected_columns) {
    if (std::string(Traits<T>::table_name) != expected) {
        std::cerr << "table mismatch: expected " << expected << ", got "
                  << Traits<T>::table_name << '\n';
        std::exit(1);
    }
    if (Traits<T>::id_column_count != 1) {
        std::cerr << expected << " does not have exactly one ODB id column\n";
        std::exit(1);
    }
    if (Traits<T>::column_count != expected_columns) {
        std::cerr << expected << " column count mismatch: expected " << expected_columns
                  << ", got " << Traits<T>::column_count << '\n';
        std::exit(1);
    }
}

const char* Env(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && *value != '\0' ? value : nullptr;
}

bool QueryHasRow(MYSQL* mysql, const std::string& query) {
    if (mysql_query(mysql, query.c_str()) != 0) {
        std::cerr << "schema query failed: " << mysql_error(mysql) << '\n';
        std::exit(1);
    }
    MYSQL_RES* result = mysql_store_result(mysql);
    const bool found = result != nullptr && mysql_num_rows(result) != 0;
    if (result != nullptr) {
        mysql_free_result(result);
    }
    return found;
}

void CheckLiveSchema() {
    const char* host = Env("OJ_ODB_TEST_HOST");
    const char* user = Env("OJ_ODB_TEST_USER");
    const char* password = Env("OJ_ODB_TEST_PASSWORD");
    const char* database = Env("OJ_ODB_TEST_DATABASE");
    if (host == nullptr || user == nullptr || password == nullptr || database == nullptr) {
        std::cout << "live MySQL schema checks skipped (set OJ_ODB_TEST_HOST, "
                     "OJ_ODB_TEST_USER, OJ_ODB_TEST_PASSWORD, OJ_ODB_TEST_DATABASE)\n";
        return;
    }

    MYSQL* mysql = mysql_init(nullptr);
    const char* port_value = Env("OJ_ODB_TEST_PORT");
    const unsigned int port = port_value == nullptr
        ? 3306U
        : static_cast<unsigned int>(std::strtoul(port_value, nullptr, 10));
    if (mysql == nullptr || mysql_real_connect(mysql, host, user, password, database,
                                               port, nullptr, 0) == nullptr) {
        std::cerr << "live MySQL connection failed: "
                  << (mysql == nullptr ? "mysql_init failed" : mysql_error(mysql)) << '\n';
        if (mysql != nullptr) mysql_close(mysql);
        std::exit(1);
    }

    const std::string schema(database);
    const auto column = [&](const char* table, const char* name, const char* nullable) {
        const std::string query =
            "SELECT 1 FROM information_schema.columns WHERE table_schema='" + schema +
            "' AND table_name='" + table + "' AND column_name='" + name +
            "' AND is_nullable='" + nullable + "' LIMIT 1";
        if (!QueryHasRow(mysql, query)) {
            std::cerr << "missing/mismatched column " << table << '.' << name << '\n';
            std::exit(1);
        }
    };
    column("users", "avatar", "YES");
    column("tests", "test_case_id", "NO");
    column("submissions", "submission_id", "NO");
    column("submissions", "completed_at", "YES");
    column("judge_outbox", "submission_id", "YES");
    column("judge_outbox", "custom_task_id", "YES");
    column("judge_outbox", "published_at", "YES");
    column("judge_result_receipts", "expires_at", "YES");

    const std::string unique_test_case =
        "SELECT 1 FROM information_schema.statistics WHERE table_schema='" + schema +
        "' AND table_name='tests' AND non_unique=0 GROUP BY index_name "
        "HAVING GROUP_CONCAT(column_name ORDER BY seq_in_index)='question_id,id' LIMIT 1";
    const std::string unique_receipt =
        "SELECT 1 FROM information_schema.statistics WHERE table_schema='" + schema +
        "' AND table_name='judge_result_receipts' AND column_name='message_id' "
        "AND non_unique=0 LIMIT 1";
    if (!QueryHasRow(mysql, unique_test_case) || !QueryHasRow(mysql, unique_receipt)) {
        std::cerr << "required unique schema constraints are missing\n";
        std::exit(1);
    }

    mysql_close(mysql);
    std::cout << "live MySQL schema checks passed\n";
}

} // namespace

int main() {
    using namespace oj::db;

    static_assert(std::is_same_v<Traits<User>::id_type, uint32_t>);
    static_assert(std::is_same_v<Traits<TestCase>::id_type, uint64_t>);
    static_assert(std::is_same_v<Traits<Submission>::id_type, uint64_t>);
    static_assert(std::is_same_v<decltype(std::declval<User>().avatar_path),
                                 odb::nullable<std::string>>);
    static_assert(std::is_same_v<decltype(std::declval<Comment>().parent_id),
                                 odb::nullable<uint64_t>>);
    static_assert(std::is_same_v<decltype(std::declval<Submission>().completed_at),
                                 odb::nullable<DateTime>>);
    static_assert(std::is_same_v<decltype(std::declval<JudgeOutbox>().submission_id),
                                 odb::nullable<uint64_t>>);

    CheckTable<User>("`users`", 9);
    CheckTable<Question>("`questions`", 8);
    CheckTable<Solution>("`solutions`", 11);
    CheckTable<Comment>("`solution_comments`", 11);
    CheckTable<Submission>("`submissions`", 15);
    CheckTable<AdminAccount>("`admin_accounts`", 5);
    CheckTable<TestCase>("`tests`", 6);
    CheckTable<SolutionAction>("`solution_actions`", 5);
    CheckTable<CommentAction>("`comment_actions`", 5);
    CheckTable<AdminAuditLog>("`admin_audit_logs`", 10);
    CheckTable<CacheMetricsSnapshot>("`cache_metrics_snapshots`", 7);
    CheckTable<JudgeOutbox>("`judge_outbox`", 10);
    CheckTable<JudgeResultReceipt>("`judge_result_receipts`", 7);
    CheckTable<LegacySubmission>("`user_submits`", 6);
    CheckTable<JudgeTask>("`judge_tasks`", 11);

    CheckLiveSchema();
    std::cout << "ODB model mapping checks passed\n";
    return 0;
}
