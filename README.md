# Astryx

Astryx is a fork of [xoreos](https://github.com/xoreos/xoreos) — an open source
implementation of BioWare's Aurora engine and its derivatives — focused on pushing
KotOR I and KotOR II toward actual gameplay parity. It is licensed under the
[GNU General Public License version 3](https://www.gnu.org/licenses/gpl.html) (or
later).

The broader xoreos goal remains: all games built on the Aurora engine running
portably, from Neverwinter Nights through Dragon Age II. Astryx concentrates
current development effort on the KotOR games while keeping the full engine
foundation intact for the other targets.


## Supported Games

- [Neverwinter Nights](https://en.wikipedia.org/wiki/Neverwinter_Nights)
- [Neverwinter Nights 2](https://en.wikipedia.org/wiki/Neverwinter_Nights_2)
- [Knights of the Old Republic](https://en.wikipedia.org/wiki/Star_Wars:_Knights_of_the_Old_Republic)
- [Knights of the Old Republic II: The Sith Lords](https://en.wikipedia.org/wiki/Star_Wars:_Knights_of_the_Old_Republic_II_The_Sith_Lords)
- [Jade Empire](https://en.wikipedia.org/wiki/Jade_Empire)
- [Sonic Chronicles: The Dark Brotherhood](https://en.wikipedia.org/wiki/Sonic_Chronicles:_The_Dark_Brotherhood)
- [The Witcher](https://en.wikipedia.org/wiki/The_Witcher_%28video_game%29)
- [Dragon Age: Origins](https://en.wikipedia.org/wiki/Dragon_Age:_Origins)
- [Dragon Age II](https://en.wikipedia.org/wiki/Dragon_Age_II)


## Status

### KotOR I — Milestone 1 & 2 complete ✅

The _Endar Spire_ tutorial is fully playable end-to-end:

- Character creation (class, attributes, skills, appearance) ✅
- Starting equipment looted from footlocker and equipped ✅
- Dialogue with Trask Ulgo, door interactions ✅
- Turn-based combat with hit/miss animations, HP reduction, and death ✅
- Module-exit script fires and returns to main menu ✅

The first Taris module (`tar_m02aa`) now loads cleanly after the tutorial:

- Party formation (`AddPartyMember` / Carth joining) ✅
- XP accumulation and alignment system ✅
- Saving throws (Fortitude / Reflex / Will) ✅
- Global string / boolean / number variable persistence across area transitions ✅

For the full acceptance-criteria checklist and upcoming work, see
[MILESTONE.md](MILESTONE.md) and [ROADMAP.md](ROADMAP.md).

### Other games

All other targeted games show partial in-game graphics (area geometry, objects,
walkmesh) and can be explored in a "spectator mode". Some show partial menus.
Full gameplay for those titles remains a long-term goal.

Contributions are very welcome — please read [CONTRIBUTING.md](CONTRIBUTING.md)
before sending a pull request.


## CI Status

| Platform | Autotools | CMake |
|----------|-----------|-------|
| Linux (GCC) | [![linux autotools gcc](https://github.com/awest813/Astryx/actions/workflows/linux_autotools_gcc.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/linux_autotools_gcc.yml/) | [![linux cmake gcc](https://github.com/awest813/Astryx/actions/workflows/linux_cmake_gcc.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/linux_cmake_gcc.yml/) |
| Linux (Clang) | [![linux autotools clang](https://github.com/awest813/Astryx/actions/workflows/linux_autotools_clang.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/linux_autotools_clang.yml/) | [![linux cmake clang](https://github.com/awest813/Astryx/actions/workflows/linux_cmake_clang.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/linux_cmake_clang.yml/) |
| macOS (Clang) | [![macos autotools clang](https://github.com/awest813/Astryx/actions/workflows/macos_autotools_clang.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/macos_autotools_clang.yml/) | [![macos cmake clang](https://github.com/awest813/Astryx/actions/workflows/macos_cmake_clang.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/macos_cmake_clang.yml/) |
| Windows (MSVC) | — | [![windows cmake msvc](https://github.com/awest813/Astryx/actions/workflows/windows_cmake_msvc.yml/badge.svg)](https://github.com/awest813/Astryx/actions/workflows/windows_cmake_msvc.yml/) |


## Quick Start

The easiest path for both Windows and Linux is the repo-local `vcpkg` bootstrap.
It handles all dependencies automatically.

**Windows 10/11 x64** — open PowerShell and run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/bootstrap_windows.ps1
powershell -ExecutionPolicy Bypass -File scripts/run_xoreos_windows.ps1
# or use the graphical launcher:
python scripts/xoreos_launcher_gui.py
```

**Linux x86_64**:

```bash
./scripts/bootstrap_linux.sh
./scripts/run_xoreos_linux.sh
# or use the graphical launcher:
python3 scripts/xoreos_launcher_gui.py
```

The launcher scripts and GUI:

- list saved config targets from `xoreos.conf`
- scan common Steam/GOG install folders automatically
- allow manual path entry if nothing is detected
- validate common asset markers before launch
- save a reusable target into `xoreos.conf`
- support windowed mode, fullscreen, resolution, and skip-videos flags

**Non-interactive examples:**

```powershell
powershell -ExecutionPolicy Bypass -File scripts/run_xoreos_windows.ps1 -GamePath D:\Path\To\Game -Windowed -Width 1280 -Height 720
```

```bash
./scripts/run_xoreos_linux.sh --game-path /path/to/game --windowed --width 1280 --height 720
```

If you prefer to drive the CMake build manually, portable presets are available:

- `portable-vcpkg-ninja` — Linux/macOS or Windows Ninja builds
- `portable-vcpkg-ninja-multi` — Windows multi-config builds

For full dependency details and manual build instructions, see
[BUILDDEP.md](BUILDDEP.md).


## Running

First, make sure the game files are fully installed on your hard disk. Then start
xoreos via the launcher scripts above or directly from the command line:

```bash
# Linux / macOS
xoreos -p/path/to/game/

# Windows
xoreos -pD:\Path\To\Game\
```

Run `xoreos --help` for a complete list of command-line options.


## Config File

xoreos reads configuration from the command line, a config file, or both. A
"target" is a named section that identifies one installed game instance.

```ini
[xoreos]
width=1024
height=768
fullscreen=false

[nwn]
path=/home/user/games/nwn/
fullscreen=true
volume_music=0.500000
volume_sfx=0.850000
volume_voice=0.850000
```

Launch a target by name:

```bash
xoreos nwn
```

Config file locations:

| OS | Path |
|----|------|
| GNU/Linux | `$XDG_CONFIG_HOME/xoreos/xoreos.conf` (defaults to `~/.config/`) |
| macOS | `~/Library/Preferences/xoreos/xoreos.conf` |
| Windows | `%APPDATA%\xoreos\xoreos.conf` |

A fully annotated example is available at
[doc/xoreos.conf.example](doc/xoreos.conf.example).


## Links

- [Upstream xoreos repository](https://github.com/xoreos/xoreos)
- [xoreos wiki](https://wiki.xoreos.org/)
- [Astryx source repository](https://github.com/awest813/Astryx)
- [Milestone & roadmap](MILESTONE.md)
- [Parity roadmap](ROADMAP.md)
