#pragma once
#include <brpc/channel.h>
#include <brpc/controller.h>
#include <string>
#include <memory>
#include <atomic>

namespace oj_judge 
{

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
    }

    /// 回传判题结果
    /// @param submission_id 提交 ID
    /// @param user_id 用户 ID
    /// @param question_id 题目 ID
    /// @param status 判题状态（AC/WA/TLE/MLE/CE/RTE）
    /// @param result_json 完整结果 JSON
    /// @param is_custom_test 是否自定义测试
    /// @return true=回传成功
    bool ReportResult(uint32_t submission_id,
                      uint32_t user_id,
                      const std::string& question_id,
                      const std::string& status,
                      const std::string& result_json,
                      bool is_custom_test) {
        // TODO: 使用生成的 Protobuf stub 调用 Business Service
        // biz::SubmissionService::Stub stub(_channel.get());
        // biz::UpdateSubmissionResultRequest req;
        // req.set_user_id(user_id);
        // req.set_question_id(question_id);
        // req.set_result_json(result_json);
        // req.set_is_pass(status == "AC");
        // req.set_action_time(time(nullptr));
        // biz::UpdateSubmissionResultResponse resp;
        // brpc::Controller cntl;
        // stub.UpdateSubmissionResult(&cntl, &req, &resp, nullptr);
        // return !cntl.Failed();

        // 当前骨架实现：仅日志
        _report_count.fetch_add(1);
        return true;
    }

    /// 获取回传统计
    uint64_t GetReportCount() const { return _report_count.load(); }

private:
    std::string _business_addr;
    std::unique_ptr<brpc::Channel> _channel;
    std::atomic<uint64_t> _report_count{0};
};

} // namespace ns_judge