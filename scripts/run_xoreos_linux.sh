#!/usr/bin/env bash
set -euo pipefail

GAME_PATH=""
TARGET=""
BUILD_DIR="build-vcpkg-linux"
WIDTH=""
HEIGHT=""
FULLSCREEN=""
SKIP_VIDEOS=0
LIST_SOURCES=0

while [[ $# -gt 0 ]]; do
	case "$1" in
		--game-path)
			GAME_PATH="$2"
			shift 2
			;;
		--target)
			TARGET="$2"
			shift 2
			;;
		--build-dir)
			BUILD_DIR="$2"
			shift 2
			;;
		--width)
			WIDTH="$2"
			shift 2
			;;
		--height)
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
		*)
			echo "Unknown argument: $1" >&2
			exit 2
			;;
	esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

get_xoreos_executable() {
	local candidates=(
		"${REPO_ROOT}/${BUILD_DIR}/bin/xoreos"
		"${REPO_ROOT}/build-vcpkg-linux/bin/xoreos"
		"${REPO_ROOT}/build-vcpkg/bin/Debug/xoreos"
		"${REPO_ROOT}/build/bin/xoreos"
	)

	for candidate in "${candidates[@]}"; do
		if [[ -x "${candidate}" ]]; then
			printf '%s\n' "${candidate}"
			return 0
		fi
	done

	return 1
}

get_config_path() {
	local candidates=()
	if [[ -n "${XDG_CONFIG_HOME:-}" ]]; then
		candidates+=("${XDG_CONFIG_HOME}/xoreos/xoreos.conf")
	fi
	candidates+=("${HOME}/.config/xoreos/xoreos.conf")

	for candidate in "${candidates[@]}"; do
		if [[ -f "${candidate}" ]]; then
			printf '%s\n' "${candidate}"
			return 0
		fi
	done

	printf '%s\n' "${candidates[0]}"
}

read_config_targets() {
	local config_path="$1"
	[[ -f "${config_path}" ]] || return 0

	awk '
		function emit() {
			if (name != "" && name != "xoreos" && path != "") {
				printf "Target|%s|%s|%s\n", name, path, desc
			}
		}
		BEGIN { name=""; path=""; desc="" }
		/^[[:space:]]*[#;]/ { next }
		/^[[:space:]]*$/ { next }
		{
			line=$0
			gsub(/\r$/, "", line)
			if (match(line, /^\[(.+)\]$/, m)) {
				emit()
				name=m[1]
				path=""
				desc=""
				next
			}
			if (name == "") next
			if (match(line, /^path=(.+)$/, m)) {
				path=m[1]
				next
			}
			if (match(line, /^description=(.+)$/, m)) {
				desc=m[1]
			}
		}
		END { emit() }
	' "${config_path}"
}

detect_games() {
	local roots=(
		"${HOME}/.local/share/Steam/steamapps/common"
		"${HOME}/.steam/steam/steamapps/common"
		"${HOME}/Games"
	)

	local labels=(
		"KotOR I|swkotor"
		"KotOR I|Star Wars - Knights of the Old Republic"
		"KotOR II|Knights of the Old Republic II"
		"KotOR II|STAR WARS Knights of the Old Republic II"
		"Neverwinter Nights|Neverwinter Nights"
		"Jade Empire|Jade Empire"
	)

	for root in "${roots[@]}"; do
		[[ -d "${root}" ]] || continue
		for pair in "${labels[@]}"; do
			local label="${pair%%|*}"
			local folder="${pair#*|}"
			local path="${root}/${folder}"
			if [[ -d "${path}" ]]; then
				printf 'Path|%s|%s|%s\n' "${label}" "${path}" "${root}"
			fi
		done
	done
}

dedupe_entries() {
	awk -F'|' '!seen[$1 FS $3]++'
}

choose_entry() {
	local -n entries_ref=$1

	if [[ ${#entries_ref[@]} -eq 0 ]]; then
		read -r -p "No saved or detected installs were found. Enter a game path: " manual_path
		if [[ -z "${manual_path}" ]]; then
			echo "No game path provided." >&2
			exit 1
		fi
		printf 'Path|Manual path|%s|Manual\n' "${manual_path}"
		return 0
	fi

	echo
	echo "Available games and targets:"
	local i=1
	for entry in "${entries_ref[@]}"; do
		IFS='|' read -r kind name path desc <<<"${entry}"
		printf '[%d] %s (%s)\n' "${i}" "${name}" "${kind}"
		printf '    %s\n' "${path}"
		if [[ -n "${desc}" ]]; then
			printf '    %s\n' "${desc}"
		fi
		i=$((i + 1))
	done
	echo "[M] Manual path"

	while true; do
		read -r -p "Choose a number or M: " choice
		if [[ "${choice}" =~ ^[Mm]$ ]]; then
			read -r -p "Enter a game path: " manual_path
			if [[ -n "${manual_path}" ]]; then
				printf 'Path|Manual path|%s|Manual\n' "${manual_path}"
				return 0
			fi
		elif [[ "${choice}" =~ ^[0-9]+$ ]] && (( choice >= 1 && choice <= ${#entries_ref[@]} )); then
			printf '%s\n' "${entries_ref[$((choice - 1))]}"
			return 0
		fi
		echo "Please choose a valid option." >&2
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
	if [[ -n "${EXE_PATH}" ]]; then
		echo "Executable : ${EXE_PATH}"
	else
		echo "Executable : <not built yet>"
	fi
	echo "Config file: ${CONFIG_PATH}"
	for entry in "${ENTRIES[@]}"; do
		IFS='|' read -r kind name path desc <<<"${entry}"
		echo "${kind} | ${name} | ${path}"
	done
	exit 0
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
	echo "xoreos launcher"
	if [[ -n "${EXE_PATH}" ]]; then
		echo "Executable : ${EXE_PATH}"
	else
		echo "Executable : <not built yet>"
	fi
	echo "Config file: ${CONFIG_PATH}"
	selected="$(choose_entry ENTRIES)"
	IFS='|' read -r launch_kind _ launch_value _ <<<"${selected}"
fi

if [[ -z "${EXE_PATH}" ]]; then
	echo "xoreos was not found in the known build directories. Run scripts/bootstrap_linux.sh first." >&2
	exit 1
fi

args=()
if [[ "${launch_kind}" == "Target" ]]; then
	args+=("${launch_value}")
else
	args+=("-p${launch_value}")
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
echo "Launching: ${EXE_PATH} ${args[*]}"
"${EXE_PATH}" "${args[@]}"
