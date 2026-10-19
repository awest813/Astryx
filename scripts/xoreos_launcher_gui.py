#!/usr/bin/env python3
"""Astryx launcher: find a game install, check it, remember it, and start xoreos.

Run it without arguments to open the launcher window.

  --list          Print the engine, config file and game installs it finds.
  --check FOLDER  Check whether FOLDER is a game install xoreos can start.

Both work without a window. Everything above the "Window" section is plain
Python, so it can be tested and used on machines without Tk.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile


# -- GAMES --

@dataclass(frozen=True)
class Entry:
    """A file or directory, relative to the game folder."""
    path: str
    is_dir: bool = False
    case_sensitive: bool = False


def F(path: str, case_sensitive: bool = False) -> Entry:
    return Entry(path, False, case_sensitive)


def D(path: str, case_sensitive: bool = False) -> Entry:
    return Entry(path, True, case_sensitive)


@dataclass(frozen=True)
class Variant:
    """One release of a game. The engine recognises it by `identity`; `data`
    lists what else a complete install has."""
    platform: str
    identity: tuple[Entry, ...]
    data: tuple[Entry, ...] = ()


@dataclass(frozen=True)
class Game:
    key: str
    label: str
    short: str
    variants: tuple[Variant, ...]
    main: bool = False


# Mirrors the engine probes in src/engines/*/probes.cpp. If the identity files
# are missing, xoreos refuses the folder with "Unable to detect the game".
GAMES: tuple[Game, ...] = (
    Game("kotor", "Star Wars: Knights of the Old Republic", "KotOR I", main=True, variants=(
        Variant("Windows", (F("swkotor.exe"),), (F("chitin.key"), F("dialog.tlk"), D("modules"), D("texturepacks"))),
        Variant("macOS", (D("Knights of the Old Republic.app"),)),
        Variant("macOS", (D("KOTOR.app"),)),
        Variant("Xbox", (D("dataxbox"), F("players.erf"))),
        Variant("Android", (F("data/2da.bzf"),)),
    )),
    Game("kotor2", "Star Wars: Knights of the Old Republic II", "KotOR II", variants=(
        Variant("Windows", (F("swkotor2.exe"),), (F("chitin.key"), F("dialog.tlk"), D("modules"), D("texturepacks"))),
        Variant("Linux", (F("KOTOR2", True), D("resources"), D("steamassets"))),
        Variant("macOS", (D("MacOS"), D("Resources"), D("GameData"), F("MacOS/KOTOR2", True))),
        Variant("Xbox", (D("dataxbox"), F("weapons.erf"))),
    )),
    Game("nwn", "Neverwinter Nights", "Neverwinter Nights", variants=(
        Variant("Windows", (F("nwmain.exe"),), (F("chitin.key"), F("dialog.tlk"))),
        Variant("Linux", (F("nwmain"),), (F("chitin.key"), F("dialog.tlk"))),
        Variant("macOS", (D("Neverwinter Nights.app"),)),
        Variant("", (F("nwn.ini"),), (F("chitin.key"), F("dialog.tlk"))),
        Variant("", (F("nwnplayer.ini"),), (F("chitin.key"), F("dialog.tlk"))),
    )),
    Game("jade", "Jade Empire", "Jade Empire", variants=(
        Variant("Windows", (F("JadeEmpire.exe"),)),
        Variant("Xbox", (F("jadedirs.ini"),)),
        Variant("Android", (F("jadeempire.par"),)),
    )),
)

GAMES_BY_KEY = {game.key: game for game in GAMES}


def find_entry(root: Path, entry: Entry) -> Path | None:
    """Find `entry` under `root`, matching names case-insensitively like the
    engine does (unless the entry says otherwise)."""
    current = root
    parts = [part for part in entry.path.split("/") if part]
    for index, part in enumerate(parts):
        last = index == len(parts) - 1
        candidate = current / part
        match = candidate if candidate.exists() else None
        if match is None and not entry.case_sensitive:
            try:
                lowered = part.lower()
                match = next((child for child in current.iterdir() if child.name.lower() == lowered), None)
            except OSError:
                match = None
        if match is None:
            return None
        if last:
            if entry.is_dir != match.is_dir():
                return None
            return match
        current = match
    return None


def match_game(root: Path) -> tuple[Game, Variant, list[str]] | None:
    """The game and release in `root`, plus any missing data entries."""
    try:
        names = {child.name.lower() for child in root.iterdir()}
    except OSError:
        return None
    for game in GAMES:
        for variant in game.variants:
            if any(entry.path.split("/")[0].lower() not in names for entry in variant.identity):
                continue
            if all(find_entry(root, entry) for entry in variant.identity):
                missing = [entry.path + ("/" if entry.is_dir else "") for entry in variant.data if not find_entry(root, entry)]
                return game, variant, missing
    return None


@dataclass
class Check:
    state: str  # "empty", "ready", "warning" or "error"
    title: str
    summary: str
    details: list[str] = field(default_factory=list)
    game: str = ""  # Game key
    suggestion: str = ""  # A better folder to use, if we found one

    @property
    def launchable(self) -> bool:
        return self.state in ("ready", "warning")


def _child_dirs(root: Path, limit: int = 300) -> list[Path]:
    try:
        children = sorted((child for child in root.iterdir() if child.is_dir()), key=lambda p: p.name.lower())
    except OSError:
        return []
    return children[:limit]


def check_game_folder(path_text: str) -> Check:
    """Check whether xoreos can start the game in this folder, and say why not."""
    if not path_text.strip():
        return Check("empty", "No game folder chosen", "Pick a game from the list, or browse to the folder you installed the game in.")

    root = Path(path_text.strip()).expanduser()
    if not root.exists():
        return Check("error", "Folder not found", "This folder doesn't exist. It may have moved, or the drive may not be connected.",
                     [f"Looked for: {display_path(root)}"])

    if not root.is_dir():
        parent_match = match_game(root.parent)
        if parent_match:
            return Check("error", "That's a file, not a folder", "Choose the folder that contains it instead.",
                         [f"You chose: {display_path(root)}"], game=parent_match[0].key, suggestion=str(root.parent))
        return Check("error", "That's a file, not a folder", "Choose the folder the game is installed in.", [f"You chose: {display_path(root)}"])

    found = match_game(root)
    if found:
        game, variant, missing = found
        details = [f"Game: {game.label}" + (f" ({variant.platform} version)" if variant.platform else "")]
        if not game.main:
            details.append("Astryx is built for KotOR I. This game uses older upstream xoreos code and is mostly not playable yet.")
        if missing:
            details.append("Missing: " + ", ".join(missing))
            details.append("The install may be incomplete or damaged. Try verifying the game files in Steam or GOG, or reinstalling.")
            return Check("warning", "Some game files are missing", "xoreos recognises this game, but it may fail to start or crash later.",
                         details, game=game.key)
        return Check("ready", "Ready to play", f"{game.label} was found in this folder.", details, game=game.key)

    # Not a game folder. See if the user picked a folder next to the right one.
    for parent in [root.parent, root.parent.parent]:
        if parent != root and match_game(parent):
            return Check("error", "This is inside the game folder", "Choose the main game folder instead, the one that contains the game's program file.",
                         [f"You chose: {display_path(root)}", f"Game folder: {display_path(parent)}"], game=match_game(parent)[0].key, suggestion=str(parent))

    games_inside = [(child, match_game(child)) for child in _child_dirs(root)]
    games_inside = [(child, match) for child, match in games_inside if match]
    if len(games_inside) == 1:
        child, match = games_inside[0]
        return Check("error", "The game is in a folder inside this one", f"{match[0].label} is in {child.name}.",
                     [f"You chose: {display_path(root)}", f"Game folder: {display_path(child)}"], game=match[0].key, suggestion=str(child))
    if games_inside:
        return Check("error", "Several games are inside this folder", "Choose one of them from the list or with Browse.",
                     [f"You chose: {display_path(root)}"] + [f"{match[0].label}: {display_path(child)}" for child, match in games_inside])

    details = [f"You chose: {display_path(root)}"]
    if find_entry(root, F("chitin.key")):
        details.append("It has game data (chitin.key) but not the game's program file, which xoreos uses to tell the games apart.")
        details.append("KotOR I needs swkotor.exe in this folder; KotOR II needs swkotor2.exe.")
    else:
        details.append("For KotOR I, choose the folder that contains swkotor.exe, chitin.key and the modules folder.")
    return Check("error", "No game found here", "This folder doesn't contain a game xoreos knows.", details)


# -- LOCATIONS --

def display_path(path: str | Path) -> str:
    """A path for people to read: the home folder shown as ~."""
    text = str(path)
    home = str(Path.home())
    if home not in ("", "/") and (text == home or text.startswith(home + os.sep)):
        return "~" + text[len(home):]
    return text


def config_dir() -> Path:
    """The folder xoreos reads xoreos.conf from (Platform::getConfigDirectory)."""
    if os.name == "nt":
        for variable in ["APPDATA", "USERPROFILE"]:
            if os.environ.get(variable):
                return Path(os.environ[variable]) / "xoreos"
        return Path(".")
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Preferences" / "xoreos"
    if os.environ.get("XDG_CONFIG_HOME"):
        return Path(os.environ["XDG_CONFIG_HOME"]) / "xoreos"
    return Path.home() / ".config" / "xoreos"


def data_dir() -> Path:
    """The folder xoreos writes its log to (Platform::getUserDataDirectory)."""
    if os.name == "nt":
        return config_dir()
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support" / "xoreos"
    if os.environ.get("XDG_DATA_HOME"):
        return Path(os.environ["XDG_DATA_HOME"]) / "xoreos"
    return Path.home() / ".local" / "share" / "xoreos"


def config_path() -> Path:
    return config_dir() / "xoreos.conf"


def engine_log_path() -> Path:
    return data_dir() / "xoreos.log"


def run_output_path() -> Path:
    return data_dir() / "launcher-last-run.txt"


def settings_path() -> Path:
    return config_dir() / "launcher.json"


def load_settings() -> dict[str, str]:
    try:
        data = json.loads(settings_path().read_text(encoding="utf-8"))
        return {str(key): str(value) for key, value in data.items()} if isinstance(data, dict) else {}
    except (OSError, ValueError):
        return {}


def save_settings(settings: dict[str, str]) -> None:
    try:
        settings_path().parent.mkdir(parents=True, exist_ok=True)
        settings_path().write_text(json.dumps(settings, indent=2) + "\n", encoding="utf-8")
    except OSError:
        pass  # Only convenience state lives here


# -- XOREOS.CONF --

# The config format has no escaping: '#' starts a comment anywhere, '[' and ']'
# delimit section names anywhere on a line, and later '=' are dropped.
UNSAVABLE_CHARS = "#[]=\r\n"


def unsavable_chars(value: str) -> str:
    return "".join(sorted({char for char in value if char in UNSAVABLE_CHARS}))


def is_valid_target_name(name: str) -> bool:
    """ConfigFile::isValidName: ASCII letters, digits, '-', '_', '.' and spaces."""
    return bool(name.strip()) and all(not char.isascii() or char.isalnum() or char in "-_. " for char in name)


@dataclass
class Target:
    name: str
    values: dict[str, str]  # Lower-case keys

    @property
    def path(self) -> str:
        return self.values.get("path", "")


def _parse_line(line: str) -> tuple[str | None, str | None, str]:
    """Returns (section name, key, value) for one config line, roughly like
    ConfigFile::parseConfigLine."""
    stripped = line.strip()
    if not stripped or stripped.startswith(";"):
        return None, None, ""
    stripped = stripped.split("#", 1)[0].strip()
    if stripped.startswith("[") and "]" in stripped:
        return stripped[1:stripped.index("]")].strip(), None, ""
    if "=" in stripped:
        key, value = stripped.split("=", 1)
        return None, key.strip(), value.replace("=", "").strip()
    return None, None, ""


def read_targets(path: Path) -> list[Target]:
    """The game targets saved in xoreos.conf, in file order."""
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []
    targets: list[Target] = []
    current: Target | None = None
    for line in text.splitlines():
        section, key, value = _parse_line(line)
        if section is not None:
            current = Target(section, {})
            targets.append(current)
        elif key and current is not None:
            current.values[key.lower()] = value
    return [target for target in targets if target.name.lower() != "xoreos" and target.path]


def _section_span(lines: list[str], name: str) -> tuple[int, int] | None:
    start = None
    for index, line in enumerate(lines):
        section, _key, _value = _parse_line(line)
        if section is None:
            continue
        if start is not None:
            return start, index
        if section.lower() == name.lower():
            start = index
    return (start, len(lines)) if start is not None else None


def _write_config(path: Path, lines: list[str]) -> None:
    """Write atomically, keeping the previous version as xoreos.conf.bak."""
    path.parent.mkdir(parents=True, exist_ok=True)
    text = "\n".join(lines).rstrip() + "\n"
    handle, temp_name = tempfile.mkstemp(prefix=".xoreos.conf.", dir=str(path.parent))
    try:
        with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as temp:
            temp.write(text)
        if path.exists():
            try:
                backup = path.with_name(path.name + ".bak")
                backup.write_bytes(path.read_bytes())
            except OSError:
                pass
        os.replace(temp_name, path)
    except BaseException:
        try:
            os.unlink(temp_name)
        except OSError:
            pass
        raise


def save_target(path: Path, name: str, values: dict[str, str]) -> None:
    """Add or update a target in xoreos.conf. Keys the launcher doesn't set, and
    comments, are kept, so settings the game saved itself survive."""
    if not is_valid_target_name(name) or name.strip().lower() == "xoreos":
        raise ValueError(f'"{name}" can\'t be used as a saved name. Use letters, numbers, spaces, "-", "_" or ".", and not "xoreos".')
    for key, value in values.items():
        bad = unsavable_chars(value)
        if bad:
            what = "game folder" if key == "path" else key
            raise ValueError(f"The {what} contains {' '.join(repr(char) for char in bad)}, which xoreos.conf can't store. "
                             "Move or rename the folder, or launch without remembering the game.")

    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except FileNotFoundError:
        lines = []

    span = _section_span(lines, name)
    if span is None:
        while lines and not lines[-1].strip():
            lines.pop()
        if lines:
            lines.append("")
        lines.append(f"[{name}]")
        lines.extend(f"{key}={value}" for key, value in values.items())
        _write_config(path, lines)
        return

    start, end = span
    body = lines[start + 1:end]
    pending = dict(values)
    for index, line in enumerate(body):
        _section, key, _value = _parse_line(line)
        if key and key.lower() in pending:
            body[index] = f"{key}={pending.pop(key.lower())}"
    insert_at = len(body)
    while insert_at > 0 and not body[insert_at - 1].strip():
        insert_at -= 1
    body[insert_at:insert_at] = [f"{key}={value}" for key, value in pending.items()]
    _write_config(path, lines[:start + 1] + body + lines[end:])


def remove_target(path: Path, name: str) -> bool:
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except FileNotFoundError:
        return False
    span = _section_span(lines, name)
    if span is None:
        return False
    start, end = span
    lines = lines[:start] + lines[end:]
    _write_config(path, lines)
    return True


def same_path(a: str, b: str) -> bool:
    try:
        return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))
    except (OSError, ValueError):
        return os.path.normcase(os.path.normpath(a)) == os.path.normcase(os.path.normpath(b))


def suggest_target_name(game_key: str, folder: str, targets: list[Target]) -> str:
    """Reuse the name of a target with the same folder, else a short free name."""
    for target in targets:
        if same_path(target.path, folder):
            return target.name
    taken = {target.name.lower() for target in targets}
    base = game_key or re.sub(r"[^A-Za-z0-9]", "", Path(folder).name) or "game"
    if base.lower() not in taken and base.lower() != "xoreos":
        return base
    number = 2
    while f"{base}_{number}".lower() in taken:
        number += 1
    return f"{base}_{number}"


# -- FINDING INSTALLS --

def steam_roots() -> list[Path]:
    home = Path.home()
    roots: list[Path] = []
    if os.name == "nt":
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam") as key:
                roots.append(Path(winreg.QueryValueEx(key, "SteamPath")[0]))
        except (ImportError, OSError):
            pass
        for variable in ["ProgramFiles(x86)", "ProgramFiles"]:
            if os.environ.get(variable):
                roots.append(Path(os.environ[variable]) / "Steam")
    elif sys.platform == "darwin":
        roots.append(home / "Library" / "Application Support" / "Steam")
    else:
        roots.extend([
            home / ".local" / "share" / "Steam",
            home / ".steam" / "steam",
            home / ".steam" / "root",
            home / ".var" / "app" / "com.valvesoftware.Steam" / ".local" / "share" / "Steam",
            home / "snap" / "steam" / "common" / ".local" / "share" / "Steam",
        ])
    return [root for root in roots if root.is_dir()]


def steam_library_paths(vdf_text: str) -> list[Path]:
    """Library folders listed in Steam's libraryfolders.vdf."""
    return [Path(match.replace("\\\\", "\\")) for match in re.findall(r'"path"\s+"([^"]+)"', vdf_text)]


def fixed_drives() -> list[str]:
    """Local hard drive letters. Probing an empty CD drive can pop up a
    "no disk" dialog, so only fixed drives are searched."""
    try:
        import ctypes
        kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
        mask = kernel32.GetLogicalDrives()
        return [letter for index, letter in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ")
                if mask & (1 << index) and kernel32.GetDriveTypeW(f"{letter}:\\") == 3]
    except (ImportError, AttributeError, OSError):
        return ["C"]


def search_roots() -> list[tuple[Path, str]]:
    """Folders whose subfolders may be game installs, with where they come from."""
    found: list[tuple[Path, str]] = []
    for steam in steam_roots():
        libraries = [steam]
        for vdf in [steam / "steamapps" / "libraryfolders.vdf", steam / "config" / "libraryfolders.vdf"]:
            try:
                libraries.extend(steam_library_paths(vdf.read_text(encoding="utf-8", errors="replace")))
            except OSError:
                pass
        found.extend((library / "steamapps" / "common", "Steam") for library in libraries)

    home = Path.home()
    if os.name == "nt":
        for variable in ["ProgramFiles(x86)", "ProgramFiles"]:
            if os.environ.get(variable):
                found.append((Path(os.environ[variable]) / "GOG Galaxy" / "Games", "GOG"))
        for drive in fixed_drives():
            found.append((Path(f"{drive}:/GOG Games"), "GOG"))
            found.append((Path(f"{drive}:/Games"), "Games folder"))
    else:
        found.extend([(home / "GOG Games", "GOG"), (home / "Games", "Games folder"), (home / "Games" / "Heroic", "Heroic")])

    unique: list[tuple[Path, str]] = []
    for root, source in found:
        if root.is_dir() and not any(same_path(str(root), str(seen)) for seen, _ in unique):
            unique.append((root, source))
    return unique


@dataclass
class Install:
    name: str      # What the list shows
    path: str
    source: str    # "Saved", "Steam", "GOG", ...
    target: str = ""  # Saved target name, if any


def find_installs(targets: list[Target]) -> list[Install]:
    """Saved targets first, then installs found on disk that aren't saved yet."""
    installs = [Install(target.values.get("description") or target.name, target.path, "Saved", target.name) for target in targets]
    found: list[tuple[Game, Install]] = []
    for root, source in search_roots():
        for child in _child_dirs(root):
            match = match_game(child)
            if match and not any(same_path(str(child), install.path) for install in installs + [i for _, i in found]):
                found.append((match[0], Install(match[0].label, str(child), source)))
    # KotOR I first: it's the game Astryx is for
    found.sort(key=lambda pair: (not pair[0].main, pair[0].label.lower(), pair[1].path.lower()))
    return installs + [install for _game, install in found]


# -- ENGINE --

def _engine_names() -> list[str]:
    return ["xoreos.exe"] if os.name == "nt" else ["xoreos"]


def find_engines(repo_root: Path) -> list[Path]:
    """Every xoreos binary in the usual build folders, newest first. Covers the
    bootstrap/CI scripts, CMake presets and the portable Windows package."""
    bases = [repo_root]
    for pattern in ["build*", "builds/*"]:
        bases.extend(sorted(path for path in repo_root.glob(pattern) if path.is_dir()))
    found: list[Path] = []
    for base in bases:
        for config in ["", "Release", "RelWithDebInfo", "MinSizeRel", "Debug"]:
            for name in _engine_names():
                candidate = base / "bin" / config / name if config else base / "bin" / name
                if candidate.is_file() and (os.name == "nt" or os.access(candidate, os.X_OK)):
                    found.append(candidate)

    def mtime(path: Path) -> float:
        try:
            return path.stat().st_mtime
        except OSError:
            return 0.0

    return sorted(found, key=mtime, reverse=True)


def choose_engine(repo_root: Path, settings: dict[str, str]) -> Path | None:
    for chosen in [os.environ.get("XOREOS_EXE", ""), settings.get("engine", "")]:
        if chosen and Path(chosen).is_file():
            return Path(chosen)
    engines = find_engines(repo_root)
    return engines[0] if engines else None


def build_instructions() -> str:
    if os.name == "nt":
        return "Build it first:\n  powershell -ExecutionPolicy Bypass -File scripts\\bootstrap_windows.ps1"
    return ("Build it first, from the repository folder:\n"
            "  ./scripts/build_ci_linux.sh     (uses your system's libraries)\n"
            "  ./scripts/bootstrap_linux.sh    (downloads its own libraries)\n"
            "See BUILDDEP.md for what to install.")


def runtime_dll_dirs(repo_root: Path, engine: Path) -> list[Path]:
    """vcpkg DLL folders a Windows build may need on PATH."""
    if os.name != "nt":
        return []
    debug = "debug" in str(engine).lower()
    dirs: list[Path] = []
    for installed in list(repo_root.glob("build*/vcpkg_installed/*")) + list(repo_root.glob("builds/*/vcpkg_installed/*")) + list(repo_root.glob(".deps/vcpkg/installed/*")):
        release_bin, debug_bin = installed / "bin", installed / "debug" / "bin"
        dirs.extend(path for path in ([debug_bin, release_bin] if debug else [release_bin]) if path.is_dir())
    return dirs


@dataclass
class LaunchOptions:
    width: int = 0
    height: int = 0
    fullscreen: bool = False
    skip_videos: bool = False


def parse_resolution(text: str) -> tuple[int, int]:
    match = re.fullmatch(r"\s*(\d{2,5})\s*[xX×*]\s*(\d{2,5})\s*", text)
    if not match:
        raise ValueError(f'"{text}" isn\'t a screen size. Use width x height, for example 1280x720.')
    width, height = int(match.group(1)), int(match.group(2))
    if width < 320 or height < 240:
        raise ValueError("The screen size must be at least 320x240.")
    return width, height


def build_command(engine: str, target: str = "", path: str = "", options: LaunchOptions | None = None, remember: bool = True) -> list[str]:
    """The xoreos command line. A saved target carries its own settings; a
    one-off folder launch passes them, and asks xoreos not to save anything."""
    if target:
        return [engine, target]
    if not path:
        raise ValueError("Choose a game first.")
    command = [engine, f"--path={path}"]
    options = options or LaunchOptions()
    if options.width and options.height:
        command += [f"--width={options.width}", f"--height={options.height}"]
    command.append(f"--fullscreen={'true' if options.fullscreen else 'false'}")
    if options.skip_videos:
        command.append("--skipvideos=true")
    if not remember:
        command.append("--saveconf=false")
    return command


def quote_command(command: list[str]) -> str:
    def quote(part: str) -> str:
        return f'"{part}"' if not part or any(char.isspace() for char in part) else part
    return " ".join(quote(part) for part in command)


def error_excerpt(output: str, limit: int = 12) -> str:
    """The lines worth showing when xoreos stops with an error."""
    lines = [line.rstrip() for line in output.splitlines() if line.strip()]
    errors = [line for line in lines if line.lstrip().startswith(("ERROR", "FATAL", "terminate called"))]
    if errors:
        first = lines.index(errors[0])
        lines = lines[first:]
    return "\n".join(lines[-limit:])


# -- COMMAND LINE --

def print_list(repo_root: Path) -> int:
    settings = load_settings()
    engine = choose_engine(repo_root, settings)
    print(f"Engine      : {engine or 'not built yet'}")
    print(f"Config file : {config_path()}")
    print(f"Engine log  : {engine_log_path()}")
    installs = find_installs(read_targets(config_path()))
    if not installs:
        print("\nNo saved games and no installs found in the usual Steam and GOG folders.")
        print("Run with --check FOLDER, or open the launcher window and use Browse.")
        return 0
    print()
    for install in installs:
        check = check_game_folder(install.path)
        label = f"{install.name} [{install.target}]" if install.target else install.name
        print(f"{install.source:<12} {check.title:<28} {label}")
        print(f"{'':<12} {install.path}")
    return 0


def print_check(folder: str) -> int:
    check = check_game_folder(folder)
    print(f"{check.title}: {check.summary}")
    for line in check.details:
        print(f"  {line}")
    if check.suggestion:
        print(f"  Try: {check.suggestion}")
    return 0 if check.state == "ready" else 1


# -- WINDOW --

RESOLUTIONS = ["1280x720", "1366x768", "1600x900", "1920x1080", "2560x1440", "1024x768", "800x600"]
STATE_MARKS = {"ready": "✔", "warning": "⚠", "error": "✖", "empty": "•"}


def run_window(repo_root: Path) -> int:
    import tkinter as tk
    from tkinter import filedialog, font as tkfont, messagebox, ttk

    class LauncherWindow:
        def __init__(self, root: tk.Tk) -> None:
            self.root = root
            self.settings = load_settings()
            self.targets: list[Target] = []
            self.installs: list[Install] = []
            self.check = check_game_folder("")
            self.selected_target = ""  # Saved target the fields came from
            self.process: subprocess.Popen | None = None
            self.process_output = None
            self.validate_job: str | None = None
            self.engine: Path | None = None

            self.path_var = tk.StringVar()
            self.name_var = tk.StringVar()
            self.remember_var = tk.BooleanVar(value=True)
            self.mode_var = tk.StringVar(value="windowed")
            self.resolution_var = tk.StringVar(value="1280x720")
            self.skip_videos_var = tk.BooleanVar(value=False)
            self.check_title_var = tk.StringVar()
            self.check_summary_var = tk.StringVar()
            self.command_var = tk.StringVar()
            self.engine_var = tk.StringVar()
            self.status_var = tk.StringVar()

            root.title("Astryx Launcher")
            root.geometry("1040x660")
            root.minsize(840, 560)
            self.build()
            self.rescan(select_path=self.settings.get("last_path", ""))

        # -- Layout --

        def build(self) -> None:
            base = tkfont.nametofont("TkDefaultFont")
            self.heading_font = base.copy()
            self.heading_font.configure(size=base.cget("size") + 6, weight="bold")
            self.bold_font = base.copy()
            self.bold_font.configure(weight="bold")
            self.big_font = base.copy()
            self.big_font.configure(size=base.cget("size") + 2, weight="bold")

            style = ttk.Style(self.root)
            style.configure("Ready.TLabel", foreground="#1d7a33")
            style.configure("Warning.TLabel", foreground="#a15c00")
            style.configure("Error.TLabel", foreground="#b3261e")
            style.configure("Empty.TLabel", foreground="#555555")
            style.configure("Launch.TButton", font=self.bold_font, padding=(18, 6))

            self.root.columnconfigure(0, weight=1)
            self.root.rowconfigure(1, weight=1)

            header = ttk.Frame(self.root, padding=(16, 14, 16, 6))
            header.grid(row=0, column=0, sticky="ew")
            ttk.Label(header, text="Astryx Launcher", font=self.heading_font).grid(row=0, column=0, sticky="w")
            ttk.Label(header, text="Pick your copy of Knights of the Old Republic, check it, and play.").grid(row=1, column=0, sticky="w")

            body = ttk.Frame(self.root, padding=(16, 6, 16, 6))
            body.grid(row=1, column=0, sticky="nsew")
            body.columnconfigure(0, weight=3)
            body.columnconfigure(1, weight=2)
            body.rowconfigure(0, weight=1)

            # Games list
            games = ttk.LabelFrame(body, text="Games", padding=10)
            games.grid(row=0, column=0, sticky="nsew", padx=(0, 10))
            games.columnconfigure(0, weight=1)
            games.rowconfigure(0, weight=1)
            self.tree = ttk.Treeview(games, columns=("status", "game", "source", "path"), show="headings", selectmode="browse")
            for column, title, width, stretch in [("status", "", 28, False), ("game", "Game", 150, False), ("source", "Found in", 130, False), ("path", "Folder", 300, True)]:
                self.tree.heading(column, text=title, anchor="w")
                self.tree.column(column, width=width, minwidth=width if not stretch else 120, stretch=stretch, anchor="w")
            self.tree.grid(row=0, column=0, sticky="nsew")
            self.tree.bind("<<TreeviewSelect>>", self.on_select)
            scroll = ttk.Scrollbar(games, orient="vertical", command=self.tree.yview)
            scroll.grid(row=0, column=1, sticky="ns")
            self.tree.configure(yscrollcommand=scroll.set)
            self.empty_label = ttk.Label(games, text="No games found yet.\nUse “Add game folder…” to pick where you installed the game.", justify="center", style="Empty.TLabel")
            buttons = ttk.Frame(games)
            buttons.grid(row=1, column=0, columnspan=2, sticky="ew", pady=(8, 0))
            ttk.Button(buttons, text="Add game folder…", command=self.browse).pack(side="left")
            ttk.Button(buttons, text="Search again", command=lambda: self.rescan(select_path=self.path_var.get())).pack(side="left", padx=(6, 0))
            self.forget_button = ttk.Button(buttons, text="Forget saved game", command=self.forget)
            self.forget_button.pack(side="right")

            # Selected game
            panel = ttk.LabelFrame(body, text="Selected game", padding=10)
            panel.grid(row=0, column=1, sticky="nsew")
            panel.columnconfigure(1, weight=1)
            row = 0
            ttk.Label(panel, text="Game folder").grid(row=row, column=0, sticky="w")
            path_entry = ttk.Entry(panel, textvariable=self.path_var)
            path_entry.grid(row=row, column=1, sticky="ew", padx=(8, 6))
            ttk.Button(panel, text="Browse…", command=self.browse).grid(row=row, column=2)
            row += 1
            self.check_title = ttk.Label(panel, textvariable=self.check_title_var, font=self.big_font)
            self.check_title.grid(row=row, column=0, columnspan=3, sticky="w", pady=(10, 0))
            row += 1
            self.check_summary = ttk.Label(panel, textvariable=self.check_summary_var, wraplength=360, justify="left")
            self.check_summary.grid(row=row, column=0, columnspan=3, sticky="w")
            row += 1
            self.details = ttk.Label(panel, wraplength=360, justify="left", style="Empty.TLabel")
            self.details.grid(row=row, column=0, columnspan=3, sticky="ew", pady=(4, 0))
            row += 1
            self.suggest_button = ttk.Button(panel, text="Use the suggested folder", command=self.use_suggestion)
            self.suggest_button.grid(row=row, column=0, columnspan=3, sticky="w", pady=(4, 0))
            row += 1

            ttk.Separator(panel).grid(row=row, column=0, columnspan=3, sticky="ew", pady=10)
            row += 1
            ttk.Label(panel, text="Display").grid(row=row, column=0, sticky="w")
            display = ttk.Frame(panel)
            display.grid(row=row, column=1, columnspan=2, sticky="w", padx=(8, 0))
            ttk.Radiobutton(display, text="Window", variable=self.mode_var, value="windowed", command=self.refresh_command).pack(side="left")
            ttk.Radiobutton(display, text="Full screen", variable=self.mode_var, value="fullscreen", command=self.refresh_command).pack(side="left", padx=(10, 0))
            row += 1
            ttk.Label(panel, text="Size").grid(row=row, column=0, sticky="w", pady=(6, 0))
            ttk.Combobox(panel, textvariable=self.resolution_var, values=RESOLUTIONS, width=12).grid(row=row, column=1, sticky="w", padx=(8, 0), pady=(6, 0))
            row += 1
            ttk.Checkbutton(panel, text="Skip the intro movies", variable=self.skip_videos_var, command=self.refresh_command).grid(row=row, column=0, columnspan=3, sticky="w", pady=(6, 0))
            row += 1
            ttk.Checkbutton(panel, text="Remember this game and these settings", variable=self.remember_var, command=self.refresh_command).grid(row=row, column=0, columnspan=3, sticky="w", pady=(6, 0))
            row += 1
            ttk.Label(panel, text="Saved as").grid(row=row, column=0, sticky="w", pady=(6, 0))
            self.name_entry = ttk.Entry(panel, textvariable=self.name_var)
            self.name_entry.grid(row=row, column=1, columnspan=2, sticky="ew", padx=(8, 0), pady=(6, 0))
            row += 1

            panel.rowconfigure(row, weight=1)
            row += 1
            actions = ttk.Frame(panel)
            actions.grid(row=row, column=0, columnspan=3, sticky="ew", pady=(10, 0))
            self.launch_button = ttk.Button(actions, text="Play", style="Launch.TButton", command=self.launch)
            self.launch_button.pack(side="right")
            self.save_button = ttk.Button(actions, text="Save only", command=self.save_only)
            self.save_button.pack(side="right", padx=(0, 6))
            row += 1
            command = ttk.Entry(panel, textvariable=self.command_var, state="readonly", font=tkfont.nametofont("TkFixedFont"))
            command.grid(row=row, column=0, columnspan=3, sticky="ew", pady=(8, 0))

            # Footer
            footer = ttk.Frame(self.root, padding=(16, 4, 16, 12))
            footer.grid(row=2, column=0, sticky="ew")
            footer.columnconfigure(1, weight=1)
            ttk.Label(footer, text="Engine:", font=self.bold_font).grid(row=0, column=0, sticky="w")
            ttk.Label(footer, textvariable=self.engine_var).grid(row=0, column=1, sticky="w", padx=(6, 0))
            ttk.Button(footer, text="Choose engine…", command=self.choose_engine).grid(row=0, column=2, padx=(6, 0))
            ttk.Button(footer, text="Open log", command=self.open_log).grid(row=0, column=3, padx=(6, 0))
            ttk.Button(footer, text="Open config folder", command=lambda: self.open_folder(config_dir())).grid(row=0, column=4, padx=(6, 0))
            ttk.Label(footer, textvariable=self.status_var, style="Empty.TLabel").grid(row=1, column=0, columnspan=5, sticky="w", pady=(6, 0))

            self.path_var.trace_add("write", lambda *_: self.schedule_check())
            for variable in [self.name_var, self.resolution_var]:
                variable.trace_add("write", lambda *_: self.refresh_command())
            def rewrap(event: tk.Event) -> None:
                for label in [self.check_summary, self.details]:
                    label.configure(wraplength=max(240, event.width - 30))
            panel.bind("<Configure>", rewrap)

        # -- Games --

        def rescan(self, select_path: str = "") -> None:
            self.status_var.set("Searching for games…")
            self.root.update_idletasks()
            self.targets = read_targets(config_path())
            self.installs = find_installs(self.targets)
            self.tree.delete(*self.tree.get_children())
            selected = None
            for index, install in enumerate(self.installs):
                check = check_game_folder(install.path)
                game = GAMES_BY_KEY.get(check.game)
                label = game.short if game else install.name
                source = f"Saved as {install.target}" if install.target else install.source
                self.tree.insert("", "end", iid=str(index), values=(STATE_MARKS[check.state], label, source, display_path(install.path)))
                if selected is None and select_path and same_path(install.path, select_path):
                    selected = str(index)
            if self.installs:
                self.empty_label.grid_forget()
            else:
                self.empty_label.grid(row=0, column=0, columnspan=2)
            self.refresh_engine()
            if selected is None and self.installs:
                selected = "0"
            if selected is not None:
                self.tree.selection_set(selected)
                self.tree.see(selected)
            elif select_path:
                self.load_folder(select_path)
            else:
                self.load_folder("")
            saved = sum(1 for install in self.installs if install.target)
            found = len(self.installs) - saved
            self.status_var.set(f"{saved} saved, {found} found on this computer." if self.installs else "No games found in the usual Steam and GOG folders.")

        def on_select(self, _event: object = None) -> None:
            selection = self.tree.selection()
            if not selection:
                return
            install = self.installs[int(selection[0])]
            if install.target:
                target = next((t for t in self.targets if t.name == install.target), None)
                values = target.values if target else {}
                self.selected_target = install.target
                self.name_var.set(install.target)
                self.mode_var.set("fullscreen" if values.get("fullscreen", "").lower() in ("true", "yes", "y", "on", "1") else "windowed")
                if values.get("width") and values.get("height"):
                    self.resolution_var.set(f"{values['width']}x{values['height']}")
                else:
                    self.resolution_var.set(RESOLUTIONS[0])
                self.skip_videos_var.set(values.get("skipvideos", "").lower() in ("true", "yes", "y", "on", "1"))
                self.remember_var.set(True)
            else:
                self.selected_target = ""
                self.name_var.set("")
            self.path_var.set(install.path)
            self.run_check()

        def load_folder(self, folder: str) -> None:
            self.selected_target = ""
            self.name_var.set("")
            self.path_var.set(folder)
            self.run_check()

        def browse(self) -> None:
            start = self.path_var.get().strip() or str(Path.home())
            folder = filedialog.askdirectory(title="Choose the folder you installed the game in", initialdir=start if Path(start).is_dir() else str(Path.home()), mustexist=True)
            if folder:
                self.tree.selection_remove(*self.tree.selection())
                self.load_folder(str(Path(folder)))

        def use_suggestion(self) -> None:
            if self.check.suggestion:
                self.load_folder(self.check.suggestion)

        def forget(self) -> None:
            name = self.selected_target
            if not name:
                return
            if not messagebox.askyesno("Forget saved game", f"Remove “{name}” from xoreos.conf?\n\nThe game itself and its saved games are not touched.", parent=self.root):
                return
            try:
                remove_target(config_path(), name)
            except OSError as exc:
                messagebox.showerror("Couldn't update xoreos.conf", str(exc), parent=self.root)
                return
            self.rescan(select_path=self.path_var.get())
            self.status_var.set(f"Forgot “{name}”.")

        # -- Checking --

        def schedule_check(self) -> None:
            if self.validate_job:
                self.root.after_cancel(self.validate_job)
            self.validate_job = self.root.after(300, self.run_check)

        def run_check(self) -> None:
            self.validate_job = None
            folder = self.path_var.get().strip()
            if self.selected_target:
                target = next((t for t in self.targets if t.name == self.selected_target), None)
                if not target or not same_path(target.path, folder):
                    self.selected_target = ""
                    self.name_var.set("")
            self.check = check_game_folder(folder)
            if self.check.launchable and not self.name_var.get().strip():
                self.name_var.set(suggest_target_name(self.check.game, folder, self.targets))
            self.check_title_var.set(f"{STATE_MARKS[self.check.state]}  {self.check.title}")
            self.check_title.configure(style=f"{self.check.state.capitalize()}.TLabel")
            self.check_summary_var.set(self.check.summary)
            self.details.configure(text="\n".join(self.check.details))
            if self.check.suggestion:
                self.suggest_button.configure(text=f"Use {Path(self.check.suggestion).name}")
                self.suggest_button.grid()
            else:
                self.suggest_button.grid_remove()
            self.forget_button.state(["!disabled"] if self.selected_target else ["disabled"])
            self.launch_button.state(["!disabled"] if self.check.launchable else ["disabled"])
            self.save_button.state(["!disabled"] if self.check.launchable else ["disabled"])
            self.refresh_command()

        def refresh_engine(self) -> None:
            self.engine = choose_engine(repo_root, self.settings)
            self.engine_var.set(display_path(self.engine) if self.engine else "not built yet")

        def options(self) -> LaunchOptions:
            width, height = parse_resolution(self.resolution_var.get())
            return LaunchOptions(width, height, self.mode_var.get() == "fullscreen", self.skip_videos_var.get())

        def target_values(self) -> dict[str, str]:
            options = self.options()
            game = GAMES_BY_KEY.get(self.check.game)
            values = {"path": Path(self.path_var.get().strip()).expanduser().resolve().as_posix()}
            if game:
                values["description"] = game.label
            values.update({
                "width": str(options.width), "height": str(options.height),
                "fullscreen": "true" if options.fullscreen else "false",
                "skipvideos": "true" if options.skip_videos else "false",
            })
            return values

        def refresh_command(self) -> None:
            self.name_entry.state(["!disabled"] if self.remember_var.get() else ["disabled"])
            try:
                engine = str(self.engine) if self.engine else "xoreos"
                if self.remember_var.get():
                    command = build_command(engine, target=self.name_var.get().strip() or "<name>")
                else:
                    command = build_command(engine, path=self.path_var.get().strip(), options=self.options(), remember=False)
                self.command_var.set(quote_command(command))
            except ValueError as exc:
                self.command_var.set(str(exc))

        # -- Saving and launching --

        def save(self) -> str | None:
            """Save the selected game to xoreos.conf. Returns its name, or None."""
            name = self.name_var.get().strip()
            try:
                if not name:
                    raise ValueError("Enter a name to save the game under.")
                values = self.target_values()
                for target in self.targets:
                    if target.name.lower() == name.lower() and not same_path(target.path, values["path"]) and target.name != self.selected_target:
                        raise ValueError(f"“{target.name}” is already saved for a different folder:\n{target.path}\n\nPick another name.")
                old = next((t for t in self.targets if t.name == self.selected_target), None)
                if old and old.name.lower() != name.lower():
                    # Renamed: carry over what the game saved under the old name
                    values = {**old.values, **values}
                    save_target(config_path(), name, values)
                    remove_target(config_path(), old.name)
                else:
                    save_target(config_path(), name, values)
            except (ValueError, OSError) as exc:
                messagebox.showerror("Couldn't save the game", str(exc), parent=self.root)
                return None
            self.rescan(select_path=self.path_var.get())
            return name

        def save_only(self) -> None:
            if self.save():
                self.status_var.set(f"Saved to {config_path()}.")

        def launch(self) -> None:
            if not self.check.launchable:
                return
            if not self.engine:
                messagebox.showerror("xoreos isn't built yet", "The launcher couldn't find the xoreos program.\n\n" + build_instructions()
                                     + "\n\nIf you built it somewhere else, use “Choose engine…”.", parent=self.root)
                return
            if self.process and self.process.poll() is None:
                if not messagebox.askyesno("Already running", "xoreos is already running. Start another copy?", parent=self.root):
                    return
            if self.check.state == "warning":
                if not messagebox.askyesno("Some game files are missing", self.check.summary + "\n\nStart anyway?", icon="warning", parent=self.root):
                    return
            try:
                if self.remember_var.get():
                    name = self.save()
                    if not name:
                        return
                    command = build_command(str(self.engine), target=name)
                else:
                    command = build_command(str(self.engine), path=self.path_var.get().strip(), options=self.options(), remember=False)
            except ValueError as exc:
                messagebox.showerror("Can't start the game", str(exc), parent=self.root)
                return
            self.start(command)

        def start(self, command: list[str]) -> None:
            self.settings["last_path"] = self.path_var.get().strip()
            save_settings(self.settings)
            env = os.environ.copy()
            dll_dirs = runtime_dll_dirs(repo_root, Path(command[0]))
            if dll_dirs:
                env["PATH"] = os.pathsep.join(str(path) for path in dll_dirs) + os.pathsep + env.get("PATH", "")
            try:
                run_output_path().parent.mkdir(parents=True, exist_ok=True)
                self.process_output = open(run_output_path(), "w", encoding="utf-8", errors="replace")
                self.process_output.write("Command: " + quote_command(command) + "\n\n")
                self.process_output.flush()
                flags = getattr(subprocess, "CREATE_NO_WINDOW", 0) if os.name == "nt" else 0
                self.process = subprocess.Popen(command, cwd=str(Path(command[0]).parent), env=env, stdin=subprocess.DEVNULL,
                                                stdout=self.process_output, stderr=subprocess.STDOUT, creationflags=flags)
            except OSError as exc:
                messagebox.showerror("Couldn't start xoreos", f"{exc}\n\nProgram: {command[0]}", parent=self.root)
                self.status_var.set("Couldn't start xoreos.")
                return
            self.status_var.set("xoreos is running. Its output goes to " + str(run_output_path()))
            self.root.after(500, self.watch)

        def watch(self) -> None:
            if not self.process:
                return
            code = self.process.poll()
            if code is None:
                self.root.after(500, self.watch)
                return
            if self.process_output:
                self.process_output.close()
                self.process_output = None
            self.process = None
            if code == 0:
                self.status_var.set("xoreos closed.")
                return
            try:
                output = run_output_path().read_text(encoding="utf-8", errors="replace")
            except OSError:
                output = ""
            excerpt = error_excerpt(output.split("\n\n", 1)[-1]) or "(xoreos printed nothing.)"
            self.status_var.set(f"xoreos stopped with an error (exit code {code}).")
            self.show_failure(code, excerpt)

        def show_failure(self, code: int, excerpt: str) -> None:
            dialog = tk.Toplevel(self.root)
            dialog.title("xoreos stopped with an error")
            dialog.transient(self.root)
            dialog.columnconfigure(0, weight=1)
            dialog.rowconfigure(1, weight=1)
            ttk.Label(dialog, text=f"xoreos stopped with an error (exit code {code}).", font=self.big_font, padding=(14, 12, 14, 4)).grid(row=0, column=0, sticky="w")
            text = tk.Text(dialog, height=min(12, excerpt.count("\n") + 2), width=90, wrap="word")
            text.insert("1.0", excerpt)
            text.configure(state="disabled")
            text.grid(row=1, column=0, sticky="nsew", padx=14)
            ttk.Label(dialog, text="If the message mentions the game folder, check it in the launcher. Otherwise the full output and\n"
                                   "the engine log have the details; include them if you report the problem.\n\n"
                                   f"Full output: {display_path(run_output_path())}\nEngine log: {display_path(engine_log_path())}",
                      padding=(14, 8), justify="left").grid(row=2, column=0, sticky="w")
            buttons = ttk.Frame(dialog, padding=(14, 0, 14, 12))
            buttons.grid(row=3, column=0, sticky="e")
            ttk.Button(buttons, text="Open full output", command=lambda: self.open_file(run_output_path())).pack(side="left")
            ttk.Button(buttons, text="Close", command=dialog.destroy).pack(side="left", padx=(6, 0))
            dialog.bind("<Escape>", lambda _event: dialog.destroy())

        # -- Files and folders --

        def choose_engine(self) -> None:
            pattern = [("xoreos", "xoreos.exe"), ("All files", "*.*")] if os.name == "nt" else [("All files", "*")]
            chosen = filedialog.askopenfilename(title="Choose the xoreos program", filetypes=pattern, parent=self.root)
            if not chosen:
                return
            self.settings["engine"] = chosen
            save_settings(self.settings)
            self.refresh_engine()
            self.refresh_command()

        def open_log(self) -> None:
            for path in [engine_log_path(), run_output_path()]:
                if path.exists():
                    self.open_file(path)
                    return
            messagebox.showinfo("No log yet", f"xoreos hasn't written a log yet. It will be here:\n{engine_log_path()}", parent=self.root)

        def open_folder(self, folder: Path) -> None:
            try:
                folder.mkdir(parents=True, exist_ok=True)
            except OSError:
                pass
            self.open_file(folder)

        def open_file(self, path: Path) -> None:
            try:
                if os.name == "nt":
                    os.startfile(str(path))  # type: ignore[attr-defined]
                elif sys.platform == "darwin":
                    subprocess.Popen(["open", str(path)])
                else:
                    subprocess.Popen(["xdg-open", str(path)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except OSError as exc:
                messagebox.showerror("Couldn't open it", f"{exc}\n\n{path}", parent=self.root)

    try:
        root = tk.Tk()
    except tk.TclError as exc:
        print(f"The launcher window can't open: {exc}", file=sys.stderr)
        print("Without a display, use: scripts/xoreos_launcher_gui.py --list, or scripts/run_xoreos_linux.sh", file=sys.stderr)
        return 1
    try:
        style = ttk.Style(root)
        if os.name == "nt" and "vista" in style.theme_names():
            style.theme_use("vista")
        elif "clam" in style.theme_names() and sys.platform != "darwin":
            style.theme_use("clam")
    except tk.TclError:
        pass
    LauncherWindow(root)
    root.mainloop()
    return 0


def missing_tk_message() -> str:
    lines = ["The launcher window needs Python's Tk support (tkinter), which isn't installed."]
    if os.name == "nt":
        lines.append("Reinstall Python from python.org and keep \"tcl/tk and IDLE\" ticked.")
    elif sys.platform == "darwin":
        lines.append("Install it with: brew install python-tk")
    else:
        lines.append("Install it with one of:")
        lines.append("  sudo apt install python3-tk      (Debian, Ubuntu)")
        lines.append("  sudo dnf install python3-tkinter (Fedora)")
        lines.append("  sudo pacman -S tk                (Arch)")
        lines.append("Or start the game from the terminal: scripts/run_xoreos_linux.sh")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Find, check and start games with xoreos.")
    parser.add_argument("--list", action="store_true", help="print the engine, config file and game installs found, then exit")
    parser.add_argument("--check", metavar="FOLDER", help="check whether FOLDER is a game install xoreos can start")
    args = parser.parse_args(argv)
    repo_root = Path(__file__).resolve().parent.parent

    if args.check is not None:
        return print_check(args.check)
    if args.list:
        return print_list(repo_root)
    try:
        import tkinter  # noqa: F401
    except ImportError:
        print(missing_tk_message(), file=sys.stderr)
        return 1
    return run_window(repo_root)


if __name__ == "__main__":
    sys.exit(main())
