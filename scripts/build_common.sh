# Shared helpers for scripts/build_ci_linux.sh and scripts/bootstrap_linux.sh.
# This file is sourced, not run. It expects REPO_ROOT to be set.
# shellcheck shell=bash disable=SC2034 # the package lists are used by the scripts that source this

# --- package names ----------------------------------------------------------
# Keep these in sync with BUILDDEP.md.

# Tools needed by every build.
TOOLS_APT=(build-essential cmake ninja-build git pkg-config)
TOOLS_DNF=(gcc-c++ cmake ninja-build git pkgconf-pkg-config)
TOOLS_PACMAN=(base-devel cmake ninja git pkgconf)
TOOLS_ZYPPER=(gcc-c++ cmake ninja git pkg-config)
TOOLS_BREW=(cmake ninja git pkg-config)

# Libraries for the system-packages build (scripts/build_ci_linux.sh).
LIBS_APT=(libboost-all-dev zlib1g-dev liblzma-dev libxml2-dev libsdl2-dev
	libopenal-dev libfreetype-dev libgl-dev libmad0-dev libogg-dev libvorbis-dev
	libfaad-dev libxvidcore-dev libvpx-dev)
# xvidcore-devel is optional and lives in RPM Fusion, so it is not listed.
LIBS_DNF=(boost-devel zlib-devel xz-devel libxml2-devel SDL2-devel
	openal-soft-devel freetype-devel mesa-libGL-devel libmad-devel libogg-devel
	libvorbis-devel faad2-devel libvpx-devel)
LIBS_PACMAN=(boost zlib xz libxml2 sdl2 openal freetype2 mesa libmad libogg
	libvorbis faad2 xvidcore libvpx)

# Extra tools and system libraries the vcpkg build needs (scripts/bootstrap_linux.sh):
# archive tools for vcpkg itself, nasm for libvpx, OpenGL (never from vcpkg),
# X11 headers for SDL2 and sound-server headers for OpenAL Soft.
VCPKG_APT=(curl zip unzip tar nasm libgl-dev libx11-dev libxext-dev libxft-dev
	libasound2-dev libpulse-dev)
VCPKG_DNF=(curl zip unzip tar nasm mesa-libGL-devel libX11-devel libXext-devel
	libXft-devel alsa-lib-devel pulseaudio-libs-devel)
VCPKG_PACMAN=(curl zip unzip tar nasm mesa libx11 libxext libxft alsa-lib libpulse)

# --- helpers ----------------------------------------------------------------

# Print the name of the system package manager we know package names for.
detect_package_manager() {
	local pm
	for pm in apt-get dnf pacman zypper brew; do
		if command -v "${pm}" >/dev/null 2>&1; then
			echo "${pm}"
			return
		fi
	done
	echo ""
}

# install_command <pm> <package>... : print the command that installs packages.
install_command() {
	local pm="$1"
	shift
	case "${pm}" in
		apt-get) echo "sudo apt-get install $*" ;;
		dnf) echo "sudo dnf install $*" ;;
		pacman) echo "sudo pacman -S --needed $*" ;;
		zypper) echo "sudo zypper install $*" ;;
		brew) echo "brew install $*" ;;
		*) echo "<your package manager> install $*" ;;
	esac
}

# Print the C++ compiler that CMake would most likely pick, or nothing.
find_cxx_compiler() {
	local cxx
	if [[ -n "${CXX:-}" ]]; then
		# CXX may carry flags or a launcher; check the first word only.
		cxx="${CXX%% *}"
		if command -v "${cxx}" >/dev/null 2>&1; then
			echo "${cxx}"
		fi
		return
	fi
	for cxx in c++ g++ clang++; do
		if command -v "${cxx}" >/dev/null 2>&1; then
			echo "${cxx}"
			return
		fi
	done
}

# Turn a --build-dir value into an absolute path. Relative paths are taken
# relative to the repository root, so the scripts work from any directory.
resolve_build_path() {
	local dir="$1"
	if [[ "${dir}" == /* ]]; then
		echo "${dir%/}"
	else
		echo "${REPO_ROOT}/${dir%/}"
	fi
}

# Exit with a clear message when an option that needs a value has none.
require_option_value() {
	local option="$1"
	local remaining="$2"
	local value="${3:-}"
	if ((remaining < 2)) || [[ -z "${value}" || "${value}" == -* && "${value}" != -[0-9]* ]]; then
		echo "error: ${option} needs a value (see --help)" >&2
		exit 2
	fi
}

validate_build_type() {
	case "$1" in
		Debug | Release | RelWithDebInfo | MinSizeRel) ;;
		*)
			echo "error: unknown build type '$1' (use Debug, Release, RelWithDebInfo or MinSizeRel)" >&2
			exit 2
			;;
	esac
}

default_jobs() {
	if command -v nproc >/dev/null 2>&1; then
		nproc
	elif command -v sysctl >/dev/null 2>&1 && sysctl -n hw.ncpu >/dev/null 2>&1; then
		sysctl -n hw.ncpu
	else
		echo 4
	fi
}

# print_next_steps <build path> <tests already run: 0|1>
print_next_steps() {
	local build_path="$1"
	local tests_ran="$2"
	local launch_args=""

	if [[ "${build_path}" == "${REPO_ROOT}/"* ]]; then
		local rel="${build_path#"${REPO_ROOT}/"}"
		case "${rel}" in
			build-linux-ci | build-vcpkg-linux) ;; # found by the launcher automatically
			*) launch_args=" --build-dir ${rel}" ;;
		esac
	fi

	echo
	echo "Build finished."
	echo "  Build directory : ${build_path}"
	echo "  Binary          : ${build_path}/bin/xoreos"
	if [[ "${tests_ran}" != "1" ]]; then
		echo "  Run unit tests  : cmake --build '${build_path}' --target check"
	fi
	echo
	echo "Launch KotOR (needs your own game files):"
	if [[ "${build_path}" == "${REPO_ROOT}/"* ]]; then
		local runner="scripts/run_xoreos_linux.sh"
		if [[ ! -x "${REPO_ROOT}/${runner}" ]]; then
			runner="bash ${runner}"
		fi
		echo "  ${runner}${launch_args} --game-path /path/to/kotor"
		echo "  python3 scripts/xoreos_launcher_gui.py      (graphical launcher)"
	else
		echo "  '${build_path}/bin/xoreos' -p/path/to/kotor"
	fi
}
