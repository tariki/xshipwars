#!/bin/bash
# Windows（MSYS2 の UCRT64）でのスモークテスト
#
#   scripts/smoke-windows.sh [出力ディレクトリ]      (既定: run-logs/smoke-windows)
#
# 画面に窓を出さないように、SDL の dummy 映像ドライバで動かす。そのため、キー操作と画面は確かめられない
# （それは手で確かめる）。確認する項目:
#   - install-data.sh でデータをインストールでき、etc/xshipwarsrc の ToplevelDir がインストール先になる
#   - server が起動して待ち受ける
#   - monitor が AUX ポートに接続し、動き続ける
#   - client が Guest でログインし、動き続ける
#   - client の効果音が SDL2_mixer から出力される（SDL の disk 出力をファイルに書かせて調べる）
#   - server が kill(pid, SIGTERM) 相当の通知で保存して正常終了する（monitor が使うのと同じ経路）
#   - EngineState = -1 の物体が server の保存を通っても -1 のまま
#   - unvedit が起動する（Windows では SIGTERM を外から送れないので、緊急保存は確かめない）
# 各項目を PASS/FAIL で表示し、FAIL があれば終了コード 1。
# 事前に各 src/<component> で make -f Makefile.Windows でビルドしておくこと。
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ROOT_W="$(cygpath -m "$ROOT")"
OUT="${1:-$ROOT_W/run-logs/smoke-windows}"
OUT="$(cygpath -m "$OUT")"

case "${MSYSTEM:-}" in
  UCRT64) ;;
  *) echo "ERROR: MSYS2 の UCRT64 環境で実行してください。" >&2; exit 2 ;;
esac
for b in src/server/build-windows/swserv.exe src/client/build-windows/xsw.exe \
         src/monitor/build-windows/monitor.exe src/unvedit/build-windows/unvedit.exe; do
  [ -x "$ROOT/$b" ] || { echo "ERROR: $b がありません。make -f Makefile.Windows を実行してください。" >&2; exit 2; }
done

# 待ち受けは実際に接続して調べる（待ち受け中のソケットが netstat に出ない環境があった）
listening() { (exec 3<>"/dev/tcp/127.0.0.1/$1") 2>/dev/null; }
# (grep -q は一致すると読むのをやめるので、pipefail の下でパイプの途中に置くと失敗になる。出力を変数に受けてから調べる)
connected() {  # ポート、Windows の PID
  local t; t="$(netstat -ano -p TCP | tr -d '\r')"
  grep -Eq "[:.]$1 +.*ESTABLISHED +$2\$" <<< "$t"
}
if listening 1701 || listening 1702; then
  echo "ERROR: ポート 1701/1702 がすでに使われています。動いている server を止めてください。" >&2
  exit 2
fi

export SDL_VIDEODRIVER=dummy

fails=0
pass() { echo "PASS  $*"; }
fail() { echo "FAIL  $*"; fails=$((fails + 1)); }
alive() { kill -0 "$1" 2>/dev/null; }
winpid() { cat "/proc/$1/winpid" 2>/dev/null; }

PIDS=()
cleanup() {
  for p in "${PIDS[@]}"; do kill -9 "$p" 2>/dev/null; done
}
trap cleanup EXIT

# ------------------------------------------------------------------
# 実行環境の組み立て
rm -rf "$OUT"
SRV="$OUT/swserv-root"
XSW="$OUT/xsw-root"
HOMEDIR="$OUT/home"
mkdir -p "$SRV"/{bin,db,etc,logs,plugins,public_html,tmp} "$HOMEDIR/.shipwars"

sed "s#^ServerToplevelDir = .*#ServerToplevelDir = $SRV#" \
  "$ROOT/src/server/default.conf" > "$SRV/etc/generic.conf"
cp "$ROOT/src/server/default.ocs" "$ROOT/src/server/default.opm" "$SRV/db/"
# EngineState = -1 (ENGINE_STATE_NONE) の物体を 1 つ作る (char の符号の回帰テスト)
awk '/^    Name = Earth$/ {e=1} e && /^    EngineState = / {sub(/= .*/, "= -1"); e=0} {print}' \
  "$ROOT/src/server/generic_in.unv" > "$SRV/db/generic_in.unv"

if "$ROOT/scripts/install-data.sh" "$XSW" > "$OUT/install-data.log" 2>&1 &&
   grep -qx "ToplevelDir = $XSW" "$XSW/etc/xshipwarsrc"; then
  pass "install-data.sh でデータをインストールでき、ToplevelDir がインストール先になった"
else
  fail "install-data.sh (log: $OUT/install-data.log)"
fi

# client は初回起動時に etc/xshipwarsrc を ~/.shipwars にコピーする（ToplevelDir はインストール先になっている）
cp "$XSW/etc/xshipwarsrc" "$XSW/etc/universes" "$HOMEDIR/.shipwars/"

# server に monitor と同じ経路で SIGTERM 相当を送る補助プログラム
cat > "$OUT/term.cpp" <<'EOF'
#include <stdlib.h>
int main(int argc, char **argv) { return kill(atoi(argv[1]), SIGTERM) ? 1 : 0; }
EOF
C="$ROOT/src/compat/win32"
g++ -I"$C" -include compat.h "$OUT/term.cpp" "$C/compat.cpp" -o "$OUT/term.exe" -lws2_32 -lwinmm

# ------------------------------------------------------------------
# server
"$ROOT/src/server/build-windows/swserv.exe" --fg "$SRV/etc/generic.conf" > "$OUT/server.log" 2>&1 &
SRV_PID=$!; PIDS+=("$SRV_PID")
for _ in $(seq 50); do listening 1702 && break; sleep 0.2; done
if listening 1701 && listening 1702; then
  pass "server がポート 1701/1702 で待ち受けている"
else
  fail "server が待ち受けていない (log: $OUT/server.log)"
fi

# ------------------------------------------------------------------
# monitor (AUX ポートにログイン。-u はアドレスより前に書く)
HOME="$HOMEDIR" "$ROOT/src/monitor/build-windows/monitor.exe" -u Defiant yiffbaby 127.0.0.1 1702 \
  -i "$XSW/images/monitor" > "$OUT/monitor.log" 2>&1 &
MON_PID=$!; PIDS+=("$MON_PID")
sleep 4
if alive "$MON_PID" && connected 1702 "$(winpid "$MON_PID")"; then
  pass "monitor が AUX ポートに接続し、動き続けている"
else
  fail "monitor (log: $OUT/monitor.log)"
fi

# ------------------------------------------------------------------
# client (URL を引数に渡して Guest で接続)。音は SDL の disk 出力でファイルに書かせる
HOME="$HOMEDIR" SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$OUT/client-sound.raw" \
  "$ROOT/src/client/build-windows/xsw.exe" "swserv://Guest:guest@localhost:1701" \
  > "$OUT/client.log" 2>&1 &
XSW_PID=$!; PIDS+=("$XSW_PID")
for _ in $(seq 40); do
  grep -q "logged in as: Guest" "$SRV/logs/generic.log" 2>/dev/null && break
  sleep 0.5
done
if grep -q "logged in as: Guest" "$SRV/logs/generic.log" 2>/dev/null; then
  pass "client が Guest でログインした"
else
  fail "client がログインしていない (log: $OUT/client.log)"
fi
sleep 5
if alive "$XSW_PID"; then pass "client が動き続けている"; else fail "client が終了した"; fi
kill -9 "$XSW_PID" "$MON_PID" 2>/dev/null; sleep 2

# 起動時のロゴの音（xsw_logo01.wav, 約 2.6 秒）が出力の先頭付近にあるか
# （16bit ステレオ 44.1kHz。振幅 200 を超える 10ms の区間を数える）
loud=$(perl -e '
  open(F, "<:raw", $ARGV[0]) or do { print "0\n"; exit };
  read(F, $d, 44100 * 4 * 5);
  @a = unpack("s<*", $d); $n = 0;
  for ($i = 0; $i < @a; $i += 882) {
    $m = 0;
    for ($j = $i; $j < $i + 882 && $j < @a; $j++) { $v = abs($a[$j]); $m = $v if $v > $m }
    $n++ if $m > 200;
  }
  print "$n\n";' "$OUT/client-sound.raw")
if [ "${loud:-0}" -ge 100 ]; then
  pass "SDL2_mixer で効果音が出力された (起動 5 秒間に音のある 10ms 区間が ${loud} 個)"
else
  fail "SDL2_mixer の効果音が出力されていない (音のある 10ms 区間 ${loud:-0} 個)"
fi

# ------------------------------------------------------------------
# server を終了させ、保存されたユニバースを確認する
"$OUT/term.exe" "$(winpid "$SRV_PID")"
for _ in $(seq 50); do alive "$SRV_PID" || break; sleep 0.2; done
if grep -q "shut down normally" "$OUT/server.log"; then
  pass "server が正常終了した"
else
  fail "server が正常終了していない (log: $OUT/server.log)"
fi
out_unv="$SRV/db/generic_out.unv"
earth_es=$(awk '/^    Name = Earth$/ {e=1} e && /^    EngineState = / {print $3; exit}' "$out_unv" 2>/dev/null)
if [ "$earth_es" = "-1" ] && ! grep -q "EngineState = 255" "$out_unv"; then
  pass "server の保存で EngineState = -1 が保たれた"
else
  fail "server の保存で EngineState が壊れた (Earth: '$earth_es')"
fi

# ------------------------------------------------------------------
# unvedit
cp "$SRV/db/generic_in.unv" "$OUT/unvedit-orig.unv"
printf 'ToplevelDir = %s\nImagesDir = %s/images\nServerDir = %s\n' \
  "$XSW" "$XSW" "$SRV" > "$HOMEDIR/.shipwars/unveditrc"
HOME="$HOMEDIR" "$ROOT/src/unvedit/build-windows/unvedit.exe" "$OUT/unvedit-orig.unv" > "$OUT/unvedit.log" 2>&1 &
UE_PID=$!; PIDS+=("$UE_PID")
sleep 4
if alive "$UE_PID"; then pass "unvedit が起動した"; else fail "unvedit が終了した (log: $OUT/unvedit.log)"; fi

echo
echo "logs: $OUT"
[ "$fails" -eq 0 ] && echo "ALL PASSED" || echo "$fails FAILED"
[ "$fails" -eq 0 ]
