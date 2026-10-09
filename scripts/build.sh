#!/bin/bash
# 使い方: scripts/build.sh <server|client|monitor|unvedit|all> [make の追加引数...]
# 例:     scripts/build.sh server clean all
# ログは build-logs/<component>.log に保存される。
# macOS（ホスト）では Makefile.Darwin を使い、ログは build-logs/darwin/<component>.log、
# 生成物は src/<component>/build-darwin/ にできる（コンテナの Linux 版と混ざらない）。
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
COMPONENTS=(server client monitor unvedit)

if [ "$(uname -s)" = "Darwin" ]; then
  makefile=Makefile.Darwin
  logdir="$ROOT/build-logs/darwin"
  jobs="$(sysctl -n hw.ncpu)"
else
  if [ -z "${DEVCONTAINER:-}" ]; then
    echo "ERROR: Dev Container の外で実行されています。コンテナ内で実行してください。" >&2
    exit 2
  fi
  makefile=Makefile.Linux
  logdir="$ROOT/build-logs"
  jobs="$(nproc)"
fi

target="${1:-all}"; shift || true
if [ "$target" = "all" ]; then targets=("${COMPONENTS[@]}"); else targets=("$target"); fi

mkdir -p "$logdir"
rc=0
for c in "${targets[@]}"; do
  dir="$ROOT/src/$c"
  [ -d "$dir" ] || { echo "unknown component: $c" >&2; exit 2; }
  log="$logdir/$c.log"
  echo "=== building $c (log: ${log#$ROOT/})"
  # "clean all" は clean を先に別の make で実行する（同じ make の中だと、clean で消したファイルを
  # make がまだあるものとして扱い、何もビルドしないことがある）
  if [ "${1:-}" = "clean" ] && [ $# -gt 1 ]; then
    make -C "$dir" -f "$makefile" clean 2>&1 | tee "$log"
    make -C "$dir" -f "$makefile" -j"$jobs" "${@:2}" 2>&1 | tee -a "$log"
  else
    make -C "$dir" -f "$makefile" -j"$jobs" "$@" 2>&1 | tee "$log"
  fi
  status=${PIPESTATUS[0]}
  echo "=== $c: exit=$status errors=$(grep -c ' error' "$log") warnings=$(grep -c ' warning' "$log")"
  [ "$status" -eq 0 ] || rc=1
done
exit $rc
