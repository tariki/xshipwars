#!/bin/bash
# XShipWars のデータファイル (client データ + グラフィックテーマ) をインストールする
#
#   scripts/install-data.sh [-n] [インストール先]
#
#   インストール先の既定は ${DESTDIR}${PREFIX}/share/games/xshipwars（PREFIX の既定は Linux・FreeBSD では /usr、
#   macOS では /usr/local。client の Makefile.install.UNIX の XSW_DIR と同じ。client はこれを ToplevelDir として使う)
#   -n  何をコピーするかを表示するだけで、書き込まない
#
# 配置する内容:
#   etc/              data/etc/* と、client の make install が入れる src/client/{xshipwarsrc,universes}
#   images/           data/images/* と theme/images/* を 1 つにまとめたもの
#   images/unvedit/   src/unvedit/images/* (unvedit の make install と同じ)
#   images/monitor/   src/monitor/images/* (monitor の make install と同じ)
#   sounds/           theme/sounds/*
#
# プログラム本体と server は扱わない。各 src/<component> で make -f Makefile.Linux install を使う。
# 既存のファイルは上書きし、インストール先にだけあるファイルは消さない。
# etc/xshipwarsrc（client が初回起動時に ~/.shipwars にコピーする設定ファイルのひな形）の ToplevelDir は、
# インストール先に書き換える（既定の場所以外に入れても、設定ファイルを直さずに使えるように）。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

dry=0
if [ "${1:-}" = "-n" ]; then dry=1; shift; fi
if [ "$(uname -s)" = "Darwin" ]; then def_prefix=/usr/local; else def_prefix=/usr; fi
DEST="${1:-${DESTDIR:-}${PREFIX:-$def_prefix}/share/games/xshipwars}"

# コピー元: "コピー元ディレクトリ:インストール先の相対パス"
# (コピー元ディレクトリの中身を、インストール先のディレクトリへコピーする)
SOURCES=(
  "data/etc:etc"
  "data/images:images"
  "theme/images:images"
  "src/unvedit/images:images/unvedit"
  "src/monitor/images:images/monitor"
  "theme/sounds:sounds"
)
# 個別のファイル: "コピー元ファイル:インストール先の相対パス"
FILES=(
  "src/client/xshipwarsrc:etc/xshipwarsrc"
  "src/client/universes:etc/universes"
)

for s in "${SOURCES[@]}"; do
  [ -d "$ROOT/${s%%:*}" ] || { echo "ERROR: $ROOT/${s%%:*} がありません" >&2; exit 1; }
done
for f in "${FILES[@]}"; do
  [ -f "$ROOT/${f%%:*}" ] || { echo "ERROR: $ROOT/${f%%:*} がありません" >&2; exit 1; }
done

# 同じインストール先に入る項目どうしで名前が重なっていないか確認する
# (data/images と theme/images など。黙って上書きしないようにする)
# macOS の /bin/bash は 3.2 で連想配列 (declare -A) が無いので、awk で調べる
list_targets() {  # 1 行に「インストール先の相対パス<TAB>コピー元の表示名」
  for s in "${SOURCES[@]}"; do
    src="${s%%:*}"; dst="${s#*:}"
    for e in "$ROOT/$src"/*; do printf '%s\t%s\n' "$dst/$(basename "$e")" "$src"; done
  done
  for f in "${FILES[@]}"; do printf '%s\t%s\n' "${f#*:}" "${f%%:*}"; done
}
if ! list_targets | awk -F'\t' '
  ($1 in owner) { printf "ERROR: %s が %s と %s の両方にあります\n", $1, owner[$1], $2; bad = 1 }
  { owner[$1] = $2 }
  END { exit bad }' >&2; then
  exit 1
fi

echo "インストール先: $DEST"
if [ "$dry" -eq 1 ]; then
  for s in "${SOURCES[@]}"; do
    printf '  %-22s -> %s/ (%s files)\n' "${s%%:*}/*" "${s#*:}" \
      "$(( $(find "$ROOT/${s%%:*}" \( -type f -o -type l \) | wc -l) ))"
  done
  for f in "${FILES[@]}"; do printf '  %-22s -> %s\n' "${f%%:*}" "${f#*:}"; done
  exit 0
fi

if ! mkdir -p "$DEST" 2>/dev/null || [ ! -w "$DEST" ]; then
  echo "ERROR: $DEST に書き込めません (システムに入れるときは sudo で実行する)" >&2
  exit 1
fi

for s in "${SOURCES[@]}"; do
  src="$ROOT/${s%%:*}"; dst="$DEST/${s#*:}"
  mkdir -p "$dst"
  # -R はシンボリックリンクをリンクのままコピーする (theme/sounds に相対リンクがある)
  cp -R "$src"/. "$dst"/
done
# install -D は GNU だけのもの (FreeBSD の -D はインストール先のルートの指定) なので、
# ディレクトリは mkdir -p で作る
for f in "${FILES[@]}"; do
  mkdir -p "$(dirname "$DEST/${f#*:}")"
  install -m 0644 "$ROOT/${f%%:*}" "$DEST/${f#*:}"
done
# ToplevelDir をインストール先にする（DESTDIR は付けない。sed -i は GNU と BSD で書き方が違うので使わない）
rc="$DEST/etc/xshipwarsrc"
sed "s#^ToplevelDir = .*#ToplevelDir = ${DEST#${DESTDIR:-}}#" "$rc" > "$rc.tmp" && mv "$rc.tmp" "$rc"

# ディレクトリ 0755、ファイル 0644 (シンボリックリンクは対象外)
find "$DEST" -type d -exec chmod 0755 {} +
find "$DEST" -type f -exec chmod 0644 {} +

# (BSD の wc -l は数字の前に空白を付けるので、算術展開で数字だけにする)
echo "完了: $(( $(find "$DEST" \( -type f -o -type l \) | wc -l) )) files"
echo "etc/xshipwarsrc の ToplevelDir は ${DEST#${DESTDIR:-}} にした"
