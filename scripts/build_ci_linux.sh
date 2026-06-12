#!/usr/bin/env bash
# Configure and build xoreos the same way GitHub Actions does (linux cmake gcc/clang).
# Uses system packages (apt) instead of vcpkg. See also scripts/bootstrap_linux.sh.
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build-linux-ci}"
CONFIGURATION="${CONFIGURATION:-Debug}"
COMPILER="${COMPILER:-}"
INSTALL_DEPS=0
RUN_TESTS=0
SKIP_BUILD=0
JOBS=""

usage() {
	cat <<'EOF'
Usage: scripts/build_ci_linux.sh [options]

Matches .github/workflows/linux_cmake_gcc.yml and linux_cmake_clang.yml:
  cmake -G Ninja -B <build-dir> && cmake --build <build-dir> [--target check]

Options:
  --build-dir DIR   CMake build directory (default: build-linux-ci)
  --config TYPE     CMAKE_BUILD_TYPE (default: Debug)
  --compiler NAME   gcc or clang (sets CC/CXX for this run)
  --install-deps    apt-get install CI packages (needs sudo)
  --test            Also run: cmake --build <dir> --target check
  --skip-build      Configure only
  -j N              Parallel build jobs (default: nproc)
  -h, --help        Show this help

Environment:
  BUILD_DIR, CONFIGURATION, CC, CXX — same as the option defaults above

Examples:
  scripts/build_ci_linux.sh
  scripts/build_ci_linux.sh --compiler clang --test
  scripts/build_ci_linux.sh --install-deps --build-dir build --config Release
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-dir)
			BUILD_DIR="$2"
			shift 2
			;;
		--config)
			CONFIGURATION="$2"
			shift 2
			;;
		--compiler)
			COMPILER="$2"
			shift 2
			;;
		--install-deps)
			INSTALL_DEPS=1
			shift
			;;
		--test)
			RUN_TESTS=1
			shift
			;;
		--skip-build)
			SKIP_BUILD=1
			shift
			;;
		-j)
			JOBS="$2"
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown argument: $1" >&2
			usage >&2
			exit 2
			;;
	esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_PATH="${REPO_ROOT}/${BUILD_DIR}"

# Same package set as .github/workflows/linux_cmake_{gcc,clang}.yml
APT_PACKAGES=(
	ninja-build
	libboost-all-dev
	libopenal-dev
	libmad0-dev
	libvorbis-dev
	libfaad-dev
	libsdl2-dev
	libgl-dev
	libglu-dev
	libxvidcore-dev
	libvpx-dev
)

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1" >&2
		return 1
	fi
}

check_build_deps() {
	local missing=()
	for cmd in cmake ninja g++ pkg-config; do
		require_command "${cmd}" || missing+=("${cmd}")
	done
	if ((${#missing[@]} > 0)); then
		echo "Missing build tools: ${missing[*]}" >&2
		echo "Install with: scripts/build_ci_linux.sh --install-deps" >&2
		echo "Or manually: sudo apt-get install cmake ninja-build g++ pkg-config ${APT_PACKAGES[*]}" >&2
		exit 1
	fi
}

install_apt_deps() {
	if ! command -v apt-get >/dev/null 2>&1; then
		echo "apt-get not found; install dependencies manually:" >&2
		printf '  %s\n' "${APT_PACKAGES[@]}" >&2
		exit 1
	fi
	if command -v sudo >/dev/null 2>&1; then
		sudo apt-get update -qq
		sudo apt-get install -y cmake g++ pkg-config "${APT_PACKAGES[@]}"
	else
		echo "sudo not available; cannot --install-deps in this environment." >&2
		exit 1
	fi
}

if [[ "${INSTALL_DEPS}" == "1" ]]; then
	install_apt_deps
fi

check_build_deps

if [[ -z "${JOBS}" ]]; then
	if command -v nproc >/dev/null 2>&1; then
		JOBS="$(nproc)"
	else
		JOBS=4
	fi
fi

case "${COMPILER}" in
	"") ;;
	gcc)
		export CC=gcc
		export CXX=g++
		;;
	clang)
		export CC=clang
		export CXX=clang++
		;;
	*)
		echo "Unsupported --compiler '${COMPILER}' (use gcc or clang)" >&2
		exit 2
		;;
esac

CMAKE_ARGS=(
	-S "${REPO_ROOT}"
	-B "${BUILD_PATH}"
	-G Ninja
	"-DCMAKE_BUILD_TYPE=${CONFIGURATION}"
)

echo "Configuring xoreos (CI-style, system packages)"
if [[ -n "${CC:-}" ]]; then
	echo "  CC=${CC} CXX=${CXX}"
fi
cmake "${CMAKE_ARGS[@]}"

if [[ "${SKIP_BUILD}" != "1" ]]; then
	echo "Building xoreos (${CONFIGURATION}, -j ${JOBS})"
	cmake --build "${BUILD_PATH}" -j "${JOBS}"

	if [[ "${RUN_TESTS}" == "1" ]]; then
		echo "Running unit tests (target check)"
		cmake --build "${BUILD_PATH}" --target check
	fi
fi

echo
echo "Ready."
echo "Build directory : ${BUILD_PATH}"
echo "Binary          : ${BUILD_PATH}/bin/xoreos"
if [[ "${RUN_TESTS}" != "1" ]]; then
	echo "Run tests       : cmake --build ${BUILD_PATH} --target check"
fi
