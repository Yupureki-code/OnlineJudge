#!/usr/bin/env bash
set -euo pipefail

BASE_URL="http://127.0.0.1:8080"
OUT_DIR="./benchmarks/c2"
LABEL=""
REQ_ALL=80
REQ_ONE=80
QUESTION_ID=1
ROUNDS=1

usage() {
    cat <<'EOF'
Usage:
  tools/c2_benchmark_compare.sh --label baseline|candidate [options]

Options:
  --url <base_url>         default: http://127.0.0.1:8080
  --out <dir>              default: ./benchmarks/c2
  --all <count>            requests for /all_questions (default: 80)
  --one <count>            requests for /questions/:id (default: 80)
  --qid <id>               question id for detail page (default: 1)
    --rounds <n>             benchmark rounds, aggregate by median (default: 1)

Description:
  1) Captures cache metrics snapshot from /api/metrics/cache before benchmark.
  2) Sends benchmark traffic to /all_questions and /questions/:id.
  3) Captures cache metrics snapshot after benchmark.
  4) Writes snapshot report to <out>/<label>.txt.
  5) If both baseline.txt and candidate.txt exist, prints C2 comparison summary.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --label) LABEL="$2"; shift 2 ;;
        --url) BASE_URL="$2"; shift 2 ;;
        --out) OUT_DIR="$2"; shift 2 ;;
        --all) REQ_ALL="$2"; shift 2 ;;
        --one) REQ_ONE="$2"; shift 2 ;;
        --qid) QUESTION_ID="$2"; shift 2 ;;
        --rounds) ROUNDS="$2"; shift 2 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Unknown arg: $1"; usage; exit 1 ;;
    esac
done

if [[ -z "$LABEL" ]]; then
    echo "--label is required"
    usage
    exit 1
fi

if [[ "$LABEL" != "baseline" && "$LABEL" != "candidate" ]]; then
    echo "--label must be baseline or candidate"
    exit 1
fi

if ! [[ "$ROUNDS" =~ ^[0-9]+$ ]] || [[ "$ROUNDS" -lt 1 ]]; then
    echo "--rounds must be a positive integer"
    exit 1
fi

mkdir -p "$OUT_DIR"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

extract_json_num() {
    local key="$1"
    local json="$2"
    local val
    val=$(printf '%s' "$json" | grep -o '"'"$key"'"[[:space:]]*:[[:space:]]*[0-9]\+' | head -n1 | grep -o '[0-9]\+$' || true)
    if [[ -z "$val" ]]; then
        echo 0
    else
        echo "$val"
    fi
}

fetch_metrics_line() {
    local json
    json=$(curl -sS "$BASE_URL/api/metrics/cache" || true)
    if [[ -z "$json" ]]; then
        echo "list_db_fallbacks=0 detail_db_fallbacks=0 list_requests=0 detail_requests=0"
        return
    fi

    local list_db detail_db list_req detail_req
    list_db=$(extract_json_num "list_db_fallbacks" "$json")
    detail_db=$(extract_json_num "detail_db_fallbacks" "$json")
    list_req=$(extract_json_num "list_requests" "$json")
    detail_req=$(extract_json_num "detail_requests" "$json")

    echo "list_db_fallbacks=$list_db detail_db_fallbacks=$detail_db list_requests=$list_req detail_requests=$detail_req"
}

calc_stats() {
    local infile="$1"
    local outprefix="$2"

    local total errors
    total=$(wc -l < "$infile")
    errors=$(awk '$1 != 200 {c++} END {print c+0}' "$infile")

    awk '{print $2}' "$infile" | sort -n > "$TMP_DIR/${outprefix}_times.sorted"

    local n idx p95 avg err
    n=$(wc -l < "$TMP_DIR/${outprefix}_times.sorted")
    if [[ "$n" -eq 0 ]]; then
        p95=0
        avg=0
        err=100
    else
        idx=$(( (95 * n + 99) / 100 ))
        p95=$(sed -n "${idx}p" "$TMP_DIR/${outprefix}_times.sorted")
        avg=$(awk '{s+=$1} END {if (NR==0) print 0; else printf "%.6f", s/NR}' "$TMP_DIR/${outprefix}_times.sorted")
        err=$(awk -v e="$errors" -v t="$total" 'BEGIN { if (t==0) print 100; else printf "%.2f", (e*100.0)/t }')
    fi

    eval "${outprefix}_total=$total"
    eval "${outprefix}_errors=$errors"
    eval "${outprefix}_p95=$p95"
    eval "${outprefix}_avg=$avg"
    eval "${outprefix}_error_rate=$err"
}

run_load() {
    local endpoint="$1"
    local count="$2"
    local out="$3"

    : > "$out"
    for ((i=1; i<=count; i++)); do
        local line
        line=$(curl -sS -o /dev/null -w "%{http_code} %{time_total}" "$BASE_URL$endpoint" || true)
        if [[ -z "$line" ]]; then
            echo "599 0" >> "$out"
        else
            echo "$line" >> "$out"
        fi
    done
}

median_num() {
    if [[ "$#" -eq 0 ]]; then
        echo 0
        return
    fi

    printf '%s\n' "$@" | sort -n | awk '
        {a[NR]=$1}
        END {
            if (NR == 0) {
                print 0
            } else if (NR % 2 == 1) {
                printf "%.6f", a[(NR + 1) / 2]
            } else {
                printf "%.6f", (a[NR / 2] + a[NR / 2 + 1]) / 2
            }
        }
    '
}

declare -a all_p95_vals all_avg_vals all_err_vals
declare -a one_p95_vals one_avg_vals one_err_vals

sum_list_db=0
sum_detail_db=0
sum_total_db=0

for ((round=1; round<=ROUNDS; round++)); do
    start_metrics=$(fetch_metrics_line)

    run_load "/all_questions?page=1&size=5" "$REQ_ALL" "$TMP_DIR/all_${round}.raw"
    run_load "/questions/${QUESTION_ID}" "$REQ_ONE" "$TMP_DIR/one_${round}.raw"

    end_metrics=$(fetch_metrics_line)

    calc_stats "$TMP_DIR/all_${round}.raw" "all"
    calc_stats "$TMP_DIR/one_${round}.raw" "one"

    # shellcheck disable=SC2086
    for kv in $start_metrics; do eval "start_$kv"; done
    # shellcheck disable=SC2086
    for kv in $end_metrics; do eval "end_$kv"; done

    delta_list_db=$(( end_list_db_fallbacks - start_list_db_fallbacks ))
    delta_detail_db=$(( end_detail_db_fallbacks - start_detail_db_fallbacks ))
    delta_db_total=$(( delta_list_db + delta_detail_db ))

    sum_list_db=$(( sum_list_db + delta_list_db ))
    sum_detail_db=$(( sum_detail_db + delta_detail_db ))
    sum_total_db=$(( sum_total_db + delta_db_total ))

    all_p95_vals+=("$all_p95")
    all_avg_vals+=("$all_avg")
    all_err_vals+=("$all_error_rate")
    one_p95_vals+=("$one_p95")
    one_avg_vals+=("$one_avg")
    one_err_vals+=("$one_error_rate")

    echo "[C2][round ${round}/${ROUNDS}] all p95=${all_p95}s avg=${all_avg}s err=${all_error_rate}% | one p95=${one_p95}s avg=${one_avg}s err=${one_error_rate}% | db_total=${delta_db_total}"
done

all_p95=$(median_num "${all_p95_vals[@]}")
all_avg=$(median_num "${all_avg_vals[@]}")
all_error_rate=$(median_num "${all_err_vals[@]}")
one_p95=$(median_num "${one_p95_vals[@]}")
one_avg=$(median_num "${one_avg_vals[@]}")
one_error_rate=$(median_num "${one_err_vals[@]}")

delta_list_db="$sum_list_db"
delta_detail_db="$sum_detail_db"
delta_db_total="$sum_total_db"

report_file="$OUT_DIR/${LABEL}.txt"
cat > "$report_file" <<EOF
label=$LABEL
timestamp=$(date '+%Y-%m-%d %H:%M:%S')
base_url=$BASE_URL
rounds=$ROUNDS
aggregation=median
req_all=$REQ_ALL
req_one=$REQ_ONE
question_id=$QUESTION_ID
all_p95=$all_p95
all_avg=$all_avg
all_error_rate=$all_error_rate
one_p95=$one_p95
one_avg=$one_avg
one_error_rate=$one_error_rate
db_fallback_list_delta=$delta_list_db
db_fallback_detail_delta=$delta_detail_db
db_fallback_total_delta=$delta_db_total
EOF

echo "[C2] snapshot written: $report_file"
echo "[C2] all_questions: p95=${all_p95}s avg=${all_avg}s err=${all_error_rate}%"
echo "[C2] one_question : p95=${one_p95}s avg=${one_avg}s err=${one_error_rate}%"
echo "[C2] mysql回源(代理): list=$delta_list_db detail=$delta_detail_db total=$delta_db_total"

if [[ -f "$OUT_DIR/baseline.txt" && -f "$OUT_DIR/candidate.txt" ]]; then
    get_kv() {
        local file="$1"
        local key="$2"
        awk -F= -v k="$key" '$1==k{print $2; exit}' "$file"
    }

    base_all_p95="$(get_kv "$OUT_DIR/baseline.txt" "all_p95")"
    base_one_p95="$(get_kv "$OUT_DIR/baseline.txt" "one_p95")"
    base_all_err="$(get_kv "$OUT_DIR/baseline.txt" "all_error_rate")"
    base_one_err="$(get_kv "$OUT_DIR/baseline.txt" "one_error_rate")"
    base_db_total="$(get_kv "$OUT_DIR/baseline.txt" "db_fallback_total_delta")"

    cand_all_p95="$(get_kv "$OUT_DIR/candidate.txt" "all_p95")"
    cand_one_p95="$(get_kv "$OUT_DIR/candidate.txt" "one_p95")"
    cand_all_err="$(get_kv "$OUT_DIR/candidate.txt" "all_error_rate")"
    cand_one_err="$(get_kv "$OUT_DIR/candidate.txt" "one_error_rate")"
    cand_db_total="$(get_kv "$OUT_DIR/candidate.txt" "db_fallback_total_delta")"

    echo ""
    echo "========== C2 Compare (baseline -> candidate) =========="
    awk -v b="$base_all_p95" -v c="$cand_all_p95" 'BEGIN{ if (b==0) p=0; else p=((c-b)/b)*100; printf("all_questions p95: %.6fs -> %.6fs (%+.2f%%)\n", b, c, p)}'
    awk -v b="$base_one_p95" -v c="$cand_one_p95" 'BEGIN{ if (b==0) p=0; else p=((c-b)/b)*100; printf("one_question  p95: %.6fs -> %.6fs (%+.2f%%)\n", b, c, p)}'
    awk -v b="$base_all_err" -v c="$cand_all_err" 'BEGIN{ printf("all_questions err: %.2f%% -> %.2f%%\n", b, c)}'
    awk -v b="$base_one_err" -v c="$cand_one_err" 'BEGIN{ printf("one_question  err: %.2f%% -> %.2f%%\n", b, c)}'
    echo "db_fallback_total: ${base_db_total} -> ${cand_db_total}"
fi
