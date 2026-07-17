#!/bin/sh
# The host enforces the wall timeout and inspects this exec's real exit status.

ulimit -S -t "${TIME_LIMIT:-2}" || exit 125
ulimit -v "${MEM_LIMIT:-262144}" || exit 125
ulimit -f "${OUTPUT_LIMIT_KB:-1024}" || exit 125

exec /home/judge/main \
    </home/judge/input.txt \
    >/home/judge/output.txt \
    2>/home/judge/stderr.txt
