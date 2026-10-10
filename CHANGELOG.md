# Changelog

English | [日本語](CHANGELOG.ja.md)

All notable changes to this project are recorded in this file. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Version numbers are those of this port. The game's own version (shown by the programs, and written into
configuration and universe files) stays at the original XShipWars 1.34.0.

## 0.2.0

- Added macOS (Apple Silicon) support. The server, client, monitor and unvedit run on the Mac itself
  - The client, monitor and unvedit are shown with SDL2 (XQuartz is not needed). A new GUI layer reproduces the X11
    parts (the window hierarchy, events, drawing, fonts and keys) with SDL2. The screens and key mappings are the same
    as the X11 version (scaled up by a whole number on Retina displays)
  - Built with `Makefile.Darwin` (`scripts/build.sh` works on macOS too). Installed under `/usr/local` (programs in
    `/usr/local/bin`, data in `/usr/local/share/games/xshipwars`, the server in `/usr/local/swserv`)
  - Added a smoke test for macOS, `scripts/smoke-macos.sh`
  - On Linux too, `GUI=sdl` builds the SDL2 version, which `GUI=sdl scripts/smoke.sh` checks (for testing)
- Background music: a SoundFont placed at `sounds/soundfont.sf2` in the data is now used (macOS has no default place
  where a SoundFont is looked for)
- Installing: `scripts/install-data.sh` now sets `ToplevelDir` in the configuration template to the install location,
  and the server's `make install` sets `ServerToplevelDir` likewise, so installing with another `PREFIX` no longer
  needs the configuration edited by hand
- `scripts/install-data.sh` now works with bash 3.2 (macOS's `/bin/bash`)
- Fixed problems found by clang (the format for printing `off_t`, unused variables), and handled macOS's `df`
  reporting 512-byte blocks
- Fixed `scripts/build.sh` sometimes building nothing when given `clean all`

## 0.1.2

- Fixed `scripts/install-data.sh` failing on FreeBSD (it relied on GNU `install -D`, which means something else in FreeBSD's `install`)

## 0.1.1

- Fixed the client failing to build on FreeBSD (the joystick defaults were defined only on Linux)
- Fixed unused-variable warnings in the server and in the file browser when building on platforms other than Linux
- Added a build with `__linux__` undefined to the checks, so that the code paths FreeBSD uses are also compiled on Linux

## 0.1.0

- Ported XShipWars 1.34.0 so that the client, server, monitor and unvedit all build and run on modern Linux
  (Debian trixie / gcc 14, 64bit) (tested on arm64)
  - Fixed old C++ idioms and x86-only compiler flags; zero warnings under gcc 14's `-Wall`
  - Fixed code that broke on 64bit and arm64 (pointers stored in ints, code that relied on char being signed, and so on)
  - Replaced temporary file creation with mkstemp, and fixed string handling that could overflow buffers
- Brought the FreeBSD Makefiles in line with the Linux settings (untested on FreeBSD)
- Removed the code and build files for AIX, HP-UX, Solaris and Windows
- Sound is played with SDL2_mixer (the original YIFF / EsounD code was removed)
  - Sound effects (on by default in the shipped configuration)
  - Background music (MIDI; needs FluidSynth and a SoundFont; off by default)
- Joysticks are read with SDL2 (instead of the original libjsw; untested with real hardware)
- Updated the key mapping in the shipped configuration to current X server (evdev) keycodes
- With multiple monitors, dialogs open on the monitor of the program's main window
- The server's `make install` also creates the plugins directory
- Added scripts
  - `scripts/install-data.sh`: installs the data the client reads (images, sounds, settings)
  - `scripts/smoke.sh`: starts the four programs on Xvfb and checks their main functions automatically
  - `scripts/xquartz.sh`: shows the programs on macOS XQuartz from the Dev Container
- For known issues, see ["Open issues" in the README](README.md#open-issues)
