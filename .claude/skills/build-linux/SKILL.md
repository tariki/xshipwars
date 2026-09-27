---
name: build-linux
description: Build XShipWars components (server/client/monitor/unvedit) in the Dev Container and triage compile/link errors. Use whenever compiling, fixing build errors, or checking whether a change still builds.
---

# XShipWars のビルドとエラー対応

1. `$DEVCONTAINER` が設定されていることを確認する（未設定ならホスト上にいるので中止してユーザーに伝える）
2. `scripts/build.sh <component>` を実行する。最初からやり直すときは `scripts/build.sh <component> clean all`
3. `build-logs/<component>.log` からエラーを種類別に集計する:
   - ヘッダが無い / 古いヘッダ（`iostream.h`、`sys/soundcard.h`、Y2、esd など）
   - 古い C++ 構文（暗黙の int、const 違反、for スコープ、`friend` 宣言など）
   - 型 / 64bit（int とポインタの相互変換、`long` をワイヤ形式として使っている箇所）
   - リンク（`-lcrypt` などライブラリ名の変化、未使用の `-lY2 -lesd -ljsw`）
4. 同じパターンのエラーはまとめて直す。1 ファイルごとに再ビルドしない
5. 再ビルドして、エラー数と警告数の推移をユーザーに報告する（警告が増えていたら理由を説明する）
6. 64bit/プロトコル/ファイル形式に関わる判断をした場合は `docs/PORTING_NOTES.md` に追記する
