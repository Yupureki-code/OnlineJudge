#pragma once
#include <amqpcpp.h>
#include <amqpcpp/libevent.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <iostream>
#include <stdexcept>
#include <chrono>

namespace ns_mq {

/// RabbitMQ 配置
struct MQConfig 
{
    std::string host = "localhost";
    uint16_t port = 5672;
    std::string user = "oj";
    std::string password = "ojpassword";
    std::string vhost = "/";
    std::string queue_name = "oj.judge.task";
    std::string exchange = "";
    std::string routing_key = "oj.judge.task";
    std::string dead_letter_exchange = "oj.judge.dlx";
    std::string dead_letter_queue = "oj.judge.task.dlx";
    int prefetch_count = 1;
    int message_ttl_ms = 60000;
};

using MessageCallback = std::function<void(const std::string& body,
                                            uint64_t delivery_tag,
                                            bool redelivered)>;

// ==================== MQProducer ====================

class MQProducer {
public:
    MQProducer() = default;
    ~MQProducer() { Close(); }

    void Init(const MQConfig& config) 
    {
        _config = config;
        evthread_use_pthreads();
        auto* base = event_base_new();
        _base = {base, event_base_free};
        _handler = std::make_unique<AMQP::LibEventHandler>(base);

        AMQP::Address addr(config.host, config.port,
                           AMQP::Login(config.user, config.password), config.vhost);
        _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), addr);
        _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

        _channel->onReady([this]() {
            AMQP::Table args;
            args["x-message-ttl"] = std::to_string(_config.message_ttl_ms);
            args["x-dead-letter-exchange"] = _config.dead_letter_exchange;
            _channel->declareQueue(_config.queue_name, AMQP::durable, args);
            _channel->declareExchange(_config.dead_letter_exchange, AMQP::direct, AMQP::durable);
            _channel->declareQueue(_config.dead_letter_queue, AMQP::durable);
            _channel->bindQueue(_config.dead_letter_exchange, _config.dead_letter_queue, _config.routing_key);
            _connected.store(true);
        });

        _channel->onError([this](const char* msg) {
            std::cerr << "[MQProducer] Error: " << msg << std::endl;
            _connected.store(false);
        });

        _event_thread = std::thread([this]() { RunEventLoop(); });
        for (int i = 0; i < 30 && !_connected.load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    bool Publish(const std::string& body) {
        if (!_connected.load() || !_channel) return false;
        std::lock_guard<std::mutex> lock(_mtx);
        AMQP::Envelope envelope(body.c_str(), body.size());
        envelope.setDeliveryMode(2);
        envelope.setExpiration(std::to_string(_config.message_ttl_ms));
        return _channel->publish(_config.exchange, _config.routing_key, envelope);
    }

    void Close() {
        _connected.store(false);
        if (_connection) _connection->close();
        if (_base) event_base_loopbreak(_base.get());
        if (_event_thread.joinable()) _event_thread.join();
    }

    bool IsConnected() const { return _connected.load(); }

private:
    void RunEventLoop() { if (_base) event_base_dispatch(_base.get()); }

    MQConfig _config;
    std::unique_ptr<event_base, void(*)(event_base*)> _base{nullptr, event_base_free};
    std::unique_ptr<AMQP::LibEventHandler> _handler;
    std::unique_ptr<AMQP::TcpConnection> _connection;
    std::unique_ptr<AMQP::TcpChannel> _channel;
    std::mutex _mtx;
    std::atomic<bool> _connected{false};
    std::thread _event_thread;
};

// ==================== MQConsumer ====================

class MQConsumer {
public:
    MQConsumer() = default;
    ~MQConsumer() { Close(); }

    void Init(const MQConfig& config) {
        _config = config;
        evthread_use_pthreads();
        auto* base = event_base_new();
        _base = {base, event_base_free};
        _handler = std::make_unique<AMQP::LibEventHandler>(base);

        AMQP::Address addr(config.host, config.port,
                           AMQP::Login(config.user, config.password), config.vhost);
        _connection = std::make_unique<AMQP::TcpConnection>(_handler.get(), addr);
        _channel = std::make_unique<AMQP::TcpChannel>(_connection.get());

        _channel->onReady([this]() { DeclareQueues(); });
        _channel->onError([this](const char* msg) {
            std::cerr << "[MQConsumer] Error: " << msg << std::endl;
            _connected.store(false);
        });
    }

    void SetMessageHandler(MessageCallback callback) { _callback = std::move(callback); }

    void StartConsuming() {
        if (_base && _connected.load()) {
            std::cout << "[MQConsumer] Starting event loop" << std::endl;
            event_base_dispatch(_base.get());
        }
    }

    void Ack(uint64_t delivery_tag) {
        if (_channel && _connected.load()) _channel->ack(delivery_tag);
    }

    void Nack(uint64_t delivery_tag, bool requeue = true) {
        if (_channel && _connected.load() && requeue)
            _channel->reject(delivery_tag, AMQP::requeue);
    }

    void Close() {
        _running.store(false);
        _connected.store(false);
        if (_connection) _connection->close();
        if (_base) event_base_loopbreak(_base.get());
    }

    bool IsConnected() const { return _connected.load(); }

private:
    void DeclareQueues() {
        _channel->setQos(_config.prefetch_count);

        AMQP::Table args;
        args["x-message-ttl"] = std::to_string(_config.message_ttl_ms);
        args["x-dead-letter-exchange"] = _config.dead_letter_exchange;
        _channel->declareQueue(_config.queue_name, AMQP::durable, args);
        _channel->declareExchange(_config.dead_letter_exchange, AMQP::direct, AMQP::durable);
        _channel->declareQueue(_config.dead_letter_queue, AMQP::durable);
        _channel->bindQueue(_config.dead_letter_exchange, _config.dead_letter_queue, _config.routing_key);

        _channel->consume(_config.queue_name)
            .onReceived([this](const AMQP::Message& msg, uint64_t tag, bool redelivered) {
                if (_callback) _callback(std::string(msg.body(), msg.bodySize()), tag, redelivered);
            })
            .onCancelled([this](const std::string& name) {
                std::cerr << "[MQConsumer] Consumer cancelled: " << name << std::endl;
            });
        _connected.store(true);
        _running.store(true);
    }

    MQConfig _config;
    std::unique_ptr<event_base, void(*)(event_base*)> _base{nullptr, event_base_free};
    std::unique_ptr<AMQP::LibEventHandler> _handler;
    std::unique_ptr<AMQP::TcpConnection> _connection;
    std::unique_ptr<AMQP::TcpChannel> _channel;
    MessageCallback _callback;
    std::atomic<bool> _running{false};
    std::atomic<bool> _connected{false};
};

// ==================== MQManager ====================

class MQManager {
public:
    static MQManager& Instance() { static MQManager m; return m; }

    void InitProducer(const MQConfig& config) {
        _producer = std::make_unique<MQProducer>();
        _producer->Init(config);
    }
    void InitConsumer(const MQConfig& config) {
        _consumer = std::make_unique<MQConsumer>();
        _consumer->Init(config);
    }

    MQProducer& GetProducer() {
        if (!_producer) throw std::runtime_error("MQProducer not initialized");
        return *_producer;
    }
    MQConsumer& GetConsumer() {
        if (!_consumer) throw std::runtime_error("MQConsumer not initialized");
        return *_consumer;
    }

    void Shutdown() {
        if (_producer) _producer->Close();
        if (_consumer) _consumer->Close();
    }

private:
    MQManager() = default;
    ~MQManager() = default;
    std::unique_ptr<MQProducer> _producer;
    std::unique_ptr<MQConsumer> _consumer;
};

} // namespace ns_mq