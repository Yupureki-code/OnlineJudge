#pragma once

#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <amqpcpp/reliable.h>
#include <event2/event.h>
#include <event2/thread.h>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <limits>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>

namespace ns_mq
{

struct MQConfig
{
    std::string host = "localhost";
    uint16_t port = 5672;
    std::string user = "oj";
    std::string password = "ojpassword";
    std::string vhost = "/";
    std::string queue_name = "oj.judge.task";
    std::string exchange = "oj.judge";
    std::string routing_key = "oj.judge.task";
    std::string retry_exchange = "oj.judge.retry";
    std::string retry_queue_prefix = "oj.judge.retry";
    std::string dead_letter_exchange = "oj.judge.dlx";
    std::string dead_letter_queue = "oj.judge.task.dlx";
    int prefetch_count = 1;
    int confirm_timeout_ms = 5000;
};

struct DeliveryContext
{
    uint64_t delivery_tag = 0;
    uint64_t channel_generation = 0;
};

using MessageCallback = std::function<void(const std::string&, DeliveryContext, bool, uint32_t)>;
using ConfirmCallback = std::function<void(bool, const std::string&)>;
using ErrorCallback = std::function<void(const std::string&)>;

class EventThreadCommands
{
protected:
    bool InitEventThread()
    {
        static std::once_flag threading;
        std::call_once(threading, [] { evthread_use_pthreads(); });
        _base.reset(event_base_new());
        if (!_base) return false;
        _command_event.reset(event_new(_base.get(), -1, EV_PERSIST, &EventThreadCommands::OnCommands, this));
        if (!_command_event || event_add(_command_event.get(), nullptr) != 0) return false;
        return true;
    }

    bool Post(std::function<void()> command)
    {
        if (!_base || _closing.load(std::memory_order_acquire)) return false;
        {
            std::lock_guard lock(_command_mutex);
            _commands.push(std::move(command));
        }
        event_active(_command_event.get(), 0, 0);
        return true;
    }

    void DrainCommands()
    {
        std::queue<std::function<void()>> commands;
        {
            std::lock_guard lock(_command_mutex);
            commands.swap(_commands);
        }
        while (!commands.empty()) {
            auto command = std::move(commands.front());
            commands.pop();
            if (command) command();
        }
    }

    void BreakLoop()
    {
        _closing.store(true, std::memory_order_release);
        if (_base) event_base_loopbreak(_base.get());
    }

    struct EventBaseDeleter { void operator()(event_base* value) const { if (value) event_base_free(value); } };
    struct EventDeleter { void operator()(event* value) const { if (value) event_free(value); } };

    std::unique_ptr<event_base, EventBaseDeleter> _base;
    std::unique_ptr<event, EventDeleter> _command_event;
    std::atomic<bool> _closing{false};

private:
    static void OnCommands(evutil_socket_t, short, void* context)
    {
        static_cast<EventThreadCommands*>(context)->DrainCommands();
    }

    std::mutex _command_mutex;
    std::queue<std::function<void()>> _commands;
};

class MQProducer : private EventThreadCommands
{
public:
    MQProducer() = default;
    ~MQProducer() { Close(); }

    void Init(const MQConfig& config)
    {
        _config = config;
        if (!InitEventThread()) throw std::runtime_error("failed to initialize MQ producer event loop");
        _event_thread = std::thread([this] { event_base_dispatch(_base.get()); });
        auto ready = std::make_shared<std::promise<void>>();
        auto future = ready->get_future();
        if (!Post([this, ready] { InitializeChannel(ready); }))
            throw std::runtime_error("failed to schedule MQ producer initialization");
        if (future.wait_for(std::chrono::seconds(5)) != std::future_status::ready)
            throw std::runtime_error("MQ producer initialization timed out");
        future.get();
    }

    bool PublishConfirmed(std::string message_id, std::string body, ConfirmCallback callback,
                          const std::string& exchange = {}, const std::string& routing_key = {})
    {
        if (message_id.empty() || !_connected.load(std::memory_order_acquire)) return false;
        return Post([this, message_id = std::move(message_id), body = std::move(body),
                     callback = std::move(callback), exchange, routing_key]() mutable {
            if (!_reliable || !_connected.load(std::memory_order_acquire)) {
                if (callback) callback(false, "NOT_CONNECTED");
                return;
            }
            AMQP::Envelope envelope(body.data(), body.size());
            envelope.setDeliveryMode(2);
            envelope.setMessageID(message_id);
            const std::string& target_exchange = exchange.empty() ? _config.exchange : exchange;
            const std::string& target_key = routing_key.empty() ? _config.routing_key : routing_key;
            _reliable->publish(target_exchange, target_key, envelope, AMQP::mandatory)
                .onAck([this, message_id, callback]() {
                    const bool returned = _returned.erase(message_id) != 0;
                    if (callback) callback(!returned, returned ? "UNROUTABLE" : std::string{});
                })
                .onNack([callback]() { if (callback) callback(false, "NACK"); })
                .onError([callback](const char* error) {
                    if (callback) callback(false, error == nullptr ? "PUBLISH_ERROR" : error);
                });
        });
    }

    bool Publish(const std::string& body)
    {
        const std::string message_id = "legacy-" + std::to_string(
            _legacy_sequence.fetch_add(1, std::memory_order_relaxed) + 1);
        auto result = std::make_shared<std::promise<bool>>();
        auto future = result->get_future();
        if (!PublishConfirmed(message_id, body, [result](bool ok, const std::string&) {
                try { result->set_value(ok); } catch (...) {}
            })) return false;
        if (future.wait_for(std::chrono::milliseconds(_config.confirm_timeout_ms)) != std::future_status::ready)
            return false;
        return future.get();
    }

    void Close()
    {
        if (!_base || _close_started.exchange(true, std::memory_order_acq_rel)) return;
        auto closed = std::make_shared<std::promise<void>>();
        auto future = closed->get_future();
        const bool posted = Post([this, closed] {
            if (_connection) _connection->close();
            _closing.store(true, std::memory_order_release);
            try { closed->set_value(); } catch (...) {}
            event_base_loopbreak(_base.get());
        });
        if (posted) future.wait_for(std::chrono::seconds(1));
        else event_base_loopbreak(_base.get());
        if (_event_thread.joinable()) _event_thread.join();
        _connected.store(false, std::memory_order_release);
        _reliable.reset();
        _channel.reset();
        _connection.reset();
        _handler.reset();
    }

    bool IsConnected() const { return _connected.load(std::memory_order_acquire); }
    void SetErrorHandler(ErrorCallback callback) { _error_callback = std::move(callback); }

private:
    void InitializeChannel(const std::shared_ptr<std::promise<void>>& ready = nullptr)
    {
        try {
            _handler = std::make_unique<AMQP::LibEventHandler>(_base.get());
            AMQP::Address address(_config.host, _config.port,
                                  AMQP::Login(_config.user, _config.password), _config.vhost);
            _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
            _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());
            _channel->onError([this](const char* error) {
                _connected.store(false, std::memory_order_release);
                if (_error_callback) _error_callback(error ? error : "MQ_CHANNEL_ERROR");
                ScheduleReconnect();
            });
            _channel->onReady([this, ready] {
                DeclareTopology();
                _reliable = std::make_unique<AMQP::Reliable<>>(*_channel);
                _channel->recall().onReceived([this](const AMQP::Message& message, int16_t,
                                                     const std::string&) {
                    if (message.hasMessageID()) _returned.insert(message.messageID());
                });
                _connected.store(true, std::memory_order_release);
                if (ready) try { ready->set_value(); } catch (...) {}
            });
        } catch (...) {
            if (ready) try { ready->set_exception(std::current_exception()); } catch (...) {}
            ScheduleReconnect();
        }
    }

    void ScheduleReconnect()
    {
        if (_closing.load(std::memory_order_acquire) ||
            _reconnect_pending.exchange(true, std::memory_order_acq_rel)) return;
        timeval delay{1, 0};
        event_base_once(_base.get(), -1, EV_TIMEOUT, [](evutil_socket_t, short, void* context) {
            auto* producer = static_cast<MQProducer*>(context);
            producer->_reconnect_pending.store(false, std::memory_order_release);
            if (producer->_closing.load(std::memory_order_acquire)) return;
            producer->_reliable.reset();
            producer->_channel.reset();
            producer->_connection.reset();
            producer->_handler.reset();
            producer->InitializeChannel();
        }, this, &delay);
    }

    void DeclareTopology()
    {
        _channel->declareExchange(_config.exchange, AMQP::direct, AMQP::durable);
        _channel->declareExchange(_config.retry_exchange, AMQP::direct, AMQP::durable);
        _channel->declareExchange(_config.dead_letter_exchange, AMQP::direct, AMQP::durable);
        _channel->declareQueue(_config.queue_name, AMQP::durable);
        _channel->bindQueue(_config.exchange, _config.queue_name, _config.routing_key);
        for (int level = 1; level <= 3; ++level) {
            const std::string queue = _config.retry_queue_prefix + "." + std::to_string(level);
            AMQP::Table arguments;
            arguments["x-message-ttl"] = 1000 * (1 << (level - 1));
            arguments["x-dead-letter-exchange"] = _config.exchange;
            arguments["x-dead-letter-routing-key"] = _config.routing_key;
            _channel->declareQueue(queue, AMQP::durable, arguments);
            _channel->bindQueue(_config.retry_exchange, queue, queue);
        }
        _channel->declareQueue(_config.dead_letter_queue, AMQP::durable);
        _channel->bindQueue(_config.dead_letter_exchange, _config.dead_letter_queue, _config.routing_key);
    }

    MQConfig _config;
    std::unique_ptr<AMQP::LibEventHandler> _handler;
    std::unique_ptr<AMQP::TcpConnection> _connection;
    std::unique_ptr<AMQP::TcpChannel> _channel;
    std::unique_ptr<AMQP::Reliable<>> _reliable;
    std::unordered_set<std::string> _returned;
    std::atomic<bool> _connected{false};
    std::atomic<uint64_t> _legacy_sequence{0};
    std::atomic<bool> _close_started{false};
    std::atomic<bool> _reconnect_pending{false};
    ErrorCallback _error_callback;
    std::thread _event_thread;
};

class MQConsumer : private EventThreadCommands
{
public:
    MQConsumer() = default;
    ~MQConsumer() { Close(); }

    void Init(const MQConfig& config)
    {
        _config = config;
        if (!InitEventThread()) throw std::runtime_error("failed to initialize MQ consumer event loop");
    }

    void SetMessageHandler(MessageCallback callback) { _callback = std::move(callback); }

    void StartConsuming()
    {
        InitializeChannel();
        event_base_dispatch(_base.get());
    }

    void Ack(DeliveryContext delivery)
    {
        Post([this, delivery] {
            if (IsCurrent(delivery)) _channel->ack(delivery.delivery_tag);
        });
    }

    void Nack(DeliveryContext delivery, bool requeue = true)
    {
        Post([this, delivery, requeue] {
            if (IsCurrent(delivery))
                _channel->reject(delivery.delivery_tag, requeue ? AMQP::requeue : 0);
        });
    }

    void Retry(DeliveryContext delivery, std::string body, uint32_t retry_after_ms,
               uint32_t retry_count,
                ConfirmCallback callback = {})
    {
        Post([this, delivery, body = std::move(body), retry_after_ms, retry_count,
               callback = std::move(callback)] {
            if (!_reliable || !IsCurrent(delivery)) {
                if (callback) callback(false, "NOT_CONNECTED");
                return;
            }
            const std::string routing_key = _config.retry_queue_prefix + ".delayed";
            const std::string message_id = "retry-" + std::to_string(delivery.channel_generation) +
                                            "-" + std::to_string(delivery.delivery_tag) + "-" +
                                            std::to_string(retry_count);
            AMQP::Envelope envelope(body.data(), body.size());
            envelope.setDeliveryMode(2);
            envelope.setMessageID(message_id);
            envelope.setExpiration(std::to_string(std::clamp(retry_after_ms, 1u, 86400000u)));
            AMQP::Table headers;
            headers["oj-retry-count"] = retry_count;
            envelope.setHeaders(headers);
            _reliable->publish(_config.retry_exchange, routing_key, envelope, AMQP::mandatory)
                .onAck([this, delivery, message_id, callback] {
                    const bool routed = _returned.erase(message_id) == 0;
                    if (routed && IsCurrent(delivery)) _channel->ack(delivery.delivery_tag);
                    else if (!routed) RecoverForwardFailure(delivery);
                    if (callback) callback(routed, routed ? "" : "UNROUTABLE");
                })
                .onNack([this, delivery, callback] {
                    RecoverForwardFailure(delivery);
                    if (callback) callback(false, "NACK");
                })
                .onError([this, delivery, callback](const char* error) {
                    RecoverForwardFailure(delivery);
                    if (callback) callback(false, error ? error : "PUBLISH_ERROR");
                });
        });
    }

    void DeadLetter(DeliveryContext delivery, std::string body, uint32_t retry_count,
                     ConfirmCallback callback = {})
    {
        Post([this, delivery, body = std::move(body), retry_count,
               callback = std::move(callback)] {
            if (!_reliable || !IsCurrent(delivery)) {
                if (callback) callback(false, "NOT_CONNECTED");
                return;
            }
            const std::string message_id = "dead-" + std::to_string(delivery.channel_generation) +
                                            "-" + std::to_string(delivery.delivery_tag) + "-" +
                                            std::to_string(retry_count);
            AMQP::Envelope envelope(body.data(), body.size());
            envelope.setDeliveryMode(2);
            envelope.setMessageID(message_id);
            AMQP::Table headers;
            headers["oj-retry-count"] = retry_count;
            envelope.setHeaders(headers);
            _reliable->publish(_config.dead_letter_exchange, _config.routing_key,
                               envelope, AMQP::mandatory)
                .onAck([this, delivery, message_id, callback] {
                    const bool routed = _returned.erase(message_id) == 0;
                    if (routed && IsCurrent(delivery)) _channel->ack(delivery.delivery_tag);
                    else if (!routed) RecoverForwardFailure(delivery);
                    if (callback) callback(routed, routed ? "" : "UNROUTABLE");
                })
                .onNack([this, delivery, callback] {
                    RecoverForwardFailure(delivery);
                    if (callback) callback(false, "NACK");
                })
                .onError([this, delivery, callback](const char* error) {
                    RecoverForwardFailure(delivery);
                    if (callback) callback(false, error ? error : "PUBLISH_ERROR");
                });
        });
    }

    void Close()
    {
        if (!_base || _close_started.exchange(true, std::memory_order_acq_rel)) return;
        if (!Post([this] {
                if (_connection) _connection->close();
                _closing.store(true, std::memory_order_release);
                event_base_loopbreak(_base.get());
            })) {
            event_base_loopbreak(_base.get());
        }
        _connected.store(false, std::memory_order_release);
    }

    bool IsConnected() const { return _connected.load(std::memory_order_acquire); }
    bool WaitUntilConnected(std::chrono::milliseconds timeout) const
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (IsConnected()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return IsConnected();
    }
    void SetErrorHandler(ErrorCallback callback) { _error_callback = std::move(callback); }

private:
    void InitializeChannel()
    {
        const uint64_t generation = _generation.fetch_add(1, std::memory_order_acq_rel) + 1;
        _handler = std::make_unique<AMQP::LibEventHandler>(_base.get());
        AMQP::Address address(_config.host, _config.port,
                              AMQP::Login(_config.user, _config.password), _config.vhost);
        _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), address);
        _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());
        _reliable = std::make_unique<AMQP::Reliable<>>(*_channel);
        _channel->onError([this](const char* error) {
            _connected.store(false, std::memory_order_release);
            if (_error_callback) _error_callback(error ? error : "MQ_CHANNEL_ERROR");
            ScheduleReconnect();
        });
        _channel->onReady([this, generation] {
            if (_generation.load(std::memory_order_acquire) == generation) DeclareAndConsume(generation);
        });
        _channel->recall().onReceived([this](const AMQP::Message& message, int16_t,
                                             const std::string&) {
            if (message.hasMessageID()) _returned.insert(message.messageID());
        });
    }

    void DeclareAndConsume(uint64_t generation)
    {
        _channel->setQos(static_cast<uint16_t>(std::max(1, _config.prefetch_count)));
        _channel->declareExchange(_config.exchange, AMQP::direct, AMQP::durable);
        _channel->declareExchange(_config.retry_exchange, AMQP::direct, AMQP::durable);
        _channel->declareExchange(_config.dead_letter_exchange, AMQP::direct, AMQP::durable);
        _channel->declareQueue(_config.queue_name, AMQP::durable);
        _channel->bindQueue(_config.exchange, _config.queue_name, _config.routing_key);
        for (int level = 1; level <= 3; ++level) {
            const std::string queue = _config.retry_queue_prefix + "." + std::to_string(level);
            AMQP::Table arguments;
            arguments["x-message-ttl"] = 1000 * (1 << (level - 1));
            arguments["x-dead-letter-exchange"] = _config.exchange;
            arguments["x-dead-letter-routing-key"] = _config.routing_key;
            _channel->declareQueue(queue, AMQP::durable, arguments);
            _channel->bindQueue(_config.retry_exchange, queue, queue);
        }
        const std::string delayed_queue = _config.retry_queue_prefix + ".delayed";
        AMQP::Table delayed_arguments;
        delayed_arguments["x-dead-letter-exchange"] = _config.exchange;
        delayed_arguments["x-dead-letter-routing-key"] = _config.routing_key;
        _channel->declareQueue(delayed_queue, AMQP::durable, delayed_arguments);
        _channel->bindQueue(_config.retry_exchange, delayed_queue, delayed_queue);
        _channel->declareQueue(_config.dead_letter_queue, AMQP::durable);
        _channel->bindQueue(_config.dead_letter_exchange, _config.dead_letter_queue, _config.routing_key);
        _channel->consume(_config.queue_name)
            .onReceived([this, generation](const AMQP::Message& message, uint64_t tag, bool redelivered) {
                uint32_t retries = 0;
                if (message.hasHeaders()) {
                    const AMQP::Field& field = message.headers()["oj-retry-count"];
                    if (field.isInteger()) retries = static_cast<uint32_t>(static_cast<uint64_t>(field));
                    const AMQP::Field& death_field = message.headers()["x-death"];
                    if (death_field.isArray()) {
                        const AMQP::Array& deaths = death_field;
                        uint64_t death_count = 0;
                        for (uint32_t index = 0; index < deaths.count(); ++index) {
                            const AMQP::Field& entry = deaths[static_cast<uint8_t>(index)];
                            if (!entry.isTable()) continue;
                            const AMQP::Table& death = entry;
                            const AMQP::Field& count = death["count"];
                            if (count.isInteger()) death_count += static_cast<uint64_t>(count);
                        }
                        retries = std::max(retries, static_cast<uint32_t>(
                            std::min<uint64_t>(death_count, std::numeric_limits<uint32_t>::max())));
                    }
                }
                if (_callback)
                    _callback(std::string(message.body(), message.bodySize()),
                              DeliveryContext{tag, generation}, redelivered, retries);
            });
        _connected.store(true, std::memory_order_release);
    }

    void ScheduleReconnect()
    {
        if (_closing.load(std::memory_order_acquire) ||
            _reconnect_pending.exchange(true, std::memory_order_acq_rel)) return;
        timeval delay{1, 0};
        event_base_once(_base.get(), -1, EV_TIMEOUT, [](evutil_socket_t, short, void* context) {
            auto* consumer = static_cast<MQConsumer*>(context);
            consumer->_reconnect_pending.store(false, std::memory_order_release);
            if (consumer->_closing.load(std::memory_order_acquire)) return;
            consumer->_reliable.reset();
            consumer->_channel.reset();
            consumer->_connection.reset();
            consumer->_handler.reset();
            consumer->InitializeChannel();
        }, this, &delay);
    }

    bool IsCurrent(DeliveryContext delivery) const
    {
        return delivery.channel_generation == _generation.load(std::memory_order_acquire) &&
               _channel && _connected.load(std::memory_order_acquire);
    }

    void RecoverForwardFailure(DeliveryContext delivery)
    {
        if (IsCurrent(delivery)) _channel->reject(delivery.delivery_tag, AMQP::requeue);
    }

    MQConfig _config;
    std::unique_ptr<AMQP::LibEventHandler> _handler;
    std::unique_ptr<AMQP::TcpConnection> _connection;
    std::unique_ptr<AMQP::TcpChannel> _channel;
    std::unique_ptr<AMQP::Reliable<>> _reliable;
    std::unordered_set<std::string> _returned;
    MessageCallback _callback;
    std::atomic<bool> _connected{false};
    std::atomic<bool> _close_started{false};
    std::atomic<bool> _reconnect_pending{false};
    std::atomic<uint64_t> _generation{0};
    ErrorCallback _error_callback;
};

class MQManager
{
public:
    static MQManager& Instance() { static MQManager manager; return manager; }
    void InitProducer(const MQConfig& config) { _producer = std::make_unique<MQProducer>(); _producer->Init(config); }
    void InitConsumer(const MQConfig& config) { _consumer = std::make_unique<MQConsumer>(); _consumer->Init(config); }
    MQProducer& GetProducer() { if (!_producer) throw std::runtime_error("MQProducer not initialized"); return *_producer; }
    MQConsumer& GetConsumer() { if (!_consumer) throw std::runtime_error("MQConsumer not initialized"); return *_consumer; }
    void Shutdown() { if (_producer) _producer->Close(); if (_consumer) _consumer->Close(); }

private:
    std::unique_ptr<MQProducer> _producer;
    std::unique_ptr<MQConsumer> _consumer;
};

} // namespace ns_mq
