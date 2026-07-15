#pragma once
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <string>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include "../../comm/proto/judge_service.pb.h"
#include "../../comm/proto/biz_service.pb.h"
#include "../../comm/logger.hpp"

namespace oj_judge 
{
    using namespace ns_logger;
/// 判题结果回传器：通过 brpc RPC 调用 Business Service 更新判题结果
///
/// 判题完成后，Judge Service 需要将结果通知 Business Service
/// Business Service 更新 submissions 表的状态
class ResultReporter 
{
public:
    /// 初始化
    /// @param business_service_addr Business Service 地址（如 "127.0.0.1:8081"）
    void Init(const std::string& business_service_addr) {
        _business_addr = business_service_addr;

        brpc::ChannelOptions options;
        options.timeout_ms = 5000;
        options.max_retry = 3;

        _channel = std::make_unique<brpc::Channel>();
        if (_channel->Init(business_service_addr.c_str(), "", &options) != 0) {
            throw std::runtime_error("Failed to init channel to " + business_service_addr);
        }

        // 创建 SubmissionService Stub
        _stub = std::make_unique<oj::biz::SubmissionService::Stub>(_channel.get());
    }

    /// 回传判题结果
    /// @param request 判题完成请求（包含 submission_id, status_list 等）
    /// @return true=回传成功
    bool ReportResult(const JudgeFinishedRequest& request) 
    {
        if (!_stub) return false;
        std::string out;
        request.SerializeToString(&out);
        LOG_DEBUG("JudgeFinishedRequest : {}",out);
        oj_judge::NullRsp resp;
        brpc::Controller cntl;
        cntl.set_timeout_ms(5000);

        // 调用 Business Service 的 UpdateJudgeResult RPC
        _stub->UpdateJudgeResult(&cntl, &request, &resp, nullptr);

        if (cntl.Failed()) {
            LOG(ERROR) << "ReportResult failed: " << cntl.ErrorText();
            return false;
        }

        _report_count.fetch_add(1);
        return true;
    }

    /// 带重试的回传
    /// @param request 判题完成请求
    /// @param max_retries 最大重试次数
    /// @return true=回传成功
    bool ReportResultWithRetry(const JudgeFinishedRequest& request, int max_retries = 3) {
        for (int i = 0; i < max_retries; ++i) {
            if (ReportResult(request)) return true;
            // 指数退避：100ms, 200ms, 400ms
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << i)));
        }
        LOG(ERROR) << "ReportResult failed after " << max_retries << " retries"
                   << ", submission_id=" << request.submission_id();
        return false;
    }

    /// 获取回传统计
    uint64_t GetReportCount() const { return _report_count.load(); }

private:
    std::string _business_addr;
    std::unique_ptr<brpc::Channel> _channel;
    std::unique_ptr<oj::biz::SubmissionService::Stub> _stub;
    std::atomic<uint64_t> _report_count{0};
};

} // namespace oj_judge