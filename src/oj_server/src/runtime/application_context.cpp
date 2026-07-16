#include "runtime/application_context.hpp"

#include <stdexcept>
#include <utility>

namespace oj::runtime
{

std::string ToString(LifecycleStage stage)
{
    switch (stage) {
    case LifecycleStage::None: return "None";
    case LifecycleStage::Logger: return "Logger";
    case LifecycleStage::Monitor: return "Monitor";
    case LifecycleStage::Odb: return "ODB";
    case LifecycleStage::Redis: return "Redis";
    case LifecycleStage::Mq: return "MQ";
    case LifecycleStage::Executor: return "Executor";
    }
    return "Unknown";
}

std::string ToString(LifecycleEvent event)
{
    switch (event) {
    case LifecycleEvent::Started: return "started";
    case LifecycleEvent::Stopped: return "stopped";
    }
    return "unknown";
}

ApplicationContext::ApplicationContext(Config config, OdbPoolService& odb, MqService& mq,
                                       LifecycleCallbacks logger, LifecycleCallbacks redis,
                                       LifecycleObserver observer)
    : config_(std::move(config)), odb_(odb), mq_(mq), logger_(std::move(logger)),
      redis_(std::move(redis)), observer_(std::move(observer)),
      _executor(config_.executor, config_._executorexception_handler)
{
}

ApplicationContext::~ApplicationContext()
{
    Stop();
}

bool ApplicationContext::Start()
{
    std::lock_guard lock(lifecycle_mutex_);
    if (running_)
        return true;
    if (terminal_)
        return false;

    last_start_error_ = nullptr;
    failed_stage_ = LifecycleStage::None;
    try {
        failed_stage_ = LifecycleStage::Logger;
        if (logger_.start)
            logger_.start();
        logger_started_ = true;
        Notify(LifecycleStage::Logger, LifecycleEvent::Started);

        failed_stage_ = LifecycleStage::Monitor;
        if (!monitor_.configure(config_.monitor))
            throw std::runtime_error("latency monitor configuration failed");
        if (!monitor_.start())
            throw std::runtime_error("latency monitor start failed");
        monitor_.enable(config_.enable_monitor);
        monitor_started_ = true;
        Notify(LifecycleStage::Monitor, LifecycleEvent::Started);

        failed_stage_ = LifecycleStage::Odb;
        odb_.Start(config_.odb, monitor_);
        odb_started_ = true;
        Notify(LifecycleStage::Odb, LifecycleEvent::Started);

        failed_stage_ = LifecycleStage::Redis;
        if (redis_.start)
            redis_.start();
        redis_started_ = true;
        Notify(LifecycleStage::Redis, LifecycleEvent::Started);

        failed_stage_ = LifecycleStage::Mq;
        mq_.Start();
        mq_started_ = true;
        Notify(LifecycleStage::Mq, LifecycleEvent::Started);

        failed_stage_ = LifecycleStage::Executor;
        if (!_executor.Start())
            throw std::runtime_error("business executor start failed");
        _executorstarted_ = true;
        Notify(LifecycleStage::Executor, LifecycleEvent::Started);

        failed_stage_ = LifecycleStage::None;
        running_ = true;
        return true;
    } catch (...) {
        last_start_error_ = std::current_exception();
        terminal_ = true;
        StopStartedComponents();
        return false;
    }
}

void ApplicationContext::Stop() noexcept
{
    std::lock_guard lock(lifecycle_mutex_);
    if (terminal_ && !running_)
        return;
    terminal_ = true;
    StopStartedComponents();
}

bool ApplicationContext::IsRunning() const
{
    std::lock_guard lock(lifecycle_mutex_);
    return running_;
}

std::exception_ptr ApplicationContext::LastStartError() const
{
    std::lock_guard lock(lifecycle_mutex_);
    return last_start_error_;
}

LifecycleStage ApplicationContext::FailedStage() const
{
    std::lock_guard lock(lifecycle_mutex_);
    return failed_stage_;
}

void ApplicationContext::Notify(LifecycleStage stage, LifecycleEvent event) noexcept
{
    try {
        if (observer_)
            observer_(stage, event);
    } catch (...) {
    }
}

void ApplicationContext::StopStartedComponents() noexcept
{
    if (_executorstarted_) {
        _executorstarted_ = false;
        Notify(LifecycleStage::Executor, LifecycleEvent::Stopped);
    }
    if (mq_started_) {
        mq_.Stop();
        mq_started_ = false;
        Notify(LifecycleStage::Mq, LifecycleEvent::Stopped);
    }
    if (redis_started_) {
        try {
            if (redis_.stop)
                redis_.stop();
        } catch (...) {
        }
        redis_started_ = false;
        Notify(LifecycleStage::Redis, LifecycleEvent::Stopped);
    }
    if (odb_started_) {
        odb_.Stop();
        odb_started_ = false;
        Notify(LifecycleStage::Odb, LifecycleEvent::Stopped);
    }
    if (monitor_started_) {
        monitor_.stop();
        monitor_started_ = false;
        Notify(LifecycleStage::Monitor, LifecycleEvent::Stopped);
    }
    if (logger_started_) {
        try {
            if (logger_.stop)
                logger_.stop();
        } catch (...) {
        }
        logger_started_ = false;
        Notify(LifecycleStage::Logger, LifecycleEvent::Stopped);
    }
    running_ = false;
}

} // namespace oj::runtime
