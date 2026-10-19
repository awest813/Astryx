#!/usr/bin/env bash
# Self-contained Linux/macOS build: fetch vcpkg into .deps/vcpkg, let it build
# every library listed in vcpkg.json, then configure and build xoreos.
# The first run takes a long time (all libraries are compiled from source).
# For a much faster build using your distribution's packages, use
# scripts/build_ci_linux.sh instead.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
# shellcheck source-path=SCRIPTDIR source=build_common.sh
source "${SCRIPT_DIR}/build_common.sh"

BUILD_DIR="${BUILD_DIR:-build-vcpkg-linux}"
CONFIGURATION="${CONFIGURATION:-Debug}"
VCPKG_ROOT_INPUT="${VCPKG_ROOT:-}"
VCPKG_REF="${VCPKG_REF:-}"
SKIP_BUILD=0
RUN_TESTS=0
JOBS=""

usage() {
	cat <<'EOF'
Usage: scripts/bootstrap_linux.sh [options]

Self-contained build with vcpkg: clones vcpkg into .deps/vcpkg (once), builds
the libraries from vcpkg.json, then configures and builds xoreos with Ninja.
The first run compiles every dependency and can take a long time.
Faster alternative using system packages: scripts/build_ci_linux.sh

Options:
  --build-dir DIR    Build directory, absolute or relative to the repository
                     root (default: build-vcpkg-linux)
  --config TYPE      Debug, Release, RelWithDebInfo or MinSizeRel (default: Debug)
  --vcpkg-root DIR   Use this vcpkg checkout (default: $VCPKG_ROOT, else
                     .deps/vcpkg in the repository)
  --vcpkg-ref REF    Check out this vcpkg commit or tag when cloning, to pin
                     library versions (default: latest vcpkg)
  --test             Also build and run the unit tests (target "check")
  --skip-build       Configure only (vcpkg still builds the libraries)
  -j N               Parallel build jobs (default: number of CPUs)
  -h, --help         Show this help

Environment:
  BUILD_DIR, CONFIGURATION, VCPKG_ROOT, VCPKG_REF, CC, CXX
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
		--vcpkg-root)
			require_option_value "$1" "$#" "${2:-}"
			VCPKG_ROOT_INPUT="$2"
			shift 2
			;;
		--vcpkg-ref)
			require_option_value "$1" "$#" "${2:-}"
			VCPKG_REF="$2"
			shift 2
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
			echo "Run 'scripts/bootstrap_linux.sh --help' for usage." >&2
			exit 2
			;;
	esac
done

validate_build_type "${CONFIGURATION}"
if [[ -n "${JOBS}" && ! "${JOBS}" =~ ^[1-9][0-9]*$ ]]; then
	echo "error: -j needs a positive number, got '${JOBS}'" >&2
	exit 2
fi

if [[ -z "${VCPKG_ROOT_INPUT}" ]]; then
	VCPKG_ROOT_INPUT="${REPO_ROOT}/.deps/vcpkg"
fi
# Make the vcpkg path absolute without needing realpath or python.
if [[ "${VCPKG_ROOT_INPUT}" != /* ]]; then
	VCPKG_ROOT_INPUT="${PWD}/${VCPKG_ROOT_INPUT}"
fi
VCPKG_ROOT_REAL="${VCPKG_ROOT_INPUT%/}"
TOOLCHAIN="${VCPKG_ROOT_REAL}/scripts/buildsystems/vcpkg.cmake"
BUILD_PATH="$(resolve_build_path "${BUILD_DIR}")"
PKG_MANAGER="$(detect_package_manager)"

suggested_packages() {
	case "${PKG_MANAGER}" in
		apt-get) echo "${TOOLS_APT[*]} ${VCPKG_APT[*]}" ;;
		dnf) echo "${TOOLS_DNF[*]} ${VCPKG_DNF[*]}" ;;
		pacman) echo "${TOOLS_PACMAN[*]} ${VCPKG_PACMAN[*]}" ;;
		zypper) echo "${TOOLS_ZYPPER[*]} curl zip unzip tar nasm" ;;
		brew) echo "${TOOLS_BREW[*]} nasm" ;;
		*) echo "" ;;
	esac
}

print_install_hint() {
	local packages
	packages="$(suggested_packages)"
	if [[ -n "${packages}" ]]; then
		echo "Install the prerequisites with:" >&2
		# shellcheck disable=SC2086 # word splitting of the package list is intended
		echo "  $(install_command "${PKG_MANAGER}" ${packages})" >&2
	fi
	echo "See BUILDDEP.md for details." >&2
}

check_prerequisites() {
	local missing=()
	local cmd
	# vcpkg itself needs curl, zip, unzip and tar; pkg-config is used by ports.
	for cmd in git cmake ninja pkg-config curl zip unzip tar; do
		command -v "${cmd}" >/dev/null 2>&1 || missing+=("${cmd}")
	done
	if [[ -z "$(find_cxx_compiler)" ]]; then
		missing+=("C++ compiler (${CXX:-c++, g++ or clang++})")
	fi

	if ((${#missing[@]} > 0)); then
		echo "error: missing required tools:" >&2
		printf '  - %s\n' "${missing[@]}" >&2
		print_install_hint
		exit 1
	fi

	# Not fatal, but these make vcpkg fail late, after a long build.
	local warnings=()
	command -v nasm >/dev/null 2>&1 || warnings+=("nasm (needed by vcpkg's libvpx port)")
	if [[ "$(uname -s)" == "Linux" && ! -e /usr/include/GL/gl.h ]]; then
		warnings+=("OpenGL headers, GL/gl.h (OpenGL always comes from the system)")
	fi
	if ((${#warnings[@]} > 0)); then
		echo "warning: these are probably needed but were not found:" >&2
		printf '  - %s\n' "${warnings[@]}" >&2
		print_install_hint
		echo "Continuing anyway." >&2
		echo >&2
	fi
}

check_prerequisites

if [[ ! -f "${TOOLCHAIN}" ]]; then
	if [[ -e "${VCPKG_ROOT_REAL}" && -n "$(ls -A "${VCPKG_ROOT_REAL}" 2>/dev/null)" ]]; then
		echo "error: ${VCPKG_ROOT_REAL} exists but is not a vcpkg checkout" >&2
		echo "(no scripts/buildsystems/vcpkg.cmake). Remove it or pass --vcpkg-root." >&2
		exit 1
	fi
	mkdir -p "$(dirname "${VCPKG_ROOT_REAL}")"
	echo "Cloning vcpkg into ${VCPKG_ROOT_REAL}"
	if ! git clone https://github.com/microsoft/vcpkg "${VCPKG_ROOT_REAL}"; then
		echo "error: could not clone vcpkg. Check your network connection, or clone" >&2
		echo "https://github.com/microsoft/vcpkg yourself and pass --vcpkg-root." >&2
		exit 1
	fi
	if [[ -n "${VCPKG_REF}" ]]; then
		echo "Checking out vcpkg ${VCPKG_REF}"
		git -C "${VCPKG_ROOT_REAL}" checkout --quiet "${VCPKG_REF}"
	fi
elif [[ -n "${VCPKG_REF}" ]]; then
	echo "note: --vcpkg-ref is only applied when cloning; ${VCPKG_ROOT_REAL} already exists." >&2
fi

if [[ ! -x "${VCPKG_ROOT_REAL}/vcpkg" ]]; then
	if [[ ! -f "${VCPKG_ROOT_REAL}/bootstrap-vcpkg.sh" ]]; then
		echo "error: vcpkg bootstrap script not found in ${VCPKG_ROOT_REAL}" >&2
		exit 1
	fi

	echo "Bootstrapping vcpkg"
	if ! "${VCPKG_ROOT_REAL}/bootstrap-vcpkg.sh" -disableMetrics; then
		echo "error: bootstrapping vcpkg failed (see above). It downloads a prebuilt" >&2
		echo "vcpkg binary, so check your network connection and that curl works." >&2
		exit 1
	fi
fi

export VCPKG_ROOT="${VCPKG_ROOT_REAL}"
VCPKG_COMMIT="$(git -C "${VCPKG_ROOT_REAL}" rev-parse --short HEAD 2>/dev/null || echo unknown)"

if [[ -z "${JOBS}" ]]; then
	JOBS="$(default_jobs)"
fi

echo "Configuring xoreos (vcpkg ${VCPKG_COMMIT}, ${CONFIGURATION})"
echo "  Build directory: ${BUILD_PATH}"
echo "  The first configure builds all libraries from vcpkg.json; this can take a while."
if ! cmake -S "${REPO_ROOT}" -B "${BUILD_PATH}" -G Ninja \
	"-DCMAKE_BUILD_TYPE=${CONFIGURATION}" \
	"-DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5" \
	"-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${TOOLCHAIN}"; then
	echo >&2
	echo "error: CMake configure failed." >&2
	echo "  - If a vcpkg port failed to build, its log is named in the output above;" >&2
	echo "    see also ${BUILD_PATH}/vcpkg-manifest-install.log" >&2
	echo "  - Missing system tools/headers (nasm, OpenGL, X11) are a common cause:" >&2
	print_install_hint
	echo "  - If the build directory was made with another generator or toolchain," >&2
	echo "    delete '${BUILD_PATH}' (or pick another --build-dir) and retry." >&2
	exit 1
fi

if [[ "${SKIP_BUILD}" == "1" ]]; then
	echo
	echo "Configured (build skipped). Build with: cmake --build '${BUILD_PATH}' --target xoreos"
	exit 0
fi

echo "Building xoreos (${CONFIGURATION}, ${JOBS} jobs)"
cmake --build "${BUILD_PATH}" --target xoreos -j "${JOBS}"

if [[ "${RUN_TESTS}" == "1" ]]; then
	echo "Building and running unit tests (target check)"
	cmake --build "${BUILD_PATH}" --target check -j "${JOBS}"
fi

echo
echo "vcpkg root      : ${VCPKG_ROOT_REAL} (commit ${VCPKG_COMMIT})"
print_next_steps "${BUILD_PATH}" "${RUN_TESTS}"
