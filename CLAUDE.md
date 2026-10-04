# XShipWars 現代 Linux / FreeBSD 移植

## ゴール
- 最新の Linux (Debian trixie / gcc 14, 64bit) と FreeBSD (64bit) で client / server / monitor / unvedit をビルド・起動できる状態にする
  （FreeBSD はこの Dev Container では検証できない。「サポート対象プラットフォーム」を参照）
- ゲームの挙動・ネットワークプロトコル・データファイル形式は変えない。最小限の修正で動かす

## 環境
- 作業は Dev Container (`.devcontainer/`) 内で行う。`$DEVCONTAINER` が未設定ならホスト上にいるので、ビルドや実行はしない
- コンテナは Apple Silicon ホスト上の **arm64 Linux**。arm64 では `char` が unsigned がデフォルトなので、
  `char` を signed 前提で扱うコード（負値比較・-1 との比較・符号拡張）に注意する
- ファイアウォールで外部通信は制限されている。apt でパッケージを追加したいときは直接インストールせず、
  `.devcontainer/Dockerfile` に追記してユーザーにリビルドを依頼する
- ビルド: `scripts/build.sh <server|client|monitor|unvedit|all>` (ログは `build-logs/`)
- ヘッドレス実行: `scripts/headless.sh start|shot|key|stop`（Xvfb :99）
- スモークテスト: `scripts/smoke.sh`（server/monitor/client/unvedit を起動して主要動作を自動判定。変更後の確認に使う）
- データのインストール: `scripts/install-data.sh [-n] [インストール先]`（data/ と theme/ を client が読む配置にまとめる）
- ホストの XQuartz に表示: `scripts/xquartz.sh [client] [monitor] [unvedit]`（事前準備はスクリプト冒頭のコメント参照）

## ソース構成とビルドシステム
- `src/<component>/` に各プログラム、`src/include/` と `src/global/` と `src/widgets/` が共通部分
- 元々は独自の pconf (`src/pconf`) が `platforms.ini` から Makefile を生成する方式。
  当面は `src/<component>/Makefile.Linux` を直接修正して使う。pconf の改修や CMake 化は後回し
- データファイル: `data/`（etc, images）、`theme/`（images, sounds）

## サポート対象プラットフォーム
- 対象は **Linux** と **FreeBSD** のみ
  - Linux: 主な対象。Dev Container（Debian trixie, arm64）でビルド・動作を確認している
  - FreeBSD: 対象とするが、この Dev Container では検証できない。`src/*/Makefile.FreeBSD` は Linux と同じ設定に
    そろえてある（コンパイラは `${CXX}`、X11 は `${LOCALBASE}`、PREFIX は Linux と同じ /usr）。Linux 側を直したら
    FreeBSD 側にも反映し、GNU make で `-f Makefile.FreeBSD` を使って Linux 上でビルドが通ることを確かめる
- **AIX・HP-UX・Solaris・Windows は対象外とし、ビルド環境とソースコードから削除する**
  - 理由: このプロジェクトで試せる環境が無く、2001 年以降ビルドされていないと考えられるため。
    試せないコードは修正のたびに保守の負担になる。（OS 自体の状況は理由にしない。AIX 7.3 と
    Solaris 11.4 / illumos は今もメンテナンスが続いている。HP-UX は 2025 年末でサポート終了）
  - AIX・HP-UX・Solaris の対象: `src/*/Makefile.AIX`・`Makefile.HPUX.10.20`・`Makefile.Solaris`、
    各 `platforms.ini` の該当する節、ソースの `_AIX`・`_AIX_`・`__HPUX__`・`__SOLARIS__` などの分岐
  - Windows は正式にサポートされていたわけではなく、移植しかけた痕跡が残っているだけ:
    元の README / INSTALL に記述は無く（CREDITS に移植協力者の名前があるのみ）、Windows 向けのビルド用ファイルも
    Windows の GUI 実装も無い（GUI は X11 版の `osw-x.cpp` だけ）。対象は、どのビルドにも含まれていない
    `src/global/diskw.cpp`・`dfw.cpp`・`mfw.cpp`（`printerw.cpp` は印刷用で Windows 用ではないので残す）と、
    共有のパス処理や server/client の `main.cpp` などにある `__MSW__`・`__WIN32__`・`_WIN32` の分岐（約 40 か所）
  - 削除するときは、分岐の対象外 OS 側だけを消し、Linux（と FreeBSD）側の処理は変えない。
    `defined(__linux__) || defined(__SOLARIS__)` のような組み合わせは、対象外 OS の部分だけを外す
  - Windows で遊びたい場合は、WSL2 で Linux 版を動かすことを案内する（未検証）

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
