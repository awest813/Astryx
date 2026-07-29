# AGENTS.md

Astryx is a C++17 fork of the [xoreos](https://github.com/xoreos/xoreos) engine
(a reimplementation of BioWare's Aurora engine) focused on KotOR I & II. It is a
single native desktop application (`xoreos`) plus a Google Test unit-test suite.
General build/run docs live in `README.md`, `BUILDDEP.md`, and `scripts/`.

## Cursor Cloud specific instructions

The VM startup update script installs all build dependencies (system apt
packages; there is no language package manager or lockfile). Below are the
non-obvious caveats discovered during setup.

### Build with clang, not gcc
- The project must be configured with clang: `CC=clang CXX=clang++`. The default
  `/usr/bin/c++` on this image is clang, but CMake's auto-detection can still be
  wrong, so pass the compiler explicitly.
- gcc (any version: 11/12/13) fails to compile
  `src/engines/kotorbase/gui/charactersheet.cpp` with
  `error: taking address of temporary array` — the code relies on a GNU
  compound-literal extension that only clang accepts. This matches the repo's
  `linux_cmake_clang.yml` CI workflow.
- clang selects the gcc-14 toolchain for its C++ runtime, so `libstdc++-14-dev`
  must be present or linking fails with `cannot find -lstdc++` (the update script
  installs it).

### Configure / build / run
- Configure: `CC=clang CXX=clang++ cmake -S . -B build-linux-ci -G Ninja -DCMAKE_BUILD_TYPE=Debug`
- Build the app: `cmake --build build-linux-ci --target xoreos -j` → binary at `build-linux-ci/bin/xoreos`.
- Run: `./build-linux-ci/bin/xoreos --version` / `--help`, or `scripts/run_xoreos_linux.sh`.
- Running an actual game requires proprietary KotOR game data, which is not in
  this repo. Without it the engine still boots its full subsystem stack (creates
  an OpenGL 2.1 context, loads shaders, inits graphics/sound/event subsystems)
  and then exits with `ERROR: Unable to detect the game`. This is the expected
  headless smoke result. Audio init fails gracefully (no ALSA device) — harmless.
- A virtual display is available at `DISPLAY=:1` with working (software) OpenGL.

### Tests
- Run tests: `cmake --build build-linux-ci --target check` then `ctest` from
  `build-linux-ci` (ctest runs the built test binaries).
- 96/102 tests pass. The following fork-specific `kotorbase` tests are
  pre-existing failures (do not treat as regressions you introduced):
  compile errors in `inventory`, `save_serialization`, `nwscript_functions`,
  `levelup`, `dantooine_cinematic`; runtime failure in `cinematic_scripting`.
- Build all buildable tests despite the broken ones with keep-going:
  `cmake --build build-linux-ci --target check -j 4 -- -k 0`.

### Lint / verification
- There is no configured linter (uncrustify/clang-format are not installed and
  there is no lint target); style is enforced via the extensive `-Wall` warning
  set at compile time.
- Project-specific check: `python3 scripts/verify_kotor_nwscript_stubs.py`
  (verifies KotOR NWScript function tables contain no null pointers).
