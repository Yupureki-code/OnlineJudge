#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <mutex>
#include <atomic>
#include <cstring>
#include <iostream>
#include <thread>

namespace oj_sandbox {

// ==================== 数据结构 ====================

/// Docker 沙箱执行结果
struct ExecResult {
    int exit_code = -1;        // 退出码
    std::string stdout_str;    // 标准输出
    std::string stderr_str;    // 标准错误
    int64_t time_ms = 0;       // 执行耗时(毫秒)
    int64_t memory_bytes = 0;  // 内存使用(字节)
    bool timed_out = false;    // 是否超时
    bool killed = false;       // 是否被杀死
};

/// Docker 沙箱配置
struct SandboxConfig {
    std::string image = "oj-sandbox:latest";  // Docker 镜像
    int cpu_limit_ms = 2000;                   // CPU 时间限制(毫秒)
    int memory_limit_mb = 256;                 // 内存限制(MB)
    int timeout_ms = 5000;                     // 挂钟时间限制(毫秒)
    bool network_disabled = true;              // 禁止网络
    bool read_only_root = true;                // 只读根文件系统
    std::string work_dir = "/home/judge";      // 工作目录
};

// ==================== 内部辅助 ====================

namespace _detail {

/// curl 全局初始化（线程安全，只执行一次）
inline void EnsureCurlInit() {
    static std::once_flag flag;
    std::call_once(flag, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

/// libcurl 写回调
inline size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/// Base64 编码（用于 WriteFile 避免命令注入）
inline std::string Base64Encode(const std::string& input) {
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    int val = 0, valb = -6;
    for (uint8_t c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) { output.push_back(table[(val >> valb) & 0x3F]); valb -= 6; }
    }
    if (valb > -6) output.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);
    while (output.size() % 4) output.push_back('=');
    return output;
}

/// Docker 多路复用流帧（Tty=false 时 exec/start 返回的二进制流）
/// 每帧: [1字节stream_type][3字节保留][4字节payload大小(大端)][payload]
struct MuxFrame {
    uint8_t stream_type;   // 1=stdout, 2=stderr
    std::string payload;
};

inline std::vector<MuxFrame> ParseMuxStream(const std::string& data) {
    std::vector<MuxFrame> frames;
    size_t pos = 0;
    while (pos + 8 <= data.size()) {
        uint8_t type = static_cast<uint8_t>(data[pos]);
        uint32_t payload_size = 0;
        for (int i = 0; i < 4; ++i)
            payload_size = (payload_size << 8) | static_cast<uint8_t>(data[pos + 4 + i]);
        pos += 8;
        if (pos + payload_size > data.size()) break;
        frames.push_back({type, data.substr(pos, payload_size)});
        pos += payload_size;
    }
    return frames;
}

} // namespace _detail

// ==================== DockerSandbox ====================

/// Docker 沙箱：通过 Docker Engine API (Unix Socket) 管理容器
///
/// 关键接口处理流程：
///
///   Create(config) ─────────────────────────────────────────────┐
///     │  构建 JSON 请求（镜像/资源限制/安全加固）                │
///     │  POST /v1.41/containers/create                          │
///     │  解析返回的容器 ID                                       │
///     │  Start() ── POST /containers/{id}/start                 │
///     ▼                                                         │
///   WriteFile(path, content)                                    │
///     │  base64 编码内容 → Exec("echo 'xxx' | base64 -d > path")│
///     ▼                                                         │
///   Exec(command, timeout_ms)                                   │
///     │  ① POST /containers/{id}/exec  → 创建 exec 实例         │
///     │  ② POST /exec/{exec_id}/start  → 执行命令               │
///     │     返回多路复用二进制流 → 解析 stdout/stderr             │
///     │  ③ GET  /exec/{exec_id}/json   → 获取 exit_code         │
///     ▼                                                         │
///   ReadFile(path)                                              │
///     │  Exec("cat " + path) → 返回 stdout_str                  │
///     ▼                                                         │
///   Destroy() ──────────────────────────────────────────────────┘
///       POST /containers/{id}/stop?t=5
///       DELETE /containers/{id}?force=true
///
class DockerSandbox {
public:
    DockerSandbox() { _detail::EnsureCurlInit(); }
    ~DockerSandbox() { Destroy(); }

    /// 创建隔离容器
    /// @param config 沙箱配置（镜像/资源限制/安全选项）
    /// @return 容器 ID，失败返回空字符串
    ///
    /// 处理流程：
    ///   1. 构建容器配置 JSON：镜像、工作目录、网络禁用
    ///   2. 设置 HostConfig：内存限制(Memory)、CPU限制(NanoCpus)、PID限制
    ///   3. 安全加固：CapDrop=ALL（删除所有Linux能力）、no-new-privileges
    ///   4. 可写工作目录：通过 Tmpfs 挂载（根文件系统只读时需要）
    ///   5. POST /v1.41/containers/create → 返回容器 ID
    std::string Create(const SandboxConfig& config) {
        _config = config;

        Json::Value req;
        req["Image"] = config.image;
        req["WorkingDir"] = config.work_dir;
        req["NetworkDisabled"] = config.network_disabled;
        req["OpenStdin"] = false;
        req["Tty"] = false;

        // 资源限制 + 安全加固
        Json::Value host_config;
        host_config["Memory"] = config.memory_limit_mb * 1024 * 1024;
        host_config["NanoCpus"] = (int64_t)config.cpu_limit_ms * 1000000;
        host_config["PidsLimit"] = 100;
        host_config["ReadonlyRootfs"] = config.read_only_root;

        // 安全加固：删除所有 Linux Capabilities
        Json::Value cap_drop(Json::arrayValue);
        cap_drop.append("ALL");
        host_config["CapDrop"] = cap_drop;

        // 安全加固：no-new-privileges
        Json::Value security_opts(Json::arrayValue);
        security_opts.append("no-new-privileges");
        host_config["SecurityOpt"] = security_opts;

        // 临时文件系统挂载（可写工作目录）
        Json::Value tmpfs;
        tmpfs[config.work_dir] = Json::Value(Json::objectValue);
        host_config["Tmpfs"] = tmpfs;

        req["HostConfig"] = host_config;

        Json::FastWriter writer;
        std::string body = writer.write(req);

        std::string response = DockerApiCall("POST", "/v1.41/containers/create", body);
        auto id = ParseDockerResponse(response, "Id");

        if (id.has_value()) {
            _container_id = id.value();
            _created = true;
        }
        return _container_id;
    }

    /// 启动容器（Create 之后必须 Start）
    int Start() {
        if (_container_id.empty()) return -1;
        DockerApiCall("POST", "/v1.41/containers/" + _container_id + "/start", "");
        return 0;
    }

    /// 写入文件到容器
    /// @param path 容器内文件路径
    /// @param content 文件内容
    /// @return 0=成功, -1=失败
    ///
    /// 处理流程：
    ///   1. 将内容 base64 编码（避免特殊字符导致命令注入）
    ///   2. 在容器内执行: echo 'base64内容' | base64 -d > path
    int WriteFile(const std::string& path, const std::string& content) {
        if (_container_id.empty()) return -1;
        std::string encoded = _detail::Base64Encode(content);
        std::string cmd = "echo '" + encoded + "' | base64 -d > " + path;
        ExecResult result = Exec(cmd, 5000);
        return result.exit_code == 0 ? 0 : -1;
    }

    /// 读取容器内文件
    /// @param path 容器内文件路径
    /// @return 文件内容（读取失败返回空字符串）
    ///
    /// 处理流程：Exec("cat " + path) → 返回 stdout_str
    std::string ReadFile(const std::string& path) {
        if (_container_id.empty()) return "";
        ExecResult result = Exec("cat " + path, 5000);
        return result.stdout_str;
    }

    /// 在容器内执行命令（同步）
    /// @param command 要执行的 shell 命令
    /// @param timeout_ms 超时（毫秒，暂未实现异步超时）
    /// @return ExecResult（exit_code + stdout + stderr）
    ///
    /// 处理流程：
    ///   ① POST /v1.41/containers/{id}/exec
    ///      请求体: {AttachStdout:true, AttachStderr:true, Cmd:["sh","-c",command]}
    ///      返回: exec_id
    ///
    ///   ② POST /v1.41/exec/{exec_id}/start
    ///      请求体: {Detach:false, Tty:false}
    ///      返回: 多路复用二进制流
    ///      流格式: [1B stream_type][3B reserved][4B payload_size big-endian][payload...]
    ///      stream_type: 1=stdout, 2=stderr
    ///
    ///   ③ 解析多路复用流，分离 stdout 和 stderr
    ///
    ///   ④ GET /v1.41/exec/{exec_id}/json
    ///      返回: {ExitCode: 0, Running: false, ...}
    ///      提取 ExitCode
    ExecResult Exec(const std::string& command, int timeout_ms) {
        ExecResult result;
        if (_container_id.empty()) { result.exit_code = -1; return result; }

        // ① 创建 exec 实例
        Json::Value req;
        req["AttachStdout"] = true;
        req["AttachStderr"] = true;
        req["Cmd"] = Json::Value(Json::arrayValue);
        req["Cmd"].append("sh");
        req["Cmd"].append("-c");
        req["Cmd"].append(command);

        Json::FastWriter writer;
        std::string body = writer.write(req);

        std::string response = DockerApiCall("POST",
            "/v1.41/containers/" + _container_id + "/exec", body);
        auto exec_id = ParseDockerResponse(response, "Id");
        if (!exec_id.has_value()) { result.exit_code = -1; return result; }

        // ② 启动 exec，捕获多路复用输出流
        Json::Value start_req;
        start_req["Detach"] = false;
        start_req["Tty"] = false;
        std::string start_body = writer.write(start_req);

        std::string raw_output = DockerApiCall("POST",
            "/v1.41/exec/" + exec_id.value() + "/start", start_body);

        // ③ 解析多路复用流：分离 stdout/stderr
        auto frames = _detail::ParseMuxStream(raw_output);
        for (const auto& frame : frames) {
            if (frame.stream_type == 1) result.stdout_str += frame.payload;
            else if (frame.stream_type == 2) result.stderr_str += frame.payload;
        }

        // ④ 查询 exec 退出码
        std::string inspect_resp = DockerApiCall("GET",
            "/v1.41/exec/" + exec_id.value() + "/json", "");
        Json::Value inspect;
        Json::Reader reader;
        if (reader.parse(inspect_resp, inspect))
            result.exit_code = inspect.get("ExitCode", -1).asInt();

        return result;
    }

    /// 销毁容器
    ///
    /// 处理流程：
    ///   1. POST /containers/{id}/stop?t=5  — 给 5 秒优雅停止
    ///   2. DELETE /containers/{id}?force=true — 强制删除
    void Destroy() {
        if (_container_id.empty()) return;
        DockerApiCall("POST", "/v1.41/containers/" + _container_id + "/stop?t=5", "");
        DockerApiCall("DELETE", "/v1.41/containers/" + _container_id + "?force=true", "");
        _container_id.clear();
        _created = false;
    }

    bool IsRunning() const { return _created && !_container_id.empty(); }
    const std::string& GetContainerId() const { return _container_id; }

private:
    /// 通过 Unix Socket 调用 Docker Engine API
    std::string DockerApiCall(const std::string& method,
                               const std::string& path,
                               const std::string& body = "") {
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        std::string url = "http://localhost" + path;
        std::string response;

        curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/var/run/docker.sock");
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _detail::CurlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            if (!body.empty()) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        return (res == CURLE_OK) ? response : "";
    }

    /// 从 JSON 响应中提取指定 key 的字符串值
    std::optional<std::string> ParseDockerResponse(const std::string& json,
                                                     const std::string& key) {
        Json::Value root;
        Json::Reader reader;
        if (!reader.parse(json, root) || !root.isMember(key)) return std::nullopt;
        return root[key].asString();
    }

    std::string _container_id;
    SandboxConfig _config;
    bool _created = false;
};

// ==================== SandboxPool ====================

/// Docker 沙箱池：预热 N 个运行容器，减少创建开销
///
/// 设计说明：
///   - 运行容器镜像小（~100MB），预热后复用
///   - 编译容器镜像大（~2GB），每次新建销毁（不在此池管理）
///   - 容器内脚本执行完后清理临时文件，实现复用
class SandboxPool {
public:
    SandboxPool(size_t pool_size, const SandboxConfig& config)
        : _pool_size(pool_size), _config(config) {
        for (size_t i = 0; i < pool_size; ++i) {
            auto sandbox = std::make_unique<DockerSandbox>();
            std::string id = sandbox->Create(config);
            if (!id.empty()) {
                sandbox->Start();
                _idle_pool.push_back(std::move(sandbox));
            }
        }
    }

    /// 获取一个沙箱实例（从池中取出或新建）
    std::unique_ptr<DockerSandbox> Acquire() {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!_idle_pool.empty()) {
            auto sandbox = std::move(_idle_pool.back());
            _idle_pool.pop_back();
            return sandbox;
        }
        // 池空，创建新容器
        auto sandbox = std::make_unique<DockerSandbox>();
        std::string id = sandbox->Create(_config);
        if (!id.empty()) sandbox->Start();
        return sandbox;
    }

    /// 归还沙箱实例（容器失效则丢弃，不放回池中）
    void Release(std::unique_ptr<DockerSandbox> sandbox) {
        if (!sandbox || !sandbox->IsRunning()) return;
        std::lock_guard<std::mutex> lock(_mtx);
        _idle_pool.push_back(std::move(sandbox));
    }

    /// 销毁所有预热容器
    void Shutdown() {
        std::lock_guard<std::mutex> lock(_mtx);
        while (!_idle_pool.empty()) _idle_pool.pop_back();  // unique_ptr 析构 → Destroy()
    }

private:
    size_t _pool_size;
    SandboxConfig _config;
    std::vector<std::unique_ptr<DockerSandbox>> _idle_pool;
    std::mutex _mtx;
};

} // namespace oj_sandbox