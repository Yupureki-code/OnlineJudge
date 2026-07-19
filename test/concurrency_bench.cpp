// test/concurrency_bench.cpp
// High Concurrency Benchmark — 10/100/1000 concurrent coroutine tasks
// Tests EventLoop throughput, latency, and correctness without Docker overhead

#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>

#include "../src/judge_service/include/event_loop.hpp"
#include "../src/comm/proto/judge_service.pb.h"

using namespace std::chrono;

// ── Metrics ───────────────────────────────────────────

struct BenchMetrics {
    int total;
    int completed;
    int errors;
    double total_time_ms;
    double min_latency_ms;
    double max_latency_ms;
    double avg_latency_ms;
    double p50_latency_ms;
    double p95_latency_ms;
    double p99_latency_ms;
    double throughput_tps;
};

// ── Benchmark Runner ──────────────────────────────────

BenchMetrics RunBenchmark(std::shared_ptr<oj::judge::JudgeEventLoop> loop, int concurrency) {

    std::atomic<int> completed{0};
    std::atomic<int> errors{0};
    std::vector<double> latencies(concurrency);
    std::vector<high_resolution_clock::time_point> start_times(concurrency);

    auto t0 = high_resolution_clock::now();

    // Launch N concurrent coroutine tasks
    for (int i = 0; i < concurrency; ++i) {
        start_times[i] = high_resolution_clock::now();

        // Create a simulated judge coroutine (no Docker — just co_return)
        auto task_lambda = [i, &completed, &errors, &latencies, &start_times]() -> oj::judge::CoTask {
            // Simulate judge work (in real pipeline: compile → run → judge)
            // For benchmark: just build a result
            
            auto resp = std::make_shared<oj::common::JudgeResult>();
            resp->set_status(oj::common::SUBMISSION_STATUS_ACCEPTED);
            resp->set_time_used_ms(i % 100);
            resp->set_memory_used_bytes(1024 + i % 2048);

            auto now = high_resolution_clock::now();
            double lat = duration<double, std::milli>(now - start_times[i]).count();
            latencies[i] = lat;

            std::cerr << "." << std::flush;  // progress indicator
            completed.fetch_add(1);
            co_return resp;
        };

        auto coro = task_lambda();
        loop->PushNewTask(std::move(coro));
    }

    // Wait for all tasks to complete (with timeout)
    auto deadline = high_resolution_clock::now() + seconds(30);
    while (completed.load() < concurrency && high_resolution_clock::now() < deadline) {
        std::this_thread::sleep_for(milliseconds(5));
    }
    // Give CheckTimeOut a moment to clean up
    std::this_thread::sleep_for(milliseconds(50));

    auto t1 = high_resolution_clock::now();
    double total_ms = duration<double, std::milli>(t1 - t0).count();

    // ── Compute metrics (even if not all completed) ──
    BenchMetrics m;
    m.total = concurrency;
    m.completed = std::min(completed.load(), concurrency);
    m.errors = errors.load();
    m.total_time_ms = total_ms;
    m.throughput_tps = (total_ms > 0) ? (m.completed / (total_ms / 1000.0)) : 0;

    if (m.completed > 0) {
        std::vector<double> valid_lats;
        for (int i = 0; i < concurrency; ++i)
            if (latencies[i] > 0) valid_lats.push_back(latencies[i]);

        if (!valid_lats.empty()) {
            std::sort(valid_lats.begin(), valid_lats.end());
            m.min_latency_ms = valid_lats.front();
            m.max_latency_ms = valid_lats.back();
            m.avg_latency_ms = std::accumulate(valid_lats.begin(), valid_lats.end(), 0.0) / valid_lats.size();
            
            auto percentile = [&](double p) {
                int idx = std::min((int)(p / 100.0 * valid_lats.size()), (int)valid_lats.size() - 1);
                return valid_lats[idx];
            };
            m.p50_latency_ms = percentile(50);
            m.p95_latency_ms = percentile(95);
            m.p99_latency_ms = percentile(99);
        }
    }

    return m;
}

// ── Print ─────────────────────────────────────────────

void PrintMetrics(const std::string& label, const BenchMetrics& m) {
    std::cout << "\n━━━ " << label << " ━━━" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  Total submitted:     " << m.total << std::endl;
    std::cout << "  Completed:           " << m.completed << " (" 
              << (m.total > 0 ? 100.0 * m.completed / m.total : 0) << "%)" << std::endl;
    std::cout << "  Errors:              " << m.errors << std::endl;
    std::cout << "  Total time:          " << m.total_time_ms << " ms" << std::endl;
    std::cout << "  Throughput:          " << m.throughput_tps << " tasks/sec" << std::endl;
    if (m.completed > 0) {
        std::cout << "  Latency (min):       " << m.min_latency_ms << " ms" << std::endl;
        std::cout << "  Latency (avg):       " << m.avg_latency_ms << " ms" << std::endl;
        std::cout << "  Latency (max):       " << m.max_latency_ms << " ms" << std::endl;
        std::cout << "  Latency (P50):       " << m.p50_latency_ms << " ms" << std::endl;
        std::cout << "  Latency (P95):       " << m.p95_latency_ms << " ms" << std::endl;
        std::cout << "  Latency (P99):       " << m.p99_latency_ms << " ms" << std::endl;
    }
}

// ── Main ───────────────────────────────────────────────

int main() {
    std::cout << "╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Judge Service — High Concurrency Benchmark  ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════╝" << std::endl;
    std::cout << "  Workers: 4  |  Test: N concurrent coroutines" << std::endl;

    std::vector<BenchMetrics> results;

    // Single EventLoop for all tests
    auto loop = std::make_shared<oj::judge::JudgeEventLoop>();
    loop->Run(4);

    // Warmup
    std::cout << "\n  [WARMUP] 10 tasks..." << std::flush;
    auto wm = RunBenchmark(loop, 10);
    std::cout << " completed=" << wm.completed << "/10" << std::endl;
    std::this_thread::sleep_for(milliseconds(200));

    // Benchmark runs
    for (int n : {10, 100, 1000}) {
        std::cout << "\n  [RUN] " << n << " concurrent tasks..." << std::flush;
        auto m = RunBenchmark(loop, n);
        results.push_back(m);
        PrintMetrics(std::to_string(n) + " Concurrent Tasks", m);
        std::this_thread::sleep_for(milliseconds(200));
    }

    // ── Summary ──
    std::cout << "\n╔══════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  SUMMARY                                     ║" << std::endl;
    std::cout << "╠═══════════╤══════════╤══════════╤════════════╣" << std::endl;
    std::cout << "║  Concur.  │  TPS     │  Avg Lat │  P99 Lat   ║" << std::endl;
    std::cout << "╠═══════════╪══════════╪══════════╪════════════╣" << std::endl;
    for (size_t i = 0; i < results.size(); ++i) {
        int n = (i == 0) ? 10 : (i == 1) ? 100 : 1000;
        std::cout << "║  " << std::setw(7) << n << "  │  " 
                  << std::setw(6) << std::fixed << std::setprecision(0) << results[i].throughput_tps << "  │  "
                  << std::setw(6) << results[i].avg_latency_ms << "ms │  "
                  << std::setw(6) << results[i].p99_latency_ms << "ms  ║" << std::endl;
    }
    std::cout << "╚═══════════╧══════════╧══════════╧════════════╝" << std::endl;

    return 0;
}
