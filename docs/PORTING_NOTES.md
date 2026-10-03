# 移植メモ

移植中に判明した非自明な事項（型のサイズ・ファイル形式の前提・無効化した機能など）を記録する。

## server ビルド (2026-09-27)

- `Makefile.Linux` の `-D__cplusplus` / `-D__USE_BSD` は削除した。`__cplusplus` は g++ が自前で定義し、
  `__USE_BSD` は glibc の内部マクロで外から定義しても無視される（g++ は `_GNU_SOURCE` を既定で有効にする）。
- `strcasestr()`: glibc が提供しており、C++ では const/非 const のオーバーロードで宣言されるため、
  独自版（`src/global/string.cpp`）と宣言（`src/include/string.h`）は `#ifndef __GLIBC__` で除外した。
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
- `data/etc/xshipwarsrc` は 1.33 形式で、`JSCalibrationFile` が未知のパラメータとして警告される。
  また `UniverseListFile = .shipwars/universes` は `~/.shipwars/` からの相対パスとして解釈されるので、
  `~/.shipwars/.shipwars/universes` を探してしまう（`universes` とだけ書けば読まれる）。

## キー入力が効かない原因 (2026-10-03)

- 設定ファイルの `BeginKeyMap` ... `EndKeyMap` には、**X のキーコード（番号）がそのまま**書かれている。
  同梱の `data/etc/xshipwarsrc` の値は、旧 XFree86 キーボードドライバでの番号（TurnLeft=100, TurnRight=102,
  ThrottleIncrease=98, ThrottleDecrease=104, ThrottleIdle=107 など）。現在の Xorg/Xvfb（evdev/XKB）では
  Left=113, Right=114, Up=111, Down=116, Delete=119 なので、カーソル・ナビゲーションキーやテンキーに割り当てた操作が効かない。
  メインのキー（英数字、space=65、Esc=9、F1=67 など、番号 97 未満）は新旧で同じなので、そちらに割り当てた操作は効く。
- キーシンボルからキーコードを引く既定値（`KeymapWinSetDefault()`、`osw_keycode.*`）は、キー設定画面の
  「Default」ボタンを押したときにしか使われない。
- 対応: 同梱の `data/etc/xshipwarsrc` の `BeginKeyMap` で、新旧の番号が異なる 8 個を evdev の番号に書き換えた
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
