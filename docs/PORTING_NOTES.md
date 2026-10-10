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

## ジョイスティック: libjsw を SDL2 で置き換え (2026-10-06)

- libjsw（wolfpack.twu.net）は Debian にも FreeBSD ports にも無い。FreeBSD の Linux 互換 `/dev/input/js*`（linux-js）も
  2013 年に廃止された。Linux と FreeBSD の両方で読めるように、SDL2 の Joystick API を使う（SDL2 はサウンドで既にリンクしている）。
- client が使う libjsw の関数は `JSInit`・`JSUpdate`・`JSGetAxisCoeff`・`JSGetAxisCoeffNZ`・`JSClose` の 5 つだけで、
  構造体は `js_data_struct` の軸・ボタンの数、ボタンの状態、`fd` だけ。この 5 つを `src/client/jsw-sdl.cpp` に SDL2 で書き、
  同梱の `src/include/jsw.h` をそのまま使う。gctl.cpp・jsmap.cpp・jsmapwin.cpp・設定ファイルの形式は変えていない。
  - `fd` には SDL のジョイスティックのインスタンス ID を入れる。開いているかどうかは `fd` ではなく `JSFlagIsInit` で判定する
    （calloc しただけの構造体は `fd` が 0 で、SDL のインスタンス ID は 0 から始まるため）。
  - デバイス名の末尾の数字を SDL のデバイス番号にする（`/dev/js0` → 0）。数字が無ければ `JSBadValue`。
  - 軸は SDL の -32768〜32767。ハットは、libjsw が使っていた Linux の joystick ドライバと同じく、軸の後ろに 2 本ずつ（x, y）並べる。
  - 不感帯は libjsw の未校正時の既定値（中心 500・最大 1000 に対して 100 = 振れ幅の 20%）。
    `JSGetAxisCoeffNZ` は不感帯を 0 にし、その外側を -1〜1 に広げる。
  - SDL の初期化で `SDL_HINT_NO_SIGNAL_HANDLERS` を立てる（ジョイスティックの初期化はイベント機能も初期化し、
    既定では SIGINT・SIGTERM のハンドラを入れるため）。読み取りは `SDL_JoystickUpdate()` のポーリングで、
    `SDL_JoystickEventState(SDL_IGNORE)` にして SDL のイベントキューには溜めない。
  - 抜かれたジョイスティックは `JSUpdate` が `JSNoEvent` を返すだけで、client はそのまま動く（仮想ジョイスティックで確認）。
- libjsw の校正ファイル（`JSCalibrationFile`）は読まない。作るツール（jscalibrator）も無い。設定の行は読み書きを続け、
  存在しなくても警告しないようにした（以前は JS_SUPPORT 無しで「Unknown parameter」、有効にすると「No such file」が出ていた）。
- 軸の向き: gctl.cpp は Normal モードの推力を `(coeff + 1) / 2`、ズームを `(coeff - 1) / -2` で求める。SDL も Linux の joystick
  ドライバも、前に倒すと負の値なので、推力はスティックを手前に引くと上がる。元は libjsw の校正（`JSAxisFlagFlipped`）で
  反転させる前提だったと思われる。反転の手段は今は無い。
- JS_SUPPORT を有効にしてビルドすると、20 年以上コンパイルされていなかったコードに 64bit の問題があった:
  jsmapwin.cpp がボタンのキーコードを `(void *)keycode` で一覧のデータポインタに入れ、`(keycode_t)` で取り出していた
  （64bit では int へのキャストがエラー）。keymapwin.cpp と同じく `uintptr_t` を経由させた。
  optwinop.cpp の校正ファイルの欄では、文字列リテラルを `char *` に入れていたので、隣の欄と同じく `dname.home` を使うようにした。
  `xsw.h` は `<jsw.h>`（システムの libjsw）ではなく同梱の `../include/jsw.h` を読むようにした。
- 検証: コンテナには `/dev/input` も `/dev/uinput` も無いので、SDL2 の仮想ジョイスティック（`SDL_JoystickAttachVirtual`）を
  gdb から client の中に作って確かめた。仮想デバイスは SDL のジョイスティック機能が終了すると消えるので、
  先に `SDL_InitSubSystem(SDL_INIT_JOYSTICK)` で参照を 1 つ持たせてから `GCtlInit(CONTROLLER_JOYSTICK)` を呼ぶ。
  スモークテストでは、設定ファイルに割り当て（軸 0 = 旋回、ボタン 0 = F8）を入れ、軸を倒して旋回すること、
  ボタンで推力モードが進むことを判定する。割り当て画面（Map Joystick）で「Refresh」を押すと、軸 4 本（軸 2・ハット 1）と
  ボタン 1 個が読み込まれることも見た。

## Linux 以外の分岐のビルド確認 (2026-10-09)

- FreeBSD 用 Makefile は Linux 上の GNU make でビルドして確かめていたが、それでは `__linux__` が定義されたままで、
  `#ifdef __linux__` の外側（FreeBSD で通る側）は一度もコンパイルされていなかった。`g++ -U__linux__` でビルドすると次が出た:
  - client: `DEF_JS_CALIBRATION_FILE` が `xsw.h` で `#ifdef __linux__` の中にしか無く、main.cpp と optwinop.cpp で未定義になる
    （ジョイスティックを SDL2 で有効にしたときに入った、**FreeBSD での client のビルドエラー**）。SDL2 で読むので OS によらない値にした。
  - server の plugins.cpp（dlerror の結果）と widgets/wfbrowser.cpp（mount・umount）に、Linux の分岐の中でしか使わない変数があり、
    FreeBSD では -Wunused-variable の警告になる。宣言を `#ifdef __linux__` の中に移した。
  - unvedit: `os.h` が `<sys/types.h>` を Linux と FreeBSD のときしか読まないので、`__linux__` を外すと `__BIT_TYPES_DEFINED__` が
    未定義のまま自前の `int64_t`（long long）を定義し、glibc の `int64_t`（long）と衝突した。FreeBSD 自体では自前の定義を
    飛ばすので実害は無いが、確認のじゃまになるので `<sys/types.h>` は常に読むようにした（POSIX の標準ヘッダ）。
- `-D__FreeBSD__` も付けると、gcc の stddef.h が FreeBSD 用の `sys/_types.h` を探して失敗するので、FreeBSD をまねるのはここまで。
  clang の警告や FreeBSD のヘッダの違いは、実機でないと分からない。
- 確認の手順: `make clean` のあと `scripts/build.sh all CPP="g++ -U__linux__"` で 4 つとも警告 0 になること。
- `scripts/install-data.sh` は個別のファイルを GNU の `install -D`（親ディレクトリも作る）で入れていたが、FreeBSD の `install` では
  `-D` はインストール先のルートを指定するオプションで意味が違う。`mkdir -p` で親を作ってから `install -m 0644` にした。
  BSD の `wc -l` は数字の前に空白を付けるので、件数の表示は算術展開で数字だけにした。直す前と後で、インストール結果
  （中身・パーミッション・シンボリックリンク）が同一なことを確かめた。`declare -A` は bash 4 以降が要るが、FreeBSD の bash パッケージで足りる。

## macOS: 段階 1（XQuartz の Xlib でのビルド） (2026-10-09)

macOS 27（Apple Silicon）、Apple clang 21、XQuartz（`/opt/X11`）で確認した。この段階の X11 版は、SDL2 版の OSW 層ができるまでの
つなぎ（clang や libc の違いを先に片付けるため）。

- `src/{server,monitor,unvedit}/Makefile.Darwin` を作った。オブジェクトと実行ファイルは `src/<component>/build-darwin/` にでき、
  コンテナの Linux 版（ソースと同じ場所にできる）と混ざらない。macOS の make は GNU make 3.81 で、Linux の Makefile の
  `VAR != cmd`（4.0 以降）が使えないので、pkg-config などを使うときは `$(shell ...)` にする。
  crypt() と dlopen() は libSystem にあるので、server の `-lcrypt -ldl` は要らない。X11 は `-I/-L $(X11BASE)`（/opt/X11）。
- `scripts/build.sh` は macOS では `Makefile.Darwin` を使い、ログは `build-logs/darwin/` に出す（Linux ではこれまでどおりコンテナ専用）。
  `build.sh <c> clean all` は、clean を先に別の make で実行するようにした。同じ make の中で clean と all を続けると、
  make が clean の前のファイルの状態を覚えていて「Nothing to be done」になる（Linux の clean は実行ファイルを消さないので表に出ていなかった）。
- clang（`-Wall`）で gcc では出なかった警告:
  - `off_t` を `%ld` で表示していた（server の cmddisk.cpp の 4 か所、monitor の monmanage.cpp のメモリ表示 4 か所）。
    macOS の `off_t` は `long long`（Linux LP64 と FreeBSD は `long`）。サイズは同じなので値は正しく出ていたが、`%lld` と
    `(long long)` にそろえた。cmddisk はクライアントに送る文字列だが、表示される数字は変わらない。
  - `-Wunused-but-set-variable`: 加算するだけで読まない変数（monitor/unvedit の `events_handled`、unvedit の rcfile.cpp の
    `bytes_written`、client の blittile.cpp の `tar_x_col`）。gcc は `x++` や `x += f()` を「使用」とみなすので警告しない。変数を削除し、関数の呼び出しは残した。
  - Apple の SDK は `sprintf`/`vsprintf` を非推奨（deprecated）にしていて、呼び出しのたびに警告が出る（ソース全体で約 960 か所。
    server だけで約 400 件）。`_POSIX_C_SOURCE` が未定義のときだけ付く印。コードの誤りではないので、
    `Makefile.Darwin` に `-Wno-deprecated-declarations` を付けて抑止した（警告の一括抑止をしない方針の例外。ユーザーと決めた）。
    ほかの非推奨の警告も macOS では見えなくなるが、Linux と FreeBSD のビルドの `-Wall` では引き続き出る。
    `_POSIX_C_SOURCE` を定義する方法は、Darwin の拡張（BSD の関数など）の宣言まで消えるので採らなかった。
- macOS は `__linux__` でも `__FreeBSD__` でもないので、Linux 以外の分岐（「POSIX」の扱い）に入る。確認した結果:
  - `df.h`: macOS の `df -P` は 512 バイト単位で表示する（`-k` を付けると 1024 バイト単位）。df.cpp は 1024 バイト単位を前提にしているので、
    容量が 2 倍になっていた。macOS では `df -P -k` にした。DiskFreeGetListing() の結果が `df -k` と一致することを確かめた。
  - `mf.h`: macOS にも `free` コマンドが無いので、FreeBSD と同じくメモリ統計は 0 になる（未対応）。
  - server のプラグイン（plugins.cpp）は `__linux__` のときだけ dlopen する。FreeBSD と同じく、macOS でもプラグインは読み込めない（機能を増やさない）。
  - `os.h` の固定幅型: macOS の `<sys/types.h>` は `__BIT_TYPES_DEFINED__` を定義しないので os.h の自前の typedef が通るが、
    型はシステムと同じ（`int64_t` は両方 `long long`）なので衝突しない。
  - 独自の `strcasestr()`（global/string.cpp）は macOS でも使われる。libc の宣言（C リンケージ）と同じ型なのでビルドは通り、
    プログラム内では独自版（NULL を渡しても落ちない）が使われる。
- 動作確認（ホスト）:
  - server: 1701/1702 で待ち受け、AUX に統計を返し、SIGTERM で「shut down normally」と出して終了した。
    保存したユニバースで `EngineState = -1` が保たれた（macOS arm64 の char は signed なので、もともと問題は起きない）。
  - monitor: XQuartz の Xvfb（`/opt/X11/bin/Xvfb`）で server の AUX につなぎ、統計が表示された。
    macOS では Xvfb が `/tmp/.X11-unix` を作れない（root でない）ので、`-nolisten unix -listen tcp` で起動し、`DISPLAY=127.0.0.1:<n>` にする。
  - unvedit: generic_in.unv を開き、SIGTERM の緊急保存（`UEDoEmergencySaveAll()`）で書き出したファイルが元と 1 バイトも違わなかった。
- 共有コードの変更（blittile.cpp など）は、コンテナで 4 つのクリーンビルド（警告 0）、`g++ -U__linux__` でのビルド（警告 0）、
  スモークテスト（全項目 PASS）で確かめた。
- client（2026-10-09 追記）: `src/client/Makefile.Darwin` を作った。SDL2・SDL2_mixer は Homebrew（`brew install sdl2 sdl2_mixer`）。
  - Homebrew の `sdl2` は今は **sdl2-compat**（SDL3 の上で SDL2 の API を提供する）で、SDL3 も入る。FluidSynth は SDL2_mixer の依存として入る。
  - pkg-config の SDL2_mixer のフラグは `-I/opt/homebrew/include` と `-L/opt/homebrew/lib` を含み、そこには Homebrew の libX11 の
    ヘッダとライブラリもある。XQuartz のものを使うように、`-I/opt/X11/include` と `-L/opt/X11/lib` を先に置く。
  - clang の警告は server などと同じ種類: cmdlog.cpp の `off_t` の `%ld`（3 か所）と、main.cpp の読まない `total_events_handled`（2 か所）。
  - 動作確認: XQuartz の Xvfb で server に Guest でログインし、ブリッジの画面が描かれた（FPS 48）。効果音は SDL の disk 出力
    （`SDL_AUDIODRIVER=disk`。sdl2-compat でも効く）で、音のある 10ms 区間が出力された。XQuartz の Xvfb にも 7x14・6x10 のフォントがある。
    キー操作は確かめていない（macOS には xdotool が無い）。
  - 背景音楽: macOS の SDL2_mixer が既定で探す SoundFont は `/usr/share/sounds/sf2/FluidR3_GM.sf2` だけで、macOS ではそこに置けない
    （SIP で /usr は書き込めない）。`SDL_SOUNDFONTS` で指定する必要がある。どこに置いてどう指定するかは、段階 4（配布の形とデータの置き場所）で決める。
- `scripts/install-data.sh` は、ファイル名の重なりの確認に bash 4 の連想配列（`declare -A`）を使っていて、macOS の `/bin/bash`（3.2）で
  止まっていた。確認を awk の連想配列で書き直した（メッセージと判定は同じ）。重なりがあるとき、以前と同じエラーで止まることを確かめた。
- client の変更（cmdlog.cpp・main.cpp）と install-data.sh は、コンテナで client のクリーンビルド（警告 0）、`g++ -U__linux__` でのビルド（警告 0）、
  スモークテスト（全項目 PASS）で確かめた。install-data.sh は、直す前と後でインストール結果（中身・パーミッション・シンボリックリンク）が同一。

## macOS: 段階 2（widgets の Xlib の直接呼び出しを OSW 層へ） (2026-10-09)

SDL2 版の OSW 層（段階 3）で widgets を変えずに済むように、OSW 層の外で Xlib を直接呼んでいた箇所を OSW 層に移した。
X のコードは変数名・処理の順序を変えずに `global/osw-x.cpp` に移し、widgets の関数は OSW の関数を呼ぶだけにした（API は同じ）。

- 調べた範囲: client・widgets・global・monitor・unvedit（osw-x.cpp を除く）。Xlib を直接呼んでいたのは widgets の
  `wutils.cpp`・`wlist.cpp`・`wfile.cpp` と、client の MIT-SHM まわり（main.cpp・vsdraw.cpp・bridgemanage.cpp・vsevent.cpp）だけだった。
  client の MIT-SHM まわりと Visual の確認は、元から `#if defined(X_H) && defined(USE_XSHM)` や `#ifdef X_H` で囲まれ、
  そうでないときは OSW の関数（`OSWPutSharedImageToDrawable()` など）を使う作りなので、変えていない。
  global/imlibosw.cpp も Xlib を使うが、どのビルドにも含まれていない。
- OSW 層に足した関数:
  - `OSWCreateCursorFromXpmFile()`・`OSWCreateCursorFromXpmData()`: XPM を深さ 1 の pixmap とマスクに読み、推奨サイズに縮め、
    ホットスポットを XPM の大きさに収めてカーソルを作る（`WidgetCreateCursorFromFile()`・`FromData()` の中身。共通部分は static の関数にまとめた）。
  - `OSWCreateCursorFromImage()`: 一覧の項目の画像から白黒のカーソルを作る（`ListWinCreateCursorFromEntry()` の中身）。
  - `OSWCreatePixmapMaskFromImage()`: 画像の 0 以外の画素を白にした深さ 1 のマスク（`WidgetPixmapMaskFromImage()` の中身）。
  - `OSWLoadImageFromXpmFile()`・`...Data()`・`OSWLoadPixmapFromXpmFile()`・`...Data()`（wfile.cpp の XPM の関数の中身）。
  - `XpmDefaultColorCloseness`（libXpm の値ではなく、widget.h で定義していた）も osw-x.cpp に移した。
- 既存の OSW の関数に置き換えたもの（中身がまったく同じ）: 画像のタイル貼りと影付き・透過の転送の `XPutImage` →
  `OSWPutImageToDrawablePos()`、pixmap のタイル貼りの `XCopyArea` → `OSWCopyDrawablesCoord()`、`WidgetGetPixel()` の
  `XParseColor`・`XAllocColor` → `OSWLoadPixelCLSP()`。
- `WidgetGetPixel()`・`WidgetPixmapMaskFromImage()`・`WidgetCreateCursorFromFile()`・XPM から画像や pixmap を読む 4 つの関数は、
  どこからも呼ばれていない（widgetdemo を除く）。API を残すために移した。
- 見つけた既存の不具合（直していない）: `OSWLoadPixelCLSP()` は `XParseColor()` の戻り値を `BadColor`・`BadValue` と比べているが、
  実際は失敗すると 0 を返すので、失敗しても初期化していない色で `OSWLoadPixelRGB()` を呼ぶ。呼び出し元（wglobal.cpp）は
  決まった色の名前（`CLSP_*`）しか渡さないので、この経路は通らない。
- 確認: Linux（コンテナ）と macOS で 4 つのクリーンビルド（警告 0）、`g++ -U__linux__` でのビルド（警告 0）、スモークテスト（全項目 PASS）。
  スモークテストでは通らないドラッグ用のカーソルについて、移す前の関数（git の HEAD から取り出したもの）と移した後の関数で
  カーソルを作り、Xvfb のルートウィンドウに設定して XFixes（`XFixesGetCursorImage()`）で読み出した画像・大きさ・ホットスポットと
  `WCursor` の値が一致することを確かめた（項目の画像からのカーソル、XPM からのカーソル 3 種類。ホットスポットの補正を含む）。
  Xvfb の推奨カーソルサイズは十分大きいので、カーソルを縮める処理は通っていない。

## macOS: 段階 3（SDL2 版の OSW 層） (2026-10-10)

- 骨組み: `osw-x.h` から GUI によらない部分を `osw-api.h` に分け、`osw-sdl.h` は X と同じ名前・値の型と定数を定義する
  （`X_H` は定義しない）。これだけで client・monitor・unvedit の全ソースがそのままコンパイルできた（ゲーム本体と widgets の変更は不要）。
  X 専用の処理（MIT-SHM・Visual の確認）は元から `X_H` で囲まれている。囲みの外で宣言していた変数（client の 5 個）だけ中に移した。
  ビルド: `make GUI=sdl`（`scripts/build.sh <c> GUI=sdl`）。生成物は `build-linux-sdl/`・`build-darwin-sdl/`。
- ゲーム本体と widgets が使う X のものは、思ったより少なかった:
  イベントのメンバーは `xany.window`・`xkey.keycode`・`xbutton.button/x/y`・`xmotion.x/y`・`xvisibility.state` だけ、
  修飾キーはイベントの `state` ではなく `osw_gui[0].shift_key_state` などから読む、キーは `XK_*` ではなく `osw_keycode.*` と比べる、
  描画は前景色 1 つ（XOR などは無し）、フォントは `7x14`・`6x10`・`6x12` だけ。`win_attr_t` は `x`・`y`・`width`・`height` しか読まない。
- ウィンドウの木: ボタンなどの部品もすべて X の子ウィンドウ。SDL のウィンドウはトップレベルだけにし、子ウィンドウは OSW 層の中で持つ。
  各ウィンドウと pixmap に 32bpp（0x00RRGGBB）の面を持たせ、トップレベルごとに重なり順に合成して表示する（`OSWEventsPending()` と
  `OSWGUISync()` のとき、変化のあったものだけ）。内容は保たれるので（X の backing store 相当）、Expose は見えるようになったときと大きくなったときだけ出す。
  トップレベルの SDL のウィンドウは最初に Map したときに作る（作成の直後に設定される枠の種類を反映するため）。
- イベントの配送は X の規則に合わせた: ポインタとキーのイベントはポインタの下のいちばん内側のウィンドウから、その種類を選んでいる
  ウィンドウまで親をたどる。ButtonPress を受けたウィンドウはボタンを離すまで暗黙のグラブを持つ。Map で MapNotify・VisibilityNotify・
  Expose。閉じるボタンは `WM_DELETE_WINDOW` の ClientMessage。キーの自動リピートは X と同じく Release と Press の組にし、
  `OSWKBAutoRepeatOff()` のときは捨てる。ホイールはボタン 4・5。
- キー: X11 版は、キーコード → キーシンボル（`XLookupKeysym`・`XkbKeycodeToKeysym`）→ その名前、で文字とキーの名前を決めている
  （`OSWGetASCIIFromKeyCode()` は引数の shift ではなく `osw_gui[0].shift_key_state` を見る、`grave` は '\0' など、癖もある）。
  SDL2 版も同じ結果になるように、Xvfb（evdev、us 配列）のキーマップ（キーコード 9〜135 の 2 段分のキーシンボル名）を
  `XGetKeyboardMapping()` で取り出して表として組み込み、同じ手順で引く。`osw_keycode.*` も `XKeysymToKeycode()` と同じ規則
  （段 0 で探し、無ければ段 1）で埋める。SDL のスキャンコードは evdev のキーコードに変換する。入力された文字は、SDL の TEXTINPUT が
  キーの押下の直後に来ていればそれを使う（JIS などの配列のため）。
- フォント: `scripts/gen-osw-fonts.py` が xfonts-base の PCF（`7x14-ISO8859-1.pcf.gz` など。`fonts.alias` の `7x14` などの実体）を読み、
  `src/include/osw-sdl-fonts.h` を作る。misc-fixed のライセンスは "Public domain font. Share and enjoy."。
  知らないフォント名は警告を出して 7x14 にする（X11 版は失敗する）。`font_t` の `char_width`・`char_height` は X11 版と同じく 0。
- 色: CLSP（`rgbi:0.80/0.10/0.80` など）を X は Xcms の強度表で変換していて、線形でもチャンネル間で同じでもない
  （0.4 は赤 177・緑 170・青 165）。Xvfb で `XParseColor()` を 0.00001 刻みでかけて、チャンネルごとに各 8 ビット値になる
  最小の強度（しきい値 255 個）を測り、表として組み込んだ。
- 確認（monitor、コンテナの Xvfb、SDL の x11 ドライバ）: X11 版と SDL2 版で同じ操作をしてスクリーンショットを比べた。
  起動直後・ボタンの強調表示（Enter/Leave）・右クリックのメニュー・メニューの項目の強調表示・選択の後は、画面全体で画素の差が 0。
  Messages ウィンドウ（新しいトップレベル）を開いた画面は、スクロールバーの矢印（`OSWDrawSolidArc()`）の縁の 33 画素だけが違う。
  X サーバーの円弧（mi の塗りつぶし）は、画素の中心が楕円の内側かという幾何学的な判定とは端の画素が少し違う。今は直していない。
- 最小化（2026-10-10 追記）: X ではウィンドウマネージャーが最小化した窓を Unmap し、client は UnmapNotify を受けると
  自分でもブリッジを `OSWUnmapWindow()` する。X ではすでに Unmap された窓なので何も起きず、アイコンから戻せる。
  SDL2 版はここで `SDL_HideWindow()` していたので、macOS の Dock から戻せなかった。最小化中のトップレベルへの Unmap は
  プログラムの側の状態を変えるだけにし、元に戻したとき（ウィンドウマネージャーが Map し直すのと同じ）は Map された状態に戻して
  MapNotify と Expose を送るようにした。最小化中の窓を Map したときは `SDL_RestoreWindow()` で戻す。macOS で確認した
  （Xvfb にはウィンドウマネージャーが無いので、コンテナでは試せない）。
- macOS の SDL2（Homebrew の sdl2-compat）では、`SDL_WINDOW_POPUP_MENU` を付けた窓は親が無いと作れない。付けずに作り直す。
- macOS で client・monitor・unvedit の SDL2 版を、この Mac の server につないで動かした（Retina での 2 倍表示、キー操作、
  メニュー、文字入力、閉じるボタン、最小化）。起動時に出る `error messaging the mach port for IMKCFRunLoopWakeUpReliable` は、
  macOS の入力メソッドの警告で、SDL のアプリで文字入力を有効にするとよく出る。害は無い。
- `Makefile.Darwin`（client・monitor・unvedit）の既定を SDL2 版にした（2026-10-10）。生成物は `build-darwin/`。
  XQuartz の X11 版は `GUI=x11` で作れ、生成物は `build-darwin-x11/`。`scripts/build.sh` のログは、`GUI=` を付けたとき
  `build-logs/darwin/<c>-<GUI>.log`（Linux では `build-logs/<c>-sdl.log`）。
