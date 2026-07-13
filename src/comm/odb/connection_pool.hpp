#pragma once
#include <odb/database.hxx>
#include <odb/connection.hxx>
#include <odb/mysql/database.hxx>
#include <odb/transaction.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/exceptions.hxx>
#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <thread>

namespace ns_odb 
{
    using DatabasePtr = std::unique_ptr<odb::mysql::database>;
    /// ODB 连接池配置
    struct ODBPoolConfig 
    {
        std::string host = "localhost";
        unsigned int port = 3306;
        std::string user;
        std::string password;
        std::string database;
        std::string charset = "utf8mb4";
        unsigned int min_connections = 5;   // 最小空闲连接数
        unsigned int max_connections = 50;  // 最大连接数
        unsigned int idle_timeout_s = 300;  // 空闲连接超时(秒)
    };

    // ==================== ODBConnectionPool（声明） ====================

    class ODBConnectionPool 
    {
    private:
        ODBConnectionPool() = default;
        ODBConnectionPool(const ODBConnectionPool&) = delete;
        ODBConnectionPool& operator=(const ODBConnectionPool&) = delete;

        DatabasePtr CreateDatabase() 
        {
            auto db = std::make_unique<odb::mysql::database>(
                _config.user, _config.password, _config.database, _config.host, _config.port);
            db->connection()->execute("SET NAMES " + _config.charset);
            return db;
        }

        void HealthCheckLoop() 
        {
            while (_running.load()) 
            {
                std::this_thread::sleep_for(std::chrono::seconds(30));
                std::lock_guard<std::mutex> lock(_mtx);
                auto now = std::chrono::steady_clock::now();
                auto timeout = std::chrono::seconds(_config.idle_timeout_s);
                std::queue<std::pair<DatabasePtr, std::chrono::steady_clock::time_point>> new_pool;
                while (!_idle_pool.empty()) 
                {
                    auto item = std::move(_idle_pool.front()); _idle_pool.pop();
                    if (new_pool.size() < _config.min_connections || (now - item.second) < timeout)
                        new_pool.push(std::move(item));
                }
                _idle_pool = std::move(new_pool);
            }
        }
    public:
        static ODBConnectionPool& Instance() {
            static ODBConnectionPool instance;
            return instance;
        }

        ~ODBConnectionPool() { Shutdown(); }

        void Init(const ODBPoolConfig& config) {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_running.load()) return;
            _config = config;
            _running.store(true);

            for (unsigned int i = 0; i < _config.min_connections; ++i) {
                try {
                    auto db = CreateDatabase();
                    _idle_pool.push({std::move(db), std::chrono::steady_clock::now()});
                } catch (const std::exception&) {
                    break;
                }
            }

            _health_check_thread = std::make_unique<std::thread>([this]() { HealthCheckLoop(); });
        }

        DatabasePtr GetDatabase() {
            std::unique_lock<std::mutex> lock(_mtx);
            _cv.wait(lock, [this]() {
                return !_idle_pool.empty() || _active_count.load() < _config.max_connections || !_running.load();
            });

            if (!_running.load()) return nullptr;

            if (_idle_pool.empty()) {
                _active_count.fetch_add(1);
                lock.unlock();
                try { return CreateDatabase(); } catch (...) {
                    _active_count.fetch_sub(1); _cv.notify_one(); throw;
                }
            }

            auto [db, ts] = std::move(_idle_pool.front());
            _idle_pool.pop();
            _active_count.fetch_add(1);
            lock.unlock();

            try { db->connection()->execute("SELECT 1"); } catch (...) {
                _active_count.fetch_sub(1);
                try { auto ndb = CreateDatabase(); _active_count.fetch_add(1); return ndb; }
                catch (...) { _cv.notify_one(); throw; }
            }
            return std::move(db);
        }

        void ReturnDatabase(DatabasePtr db) {
            if (!db) return;
            std::lock_guard<std::mutex> lock(_mtx);
            _idle_pool.push({std::move(db), std::chrono::steady_clock::now()});
            _active_count.fetch_sub(1);
            _cv.notify_one();
        }

        size_t ActiveCount() const { return _active_count.load(); }

        size_t IdleCount() const {
            std::lock_guard<std::mutex> lock(_mtx);
            return _idle_pool.size();
        }

        void Shutdown() {
            _running.store(false);
            _cv.notify_all();
            if (_health_check_thread && _health_check_thread->joinable()) _health_check_thread->join();
            std::lock_guard<std::mutex> lock(_mtx);
            while (!_idle_pool.empty()) _idle_pool.pop();
        }

    private:
        ODBPoolConfig _config;
        std::queue<std::pair<DatabasePtr, std::chrono::steady_clock::time_point>> _idle_pool;
        mutable std::mutex _mtx;
        std::condition_variable _cv;
        std::atomic<size_t> _active_count{0};
        std::atomic<bool> _running{false};
        std::unique_ptr<std::thread> _health_check_thread;
    };

    // ==================== ScopedDB ====================

    class ScopedDB {
    public:
        ScopedDB() { _db = ODBConnectionPool::Instance().GetDatabase(); }
        odb::database* operator->() { return _db.get(); }
        odb::database& operator*() { return *_db; }
        odb::database* Get() { return _db.get(); }
        ~ScopedDB() { if (_db) ODBConnectionPool::Instance().ReturnDatabase(std::move(_db)); }
        ScopedDB(const ScopedDB&) = delete;
        ScopedDB& operator=(const ScopedDB&) = delete;
        ScopedDB(ScopedDB&&) = default;
        ScopedDB& operator=(ScopedDB&&) = default;

    private:
        DatabasePtr _db;
    };

    // ==================== ScopedTransaction ====================

    class ScopedTransaction {
    public:
        explicit ScopedTransaction(odb::database& db) {
            _tx = std::make_unique<odb::transaction>(db.begin());
        }
        void Commit() { if (_tx && !_committed) { _tx->commit(); _committed = true; } }
        void Rollback() { if (_tx && !_committed) { _tx->rollback(); _committed = true; } }
        ~ScopedTransaction() {
            if (_tx && !_committed) { try { _tx->rollback(); } catch (...) {} }
        }
        ScopedTransaction(const ScopedTransaction&) = delete;
        ScopedTransaction& operator=(const ScopedTransaction&) = delete;

    private:
        std::unique_ptr<odb::transaction> _tx;
        bool _committed = false;
    };

} // namespace ns_odb