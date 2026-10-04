#!/bin/bash
# ホスト (macOS) の XQuartz に XShipWars を表示する
#
#   scripts/xquartz.sh [client] [monitor] [unvedit]     (省略時は client)
#
# コンテナ内で server を起動し、指定したプログラムをホストの X サーバに表示する。
# 表示したプログラムをすべて閉じるか Ctrl-C で、server も止める。
#
# 事前準備:
#   - macOS: XQuartz の 設定 → セキュリティ →「ネットワーク・クライアントからの接続を許可」をオンにして
#     XQuartz を再起動し、XQuartz の xterm で `xhost +localhost` を実行する
#   - コンテナ: .devcontainer/init-firewall.sh が host.docker.internal の tcp/6000 を許可していること
#     (変更後はコンテナのリビルドが必要)
#   - scripts/build.sh all でビルド済みであること
#
# 環境変数:
#   XSW_HOST_DISPLAY  表示先 (既定: host.docker.internal:0)
#
# 実行環境は run-logs/xquartz/ に置く。server の環境と HOME は初回だけ作って残す
# (Key Mappings の「Default All」で割り当て直したキーを保つため)。データは毎回インストールし直す。
#
# 注意:
#   - XQuartz のキーコードは Xorg と違うので、同梱のキー割り当ては効かない。client の
#     Key Mappings ウィンドウで「Default All」→「Apply」(または OK) を押して割り当て直す。
#     Default All だけでは反映されない。設定は client を正規に終了したとき (bridge ウィンドウを
#     閉じる、Exit キー) に HOME の xshipwarsrc に保存される。Ctrl-C で止めると保存されない
#   - Ctrl-C で止めると、unvedit は緊急保存のファイル (univXXXXXX) を HOME に作る
#   - 共有メモリ (MIT-SHM) はネットワーク越しに使えないので、--no_xshm を付けて起動する
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/run-logs/xquartz"
DISP="${XSW_HOST_DISPLAY:-host.docker.internal:0}"

if [ -z "${DEVCONTAINER:-}" ]; then
  echo "ERROR: Dev Container の外で実行されています。" >&2
  exit 2
fi

progs=("$@")
[ ${#progs[@]} -eq 0 ] && progs=(client)
for p in "${progs[@]}"; do
  case "$p" in client|monitor|unvedit) ;; *) echo "ERROR: 不明なプログラム: $p (client|monitor|unvedit)" >&2; exit 2;; esac
done
for b in src/server/swserv src/client/xsw src/monitor/monitor src/unvedit/unvedit; do
  [ -x "$ROOT/$b" ] || { echo "ERROR: $b がありません。scripts/build.sh all を実行してください。" >&2; exit 2; }
done

# ------------------------------------------------------------------
# 表示先に接続できるか
if ! DISPLAY="$DISP" xdpyinfo >/dev/null 2>&1; then
  cat >&2 <<EOF
ERROR: X サーバ $DISP に接続できません。次を確認してください。
  1. XQuartz が起動している
  2. XQuartz の 設定 → セキュリティ →「ネットワーク・クライアントからの接続を許可」がオン
     (変更後は XQuartz の再起動が必要)
  3. XQuartz の xterm で xhost +localhost を実行した
  4. コンテナのファイアウォールが tcp/6000 を許可している
     (.devcontainer/init-firewall.sh の変更後は、コンテナのリビルドが必要)
EOF
  exit 1
fi
for f in 7x14 6x10; do
  DISPLAY="$DISP" xlsfonts -fn "$f" >/dev/null 2>&1 ||
    echo "WARNING: $DISP にフォント $f がありません。client は起動できない可能性があります。" >&2
done

# ------------------------------------------------------------------
# 実行環境
SRV="$OUT/swserv-root"
XSW="$OUT/xsw-root"
HOMEDIR="$OUT/home"
mkdir -p "$OUT"

if [ ! -d "$SRV" ]; then
  mkdir -p "$SRV"/{bin,db,etc,logs,plugins,public_html,tmp}
  sed "s#^ServerToplevelDir = .*#ServerToplevelDir = $SRV#" \
    "$ROOT/src/server/default.conf" > "$SRV/etc/generic.conf"
  cp "$ROOT/src/server/default.ocs" "$ROOT/src/server/default.opm" \
     "$ROOT/src/server/generic_in.unv" "$SRV/db/"
fi
# 前回終了時に server が保存した宇宙を次の入力にする
[ -f "$SRV/db/generic_out.unv" ] && mv -f "$SRV/db/generic_out.unv" "$SRV/db/generic_in.unv"

"$ROOT/scripts/install-data.sh" "$XSW" > "$OUT/install-data.log" 2>&1 ||
  { echo "ERROR: install-data.sh が失敗しました (log: $OUT/install-data.log)" >&2; exit 1; }

if [ ! -f "$HOMEDIR/.shipwars/xshipwarsrc" ]; then
  mkdir -p "$HOMEDIR/.shipwars"
  sed -e "s#^ToplevelDir = .*#ToplevelDir = $XSW#" \
      -e "s#/home/learfox#$HOMEDIR#" \
      "$XSW/etc/xshipwarsrc" > "$HOMEDIR/.shipwars/xshipwarsrc"
  cp "$XSW/etc/universes" "$HOMEDIR/.shipwars/"
  printf 'ToplevelDir = %s\nImagesDir = %s/images\nServerDir = %s\n' \
    "$XSW" "$XSW" "$SRV" > "$HOMEDIR/.shipwars/unveditrc"
  echo "初回: キー操作を使うには、client の Key Mappings で「Default All」→「Apply」を押し、"
  echo "      bridge ウィンドウを閉じて終了してください (設定が保存されます)。"
fi
[ -f "$OUT/unvedit.unv" ] || cp "$SRV/db/generic_in.unv" "$OUT/unvedit.unv"

# ------------------------------------------------------------------
# 起動
PIDS=()
SRV_PID=""
cleanup() {
  for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done
  if [ -n "$SRV_PID" ]; then
    kill "$SRV_PID" 2>/dev/null
    wait "$SRV_PID" 2>/dev/null
  fi
  echo "終了しました (logs: $OUT)"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

if ss -ltn | grep -q ':1701 '; then
  echo "ポート 1701 は使用中なので、server は起動しません (既存の server に接続します)。"
else
  (cd "$SRV" && exec "$ROOT/src/server/swserv" --fg etc/generic.conf) > "$OUT/server.log" 2>&1 &
  SRV_PID=$!
  for _ in $(seq 50); do ss -ltn | grep -q ':1702 ' && break; sleep 0.2; done
  ss -ltn | grep -q ':1701 ' || { echo "ERROR: server が起動しません (log: $OUT/server.log)" >&2; exit 1; }
fi

export DISPLAY="$DISP"
for p in "${progs[@]}"; do
  case "$p" in
    client)
      (HOME="$HOMEDIR" exec "$ROOT/src/client/xsw" --no_xshm "swserv://Guest:guest@localhost:1701") \
        > "$OUT/client.log" 2>&1 & ;;
    monitor)
      (exec "$ROOT/src/monitor/monitor" --no_xshm -u Defiant yiffbaby 127.0.0.1 1702 \
        -i "$XSW/images/monitor") > "$OUT/monitor.log" 2>&1 & ;;
    unvedit)
      (HOME="$HOMEDIR" exec "$ROOT/src/unvedit/unvedit" --no_xshm "$OUT/unvedit.unv") \
        > "$OUT/unvedit.log" 2>&1 & ;;
  esac
  PIDS+=($!)
  echo "$p を $DISP に表示しました (log: $OUT/$p.log)"
done

echo "すべてのウィンドウを閉じるか Ctrl-C で終了します。"
wait "${PIDS[@]}"
