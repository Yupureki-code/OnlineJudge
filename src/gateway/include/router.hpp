#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace ns_gateway {

/// 路由条目
struct RouteEntry {
    std::string path_prefix;       // 如 /api/user
    std::string backend_service;   // business / judge
    std::string backend_method;    // RPC 方法名
    bool require_auth = false;     // 是否需要认证
    bool require_admin = false;    // 是否需要管理员权限
};

/// 路由表：从配置文件加载，支持热加载
class Router {
public:
    /// 从配置文件加载路由表
    /// @param conf_path 配置文件路径
    bool LoadFromFile(const std::string& conf_path);

    /// 根据请求路径查找路由
    /// @param path 请求路径
    /// @return 匹配的路由条目，未找到返回 nullptr
    const RouteEntry* Match(const std::string& path) const;

    /// 重新加载配置（热更新）
    bool Reload();

    /// 获取所有路由（调试用）
    std::vector<RouteEntry>& GetRoutes() { return _routes; }
    const std::vector<RouteEntry>& GetRoutes() const { return _routes; }

private:
    std::vector<RouteEntry> _routes;
    std::string _conf_path;
};

} // namespace ns_gateway