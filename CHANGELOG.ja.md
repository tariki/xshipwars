# 変更履歴 (Changelog)

[English](CHANGELOG.md) | 日本語

このプロジェクトの変更履歴を記録します。[Keep a Changelog](https://keepachangelog.com/ja/1.0.0/) の形式に準拠します。
版番号はこの移植版のものです。ゲーム本体の版（プログラムが表示する版、設定ファイル・宇宙ファイルに書かれる版）は
元の XShipWars 1.34.0 のままです。

## 0.2.0

- macOS（Apple Silicon）に対応。server・client・monitor・unvedit が Mac でそのまま動く
  - client・monitor・unvedit は SDL2 で表示する（XQuartz は不要）。X11 の部品（ウィンドウの階層、イベント、描画、フォント、キー）を
    SDL2 で再現する GUI の層を新しく作った。画面とキー割り当ては X11 版と同じ（Retina では整数倍で拡大して表示）
  - ビルドは `Makefile.Darwin`（`scripts/build.sh` も macOS で使える）。インストール先は `/usr/local`
    （プログラムは `/usr/local/bin`、データは `/usr/local/share/games/xshipwars`、server は `/usr/local/swserv`）
  - macOS 用のスモークテスト `scripts/smoke-macos.sh` を追加
  - Linux でも `GUI=sdl` で SDL2 版をビルドでき、`GUI=sdl scripts/smoke.sh` で確かめられる（試験用）
- 背景音楽: データの `sounds/soundfont.sf2` に SoundFont を置くと、それを使うようにした（macOS では SoundFont を探す既定の場所が無いため）
- インストール: `scripts/install-data.sh` が、設定ファイルのひな形の `ToplevelDir` をインストール先に書き換えるようにした。
  server の `make install` も `ServerToplevelDir` をインストール先に書き換える。`PREFIX` を変えてインストールしたときに、設定を手で直す必要がなくなった
- `scripts/install-data.sh` が bash 3.2（macOS の `/bin/bash`）でも動くようにした
- clang で見つかった問題を修正（`off_t` の表示の書式、使われない変数）。macOS の `df` の単位の違い（512 バイト単位）に対応
- `scripts/build.sh` で `clean all` を指定したときに、何もビルドされないことがある問題を修正

## 0.1.2

- FreeBSD で `scripts/install-data.sh` が失敗する問題を修正（GNU の `install -D` を使っていたが、FreeBSD の `install` では `-D` の意味が違う）

## 0.1.1

- FreeBSD で client のビルドが失敗する問題を修正（ジョイスティックの既定値が Linux のときしか定義されていなかった）
- Linux 以外でビルドしたときに、server とファイル選択画面で出ていた「使われていない変数」の警告を修正
- 確認の手順に `__linux__` を外したビルドを追加し、FreeBSD で通る分岐も Linux 上でコンパイルして確かめるようにした

## 0.1.0

- XShipWars 1.34.0 を現代の Linux（Debian trixie / gcc 14, 64bit）で client・server・monitor・unvedit とも
  ビルド・起動できるように移植（arm64 で確認）
  - 古い C++ の書き方と x86 専用のコンパイルフラグを修正し、gcc 14 の `-Wall` で警告 0
  - 64bit・arm64 で壊れていた箇所を修正（ポインタを int に入れる処理、char の符号に依存した処理など）
  - 一時ファイルの作成を mkstemp に置き換え、バッファあふれのおそれのある文字列処理を修正
- FreeBSD 用の Makefile を Linux と同じ設定にそろえた（FreeBSD 上では未確認）
- AIX・HP-UX・Solaris・Windows 向けのコードとビルド用ファイルを削除
- サウンドを SDL2_mixer で再生（元の YIFF / EsounD のコードは削除）
  - 効果音（同梱の設定では既定で有効）
  - 背景音楽（MIDI。FluidSynth と SoundFont が必要。既定は無効）
- ジョイスティックを SDL2 で読む（元の libjsw の代わり。実機では未確認）
- 同梱の設定ファイルのキー割り当てを、現在の X サーバ（evdev）のキーコードに更新
- 複数ディスプレイの環境で、ダイアログを本体のウィンドウがあるディスプレイに開く
- server の `make install` で plugins ディレクトリも作る
- スクリプトを追加
  - `scripts/install-data.sh`: client が読むデータ（画像・音・設定）のインストール
  - `scripts/smoke.sh`: 4 つのプログラムを Xvfb 上で起動して主要な動作を自動確認
  - `scripts/xquartz.sh`: Dev Container から macOS の XQuartz に表示
- 既知の問題は [README の「残っている課題」](README.ja.md#残っている課題)を参照
