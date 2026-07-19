#include "comm/mq_client.hpp"

#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
using namespace std::chrono_literals;

void Check(bool condition, const char* message)
{
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}
}

int main()
{
    ns_mq::MQConfig config;
    if (const char* host = std::getenv("OJ_MQ_HOST")) config.host = host;
    if (const char* port = std::getenv("OJ_MQ_PORT")) config.port = std::stoi(port);
    if (const char* user = std::getenv("OJ_MQ_USER")) config.user = user;
    if (const char* password = std::getenv("OJ_MQ_PASS")) config.password = password;
    config.queue_name = "oj.phase7.integration.v2.task";
    config.exchange = "oj.phase7.integration.v2";
    config.routing_key = "oj.phase7.integration.v2.task";
    config.retry_exchange = "oj.phase7.integration.v2.retry";
    config.retry_queue_prefix = "oj.phase7.integration.v2.retry";
    config.dead_letter_exchange = "oj.phase7.integration.v2.dlx";
    config.dead_letter_queue = "oj.phase7.integration.v2.task.dlx";
    ns_mq::MQConsumer consumer;
    consumer.Init(config);
    consumer.SetErrorHandler([](const std::string& error) {
        std::cerr << "consumer error: " << error << '\n';
    });
    std::promise<bool> completed;
    auto future = completed.get_future();
    std::mutex mutex;
    std::vector<uint32_t> deliveries;
    consumer.SetMessageHandler([&](const std::string& body, ns_mq::DeliveryContext delivery,
                                   bool, uint32_t retries) {
        {
            std::lock_guard lock(mutex);
            deliveries.push_back(retries);
        }
        if (retries < 3) {
            consumer.Retry(delivery, body, 1000u << retries, retries + 1,
                [&completed](bool success, const std::string&) {
                    if (!success) try { completed.set_value(false); } catch (...) {}
                });
        } else {
            consumer.DeadLetter(delivery, body, retries,
                [&completed](bool success, const std::string&) {
                    try { completed.set_value(success); } catch (...) {}
                });
        }
    });
    std::thread consumer_thread([&] { consumer.StartConsuming(); });
    for (int attempt = 0; attempt < 100 && !consumer.IsConnected(); ++attempt)
        std::this_thread::sleep_for(50ms);
    Check(consumer.IsConnected(), "consumer connects to RabbitMQ");

    ns_mq::MQProducer producer;
    producer.Init(config);
    std::promise<bool> published;
    auto publish_future = published.get_future();
    Check(producer.PublishConfirmed("mq-delivery-integration", "phase7-delivery",
        [&published](bool success, const std::string& error) {
            if (!success) std::cerr << "initial publish error: " << error << '\n';
            published.set_value(success);
        }),
        "initial publish is accepted");
    Check(publish_future.wait_for(5s) == std::future_status::ready && publish_future.get(),
          "initial mandatory publish is confirmed");
    Check(future.wait_for(20s) == std::future_status::ready && future.get(),
          "third retry is confirmed into DLQ before original ACK");

    std::promise<bool> unroutable;
    auto unroutable_future = unroutable.get_future();
    Check(producer.PublishConfirmed("mq-unroutable", "unroutable",
        [&unroutable](bool success, const std::string& error) {
            unroutable.set_value(!success && error == "UNROUTABLE");
        }, config.exchange, "missing.route"), "unroutable publish is accepted for confirmation");
    Check(unroutable_future.wait_for(5s) == std::future_status::ready && unroutable_future.get(),
          "mandatory return is observed before confirm success");

    producer.Close();
    consumer.Close();
    consumer_thread.join();
    std::lock_guard lock(mutex);
    Check(deliveries == std::vector<uint32_t>({0, 1, 2, 3}),
          "retry headers and x-death advance monotonically");
    std::cout << "mq_delivery_integration_test: PASS\n";
}
