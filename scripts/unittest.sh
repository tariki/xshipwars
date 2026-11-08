#!/bin/bash
# 単体テスト
#
#   scripts/unittest.sh [テスト名 ...]      (既定: tests/unit/*_test.cpp のすべて)
#
# tests/unit/<名前>_test.cpp を 1 つずつビルドして実行し、PASS/FAIL を表示する。FAIL があれば終了コード 1。
# Linux・FreeBSD・macOS ではそのままビルドする。Windows では MSYS2 の UCRT64 環境で実行し、
# src/compat/win32 の互換層を Makefile.Windows と同じようにリンクする。
#
# テストファイルの先頭のコメントで、次のものを指定する（パスは src/ からの相対）:
#   // unittest-sources: global/disk.cpp global/string.cpp ...
#       一緒にビルドするソース
#   // unittest-extract: widgets/wfbrowser.cpp 関数名 ...
#       ファイル全体はリンクしにくいソース（GUI に依存するものなど）から、名前の関数だけを取り出す。
#       取り出したものは extract.inc になり、テストで #include "extract.inc" する。
#       関数は「行頭から始まる、名前を含む行」から「行頭の }」まで。static は付けない（未使用の警告になるため）
#
# 生成物は build-tests/<OS>/ に作る。各テストは、作業用のディレクトリを引数に受け取る。
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/src"
TESTS="$ROOT/tests/unit"

case "$(uname -s)" in
  MINGW*|MSYS*)
    if [ "${MSYSTEM:-}" != "UCRT64" ]; then
      echo "ERROR: Windows では MSYS2 の UCRT64 環境で実行してください。" >&2
      exit 2
    fi
    OS=windows
    CXX="${CXX:-g++}"
    C="$SRC/compat/win32"
    PLATFORM_FLAGS=(-I"$C" -include compat.h)
    PLATFORM_SOURCES=("$C/compat.cpp")
    PLATFORM_LIBS=(-lws2_32 -lwinmm)
    ;;
  Linux)   OS=linux;   CXX="${CXX:-g++}"; PLATFORM_FLAGS=(); PLATFORM_SOURCES=(); PLATFORM_LIBS=() ;;
  FreeBSD) OS=freebsd; CXX="${CXX:-c++}"; PLATFORM_FLAGS=(); PLATFORM_SOURCES=(); PLATFORM_LIBS=() ;;
  Darwin)  OS=darwin;  CXX="${CXX:-c++}"; PLATFORM_FLAGS=(-Wno-deprecated-declarations); PLATFORM_SOURCES=(); PLATFORM_LIBS=() ;;
  *) echo "ERROR: 対応していない OS です: $(uname -s)" >&2; exit 2 ;;
esac
OUT="$ROOT/build-tests/$OS"

if [ $# -gt 0 ]; then
  names=("$@")
else
  names=()
  for f in "$TESTS"/*_test.cpp; do b="$(basename "$f")"; names+=("${b%_test.cpp}"); done
fi

# 行頭の「// <key>:」の値（なければ空）
directive() { sed -n "s#^// $2: *##p" "$1" | head -1; }

fails=0
for name in "${names[@]}"; do
  test_src="$TESTS/${name}_test.cpp"
  if [ ! -f "$test_src" ]; then
    echo "FAIL  $name: $test_src がありません"; fails=$((fails + 1)); continue
  fi
  dir="$OUT/$name"
  rm -rf "$dir"; mkdir -p "$dir/work"

  sources=()
  for s in $(directive "$test_src" unittest-sources); do sources+=("$SRC/$s"); done

  : > "$dir/extract.inc"
  read -r -a extract <<< "$(directive "$test_src" unittest-extract)"
  if [ ${#extract[@]} -gt 0 ]; then
    from="$SRC/${extract[0]}"
    for fn in "${extract[@]:1}"; do
      body="$(awk -v fn="$fn" '
        !inside && $0 ~ "^[A-Za-z_].*[^A-Za-z0-9_]" fn "\\(" { inside = 1 }
        inside { print }
        inside && /^}/ { exit }' "$from")"
      if [ -z "$body" ]; then
        echo "FAIL  $name: $fn が ${extract[0]} にありません"; fails=$((fails + 1)); continue 2
      fi
      printf '/* %s, from %s */\n%s\n\n' "$fn" "${extract[0]}" "$body" >> "$dir/extract.inc"
    done
  fi

  if ! "$CXX" -O1 -g -Wall "${PLATFORM_FLAGS[@]}" -I"$dir" -I"$TESTS" \
       "$test_src" "${sources[@]}" "${PLATFORM_SOURCES[@]}" \
       -o "$dir/$name" "${PLATFORM_LIBS[@]}" > "$dir/build.log" 2>&1; then
    echo "FAIL  $name: ビルドできない (log: $dir/build.log)"
    sed -n '1,20p' "$dir/build.log"
    fails=$((fails + 1)); continue
  fi
  if grep -q 'warning:' "$dir/build.log"; then
    echo "FAIL  $name: ビルドで警告が出た (log: $dir/build.log)"
    grep 'warning:' "$dir/build.log" | head -10
    fails=$((fails + 1)); continue
  fi

  if "$dir/$name" "$dir/work" > "$dir/run.log" 2>&1; then
    echo "PASS  $name ($(tail -1 "$dir/run.log"))"
  else
    echo "FAIL  $name"
    sed 's/^/      /' "$dir/run.log"
    fails=$((fails + 1))
  fi
done

echo
[ "$fails" -eq 0 ] && echo "ALL PASSED" || echo "$fails FAILED"
[ "$fails" -eq 0 ]
