#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include "../../comm/models/admin.hxx"
#include "../../comm/logger.hpp"

namespace oj::model { class Model; }

namespace oj::rpc
{
    using namespace oj::db;
    using namespace oj::logger;
    class AdminAuditWorker
    {
    public:
        explicit AdminAuditWorker(oj::model::Model& model, std::size_t capacity = 10000);
        ~AdminAuditWorker();

        AdminAuditWorker(const AdminAuditWorker&) = delete;
        AdminAuditWorker& operator=(const AdminAuditWorker&) = delete;

        bool Start();
        void Stop() noexcept;
        bool Enqueue(std::string request_id, const AdminAccount& admin,
                    std::string action, std::string resource_type,
                    std::string result, std::string payload_text);

    private:
        struct Item
        {
            std::string request_id;
            uint32_t admin_id = 0;
            uint32_t uid = 0;
            std::string role;
            std::string action;
            std::string resource_type;
            std::string result;
            std::string payload_text;
        };

        void Run();

        oj::model::Model& model_;
        const std::size_t capacity_;
        std::mutex mutex_;
        std::condition_variable condition_;
        std::deque<Item> queue_;
        std::atomic<bool> running_{false};
        std::thread thread_;
    };

} // namespace oj::rpc
