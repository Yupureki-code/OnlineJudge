#!/bin/sh
# docker/scripts/compile.sh
# 编译容器脚本 — 编译用户代码后通知 JudgeService
#
# 环境变量:
#   TASK_ID    — 任务唯一标识（对应协程句柄）
#   JUDGE_HOST — JudgeService 地址（如 host.docker.internal:8082）
#
# 容器内文件:
#   /home/judge/source.cpp — 用户源代码
#   /home/judge/main       — 编译输出
#   /home/judge/compile_err — 编译错误信息

set -e

# 1. 执行编译
g++ -o /home/judge/main /home/judge/source.cpp -std=c++11 -O2 -w 2>/home/judge/compile_err || true
EXIT_CODE=$?

# 2. 判断编译结果
if [ $EXIT_CODE -eq 0 ]; then
    STATUS="OK"
else
    STATUS="COMPILE_ERROR"
fi

# 3. 通知 JudgeService
curl -s -X POST \
    "http://${JUDGE_HOST:-host.docker.internal:8082}/JudgeService/DockerWorkDone" \
    -H "Content-Type: application/json" \
    -d "{\"id\":\"${TASK_ID}\",\"status\":\"${STATUS}\",\"exit_code\":${EXIT_CODE}}"

# 4. 编译容器自动退出，由 JudgeService 负责销毁