// test/test_mq_consumer.cpp
// Judge Service MQ Consumer 单元测试
//
// 前置条件：
// 1. RabbitMQ 运行在 localhost:5672
// 2. 用户名/密码：oj/ojpassword
// 启动：docker-compose up -d rabbitmq

#include "../src/comm/mq_client.hpp"
#include "../src/judge_service/include/result_reporter.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <jsoncpp/json/json.h>

using namespace ns_mq;

/// 测试 1：生产者发布消息
void test_producer_publish() {
    std::cout << "[TEST] Producer publish...";

    MQConfig config;
    config.host = "localhost";
    config.port = 5672;
    config.user = "oj";
    config.password = "ojpassword";
    config.queue_name = "test_judge_queue";

    MQProducer producer;
    producer.Init(config);

    // 发布测试消息
    Json::Value msg;
    msg["submission_id"] = 1;
    msg["question_id"] = "T1";
    msg["user_id"] = 1;
    msg["code"] = "#include <iostream>\nint main() { return 0; }";
    msg["language"] = "cpp";

    bool ok = producer.Publish(msg.toStyledString());
    assert(ok);

    producer.Close();

    std::cout << " PASSED" << std::endl;
}

/// 测试 2：消费者接收消息
void test_consumer_receive() {
    std::cout << "[TEST] Consumer receive...";

    MQConfig config;
    config.host = "localhost";
    config.port = 5672;
    config.user = "oj";
    config.password = "ojpassword";
    config.queue_name = "test_judge_queue";

    // 先发布消息
    MQProducer producer;
    producer.Init(config);
    producer.Publish("test_message_body");
    producer.Close();

    // 消费消息
    MQConsumer consumer;
    consumer.Init(config);

    std::atomic<bool> received{false};
    std::string received_body;

    consumer.SetMessageHandler([&](const std::string& body,
                                     ns_mq::DeliveryContext delivery,
                                     bool redelivered,
                                     uint32_t) {
        received.store(true);
        received_body = body;
        consumer.Ack(delivery);
    });

    // 启动消费线程
    std::thread consumer_thread([&consumer]() {
        consumer.StartConsuming();
    });

    // 等待消息
    std::this_thread::sleep_for(std::chrono::seconds(2));

    assert(received.load());
    assert(received_body == "test_message_body");

    consumer.Close();
    consumer_thread.join();

    std::cout << " PASSED" << std::endl;
}

/// 测试 3：结果回传器初始化
void test_result_reporter_init() {
    std::cout << "[TEST] Result reporter init...";

    oj::judge::ResultReporter reporter;
    try {
        reporter.Init("127.0.0.1:8081");
        // 初始化成功（即使 Business Service 未运行）
        std::cout << " PASSED" << std::endl;
    } catch (const std::exception& e) {
        // Business Service 未运行时可能失败，这是预期的
        std::cout << " SKIPPED (Business Service not running)" << std::endl;
    }
}

/// 测试 4：完整判题消息流程
void test_judge_message_flow() {
    std::cout << "[TEST] Judge message flow...";

    MQConfig config;
    config.host = "localhost";
    config.port = 5672;
    config.user = "oj";
    config.password = "ojpassword";
    config.queue_name = "test_judge_flow";

    // 发布判题任务
    MQProducer producer;
    producer.Init(config);

    Json::Value task;
    task["submission_id"] = 42;
    task["question_id"] = "T1";
    task["user_id"] = 1;
    task["code"] = "#include <iostream>\nint main() { std::cout << 42; return 0; }";
    task["language"] = "cpp";
    task["is_custom_test"] = false;
    task["timestamp"] = (Json::Int64)time(nullptr);

    producer.Publish(task.toStyledString());
    producer.Close();

    // 消费并验证
    MQConsumer consumer;
    consumer.Init(config);

    std::atomic<bool> received{false};
    std::atomic<uint32_t> submission_id{0};

    consumer.SetMessageHandler([&](const std::string& body,
                                     ns_mq::DeliveryContext delivery,
                                     bool redelivered,
                                     uint32_t) {
        Json::Value msg;
        Json::Reader reader;
        reader.parse(body, msg);
        submission_id.store(msg["submission_id"].asUInt());
        received.store(true);
        consumer.Ack(delivery);
    });

    std::thread t([&consumer]() { consumer.StartConsuming(); });
    std::this_thread::sleep_for(std::chrono::seconds(2));

    assert(received.load());
    assert(submission_id.load() == 42);

    consumer.Close();
    t.join();

    std::cout << " PASSED" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "=== MQ Consumer Tests ===" << std::endl;

    try {
        test_producer_publish();
        test_consumer_receive();
        test_result_reporter_init();
        test_judge_message_flow();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "=== All tests passed ===" << std::endl;
    return 0;
}
