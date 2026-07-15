#!/bin/sh
# docker/scripts/run.sh
# 运行容器脚本 — 运行用户程序后通知 JudgeService
#
# 环境变量:
#   TASK_ID    — 任务唯一标识（对应协程句柄）
#   JUDGE_HOST — JudgeService 地址
#   TIME_LIMIT — CPU 时间限制（秒）
#   MEM_LIMIT  — 内存限制（KB）
#
# 容器内文件:
#   /home/judge/main        — 用户可执行文件
#   /home/judge/input.txt   — 测试输入
#   /home/judge/output.txt  — 用户输出
#   /home/judge/stderr.txt  — 用户错误输出

# 1. 设置资源限制
ulimit -t ${TIME_LIMIT:-2} 2>/dev/null || true
ulimit -v ${MEM_LIMIT:-262144} 2>/dev/null || true

# 2. 执行用户程序
./main < /home/judge/input.txt > /home/judge/output.txt 2> /home/judge/stderr.txt || true
EXIT_CODE=$?

# 3. 退出码映射到判题状态
case $EXIT_CODE in
    0)   STATUS="OK" ;;
    134) STATUS="MEMORY_LIMIT" ;;   # SIGABRT
    139) STATUS="SEGV" ;;           # SIGSEGV
    136) STATUS="FPE" ;;            # SIGFPE
    152) STATUS="TIME_LIMIT" ;;     # SIGXCPU
    *)   STATUS="RUNTIME_ERROR" ;;
esac

# 4. 通知 JudgeService
curl -s -X POST \
    "http://${JUDGE_HOST:-host.docker.internal:8082}/JudgeService/DockerWorkDone" \
    -H "Content-Type: application/json" \
    -d "{\"id\":\"${TASK_ID}\",\"status\":\"${STATUS}\",\"exit_code\":${EXIT_CODE}}"

# 5. 清理临时文件（容器复用）
rm -f /home/judge/main /home/judge/input.txt /home/judge/output.txt /home/judge/stderr.txt