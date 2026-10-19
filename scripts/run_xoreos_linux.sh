#!/usr/bin/env bash
# Start xoreos from the terminal: pick a saved game or a detected install, then launch.
# For a window instead, use scripts/xoreos_launcher_gui.py.
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: scripts/run_xoreos_linux.sh [options]

Without --game-path or --target, lists your saved games and the installs found
in the usual Steam and GOG folders, and asks which one to start.

Options:
  --game-path DIR   Start the game installed in DIR (the folder with swkotor.exe)
  --target NAME     Start a game saved in xoreos.conf under NAME
  --build-dir DIR   Use the xoreos binary from this build folder
  --width N         Window width
  --height N        Window height
  --fullscreen      Start in full screen
  --windowed        Start in a window
  --skip-videos     Skip the intro movies
  --list-sources    Print the engine, config file and games found, then exit
  -h, --help        Show this help

Environment:
  XOREOS_EXE        Path to the xoreos binary to use
EOF
}

GAME_PATH=""
TARGET=""
BUILD_DIR=""
WIDTH=""
HEIGHT=""
FULLSCREEN=""
SKIP_VIDEOS=0
LIST_SOURCES=0

need_value() {
	if [[ $# -lt 2 || -z "$2" || "$2" == --* ]]; then
		echo "Option $1 needs a value. See --help." >&2
		exit 2
	fi
}

need_number() {
	if [[ ! "$2" =~ ^[0-9]+$ ]] || (( $2 == 0 )); then
		echo "Option $1 needs a positive number, not '$2'." >&2
		exit 2
	fi
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--game-path)
			need_value "$@"
			GAME_PATH="$2"
			shift 2
			;;
		--target)
			need_value "$@"
			TARGET="$2"
			shift 2
			;;
		--build-dir)
			need_value "$@"
			BUILD_DIR="$2"
			shift 2
			;;
		--width)
			need_value "$@"
			need_number "$1" "$2"
			WIDTH="$2"
			shift 2
			;;
		--height)
			need_value "$@"
			need_number "$1" "$2"
			HEIGHT="$2"
			shift 2
			;;
		--fullscreen)
			FULLSCREEN="true"
			shift
			;;
		--windowed)
			FULLSCREEN="false"
			shift
			;;
		--skip-videos)
			SKIP_VIDEOS=1
			shift
			;;
		--list-sources)
			LIST_SOURCES=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1. See --help." >&2
			exit 2
			;;
	esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Program files xoreos uses to recognise each game (src/engines/*/probes.cpp)
GAME_PROGRAMS=(
	"KotOR I|swkotor.exe"
	"KotOR II|swkotor2.exe"
	"Neverwinter Nights|nwmain.exe"
	"Neverwinter Nights|nwmain"
	"Jade Empire|JadeEmpire.exe"
)

# The newest xoreos binary: XOREOS_EXE, then --build-dir, then any build folder.
get_xoreos_executable() {
	if [[ -n "${XOREOS_EXE:-}" ]]; then
		if [[ -x "${XOREOS_EXE}" ]]; then
			printf '%s\n' "${XOREOS_EXE}"
			return 0
		fi
		echo "XOREOS_EXE is set, but '${XOREOS_EXE}' isn't an executable file." >&2
		return 1
	fi

	local bases=()
	if [[ -n "${BUILD_DIR}" ]]; then
		if [[ "${BUILD_DIR}" = /* ]]; then
			bases+=("${BUILD_DIR}")
		else
			bases+=("${REPO_ROOT}/${BUILD_DIR}")
		fi
	else
		bases+=("${REPO_ROOT}")
		local dir
		for dir in "${REPO_ROOT}"/build* "${REPO_ROOT}"/builds/*; do
			[[ -d "${dir}" ]] && bases+=("${dir}")
		done
	fi

	local newest="" base config candidate
	for base in "${bases[@]}"; do
		for config in "" Release RelWithDebInfo MinSizeRel Debug; do
			candidate="${base}/bin/${config:+${config}/}xoreos"
			if [[ -f "${candidate}" && -x "${candidate}" ]]; then
				if [[ -z "${newest}" || "${candidate}" -nt "${newest}" ]]; then
					newest="${candidate}"
				fi
			fi
		done
	done

	[[ -n "${newest}" ]] || return 1
	printf '%s\n' "${newest}"
}

# Where xoreos reads its config and writes its log (src/common/platform.cpp)
get_config_path() {
	printf '%s\n' "${XDG_CONFIG_HOME:-${HOME}/.config}/xoreos/xoreos.conf"
}

get_log_path() {
	printf '%s\n' "${XDG_DATA_HOME:-${HOME}/.local/share}/xoreos/xoreos.log"
}

# Prints Target|name|path|description for each game saved in xoreos.conf
read_config_targets() {
	local config_path="$1"
	[[ -f "${config_path}" ]] || return 0

	awk '
		function emit() {
			if (name != "" && tolower(name) != "xoreos" && path != "") {
				printf "Target|%s|%s|%s\n", name, path, desc
			}
		}
		BEGIN { name=""; path=""; desc="" }
		{
			line=$0
			gsub(/\r$/, "", line)
			if (line ~ /^[[:space:]]*;/) next
			sub(/#.*/, "", line)
			gsub(/^[[:space:]]+|[[:space:]]+$/, "", line)
			if (line == "") next
			if (line ~ /^\[.+\]$/) {
				emit()
				name=substr(line, 2, length(line) - 2)
				path=""
				desc=""
				next
			}
			if (name == "") next
			if (tolower(line) ~ /^path[[:space:]]*=/) {
				path=line
				sub(/^[^=]*=[[:space:]]*/, "", path)
				next
			}
			if (tolower(line) ~ /^description[[:space:]]*=/) {
				desc=line
				sub(/^[^=]*=[[:space:]]*/, "", desc)
			}
		}
		END { emit() }
	' "${config_path}"
}

# Prints the label of the game installed in a folder, if xoreos would recognise it
game_in_folder() {
	local folder="$1" pair label program
	[[ -d "${folder}" ]] || return 1
	for pair in "${GAME_PROGRAMS[@]}"; do
		label="${pair%%|*}"
		program="${pair#*|}"
		if [[ -n "$(find "${folder}" -maxdepth 1 -type f -iname "${program}" -print -quit 2>/dev/null)" ]]; then
			printf '%s\n' "${label}"
			return 0
		fi
	done
	return 1
}

# Folders whose subfolders may be game installs: Steam libraries, GOG, ~/Games
search_roots() {
	local steam vdf library
	for steam in \
		"${HOME}/.local/share/Steam" \
		"${HOME}/.steam/steam" \
		"${HOME}/.steam/root" \
		"${HOME}/.var/app/com.valvesoftware.Steam/.local/share/Steam" \
		"${HOME}/snap/steam/common/.local/share/Steam"; do
		[[ -d "${steam}" ]] || continue
		printf '%s\n' "${steam}/steamapps/common"
		for vdf in "${steam}/steamapps/libraryfolders.vdf" "${steam}/config/libraryfolders.vdf"; do
			[[ -f "${vdf}" ]] || continue
			sed -n 's/^[[:space:]]*"path"[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p' "${vdf}" | while IFS= read -r library; do
				printf '%s\n' "${library}/steamapps/common"
			done
		done
	done
	printf '%s\n' "${HOME}/GOG Games" "${HOME}/Games" "${HOME}/Games/Heroic"
}

# Prints Path|label|folder|where for each install found
detect_games() {
	local root folder label
	declare -A seen=()
	while IFS= read -r root; do
		[[ -d "${root}" ]] || continue
		root="$(cd "${root}" && pwd -P)"
		[[ -z "${seen[${root}]:-}" ]] || continue
		seen[${root}]=1
		for folder in "${root}"/*/; do
			folder="${folder%/}"
			if label="$(game_in_folder "${folder}")"; then
				printf 'Path|%s|%s|%s\n' "${label}" "${folder}" "${root}"
			fi
		done
	done < <(search_roots)
}

# Drops detected installs that are already saved
dedupe_entries() {
	awk -F'|' '!seen[$3]++'
}

# Asks which entry to start. The menu goes to stderr; only the choice goes to stdout.
choose_entry() {
	local -n entries_ref=$1
	local manual_path choice

	if [[ ${#entries_ref[@]} -eq 0 ]]; then
		{
			echo
			echo "No saved games, and no installs found in the usual Steam and GOG folders."
		} >&2
		read -r -p "Enter the folder you installed the game in: " manual_path
		if [[ -z "${manual_path}" ]]; then
			echo "No game folder given." >&2
			exit 1
		fi
		printf 'Path|Manual|%s|\n' "${manual_path}"
		return 0
	fi

	{
		echo
		echo "Games:"
		local i=1 entry kind name path
		for entry in "${entries_ref[@]}"; do
			IFS='|' read -r kind name path _ <<<"${entry}"
			if [[ "${kind}" == "Target" ]]; then
				printf '  [%d] %s (saved)\n' "${i}" "${name}"
			else
				printf '  [%d] %s\n' "${i}" "${name}"
			fi
			printf '      %s\n' "${path}"
			i=$((i + 1))
		done
		echo "  [M] Another folder"
	} >&2

	while true; do
		read -r -p "Choose a number or M: " choice
		if [[ "${choice}" =~ ^[Mm]$ ]]; then
			read -r -p "Enter the folder you installed the game in: " manual_path
			if [[ -n "${manual_path}" ]]; then
				printf 'Path|Manual|%s|\n' "${manual_path}"
				return 0
			fi
		elif [[ "${choice}" =~ ^[0-9]+$ ]] && (( choice >= 1 && choice <= ${#entries_ref[@]} )); then
			printf '%s\n' "${entries_ref[$((choice - 1))]}"
			return 0
		fi
		echo "Please type one of the numbers shown, or M." >&2
	done
}

EXE_PATH=""
if EXE_CANDIDATE="$(get_xoreos_executable)"; then
	EXE_PATH="${EXE_CANDIDATE}"
fi
CONFIG_PATH="$(get_config_path)"

mapfile -t ENTRIES < <(
	{
		read_config_targets "${CONFIG_PATH}"
		detect_games
	} | dedupe_entries
)

if [[ "${LIST_SOURCES}" == "1" ]]; then
	echo "Executable : ${EXE_PATH:-<not built yet>}"
	echo "Config file: ${CONFIG_PATH}"
	echo "Engine log : $(get_log_path)"
	for entry in "${ENTRIES[@]}"; do
		IFS='|' read -r kind name path _desc <<<"${entry}"
		echo "${kind} | ${name} | ${path}"
	done
	exit 0
fi

if [[ -z "${EXE_PATH}" ]]; then
	{
		echo "Can't find the xoreos program. Build it first, from ${REPO_ROOT}:"
		echo "  ./scripts/build_ci_linux.sh     (uses your system's libraries)"
		echo "  ./scripts/bootstrap_linux.sh    (downloads its own libraries)"
		echo "If you built it elsewhere, pass --build-dir DIR or set XOREOS_EXE."
	} >&2
	exit 1
fi

launch_kind=""
launch_value=""

if [[ -n "${TARGET}" ]]; then
	launch_kind="Target"
	launch_value="${TARGET}"
elif [[ -n "${GAME_PATH}" ]]; then
	launch_kind="Path"
	launch_value="${GAME_PATH}"
else
	echo "xoreos: ${EXE_PATH}"
	selected="$(choose_entry ENTRIES)"
	IFS='|' read -r launch_kind _ launch_value _ <<<"${selected}"
fi

args=()
if [[ "${launch_kind}" == "Target" ]]; then
	if ! read_config_targets "${CONFIG_PATH}" | awk -F'|' -v t="${launch_value}" 'tolower($2) == tolower(t) { found=1 } END { exit !found }'; then
		echo "There's no game saved as '${launch_value}' in ${CONFIG_PATH}." >&2
		echo "Run without --target to see your games, or use --game-path." >&2
		exit 1
	fi
	args+=("${launch_value}")
else
	launch_value="${launch_value/#\~/${HOME}}"
	if [[ ! -d "${launch_value}" ]]; then
		echo "The game folder doesn't exist: ${launch_value}" >&2
		exit 1
	fi
	launch_value="$(cd "${launch_value}" && pwd -P)"
	if ! game_in_folder "${launch_value}" >/dev/null; then
		echo "Warning: no game program (such as swkotor.exe) in ${launch_value}." >&2
		echo "xoreos will probably not recognise it. Use the folder that contains swkotor.exe." >&2
	fi
	args+=("--path=${launch_value}")
fi

if [[ -n "${WIDTH}" ]]; then
	args+=("--width=${WIDTH}")
fi
if [[ -n "${HEIGHT}" ]]; then
	args+=("--height=${HEIGHT}")
fi
if [[ -n "${FULLSCREEN}" ]]; then
	args+=("--fullscreen=${FULLSCREEN}")
fi
if [[ "${SKIP_VIDEOS}" == "1" ]]; then
	args+=("--skipvideos=true")
fi

echo
printf 'Launching: %q' "${EXE_PATH}"
printf ' %q' "${args[@]}"
echo

status=0
"${EXE_PATH}" "${args[@]}" || status=$?
if (( status != 0 )); then
	echo >&2
	echo "xoreos stopped with an error (exit code ${status}). The messages above say why;" >&2
	echo "the full log is in $(get_log_path)" >&2
fi
exit "${status}"
