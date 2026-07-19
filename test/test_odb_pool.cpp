// test/test_odb_pool.cpp
// ODB 连接池 + ORM 模型单元测试
//
// 编译：需要 ODB 生成的代码 + libodb-mysql
// 运行：需要 MySQL 可访问

#include "../src/comm/odb/connection_pool.hpp"
#include "../src/comm/models/user.hxx"
#include "../src/comm/models/question.hxx"
#include "../src/comm/models/solution.hxx"
#include "../src/comm/models/comment.hxx"
#include "../src/comm/models/submission.hxx"
#include "../src/comm/models/admin.hxx"

#include <cassert>
#include <iostream>
#include <string>

using namespace ns_odb;
using namespace ns_model;

// ==================== 测试辅助 ====================

static void InitPool() {
    ODBPoolConfig config;
    config.host = "localhost";
    config.port = 3306;
    config.user = "oj_server";
    config.password = "password";
    config.database = "myoj";
    config.min_connections = 2;
    config.max_connections = 10;
    ODBConnectionPool::Instance().Init(config);
}

// ==================== 测试用例 ====================

/// 测试 1：连接池基本获取/归还
void test_pool_basic() {
    std::cout << "[TEST] Pool basic acquire/release...";

    auto db1 = ScopedDB();
    assert(db1.Get() != nullptr);
    assert(ODBConnectionPool::Instance().ActiveCount() >= 1);

    {
        auto db2 = ScopedDB();
        assert(db2.Get() != nullptr);
        assert(ODBConnectionPool::Instance().ActiveCount() >= 2);
    }  // db2 析构归还

    assert(ODBConnectionPool::Instance().IdleCount() >= 1);

    std::cout << " PASSED" << std::endl;
}

/// 测试 2：User CRUD
void test_user_crud() {
    std::cout << "[TEST] User CRUD...";

    auto db = ScopedDB();

    // INSERT
    User user;
    user.name = "test_user_odb";
    user.email = "test_odb@example.com";
    user.password_hash = "salt$120000$hash";
    user.password_algo = "sha256_iter_v1";
    user.role = 0;

    {
        ScopedTransaction tx(*db);
        db->persist(user);
        tx.Commit();
    }
    assert(user.uid > 0);

    // SELECT
    {
        ScopedTransaction tx(*db);
        auto result = db->query_one<User>(
            odb::query<User>::email == "test_odb@example.com");
        assert(result != nullptr);
        assert(result->name == "test_user_odb");
    }

    // UPDATE
    {
        ScopedTransaction tx(*db);
        user.name = "updated_odb_user";
        db->update(user);
        tx.Commit();
    }

    // 验证更新
    {
        ScopedTransaction tx(*db);
        auto result = db->query_one<User>(
            odb::query<User>::uid == user.uid);
        assert(result != nullptr);
        assert(result->name == "updated_odb_user");
    }

    // DELETE
    {
        ScopedTransaction tx(*db);
        db->erase(user);
        tx.Commit();
    }

    // 验证删除
    {
        ScopedTransaction tx(*db);
        auto result = db->query_one<User>(
            odb::query<User>::uid == user.uid);
        assert(result == nullptr);
    }

    std::cout << " PASSED" << std::endl;
}

/// 测试 3：Question 查询
void test_question_query() {
    std::cout << "[TEST] Question query...";

    auto db = ScopedDB();

    // 查询所有题目
    {
        ScopedTransaction tx(*db);
        auto result = db->query<Question>("ORDER BY id LIMIT 10");
        int count = 0;
        for (auto it = result.begin(); it != result.end(); ++it) {
            count++;
        }
        std::cout << " (found " << count << " questions) ";
    }

    std::cout << " PASSED" << std::endl;
}

/// 测试 4：事务回滚
void test_transaction_rollback() {
    std::cout << "[TEST] Transaction rollback...";

    auto db = ScopedDB();

    User user;
    user.name = "rollback_test_user";
    user.email = "rollback@example.com";
    user.password_hash = "hash";
    user.password_algo = "sha256_iter_v1";
    user.role = 0;

    // 插入后回滚
    {
        ScopedTransaction tx(*db);
        db->persist(user);
        tx.Rollback();  // 回滚
    }

    // 验证未插入
    {
        ScopedTransaction tx(*db);
        auto result = db->query_one<User>(
            odb::query<User>::email == "rollback@example.com");
        assert(result == nullptr);
    }

    std::cout << " PASSED" << std::endl;
}

/// 测试 5：并发安全
void test_concurrent_access() {
    std::cout << "[TEST] Concurrent access...";

    const int thread_count = 5;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&success_count, i]() {
            try {
                auto db = ScopedDB();
                ScopedTransaction tx(*db);
                auto result = db->query<User>("LIMIT 1");
                tx.Commit();
                success_count.fetch_add(1);
            } catch (...) {
                // 忽略错误
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    assert(success_count.load() == thread_count);
    std::cout << " PASSED (" << success_count.load() << "/" << thread_count << ")" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "=== ODB Connection Pool Tests ===" << std::endl;

    try {
        InitPool();
    } catch (const std::exception& e) {
        std::cerr << "Failed to init pool: " << e.what() << std::endl;
        return 1;
    }

    test_pool_basic();
    test_user_crud();
    test_question_query();
    test_transaction_rollback();
    test_concurrent_access();

    ODBConnectionPool::Instance().Shutdown();
    std::cout << "=== All tests passed ===" << std::endl;
    return 0;
}