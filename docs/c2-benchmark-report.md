# C2 Benchmark Report

Date: 2026-04-21
Scope: Redis cache optimization regression benchmark for `/all_questions` and `/questions/:id`.
Method: 5 rounds, 500 requests/round for each endpoint, median aggregation.

## Environment

- Base URL: http://127.0.0.1:8080
- Script: tools/c2_benchmark_compare.sh
- Command (baseline):
  - `./tools/c2_benchmark_compare.sh --label baseline --url http://127.0.0.1:8080 --all 500 --one 500 --qid 1 --rounds 5`
- Command (candidate):
  - `./tools/c2_benchmark_compare.sh --label candidate --url http://127.0.0.1:8080 --all 500 --one 500 --qid 1 --rounds 5`

## Snapshot Results

### Baseline

- all_p95: 0.001482s
- all_avg: 0.001023s
- all_error_rate: 0.000000%
- one_p95: 0.001186s
- one_avg: 0.000849s
- one_error_rate: 0.000000%
- db_fallback_list_delta: 4
- db_fallback_detail_delta: 0
- db_fallback_total_delta: 4

Source: benchmarks/c2/baseline.txt

### Candidate

- all_p95: 0.001180s
- all_avg: 0.000852s
- all_error_rate: 0.000000%
- one_p95: 0.001063s
- one_avg: 0.000747s
- one_error_rate: 0.000000%
- db_fallback_list_delta: 1
- db_fallback_detail_delta: 0
- db_fallback_total_delta: 1

Source: benchmarks/c2/candidate.txt

## Compare Summary (baseline -> candidate)

- all_questions p95: 0.001482s -> 0.001180s (-20.38%)
- one_question p95: 0.001186s -> 0.001063s (-10.37%)
- all_questions error rate: 0.00% -> 0.00%
- one_question error rate: 0.00% -> 0.00%
- db_fallback_total: 4 -> 1

## Conclusion

C2 acceptance goals are met for this run:

- P95 latency improved on both hot endpoints.
- Error rate stayed at 0%.
- MySQL fallback proxy count decreased.

This report can be used as the current optimization baseline for follow-up changes.
