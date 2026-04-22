# C2 Benchmark Report

Date: 2026-04-22
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

- all_p95: 0.001274s
- all_avg: 0.000956s
- all_error_rate: 0.000000%
- one_p95: 0.001105s
- one_avg: 0.000809s
- one_error_rate: 0.000000%
- db_fallback_list_delta: 0
- db_fallback_detail_delta: 0
- db_fallback_total_delta: 0
- html_static_hit_rate: 0.00%
- html_list_hit_rate: 100.00%
- html_detail_hit_rate: 100.00%

Source: benchmarks/c2/baseline.txt

### Candidate

- all_p95: 0.001197s
- all_avg: 0.000876s
- all_error_rate: 0.000000%
- one_p95: 0.001006s
- one_avg: 0.000746s
- one_error_rate: 0.000000%
- db_fallback_list_delta: 0
- db_fallback_detail_delta: 0
- db_fallback_total_delta: 0
- html_static_hit_rate: 0.00%
- html_list_hit_rate: 100.00%
- html_detail_hit_rate: 100.00%

Source: benchmarks/c2/candidate.txt

## Compare Summary (baseline -> candidate)

- all_questions p95: 0.001274s -> 0.001197s (-6.04%)
- one_question p95: 0.001105s -> 0.001006s (-8.96%)
- all_questions error rate: 0.00% -> 0.00%
- one_question error rate: 0.00% -> 0.00%
- db_fallback_total: 0 -> 0
- html list hit: 100.00% -> 100.00%
- html detail hit: 100.00% -> 100.00%

## Conclusion

C2 acceptance goals are met for this run:

- P95 latency improved on both hot endpoints.
- Error rate stayed at 0%.
- MySQL fallback proxy count stayed at 0 in both runs.
- HTML cache hit rate stayed stable at 100% on list/detail pages during benchmark traffic.

This report can be used as the current optimization baseline for follow-up changes.
