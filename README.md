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

### KotOR I — Milestones 1–4 complete ✅

Development has progressed through four formal milestones, each verified by CI
unit tests.  The playable path now runs from character creation on the _Endar
Spire_ through Taris and into the _Dantooine_ entry module (`danm13`).

**Milestone 1 — Endar Spire tutorial**
- Character creation: class selection, attribute/skill point allocation, appearance ✅
- Starting equipment looted from footlocker and equipped ✅
- Dialogue with Trask Ulgo and door interactions ✅
- Turn-based combat: d20 attack rolls, hit/miss animations, HP reduction, death ✅
- Module-exit script fires and returns to main menu ✅

**Milestone 2 — Taris entry (`tar_m02aa`)**
- Module transition from Endar Spire loads without script errors ✅
- Party formation (`AddPartyMember` / Carth joining) ✅
- XP accumulation and alignment system ✅
- Saving throws (Fortitude / Reflex / Will) with natural-1/20 rules ✅
- Global string / boolean / number variable persistence across area transitions ✅

**Milestone 3 — Taris Upper City**
- Faction reputation queries and adjustments (`GetReputation` / `AdjustReputation`) ✅
- Distance-based NPC AI range checks (`GetDistanceBetween`) ✅
- Buff/debuff effect types: AC increase, attack increase, skill increase, temporary HP ✅
- Item base-type query (`GetBaseItemType`) ✅
- Auto-assign level-up path so the PC gains HP and skills when `ShowLevelUpGUI` fires ✅

**Milestone 4 — Dantooine arrival (`danm13`)**
- Configurable start module (`KOTOR_startModule` config key) for developer iteration ✅
- Difficulty system (`GetGameDifficulty` / `GetDifficultyModifier`) ✅
- Cutscene choreography actions (`CutsceneAttack` / `CutsceneMove`) ✅
- Cutscene crowd-control effects (horrified, paralyze, stunned) ✅
- Plot flag system (`GetPlotFlag` / `SetPlotFlag`) for plot-critical NPCs ✅
- Camera-mode and dialog-orientation stubs so cutscene scripts proceed ✅

#### Honest caveats

A number of systems are stubs or partially implemented:

- **Level-up GUI** — skill points are auto-assigned; no feat selection or manual
  attribute spending yet.
- **OpenStore** — logs a warning and returns; no merchant/barter screen.
- **Cutscene camera** — `SetCameraMode` is a logged no-op; camera does not
  actually switch modes during cinematics.
- **Cinematic choreography flags** — `CutsceneAttack` queues the attack action
  but does not honour point-of-impact or miss-direction modifiers.
- **Audio** — ambient sound and voiced dialogue are not yet reliably playing for
  these areas.
- **Saving to disk** — in-session state is maintained across area transitions, but
  writing and reloading a full saved game is not yet supported.
- **~750 NWScript functions** remain unimplemented out of ~850 per game; areas
  beyond Dantooine will surface more stubs.

For the full acceptance-criteria checklist and upcoming work, see
[MILESTONE.md](MILESTONE.md) and [ROADMAP.md](ROADMAP.md).

### KotOR II

KotOR II shares the `kotorbase` engine layer and benefits from all NWScript
functions wired for KotOR I.  The prologue can be skipped and intro sequences
play.  Area rendering and walkmesh are functional.  The KotOR II GUI is
minimal compared to KotOR I — full option screens, advanced menus, and
influence/prestige systems are not yet implemented.

### Other games

Neverwinter Nights, Neverwinter Nights 2, Jade Empire, Sonic Chronicles,
The Witcher, Dragon Age: Origins, and Dragon Age II all reach an in-engine
spectator state (area geometry, objects, and walkmesh visible) via the
upstream xoreos foundation.  Some show partial menus.  Full gameplay for
those titles remains a long-term goal.

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
