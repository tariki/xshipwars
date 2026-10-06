# 移植メモ

移植中に判明した非自明な事項（型のサイズ・ファイル形式の前提・無効化した機能など）を記録する。

## server ビルド (2026-09-27)

- `Makefile.Linux` の `-D__cplusplus` / `-D__USE_BSD` は削除した。`__cplusplus` は g++ が自前で定義し、
  `__USE_BSD` は glibc の内部マクロで外から定義しても無視される（g++ は `_GNU_SOURCE` を既定で有効にする）。
- `strcasestr()`: glibc が提供しており、C++ では const/非 const のオーバーロードで宣言されるため、
  独自版（`src/global/string.cpp`）と宣言（`src/include/string.h`）は `#ifndef __GLIBC__` で除外した
  （2026-10-04: FreeBSD の libc も持っているので、`!defined(__GLIBC__) && !defined(__FreeBSD__)` に変えた）。
  独自版は NULL 引数で NULL を返したが glibc 版は NULL を渡すとクラッシュする。server は呼んでいない。
  unvedit の呼び出し元は確認済み: `uewpropsio.cpp` の引数はローカル配列なので NULL にならない。
  `wepw.cpp` は `PromptGetS()` の戻り値（プロンプトのバッファが無いと NULL）を渡していたので、NULL チェックを追加した。
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
- （2026-10-04 訂正）当初 `data/etc/xshipwarsrc`（1.33 形式）を使って確認していたが、これは古い複製だった。
  下の「client の設定ファイルのひな形」を参照。1.33 版の `UniverseListFile = .shipwars/universes` は
  `~/.shipwars/` からの相対パスとして解釈され `~/.shipwars/.shipwars/universes` を探していたが、1.34 版は
  `universes` で正しい。`JSCalibrationFile` の警告は版の古さではなく、ジョイスティックを無効にしたことが原因
  （読み込み処理が `#ifdef JS_SUPPORT` の中にある）。害は無く、ジョイスティックを復活させるときのために行は残す。

## キー入力が効かない原因 (2026-10-03)

- 設定ファイルの `BeginKeyMap` ... `EndKeyMap` には、**X のキーコード（番号）がそのまま**書かれている。
  同梱の設定ファイルの値は、旧 XFree86 キーボードドライバでの番号（TurnLeft=100, TurnRight=102,
  ThrottleIncrease=98, ThrottleDecrease=104, ThrottleIdle=107 など）。現在の Xorg/Xvfb（evdev/XKB）では
  Left=113, Right=114, Up=111, Down=116, Delete=119 なので、カーソル・ナビゲーションキーやテンキーに割り当てた操作が効かない。
  メインのキー（英数字、space=65、Esc=9、F1=67 など、番号 97 未満）は新旧で同じなので、そちらに割り当てた操作は効く。
- キーシンボルからキーコードを引く既定値（`KeymapWinSetDefault()`、`osw_keycode.*`）は、キー設定画面の
  「Default」ボタンを押したときにしか使われない。
- 対応: 同梱の `src/client/xshipwarsrc` の `BeginKeyMap` で、新旧の番号が異なる 8 個を evdev の番号に書き換えた
  （コードと設定ファイルの形式は変更なし）。変換には `/usr/share/X11/xkb/keycodes/xfree86` と `evdev` を使い、
  キー名（`<RGHT>` など）を経由して番号を対応させた。
  TurnLeft 100→113, TurnRight 102→114, ThrottleIncrease 98→111, ThrottleDecrease 104→116, ThrottleIdle 107→119,
  MessageScrollUp 99→112, MessageScrollDown 105→117, ScreenShot 111→107。
  キーコードは X サーバによって変わるので、evdev 以外の X サーバでは再びずれる可能性がある。
  また、すでに `~/.shipwars/xshipwarsrc` にコピーされた設定ファイルは古い番号のままなので、
  キー設定画面の「Default」ボタンで直す必要がある。
- ゲーム中のキー処理（`gctl.cpp`）は `bridge_win.is_in_focus` が真のときだけ動き、このフラグは bridge の
  トップレベルウィンドウが FocusIn を受けたときにしか立たない。ウィンドウマネージャの無い Xvfb では
  フォーカスが PointerRoot のままで FocusIn が来ないので、ヘッドレス確認では
  `xdotool windowfocus --sync <bridge のウィンドウ>` で明示的にフォーカスを与える必要がある（通常のデスクトップでは問題にならない）。

## monitor ビルド (2026-10-03)

- `Makefile.Linux` のフラグを `-DUSE_XSHM -O2 -g -Wall` にし、`-L/usr/X11R6/lib` を削除しただけでビルドできた。
  monitor は元々サウンドもジョイスティックも使っていない。client で直した共有部分（widgets/、global/osw-x.cpp）の修正がそのまま効いている。
- 警告に 64bit 関連（ポインタと整数の変換）は無い。`-Wsequence-point` は client と共通の global/disk.cpp:926 と global/string.cpp:346,357。
- monitor は server の AUX ポート（既定 1702）に接続する。

## monitor のヘッドレス起動 (2026-10-03)

- 起動: `monitor -u <名前> <パスワード> <アドレス> <ポート> -i src/monitor/images`。
  monitor の各ウィンドウは、ダッシュで始まらない最初の引数をアドレスとみなし、その時点で接続する。
  そのため `-u` はアドレスより前に書き、`-i`/`-s` のパスはアドレスとポートより後ろに書く
  （先に書くとパスがホスト名として扱われ「Unknown Host」になる）。画像の既定パスは
  `/usr/share/games/xshipwars/images/monitor` で、リポジトリでは `src/monitor/images`。
- AUX プロトコルはテキスト形式（`CONNECTIONS: n n`、`MEMORY: %ld %ld %ld` などの行）なので、
  バイナリの整数サイズには依存しない。server は time_t や long を `%ld` で送り、monitor は `%ld` で long に読む。
  64bit ではどちらも 64bit になり、一致している。
- AUX の Guest ログイン（`-u` を付けない場合の既定）は、server 側で「ログイン済み」にならない
  （`auxconn.cpp` の "Do nothing for Guest logins"）。サーバのログ（MESSAGE）はログイン済みの接続にしか
  転送されないので、Guest だと Messages ウィンドウには何も出ない。統計は Guest でも表示される。元からの仕様。
- メモリ表示の「Objects: 11 8608 bytes」から、64bit の server では xsw_object_struct が約 880 バイト
  （オブジェクトが 1 個増えると 880 増える）。32bit 版とはサイズが違うはずなので、64bit 監査で確認する。

## unvedit ビルド (2026-10-03)

- `Makefile.Linux` のフラグを `-DUSE_XSHM -O2 -g -Wall` にし、`-L/usr/X11R6/lib` を削除した。サウンドもジョイスティックも使っていない。
- `src/include/os.h` の固定幅型: glibc の `<sys/types.h>` が定義する `__BIT_TYPES_DEFINED__` が無いと、
  os.h は `int64_t` を `long long` などと自前で typedef する。システムのヘッダより先に os.h をインクルードするファイル
  （unvedit/rcfile.cpp など）では、あとから来る glibc の定義（LP64 の arm64/x86_64 では `int64_t` は `long`）と衝突していた。
  32bit の glibc では `long long` 同士だったので衝突しなかった。os.h で Linux/FreeBSD のときに先に `<sys/types.h>`
  をインクルードするようにし、常にシステムの型を使う（どの環境でもサイズは同じ）。
- unvedit はオブジェクト番号（int）を CList の項目データ（`void *`）に入れて往復させている。`intptr_t` を経由するようにした。
- **arm64 の char 問題**: `xsw_object_struct::engine_state` は `char` で、`ENGINE_STATE_NONE`（-1）を入れることがある。
  arm64 では char が unsigned なので 255 になり、次の問題が起きる。`signed char` にした（サイズ・レイアウトは同じ）。
  - ユニバースファイルに `EngineState = 255` と書き出される（x86 版は `-1`）。ファイル形式の値が変わってしまう。
  - server が `SWEXTCMD_SETENGINE` で 255 を送る（プロトコルの値が変わってしまう）。
  - client の `engine_state > 0` の判定が、エンジンの無い物体でも真になる。unvedit では None が "On" と表示される。
  `xsw_object_struct` の中で char 単体のメンバーはこれだけ（shield_state / cloak_state は int）。
  ほかの構造体やローカル変数の char については、64bit 監査で別途確認する。

## unvedit のヘッドレス起動 (2026-10-03)

- 起動: `HOME=<dir> unvedit <ユニバースファイル>`。設定は `~/.shipwars/unveditrc`（`ToplevelDir`・`ImagesDir`・`ServerDir`）。
  unvedit 自身の画像は `<ImagesDir>/unvedit/` から読み、リポジトリでは `src/unvedit/images`。ヘッドレス確認では
  `run-logs/xsw-root/images/unvedit` にシンボリックリンクを張った。
- generic_in.unv（オブジェクト 11 個）を開いて保存し直すと、選択していないオブジェクトは 1 バイトも変わらなかった。
  選択中のオブジェクトは保存時にプロパティ欄の値で上書きされるので、次の差分が出る（どれも元からの仕様で、64bit/arm64 とは関係ない）。
  - `ObjectHeading`: 画面には度数を `%.2f` で表示し、その文字列からラジアンに戻すので丸め誤差が出る（2.0969 → 2.0968）。
  - `CloakStrength`: unvedit は 0.0〜1.0 に収める。同梱データの Defiant は 1.25 なので 1.0 になる。
  - `ELink = `: 選択すると elink が空文字列で確保され、空の行が書き出される。

## 64bit / arm64 監査 (2026-10-03)

調べた範囲と結果。ここに書いた以外の問題は見つかっていない。

- **client↔server 通信・AUX 通信**: どちらも改行で終わるテキスト行（`sprintf`/`sscanf`）。server の送信関数は
  改行の無いデータを送らない。構造体や整数をバイナリのまま送る箇所、`htonl` などのバイト順変換は無い。
  `%ld` で送っているのはオブジェクト番号・セクター座標・画像セット番号・フラグ・通信間隔（ms）などの小さな値だけで、
  時刻のような大きな値は無い。64bit 版と 32bit 版をつないでも値はずれない。書式と引数の型は `-Wall`（`-Wformat`）の警告が無いことで確認した。
- **ファイル形式**: ユニバース・OCS・OPM・設定ファイルはすべてテキスト。`fwrite`/`fread` で構造体を書く箇所は無い
  （fwrite はログの文字列、fread は /proc の読み込みだけ）。unvedit で開いて保存し直しても、選択していない
  オブジェクトは 1 バイトも変わらなかった。`xsw_object_struct` のサイズ（64bit で約 880 バイト）はメモリ上の話だけで、ファイルにも通信にも出ない。
- **画像・X11**: 画像バッファへの画素の書き込みは、深度ごとに `u_int8_t`/`u_int16_t`/`u_int32_t` を使っている。
  `pixel_t`（`unsigned long`、64bit では 8 バイト）は X11 の API に渡すときだけ使っている。
- **時刻**: `MilliTime()` は「その日の 0 時からのミリ秒」（最大 86,400,000）なので int にも収まる。
- **ポインタ配列の確保**: `malloc`/`realloc`/`calloc` でポインタの配列を確保する箇所は、すべて `sizeof(型 *)` を使っている。
- **char の符号（arm64）**: `-Wtype-limits` 付きで 4 つをビルドし、char を負の定数と比べている箇所を洗い出した。
  - client の `option.throttle_mode`・`show_viewscreen_labels`・`show_formal_label`・`sounds` が `char` だった。
    推力モードを逆に回すと 0 から 255 になり（x86 では -1）、Incremental ではなく Normal に戻ってしまう。
    また、設定ファイルの負の値の補正（0 にする）が効かず、上限の値になっていた。`signed char` にした。
  - 構造体の char 単体のメンバー（74 個）について、負の定数を代入したり、減算したりしている箇所はほかに無い
    （`engine_state` は unvedit のときに修正済み）。`char c = getc(); c == EOF` のような箇所も無い（あれば `-Wtype-limits` で検出される）。
  - `strftime` の戻り値（size_t）や keycode（unsigned int）の `< 0` も警告に出たが、移植前から常に偽の無害なコード。
- **ついでに見つけたメモリ破壊（64bit とは無関係）**: server の `CryptDoEncrypt()` が、要素数 14 の `encrypted[]` に添字 14
  で書き込んでいた（グローバル配列の直後の 1 バイトを壊す）。直後で正しく終端しているので、範囲外に書く行を削除した。
  ASan/UBSan 付きのテストで、ハッシュの生成と照合（正しいパスワードは通り、誤りは拒否）を確認した。
- 同梱データの Defiant などはパスワードが `*` で、これは「どのパスワードでも通す」裏口の値
  （`CryptHandleVerify()` の `BACK_DOOR_PASSWORD`）。テストのときは、誤ったパスワードでもログインできる。

## 警告の修正 (2026-10-04)

- 「切り詰めてでも必ず終端するコピー」は `strlcpy()` を使う。glibc 2.38 以降（Debian trixie は 2.41）にある。
  `strncpy(dst, src, N); dst[N - 1] = '\0';` の形を置き換えた。`strncpy` と違って残りをゼロで埋めないが、
  構造体をそのままファイルや通信に書く箇所は無い（64bit 監査で確認済み）ので影響は無い。glibc 2.38 より古い環境へ
  移植するときは、自前の実装が必要になる。
- `strncpy` のあとに終端していなかった箇所（server の cmdeco.cpp の parm/val、cmdnetstat.cpp の larg、global/disk.cpp の fullpath）は、
  長い入力で終端の無い文字列になっていた。`strlcpy` にしたことで終端されるようになった。
- 4 つすべてが gcc 14 `-Wall` で警告 0（リンカの警告も含む）になった。一括抑止（`-Wno-*`、`-fpermissive`、`-w`）は使っていない。
- **キャストを使った唯一の箇所**: libXpm の `XpmCreatePixmapFromData()` は引数が `char **` のまま（const 非対応）だが、
  データを書き換えない。XPM のカーソルデータを `const char *[]` にしたので、`WidgetCreateCursorFromData()` から渡す
  1 か所だけ `const_cast<char **>` を使っている（widgets/wutils.cpp、理由をコメントに記載）。
- X11 の `XClassHint` のメンバーも `char *` なので、こちらはキャストせず、書き換え可能な静的配列（"Eterm"）を指すようにした。
- 文字列リテラルを返していた関数は、2 通りで直した。常に読み取り専用の文字列を返すものは戻り値を `const char *` にした。
  普段は静的バッファを返し、エラー時だけ `""` などを返していたものは、エラー時も静的バッファにその文字列を入れて返す。
- `tmpnam()`/`tempnam()` は `mkstemp()` にした。作られるファイルの権限が 0600 になる（以前は 0666 & ~umask）。対象は
  server の df（ディスク使用量）、client/unvedit の mf（メモリ統計）、server・unvedit の緊急保存、unvedit の印刷用一時ファイル。
  `PrinterPrintImage()` に渡す `tmp_file` は、末尾が XXXXXX の `mkstemp()` の雛形になった（実際の名前に書き換えられる）。
  このうち、df・mf・印刷は実際には動かして確認していない。
- `-Wformat-truncation`: 書き込み先はどれも通信やファイルの形式ではないので、最悪の長さが入る大きさにした。
  server の応答メッセージは、渡し先の `NetSendLiveMessage()` が CS_MESG_MAX(128) に切るので、クライアントに届く内容は変わらない。
  netfile.cpp のファイル名は、元から 192 文字で切って通信の 1 行（256 バイト）に 64 文字の余裕を残しており、
  その上限を配列の大きさで表した（上限そのものは変えていない）。
- Makefile はヘッダの依存関係を追跡しないので、ヘッダを変えたときは `clean` してから再ビルドすること。

## スモークテスト (2026-10-04)

- `scripts/smoke.sh` で、server・monitor・client・unvedit の主要な動作を自動で確認できる。
  実行環境（server の toplevel、client・unvedit 用の images の統合、HOME と設定ファイル）は毎回 `run-logs/smoke/` に作り直す。
  - client は URL を引数に渡して Guest で接続する（`xsw swserv://Guest:guest@localhost:1701`）。
  - 旋回・加速・推力モードは、client の内部の値を gdb で読んで判定する。
  - テスト用ユニバースでは Earth の EngineState を -1 にしてあり、server（終了時に generic_out.unv を書く）と
    unvedit（開いて保存し直す）を通っても -1 のままかを確認する（arm64 の char 符号の回帰テスト）。
  - unvedit の保存は、画面の座標（File メニュー → Save）をクリックしている。ウィンドウの配置が変わったら座標を直す必要がある。
- コンテナに `xfonts-base` が入ったので、Xvfb のフォントの別名による代用はもう必要ない。
- `mkstemp()` に変えた一時ファイルは、server の df と client の mf（関数を直接呼ぶテスト）で確認した。
  どちらもファイルを 0600 で排他的に作り、外部コマンドが追記し、読み終えてから削除している。unvedit の印刷は未確認。

## client の設定ファイルのひな形 (2026-10-04)

- client は初回起動時に、`<ToplevelDir>/etc/xshipwarsrc` を `~/.shipwars/xshipwarsrc` にコピーして使う
  （`universes` も同様）。インストールでこの `etc/` に置かれるのは、client の `make install` が入れる
  `src/client/xshipwarsrc` と `src/client/universes`（1.34 版）。
- リポジトリには `data/etc/xshipwarsrc`・`data/etc/universes`（1.33 版）もあったが、これは元の配布の
  「client データ」パッケージに入っていた古い複製で、インストール時には 1.34 版で上書きされる。
  キーコードの修正を最初はこちらにだけ入れてしまい、インストールされる 1.34 版は旧番号のままだった。
  混乱を避けるため 1.33 版の 2 ファイルは削除し、キーコードの修正は 1.34 版に入れた（変更した 8 個は同じ）。
- `scripts/smoke.sh` も、`etc/` をインストール後と同じ構成（`data/etc/*` に `src/client/` の 2 ファイルを重ねる）で組み立てる。

## インストール手順 (2026-10-04)

元の配布では、プログラム本体とは別に「client データ」と「グラフィックテーマ」のパッケージを
`/usr/share/games/xshipwars/` に展開する前提だった。このリポジトリではそれぞれ `data/` と `theme/` にあたる。
これらを配置する手順が無かったので `scripts/install-data.sh` を作った。全体の手順は次のとおり。

1. ビルド: `scripts/build.sh all`
2. プログラム本体: 各 `src/<component>` で `make -f Makefile.Linux install`（`PREFIX` は Makefile.Linux の値）
   - client: `/usr/games/xsw`、`/usr/share/games/xshipwars/etc/{xshipwarsrc,universes}`
   - monitor / unvedit: `/usr/games/` と `images/monitor`・`images/unvedit`
   - server: `/home/swserv/` の下に bin・db・etc（`default.conf`）など。`etc/default.conf` の `ServerToplevelDir` を合わせる
3. データ: `sudo scripts/install-data.sh`（既定のインストール先は `/usr/share/games/xshipwars`）
   - `etc/`（`data/etc` ＋ `src/client` の rc と universes）、`images/`（`data/images` と `theme/images` をまとめる
     ＋ `images/unvedit`・`images/monitor`）、`sounds/`（`theme/sounds`）を配置する
   - 2 と内容が重なる部分（client の rc、monitor/unvedit の画像）は同じファイルなので、どちらを先にしてもよい
   - `data/images` と `theme/images` で名前が重なると、何も書かずにエラーで止まる。`-n` で配置内容だけを表示する
   - インストール先を変えたときは、`~/.shipwars/xshipwarsrc` の `ToplevelDir` をそこに合わせる
- `scripts/smoke.sh` は、client・monitor・unvedit 用のデータをこのスクリプトでインストールしてから使う。
- 2 の `make install` は、`PREFIX` を変えて試した（`run-logs/make-install-test/`）。client・monitor・unvedit を
  同じ PREFIX に、server を別の PREFIX に入れ、3 を重ねてから、インストールしたプログラムで接続・表示・unvedit の
  読み込みまで確認した。インストールされた xshipwarsrc のキー割り当ても修正後の番号（TurnLeft=113 など）だった。
  - client は初回起動時に、既定の `/usr/share/games/xshipwars/etc` から rc をコピーする。既定以外の場所に
    データを入れたときは、`ToplevelDir` を合わせた `~/.shipwars/xshipwarsrc` を先に用意する必要がある。
  - server と一緒に入る `restart` は csh のサンプルスクリプトで、設定ファイルとして `etc/generic.conf` を参照するが、
    `make install` が置くのは `etc/default.conf`。使うときはコピーするかスクリプトを書き換える
    （コメントにも「環境に合わせて書き換えること」とある）。コンテナには csh が入っていない。
  - server の `make install` は `plugins/` ディレクトリを作らないが、`default.conf` は `PluginsDir = plugins` を指定している
    ので、起動時に「No such directory」の警告が出ていた（プラグインを使わなければ動作に影響は無い）。元からのインストール手順の抜けで、
    `src/server/Makefile.install.UNIX` で `plugins/` も作るようにした。

## ホストの XQuartz での表示 (2026-10-04)

- `scripts/xquartz.sh` で、コンテナ内の server に接続した client・monitor・unvedit を、ホスト（macOS）の XQuartz に表示できる。
  実行環境は `run-logs/xquartz/` に置き、HOME（キー割り当ての保存先）と server の宇宙は次回に引き継ぐ。
- コンテナのファイアウォールは、ゲートウェイ（172.17.0.1）と同じ /24 しか許可していなかった。Docker Desktop の
  `host.docker.internal`（192.168.65.254）はその範囲外なので、`.devcontainer/init-firewall.sh` に
  「`host.docker.internal` の tcp/6000 だけを許可する」ルールを足した（名前が引けない環境では何もしない）。
  起動時に使われるのはイメージに入れた `/usr/local/bin/init-firewall.sh` なので、反映にはリビルドが必要。
- MIT-SHM（`USE_XSHM`）はネットワーク越しには使えず、プログラム側に自動で切り替える処理も無い。`--no_xshm` で無効にする
  （osw-x.cpp の `OSWGUIConnect()` が解釈するので、client・monitor・unvedit のどれでも使える）。Xvfb で `--no_xshm` の
  描画が SHM ありと同じになることを確認した。
- XQuartz のキーコードは Xorg（evdev）と違う（macOS のキー番号 + 8 の見込み）ので、同梱のキー割り当ては効かない。
  Key Mappings ウィンドウの「Default All」は一覧の表示を既定値（キーシンボルから引いたキーコード）に戻すだけで、
  「Apply」（`KeymapWinApply()`）で初めて `xsw_keymap` に反映される。gdb から同じ関数を順に呼び、TurnRight を
  999 にしておいた状態から 114 に戻ることを確認した。設定は正規の終了（runlevel 1。bridge ウィンドウを閉じる
  `WM_DELETE_WINDOW` でも入る）のときに保存され、SIGTERM では保存されない。
- unvedit は SIGTERM を受けると、`$HOME/univXXXXXX` に緊急保存する（mkstemp に置き換えた経路が動くことの確認にもなった）。
- XQuartz 実機（X.Org 21.1.23）で確認した (2026-10-04)。リビルド後、`host.docker.internal:0` に接続でき、
  client が `--no_xshm` で表示された。
  - フォント `7x14`・`6x10` は XQuartz にある。
  - キーコードは macOS のキー番号 + 8: Left=131, Right=132, Up=134, Down=133, Delete=125, space=57, Escape=61,
    F1=130, F8=108, a=8, x=15。英数字も含めて Xorg（evdev）とすべて違うので、Key Mappings で割り当て直す必要がある。
  - XQuartz はルートレス表示なので、ルートウィンドウを撮っても中身は写らない。`xwininfo -root -tree` で
    "XShipWars: localhost" を探し、`import -window <id>` で client のウィンドウだけを撮る。
  - ログイン直後に、物体の名前だけが届かないことがあった（名前は "Object 3" などの仮のまま。自分の船の名前は届く）。
    server はログイン時のリフレッシュで物体ごとに作成情報と名前を送っており、client から `NetSendRefresh()` を
    呼んでリフレッシュを要求し直すと名前が届いた。XQuartz ではネットワーク越しの描画で client の起動が遅く、
    その間に一度に届くデータの一部が送信キューからあふれた可能性がある（Xvfb では起きない）。ログでの直接の確認はできていない。
  - リビルド直後の最初の実行で、`install-data.sh` の `install`（権限の設定）が "Operation not permitted" で
    失敗したことが 1 回あった。`/workspace` は macOS と共有している virtiofs で、その後 5 回繰り返しても再現しなかった。

## 複数ディスプレイでのダイアログの位置 (2026-10-04)

- client・monitor・unvedit は、ダイアログやメニューの位置を `osw_gui[0].display_width/height`（`OSWGUIConnect()` で
  `DisplayWidth/Height` から取る X の画面全体の大きさ）をもとに計算する（約 50 か所）。複数ディスプレイの環境では
  X の画面は全ディスプレイをまとめた大きさになるので、ダイアログが「全体の中央」に置かれ、ディスプレイの境目や
  別のディスプレイ、ディスプレイの無い位置に出ていた。XQuartz（2 台構成で画面 4448x1662）で、Options と Key Mappings が
  見えない位置（x=1904, 2024）に出て、開いていないように見えた。
- `HAVE_XINERAMA` を定義したときは、Xinerama で原点 (0, 0) にあるモニターを探し、その大きさを `display_width/height`
  に使うようにした（osw-x.cpp の 1 か所。50 か所の配置計算は変えていない）。Linux の client・monitor・unvedit の
  Makefile に `-DHAVE_XINERAMA` と `-lXinerama` を足した（Dockerfile に `libxinerama-dev`）。ディスプレイが 1 台なら
  今までと同じ。原点にメインのディスプレイがある前提で、そうでない配置ではずれが残る。
- 検証: Xvfb の `+xinerama -screen ... -screen ...` は 2 画面が原点に重なって横並びにならないので使えない。
  横 2048 の 1 画面の Xvfb に、libXrandr の `XRRSetMonitor()` で左右 2 つのモニターを定義すると、X サーバが
  Xinerama の問い合わせにそのモニターを返す。ただし、定義したクライアントが切断するとモニターは消えるので、
  定義したプロセスを動かしたまま client を起動する必要がある。この環境で、Options が全体の中央（x=704）ではなく
  左のモニターの中央（x=192）に出ることを確認した。
- XQuartz（モニター 0: 原点 1440x870、モニター 1: x=1440 の 3008x1662）でも、Options と Key Mappings がメインの
  ディスプレイ（モニター 0）に開くことをユーザーに確認してもらった。
- さらに、ダイアログを「メインウィンドウのあるモニター」に出すようにした。各プログラムが `OSWSetMainWindow()` で
  メインウィンドウを登録する（client: bridge、monitor: 最初のモニターウィンドウ、unvedit: 最初の編集ウィンドウ）。
  `OSWCreateWindow()` がルートの子として作ったトップレベルを記録しておき、`OSWMapRaised()`/`OSWMapWindow()` が
  閉じているトップレベルを開く直前に、メインウィンドウの中心があるモニターが別なら、同じ相対位置でそのモニターへ移す。
  メインウィンドウ自身、メインウィンドウがまだ開いていないとき（起動直後）、すでに開いているウィンドウ（前面に出すだけ）は
  動かさない（ユーザーがドラッグした位置を戻さないため）。Xinerama が無い・モニターが 1 台なら何もしない。
  2 モニターの Xvfb で、bridge を右のモニターに移すと Options が右の中央（1216, 144）に、左に戻すと左（192, 144）に開き、
  開いたままの Options は bridge を移して選び直しても動かないことを確認した。
  XQuartz（2 台構成）でも、bridge を外部ディスプレイに移すと Options が外部ディスプレイに開くことをユーザーに確認してもらった。

## サポート対象外の OS の削除 (2026-10-04)

CLAUDE.md の方針（対象は Linux と FreeBSD のみ）に沿って、次を削除した。どの削除でも、分岐の対象外 OS 側だけを消し、
Linux（と FreeBSD）側の処理は変えていない。削除のたびに 4 つをクリーンビルド（警告 0）し、スモークテストが全項目 PASS した。

- AIX・HP-UX・Solaris: `src/{client,monitor,server}/Makefile.{AIX,HPUX.10.20,Solaris}`（7 個）、各 `platforms.ini` の
  Solaris・AIX・HPUX の節、ソースの分岐（AIX の df の 512 バイト単位の換算、HP-UX・Solaris 用の df コマンド、
  空の HP-UX ジョイスティック節、Solaris の `dlfcn.h`、server の plugins.cpp の `__linux__ || __SOLARIS__` の Solaris 側）。
- Windows: `__MSW__`・`__WIN32__`・`_WIN32` の分岐（12 ファイル。ドライブ名と `\` のパス処理、`_findfirst()` による
  ファイル一覧、独自の `strcasecmp()`、Windows 用の既定ディレクトリ、server の `GetDiskFreeSpace()`、os.h の
  Solaris/Windows 用の固定幅型の節）と、どのビルドにも含まれていなかった `global/diskw.cpp`・`dfw.cpp`・`mfw.cpp`
  （と client 側のシンボリックリンク `mfw.cpp`）、pconf の `platforms.ini` の仮置きの Windows 節。
  分岐は、`__MSW__` などを未定義とみなして整理する小さなスクリプト（unifdef 相当）で機械的に消し、差分を確認した。
- 後回しにしている widgetdemo（`src/widgetdemo/`）には手を付けていない。Cygwin 用の `Makefile.cygwin` と、
  昔の configure が生成した `config.status`（AIX などの Makefile 名を含む）が残っている。
- `include/os.h` の `_PATH_MAILDIR` の条件には `__NetBSD__` が残っている（NetBSD は削除の対象にしていない）。

## FreeBSD への対応 (2026-10-04)

FreeBSD の実機・ヘッダはこの環境に無いので、知られている FreeBSD との違いに当てはまる箇所をコードから探して直した。
**どれも FreeBSD では未確認。** Linux でのビルド（警告 0）とスモークテストは毎回確認した。

- `<malloc.h>`（31 ファイル）を `<stdlib.h>` にした。FreeBSD の `<malloc.h>` は「`<stdlib.h>` を使え」という `#error` になる。
  malloc.h にしか無い関数（mallinfo, memalign など）は使っていない。
- server/crypt.cpp の `<crypt.h>`: FreeBSD には無い（crypt は `<unistd.h>`）。`__has_include(<crypt.h>)` のときだけ読み、
  `<unistd.h>` は常に読む。
- 独自の `strcasestr()`: FreeBSD の libc にもあり、`<string.h>` で C リンケージで宣言されるので、C++ の独自版とぶつかる。
  FreeBSD でも独自版を使わないようにした。
- `Makefile.FreeBSD`（4 つ）を Linux と同じ設定にした。BSD make は `CPP` に `cpp` を最初から入れるので、
  `CPP ?= g++` では上書きされず、client は `CPP? = g++` の書き間違いもあった。FreeBSD の基本システムには g++ も無いので
  `CPP = ${CXX}`（標準は c++ = clang）にした。X11 は `/usr/X11R6` ではなく `${LOCALBASE}`（/usr/local）。
  client の PREFIX は /usr/local だったが、プログラムはデータを `/usr/share/games/xshipwars` で探し、同梱の設定ファイルの
  ひな形の ToplevelDir も同じなので、Linux と同じ /usr にした（FreeBSD の慣習からは外れる。置き場所を変えられるように
  するのは、ビルドシステムの改修でまとめて行う）。
- Linux 上で GNU make に `-f Makefile.FreeBSD` を渡すと、4 つとも警告 0 でビルドできることを確認した。
  BSD make（bmake）での動作と、Makefile の `include`（ドットなし）が BSD make で通るかは未確認。
- 未対応のまま記録だけにしたもの:
  - FreeBSD には `free` コマンドが無いので、mf（メモリ統計）が失敗し、client のオプションウィンドウのメモリ表示が 0 になる。
  - server のプラグイン読み込み（plugins.cpp）は `__linux__` のときだけ有効。FreeBSD でも dlopen は使えるが、
    機能を増やすことになるので変えていない。
  - clang の `-Wall` での警告は確認できていない（この環境には gcc しかない）。

## サウンド: SDL2_mixer への置き換え (2026-10-05)

- 元の client は YIFF と EsounD のサウンドサーバに対応していたが、どちらも今は無い。`sound.cpp` は
  `sound.server_type` で方式を切り替える作りで、呼び出し側（`SoundPlay()` だけで 34 か所）は方式を意識していない。
  server は `CS_CODE_PLAYSOUND`（音の番号と音量）を送るだけなので、方式を変えてもプロトコルは変わらない。
- YIFF / EsounD のコードと同梱の YIFF ヘッダ（`src/include/Y2/`）を削除した。EsounD のコードには接続を表すポインタを
  int にキャストする 64bit で壊れる書き方もあった。設定の `SoundServerType` の 1〜3（YIFF / EsounD / MikMod）は
  読み込めるが音は鳴らない。
- `SoundServerType = 4`（`SNDSERV_TYPE_SDL`）を追加した。`HAVE_SDL_MIXER` を定義したときだけ有効。
  - 初期化: `SDL_InitSubSystem(SDL_INIT_AUDIO)` と `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)`、16 チャンネル。
    失敗すると -1 を返し、client は音を無効にして続ける（既存の処理）。
  - 効果音: サウンドスキームの WAV（同梱はすべて PCM 8bit ステレオ 11025Hz）を初めて鳴らすときに読み、パスと一緒に保持する。
    再生は `Mix_GroupAvailable(-1)` で空きチャンネルを選び、`Mix_SetPanning()` で左右の音量を設定してから
    `Mix_PlayChannel()` で始める（先に再生すると、最初の数十ミリ秒が前の音量で鳴りうるため）。
  - 背景音楽（同梱は MIDI 3 曲）は、後に対応した（下の「背景音楽 (MIDI)」）。
- 検証: 音声デバイスの無いコンテナでは、SDL の disk 出力（`SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=<file>`）で
  ミキサーの出力を 16bit ステレオ 44.1kHz の raw ファイルに書かせて調べた。起動時のロゴの音（2.59 秒）、
  エンジンのオン・オフ（重なって再生）、オプション画面の Test Sound の左だけ・右だけ・両方（反対側は振幅 0）を確認した。
  gdb から `SoundPlay()` を呼ぶと音声スレッドも止まるので、再生と左右の設定の順序の問題は gdb では見えないことに注意。
  スモークテストにも、起動時のロゴの音が出力されるかの項目を足した。
- macOS の Docker Desktop のコンテナには音声の出力が無いので、XQuartz 経由で遊んでいても音は聞けない。
- 同梱の設定ファイルのひな形（`src/client/xshipwarsrc`）で、音を既定で有効にした（`SoundServerType = 4`、`Sounds = 3`）。
  今は Sounds の 1〜3 で鳴る効果音に違いは無い（判定はすべて「なし以外」か「イベント以上」）。
  音声デバイスの無い環境では、ALSA の警告と初期化失敗のメッセージを出して音を無効にし、client はそのまま動く。
  このとき `option.sounds` が 0 になり、終了時に `Sounds = 0` が保存されるので、その HOME では次から音が無効になる
  （元からの作り。XQuartz 用の `run-logs/xquartz/home` もこうなる）。スモークテストは、同梱の既定値のまま音を確認する。

## 背景音楽 (MIDI) (2026-10-06)

- 曲の選択は元のまま: `XSWDoChangeBackgroundMusic()`（main.cpp）が状況から雰囲気のコードを決め
  （メインメニュー 104、通常 100、星雲の中 103、戦闘 102）、`sound.bkg_mood_code` と違うときだけ
  `SoundChangeBackgroundMusic()` を呼ぶ。呼ばれるのはイベント時（自分の物体が決まったとき、ロックの変化、
  星雲の出入り、メインメニューの表示・非表示、オプションの適用）で、`option.music`（設定の `Music`）が on のときだけ。
- 同梱の `default.ss` は 100・101 が bluedanube.mid、102 が marsbringerofwar.mid、103 が aquarium.mid。104（メインメニュー）と
  105 には割り当てが無い。元の YIFF 版は、割り当ての無いコードでは前の曲を止めるだけなので、メインメニューは無音。これも同じにした。
- SDL 版: 前の曲を `Mix_HaltMusic()`・`Mix_FreeMusic()` で止めて、`Mix_LoadMUS()`・`Mix_PlayMusic(music, -1)` で
  無限ループ再生する（YIFF 版の `total_repeats = -1` と同じ）。読み込みに失敗してもコードは記録するので、同じ雰囲気の間は再試行しない。
- MIDI の音源: Debian の SDL2_mixer は、MIDI を FluidSynth（`libfluidsynth.so.3` を実行時に dlopen）か、組み込みの
  Timidity で鳴らす。**`libsdl2-mixer-dev` を入れても `libfluidsynth3` も SoundFont も入らない**（依存関係に無い）。
  - FluidSynth の SoundFont は既定で `/usr/share/sounds/sf3/default-GM.sf3` と `/usr/share/sounds/sf2/FluidR3_GM.sf2` を探す。
    Debian の SoundFont パッケージは alternatives で `default-GM.sf3` も登録するので、`timgm6mb-soundfont` を入れるだけで見つかる
    （中身は sf2 だが FluidSynth は問題なく読む）。環境変数 `SDL_SOUNDFONTS` は、既定の場所に SoundFont が無いときか、
    `SDL_FORCE_SOUNDFONTS=1` のときだけ使われる。
  - SoundFont が見つからないと Timidity に落ち、エラーは「Couldn't open timidity.cfg」になる。
  - freepats（Timidity 用）は楽器が欠けていることがあるので選ばなかった。marsbringerofwar.mid は 11 種類、aquarium.mid は 9 種類の
    GM 音色とドラムを使う。
  - `fluid-soundfont-gm`（約 140MB）ではなく `timgm6mb-soundfont`（約 6MB）にしたのは、SDL2_mixer が曲を読むたびに
    SoundFont を読み込み直すため（雰囲気が変わるたびに main ループの中で読む）。TimGM6mb で、読み込みは初回 約 130ms（FluidSynth の dlopen を含む）、
    2 回目以降 約 40ms（arm64 コンテナ）。
- 同梱の MIDI の中身: marsbringerofwar.mid と aquarium.mid にはテキストのメタイベント 0x21（MIDI port）があり、FluidSynth が
  読み込みのたびに「Ignoring unrecognized meta event type 0x21」を 20 行ほど出す。害は無い。
  aquarium.mid には「Copyright © 1999 by Ramon Pajares Box - All Rights Reserved」、marsbringerofwar.mid には
  「Sequenced by Jack Hines」という記載がある（曲自体は Saint-Saëns、Holst）。元の配布物にも入っていたファイル。
- 検証: スモークテストで、ログイン後に曲 100 が再生中（`Mix_PlayingMusic()`）であること、gdb から 102 に切り替えて再生中、
  104（割り当て無し）で停止すること、detach 後にゲームが通常の曲 100 に戻して再生することを確かめる。
  disk 出力で 3 曲とも音が出ることも確かめた。スモークテストは雛形の既定値によらず `Music = on` にして実行する。
