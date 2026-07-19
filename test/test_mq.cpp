// test/test_mq.cpp
// RabbitMQ 客户端测试
//
// 前置条件：RabbitMQ 运行在 localhost:5672
// 启动：docker-compose up -d rabbitmq

#include "../src/comm/mq_client.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace ns_mq;

void test_producer_consumer() {
    std::cout << "[TEST] MQ Producer/Consumer...";

    MQConfig config;
    config.host = "localhost";
    config.port = 5672;
    config.user = "oj";
    config.password = "ojpassword";
    config.queue_name = "test_queue";

    // 初始化生产者
    MQProducer producer;
    producer.Init(config);

    // 初始化消费者
    MQConsumer consumer;
    consumer.Init(config);

    std::atomic<int> received_count{0};
    consumer.SetMessageHandler([&received_count](const std::string& body,
                                                   ns_mq::DeliveryContext,
                                                   bool redelivered,
                                                   uint32_t) {
        std::cout << "Received: " << body << std::endl;
        received_count.fetch_add(1);
        // ACK 消息
        // consumer.Ack(tag);
    });

    // 启动消费线程
    std::thread consumer_thread([&consumer]() {
        consumer.StartConsuming();
    });

    // 发布消息
    for (int i = 0; i < 5; ++i) {
        std::string msg = "test_message_" + std::to_string(i);
        producer.Publish(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 等待消费
    std::this_thread::sleep_for(std::chrono::seconds(2));

    consumer.Close();
    consumer_thread.join();
    producer.Close();

    assert(received_count.load() == 5);
    std::cout << " PASSED (" << received_count.load() << " messages)" << std::endl;
}

int main() {
    std::cout << "=== RabbitMQ Tests ===" << std::endl;

    try {
        test_producer_consumer();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "=== All tests passed ===" << std::endl;
    return 0;
}
