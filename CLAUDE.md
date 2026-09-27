# XShipWars 現代 Linux 移植

## ゴール
- 最新の Linux (Debian trixie / gcc 14, 64bit) で client / server / monitor / unvedit をビルド・起動できる状態にする
- ゲームの挙動・ネットワークプロトコル・データファイル形式は変えない。最小限の修正で動かす

## 環境
- 作業は Dev Container (`.devcontainer/`) 内で行う。`$DEVCONTAINER` が未設定ならホスト上にいるので、ビルドや実行はしない
- コンテナは Apple Silicon ホスト上の **arm64 Linux**。arm64 では `char` が unsigned がデフォルトなので、
  `char` を signed 前提で扱うコード（負値比較・-1 との比較・符号拡張）に注意する
- ファイアウォールで外部通信は制限されている。apt でパッケージを追加したいときは直接インストールせず、
  `.devcontainer/Dockerfile` に追記してユーザーにリビルドを依頼する
- ビルド: `scripts/build.sh <server|client|monitor|unvedit|all>` (ログは `build-logs/`)
- ヘッドレス実行: `scripts/headless.sh start|shot|key|stop`（Xvfb :99）

## ソース構成とビルドシステム
- `src/<component>/` に各プログラム、`src/include/` と `src/global/` と `src/widgets/` が共通部分
- 元々は独自の pconf (`src/pconf`) が `platforms.ini` から Makefile を生成する方式。
  当面は `src/<component>/Makefile.Linux` を直接修正して使う。pconf の改修や CMake 化は後回し
- データファイル: `data/`（etc, images）、`theme/`（images, sounds）

## 移植方針
- 優先順: server → client → monitor → unvedit（widgetdemo は後回し）
- `-march=i586` や `-O6` など古い/x86 固有のフラグは除去し、`-O2 -g -Wall` を基本とする
- 古い C++ は標準準拠に直す（`<iostream.h>`、暗黙の int、文字列リテラルの非 const 代入など）
- **64bit 問題に特に注意**: ネットワークプロトコル・ファイルフォーマットで long/int/ポインタのサイズを
  前提にしている箇所は固定幅型（`int32_t` 等）にする。警告を黙らせるだけのキャストはしない
- サウンド (YIFF/ESD) とジョイスティック (libjsw) は当面 Makefile の define を外して無効化する。
  代替実装 (SDL2 等) は別フェーズで検討する
- `-fpermissive`・`-w`・警告の一括抑止でごまかさない
- 修正は小さな単位でコミットし、メッセージに「現代の環境で何が壊れていたか」を書く

## 記録
- 移植中に判明した非自明な事項（プロトコル上の型のサイズ、ファイル形式の前提、無効化した機能など）は
  `docs/PORTING_NOTES.md` に追記する
