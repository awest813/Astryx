Build-time library dependencies
===============================

Recommended easy-start paths
============================

For current development on 64-bit Windows 10/11 and Linux x86_64, the easiest
path is now the repo-local vcpkg bootstrap flow:

- Windows 10/11 x64:

      powershell -ExecutionPolicy Bypass -File scripts/bootstrap_windows.ps1

- Linux x86_64:

      ./scripts/bootstrap_linux.sh

These scripts:

- clone `vcpkg` into `.deps/vcpkg` if needed
- bootstrap it locally for the repo
- configure CMake with the correct toolchain file
- build `xoreos`

If you prefer manual setup, keep reading below.

Minimum host tools
==================

Windows 10/11 x64
-----------------

- Visual Studio 2022 Build Tools or Visual Studio Community with C++
- CMake
- Ninja
- Git

Linux x86_64
------------

- CMake
- Ninja
- Git
- a C++17 compiler (`g++` or `clang++`)
- `pkg-config`
- common archive tools used by vcpkg ports (`curl`, `zip`, `unzip`, `tar`)

Typical Debian/Ubuntu bootstrap set:

    sudo apt install build-essential cmake ninja-build git pkg-config curl zip unzip tar

Typical Fedora bootstrap set:

    sudo dnf install gcc-c++ cmake ninja-build git pkgconf-pkg-config curl zip unzip tar

Typical Arch bootstrap set:

    sudo pacman -S --needed base-devel cmake ninja git pkgconf curl zip unzip tar

- iconv
- zlib (>= 1.2.3)
- liblzma (>= 5.0.3)
- libxml2 (>= 2.8.0)

- Boost (>= 1.53.0)
  - Boost.Utility
  - Boost.StringAlgo
  - Boost.System
  - Boost.Filesystem
  - Boost.Date_Time
  - Boost.Uuid
  - Boost.Smart_Ptr
  - Boost.ScopeExit
  - Boost.Locale
  - Boost.TypeTraits

- OpenGL (>= 2.1)
- SDL2 (>= 2.0.4)
- FreeType 2 (>= 2.4.0 (libtool number >= 11.0.5))

- OpenAL Soft (>= 1.12)
- MAD (>= 0.15.1b)
- libogg (>= 1.2.0)
- libvorbis (>= 1.3.1)
- libvorbisfile (>= 1.3.1)
- libfaad (>= 2.7)

- libxvidcore (>= 1.2.2)
- libvpx (>= 1.6.0)
