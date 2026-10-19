#!/usr/bin/env bash
# Configure and build xoreos against system packages, the same way the Linux
# GitHub Actions jobs do. This is the fastest way to build on Linux.
# For a self-contained build that fetches its own libraries, use
# scripts/bootstrap_linux.sh (vcpkg) instead.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
# shellcheck source-path=SCRIPTDIR source=build_common.sh
source "${SCRIPT_DIR}/build_common.sh"

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

Build xoreos with libraries from your system package manager (the same
setup as .github/workflows/linux_cmake_{gcc,clang}.yml):
  cmake -G Ninja -B <build-dir> && cmake --build <build-dir> [--target check]

Options:
  --build-dir DIR   Build directory, absolute or relative to the repository
                    root (default: build-linux-ci)
  --config TYPE     Debug, Release, RelWithDebInfo or MinSizeRel (default: Debug)
  --compiler NAME   gcc or clang (sets CC/CXX for this run)
  --install-deps    Install build tools and libraries with apt-get, dnf or
                    pacman first (uses sudo)
  --test            Also build and run the unit tests (target "check")
  --skip-build      Configure only
  -j N              Parallel build jobs (default: number of CPUs)
  -h, --help        Show this help

Environment:
  BUILD_DIR, CONFIGURATION, COMPILER, CC, CXX

Examples:
  scripts/build_ci_linux.sh --install-deps --test
  scripts/build_ci_linux.sh --compiler clang --config Release
EOF
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-dir)
			require_option_value "$1" "$#" "${2:-}"
			BUILD_DIR="$2"
			shift 2
			;;
		--config)
			require_option_value "$1" "$#" "${2:-}"
			CONFIGURATION="$2"
			shift 2
			;;
		--compiler)
			require_option_value "$1" "$#" "${2:-}"
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
			require_option_value "$1" "$#" "${2:-}"
			JOBS="$2"
			shift 2
			;;
		-j[0-9]*)
			JOBS="${1#-j}"
			shift
			;;
		-h | --help)
			usage
			exit 0
			;;
		*)
			echo "error: unknown argument: $1" >&2
			echo "Run 'scripts/build_ci_linux.sh --help' for usage." >&2
			exit 2
			;;
	esac
done

validate_build_type "${CONFIGURATION}"
if [[ -n "${JOBS}" && ! "${JOBS}" =~ ^[1-9][0-9]*$ ]]; then
	echo "error: -j needs a positive number, got '${JOBS}'" >&2
	exit 2
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
		echo "error: unsupported --compiler '${COMPILER}' (use gcc or clang)" >&2
		exit 2
		;;
esac

BUILD_PATH="$(resolve_build_path "${BUILD_DIR}")"
PKG_MANAGER="$(detect_package_manager)"

all_packages() {
	case "${PKG_MANAGER}" in
		apt-get) echo "${TOOLS_APT[*]} ${LIBS_APT[*]}" ;;
		dnf) echo "${TOOLS_DNF[*]} ${LIBS_DNF[*]}" ;;
		pacman) echo "${TOOLS_PACMAN[*]} ${LIBS_PACMAN[*]}" ;;
		*) echo "" ;;
	esac
}

install_deps() {
	local packages
	packages="$(all_packages)"
	if [[ -z "${packages}" ]]; then
		echo "error: --install-deps knows apt-get, dnf and pacman; none was found." >&2
		echo "Install the libraries listed in BUILDDEP.md, or use scripts/bootstrap_linux.sh (vcpkg)." >&2
		exit 1
	fi

	local sudo=()
	if [[ "${EUID}" -ne 0 ]]; then
		if ! command -v sudo >/dev/null 2>&1; then
			echo "error: --install-deps needs root or sudo. Run as root:" >&2
			# shellcheck disable=SC2086 # word splitting of the package list is intended
			echo "  $(install_command "${PKG_MANAGER}" ${packages} | sed 's/^sudo //')" >&2
			exit 1
		fi
		sudo=(sudo)
	fi

	echo "Installing build dependencies with ${PKG_MANAGER}"
	# shellcheck disable=SC2086 # word splitting of the package list is intended
	case "${PKG_MANAGER}" in
		apt-get)
			"${sudo[@]}" apt-get update -qq
			"${sudo[@]}" apt-get install -y ${packages}
			;;
		dnf) "${sudo[@]}" dnf install -y ${packages} ;;
		pacman) "${sudo[@]}" pacman -S --needed --noconfirm ${packages} ;;
	esac
}

check_tools() {
	local missing=()
	local cmd
	for cmd in cmake ninja pkg-config; do
		command -v "${cmd}" >/dev/null 2>&1 || missing+=("${cmd}")
	done
	if [[ -z "$(find_cxx_compiler)" ]]; then
		missing+=("C++ compiler (${CXX:-c++, g++ or clang++})")
	fi
	if ((${#missing[@]} == 0)); then
		return
	fi

	echo "error: missing build tools:" >&2
	printf '  - %s\n' "${missing[@]}" >&2
	local packages
	packages="$(all_packages)"
	if [[ -n "${packages}" ]]; then
		echo "Install everything with:" >&2
		echo "  scripts/build_ci_linux.sh --install-deps" >&2
		echo "or:" >&2
		# shellcheck disable=SC2086 # word splitting of the package list is intended
		echo "  $(install_command "${PKG_MANAGER}" ${packages})" >&2
	else
		echo "See BUILDDEP.md for the packages to install." >&2
	fi
	exit 1
}

if [[ "${INSTALL_DEPS}" == "1" ]]; then
	install_deps
fi
check_tools

if [[ -z "${JOBS}" ]]; then
	JOBS="$(default_jobs)"
fi

echo "Configuring xoreos (system packages, ${CONFIGURATION})"
echo "  Build directory: ${BUILD_PATH}"
if [[ -n "${CC:-}" || -n "${CXX:-}" ]]; then
	echo "  CC=${CC:-} CXX=${CXX:-}"
fi
if ! cmake -S "${REPO_ROOT}" -B "${BUILD_PATH}" -G Ninja "-DCMAKE_BUILD_TYPE=${CONFIGURATION}"; then
	echo >&2
	echo "error: CMake configure failed. The message above names any missing library." >&2
	echo "  Install everything at once: scripts/build_ci_linux.sh --install-deps" >&2
	echo "  If the build directory was made by another generator or toolchain," >&2
	echo "  delete '${BUILD_PATH}' (or pick another --build-dir) and retry." >&2
	exit 1
fi

if [[ "${SKIP_BUILD}" == "1" ]]; then
	echo
	echo "Configured (build skipped). Build with: cmake --build '${BUILD_PATH}'"
	exit 0
fi

echo "Building xoreos (${CONFIGURATION}, ${JOBS} jobs)"
cmake --build "${BUILD_PATH}" -j "${JOBS}"

if [[ "${RUN_TESTS}" == "1" ]]; then
	echo "Building and running unit tests (target check)"
	cmake --build "${BUILD_PATH}" --target check -j "${JOBS}"
fi

print_next_steps "${BUILD_PATH}" "${RUN_TESTS}"
