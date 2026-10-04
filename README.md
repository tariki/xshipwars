# XShipWars — 現代の Linux 向け移植版

XShipWars は、WolfPack Entertainment が 1999〜2001 年に開発した、X Window System 用のネットワーク対戦型
宇宙船ゲームです（このリポジトリの元は バージョン 1.34.0）。このリポジトリは、それを最新の Linux
（Debian trixie / gcc 14 / 64bit）でビルド・実行できるようにしたものです。
ゲームの挙動・ネットワークプロトコル・データファイル形式は変えていません。

| プログラム | 内容 |
|---|---|
| `swserv` | サーバ。宇宙（ユニバース）を管理し、クライアントの接続を受け付ける（既定でポート 1701、監視用 1702） |
| `xsw` | クライアント。サーバに接続して宇宙船を操縦する X11 アプリケーション |
| `monitor` | サーバの状態（接続数・オブジェクト数・ログ）を表示する監視ツール |
| `unvedit` | ユニバースファイル（`.unv`）のエディタ |

動作を確認した環境は、Dev Container 内の Debian trixie（arm64）です。表示先は Xvfb と macOS の XQuartz で確かめています。
x86_64 では試していませんが、64bit 環境向けの修正（固定幅型、char の符号など）は両方に共通です。

---

## とりあえず遊ぶ

### 必要なもの

Debian / Ubuntu 系なら、次のパッケージを入れます。

```sh
sudo apt install build-essential libx11-dev libxext-dev libxpm-dev libxinerama-dev xfonts-base
```

`xfonts-base` は実行時に必要です。client は X のコアフォント `7x14` と `6x10` が無いと起動しません。

### ビルド

```sh
make -C src/server  -f Makefile.Linux
make -C src/client  -f Makefile.Linux
make -C src/monitor -f Makefile.Linux
make -C src/unvedit -f Makefile.Linux
```

できあがるのは `src/server/swserv`、`src/client/xsw`、`src/monitor/monitor`、`src/unvedit/unvedit` です。
Dev Container の中では `scripts/build.sh all` でも同じことができます（ログが `build-logs/` に残ります）。

### インストール（システム全体）

```sh
sudo make -C src/client  -f Makefile.Linux install   # /usr/games/xsw と設定ファイルのひな形
sudo make -C src/monitor -f Makefile.Linux install   # /usr/games/monitor
sudo make -C src/unvedit -f Makefile.Linux install   # /usr/games/unvedit
sudo make -C src/server  -f Makefile.Linux install   # /home/swserv/ 以下
sudo scripts/install-data.sh                          # 画像・音・設定 → /usr/share/games/xshipwars
```

- `scripts/install-data.sh` は、`data/`（client データ）と `theme/`（画像・音）を、client が読む配置
  （`/usr/share/games/xshipwars/{etc,images,sounds}`）にまとめてコピーします。`-n` を付けると、配置内容を表示するだけです。
- server は、所有者を server を動かすユーザーにしておきます: `sudo chown -R $USER /home/swserv`
- server の `make install` は `cp -i` でコピーするので、再インストールのときは既存ファイルの上書きを確認されます。

### サーバを起動する

```sh
cd /home/swserv
bin/swserv --fg etc/default.conf
```

- `--fg` を付けるとフォアグラウンドで動きます。付けないと、バックグラウンドで動くデーモンになります。
- 止めるときは Ctrl-C（または SIGTERM）を送ります。宇宙を `db/generic_out.unv` に保存してから終了します。
- 起動時に読むのは `db/generic_in.unv` です。前回の状態（作ったアカウントを含む）から続けるときは、起動前に
  `mv db/generic_out.unv db/generic_in.unv` で入れ替えます。入れ替えないと、前回の変更は失われます
  （同梱の `restart` は、これを行う csh のサンプルスクリプトです）。
- ログは `logs/generic.log` に出ます。設定は `etc/default.conf` で、ポート番号もここで変えられます。

### クライアントで接続する

```sh
/usr/games/xsw                                      # 宇宙の一覧から選んでダブルクリックで接続
/usr/games/xsw swserv://Guest:guest@localhost:1701  # URL を指定してすぐ接続
```

- 初回起動時に、`/usr/share/games/xshipwars/etc/xshipwarsrc` が `~/.shipwars/xshipwarsrc` にコピーされ、
  以後はそれが使われます。設定は、client を正規に終了したとき（bridge ウィンドウを閉じる、または x キーで出る
  「Exit?:」に y と答える）に保存されます。
- 接続には、名前とパスワードが必要です（次の「ログインとアカウント」を参照）。
- ブリッジ画面を右クリックすると、クイックメニューが開きます（接続、Refresh、Star Chart、Economy、Options など）。

主なキー操作（既定の割り当て）:

| キー | 操作 | キー | 操作 |
|---|---|---|---|
| ← / → | 旋回 | space | 武器の発射 |
| ↑ / ↓ | 推力の増減 | Tab | 武器のロック |
| Delete | 推力をアイドルに | s | シールド |
| F8 / Shift+F8 | 推力モードの切り替え | ` | エンジンのオン・オフ |
| = / - | ビュースクリーンのズーム | F1 / x | ヘルプ / 終了（確認あり） |

**キーが効かないとき**: 設定ファイルにはキーが X のキーコード（番号）のまま保存されていて、この番号は
X サーバによって違います（同梱の値は Xorg の evdev 方式の番号です。macOS の XQuartz ではすべて違います）。
その場合は、右クリック → Options... → General タブの「Map Keyboard」で Key Mappings ウィンドウを開き、
「Default All」→「OK」を押してください（Default All だけでは反映されません）。
古い版で作られた `~/.shipwars/xshipwarsrc` が残っている場合も、同じ操作で直ります。

### ログインとアカウント

アカウントにあたるのは、宇宙（ユニバースファイル）の中にある「プレイヤー」型の宇宙船です。船ごとに名前と
パスワード（crypt で暗号化したもの）が保存されていて、ログインは「どの船を操縦するか」を名前とパスワードで
指定する操作です。server が受け付けるログインは 2 種類あります。

| 種類 | 名前 | パスワード | 操縦する船 |
|---|---|---|---|
| ゲスト | `Guest`（大文字小文字は区別しない） | **照合しない**（何を入れても通る） | ログインのたびに「Guest 1」「Guest 2」…が新しく作られ、切断すると消える。同時に最大 5 人 |
| 登録プレイヤー | 宇宙にあるプレイヤー船の名前 | その船のパスワードで照合する | 既存の船。切断しても残り、次回も同じ船に乗れる |

- URL の `swserv://Guest:guest@...` の `guest` は、パスワードの欄を埋めているだけで意味はありません。
  ゲストの受け付けと人数は、server の `etc/default.conf` の `AllowGuestLogins`・`GuestLoginName`・`MaxGuests` で変えられます。
- 同梱の宇宙には、登録プレイヤーの船が 2 隻あります: **Defiant** と **IKS Dark Vixen**。どちらもパスワードの欄が `*` で、
  これは「どんなパスワードでも通す」特別な値です。どちらも管理者権限（アクセスレベル 0）を持っています。
- 名前やパスワードが違うと、client に「Either that player does not exist, or has a different password.」と表示されます。

**自分のアカウントを作る**: server のコマンド `createplayer <名前>=<パスワード>` を使います。
アクセスレベル 1 以上（数字が小さいほど強い権限）が必要なので、ゲストでは使えません。

1. 管理者権限のある船でログインする: `xsw swserv://Defiant:x@localhost:1701`（パスワードは何でもよい）
2. `e` キーを押すと、ビュースクリーンの下に「Server Command:」の入力欄が出るので、`createplayer myname=mypass` と入力して Enter
   （メッセージ欄に「Created new player myname(#番号P)」と出る）
3. 以後は `xsw swserv://myname:mypass@localhost:1701` でログインする（作られた船のアクセスレベルは 5）

作ったアカウントは、server を止めたときに `db/generic_out.unv` に保存されます。次に起動する前に
`generic_in.unv` へ入れ替えないと、アカウントは消えます（上の「サーバを起動する」を参照）。

**client での指定のしかた**: URL（`swserv://名前:パスワード@ホスト:ポート`）で指定するか、引数なしで起動して
宇宙の一覧から選びます。一覧の各項目は `~/.shipwars/universes` に URL として保存されていて、同梱分には
「Localhost: Guest」（`Guest:guest`）、「Localhost: Defiant」（`Defiant:yiffbaby`）などがあります。

### monitor と unvedit

```sh
/usr/games/monitor -u <名前> <パスワード> localhost 1702   # -u はアドレスより前に書く
/usr/games/unvedit /home/swserv/db/generic_in.unv
```

- monitor は、引数の順番に意味があります。名前とパスワードは client と同じ登録プレイヤーのもので、
  同梱の宇宙なら `-u Defiant x` で入れます。`-u` を省くと Guest でつなぎますが、その場合は統計だけが表示され、
  サーバのログ（Messages ウィンドウ）は表示されません。画像の場所を変えたときは `-i <画像ディレクトリ>` を付けます。
- unvedit の設定は `~/.shipwars/unveditrc` です。

### root を使わずにホームディレクトリへ入れる

インストール先を `PREFIX` で変えられます。その場合、client の設定ファイルは自分で用意します
（client は初回起動時に、既定の場所 `/usr/share/games/xshipwars/etc` からしか設定ファイルをコピーしないためです）。

```sh
P=$HOME/xshipwars
for c in client monitor unvedit server; do make -C src/$c -f Makefile.Linux install PREFIX=$P; done
PREFIX=$P scripts/install-data.sh

# server: ServerToplevelDir をインストール先に合わせる
sed -i "s#^ServerToplevelDir = .*#ServerToplevelDir = $P/swserv#" $P/swserv/etc/default.conf

# client: ToplevelDir をデータの場所に合わせた設定ファイルを置く
mkdir -p ~/.shipwars
sed "s#^ToplevelDir = .*#ToplevelDir = $P/share/games/xshipwars#" \
    $P/share/games/xshipwars/etc/xshipwarsrc > ~/.shipwars/xshipwarsrc
cp $P/share/games/xshipwars/etc/universes ~/.shipwars/

(cd $P/swserv && bin/swserv --fg etc/default.conf) &
$P/games/xsw swserv://Guest:guest@localhost:1701
```

### macOS（Dev Container ＋ XQuartz）で遊ぶ

Dev Container の中で server を動かし、画面を macOS の XQuartz に表示できます。

1. XQuartz の 設定 → セキュリティ →「ネットワーク・クライアントからの接続を許可」をオンにして、XQuartz を再起動する
2. XQuartz の xterm で `xhost +localhost` を実行する
3. コンテナで `scripts/build.sh all` のあと `scripts/xquartz.sh`（monitor・unvedit も出すなら `scripts/xquartz.sh client monitor unvedit`）
4. 初回だけ、上の「キーが効かないとき」の手順でキーを割り当て直し、bridge ウィンドウを閉じて保存する

実行環境は `run-logs/xquartz/` に作られ、キーの割り当てと宇宙の状態は次回に引き継がれます。

### 既知の制限

- **サウンドとジョイスティックは無効**です（元のライブラリ YIFF / ESD / libjsw が現在は無いため）。
  設定ファイルの `JSCalibrationFile` が「Unknown parameter」と警告されるのは、このためで害はありません。
- **キー割り当ては X サーバに依存**します（上記）。
- 表示が遅い環境（ネットワーク越しの XQuartz など）では、ログイン直後に物体の名前が届かず、"Object 3" のように
  表示されることがあります。クイックメニューの「Refresh」で直ります。
- 複数ディスプレイでは、ダイアログは bridge ウィンドウ（monitor・unvedit では最初のウィンドウ）のあるディスプレイに開きます。

---

## 開発者向け

### 開発環境（Dev Container）

`.devcontainer/` に、Debian trixie ベースの Dev Container があります（VS Code の Dev Containers などで開きます）。

- ビルドとデバッグの道具（gcc 14, gdb, valgrind, strace）、X11 の開発ライブラリ、Xvfb・xdotool・ImageMagick が入っています。
- 起動時に `init-firewall.sh` が外部への通信を制限します（GitHub、npm、Anthropic API など一部と、
  macOS ホストの XQuartz 用 `host.docker.internal:6000` だけを許可）。コンテナ内から apt は使えないので、
  パッケージを足すときは `.devcontainer/Dockerfile` に書いてリビルドします。
- Apple Silicon ホスト上では **arm64** で動きます。arm64 では `char` が符号なしなので注意してください（下記）。

### ディレクトリ構成

| パス | 内容 |
|---|---|
| `src/server/`, `src/client/`, `src/monitor/`, `src/unvedit/` | 各プログラム。`Makefile.Linux` でビルドする |
| `src/include/`, `src/global/`, `src/widgets/` | 共通のヘッダ・ユーティリティ・独自 GUI ウィジェット。各プログラムの `.cpp` の多くはここへのシンボリックリンク |
| `src/pconf/`, `src/*/platforms.ini` | 元の Makefile 生成ツール（現在は使っていない） |
| `data/` | client データ（`etc/` のページ・OCS 名など、`images/` の client 用画像） |
| `theme/` | グラフィックテーマ（`images/` の天体・船など、`sounds/`） |
| `scripts/` | ビルド・実行・テスト用のスクリプト（下記） |
| `docs/PORTING_NOTES.md` | 移植の記録。何が壊れていて、どう直し、何を確かめたか |
| `CLAUDE.md` | 移植作業の方針（AI エージェント向けの指示を兼ねる） |
| `build-logs/`, `run-logs/` | ビルドログ・実行時の作業ディレクトリ（git 管理外） |

### ビルドシステム

- 各プログラムの `src/<component>/Makefile.Linux` を直接使います。元の `pconf` による生成や CMake 化は、まだ手を付けていません。
- コンパイルオプションは `-O2 -g -Wall` が基本です。gcc 14 の `-Wall` で、4 つとも**警告 0** です。
  `-Wno-*`・`-fpermissive`・`-w` による一括抑止は使っていません。
- 主な define:
  - `USE_XSHM`: MIT-SHM で描画する。ネットワーク越しの X サーバでは実行時に `--no_xshm` を付けて無効にする
  - `HAVE_XINERAMA`: 複数ディスプレイを考慮してウィンドウを配置する（`-lXinerama`）
  - `PLUGIN_SUPPORT`（server）
  - `JS_SUPPORT`・`HAVE_YIFF`・`HAVE_ESD` は外してある
- **Makefile はヘッダの依存関係を追跡しません。** ヘッダを変えたら `make -f Makefile.Linux clean` してからビルドしてください。

### スクリプト

| スクリプト | 用途 |
|---|---|
| `scripts/build.sh <server\|client\|monitor\|unvedit\|all> [make の引数]` | ビルド（Dev Container 内専用）。ログは `build-logs/<component>.log`、最後にエラー数・警告数を表示 |
| `scripts/smoke.sh` | スモークテスト。下記 |
| `scripts/headless.sh start\|shot <png>\|key <keysym>\|stop` | Xvfb（:99）の起動、スクリーンショット、キー入力、停止 |
| `scripts/install-data.sh [-n] [インストール先]` | データのインストール |
| `scripts/xquartz.sh [client] [monitor] [unvedit]` | macOS ホストの XQuartz に表示 |

### テスト

`scripts/smoke.sh` は、実行環境を `run-logs/smoke/` に毎回作り直して、Xvfb 上で次を自動判定します
（PASS/FAIL を表示し、FAIL があれば終了コード 1）。変更したら実行してください。

- `install-data.sh` でデータをインストールできる
- server が 1701/1702 で待ち受け、正常終了する
- monitor が AUX ポートにログインして統計を受け取る
- client が Guest でログインし、旋回・加速・推力モードの逆回し（Normal → Incremental の折り返し）が効く
  （client の内部の値を gdb で読んで判定する）
- `EngineState = -1` の物体が、server と unvedit の保存を通っても -1 のまま（arm64 の char 符号の回帰テスト）
- unvedit で開いて保存し直したファイルが、元と完全に同じ

ヘッドレスで個別に調べるときのコツ:

- ウィンドウマネージャが無いので、ゲーム中のキー入力の前に `xdotool windowfocus --sync <bridge のウィンドウ>` でフォーカスを与える
- client はデバッグ情報付きなので、`gdb -batch -p <PID> -ex 'print <式>'` で内部の値を読める
- gcc の診断の列番号は、タブを 8 桁に展開した表示上の列

### 移植で気をつけていること

詳しくは `CLAUDE.md` と `docs/PORTING_NOTES.md` を参照してください。要点は次のとおりです。

- **プロトコル・ファイル形式は変えない。** client↔server と AUX の通信、ユニバース・設定ファイルは、どれもテキスト形式です。
  構造体をバイナリのまま書く箇所はありません（64bit 監査で確認済み）。
- **64bit**: long やポインタのサイズを前提にしない。ポインタを int に詰める箇所は `intptr_t` 経由にする。
- **arm64 の char**: `char` は符号なしです。負の値（-1 など）を入れる変数は `signed char` にします
  （例: `engine_state`、client の `option.throttle_mode`）。`-Wtype-limits` を付けたビルドで洗い出せます。
- 警告を黙らせるだけのキャストはしない。例外は、const に対応していない libXpm の API に渡す 1 か所だけです。
- コミットは小さな単位にし、メッセージには「現代の環境で何が壊れていたか」を書きます。非自明な発見は
  `docs/PORTING_NOTES.md` に追記します。

### 残っている課題

- サウンド・ジョイスティックの代替実装（SDL2 など）
- ビルドシステムの改修（ヘッダの依存関係、pconf の改修や CMake 化）
- キー割り当てを X サーバに依存しない形にすること（設定ファイルの形式に関わるので保留）
- ログイン直後に物体の名前を取りこぼす件の原因調査（送信キューのあふれと推測）
- server の `restart` スクリプトが csh で、`etc/generic.conf` を前提にしていること

---

## ライセンス

元の XShipWars は、GNU General Public License バージョン 2 に追加条項を付けたライセンスで配布されています
（全文は `src/LICENSE`）。追加条項は、改変したものを配布する場合に、改変内容を明示することを求めています。
このリポジトリでの改変内容は、git の履歴と `docs/PORTING_NOTES.md` に記録しています。
元の作者と貢献者は `src/CREDITS` にあります。
