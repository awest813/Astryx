#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build-vcpkg-linux}"
CONFIGURATION="${CONFIGURATION:-Debug}"
VCPKG_ROOT_INPUT="${VCPKG_ROOT:-}"
SKIP_BUILD=0

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
		--vcpkg-root)
			VCPKG_ROOT_INPUT="$2"
			shift 2
			;;
		--skip-build)
			SKIP_BUILD=1
			shift
			;;
		*)
			echo "Unknown argument: $1" >&2
			exit 2
			;;
	esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

if [[ -z "${VCPKG_ROOT_INPUT}" ]]; then
	VCPKG_ROOT_INPUT="${REPO_ROOT}/.deps/vcpkg"
fi

VCPKG_ROOT_REAL="$(python3 -c 'import os,sys; print(os.path.realpath(sys.argv[1]))' "${VCPKG_ROOT_INPUT}")"
TOOLCHAIN="${VCPKG_ROOT_REAL}/scripts/buildsystems/vcpkg.cmake"
BUILD_PATH="${REPO_ROOT}/${BUILD_DIR}"

require_command() {
	if ! command -v "$1" >/dev/null 2>&1; then
		echo "Missing required command: $1" >&2
		echo "Install at least: git cmake ninja-build g++ pkg-config unzip tar curl zip" >&2
		exit 1
	fi
}

require_command git
require_command cmake
require_command ninja
require_command python3

if [[ ! -f "${TOOLCHAIN}" ]]; then
	mkdir -p "$(dirname "${VCPKG_ROOT_REAL}")"
	echo "Cloning vcpkg into ${VCPKG_ROOT_REAL}"
	git clone https://github.com/microsoft/vcpkg "${VCPKG_ROOT_REAL}"
fi

if [[ ! -x "${VCPKG_ROOT_REAL}/vcpkg" ]]; then
	if [[ ! -f "${VCPKG_ROOT_REAL}/bootstrap-vcpkg.sh" ]]; then
		echo "vcpkg bootstrap script not found in ${VCPKG_ROOT_REAL}" >&2
		exit 1
	fi

	echo "Bootstrapping vcpkg"
	"${VCPKG_ROOT_REAL}/bootstrap-vcpkg.sh" -disableMetrics
fi

export VCPKG_ROOT="${VCPKG_ROOT_REAL}"

echo "Configuring xoreos"
cmake -S "${REPO_ROOT}" -B "${BUILD_PATH}" -G Ninja \
	"-DCMAKE_BUILD_TYPE=${CONFIGURATION}" \
	"-DCMAKE_POLICY_VERSION_MINIMUM:STRING=3.5" \
	"-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${TOOLCHAIN}"

if [[ "${SKIP_BUILD}" != "1" ]]; then
	echo "Building xoreos (${CONFIGURATION})"
	cmake --build "${BUILD_PATH}" --target xoreos -j
fi

echo
echo "Ready."
echo "Build directory : ${BUILD_PATH}"
echo "vcpkg root      : ${VCPKG_ROOT_REAL}"
echo "Binary          : ${BUILD_PATH}/bin/xoreos"
