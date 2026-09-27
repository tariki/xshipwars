#!/bin/bash
# 使い方: scripts/build.sh <server|client|monitor|unvedit|all> [make の追加引数...]
# 例:     scripts/build.sh server clean all
# ログは build-logs/<component>.log に保存される。
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPONENTS=(server client monitor unvedit)

if [ -z "${DEVCONTAINER:-}" ]; then
  echo "ERROR: Dev Container の外で実行されています。コンテナ内で実行してください。" >&2
  exit 2
fi

target="${1:-all}"; shift || true
if [ "$target" = "all" ]; then targets=("${COMPONENTS[@]}"); else targets=("$target"); fi

mkdir -p "$ROOT/build-logs"
rc=0
for c in "${targets[@]}"; do
  dir="$ROOT/src/$c"
  [ -d "$dir" ] || { echo "unknown component: $c" >&2; exit 2; }
  log="$ROOT/build-logs/$c.log"
  echo "=== building $c (log: build-logs/$c.log)"
  make -C "$dir" -f Makefile.Linux -j"$(nproc)" "$@" 2>&1 | tee "$log"
  status=${PIPESTATUS[0]}
  echo "=== $c: exit=$status errors=$(grep -c ' error' "$log") warnings=$(grep -c ' warning' "$log")"
  [ "$status" -eq 0 ] || rc=1
done
exit $rc
