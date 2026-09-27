---
name: run-headless
description: Launch the XShipWars server (swserv) and X11 client (xsw) headlessly under Xvfb in the Dev Container, connect them, and verify with screenshots and logs. Use to confirm the game actually runs, reproduce crashes, or check a rendering change.
---

# ヘッドレスでの起動確認

1. 対象がビルド済みか確認する（まだなら build-linux skill でビルドする）
2. `scripts/headless.sh start` で Xvfb :99 を起動する
3. サーバを起動する: `src/server/swserv` をフォアグラウンドオプション (`--fg`) と設定ファイルを指定してバックグラウンドで実行し、
   ログを `run-logs/server.log` に保存する。待ち受けポートが開くまで待つ（`ss -ltnp` で確認）
   - 設定ファイルやデータのパスが分からないときは、`src/server/main.cpp` の引数処理と `default.conf` を読んで判断する
4. クライアントを起動する: `DISPLAY=:99 src/client/xsw` をバックグラウンドで実行し、ログを `run-logs/client.log` に保存する
5. 数秒おきに `scripts/headless.sh shot run-logs/shot-N.png` を撮り、Read で画像を確認する。
   接続などの操作は `scripts/headless.sh key <keysym>` で送る
6. クラッシュしたら `gdb -batch -ex run -ex bt --args <cmd...>` でバックトレースを取る。
   メモリ破壊が疑われる場合は `-fsanitize=address,undefined` を付けて再ビルドして再現させる
7. 終了時はクライアント・サーバのプロセスを kill し、`scripts/headless.sh stop` を実行する
8. 結果（起動できたか、スクリーンショットの内容、エラー）をユーザーに報告する
