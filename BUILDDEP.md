Building Astryx
===============

Astryx builds with CMake (3.21+ for the presets, 3.10+ otherwise), Ninja and
a C++17 compiler. Pick one way to get the libraries:

| Path | Script | Build dir | When to use |
|------|--------|-----------|-------------|
| System packages (Linux) | `scripts/build_ci_linux.sh` | `build-linux-ci` | Fastest. Same setup as CI. Use this on Debian/Ubuntu, Fedora and Arch. |
| vcpkg (Linux/macOS) | `scripts/bootstrap_linux.sh` | `build-vcpkg-linux` | Self-contained: builds every library from source into `.deps/vcpkg`. Slow first run. Use it if your distribution's packages are missing or too old. |
| vcpkg (Windows) | `scripts/bootstrap_windows.ps1` | `build-vcpkg-portable` | The normal Windows path. |

Every script takes `--help` (PowerShell: `Get-Help scripts\bootstrap_windows.ps1 -Detailed`),
accepts `--build-dir`/`-BuildDir` (absolute, or relative to the repository
root) and `--test`/`-Test` to also run the unit tests.

Prerequisites
-------------

Copy the line for your system.

**Debian / Ubuntu** (system packages; `scripts/build_ci_linux.sh --install-deps` runs this for you):

    sudo apt-get install build-essential cmake ninja-build git pkg-config libboost-all-dev zlib1g-dev liblzma-dev libxml2-dev libsdl2-dev libopenal-dev libfreetype-dev libgl-dev libmad0-dev libogg-dev libvorbis-dev libfaad-dev libxvidcore-dev libvpx-dev

**Fedora** (`--install-deps` works here too; for XviD video also install
`xvidcore-devel` from RPM Fusion):

    sudo dnf install gcc-c++ cmake ninja-build git pkgconf-pkg-config boost-devel zlib-devel xz-devel libxml2-devel SDL2-devel openal-soft-devel freetype-devel mesa-libGL-devel libmad-devel libogg-devel libvorbis-devel faad2-devel libvpx-devel

**Arch** (`--install-deps` works here too):

    sudo pacman -S --needed base-devel cmake ninja git pkgconf boost zlib xz libxml2 sdl2 openal freetype2 mesa libmad libogg libvorbis faad2 xvidcore libvpx

**Linux, vcpkg path instead** (tools for vcpkg plus the few system libraries
it does not provide: OpenGL, X11 headers for SDL2, sound-server headers for
OpenAL Soft, and `nasm` for libvpx). Debian/Ubuntu shown; the script prints
the Fedora/Arch equivalent when something is missing:

    sudo apt-get install build-essential cmake ninja-build git pkg-config curl zip unzip tar nasm libgl-dev libx11-dev libxext-dev libxft-dev libasound2-dev libpulse-dev

**Windows 10/11 x64** (then open a new terminal):

    winget install Git.Git; winget install Kitware.CMake; winget install Ninja-build.Ninja
    winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Workload.VCTools --includeRecommended --passive"

Visual Studio 2022 or 2026 Community with "Desktop development with C++"
works as well. Ninja is optional; without it the Visual Studio generator is used.

Build, test, run
----------------

    scripts/build_ci_linux.sh --test          # Linux, system packages
    scripts/bootstrap_linux.sh --test         # Linux/macOS, vcpkg
    powershell -ExecutionPolicy Bypass -File scripts\bootstrap_windows.ps1 -Test

- Binary: `<build-dir>/bin/xoreos` (Linux) or
  `<build-dir>\bin\<Debug|Release>\xoreos.exe` (Windows).
- Unit tests in an existing build dir: `cmake --build <build-dir> --target check`
  (add `--config Debug` on Windows).
- Launch: `scripts/run_xoreos_linux.sh` / `scripts\run_xoreos_windows.ps1`, or
  the graphical `scripts/xoreos_launcher_gui.py`. Each script prints the exact
  command when it finishes.

Plain CMake works too. With presets (`cmake --list-presets=all`):

    cmake --preset linux-system && cmake --build --preset linux-system-check

or by hand:

    cmake -S . -B build -G Ninja && cmake --build build && cmake --build build --target check

If configuration fails because a library is missing, CMake prints the
package name for Debian/Ubuntu, Fedora, Arch and vcpkg, plus an install
command for the system it detected.

Library dependencies
====================

Required:

- iconv (part of glibc on Linux; on Windows an internal fallback is used)
- zlib (>= 1.2.3)
- liblzma (>= 5.0.3), needed for KotOR. Configure stops without it unless
  KotOR is disabled with `-DENABLE_KOTOR=OFF -DENABLE_KOTOR2=OFF`.
- Boost (>= 1.53.0)
  - compiled: Boost.System, Boost.Filesystem, Boost.Date_Time, Boost.Locale
  - header-only: Boost.Utility, Boost.StringAlgo, Boost.Uuid, Boost.Smart_Ptr,
    Boost.ScopeExit, Boost.TypeTraits
- OpenGL (>= 2.1)
- SDL2 (>= 2.0.4)
- FreeType 2 (>= 2.4.0 (libtool number >= 11.0.5))
- OpenAL Soft (>= 1.12)

Optional (configure warns and lists what was disabled if they are missing;
turn one off on purpose with `-DENABLE_<NAME>=OFF`):

- MAD (>= 0.15.1b), MP3 audio, used for KotOR music and voice-over (`ENABLE_MAD`)
- libogg (>= 1.2.0), libvorbis and libvorbisfile (>= 1.3.1), Ogg Vorbis audio (`ENABLE_VORBIS`)
- libfaad (>= 2.7), AAC audio (`ENABLE_FAAD`)
- libxvidcore (>= 1.2.2), XviD/H.263 video (`ENABLE_XVIDCORE`; no vcpkg port)
- libvpx (>= 1.6.0), VP8/VP9 video (`ENABLE_VPX`)
- libxml2 (>= 2.8.0), only needed by the Dragon Age engines (`ENABLE_XML`)
- GLEW: a bundled copy is used unless a system GLEW is found

Bundled (nothing to install): Lua, tolua++, Dear ImGui, libmspack, GLEW,
GLM and utf8-cpp in `external/`, and Google Test in `tests/googletest/`.
