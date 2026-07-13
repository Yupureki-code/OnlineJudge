#pragma once
#include "async/pending_task_manager.hpp"
#include "async/event_loop.hpp"
#include "../../comm/proto/judge_service.pb.h"
#include <brpc/controller.h>
#include <brpc/closure_guard.h>
#include <string>

namespace ns_judge {

/// DockerWorkDoneServiceImpl — brpc DockerWorkDone 接口实现
///
/// 容器内脚本执行完后 curl 通知此接口
/// 收到通知后标记任务完成并唤醒事件循环
class DockerWorkDoneServiceImpl : public oj_judge::JudgeService {
public:
    explicit DockerWorkDoneServiceImpl(ns_async::JudgeEventLoop* loop)
        : _loop(loop) {}

    /// 容器完成回调
    void DockerWorkDone(::google::protobuf::RpcController* cntl_base,
                        const oj_judge::DockerWorkDoneRequest* req,
                        oj_judge::NullRsp* resp,
                        ::google::protobuf::Closure* done) override {
        brpc::ClosureGuard done_guard(done);

        // 标记任务完成
        ns_async::PendingTaskManager::Instance().MarkDone(
            req->id(), req->status(), req->exit_code());

        // 唤醒事件循环（线程安全：brpc 线程 → 事件循环线程）
        if (_loop) {
            _loop->Wakeup();
        }
    }

    /// 提交判题任务（接收 → 入 MQ → 返回 submission_id）
    void Submit(::google::protobuf::RpcController* cntl_base,
                const oj_judge::SubmitRequest* req,
                oj_judge::SubmitResponse* resp,
                ::google::protobuf::Closure* done) override {
        brpc::ClosureGuard done_guard(done);
        // TODO: 将任务序列化为 Protobuf 二进制 → 发送到 MQ
        resp->set_success(true);
        resp->set_submission_id(req->submission_id());
    }

    /// 查询判题状态
    void GetStatus(::google::protobuf::RpcController* cntl_base,
                   const oj_judge::GetStatusRequest* req,
                   oj_judge::JudgeResult* resp,
                   ::google::protobuf::Closure* done) override {
        brpc::ClosureGuard done_guard(done);
        // TODO: 从数据库查询判题状态
        resp->set_submission_id(req->submission_id());
        resp->set_status("PENDING");
    }

private:
    ns_async::JudgeEventLoop* _loop;
};

} // namespace ns_judge