# XShipWars 現代 Linux / FreeBSD / macOS 移植

## ゴール
- 最新の Linux (Debian trixie / gcc 14, 64bit) と FreeBSD (64bit) で client / server / monitor / unvedit をビルド・起動できる状態にする
  （FreeBSD はこの Dev Container では検証できない。「サポート対象プラットフォーム」を参照）
- macOS（Apple Silicon）でも client / server / monitor / unvedit をビルド・起動できるようにする。
  GUI は X11 ではなく SDL2 で描く（「macOS 対応」を参照）
- ゲームの挙動・ネットワークプロトコル・データファイル形式は変えない。最小限の修正で動かす

## 環境
- Linux 向けの作業は Dev Container (`.devcontainer/`) 内で行う。`$DEVCONTAINER` が未設定ならホスト（macOS）上にいる
- ホスト（macOS）では macOS 向けのビルドと実行だけを行う。Linux 向けのビルド・`smoke.sh` などのスクリプトはコンテナで実行する
  - ホストのツール: Xcode（clang++）、Homebrew（`/opt/homebrew`）、XQuartz（`/opt/X11`）
  - Homebrew でパッケージを追加したいときは、直接インストールせずにユーザーに依頼する
  - ホストとコンテナは同じ作業ツリーを共有している。オブジェクトファイルや実行ファイルが混ざらないように、
    もう一方でビルドする前に `make clean` する（ビルド先の分離は「macOS 対応」の段階 1 で整える）
- コンテナは Apple Silicon ホスト上の **arm64 Linux**。arm64 Linux では `char` が unsigned がデフォルトなので、
  `char` を signed 前提で扱うコード（負値比較・-1 との比較・符号拡張）に注意する
  （macOS の arm64 では `char` は signed。Linux と macOS で結果が変わるコードは、どちらでも同じになるように直す）
- コンテナはファイアウォールで外部通信が制限されている。apt でパッケージを追加したいときは直接インストールせず、
  `.devcontainer/Dockerfile` に追記してユーザーにリビルドを依頼する
- ビルド: `scripts/build.sh <server|client|monitor|unvedit|all>` (ログは `build-logs/`。コンテナ専用。macOS 用は段階 1 で用意する)
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
- 対象は **Linux**・**FreeBSD**・**macOS** のみ
  - Linux: 主な対象。Dev Container（Debian trixie, arm64）でビルド・動作を確認している
  - FreeBSD: 対象とするが、この Dev Container では検証できない。`src/*/Makefile.FreeBSD` は Linux と同じ設定に
    そろえてある（コンパイラは `${CXX}`、X11 は `${LOCALBASE}`、PREFIX は Linux と同じ /usr）。Linux 側を直したら
    FreeBSD 側にも反映し、GNU make で `-f Makefile.FreeBSD` を使って Linux 上でビルドが通ることを確かめる
    （これだけでは `__linux__` が定義されたままなので、Linux 以外の分岐は `make clean` のあと
    `scripts/build.sh all CPP="g++ -U__linux__"` でもビルドして確かめ、終わったら clean して通常どおりビルドし直す）
  - macOS: Apple Silicon のホストでビルド・動作を確認する。Intel Mac は未検証。
    4 つのプログラムすべてを対象にする（client の「server を起動する」機能も含めて、Mac 1 台で遊べるようにするため）
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
- サウンドは SDL2_mixer で置き換えた（client の `HAVE_SDL_MIXER`、設定の `SoundServerType = 4`）。背景音楽（MIDI）は
  FluidSynth と SoundFont（Debian では `libfluidsynth3`・`timgm6mb-soundfont`）で鳴らす。
  YIFF / ESD のコードは削除済み。ジョイスティックは
  libjsw の代わりに SDL2 で読む（client の `JS_SUPPORT`、`jsw-sdl.cpp`。実機では未確認）
- `-fpermissive`・`-w`・警告の一括抑止でごまかさない（clang で新たに出る警告も同じ）。
  例外: macOS の SDK が sprintf/vsprintf を非推奨にしている警告だけは、`Makefile.Darwin` の `-Wno-deprecated-declarations` で抑止する
- 修正は小さな単位でコミットし、メッセージに「現代の環境で何が壊れていたか」を書く

## macOS 対応
- GUI は OSW 層（`src/global/osw-x.cpp`）の SDL2 版（`osw-sdl.cpp`）を新しく作って描く。Cocoa や XQuartz は使わない
  - SDL2 版はどの OS でもビルドできるように作り、コンテナの Linux（Xvfb）でも動作を確認する。
    macOS では SDL2 版だけを使う
  - Linux / FreeBSD の既定は X11 版のままにする。X11 版の動作は変えない
  - ゲーム本体と widgets は XEvent のフィールド・イベント種別・`XK_*`・`Button1` などを直接使っているので、
    SDL2 版では X と同じ形の型と定数を自前のヘッダで用意し、SDL のイベントをそれに変換する（呼び出し側は変えない）
  - 設定ファイルの `BeginKeyMap` は X のキーコードの番号をそのまま書く形式なので、SDL のスキャンコードは
    evdev の番号に変換して渡す（設定ファイルの形式と同梱の値は変えない）
  - X のコアフォント（`7x14`・`6x10`）の代わりに、misc-fixed のビットマップフォントを組み込んで描く（文字の幅と配置を変えないため）
- ソースの分岐は `__APPLE__` で足す。Linux（と FreeBSD）側の処理は変えない
- 進める順番:
  1. ホストで XQuartz の Xlib を使って 4 つをビルドし、clang や libc の違い・`__APPLE__` の分岐を片付ける
     （server はこの段階で完成させる）。macOS 用の Makefile とビルド用スクリプト、ホストとコンテナのビルド先の分離もここで整える
  2. widgets などが Xlib を直接呼んでいる箇所（主に `widgets/wutils.cpp`・`wlist.cpp`）を OSW 層の関数に移す。
     コンテナで `smoke.sh` を流して、Linux の動作が変わらないことを確かめる
  3. `osw-sdl.cpp` を作る。コンテナの Linux で client → monitor → unvedit の順に動かしてから、macOS でビルドする
  4. 配布の形（`.app` にするか、データの置き場所）、`install-data.sh` の対応、ドキュメント、macOS 用のスモークテスト
- まだ決めていないこと: 配布の形とデータの置き場所（macOS では `/usr/share` に書き込めない）、
  server のプラグインの拡張子（`.so` / `.dylib`）、SoundFont の置き場所

## 記録
- 移植中に判明した非自明な事項（プロトコル上の型のサイズ、ファイル形式の前提、無効化した機能など）は
  `docs/PORTING_NOTES.md` に追記する
- `README.md`・`CHANGELOG.md`（英語）と `README.ja.md`・`CHANGELOG.ja.md`（日本語）は、同じ内容を同時に更新する。
  英語版は日本語版を省略せずに訳す。`docs/PORTING_NOTES.md` と `CLAUDE.md` は日本語のみ
- リリースは、`CHANGELOG.md` と `CHANGELOG.ja.md` の先頭に新しい版の節（`## x.y.z`、日付なし）を足し、注釈付きタグ `vx.y.z` を付ける。
  GitHub の Release の本文は、その節の英語・日本語を続けたもの。版番号は移植版のもので、プログラムの `PROG_VERSION`（1.34.0）は変えない
