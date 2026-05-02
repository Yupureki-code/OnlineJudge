
# 延迟记录计划文档

# 1.介绍

oj_server处理每一条用户发来的请求，部分请求处理延迟可能较大，而大部分延迟都来自于对数据库(MySQL,Redis)的增删查改，因此需要记录每次处理数据库的操作时，具体的信息:什么操作，是否命中Redis，总体延迟，等

# 2.现有缓存指标调整

- **现状**:由于业务的不断扩大，现有的缓存指标已经不不够实用了:
struct CacheMetrics
{
    std::atomic<long long> list_requests{0};
    std::atomic<long long> list_hits{0};
    std::atomic<long long> list_misses{0};
    std::atomic<long long> list_db_fallbacks{0};
    std::atomic<long long> list_total_ms{0};
    std::atomic<long long> detail_requests{0};
    std::atomic<long long> detail_hits{0};
    std::atomic<long long> detail_misses{0};
    std::atomic<long long> detail_db_fallbacks{0};
    std::atomic<long long> detail_total_ms{0};
    std::atomic<long long> html_static_requests{0};
    std::atomic<long long> html_static_hits{0};
    std::atomic<long long> html_static_misses{0};
    std::atomic<long long> html_list_requests{0};
    std::atomic<long long> html_list_hits{0};
    std::atomic<long long> html_list_misses{0};
    std::atomic<long long> html_detail_requests{0};
    std::atomic<long long> html_detail_hits{0};
    std::atomic<long long> html_detail_misses{0};
};

- **要求**:缓存指标结构体更改方向:
1.action_type(按照业务类型划分缓存种类，如题目(question),认证(auth),评论(comment),题解(solution),用户(user))
2.不同业务的总请求数(total_hits)总体延迟total_ms,命中缓存的总次数(cache_hits),没命中回源MySQL的次数(db_fallbacks)。由于有很多种业务类型，因此需要处理多份
3.具体的请求:包含业务类型，是否命中缓存，单条延迟，请求Redis缓存的Key,MySQL操作的命令。(访问Redis时就要记录Key，操作MySQL时就要记录命令)

# 3.缓存指标的持续性保存

- **现状**:如果只保存在进程内，则容易丢失。因此需要保存到MySQL种，每1个月进行重置清理
- **设计**:一切商量好后，请你规划MySQL表的构造(可能有几张表)

# 4.缓存记录

- **要求**:你需要在每条设计到数据库的处理(Redis,MySQL)中，进行记录。每次数据库的处理可能分两种情况:1.先访问Redis，若未命中，则回源MySQL2.不访问Redis，直接访问MySQL。对于这两种情况，统一按照一个规则记录:只要没命中Redis，就记为回源MySQL。随后把相应的信息记录到进程内部

# 5.缓存保存

- **要求**:oj_server需要把缓存相关记录保存到MySQL中，但是若频繁地访问MySQL，会导致性能的下降。因此需要考虑保存的时机:例如每隔一段时间，或者每隔一些记录次数后保存

# 6.oj_admin访问缓存记录

- **要求**:为了降低oj_server的压力。oj_admin每次获取缓存记录时，直接从MySQL中获取，随后显示到网页中
