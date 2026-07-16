#pragma once

#include "runtime/business_executor.hpp"
#include "../../../comm/latecymonitor.hpp"
#include "../../../comm/odb/connection_pool.hpp"

#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <utility>

namespace oj::runtime
{

struct LifecycleCallbacks
{
    std::function<void()> start;
    std::function<void()> stop;
};

class OdbPoolService
{
public:
    virtual ~OdbPoolService() = default;
    virtual void Start(const ns_odb::ODBPoolConfig& config,
                       latecyMonitor::LatencyMonitor& monitor) = 0;
    virtual void Stop() noexcept = 0;
};

// Adapts a pool owned elsewhere, including ODBConnectionPool::Instance().
class BorrowedOdbPool final : public OdbPoolService
{
public:
    explicit BorrowedOdbPool(ns_odb::ODBConnectionPool& pool) : pool_(pool) {}

    void Start(const ns_odb::ODBPoolConfig& config,
               latecyMonitor::LatencyMonitor& monitor) override
    {
        pool_.Init(config, monitor);
    }

    void Stop() noexcept override { pool_.Shutdown(); }
    ns_odb::ODBConnectionPool& Pool() noexcept { return pool_; }

private:
    ns_odb::ODBConnectionPool& pool_;
};

class MqService
{
public:
    virtual ~MqService() = default;
    virtual void Start() = 0;
    virtual void Stop() noexcept = 0;
    virtual bool Publish(const std::string& body) = 0;
};

// Keeps ownership of an MQ manager/producer outside the context and delegates through callbacks.
class BorrowedMqService final : public MqService
{
public:
    using PublishCallback = std::function<bool(const std::string&)>;

    BorrowedMqService(LifecycleCallbacks lifecycle, PublishCallback publish)
        : lifecycle_(std::move(lifecycle)), publish_(std::move(publish)) {}

    void Start() override
    {
        if (lifecycle_.start)
            lifecycle_.start();
    }

    void Stop() noexcept override
    {
        try {
            if (lifecycle_.stop)
                lifecycle_.stop();
        } catch (...) {
        }
    }

    bool Publish(const std::string& body) override
    {
        return publish_ && publish_(body);
    }

private:
    LifecycleCallbacks lifecycle_;
    PublishCallback publish_;
};

enum class LifecycleStage
{
    None,
    Logger,
    Monitor,
    Odb,
    Redis,
    Mq,
    Executor
};

enum class LifecycleEvent
{
    Started,
    Stopped
};

std::string ToString(LifecycleStage stage);
std::string ToString(LifecycleEvent event);

class ApplicationContext
{
public:
    struct Config
    {
        std::string process_name;
        latecyMonitor::LatencyMonitorConfig monitor;
        ns_odb::ODBPoolConfig odb;
        BusinessExecutor::Config executor;
        BusinessExecutor::ExceptionHandler _executorexception_handler;
        bool enable_monitor = true;
    };

    using LifecycleObserver = std::function<void(LifecycleStage, LifecycleEvent)>;

    ApplicationContext(Config config, OdbPoolService& odb, MqService& mq,
                       LifecycleCallbacks logger, LifecycleCallbacks redis,
                       LifecycleObserver observer = {});
    ~ApplicationContext();

    ApplicationContext(const ApplicationContext&) = delete;
    ApplicationContext& operator=(const ApplicationContext&) = delete;

    bool Start();
    void Stop() noexcept;
    bool IsRunning() const;
    std::exception_ptr LastStartError() const;
    LifecycleStage FailedStage() const;

    latecyMonitor::LatencyMonitor& Monitor() noexcept { return monitor_; }
    BusinessExecutor& Executor() noexcept { return _executor; }
    OdbPoolService& Odb() noexcept { return odb_; }
    MqService& Mq() noexcept { return mq_; }
    const ns_odb::ODBPoolConfig& OdbConfig() const noexcept { return config_.odb; }

private:
    void Notify(LifecycleStage stage, LifecycleEvent event) noexcept;
    void StopStartedComponents() noexcept;

    Config config_;
    OdbPoolService& odb_;
    MqService& mq_;
    LifecycleCallbacks logger_;
    LifecycleCallbacks redis_;
    LifecycleObserver observer_;
    latecyMonitor::LatencyMonitor monitor_;
    BusinessExecutor _executor;
    mutable std::mutex lifecycle_mutex_;
    std::exception_ptr last_start_error_;
    LifecycleStage failed_stage_ = LifecycleStage::None;
    bool logger_started_ = false;
    bool monitor_started_ = false;
    bool odb_started_ = false;
    bool redis_started_ = false;
    bool mq_started_ = false;
    bool _executorstarted_ = false;
    bool running_ = false;
    bool terminal_ = false;
};

} // namespace oj::runtime
