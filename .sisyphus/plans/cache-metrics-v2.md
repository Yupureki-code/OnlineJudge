# 缓存记录改造开发文档

## TL;DR

> **目标**: 替换旧的 list/detail/html 指标为按业务类型分类的聚合缓存指标，持久化到 MySQL，admin 实时查看
> **改动量**: model_base.hpp + oj_model.hpp + oj_admin.cpp，约 150 行

---

## 1. 新 CacheMetrics 结构体

替换旧的 `CacheMetrics`：

```cpp
struct CacheMetrics {
    // 按业务类型分别统计
    struct ActionMetrics {
        std::atomic<long long> total_requests{0};
        std::atomic<long long> cache_hits{0};
        std::atomic<long long> db_fallbacks{0};
        std::atomic<long long> total_ms{0};
    };
    
    std::array<ActionMetrics, 5> actions; // 0=question, 1=auth, 2=comment, 3=solution, 4=user
    
    ActionMetrics& Get(RecordActionType t) { return actions[static_cast<int>(t)]; }
};
```

### RecordActionType 枚举

```cpp
enum class RecordActionType : int {
    Question = 0,
    Auth = 1,
    Comment = 2,
    Solution = 3,
    User = 4,
};
```

## 2. MySQL 持久化表

一张聚合表，按 action_type 归并：

```sql
CREATE TABLE IF NOT EXISTS cache_metrics_snapshots (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    action_type TINYINT UNSIGNED NOT NULL COMMENT '业务类型: 0=question,1=auth,2=comment,3=solution,4=user',
    total_requests BIGINT UNSIGNED NOT NULL DEFAULT 0,
    cache_hits BIGINT UNSIGNED NOT NULL DEFAULT 0,
    db_fallbacks BIGINT UNSIGNED NOT NULL DEFAULT 0,
    total_ms BIGINT UNSIGNED NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_action_time (action_type, created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='缓存指标聚合快照';
```

### 数据清理

```sql
DELETE FROM cache_metrics_snapshots WHERE created_at < DATE_SUB(NOW(), INTERVAL 30 DAY);
```

## 3. 记录规则

| 场景 | 记录逻辑 |
|------|---------|
| Redis 命中 | `cache_hits++`，不记 `db_fallbacks` |
| Redis 未命中 → MySQL | `cache_hits` 不变，`db_fallbacks++` |
| 直接 MySQL（无缓存） | `cache_hits` 不变，`db_fallbacks++` |

即：**只要没命中 Redis，就算 db_fallback**。

每条操作都记录：`total_requests++`，`total_ms += cost_ms`。

## 4. 刷新策略

oj_server 后台线程每 **30 秒** 检查一次：
- 如果某 `action_type` 的 `total_requests >= 100`，将数据 INSERT 到 MySQL，计数器归零
- 如果 30 秒到了但 `total_requests < 100`，也强制 INSERT，计数器归零

### 刷新线程伪代码

```cpp
void FlushWorker() {
    while (_running) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        for (int t = 0; t < 5; ++t) {
            auto& m = Metrics().actions[t];
            long long req = m.total_requests.exchange(0);
            if (req == 0) continue;
            long long hits = m.cache_hits.exchange(0);
            long long falls = m.db_fallbacks.exchange(0);
            long long ms = m.total_ms.exchange(0);
            InsertSnapshot(static_cast<RecordActionType>(t), req, hits, falls, ms);
        }
    }
}
```

## 5. oj_admin 查询

新增 `GET /api/admin/cache/metrics` 路由：

```cpp
svr.Get("/api/admin/cache/metrics", [model](...) {
    // SELECT action_type, SUM(total_requests), SUM(cache_hits), SUM(db_fallbacks), SUM(total_ms)
    // FROM cache_metrics_snapshots
    // WHERE created_at >= DATE_SUB(NOW(), INTERVAL 1 DAY)
    // GROUP BY action_type
});
```

返回格式：
```json
{
    "success": true,
    "metrics": [
        {"action_type": 0, "name": "question", "total_requests": 1234, "cache_hits": 800, "db_fallbacks": 434, "total_ms": 56000, "hit_rate": 64.8},
        ...
    ]
}
```

---

## 6. 旧代码清理

- 删除旧的 `CacheMetrics` 结构体（list/detail/html 3 类指标）
- 删除旧的 `RecordListMetrics` / `RecordDetailMetrics` / `RecordHtml*Metrics` 方法
- 替换为统一接口：`RecordCacheMetrics(RecordActionType type, bool cache_hit, bool db_fallback, long long cost_ms)`
- 更新所有调用点（model_comment, model_solution, model_user, model_question, control_base, control_question）

## 7. 实施顺序

| 阶段 | 内容 |
|------|------|
| Phase 1 | 新 `CacheMetrics` 结构体 + `RecordCacheMetrics` 统一接口 |
| Phase 2 | SQL 建表 + `InsertSnapshot` + 后台刷线程 |
| Phase 3 | 全局替换旧的 `Record*Metrics` 调用 |
| Phase 4 | oj_admin API + 前端展示 |
| Phase 5 | 删除旧指标代码 |
