#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
exec protoc \
  --proto_path="$ROOT/src/comm/proto" \
  --include_imports \
  --descriptor_set_out="$ROOT/gateway/generated/onlinejudge.pb" \
  "$ROOT/src/comm/proto/common.proto" \
  "$ROOT/src/comm/proto/judge_service.proto" \
  "$ROOT/src/comm/proto/biz_service.proto"
