#pragma once

#include "../latecymonitor.hpp"

#include <odb/connection.hxx>
#include <odb/database.hxx>
#include <odb/exceptions.hxx>
#include <odb/mysql/database.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/transaction.hxx>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

namespace ns_odb
{
    using DatabasePtr = std::unique_ptr<odb::mysql::database>;

    struct ODBPoolConfig
    {
        std::string host = "localhost";
        unsigned int port = 3306;
        std::string user;
        std::string password;
        std::string database;
        std::string charset = "utf8mb4";
        unsigned int min_connections = 5;
        unsigned int max_connections = 50;
        unsigned int idle_timeout_s = 300;
        unsigned int health_check_interval_ms = 30000;
    };

    class ODBConnectionPool
    {
    public:
        using DatabaseFactory = std::function<DatabasePtr(const ODBPoolConfig&)>;
        using DatabaseSetup = std::function<void(odb::mysql::database&, const ODBPoolConfig&)>;
        using HealthCheck = std::function<void(odb::mysql::database&)>;

        ODBConnectionPool() = default;
        ~ODBConnectionPool() { Shutdown(); }

        ODBConnectionPool(const ODBConnectionPool&) = delete;
        ODBConnectionPool& operator=(const ODBConnectionPool&) = delete;

        static ODBConnectionPool& Instance()
        {
            static ODBConnectionPool instance;
            return instance;
        }

        void Init(const ODBPoolConfig& config)
        {
            Init(config, nullptr);
        }

        void Init(const ODBPoolConfig& config, latecyMonitor::LatencyMonitor& monitor)
        {
            Init(config, &monitor);
        }

        void Init(const ODBPoolConfig& config, latecyMonitor::LatencyMonitor* monitor,
                  DatabaseFactory factory = {}, DatabaseSetup setup = {}, HealthCheck health_check = {})
        {
            if (config.max_connections == 0 || config.min_connections > config.max_connections)
                throw std::invalid_argument("invalid ODB connection pool limits");

            std::lock_guard<std::mutex> lifecycle_lock(_lifecycle_mtx);
            std::lock_guard<std::mutex> lock(_mtx);
            if (_running)
                return;
            if (_active_count != 0)
                throw std::logic_error("cannot initialize ODB pool with checked-out connections");

            _config = config;
            _monitor = monitor;
            _database_factory = factory ? std::move(factory) : DefaultFactory();
            _database_setup = setup ? std::move(setup) : DefaultSetup();
            _health_check = health_check ? std::move(health_check) : DefaultHealthCheck();
            _running = true;

            for (unsigned int i = 0; i < _config.min_connections; ++i) {
                try {
                    _idle_pool.push({CreateDatabase(), std::chrono::steady_clock::now()});
                } catch (...) {
                    break;
                }
            }

            try {
                _health_check_thread = std::thread([this] { HealthCheckLoop(); });
            } catch (...) {
                _running = false;
                while (!_idle_pool.empty())
                    _idle_pool.pop();
                throw;
            }
        }

        DatabasePtr GetDatabase()
        {
            return Timed("DBPool.Acquire", [this]() -> DatabasePtr {
                std::unique_lock<std::mutex> lock(_mtx);
                _cv.wait(lock, [this] {
                    return !_running || !_idle_pool.empty() ||
                           _active_count < _config.max_connections;
                });

                if (!_running)
                    return nullptr;

                if (_idle_pool.empty()) {
                    ++_active_count;
                    lock.unlock();
                    try {
                        auto database = CreateDatabase();
                        return FinishCheckout(std::move(database));
                    } catch (...) {
                        ReleaseReservation();
                        throw;
                    }
                }

                auto item = std::move(_idle_pool.front());
                _idle_pool.pop();
                ++_active_count;
                lock.unlock();

                try {
                    Timed("DBPool.HealthCheck.Select1", [this, &item] {
                        _health_check(*item.first);
                    });
                    return FinishCheckout(std::move(item.first));
                } catch (...) {
                    {
                        std::lock_guard<std::mutex> state_lock(_mtx);
                        if (!_running) {
                            --_active_count;
                            _cv.notify_all();
                            return nullptr;
                        }
                    }

                    try {
                        auto replacement = Timed("DBPool.Replace", [this] {
                            return CreateDatabase();
                        });
                        return FinishCheckout(std::move(replacement));
                    } catch (...) {
                        ReleaseReservation();
                        throw;
                    }
                }
            });
        }

        void ReturnDatabase(DatabasePtr database)
        {
            if (!database)
                return;

            {
                std::lock_guard<std::mutex> lock(_mtx);
                if (_active_count == 0)
                    return;
                --_active_count;
                if (_running)
                    _idle_pool.push({std::move(database), std::chrono::steady_clock::now()});
            }
            _cv.notify_all();
        }

        size_t ActiveCount() const
        {
            std::lock_guard<std::mutex> lock(_mtx);
            return _active_count;
        }

        size_t IdleCount() const
        {
            std::lock_guard<std::mutex> lock(_mtx);
            return _idle_pool.size();
        }

        void Shutdown()
        {
            std::lock_guard<std::mutex> lifecycle_lock(_lifecycle_mtx);
            {
                std::lock_guard<std::mutex> lock(_mtx);
                _running = false;
            }
            _cv.notify_all();

            if (_health_check_thread.joinable())
                _health_check_thread.join();

            std::lock_guard<std::mutex> lock(_mtx);
            while (!_idle_pool.empty())
                _idle_pool.pop();
        }

    private:
        using IdleDatabase = std::pair<DatabasePtr, std::chrono::steady_clock::time_point>;

        static DatabaseFactory DefaultFactory()
        {
            return [](const ODBPoolConfig& config) {
                return std::make_unique<odb::mysql::database>(
                    config.user, config.password, config.database, config.host, config.port);
            };
        }

        static DatabaseSetup DefaultSetup()
        {
            return [](odb::mysql::database& database, const ODBPoolConfig& config) {
                database.connection()->execute("SET NAMES " + config.charset);
            };
        }

        static HealthCheck DefaultHealthCheck()
        {
            return [](odb::mysql::database& database) {
                database.connection()->execute("SELECT 1");
            };
        }

        template <typename Function>
        std::invoke_result_t<Function> Timed(const std::string& operation, Function&& function)
        {
            if (_monitor != nullptr) {
                latecyMonitor::Timer timer(*_monitor, operation);
                return std::forward<Function>(function)();
            }
            return std::forward<Function>(function)();
        }

        DatabasePtr CreateDatabase()
        {
            auto database = Timed("DBPool.Create", [this] {
                return _database_factory(_config);
            });
            if (!database)
                throw std::runtime_error("ODB database factory returned null");
            Timed("DBPool.SetNames", [this, &database] {
                _database_setup(*database, _config);
            });
            return database;
        }

        DatabasePtr FinishCheckout(DatabasePtr database)
        {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_running)
                return database;
            --_active_count;
            _cv.notify_all();
            return nullptr;
        }

        void ReleaseReservation()
        {
            {
                std::lock_guard<std::mutex> lock(_mtx);
                --_active_count;
            }
            _cv.notify_all();
        }

        void HealthCheckLoop()
        {
            std::unique_lock<std::mutex> lock(_mtx);
            while (_running) {
                const auto interval = std::chrono::milliseconds(
                    std::max(1u, _config.health_check_interval_ms));
                if (_cv.wait_for(lock, interval, [this] { return !_running; }))
                    break;

                const auto now = std::chrono::steady_clock::now();
                const auto timeout = std::chrono::seconds(_config.idle_timeout_s);
                std::queue<IdleDatabase> retained;
                while (!_idle_pool.empty()) {
                    auto item = std::move(_idle_pool.front());
                    _idle_pool.pop();
                    if (retained.size() < _config.min_connections || now - item.second < timeout)
                        retained.push(std::move(item));
                }
                _idle_pool = std::move(retained);
            }
        }

        ODBPoolConfig _config;
        std::queue<IdleDatabase> _idle_pool;
        mutable std::mutex _mtx;
        std::mutex _lifecycle_mtx;
        std::condition_variable _cv;
        size_t _active_count = 0;
        bool _running = false;
        std::thread _health_check_thread;
        latecyMonitor::LatencyMonitor* _monitor = nullptr;
        DatabaseFactory _database_factory;
        DatabaseSetup _database_setup;
        HealthCheck _health_check;
    };

    class ScopedDB
    {
    public:
        ScopedDB() : ScopedDB(ODBConnectionPool::Instance()) {}

        explicit ScopedDB(ODBConnectionPool& pool) : _pool(&pool), _db(pool.GetDatabase()) {}

        ~ScopedDB() { Reset(); }

        ScopedDB(const ScopedDB&) = delete;
        ScopedDB& operator=(const ScopedDB&) = delete;

        ScopedDB(ScopedDB&& other) noexcept
            : _pool(other._pool), _db(std::move(other._db))
        {
            other._pool = nullptr;
        }

        ScopedDB& operator=(ScopedDB&& other) noexcept
        {
            if (this != &other) {
                Reset();
                _pool = other._pool;
                _db = std::move(other._db);
                other._pool = nullptr;
            }
            return *this;
        }

        odb::database* operator->() { return _db.get(); }
        odb::database& operator*() { return *_db; }
        odb::database* Get() { return _db.get(); }

    private:
        void Reset() noexcept
        {
            if (_pool != nullptr && _db)
                _pool->ReturnDatabase(std::move(_db));
            _pool = nullptr;
        }

        ODBConnectionPool* _pool = nullptr;
        DatabasePtr _db;
    };

    class ScopedTransaction
    {
    public:
        explicit ScopedTransaction(odb::database& database)
            : _tx(std::make_unique<odb::transaction>(database.begin())) {}

        void Commit()
        {
            if (_tx && !_committed) {
                _tx->commit();
                _committed = true;
            }
        }

        void Rollback()
        {
            if (_tx && !_committed) {
                _tx->rollback();
                _committed = true;
            }
        }

        ~ScopedTransaction() noexcept
        {
            if (_tx && !_committed) {
                try {
                    _tx->rollback();
                } catch (...) {
                    const std::uint64_t failures =
                        RollbackFailureCounter().fetch_add(1, std::memory_order_relaxed) + 1;
                    if ((failures & (failures - 1)) == 0) {
                        std::fprintf(stderr,
                                     "ScopedTransaction rollback failed during destruction "
                                     "(total failures: %llu)\n",
                                     static_cast<unsigned long long>(failures));
                    }
                }
            }
        }

        static std::uint64_t RollbackFailureCount() noexcept
        {
            return RollbackFailureCounter().load(std::memory_order_relaxed);
        }

        ScopedTransaction(const ScopedTransaction&) = delete;
        ScopedTransaction& operator=(const ScopedTransaction&) = delete;

    private:
        static std::atomic<std::uint64_t>& RollbackFailureCounter() noexcept
        {
            static std::atomic<std::uint64_t> failures{0};
            return failures;
        }

        std::unique_ptr<odb::transaction> _tx;
        bool _committed = false;
    };
} // namespace ns_odb
