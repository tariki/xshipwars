# XShipWars — 現代の Linux / FreeBSD / macOS 向け移植版

[English](README.md) | 日本語

XShipWars は、WolfPack Entertainment が 1999〜2001 年に開発した、X Window System 用のネットワーク対戦型
宇宙船ゲームです（このリポジトリの元は バージョン 1.34.0）。このリポジトリは、それを最新の Linux
（Debian trixie / gcc 14 / 64bit）と macOS でビルド・実行できるようにしたものです。
ゲームの挙動・ネットワークプロトコル・データファイル形式は変えていません。

元のソースは、Debian のスナップショット・アーカイブにある xshipwars のソースパッケージ 1.34.1-3
（https://snapshot.debian.org/package/xshipwars/1.34.1-3/ ）から入手しました。Debian のパッチは当てていません
（プログラムが表示する版は 1.34.0 です）。

| プログラム | 内容 |
|---|---|
| `swserv` | サーバ。宇宙（ユニバース）を管理し、クライアントの接続を受け付ける（既定でポート 1701、監視用 1702） |
| `xsw` | クライアント。サーバに接続して宇宙船を操縦する（Linux・FreeBSD では X11、macOS では SDL2 で表示） |
| `monitor` | サーバの状態（接続数・オブジェクト数・ログ）を表示する監視ツール |
| `unvedit` | ユニバースファイル（`.unv`）のエディタ |

動作を確認した環境は、Dev Container 内の Debian trixie（arm64）と、macOS（Apple Silicon）です。Linux の表示先は
Xvfb と macOS の XQuartz で確かめています。x86_64 では試していませんが、64bit 環境向けの修正（固定幅型、char の符号など）は
両方に共通です。

**サポート対象は Linux・FreeBSD・macOS です。** FreeBSD 用の Makefile は Linux と同じ設定にそろえてありますが、
FreeBSD の実機ではまだ試せていません。macOS では 4 つのプログラムがすべて動きます。client・monitor・unvedit は X11 ではなく
SDL2 で表示します（XQuartz は不要。X11 の部品を SDL2 で再現する層を新しく作りました）。Intel Mac では試していません。
元の配布物にあった AIX・HP-UX・Solaris 向けのビルド用ファイルとコード、それに Windows への移植の名残は削除しました
（試せる環境が無く、保守できないため）。Windows では、WSL2 で Linux 版を動かすのが現実的です（未検証）。

---

## とりあえず遊ぶ

### 必要なもの

Debian / Ubuntu 系なら、次のパッケージを入れます。

```sh
sudo apt install build-essential libx11-dev libxext-dev libxpm-dev libxinerama-dev libsdl2-mixer-dev xfonts-base \
  libfluidsynth3 timgm6mb-soundfont
```

`xfonts-base` は実行時に必要です。client は X のコアフォント `7x14` と `6x10` が無いと起動しません。
`libfluidsynth3` と `timgm6mb-soundfont` は背景音楽（MIDI）を鳴らすときだけ必要です（「サウンド」を参照）。

FreeBSD では、次のパッケージが必要なはずです（未確認）。スクリプト類（`scripts/*.sh`）を使うなら `bash` も必要です。

```sh
pkg install libX11 libXext libXpm libXinerama sdl2_mixer font-misc-misc
```

背景音楽には、さらに FluidSynth と GM の SoundFont が要ります（「サウンド」を参照。未確認）。

macOS では、Xcode Command Line Tools（`xcode-select --install`）と、Homebrew の次のパッケージを入れます。
FluidSynth は SDL2_mixer と一緒に入ります。X11 や XQuartz は要りません。

```sh
brew install sdl2 sdl2_mixer pkg-config
```

### ビルド

```sh
make -C src/server  -f Makefile.Linux
make -C src/client  -f Makefile.Linux
make -C src/monitor -f Makefile.Linux
make -C src/unvedit -f Makefile.Linux
```

できあがるのは `src/server/swserv`、`src/client/xsw`、`src/monitor/monitor`、`src/unvedit/unvedit` です。
Dev Container の中では `scripts/build.sh all` でも同じことができます（ログが `build-logs/` に残ります）。

FreeBSD では `Makefile.Linux` の代わりに `Makefile.FreeBSD` を使います（未確認）。コンパイラは `c++`（clang）、
X11 は `/usr/local` から探します。BSD の make でうまく動かない場合は、GNU make（`gmake`）で試してください。
インストール先は Linux と同じです（client などは `/usr/games`、データは `/usr/share/games/xshipwars`）。

macOS では `Makefile.Darwin` を使います（`scripts/build.sh all` でも同じです）。できあがるのは
`src/<プログラム>/build-darwin/` の下の `swserv`・`xsw`・`monitor`・`unvedit` です。

```sh
for c in server client monitor unvedit; do make -C src/$c -f Makefile.Darwin; done
```

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
  `etc/default.conf` の `ServerToplevelDir` は、インストール先に合わせて書き換えられます。

macOS では `/usr/share` と `/home` を使えないので、`/usr/local` の下に入れます（client・monitor・unvedit は
`/usr/local/bin`、データは `/usr/local/share/games/xshipwars`、server は `/usr/local/swserv`）。

```sh
for c in client monitor unvedit server; do sudo make -C src/$c -f Makefile.Darwin install; done
sudo scripts/install-data.sh            # macOS では /usr/local/share/games/xshipwars に入る
sudo chown -R $USER /usr/local/swserv
```

インストール先は、プログラムに埋め込まれます（データや server を探す既定の場所になります）。`PREFIX` を変えるときは、
ビルドとインストールの両方で同じ `PREFIX` を指定してください（変えてビルドし直すときは、先に `make -f Makefile.Darwin clean`）。

### サーバを起動する

```sh
cd /home/swserv          # macOS では /usr/local/swserv
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
/usr/games/xsw                                      # 宇宙の一覧から選んでダブルクリックで接続（macOS では /usr/local/bin/xsw）
/usr/games/xsw swserv://Guest:guest@localhost:1701  # URL を指定してすぐ接続
```

- 初回起動時に、`/usr/share/games/xshipwars/etc/xshipwarsrc`（macOS では `/usr/local/share/...`）が `~/.shipwars/xshipwarsrc` にコピーされ、
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
macOS 版（SDL2）は、キーを Xorg の evdev 方式の番号に変換して扱うので、同梱の値のまま使えます。

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

macOS では `/usr/local/bin/monitor`・`/usr/local/bin/unvedit` と `/usr/local/swserv/db/generic_in.unv` です。

- monitor は、引数の順番に意味があります。名前とパスワードは client と同じ登録プレイヤーのもので、
  同梱の宇宙なら `-u Defiant x` で入れます。`-u` を省くと Guest でつなぎますが、その場合は統計だけが表示され、
  サーバのログ（Messages ウィンドウ）は表示されません。画像の場所を変えたときは `-i <画像ディレクトリ>` を付けます。
- unvedit の設定は `~/.shipwars/unveditrc` です。

### root を使わずにホームディレクトリへ入れる

インストール先を `PREFIX` で変えられます。server の `ServerToplevelDir` と、データの中の設定ファイルのひな形の `ToplevelDir` は、
インストールのときにインストール先に書き換えられます。Linux では、client は初回起動時に既定の場所
（`/usr/share/games/xshipwars/etc`）からしか設定ファイルをコピーしないので、設定ファイルは自分でコピーします。

```sh
P=$HOME/xshipwars
for c in client monitor unvedit server; do make -C src/$c -f Makefile.Linux install PREFIX=$P; done
PREFIX=$P scripts/install-data.sh

# client: インストールした設定ファイルのひな形を置く（ToplevelDir はインストール先になっている）
mkdir -p ~/.shipwars
cp $P/share/games/xshipwars/etc/{xshipwarsrc,universes} ~/.shipwars/

(cd $P/swserv && bin/swserv --fg etc/default.conf) &
$P/games/xsw swserv://Guest:guest@localhost:1701
```

macOS では、インストール先がプログラムに埋め込まれるので、ビルドから同じ `PREFIX` を指定すれば、設定ファイルのコピーは要りません
（実行ファイルは `$P/bin` に入ります）。

```sh
P=$HOME/xshipwars
for c in client monitor unvedit server; do
  make -C src/$c -f Makefile.Darwin clean      # 別の PREFIX でビルドしたものを消す
  make -C src/$c -f Makefile.Darwin PREFIX=$P all install
done
PREFIX=$P scripts/install-data.sh
```

### macOS で遊ぶ

macOS では、上の手順（必要なもの・ビルド・インストール）で、4 つのプログラムをそのまま Mac で動かせます。
client・monitor・unvedit は SDL2 で表示するので、XQuartz は要りません。

- 画面は Linux 版と同じ大きさ・同じ見た目で描きます。Retina ディスプレイでは、ぼやけないように整数倍で拡大して表示します。
- キーの割り当ては、キーボード上の位置（US 配列の位置）で決まります。JIS 配列では、ズームの `=` など一部の記号のキーの位置が
  US 配列と違います。文字の入力（接続先の URL など）は、macOS のキーボードの設定どおりに入力できます。
- client のクイックメニューの「Run Server...」は、`/usr/local/swserv` を開きます。
- 背景音楽を鳴らすには、SoundFont を置きます（次の「サウンド」を参照）。

### サウンド

効果音と背景音楽は SDL2_mixer で鳴らします（元の YIFF / EsounD サウンドサーバは現在は無いため置き換えました）。
同梱の設定では、音は最初から有効です（`SoundServerType = 4`、`Sounds = 3`）。変えるときは次のどちらかで行います。

- client の右クリック → Options... → Sounds タブの Sound Server Type（「SDL」か「None」）と Amount level
- `~/.shipwars/xshipwarsrc` の `SoundServerType`（4: SDL、0: なし）と `Sounds`（0: なし、1: イベント、2: ＋エンジン、3: すべて。
  今は 1〜3 で鳴る音は同じ）

古い版で作られた `~/.shipwars/xshipwarsrc` が残っている場合は、音が無効のままなので、上の方法で有効にしてください。

Sounds タブの「Test Sound」で、左・右・両方の順に音が鳴ります。音声デバイスが無いなどで初期化に失敗したときは、
メッセージ（ALSA の警告が続けて出ることもあります）を出して音を無効にし、そのまま動きます。このとき、client を終了すると
`Sounds = 0` が設定ファイルに保存されるので、音声デバイスのある環境で遊ぶときは、上の方法でもう一度有効にしてください。

- **背景音楽（MIDI）は既定では無効**です。有効にするには、Options... → Sounds タブの Music をオンにするか、
  `~/.shipwars/xshipwarsrc` を `Music = on` にします。曲は状況（通常・星雲の中・戦闘）に合わせて切り替わり、
  メインメニューでは鳴りません（元のゲームと同じ）。
  - MIDI を鳴らすには、SDL2_mixer が使う FluidSynth と GM の SoundFont が必要です。Debian / Ubuntu では
    `libfluidsynth3` と `timgm6mb-soundfont`（約 6MB）を入れれば、追加の設定なしで鳴ります。
    `fluid-soundfont-gm`（約 140MB）など、`/usr/share/sounds/sf3/default-GM.sf3` に登録される別の SoundFont でもかまいません。
  - 別の場所の SoundFont を使うときは、環境変数 `SDL_SOUNDFONTS=<.sf2 のパス>` を指定します
    （上の既定の場所にも SoundFont があるときは、`SDL_FORCE_SOUNDFONTS=1` も必要です）。
  - SoundFont が見つからないと、`bluedanube.mid: Couldn't open timidity.cfg` のようなメッセージを出して、
    背景音楽なしで動きます。曲によっては FluidSynth が `Ignoring unrecognized meta event type 0x21` を
    何十行か出しますが、害はありません。
  - FreeBSD では `fluidsynth` と SoundFont（`fluid-soundfont` など）を入れ、必要なら `SDL_SOUNDFONTS` で
    場所を指定すれば鳴るはずです（未確認）。
  - データの `sounds/` に `soundfont.sf2` という名前で SoundFont を置くと、それを使います（`SDL_SOUNDFONTS` を
    指定したときは、そちらが優先されます）。**macOS では SoundFont を探す既定の場所が無い**ので、GM の SoundFont（.sf2。
    たとえば Debian の `timgm6mb-soundfont` と同じ TimGM6mb や、GeneralUser GS など）を
    `/usr/local/share/games/xshipwars/sounds/soundfont.sf2` に置いてください。SoundFont はこのリポジトリには含めていません。
- **macOS の Docker（Dev Container）の中では音を聞けません**。Docker Desktop のコンテナには音声の出力が無いので、
  XQuartz で遊んでいても音は鳴りません。音を聞くには、macOS 版を使うか、Linux や FreeBSD のデスクトップで直接動かしてください。

### ジョイスティック

ジョイスティック（ゲームパッド）は SDL2 で読みます（元のライブラリ libjsw は現在は無いため置き換えました）。
**実機では未確認**です（Dev Container には入力デバイスが無く、SDL の仮想ジョイスティックで確かめています）。

1. client の右クリック → Options... → General タブの「Map Joystick」で割り当て画面を開く
2. 「Add」でジョイスティックを追加し、Device に番号を書いて（下記）「Refresh」を押すと、軸とボタンが読み込まれる
3. 各軸に使う機能（Turn・Throttle など）の欄に軸の番号を書き、ボタンは一覧で選んで「Scan Key」で
   キーを割り当てる（ボタンを押すと、そのキーを押したことになる）
4. General タブの Controller Type を「Joystick」にする

- Device は**番号で選びます**。末尾の数字が SDL のジョイスティック番号（0 から）で、元の設定の `/dev/js0`・
  `/dev/input/js1` はそれぞれ 0 番・1 番として、そのまま使えます。
- 軸の番号は SDL の番号で、ハット（十字キー）は軸の後ろに 2 本ずつ（横、縦）続きます。
- 中央付近の遊び（不感帯）は振れ幅の 20%（libjsw の既定値）で固定です。libjsw の校正ファイル（`JSCalibrationFile`、
  `~/.joystick`）は読みません（設定の行は残してあり、保存もされます）。
- 軸の向きは反転できません。SDL では、スティックを前に倒すと負の値になるので、Throttle を割り当てた軸は
  **手前に引くと推力が上がります**（Normal モード。元は libjsw の校正で反転させる前提だったと思われます）。
- Linux では `/dev/input/event*` を読む権限が要ります（通常のデスクトップでは、ログイン中のユーザーに自動で付きます）。
  FreeBSD でも SDL が対応しているゲームパッドなら動くはずです（未確認）。
- macOS の Docker（Dev Container）の中からは、ホストの USB 機器が見えないので使えません。macOS 版では SDL2 が対応している
  ゲームパッドが使えるはずです（未確認）。

### 既知の制限

- **ジョイスティックは実機で未確認**です。Throttle の軸の向きは反転できません（上記）。
- **キー割り当ては X サーバに依存**します（上記）。
- 表示が遅い環境（ネットワーク越しの XQuartz など）では、ログイン直後に物体の名前が届かず、"Object 3" のように
  表示されることがあります。クイックメニューの「Refresh」で直ります。
- 複数ディスプレイでは、ダイアログは bridge ウィンドウ（monitor・unvedit では最初のウィンドウ）のあるディスプレイに開きます
  （X11 版のみ。macOS 版では、作られた位置に開きます）。
- macOS 版（SDL2）では、塗りつぶした円弧（スクロールバーの矢印の先など）の縁の数画素が、X11 版と少し違います。
  また、トップレベルのウィンドウどうしの重なり順の指定と、ダイアログを親ウィンドウに付ける指定（transient）は効きません。
- macOS 版では、client のオプション画面のメモリ表示が 0 になり、server のプラグインは読み込まれません（FreeBSD と同じ）。

---

## 開発者向け

### 開発環境（Dev Container）

`.devcontainer/` に、Debian trixie ベースの Dev Container があります（VS Code の Dev Containers などで開きます）。

- ビルドとデバッグの道具（gcc 14, gdb, valgrind, strace）、X11 の開発ライブラリ、Xvfb・xdotool・ImageMagick が入っています。
- 起動時に `init-firewall.sh` が外部への通信を制限します（GitHub、npm、Anthropic API など一部と、
  macOS ホストの XQuartz 用 `host.docker.internal:6000` だけを許可）。コンテナ内から apt は使えないので、
  パッケージを足すときは `.devcontainer/Dockerfile` に書いてリビルドします。
- Apple Silicon ホスト上では **arm64** で動きます。arm64 の Linux では `char` が符号なしなので注意してください（下記）。
- macOS 版は、コンテナではなく macOS（ホスト）でビルドします。ホストとコンテナは同じ作業ツリーを使いますが、macOS 版の
  生成物は `build-darwin/` に作るので、Linux 版（ソースと同じ場所）とは混ざりません。

### ディレクトリ構成

| パス | 内容 |
|---|---|
| `src/server/`, `src/client/`, `src/monitor/`, `src/unvedit/` | 各プログラム。`Makefile.Linux`・`Makefile.FreeBSD`・`Makefile.Darwin` でビルドする |
| `src/include/`, `src/global/`, `src/widgets/` | 共通のヘッダ・ユーティリティ・独自 GUI ウィジェット。各プログラムの `.cpp` の多くはここへのシンボリックリンク |
| `src/global/osw-x.cpp`, `src/global/osw-sdl.cpp` | GUI の層（OSW）の X11 版と SDL2 版。共通の宣言は `src/include/osw-api.h` |
| `src/pconf/`, `src/*/platforms.ini` | 元の Makefile 生成ツール（現在は使っていない） |
| `data/` | client データ（`etc/` のページ・OCS 名など、`images/` の client 用画像） |
| `theme/` | グラフィックテーマ（`images/` の天体・船など、`sounds/`） |
| `scripts/` | ビルド・実行・テスト用のスクリプト（下記） |
| `docs/PORTING_NOTES.md` | 移植の記録。何が壊れていて、どう直し、何を確かめたか |
| `CLAUDE.md` | 移植作業の方針（AI エージェント向けの指示を兼ねる） |
| `build-logs/`, `run-logs/` | ビルドログ・実行時の作業ディレクトリ（git 管理外） |

### ビルドシステム

- 各プログラムの `src/<component>/Makefile.Linux`（macOS では `Makefile.Darwin`）を直接使います。元の `pconf` による生成や
  CMake 化は、まだ手を付けていません。
- GUI の層は 2 つあります。`GUI=sdl` で SDL2 版（`osw-sdl.cpp`）、`GUI=x11` で X11 版（`osw-x.cpp`）を使います。
  既定は Linux・FreeBSD が X11 版、macOS が SDL2 版です。Linux の SDL2 版（生成物は `build-linux-sdl/`）は、macOS 版を
  コンテナで確かめるためのもので、macOS の XQuartz 用の X11 版の生成物は `build-darwin-x11/` です。
- コンパイルオプションは `-O2 -g -Wall` が基本です。gcc 14 と macOS の clang の `-Wall` で、4 つとも**警告 0** です。
  `-Wno-*`・`-fpermissive`・`-w` による一括抑止は使っていません。例外は、macOS の SDK が `sprintf` を非推奨にしている警告で、
  macOS だけ `-Wno-deprecated-declarations` で抑止しています（すべての呼び出しで出るため）。
- 主な define:
  - `USE_XSHM`: MIT-SHM で描画する。ネットワーク越しの X サーバでは実行時に `--no_xshm` を付けて無効にする
  - `HAVE_XINERAMA`: 複数ディスプレイを考慮してウィンドウを配置する（`-lXinerama`）
  - `PLUGIN_SUPPORT`（server）
  - `HAVE_SDL_MIXER`（client）: SDL2_mixer で効果音と背景音楽を鳴らす。フラグは `pkg-config SDL2_mixer` から取る
  - `JS_SUPPORT`（client）: ジョイスティックを SDL2 で読む（libjsw の代わりに `jsw-sdl.cpp`）。SDL2 のフラグは
    SDL2_mixer の `pkg-config` から取るので、`HAVE_SDL_MIXER` を外すときは SDL2 のフラグを別に足す必要がある
  - YIFF / ESD 用の `HAVE_YIFF`・`HAVE_ESD` とそのコードは削除した
  - `OSW_SDL`: SDL2 版の GUI の層を使う（`GUI=sdl` のときに付く）
  - `XSW_DATA_DIR`・`SWSERV_DIR`: プログラムに埋め込むデータと server の場所（`src/include/xsw-paths.h`。`Makefile.Darwin` が PREFIX から付ける）
- **Makefile はヘッダの依存関係を追跡しません。** ヘッダを変えたら `make -f Makefile.Linux clean` してからビルドしてください。

### スクリプト

| スクリプト | 用途 |
|---|---|
| `scripts/build.sh <server\|client\|monitor\|unvedit\|all> [make の引数]` | ビルド（Dev Container 内と macOS）。ログは `build-logs/<component>.log`（macOS では `build-logs/darwin/`）、最後にエラー数・警告数を表示 |
| `scripts/smoke.sh` | スモークテスト（`GUI=sdl` で SDL2 版）。下記 |
| `scripts/smoke-macos.sh` | macOS でのスモークテスト。下記 |
| `scripts/gen-osw-fonts.py` | SDL2 版に組み込むフォント（`src/include/osw-sdl-fonts.h`）を X の misc-fixed から作る |
| `scripts/headless.sh start\|shot <png>\|key <keysym>\|stop` | Xvfb（:99）の起動、スクリーンショット、キー入力、停止 |
| `scripts/install-data.sh [-n] [インストール先]` | データのインストール |
| `scripts/xquartz.sh [client] [monitor] [unvedit]` | Dev Container の Linux 版を macOS ホストの XQuartz に表示（下記） |

### Dev Container の Linux 版を XQuartz に表示する

macOS 版を使わずに、Dev Container の中の Linux 版を macOS の XQuartz に表示することもできます（Linux 版の確認用）。

1. XQuartz の 設定 → セキュリティ →「ネットワーク・クライアントからの接続を許可」をオンにして、XQuartz を再起動する
2. XQuartz の xterm で `xhost +localhost` を実行する
3. コンテナで `scripts/build.sh all` のあと `scripts/xquartz.sh`（monitor・unvedit も出すなら `scripts/xquartz.sh client monitor unvedit`）
4. 初回だけ、上の「キーが効かないとき」の手順でキーを割り当て直し、bridge ウィンドウを閉じて保存する

実行環境は `run-logs/xquartz/` に作られ、キーの割り当てと宇宙の状態は次回に引き継がれます。

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
- client の効果音が SDL2_mixer から出力される（SDL の disk 出力 `SDL_AUDIODRIVER=disk` でファイルに書かせ、
  起動時のロゴの音が入っているかを調べる）
- client の背景音楽（MIDI）が流れ、曲の切り替えと、割り当ての無い曲での停止ができる
  （client の中で gdb から曲を切り替え、その後ゲームが通常の曲に戻すことも確かめる）
- ジョイスティックの軸で旋回し、ボタンに割り当てたキー（F8、推力モードの切り替え）が効く
  （gdb から SDL の仮想ジョイスティックをつないで操作する）

`GUI=sdl scripts/smoke.sh` は、client・monitor・unvedit を SDL2 版（`scripts/build.sh <c> GUI=sdl` でビルドしたもの）にして、
同じ項目を確かめます（結果は `run-logs/smoke-sdl/`）。SDL2 版の窓も Xvfb 上の X の窓なので、xdotool で操作できます。

macOS では `scripts/smoke-macos.sh` を使います。窓を画面に出さないように SDL の dummy 映像ドライバで動かすので、
確かめるのは、データのインストール、server の起動と終了、monitor の接続、client のログインと効果音、server の保存での
`EngineState = -1`、unvedit の読み込みと保存（終了時の緊急保存）です。キー操作と画面は、手で確かめてください。

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
- macOS 版の GUI（SDL2）は、X11 版と同じ画面になるように作っています。フォントは X の misc-fixed を組み込み、色の換算
  （`rgbi:`）は X の値を測って再現し、キーは X のキーコードで扱います。Xvfb 上で X11 版と SDL2 版のスクリーンショットを
  画素の単位で比べて確かめています（詳しくは `docs/PORTING_NOTES.md`）。
- コミットは小さな単位にし、メッセージには「現代の環境で何が壊れていたか」を書きます。非自明な発見は
  `docs/PORTING_NOTES.md` に追記します。

### 残っている課題

- ジョイスティックの実機での確認と、軸の反転の要否
- ビルドシステムの改修（ヘッダの依存関係、pconf の改修や CMake 化）
- キー割り当てを X サーバに依存しない形にすること（設定ファイルの形式に関わるので保留）
- ログイン直後に物体の名前を取りこぼす件の原因調査（送信キューのあふれと推測）
- server の `restart` スクリプトが csh で、`etc/generic.conf` を前提にしていること
- FreeBSD: `free` コマンドが無いので、client のオプションウィンドウのメモリ表示が 0 になる
- FreeBSD: server のプラグイン読み込みが Linux のときだけ有効になっている（FreeBSD でも dlopen は使えるが、
  機能を増やすことになるので変えていない）
- macOS 版: 塗りつぶした円弧の縁の画素を X サーバー（mi）と同じにすること、`.app` での配布、トップレベルの重なり順と
  transient の指定、複数ディスプレイでのダイアログの位置、メモリ表示（`free` コマンドが無い）、server のプラグイン
- 背景音楽の `theme/sounds/aquarium.mid` に「Copyright © 1999 by Ramon Pajares Box - All Rights Reserved」という
  権利表示がある（曲は Saint-Saëns で、元の配布物にも入っていたファイル）。扱い（そのまま残す・差し替える）は未定

---

## ライセンス

元の XShipWars は、GNU General Public License バージョン 2 に追加条項を付けたライセンスで配布されています
（全文は `src/LICENSE`）。追加条項は、改変したものを配布する場合に、改変内容を明示することを求めています。
このリポジトリでの改変内容は、git の履歴と `docs/PORTING_NOTES.md` に記録しています。
元の作者と貢献者は `src/CREDITS` にあります。
