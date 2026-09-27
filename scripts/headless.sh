#!/bin/bash
# Xvfb によるヘッドレス X 環境の操作
#   scripts/headless.sh start            Xvfb を :99 で起動 (起動済みなら何もしない)
#   scripts/headless.sh shot <out.png>   画面全体のスクリーンショットを保存
#   scripts/headless.sh key <keysym...>  xdotool でキー入力を送る
#   scripts/headless.sh stop             Xvfb を停止
# X クライアントは DISPLAY=:99 を付けて起動すること。
set -euo pipefail

DISP="${XSW_DISPLAY:-:99}"
PIDFILE="/tmp/xvfb${DISP#:}.pid"

case "${1:-}" in
  start)
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
      echo "Xvfb already running on $DISP"; exit 0
    fi
    Xvfb "$DISP" -screen 0 1024x768x24 -nolisten tcp >/tmp/xvfb.log 2>&1 &
    echo $! >"$PIDFILE"
    for _ in $(seq 50); do DISPLAY="$DISP" xdpyinfo >/dev/null 2>&1 && break; sleep 0.1; done
    echo "Xvfb started on $DISP (pid $(cat "$PIDFILE"))"
    ;;
  shot)
    out="${2:?output file required}"
    mkdir -p "$(dirname "$out")"
    DISPLAY="$DISP" import -window root "$out"
    echo "saved $out"
    ;;
  key)
    shift
    DISPLAY="$DISP" xdotool key "$@"
    ;;
  stop)
    [ -f "$PIDFILE" ] && kill "$(cat "$PIDFILE")" 2>/dev/null || true
    rm -f "$PIDFILE"
    ;;
  *)
    sed -n '2,7p' "$0"; exit 2
    ;;
esac
