#include "rpc/admin_audit_worker.hpp"

#include <chrono>
#include <utility>

#include "comm.hpp"
#include "model/oj_model.hpp"

namespace oj::rpc
{

AdminAuditWorker::AdminAuditWorker(oj::model::Model& model, std::size_t capacity)
    : model_(model), capacity_(capacity)
{
}

AdminAuditWorker::~AdminAuditWorker()
{
    Stop();
}

bool AdminAuditWorker::Start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return true;
    try {
        thread_ = std::thread(&AdminAuditWorker::Run, this);
    } catch (...) {
        running_.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

void AdminAuditWorker::Stop() noexcept
{
    running_.store(false, std::memory_order_release);
    condition_.notify_all();
    if (thread_.joinable()) thread_.join();
}

bool AdminAuditWorker::Enqueue(std::string request_id, const AdminAccount& admin,
                               std::string action, std::string resource_type,
                               std::string result, std::string payload_text)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_.load(std::memory_order_acquire) || queue_.size() >= capacity_)
        return false;
    queue_.push_back({std::move(request_id), admin.admin_id, admin.uid, admin.role,
                      std::move(action), std::move(resource_type), std::move(result),
                      std::move(payload_text)});
    condition_.notify_one();
    return true;
}

void AdminAuditWorker::Run()
{
    for (;;) {
        Item item;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return !running_.load(std::memory_order_acquire) || !queue_.empty();
            });
            if (queue_.empty()) {
                if (!running_.load(std::memory_order_acquire)) return;
                continue;
            }
            item = std::move(queue_.front());
            queue_.pop_front();
        }

        AdminAuditLog log;
        log.request_id = std::move(item.request_id);
        log.operator_admin_id = item.admin_id;
        log.operator_uid = item.uid;
        log.operator_role = std::move(item.role);
        log.action = std::move(item.action);
        log.resource_type = std::move(item.resource_type);
        log.result = std::move(item.result);
        log.payload_text = std::move(item.payload_text);
        bool persisted = false;
        for (int attempt = 0; attempt < 3 && !persisted; ++attempt) {
            persisted = model_.InsertAdminAuditLog(log);
            if (!persisted && attempt < 2)
                std::this_thread::sleep_for(std::chrono::milliseconds(50 * (attempt + 1)));
        }
        if (!persisted) {
            LOG_WARNING("admin audit persistence failed, request_id={}", log.request_id);
        } else {
            LOG_INFO("admin audit action={} type={} admin={} result={}",
                     log.action, log.resource_type, log.operator_admin_id, log.result);
        }
    }
}

} // namespace oj::rpc
