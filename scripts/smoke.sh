#!/bin/bash
# XShipWars のヘッドレス・スモークテスト
#
#   scripts/smoke.sh [出力ディレクトリ]      (既定: run-logs/smoke)
#   GUI=sdl scripts/smoke.sh [出力ディレクトリ]  (既定: run-logs/smoke-sdl)
#
# GUI=sdl のときは client/monitor/unvedit に SDL2 版（scripts/build.sh <c> GUI=sdl で build-linux-sdl/ に
# できるもの）を使い、SDL の x11 ドライバで Xvfb に表示する。確認する項目は同じ。
#
# 実行環境を毎回作り直し、Xvfb 上で次を確認する。
#   - server が起動して待ち受ける
#   - monitor が AUX ポートにログインして統計を受け取る
#   - client が Guest で接続し、旋回・推力・推力モード(Shift+F8 で逆回し)が効く
#     (client の内部の値を gdb で読んで判定する)
#   - EngineState = -1 の物体が server と unvedit の保存を通っても -1 のまま
#   - unvedit で開いて保存し直したファイルが元と同一
#   - client の効果音が SDL2_mixer から出力される（SDL の disk 出力をファイルに書かせて調べる）
# client/monitor/unvedit のデータは scripts/install-data.sh でインストールしたものを使う。
# 各項目を PASS/FAIL で表示し、FAIL があれば終了コード 1。
# 事前に scripts/build.sh all でビルドしておくこと。
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GUI="${GUI:-x11}"
case "$GUI" in
  x11) BINDIR=""; OUT="${1:-$ROOT/run-logs/smoke}" ;;
  sdl) BINDIR="build-linux-sdl/"; OUT="${1:-$ROOT/run-logs/smoke-sdl}"; export SDL_VIDEODRIVER=x11 ;;
  *) echo "ERROR: GUI は x11 か sdl" >&2; exit 2 ;;
esac
XSW_BIN="src/client/${BINDIR}xsw"
MON_BIN="src/monitor/${BINDIR}monitor"
UE_BIN="src/unvedit/${BINDIR}unvedit"
DISP=:99
export DISPLAY=$DISP

if [ -z "${DEVCONTAINER:-}" ]; then
  echo "ERROR: Dev Container の外で実行されています。" >&2
  exit 2
fi
for b in src/server/swserv "$XSW_BIN" "$MON_BIN" "$UE_BIN"; do
  [ -x "$ROOT/$b" ] || { echo "ERROR: $b がありません。scripts/build.sh all (GUI=sdl なら scripts/build.sh <c> GUI=sdl) を実行してください。" >&2; exit 2; }
done

fails=0
pass() { echo "PASS  $*"; }
fail() { echo "FAIL  $*"; fails=$((fails + 1)); }
pause() { timeout "$1" tail -f /dev/null; }
alive() { kill -0 "$1" 2>/dev/null; }
# gdb で実行中のプロセスの式を読む (例: gdbval PID 'option.throttle_mode')
gdbval() {
  timeout 30 gdb -batch -p "$1" -ex "print $2" 2>/dev/null |
    sed -n 's/^\$1 = //p' | tail -1 | awk '{print $1}'
}

PIDS=()
cleanup() {
  for p in "${PIDS[@]}"; do kill "$p" 2>/dev/null; done
  pause 2
  "$ROOT/scripts/headless.sh" stop
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
# EngineState = -1 (ENGINE_STATE_NONE) の物体を 1 つ作る (arm64 の char 符号の回帰テスト)
awk '/^    Name = Earth$/ {e=1} e && /^    EngineState = / {sub(/= .*/, "= -1"); e=0} {print}' \
  "$ROOT/src/server/generic_in.unv" > "$SRV/db/generic_in.unv"

# client/monitor/unvedit のデータは、インストールスクリプトで実際にインストールして使う
# (インストール手順の確認も兼ねる)
if "$ROOT/scripts/install-data.sh" "$XSW" > "$OUT/install-data.log" 2>&1; then
  pass "install-data.sh でデータをインストールできた"
else
  fail "install-data.sh が失敗した (log: $OUT/install-data.log)"
fi

# client は初回起動時に etc/xshipwarsrc を ~/.shipwars にコピーする。その代わりに、
# パスだけをこの実行環境に合わせたものを置く
# （音の設定は同梱の既定値のまま: SDL2_mixer で鳴らす SoundServerType = 4、Sounds = 3。
#   背景音楽は既定値によらず確かめるため Music = on にする）
# ジョイスティック 0 の割り当て: 軸 0 = 旋回、ボタン 0 = 推力モードの切り替え (F8、キーコード 74)。
# 操作はキーボードのまま始め、後で gdb から SDL の仮想ジョイスティックをつないで切り替える
cat > "$OUT/jsmap.rc" <<'EOF'
BeginJSMap
    DeviceName = /dev/js0
    BeginAxis
        OpCode = 0
    EndAxis
    BeginButton
        Keycode = 74
    EndButton
EndJSMap
EOF
sed -e "s#^ToplevelDir = .*#ToplevelDir = $XSW#" \
    -e "s#/home/learfox#$HOMEDIR#" \
    -e "s#^Music = .*#Music = on#" \
    -e "/^# Joystick mappings:/r $OUT/jsmap.rc" \
    "$XSW/etc/xshipwarsrc" > "$HOMEDIR/.shipwars/xshipwarsrc"
cp "$XSW/etc/universes" "$HOMEDIR/.shipwars/"
printf 'ToplevelDir = %s\nImagesDir = %s/images\nServerDir = %s\n' \
  "$XSW" "$XSW" "$SRV" > "$HOMEDIR/.shipwars/unveditrc"

# ------------------------------------------------------------------
# Xvfb とフォント
"$ROOT/scripts/headless.sh" start >/dev/null
if xlsfonts -fn 7x14 >/dev/null 2>&1 && xlsfonts -fn 6x10 >/dev/null 2>&1; then
  pass "X コアフォント 7x14 / 6x10 がある"
else
  fail "X コアフォント 7x14 / 6x10 が無い (xfonts-base が必要)"
fi

# ------------------------------------------------------------------
# server
"$ROOT/src/server/swserv" --fg "$SRV/etc/generic.conf" > "$OUT/server.log" 2>&1 &
SRV_PID=$!; PIDS+=("$SRV_PID")
for _ in $(seq 50); do ss -ltn | grep -q ':1702 ' && break; pause 0.2; done
if ss -ltn | grep -q ':1701 ' && ss -ltn | grep -q ':1702 '; then
  pass "server がポート 1701/1702 で待ち受けている"
else
  fail "server が待ち受けていない (log: $OUT/server.log)"
fi

# ------------------------------------------------------------------
# monitor (AUX ポートにログイン。-u はアドレスより前に書く)
"$ROOT/$MON_BIN" -u Defiant yiffbaby 127.0.0.1 1702 \
  -i "$XSW/images/monitor" > "$OUT/monitor.log" 2>&1 &
MON_PID=$!; PIDS+=("$MON_PID")
# AUX の TITLE を受け取るとウィンドウ名がユニバース名になる
for _ in $(seq 20); do
  xwininfo -root -children 2>/dev/null | grep -q '"Generic Universe"' && break
  pause 0.5
done
if alive "$MON_PID" && xwininfo -root -children 2>/dev/null | grep -q '"Generic Universe"'; then
  pass "monitor が AUX 統計を受け取った (ウィンドウ名 = ユニバース名)"
else
  fail "monitor が AUX 統計を受け取っていない"
fi

# ------------------------------------------------------------------
# client (URL を引数に渡して Guest で接続)
# 音声デバイスは無いので、SDL の disk 出力でミキサーの出力をファイルに書かせる
HOME="$HOMEDIR" SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$OUT/client-sound.raw" \
  "$ROOT/$XSW_BIN" "swserv://Guest:guest@localhost:1701" \
  > "$OUT/client.log" 2>&1 &
XSW_PID=$!; PIDS+=("$XSW_PID")
bridge=""
for _ in $(seq 40); do
  bridge=$(xdotool search --name "XShipWars: localhost" 2>/dev/null | head -1)
  [ -n "$bridge" ] && grep -q "logged in as: Guest" "$SRV/logs/generic.log" 2>/dev/null && break
  pause 0.5
done
if [ -n "$bridge" ] && grep -q "logged in as: Guest" "$SRV/logs/generic.log" 2>/dev/null; then
  pass "client が Guest でログインした"
else
  fail "client がログインしていない (log: $OUT/client.log)"
fi
pause 3
"$ROOT/scripts/headless.sh" shot "$OUT/bridge-1.png" >/dev/null

# ウィンドウマネージャが無いので bridge に明示的にフォーカスを与える
[ -n "$bridge" ] && xdotool windowfocus --sync "$bridge"
pause 0.5
hold() { xdotool keydown "$1"; pause "$2"; xdotool keyup "$1"; pause 1; }

hd0=$(gdbval "$XSW_PID" 'net_parms.player_obj_ptr->heading')
hold Right 1
hd1=$(gdbval "$XSW_PID" 'net_parms.player_obj_ptr->heading')
if [ -n "$hd0" ] && [ -n "$hd1" ] && [ "$hd0" != "$hd1" ]; then
  pass "Right キーで旋回した (heading $hd0 -> $hd1 rad)"
else
  fail "Right キーで旋回しない (heading '$hd0' -> '$hd1')"
fi

hold Up 2
vel=$(gdbval "$XSW_PID" 'net_parms.player_obj_ptr->velocity')
if [ -n "$vel" ] && awk "BEGIN{exit !($vel > 0)}"; then
  pass "Up キーで加速した (velocity $vel)"
else
  fail "Up キーで加速しない (velocity '$vel')"
fi

# 3 回押して一周させ、Normal(0) から Incremental(2) への折り返しを必ず通す
# (char が unsigned の arm64 では 0 - 1 が 255 になり Normal に戻っていた)
tm=$(gdbval "$XSW_PID" 'option.throttle_mode'); seq_tm="$tm"; tm_ok=1
for _ in 1 2 3; do
  xdotool key shift+F8; pause 1
  next=$(gdbval "$XSW_PID" 'option.throttle_mode')
  [[ "$tm" =~ ^[0-9]+$ ]] && [ "$next" = "$(( tm > 0 ? tm - 1 : 2 ))" ] || tm_ok=0
  tm="$next"; seq_tm="$seq_tm -> $next"
done
if [ "$tm_ok" -eq 1 ]; then
  pass "Shift+F8 で推力モードを逆に一周できた ($seq_tm)"
else
  fail "Shift+F8 の推力モードの逆回し ($seq_tm)"
fi

# 背景音楽: ログイン後は通常の曲 (100) が流れていること、戦闘の曲 (102) に切り替わり、
# 割り当ての無いメインメニューの曲 (104) では止まること
# (ゲームは状況から曲を選び直すので、切り替えと確認は 1 回の attach の中で行い、
#  detach 後には通常の曲に戻ることを確かめる)
music=$(timeout 30 gdb -batch -p "$XSW_PID" \
  -ex 'print sound.bkg_mood_code' -ex 'print (int)Mix_PlayingMusic()' \
  -ex 'call (void)SoundChangeBackgroundMusic(102, 0, 0)' \
  -ex 'print sound.bkg_mood_code' -ex 'print (int)Mix_PlayingMusic()' \
  -ex 'call (void)SoundChangeBackgroundMusic(104, 0, 0)' \
  -ex 'print (int)Mix_PlayingMusic()' 2>/dev/null |
  sed -n 's/^\$[0-9]* = //p' | tr '\n' ' ')
pause 2
mood_after=$(gdbval "$XSW_PID" 'sound.bkg_mood_code')
playing_after=$(gdbval "$XSW_PID" '(int)Mix_PlayingMusic()')
if [ "$music" = "100 1 102 1 0 " ] && [ "$mood_after" = 100 ] && [ "$playing_after" = 1 ]; then
  pass "背景音楽 (MIDI) が流れ、曲の切り替えと停止ができた"
else
  fail "背景音楽 (曲/再生中: ${music}-> 戻った後 $mood_after $playing_after)"
fi
# ジョイスティック: 実機が無いので、SDL の仮想ジョイスティック (軸 2・ボタン 1・ハット 1) を
# gdb からつなぐ。先に SDL のジョイスティック機能を 1 回初期化しておくのは、GCtlInit() が
# 開き直すときに仮想デバイスが消えないようにするため
js=$(timeout 30 gdb -batch -p "$XSW_PID" \
  -ex 'print (int)SDL_InitSubSystem(0x200)' \
  -ex 'print (int)SDL_JoystickAttachVirtual(1, 2, 1, 1)' \
  -ex 'print GCtlInit(1)' \
  -ex 'print jsmap[0]->jsd.total_axises' -ex 'print jsmap[0]->jsd.total_buttons' \
  -ex 'print (int)SDL_JoystickSetVirtualAxis((void *)SDL_JoystickFromInstanceID(jsmap[0]->jsd.fd), 0, 32767)' \
  2>/dev/null | sed -n 's/^\$[0-9]* = //p' | tr '\n' ' ')
hd0=$(gdbval "$XSW_PID" 'net_parms.player_obj_ptr->heading')
pause 1
turn=$(gdbval "$XSW_PID" 'gctl[0].turn')
hd1=$(gdbval "$XSW_PID" 'net_parms.player_obj_ptr->heading')
tm0=$(gdbval "$XSW_PID" 'option.throttle_mode')
gdbval "$XSW_PID" '(int)SDL_JoystickSetVirtualButton((void *)SDL_JoystickFromInstanceID(jsmap[0]->jsd.fd), 0, 1)' >/dev/null
pause 0.5
gdbval "$XSW_PID" '(int)SDL_JoystickSetVirtualButton((void *)SDL_JoystickFromInstanceID(jsmap[0]->jsd.fd), 0, 0)' >/dev/null
pause 0.5
tm1=$(gdbval "$XSW_PID" 'option.throttle_mode')
if [ "$js" = "0 0 0 4 1 0 " ] && [ "$turn" = 1 ] && [ "$hd0" != "$hd1" ] &&
   [[ "$tm0" =~ ^[0-9]+$ ]] && [ "$tm1" = "$(( (tm0 + 1) % 3 ))" ]; then
  pass "仮想ジョイスティックの軸で旋回し、ボタンで推力モードが変わった (heading $hd0 -> $hd1, モード $tm0 -> $tm1)"
else
  fail "仮想ジョイスティック (初期化: ${js}旋回 '$turn' heading '$hd0' -> '$hd1', モード '$tm0' -> '$tm1')"
fi
"$ROOT/scripts/headless.sh" shot "$OUT/bridge-2.png" >/dev/null

if alive "$XSW_PID"; then pass "client が動き続けている"; else fail "client が終了した"; fi
kill "$XSW_PID" "$MON_PID" 2>/dev/null; pause 2

# 起動時のロゴの音（xsw_logo01.wav, 約 2.6 秒）が出力の先頭付近にあるか
# （16bit ステレオ 44.1kHz。振幅 200 を超える 10ms の区間を数える）
loud=$(python3 - "$OUT/client-sound.raw" <<'PY'
import sys, array
a = array.array('h')
try:
    a.frombytes(open(sys.argv[1], 'rb').read(44100 * 4 * 5))
except OSError:
    pass
win = 441 * 2
print(sum(1 for i in range(0, len(a), win) if max((abs(x) for x in a[i:i + win]), default=0) > 200))
PY
)
if [ "${loud:-0}" -ge 100 ]; then
  pass "SDL2_mixer で効果音が出力された (起動 5 秒間に音のある 10ms 区間が ${loud} 個)"
else
  fail "SDL2_mixer の効果音が出力されていない (音のある 10ms 区間 ${loud:-0} 個)"
fi

# ------------------------------------------------------------------
# server を終了させ、保存されたユニバースを確認する
kill "$SRV_PID" 2>/dev/null
for _ in $(seq 50); do alive "$SRV_PID" || break; pause 0.2; done
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
# unvedit: 開いて保存し直す (物体を選択していないので内容は変わらないはず)
cp "$SRV/db/generic_in.unv" "$OUT/unvedit-orig.unv"
cp "$OUT/unvedit-orig.unv" "$OUT/unvedit-test.unv"
touch -d '2000-01-01' "$OUT/unvedit-test.unv"
HOME="$HOMEDIR" "$ROOT/$UE_BIN" "$OUT/unvedit-test.unv" > "$OUT/unvedit.log" 2>&1 &
UE_PID=$!; PIDS+=("$UE_PID")
pause 5
"$ROOT/scripts/headless.sh" shot "$OUT/unvedit-1.png" >/dev/null
xdotool mousemove 18 15 click 1; pause 1.5     # File メニュー
xdotool mousemove 20 154 click 1; pause 3      # Save
if [ "$(stat -c %Y "$OUT/unvedit-test.unv")" -gt 946684800 ]; then
  if cmp -s "$OUT/unvedit-orig.unv" "$OUT/unvedit-test.unv"; then
    pass "unvedit で保存し直したファイルが元と同一 (EngineState = -1 を含む)"
  else
    fail "unvedit で保存し直したファイルが元と違う (diff $OUT/unvedit-orig.unv $OUT/unvedit-test.unv)"
  fi
else
  fail "unvedit で保存されなかった"
fi
alive "$UE_PID" && pass "unvedit が動き続けている" || fail "unvedit が終了した"

echo
echo "screenshots/logs: $OUT"
[ "$fails" -eq 0 ] && echo "ALL PASSED" || echo "$fails FAILED"
[ "$fails" -eq 0 ]
