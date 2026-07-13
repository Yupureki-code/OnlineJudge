#include "../include/router.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace ns_gateway {

bool Router::LoadFromFile(const std::string& conf_path) {
    _conf_path = conf_path;
    _routes.clear();

    std::ifstream file(conf_path);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 跳过空行和注释
        if (line.empty() || line[0] == '#') continue;

        // 格式：path_prefix backend_service backend_method require_auth require_admin
        std::istringstream iss(line);
        RouteEntry entry;
        std::string auth_str, admin_str;
        if (!(iss >> entry.path_prefix >> entry.backend_service >> entry.backend_method
                  >> auth_str >> admin_str)) {
            continue;
        }
        entry.require_auth = (auth_str == "true" || auth_str == "1");
        entry.require_admin = (admin_str == "true" || admin_str == "1");
        _routes.push_back(std::move(entry));
    }

    // 按前缀长度降序排序，确保最长匹配优先
    std::sort(_routes.begin(), _routes.end(),
        [](const RouteEntry& a, const RouteEntry& b) {
            return a.path_prefix.size() > b.path_prefix.size();
        });

    return !_routes.empty();
}

const RouteEntry* Router::Match(const std::string& path) const {
    for (const auto& route : _routes) {
        if (path.compare(0, route.path_prefix.size(), route.path_prefix) == 0) {
            return &route;
        }
    }
    return nullptr;
}

bool Router::Reload() {
    return LoadFromFile(_conf_path);
}

} // namespace ns_gateway