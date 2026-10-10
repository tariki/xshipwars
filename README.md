# XShipWars — a port to modern Linux, FreeBSD and macOS

English | [日本語](README.ja.md)

XShipWars is a networked space ship combat game for the X Window System, developed by WolfPack Entertainment
from 1999 to 2001 (this repository starts from version 1.34.0). This repository makes it build and run on current
Linux (Debian trixie / gcc 14 / 64bit) and on macOS.
The game's behavior, network protocol and data file formats are unchanged.

The original source was obtained from the xshipwars source package 1.34.1-3 in the Debian snapshot archive
(https://snapshot.debian.org/package/xshipwars/1.34.1-3/). Debian's patches were not applied (the programs show
version 1.34.0).

| Program | What it is |
|---|---|
| `swserv` | The server. Manages the universe and accepts client connections (port 1701 by default, 1702 for monitoring) |
| `xsw` | The client. Connects to a server and lets you pilot a ship (shown with X11 on Linux and FreeBSD, with SDL2 on macOS) |
| `monitor` | A monitoring tool that shows the server's state (connections, object count, log) |
| `unvedit` | An editor for universe files (`.unv`) |

It has been tested on Debian trixie (arm64) in a Dev Container and on macOS (Apple Silicon). On Linux, it has been
shown on Xvfb and on macOS XQuartz. It has not been tried on x86_64, but the 64bit fixes (fixed-width types, char
signedness and so on) apply to both.

**Supported platforms are Linux, FreeBSD and macOS.** The FreeBSD Makefiles use the same settings as the Linux ones,
but have not been tried on FreeBSD itself yet. On macOS all four programs run. The client, monitor and unvedit are shown
with SDL2 rather than X11 (XQuartz is not needed; a new layer reproduces the X11 parts with SDL2). Intel Macs have not
been tried.
The AIX, HP-UX and Solaris build files and code from the original distribution, and the remains of a Windows port,
have been removed (there is no environment to test them on, so they cannot be maintained). On Windows, running the
Linux version under WSL2 is the practical option (untested).

---

## Playing

### Requirements

On Debian / Ubuntu, install these packages:

```sh
sudo apt install build-essential libx11-dev libxext-dev libxpm-dev libxinerama-dev libsdl2-mixer-dev xfonts-base \
  libfluidsynth3 timgm6mb-soundfont
```

`xfonts-base` is needed at run time: the client does not start without the X core fonts `7x14` and `6x10`.
`libfluidsynth3` and `timgm6mb-soundfont` are needed only for background music (MIDI) (see "Sound").

On FreeBSD, these packages should be needed (untested). The scripts (`scripts/*.sh`) also need `bash`.

```sh
pkg install libX11 libXext libXpm libXinerama sdl2_mixer font-misc-misc
```

Background music also needs FluidSynth and a GM SoundFont (see "Sound"; untested).

On macOS, install the Xcode Command Line Tools (`xcode-select --install`) and these Homebrew packages.
FluidSynth comes with SDL2_mixer. X11 and XQuartz are not needed.

```sh
brew install sdl2 sdl2_mixer pkg-config
```

### Building

```sh
make -C src/server  -f Makefile.Linux
make -C src/client  -f Makefile.Linux
make -C src/monitor -f Makefile.Linux
make -C src/unvedit -f Makefile.Linux
```

This builds `src/server/swserv`, `src/client/xsw`, `src/monitor/monitor` and `src/unvedit/unvedit`.
Inside the Dev Container, `scripts/build.sh all` does the same (and keeps logs in `build-logs/`).

On FreeBSD, use `Makefile.FreeBSD` instead of `Makefile.Linux` (untested). It compiles with `c++` (clang) and
looks for X11 under `/usr/local`. If BSD make has trouble with it, try GNU make (`gmake`).
The install locations are the same as on Linux (programs in `/usr/games`, data in `/usr/share/games/xshipwars`).

On macOS, use `Makefile.Darwin` (`scripts/build.sh all` does the same). This builds `swserv`, `xsw`, `monitor` and
`unvedit` under `src/<program>/build-darwin/`.

```sh
for c in server client monitor unvedit; do make -C src/$c -f Makefile.Darwin; done
```

### Installing (system-wide)

```sh
sudo make -C src/client  -f Makefile.Linux install   # /usr/games/xsw and the configuration template
sudo make -C src/monitor -f Makefile.Linux install   # /usr/games/monitor
sudo make -C src/unvedit -f Makefile.Linux install   # /usr/games/unvedit
sudo make -C src/server  -f Makefile.Linux install   # under /home/swserv/
sudo scripts/install-data.sh                          # images, sounds, settings -> /usr/share/games/xshipwars
```

- `scripts/install-data.sh` copies `data/` (client data) and `theme/` (images and sounds) into the layout the client
  reads (`/usr/share/games/xshipwars/{etc,images,sounds}`). With `-n` it only shows what it would install.
- Make the server's files owned by the user who runs the server: `sudo chown -R $USER /home/swserv`
- The server's `make install` copies with `cp -i`, so a reinstall asks before overwriting existing files.
  `ServerToplevelDir` in `etc/default.conf` is set to the install location.

macOS cannot use `/usr/share` or `/home`, so everything goes under `/usr/local` (the client, monitor and unvedit in
`/usr/local/bin`, the data in `/usr/local/share/games/xshipwars`, the server in `/usr/local/swserv`).

```sh
for c in client monitor unvedit server; do sudo make -C src/$c -f Makefile.Darwin install; done
sudo scripts/install-data.sh            # goes to /usr/local/share/games/xshipwars on macOS
sudo chown -R $USER /usr/local/swserv
```

The install location is compiled into the programs (as the default place to look for the data and the server). If you
change `PREFIX`, give the same `PREFIX` when building and when installing (to rebuild with another one, run
`make -f Makefile.Darwin clean` first).

### Starting the server

```sh
cd /home/swserv          # /usr/local/swserv on macOS
bin/swserv --fg etc/default.conf
```

- With `--fg` it runs in the foreground. Without it, it runs in the background as a daemon.
- Stop it with Ctrl-C (or SIGTERM). It saves the universe to `db/generic_out.unv` before exiting.
- At startup it reads `db/generic_in.unv`. To continue from the previous state (including accounts you created),
  swap the files with `mv db/generic_out.unv db/generic_in.unv` before starting. Otherwise the previous changes are lost
  (the bundled `restart` is a sample csh script that does this).
- The log goes to `logs/generic.log`. The settings are in `etc/default.conf`, where the port numbers can be changed too.

### Connecting with the client

```sh
/usr/games/xsw                                      # pick a universe from the list and double-click to connect (/usr/local/bin/xsw on macOS)
/usr/games/xsw swserv://Guest:guest@localhost:1701  # connect right away to the given URL
```

- On first start, `/usr/share/games/xshipwars/etc/xshipwarsrc` (`/usr/local/share/...` on macOS) is copied to `~/.shipwars/xshipwarsrc`, and that copy
  is used from then on. Settings are saved when the client exits normally (closing the bridge window, or answering y to
  the "Exit?:" prompt from the x key).
- Connecting needs a name and a password (see "Logging in and accounts" below).
- Right-clicking the bridge window opens the quick menu (Connect, Refresh, Star Chart, Economy, Options and so on).

Main keys (default mapping):

| Key | Action | Key | Action |
|---|---|---|---|
| ← / → | Turn | space | Fire weapon |
| ↑ / ↓ | Throttle up / down | Tab | Lock weapons |
| Delete | Throttle to idle | s | Shields |
| F8 / Shift+F8 | Cycle throttle mode | ` | Engine on / off |
| = / - | Viewscreen zoom | F1 / x | Help / Exit (asks first) |

**If keys don't work**: the configuration file stores keys as X keycodes (numbers), and these numbers differ between
X servers (the shipped values are Xorg's evdev keycodes; on macOS XQuartz they are all different).
In that case, right-click → Options... → General tab → "Map Keyboard" to open the Key Mappings window, then press
"Default All" and then "OK" ("Default All" alone does not apply the change).
The same fixes an old `~/.shipwars/xshipwarsrc` left over from an earlier version.
The macOS version (SDL2) converts keys to Xorg's evdev keycodes, so the shipped values work as they are.

### Logging in and accounts

An account is a "player" type ship in the universe (the universe file). Each ship stores a name and a password
(encrypted with crypt), and logging in means choosing, by name and password, which ship to pilot. The server accepts
two kinds of login:

| Kind | Name | Password | Ship you pilot |
|---|---|---|---|
| Guest | `Guest` (case-insensitive) | **Not checked** (anything is accepted) | A new "Guest 1", "Guest 2", ... is created on each login and removed on disconnect. Up to 5 at a time |
| Registered player | The name of a player ship in the universe | Checked against that ship's password | An existing ship. It stays after you disconnect, and you get the same ship next time |

- The `guest` in the URL `swserv://Guest:guest@...` only fills the password field and means nothing.
  Whether guests are accepted, and how many, is set by `AllowGuestLogins`, `GuestLoginName` and `MaxGuests` in the
  server's `etc/default.conf`.
- The bundled universe has two registered player ships: **Defiant** and **IKS Dark Vixen**. Both have `*` as the
  password, a special value that accepts any password. Both have administrator rights (access level 0).
- With a wrong name or password, the client shows "Either that player does not exist, or has a different password."

**Creating your own account**: use the server command `createplayer <name>=<password>`.
It needs access level 1 or better (smaller numbers are more powerful), so guests cannot use it.

1. Log in with a ship that has administrator rights: `xsw swserv://Defiant:x@localhost:1701` (any password works)
2. Press `e` to get a "Server Command:" field below the viewscreen, type `createplayer myname=mypass` and press Enter
   (the message area shows "Created new player myname(#<number>P)")
3. From then on, log in with `xsw swserv://myname:mypass@localhost:1701` (the new ship has access level 5)

Accounts you create are saved to `db/generic_out.unv` when the server stops. Unless you swap it into
`generic_in.unv` before the next start, the accounts are lost (see "Starting the server" above).

**How to give them to the client**: use a URL (`swserv://name:password@host:port`), or start without arguments and
pick from the universe list. Each list entry is stored as a URL in `~/.shipwars/universes`; the bundled ones include
"Localhost: Guest" (`Guest:guest`) and "Localhost: Defiant" (`Defiant:yiffbaby`).

### monitor and unvedit

```sh
/usr/games/monitor -u <name> <password> localhost 1702   # put -u before the address
/usr/games/unvedit /home/swserv/db/generic_in.unv
```

On macOS they are `/usr/local/bin/monitor`, `/usr/local/bin/unvedit` and `/usr/local/swserv/db/generic_in.unv`.

- monitor's arguments are order-sensitive. The name and password are those of a registered player, as for the client;
  with the bundled universe, `-u Defiant x` works. Without `-u` it connects as Guest, which shows only the statistics,
  not the server log (the Messages window). If the images are elsewhere, add `-i <image directory>`.
- unvedit's settings are in `~/.shipwars/unveditrc`.

### Installing into your home directory without root

`PREFIX` changes the install location. The server's `ServerToplevelDir`, and `ToplevelDir` in the configuration
template among the data, are set to the install location when installing. On Linux the client copies its
configuration on first start only from the default location (`/usr/share/games/xshipwars/etc`), so copy it yourself.

```sh
P=$HOME/xshipwars
for c in client monitor unvedit server; do make -C src/$c -f Makefile.Linux install PREFIX=$P; done
PREFIX=$P scripts/install-data.sh

# client: put the installed configuration template in place (its ToplevelDir is the install location)
mkdir -p ~/.shipwars
cp $P/share/games/xshipwars/etc/{xshipwarsrc,universes} ~/.shipwars/

(cd $P/swserv && bin/swserv --fg etc/default.conf) &
$P/games/xsw swserv://Guest:guest@localhost:1701
```

On macOS the install location is compiled into the programs, so giving the same `PREFIX` from the build on means no
configuration file needs copying (the programs go to `$P/bin`).

```sh
P=$HOME/xshipwars
for c in client monitor unvedit server; do
  make -C src/$c -f Makefile.Darwin clean      # remove what was built with another PREFIX
  make -C src/$c -f Makefile.Darwin PREFIX=$P all install
done
PREFIX=$P scripts/install-data.sh
```

### Playing on macOS

On macOS, the steps above (requirements, building, installing) let you run all four programs on the Mac itself.
The client, monitor and unvedit are shown with SDL2, so XQuartz is not needed.

- The screens are drawn at the same size and look the same as the Linux version. On Retina displays they are scaled up
  by a whole number so they stay sharp.
- Key mappings follow the position of the key on the keyboard (its position on a US layout). On a JIS keyboard, some
  symbol keys, such as `=` for zoom, are in different places from a US layout. Typing text (such as the URL to connect
  to) follows the macOS keyboard settings.
- "Run Server..." on the client's quick menu opens `/usr/local/swserv`.
- To play background music, put a SoundFont in place (see "Sound" below).

### Sound

Sound effects and background music are played with SDL2_mixer (it replaces the original YIFF / EsounD sound servers,
which no longer exist).
With the shipped configuration, sound is on from the start (`SoundServerType = 4`, `Sounds = 3`). To change it, use either:

- Client right-click → Options... → Sounds tab: Sound Server Type ("SDL" or "None") and Amount level
- `SoundServerType` (4: SDL, 0: none) and `Sounds` (0: none, 1: events, 2: + engine, 3: all; currently 1 to 3 play
  the same sounds) in `~/.shipwars/xshipwarsrc`

An old `~/.shipwars/xshipwarsrc` left over from an earlier version keeps sound off, so turn it on as above.

"Test Sound" on the Sounds tab plays a sound on the left, the right, and both. If sound fails to initialize (for
example, with no audio device), the client prints a message (possibly followed by ALSA warnings), turns sound off
and keeps running. In that case, `Sounds = 0` is saved to the configuration file when the client exits, so turn it
on again as above when you play somewhere with an audio device.

- **Background music (MIDI) is off by default.** To turn it on, switch on Music on the Options... → Sounds tab, or set
  `Music = on` in `~/.shipwars/xshipwarsrc`. The music changes with the situation (normal, inside a nebula, combat)
  and does not play in the main menu (as in the original game).
  - MIDI needs FluidSynth, which SDL2_mixer uses, and a GM SoundFont. On Debian / Ubuntu, installing
    `libfluidsynth3` and `timgm6mb-soundfont` (about 6MB) is enough, with no further setup.
    Another SoundFont registered as `/usr/share/sounds/sf3/default-GM.sf3`, such as `fluid-soundfont-gm`
    (about 140MB), works too.
  - To use a SoundFont elsewhere, set the environment variable `SDL_SOUNDFONTS=<path to .sf2>`
    (if there is also a SoundFont in the default location above, `SDL_FORCE_SOUNDFONTS=1` is needed as well).
  - Without a SoundFont, the client prints a message such as `bluedanube.mid: Couldn't open timidity.cfg` and runs
    without music. For some songs FluidSynth prints a few dozen lines of
    `Ignoring unrecognized meta event type 0x21`; they are harmless.
  - On FreeBSD, installing `fluidsynth` and a SoundFont (such as `fluid-soundfont`), and pointing `SDL_SOUNDFONTS`
    at it if needed, should work (untested).
  - A SoundFont named `soundfont.sf2` in the data's `sounds/` is used (`SDL_SOUNDFONTS`, when set, takes precedence).
    **On macOS there is no default place where a SoundFont is looked for**, so put a GM SoundFont (.sf2, such as
    TimGM6mb, the same as Debian's `timgm6mb-soundfont`, or GeneralUser GS) at
    `/usr/local/share/games/xshipwars/sounds/soundfont.sf2`. No SoundFont is included in this repository.
- **You cannot hear sound inside Docker (the Dev Container) on macOS.** Docker Desktop containers have no audio
  output, so there is no sound even when playing through XQuartz. To hear it, use the macOS version, or run the game
  directly on a Linux or FreeBSD desktop.

### Joystick

Joysticks (gamepads) are read with SDL2 (it replaces the original libjsw library, which no longer exists).
**Not yet tested with real hardware** (the Dev Container has no input devices; it was tested with SDL's virtual
joystick).

1. Client right-click → Options... → General tab → "Map Joystick" to open the mapping window
2. Add a joystick with "Add", enter its number as the Device (see below) and press "Refresh" to load its axes and buttons
3. Enter an axis number in the field of each function (Turn, Throttle and so on); for buttons, select one in the list
   and assign a key with "Scan Key" (pressing the button then acts as pressing that key)
4. Set Controller Type on the General tab to "Joystick"

- **Devices are chosen by number.** The trailing number is the SDL joystick number (from 0), so `/dev/js0` and
  `/dev/input/js1` from old configurations still work as joysticks 0 and 1.
- Axis numbers are SDL's; hats (D-pads) follow the axes as two axes each (horizontal, then vertical).
- The null zone around the center is fixed at 20% of the range (libjsw's default). libjsw calibration files
  (`JSCalibrationFile`, `~/.joystick`) are not read (the setting is kept and still saved).
- Axes cannot be reversed. In SDL, pushing a stick forward gives negative values, so an axis mapped to Throttle
  **increases thrust when pulled back** (in Normal mode; the original probably relied on libjsw calibration to
  reverse it).
- On Linux, read access to `/dev/input/event*` is needed (on a normal desktop the logged-in user gets it
  automatically). On FreeBSD, gamepads that SDL supports should work (untested).
- Inside Docker (the Dev Container) on macOS, the host's USB devices are not visible, so joysticks cannot be used.
  The macOS version should work with gamepads that SDL2 supports (untested).

### Known limitations

- **Joysticks are untested with real hardware.** The Throttle axis cannot be reversed (see above).
- **Key mappings depend on the X server** (see above).
- On slow displays (such as XQuartz over a network), object names may not arrive right after login and show up as
  "Object 3" and the like. "Refresh" on the quick menu fixes it.
- With multiple monitors, dialogs open on the monitor of the bridge window (for monitor and unvedit, their first window)
  (X11 version only; on macOS they open where they were created).
- In the macOS version (SDL2), a few pixels at the edges of filled arcs (such as the tips of scroll bar arrows) differ
  slightly from the X11 version. Stacking order between toplevel windows, and attaching dialogs to their parent window
  (transient), have no effect.
- In the macOS version, the memory display in the client's options window shows 0, and the server does not load
  plugins (as on FreeBSD).

---

## For developers

### Development environment (Dev Container)

`.devcontainer/` has a Dev Container based on Debian trixie (open it with VS Code Dev Containers or similar).

- It has build and debugging tools (gcc 14, gdb, valgrind, strace), the X11 development libraries, Xvfb, xdotool
  and ImageMagick.
- At startup, `init-firewall.sh` restricts outgoing connections (only some destinations such as GitHub, npm and the
  Anthropic API, and `host.docker.internal:6000` for XQuartz on a macOS host, are allowed). apt cannot be used inside
  the container, so to add packages, add them to `.devcontainer/Dockerfile` and rebuild.
- On an Apple Silicon host it runs as **arm64**. On arm64 Linux, `char` is unsigned, so take care (see below).
- The macOS version is built on macOS (the host), not in the container. The host and the container share the work
  tree, but the macOS build goes to `build-darwin/`, so it does not mix with the Linux build (made next to the sources).

### Directory layout

| Path | Contents |
|---|---|
| `src/server/`, `src/client/`, `src/monitor/`, `src/unvedit/` | The programs. Build with `Makefile.Linux`, `Makefile.FreeBSD` or `Makefile.Darwin` |
| `src/include/`, `src/global/`, `src/widgets/` | Shared headers, utilities and the custom GUI widgets. Many `.cpp` files of each program are symbolic links to these |
| `src/global/osw-x.cpp`, `src/global/osw-sdl.cpp` | The X11 and SDL2 versions of the GUI layer (OSW). The shared declarations are in `src/include/osw-api.h` |
| `src/pconf/`, `src/*/platforms.ini` | The original Makefile generator (no longer used) |
| `data/` | Client data (pages, OCS names and so on in `etc/`, client images in `images/`) |
| `theme/` | The graphics theme (celestial objects, ships and so on in `images/`, and `sounds/`) |
| `scripts/` | Scripts for building, running and testing (see below) |
| `docs/PORTING_NOTES.md` | The porting record: what was broken, how it was fixed, and what was checked (in Japanese) |
| `CLAUDE.md` | The porting policy (also the instructions for AI agents; in Japanese) |
| `build-logs/`, `run-logs/` | Build logs and run-time working directories (not in git) |

### Build system

- Each program's `src/<component>/Makefile.Linux` (`Makefile.Darwin` on macOS) is used directly. Generating them with
  the original `pconf`, or moving to CMake, has not been started.
- There are two GUI layers. `GUI=sdl` uses the SDL2 one (`osw-sdl.cpp`), `GUI=x11` the X11 one (`osw-x.cpp`).
  The default is X11 on Linux and FreeBSD and SDL2 on macOS. The Linux SDL2 build (in `build-linux-sdl/`) is for
  checking the macOS version in the container; the X11 build for XQuartz on macOS goes to `build-darwin-x11/`.
- The basic compiler options are `-O2 -g -Wall`. All four programs build with **zero warnings** under the `-Wall` of
  gcc 14 and of macOS's clang. No blanket suppression with `-Wno-*`, `-fpermissive` or `-w` is used. The exception is
  the macOS SDK marking `sprintf` deprecated, which is suppressed with `-Wno-deprecated-declarations` on macOS only
  (it warns at every call).
- Main defines:
  - `USE_XSHM`: draw with MIT-SHM. For an X server over the network, disable it at run time with `--no_xshm`
  - `HAVE_XINERAMA`: place windows with multiple monitors in mind (`-lXinerama`)
  - `PLUGIN_SUPPORT` (server)
  - `HAVE_SDL_MIXER` (client): play sound effects and background music with SDL2_mixer. The flags come from
    `pkg-config SDL2_mixer`
  - `JS_SUPPORT` (client): read joysticks with SDL2 (`jsw-sdl.cpp` instead of libjsw). The SDL2 flags come from
    SDL2_mixer's `pkg-config`, so dropping `HAVE_SDL_MIXER` means adding the SDL2 flags separately
  - `HAVE_YIFF` and `HAVE_ESD` for YIFF / ESD, and their code, have been removed
  - `OSW_SDL`: use the SDL2 GUI layer (set with `GUI=sdl`)
  - `XSW_DATA_DIR`, `SWSERV_DIR`: the data and server locations compiled into the programs (`src/include/xsw-paths.h`;
    `Makefile.Darwin` sets them from PREFIX)
- **The Makefiles do not track header dependencies.** After changing a header, run `make -f Makefile.Linux clean`
  before building.

### Scripts

| Script | Purpose |
|---|---|
| `scripts/build.sh <server\|client\|monitor\|unvedit\|all> [make arguments]` | Build (inside the Dev Container, and on macOS). Logs go to `build-logs/<component>.log` (`build-logs/darwin/` on macOS); prints the error and warning counts at the end |
| `scripts/smoke.sh` | Smoke test (`GUI=sdl` for the SDL2 builds). See below |
| `scripts/smoke-macos.sh` | Smoke test on macOS. See below |
| `scripts/gen-osw-fonts.py` | Makes the fonts built into the SDL2 version (`src/include/osw-sdl-fonts.h`) from X's misc-fixed fonts |
| `scripts/headless.sh start\|shot <png>\|key <keysym>\|stop` | Start Xvfb (:99), take a screenshot, send a key, stop |
| `scripts/install-data.sh [-n] [install location]` | Install the data |
| `scripts/xquartz.sh [client] [monitor] [unvedit]` | Show the Linux version in the Dev Container on the macOS host's XQuartz (see below) |

### Showing the Dev Container's Linux version on XQuartz

Instead of the macOS version, you can also show the Linux version inside the Dev Container on macOS's XQuartz (for
checking the Linux version).

1. In XQuartz Settings → Security, turn on "Allow connections from network clients", then restart XQuartz
2. Run `xhost +localhost` in an XQuartz xterm
3. In the container, run `scripts/build.sh all` and then `scripts/xquartz.sh` (to also show monitor and unvedit,
   `scripts/xquartz.sh client monitor unvedit`)
4. The first time only, remap the keys as described in "If keys don't work" above, and close the bridge window to save

The run environment is created in `run-logs/xquartz/`, and the key mapping and universe state carry over to the next run.

### Tests

`scripts/smoke.sh` recreates its run environment in `run-logs/smoke/` each time and checks the following
automatically on Xvfb (it prints PASS/FAIL and exits with status 1 if anything fails). Run it after making changes.

- The data installs with `install-data.sh`
- The server listens on 1701/1702 and shuts down normally
- monitor logs in to the AUX port and receives statistics
- The client logs in as Guest, and turning, accelerating and reverse-cycling the throttle mode (wrapping from Normal
  to Incremental) work (judged by reading the client's internal values with gdb)
- An object with `EngineState = -1` stays -1 through saves by the server and by unvedit (a regression test for
  arm64's char signedness)
- A file opened and saved again with unvedit is identical to the original
- The client's sound effects come out of SDL2_mixer (SDL's disk output, `SDL_AUDIODRIVER=disk`, writes them to a file,
  which is checked for the logo sound at startup)
- The client's background music (MIDI) plays, switches songs, and stops for a mood with no song (also checks that,
  after a switch made from gdb inside the client, the game switches back to the normal song)
- A joystick axis turns the ship, and the key mapped to a button (F8, cycle throttle mode) works (an SDL virtual
  joystick is attached and operated from gdb)

`GUI=sdl scripts/smoke.sh` checks the same things with the SDL2 builds of the client, monitor and unvedit (built with
`scripts/build.sh <c> GUI=sdl`; results in `run-logs/smoke-sdl/`). The SDL2 windows are X windows on Xvfb too, so
xdotool works on them.

On macOS, use `scripts/smoke-macos.sh`. It runs with SDL's dummy video driver so that no windows show up, and checks
the data install, the server starting and shutting down, the monitor connecting, the client logging in and playing
sound effects, `EngineState = -1` surviving the server's save, and unvedit loading and saving (with the emergency save
on exit). Check keys and the screen by hand.

Tips for investigating headlessly:

- There is no window manager, so give the bridge window focus with `xdotool windowfocus --sync <bridge window>`
  before sending game keys
- The client has debugging information, so `gdb -batch -p <PID> -ex 'print <expression>'` reads its internal values
- gcc's diagnostic column numbers are display columns with tabs expanded to 8

### Porting principles

See `CLAUDE.md` and `docs/PORTING_NOTES.md` for details (both in Japanese). The main points:

- **Protocols and file formats are not changed.** Client↔server and AUX communication, and the universe and
  configuration files, are all text. Nothing writes structs out as raw binary (confirmed in the 64bit audit).
- **64bit**: don't assume the size of long or pointers. Integers stored in pointers go through `intptr_t`.
- **char on arm64**: `char` is unsigned. Variables that hold negative values (such as -1) are `signed char`
  (for example `engine_state` and the client's `option.throttle_mode`). A build with `-Wtype-limits` finds them.
- No casts just to silence warnings. The one exception is a single call into libXpm's API, which does not take const.
- The macOS GUI (SDL2) is made to give the same screens as the X11 version. The fonts are X's misc-fixed built in,
  the color conversion (`rgbi:`) reproduces X's measured values, and keys are handled as X keycodes. This is checked
  by comparing screenshots of the X11 and SDL2 versions on Xvfb pixel by pixel (see `docs/PORTING_NOTES.md`).
- Commits are kept small, and their messages say what was broken in the modern environment. Non-obvious findings
  are added to `docs/PORTING_NOTES.md`.

### Open issues

- Testing joysticks with real hardware, and whether axes need reversing
- Reworking the build system (header dependencies, reworking pconf or moving to CMake)
- Making key mappings independent of the X server (on hold, since it affects the configuration file format)
- Finding out why object names are sometimes missed right after login (suspected send queue overflow)
- The server's `restart` script is csh and assumes `etc/generic.conf`
- FreeBSD: there is no `free` command, so the memory display in the client's options window shows 0
- FreeBSD: the server loads plugins only on Linux (dlopen works on FreeBSD too, but enabling it would add a feature,
  so it is left as is)
- macOS version: making the edge pixels of filled arcs match the X server's (mi), distribution as an `.app`, stacking
  order of toplevels and transient windows, dialog placement with multiple monitors, the memory display (there is no
  `free` command), and server plugins
- The background music file `theme/sounds/aquarium.mid` carries the notice "Copyright © 1999 by Ramon Pajares Box -
  All Rights Reserved" (the music is by Saint-Saëns, and the file was in the original distribution). Whether to keep
  or replace it is undecided

---

## License

The original XShipWars is distributed under the GNU General Public License version 2 with an additional clause
(full text in `src/LICENSE`). The clause requires that modified versions, when distributed, clearly state what was
changed. The changes in this repository are recorded in the git history and in `docs/PORTING_NOTES.md`.
The original authors and contributors are listed in `src/CREDITS`.
