# 変更履歴 (Changelog)

[English](CHANGELOG.md) | 日本語

このプロジェクトの変更履歴を記録します。[Keep a Changelog](https://keepachangelog.com/ja/1.0.0/) の形式に準拠します。
版番号はこの移植版のものです。ゲーム本体の版（プログラムが表示する版、設定ファイル・宇宙ファイルに書かれる版）は
元の XShipWars 1.34.0 のままです。

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
