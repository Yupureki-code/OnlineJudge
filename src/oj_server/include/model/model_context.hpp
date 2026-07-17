#pragma once

#include "../../../comm/config.h"
#include "../../../comm/latecymonitor.hpp"
#include "../../../comm/odb/connection_pool.hpp"
#include "../../../comm/logger.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

namespace oj::model
{
    using namespace oj::logger;
    class ModelContext
    {
    public:
        static ModelContext& Instance()
        {
            static ModelContext context;
            return context;
        }

        bool Start(const std::string& process_name)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_started)
                return true;

            try
            {
                auto monitor_config = latecyMonitor::LatencyConfigFromEnv(process_name);
                if (!_monitor.configure(monitor_config) || !_monitor.start())
                    return false;
            }
            catch (...)
            {
                _monitor.stop();
                return false;
            }
            _monitor.enable(true);

            ns_odb::ODBPoolConfig pool_config;
            pool_config.host = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_HOST", host);
            const int configured_port = ns_runtime_cfg::GetEnvIntOrDefault("OJ_DB_PORT", port);
            if (configured_port <= 0 || configured_port > 65535)
            {
                _monitor.stop();
                return false;
            }
            pool_config.port = static_cast<unsigned int>(configured_port);
            pool_config.user = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_USER", user);
            pool_config.password = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_PASS", passwd);
            pool_config.database = ns_runtime_cfg::GetEnvOrDefault("OJ_DB_NAME", ::db);
            try {
                _pool.Init(pool_config, _monitor);
            }
            catch (...)
            {
                _monitor.stop();
                return false;
            }
            _started = true;
            return true;
        }

        void Stop() noexcept
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_started.exchange(false, std::memory_order_acq_rel))
                return;
            _pool.Shutdown();
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
            while (_pool.ActiveCount() != 0 && std::chrono::steady_clock::now() < deadline)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (_pool.ActiveCount() != 0)
                LOG_ERROR("ODB shutdown timed out with {} active connection(s)", _pool.ActiveCount());
            _monitor.stop();
        }

        ns_odb::ODBConnectionPool& Pool() noexcept { return _pool; }
        latecyMonitor::LatencyMonitor& Monitor() noexcept { return _monitor; }
        bool Started() const noexcept { return _started.load(std::memory_order_acquire); }

    private:
        ModelContext() : _pool(ns_odb::ODBConnectionPool::Instance()) {}
        ~ModelContext() { Stop(); }

        ns_odb::ODBConnectionPool& _pool;
        latecyMonitor::LatencyMonitor _monitor;
        mutable std::mutex _mutex;
        std::atomic<bool> _started{false};
    };
}
