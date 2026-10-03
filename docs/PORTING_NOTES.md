# 移植メモ

移植中に判明した非自明な事項（型のサイズ・ファイル形式の前提・無効化した機能など）を記録する。

## server ビルド (2026-09-27)

- `Makefile.Linux` の `-D__cplusplus` / `-D__USE_BSD` は削除した。`__cplusplus` は g++ が自前で定義し、
  `__USE_BSD` は glibc の内部マクロで外から定義しても無視される（g++ は `_GNU_SOURCE` を既定で有効にする）。
- `strcasestr()`: glibc が提供しており、C++ では const/非 const のオーバーロードで宣言されるため、
  独自版（`src/global/string.cpp`）と宣言（`src/include/string.h`）は `#ifndef __GLIBC__` で除外した。
  独自版は NULL 引数で NULL を返したが glibc 版は NULL を渡すとクラッシュする。server は呼んでいない。
  unvedit の呼び出し元（`uewpropsio.cpp`, `wepw.cpp`）は移植時に NULL が来ないか確認すること。
- `src/include/xsw_ctype.h` の `bool isblank(int)` 宣言は標準の `int isblank(int)` と衝突し、しかも定義が
  どこにも無かったので削除した。`global/ctype.cpp` が実際に定義している `isblankChar` / `isblankInt` を宣言する。
- `CmdSysparm()` は `const char *arg` に `'\0'` を書き込んでいた（呼び出し元のローカルバッファなので実害は無し）。
  書き込まずに `=` の前までをコピーするよう変更。
- `Makefile.Linux` の `clean` は `swserv` を消さないので、`clean all` だとリンクが走らないことがある
  （元の Makefile の挙動。完全に作り直すときは `swserv` も消す）。

## client ビルド (2026-10-03)

- `Makefile.Linux` から `-DJS_SUPPORT -DHAVE_YIFF -DHAVE_ESD` と `-ljsw -lY2 -lesd -laudiofile` を外し、
  **ジョイスティックとサウンドは無効**にした（代替実装は別フェーズ）。`-DUSE_XSHM`（MIT-SHM）は残している。
  `-L/usr/X11R6/lib` も削除（現代の Debian では X11 のライブラリは標準のパスにある）。
- 64bit でエラーになったポインタ→32bit 整数のキャストは次の 2 種類だった。どちらもプロトコルやファイル形式には関係しない。
  - デバッグ出力でポインタを `0x%.8x` と `(u_int32_t)ptr` で表示していた箇所（widgets/wfile, wutils, wtogglebtn,
    wglobal, global/osw-x, client/images）→ `%p` にした。
  - `keymapwin.cpp` は keycode（`keycode_t` = `unsigned int`）の値を CList の項目データ（`void *`）に入れて往復させている。
    `uintptr_t` を経由するようにした（値は失われない）。
- `global/osw-x.cpp` の `keytable` は「キーシンボル名, `(char *)'文字'`」を交互に並べた `char *` 配列だった。
  文字をポインタに詰め込むのをやめ、`{名前, 文字}` の構造体の配列にした（`OSWGetASCIIFromKeyCode()` の挙動は同じ）。
- 残っている警告で、あとで確認したいもの: `-Wsequence-point`（blitfade.cpp の `tar_buf_ptr`、disk.cpp:926、
  global/string.cpp:346,357）。これらは未定義動作の可能性がある。

## client のヘッドレス起動 (2026-10-03)

- client は起動時に X のコアフォント `7x14` と `6x10` を必須としていて、無いと exit 1 で終了する（クラッシュではない）。
  Debian では `xfonts-base` に入っているので Dockerfile に追加した。リビルド前の確認では、Xvfb に
  `-fp built-ins,<dir>` を付け、fonts.alias で `7x14`/`6x10` を組み込みの `fixed` に割り当てて代用した。
- client は toplevel ディレクトリ（既定は `/usr/share/games/xshipwars`）の下に `etc/`・`images/`・`sounds/` があることを前提にしている。
  リポジトリでは画像が `data/images`（client, effects, *.page）と `theme/images`（celestial, vessels など）に
  分かれているので、両方を 1 つの `images/` にまとめる必要がある。ヘッドレス確認では `run-logs/xsw-root/` に
  シンボリックリンクで組み立て、`HOME` を `run-logs/xsw-home/` にして `.shipwars/xshipwarsrc` の `ToplevelDir` で指した。
- `data/etc/xshipwarsrc` は 1.33 形式で、`JSCalibrationFile` が未知のパラメータとして警告される。
  また `UniverseListFile = .shipwars/universes` は `~/.shipwars/` からの相対パスとして解釈されるので、
  `~/.shipwars/.shipwars/universes` を探してしまう（`universes` とだけ書けば読まれる）。
