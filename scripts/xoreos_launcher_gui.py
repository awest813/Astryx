#!/usr/bin/env python3
"""Desktop launcher and first-run setup assistant for xoreos."""

from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk


GAME_DEFS = [
    {
        "label": "KotOR I",
        "hints": ["swkotor", "Star Wars - Knights of the Old Republic", "KotOR I"],
        "target": "kotor",
        "markers": [
            ("chitin.key", True),
            ("dialog.tlk", True),
            ("modules", True),
            ("swpc_tex_gui.erf", True),
            ("texturepacks", True),
            ("override", False),
        ],
    },
    {
        "label": "KotOR II",
        "hints": ["Knights of the Old Republic II", "STAR WARS Knights of the Old Republic II", "KotOR II"],
        "target": "kotor2",
        "markers": [
            ("chitin.key", True),
            ("dialog.tlk", True),
            ("modules", True),
            ("swpc_tex_gui.erf", True),
            ("texturepacks", True),
            ("override", False),
        ],
    },
    {
        "label": "Neverwinter Nights",
        "hints": ["Neverwinter Nights"],
        "target": "nwn",
        "markers": [("chitin.key", True), ("dialog.tlk", True), ("data", True), ("hak", False)],
    },
    {
        "label": "Jade Empire",
        "hints": ["Jade Empire"],
        "target": "jade",
        "markers": [("data", True), ("dialog.tlk", True)],
    },
]


def get_xoreos_executable(repo_root: Path) -> Path | None:
    for candidate in [
        repo_root / "bin" / "Release" / "xoreos.exe",
        repo_root / "bin" / "Debug" / "xoreos.exe",
        repo_root / "build-vcpkg-portable" / "bin" / "Release" / "xoreos.exe",
        repo_root / "build-vcpkg" / "bin" / "Release" / "xoreos.exe",
        repo_root / "build-vcpkg-portable" / "bin" / "Debug" / "xoreos.exe",
        repo_root / "build-vcpkg" / "bin" / "Debug" / "xoreos.exe",
        repo_root / "build" / "bin" / "Debug" / "xoreos.exe",
        repo_root / "build-vcpkg-linux" / "bin" / "xoreos",
        repo_root / "build-vcpkg-portable" / "bin" / "xoreos",
        repo_root / "build" / "bin" / "xoreos",
    ]:
        if candidate.exists():
            return candidate
    return None


def get_runtime_dll_dirs(repo_root: Path) -> list[Path]:
    candidates = [
        repo_root / "build-vcpkg" / "vcpkg_installed" / "x64-windows" / "bin",
        repo_root / "build-vcpkg" / "vcpkg_installed" / "x64-windows" / "debug" / "bin",
        repo_root / "build-vcpkg-portable" / "vcpkg_installed" / "x64-windows" / "bin",
        repo_root / "build-vcpkg-portable" / "vcpkg_installed" / "x64-windows" / "debug" / "bin",
        repo_root / ".deps" / "vcpkg" / "installed" / "x64-windows" / "bin",
        repo_root / ".deps" / "vcpkg" / "installed" / "x64-windows" / "debug" / "bin",
    ]
    return [path for path in candidates if path.exists()]


def get_config_path() -> Path:
    candidates: list[Path] = []
    if os.name == "nt":
        if os.environ.get("APPDATA"):
            candidates.append(Path(os.environ["APPDATA"]) / "xoreos" / "xoreos.conf")
        if os.environ.get("USERPROFILE"):
            candidates.append(Path(os.environ["USERPROFILE"]) / ".config" / "xoreos" / "xoreos.conf")
            candidates.append(Path(os.environ["USERPROFILE"]) / "xoreos" / "xoreos.conf")
    else:
        if os.environ.get("XDG_CONFIG_HOME"):
            candidates.append(Path(os.environ["XDG_CONFIG_HOME"]) / "xoreos" / "xoreos.conf")
        candidates.append(Path.home() / ".config" / "xoreos" / "xoreos.conf")
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0] if candidates else Path("xoreos.conf")


def sanitize_target_name(raw: str) -> str:
    value = re.sub(r"[^-_. A-Za-z0-9]", " ", raw).strip()
    return re.sub(r"\s+", " ", value)[:64] or "game"


def normalized_path_key(path_text: str) -> str:
    normalized = os.path.normpath(path_text.strip())
    return os.path.normcase(normalized)


def scan_roots() -> list[Path]:
    roots: list[Path] = []
    if os.name == "nt":
        for value in [os.environ.get("ProgramFiles(x86)"), os.environ.get("ProgramFiles")]:
            if value:
                roots.append(Path(value) / "Steam" / "steamapps" / "common")
                roots.append(Path(value) / "GOG Galaxy" / "Games")
        roots.extend([Path("C:/GOG Games"), Path("D:/GOG Games")])
    else:
        roots.extend(
            [
                Path.home() / ".local" / "share" / "Steam" / "steamapps" / "common",
                Path.home() / ".steam" / "steam" / "steamapps" / "common",
                Path.home() / "Games",
            ]
        )
    return [root for root in roots if root.exists()]


def read_config_targets(config_path: Path) -> list[dict[str, str]]:
    if not config_path.exists():
        return []
    entries: list[dict[str, str]] = []
    name = ""
    path = ""
    description = ""

    def emit() -> None:
        if name and name != "xoreos" and path:
            entries.append(
                {
                    "name": name,
                    "path": path,
                    "description": description,
                    "source": "Saved target",
                    "launch_kind": "Target",
                    "launch_value": name,
                }
            )

    for raw in config_path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith(";"):
            continue
        if line.startswith("[") and line.endswith("]"):
            emit()
            name = line[1:-1]
            path = ""
            description = ""
        elif name and line.startswith("path="):
            path = line[5:].strip()
        elif name and line.startswith("description="):
            description = line[12:].strip()
    emit()
    return entries


def detect_installs() -> list[dict[str, str]]:
    entries: list[dict[str, str]] = []
    seen: set[str] = set()
    for root in scan_roots():
        for game in GAME_DEFS:
            for hint in game["hints"]:
                candidate = root / hint
                if candidate.exists() and str(candidate) not in seen:
                    seen.add(str(candidate))
                    entries.append(
                        {
                            "name": game["label"],
                            "path": str(candidate),
                            "description": str(root),
                            "source": "Detected install",
                            "launch_kind": "Path",
                            "launch_value": str(candidate),
                        }
                    )
    return entries


def guess_game(path_text: str, selected_name: str) -> dict[str, object]:
    lowered = f"{selected_name} {Path(path_text).name}".lower()
    for game in GAME_DEFS:
        if any(hint.lower() in lowered for hint in game["hints"]):
            return game
    best = GAME_DEFS[0]
    best_score = -1
    root = Path(path_text)
    for game in GAME_DEFS:
        score = sum(2 if required else 1 for marker, required in game["markers"] if (root / marker).exists())
        if score > best_score:
            best = game
            best_score = score
    return best


def validate_game_path(path_text: str, selected_name: str) -> dict[str, object]:
    if not path_text.strip():
        return {
            "state": "pending",
            "title": "Choose a game folder",
            "summary": "Select a detected install or browse to the root game folder first.",
            "details": ["Examples: KotOR needs chitin.key, dialog.tlk, and modules/ in the selected folder."],
            "game": "",
            "target": "",
        }
    root = Path(path_text).expanduser()
    if not root.exists():
        return {"state": "error", "title": "Folder not found", "summary": "The selected folder does not exist.", "details": [f"Checked path: {root}"], "game": "", "target": ""}
    if not root.is_dir():
        return {"state": "error", "title": "Choose a folder", "summary": "xoreos needs the root game directory, not a file.", "details": [f"Checked path: {root}"], "game": "", "target": ""}
    game = guess_game(str(root), selected_name)
    found = [marker for marker, _required in game["markers"] if (root / marker).exists()]
    missing_required = [marker for marker, required in game["markers"] if required and not (root / marker).exists()]
    target = sanitize_target_name(f"{game['target']} {root.name}")
    if not missing_required:
        return {
            "state": "ready",
            "title": "Assets look ready",
            "summary": "The selected folder contains the core files xoreos usually expects.",
            "details": [f"Detected game: {game['label']}", f"Folder: {root}", "Found markers: " + ", ".join(found)],
            "game": game["label"],
            "target": target,
        }
    if found:
        return {
            "state": "warning",
            "title": "Folder looks close, but incomplete",
            "summary": "Some game assets were found, but at least one required marker is missing.",
            "details": [f"Best guess: {game['label']}", f"Folder: {root}", "Found markers: " + ", ".join(found), "Missing required markers: " + ", ".join(missing_required), "Try the top-level Steam, GOG, or disc install folder."],
            "game": game["label"],
            "target": target,
        }
    return {
        "state": "error",
        "title": "This does not look like a game asset folder",
        "summary": "The selected directory is missing the common files and folders xoreos expects.",
        "details": [f"Best guess: {game['label']}", f"Folder: {root}", "Choose the root install directory rather than a subfolder."],
        "game": game["label"],
        "target": target,
    }


def write_target_config(config_path: Path, target: str, game_path: str, description: str, width: str, height: str, fullscreen: bool, skip_videos: bool) -> None:
    config_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        resolved_path = Path(game_path).resolve().as_posix()
    except OSError:
        resolved_path = game_path.replace("\\", "/")

    block = [f"[{target}]", f"path={resolved_path}"]
    if description:
        block.append(f"description={description}")
    if width:
        block.append(f"width={width}")
    if height:
        block.append(f"height={height}")
    block.append(f"fullscreen={'true' if fullscreen else 'false'}")
    block.append(f"skipvideos={'true' if skip_videos else 'false'}")
    block.append("")
    if not config_path.exists():
        config_path.write_text("\n".join(block) + "\n", encoding="utf-8")
        return
    lines = config_path.read_text(encoding="utf-8", errors="ignore").splitlines()
    start = None
    end = len(lines)
    for index, raw in enumerate(lines):
        stripped = raw.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            if stripped[1:-1] == target:
                start = index
            elif start is not None:
                end = index
                break
    merged = lines[:]
    if start is None:
        if merged and merged[-1].strip():
            merged.append("")
        merged.extend(block)
    else:
        merged = lines[:start]
        if merged and merged[-1].strip():
            merged.append("")
        merged.extend(block)
        merged.extend(lines[end:])
    config_path.write_text("\n".join(merged).rstrip() + "\n", encoding="utf-8")


def open_path(target: Path) -> None:
    try:
        if os.name == "nt":
            os.startfile(target)  # type: ignore[attr-defined]
        elif sys.platform == "darwin":
            subprocess.Popen(["open", str(target)])
        else:
            subprocess.Popen(["xdg-open", str(target)])
    except OSError as exc:
        raise OSError(f"Could not open path '{target}': {exc}") from exc


class XoreosLauncherApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.repo_root = Path(__file__).resolve().parent.parent
        self.config_path = get_config_path()
        self.entries: list[dict[str, str]] = []
        self.saved_targets: set[str] = set()
        self.selected_name = ""
        self.prefer_saved_target = False
        self.last_validation = {"state": "pending", "title": "Choose a game folder", "summary": "Select a detected install or browse to a root game folder.", "details": [], "game": "", "target": ""}

        self.path_var = tk.StringVar()
        self.target_var = tk.StringVar()
        self.description_var = tk.StringVar()
        self.width_var = tk.StringVar(value="1280")
        self.height_var = tk.StringVar(value="720")
        self.mode_var = tk.StringVar(value="windowed")
        self.skip_videos_var = tk.BooleanVar(value=False)
        self.remember_var = tk.BooleanVar(value=True)
        self.status_var = tk.StringVar()
        self.preview_var = tk.StringVar()
        self.exe_var = tk.StringVar()
        self.config_var = tk.StringVar()
        self.setup_var = tk.StringVar()
        self.validation_title_var = tk.StringVar()
        self.validation_summary_var = tk.StringVar()

        self.root.title("xoreos First-Run Launcher")
        self.root.geometry("1180x760")
        self.root.minsize(980, 620)

        self.build_ui()
        self.refresh_sources()

    def build_ui(self) -> None:
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(1, weight=1)

        hero = tk.Frame(self.root, bg="#16324f", padx=22, pady=20)
        hero.grid(row=0, column=0, sticky="ew")
        hero.columnconfigure(0, weight=1)
        tk.Label(hero, text="From first asset scan to game launch, in one place.", bg="#16324f", fg="white", font=("Segoe UI", 18, "bold") if os.name == "nt" else ("TkDefaultFont", 15, "bold")).grid(row=0, column=0, sticky="w")
        tk.Label(hero, text="Choose a detected install or browse to the root game folder, validate the assets, optionally save a target in xoreos.conf, then launch.", bg="#16324f", fg="#dce7f2", wraplength=1080, justify="left").grid(row=1, column=0, sticky="w", pady=(8, 0))

        body = ttk.Frame(self.root, padding=(18, 16, 18, 18))
        body.grid(row=1, column=0, sticky="nsew")
        body.columnconfigure(0, weight=5)
        body.columnconfigure(1, weight=4)
        body.rowconfigure(1, weight=1)

        env = ttk.LabelFrame(body, text="Environment", padding=12)
        env.grid(row=0, column=0, columnspan=2, sticky="ew")
        env.columnconfigure(1, weight=1)
        ttk.Label(env, text="Executable").grid(row=0, column=0, sticky="w", padx=(0, 10))
        ttk.Label(env, textvariable=self.exe_var).grid(row=0, column=1, sticky="w")
        ttk.Button(env, text="Open Build Folder", command=self.open_build_folder).grid(row=0, column=2, padx=(12, 0))
        ttk.Label(env, text="Config file").grid(row=1, column=0, sticky="w", padx=(0, 10), pady=(8, 0))
        ttk.Label(env, textvariable=self.config_var).grid(row=1, column=1, sticky="w", pady=(8, 0))
        ttk.Button(env, text="Open Config Folder", command=self.open_config_folder).grid(row=1, column=2, padx=(12, 0), pady=(8, 0))

        left = ttk.LabelFrame(body, text="1. Choose A Game Install", padding=12)
        left.grid(row=1, column=0, sticky="nsew", padx=(0, 10), pady=(14, 0))
        left.columnconfigure(0, weight=1)
        left.rowconfigure(1, weight=1)
        ttk.Label(left, text="Saved targets come from xoreos.conf. Detected installs come from common Steam and GOG folders. If nothing is listed, browse manually to the root game folder.", wraplength=560, justify="left").grid(row=0, column=0, sticky="w")
        self.tree = ttk.Treeview(left, columns=("name", "source", "path"), show="headings", height=18)
        self.tree.heading("name", text="Game")
        self.tree.heading("source", text="Source")
        self.tree.heading("path", text="Install path")
        self.tree.column("name", width=180, anchor="w")
        self.tree.column("source", width=140, anchor="w")
        self.tree.column("path", width=430, anchor="w")
        self.tree.grid(row=1, column=0, sticky="nsew", pady=(10, 0))
        self.tree.bind("<<TreeviewSelect>>", self.on_tree_select)
        scroll = ttk.Scrollbar(left, orient="vertical", command=self.tree.yview)
        scroll.grid(row=1, column=1, sticky="ns", pady=(10, 0))
        self.tree.configure(yscrollcommand=scroll.set)
        actions = ttk.Frame(left)
        actions.grid(row=2, column=0, columnspan=2, sticky="ew", pady=(10, 0))
        ttk.Button(actions, text="Refresh Scan", command=self.refresh_sources).grid(row=0, column=0, sticky="w")
        ttk.Button(actions, text="Browse For Folder", command=self.browse_for_path).grid(row=0, column=1, padx=(8, 0))
        ttk.Button(actions, text="Open Selected Folder", command=self.open_selected_folder).grid(row=0, column=2, padx=(8, 0))

        right = ttk.Frame(body)
        right.grid(row=1, column=1, sticky="nsew", pady=(14, 0))
        right.columnconfigure(0, weight=1)
        right.rowconfigure(2, weight=1)

        checklist = ttk.LabelFrame(right, text="2. First-Run Checklist", padding=12)
        checklist.grid(row=0, column=0, sticky="ew")
        ttk.Label(checklist, textvariable=self.setup_var, justify="left", wraplength=420).grid(row=0, column=0, sticky="w")

        target = ttk.LabelFrame(right, text="3. Asset Folder And Target", padding=12)
        target.grid(row=1, column=0, sticky="ew", pady=(12, 0))
        target.columnconfigure(1, weight=1)
        ttk.Label(target, text="Game folder").grid(row=0, column=0, sticky="w")
        ttk.Entry(target, textvariable=self.path_var).grid(row=0, column=1, sticky="ew")
        ttk.Button(target, text="Browse", command=self.browse_for_path).grid(row=0, column=2, padx=(8, 0))
        ttk.Label(target, text="Target name").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(target, textvariable=self.target_var).grid(row=1, column=1, columnspan=2, sticky="ew", pady=(8, 0))
        ttk.Label(target, text="Description").grid(row=2, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(target, textvariable=self.description_var).grid(row=2, column=1, columnspan=2, sticky="ew", pady=(8, 0))
        ttk.Checkbutton(target, text="Remember this game in xoreos.conf", variable=self.remember_var, command=self.update_preview).grid(row=3, column=0, columnspan=3, sticky="w", pady=(10, 0))
        row = ttk.Frame(target)
        row.grid(row=4, column=0, columnspan=3, sticky="ew", pady=(10, 0))
        ttk.Button(row, text="Validate Assets", command=self.validate_current_path).grid(row=0, column=0, sticky="w")
        ttk.Button(row, text="Save Target", command=self.save_target).grid(row=0, column=1, padx=(8, 0))

        launch = ttk.LabelFrame(right, text="4. Validation And Launch", padding=12)
        launch.grid(row=2, column=0, sticky="nsew", pady=(12, 0))
        launch.columnconfigure(0, weight=1)
        launch.rowconfigure(2, weight=1)
        ttk.Label(launch, textvariable=self.validation_title_var, font=("Segoe UI", 11, "bold") if os.name == "nt" else ("TkDefaultFont", 10, "bold")).grid(row=0, column=0, sticky="w")
        ttk.Label(launch, textvariable=self.validation_summary_var, wraplength=430, justify="left").grid(row=1, column=0, sticky="w", pady=(4, 10))
        self.validation_text = tk.Text(launch, height=10, wrap="word", relief="solid", borderwidth=1)
        self.validation_text.grid(row=2, column=0, sticky="nsew")
        options = ttk.Frame(launch)
        options.grid(row=3, column=0, sticky="ew", pady=(10, 0))
        ttk.Label(options, text="Video mode").grid(row=0, column=0, sticky="w")
        mode = ttk.Frame(options)
        mode.grid(row=0, column=1, sticky="w")
        ttk.Radiobutton(mode, text="Windowed", variable=self.mode_var, value="windowed", command=self.update_preview).grid(row=0, column=0, padx=(0, 10))
        ttk.Radiobutton(mode, text="Fullscreen", variable=self.mode_var, value="fullscreen", command=self.update_preview).grid(row=0, column=1)
        ttk.Label(options, text="Width").grid(row=1, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(options, textvariable=self.width_var, width=10).grid(row=1, column=1, sticky="w", pady=(8, 0))
        ttk.Label(options, text="Height").grid(row=2, column=0, sticky="w", pady=(8, 0))
        ttk.Entry(options, textvariable=self.height_var, width=10).grid(row=2, column=1, sticky="w", pady=(8, 0))
        ttk.Checkbutton(options, text="Skip intro videos", variable=self.skip_videos_var, command=self.update_preview).grid(row=3, column=0, columnspan=2, sticky="w", pady=(8, 0))
        ttk.Label(launch, text="Command preview").grid(row=4, column=0, sticky="w", pady=(10, 4))
        ttk.Label(launch, textvariable=self.preview_var, relief="solid", padding=10, anchor="w", justify="left", wraplength=430).grid(row=5, column=0, sticky="ew")
        buttons = ttk.Frame(launch)
        buttons.grid(row=6, column=0, sticky="ew", pady=(10, 0))
        ttk.Button(buttons, text="Launch Directly", command=self.launch_direct).grid(row=0, column=0, sticky="ew")
        ttk.Button(buttons, text="Save And Launch", command=self.save_and_launch).grid(row=0, column=1, padx=(8, 0), sticky="ew")

        footer = ttk.Frame(self.root, padding=(18, 0, 18, 18))
        footer.grid(row=2, column=0, sticky="ew")
        ttk.Label(footer, textvariable=self.status_var).grid(row=0, column=0, sticky="w")
        for variable in [self.path_var, self.target_var, self.description_var, self.width_var, self.height_var]:
            variable.trace_add("write", self.trace_refresh)

    # -- SOURCE HANDLING --
    def trace_refresh(self, *_args: object) -> None:
        self.update_preview()
        self.update_setup_summary()

    def refresh_sources(self) -> None:
        saved_entries = read_config_targets(self.config_path)
        detected_entries = detect_installs()
        saved_path_keys = {normalized_path_key(entry["path"]) for entry in saved_entries if entry["path"].strip()}
        filtered_detected = [entry for entry in detected_entries if normalized_path_key(entry["path"]) not in saved_path_keys]
        self.entries = saved_entries + filtered_detected
        self.saved_targets = {entry["launch_value"] for entry in self.entries if entry["launch_kind"] == "Target"}
        self.tree.delete(*self.tree.get_children())
        for index, entry in enumerate(self.entries):
            self.tree.insert("", "end", iid=str(index), values=(entry["name"], entry["source"], entry["path"]))
        exe_path = get_xoreos_executable(self.repo_root)
        self.exe_var.set(str(exe_path) if exe_path else "<not built yet>")
        self.config_var.set(str(self.config_path))
        if self.entries:
            self.tree.selection_set("0")
            self.tree.focus("0")
            self.on_tree_select()
        else:
            self.status_var.set("No saved targets or detected installs were found. Browse to a root game folder to get started.")
            self.validate_current_path(auto_only=True)
        self.update_setup_summary()
        self.update_preview()

    def on_tree_select(self, _event: object | None = None) -> None:
        if not self.tree.selection():
            return
        entry = self.entries[int(self.tree.selection()[0])]
        self.selected_name = entry["name"]
        self.path_var.set(entry["path"])
        self.description_var.set(entry["name"] if entry["source"] == "Detected install" else (entry["description"] or entry["name"]))
        if entry["launch_kind"] == "Target":
            self.target_var.set(entry["launch_value"])
            self.prefer_saved_target = True
        else:
            self.target_var.set("")
            self.prefer_saved_target = False
        self.status_var.set(f"Selected {entry['name']} from {entry['source']}.")
        self.validate_current_path(auto_only=True)

    def browse_for_path(self) -> None:
        selected = filedialog.askdirectory(title="Choose the root game folder")
        if not selected:
            return
        self.selected_name = Path(selected).name
        self.path_var.set(selected)
        self.target_var.set("")
        self.prefer_saved_target = False
        if not self.description_var.get().strip():
            self.description_var.set(Path(selected).name)
        self.status_var.set("Manual game folder selected. Validate it before launch.")
        self.validate_current_path(auto_only=True)

    def open_selected_folder(self) -> None:
        if not self.path_var.get().strip():
            messagebox.showinfo("No folder selected", "Choose a detected install or browse to a game folder first.")
            return
        target = Path(self.path_var.get().strip())
        if not target.exists():
            messagebox.showerror("Folder not found", f"Path does not exist: {target}")
            return
        try:
            open_path(target)
        except OSError as exc:
            messagebox.showerror("Cannot open folder", str(exc))

    def open_build_folder(self) -> None:
        exe = get_xoreos_executable(self.repo_root)
        if not exe:
            messagebox.showinfo("Build not found", "xoreos is not in the known build directories yet. Run a bootstrap script first.")
            return
        try:
            open_path(exe.parent)
        except OSError as exc:
            messagebox.showerror("Cannot open folder", str(exc))

    def open_config_folder(self) -> None:
        self.config_path.parent.mkdir(parents=True, exist_ok=True)
        try:
            open_path(self.config_path.parent)
        except OSError as exc:
            messagebox.showerror("Cannot open folder", str(exc))

    # -- VALIDATION --
    def validate_current_path(self, auto_only: bool = False) -> dict[str, object]:
        result = validate_game_path(self.path_var.get(), self.selected_name)
        self.last_validation = result
        if result["target"] and not self.target_var.get().strip():
            self.target_var.set(str(result["target"]))
        if result["game"] and not self.description_var.get().strip():
            folder = Path(self.path_var.get().strip()).name if self.path_var.get().strip() else str(result["game"])
            self.description_var.set(f"{result['game']} ({folder})")
        self.validation_title_var.set(str(result["title"]))
        self.validation_summary_var.set(str(result["summary"]))
        self.validation_text.configure(state="normal")
        self.validation_text.delete("1.0", "end")
        self.validation_text.insert("1.0", "\n".join(str(item) for item in result["details"]))
        self.validation_text.configure(state="disabled")
        self.update_setup_summary()
        self.update_preview()
        if not auto_only:
            self.status_var.set(str(result["summary"]))
        return result

    def update_setup_summary(self) -> None:
        build_ready = get_xoreos_executable(self.repo_root) is not None
        asset_selected = bool(self.path_var.get().strip())
        target_saved = self.target_var.get().strip() in self.saved_targets
        lines = [
            f"1. Engine build: {'ready' if build_ready else 'missing'}",
            f"2. Asset folder: {'selected' if asset_selected else 'needed'}",
            f"3. Asset validation: {self.last_validation['state']}",
            f"4. Saved target: {'ready' if target_saved else 'optional'}",
        ]
        if self.last_validation["state"] == "ready":
            lines.append("Next: use Save And Launch for a repeatable setup, or Launch Directly for a one-off test.")
        elif self.last_validation["state"] == "warning":
            lines.append("Next: try the parent install folder, then validate again.")
        else:
            lines.append("Next: point the launcher at the top-level game install directory and validate it.")
        self.setup_var.set("\n".join(lines))

    # -- SAVE/LAUNCH --
    def validate_dimension(self, value: str, label: str) -> None:
        if value and (not value.isdigit() or int(value) <= 0):
            raise ValueError(f"Please enter a valid {label}.")

    def quote_argument(self, argument: str) -> str:
        return f'"{argument}"' if argument and any(ch.isspace() for ch in argument) else argument

    def command_should_use_target(self) -> bool:
        return self.prefer_saved_target and self.target_var.get().strip() in self.saved_targets

    def build_command(self, use_saved_target: bool, require_exe: bool) -> list[str]:
        exe = get_xoreos_executable(self.repo_root)
        if require_exe and not exe:
            raise ValueError("xoreos was not found in the known build directories. Run a bootstrap script first.")
        command = [str(exe) if exe else "<xoreos>"]
        self.validate_dimension(self.width_var.get().strip(), "width")
        self.validate_dimension(self.height_var.get().strip(), "height")
        if use_saved_target:
            if self.target_var.get().strip() not in self.saved_targets:
                raise ValueError("Save the target first before launching by target name.")
            command.append(self.target_var.get().strip())
        elif self.path_var.get().strip():
            command.append(f"-p{self.path_var.get().strip()}")
        else:
            raise ValueError("Choose a saved target or set a game path.")
        if self.width_var.get().strip():
            command.append(f"--width={self.width_var.get().strip()}")
        if self.height_var.get().strip():
            command.append(f"--height={self.height_var.get().strip()}")
        command.append(f"--fullscreen={'true' if self.mode_var.get() == 'fullscreen' else 'false'}")
        if self.skip_videos_var.get():
            command.append("--skipvideos=true")
        return command

    def update_preview(self) -> None:
        try:
            command = self.build_command(use_saved_target=self.command_should_use_target(), require_exe=False)
            self.preview_var.set(" ".join(self.quote_argument(part) for part in command))
        except ValueError as exc:
            self.preview_var.set(str(exc))

    def save_target(self) -> bool:
        try:
            self.validate_dimension(self.width_var.get().strip(), "width")
            self.validate_dimension(self.height_var.get().strip(), "height")
            result = self.validate_current_path(auto_only=True)
            if result["state"] != "ready":
                raise ValueError("The selected folder is incomplete. Fix validation issues before saving this target.")
            if not self.remember_var.get():
                raise ValueError("Enable 'Remember this game in xoreos.conf' before saving a target.")
            target = sanitize_target_name(self.target_var.get().strip())
            if not target or target.lower() == "xoreos":
                raise ValueError("Enter a valid target name that is not 'xoreos'.")
            self.target_var.set(target)
            write_target_config(self.config_path, target, self.path_var.get().strip(), self.description_var.get().strip(), self.width_var.get().strip(), self.height_var.get().strip(), self.mode_var.get() == "fullscreen", self.skip_videos_var.get())
        except (ValueError, OSError) as exc:
            messagebox.showerror("Cannot save target", str(exc))
            return False
        saved = self.target_var.get().strip()
        self.refresh_sources()
        self.prefer_saved_target = True
        for index, entry in enumerate(self.entries):
            if entry["launch_kind"] == "Target" and entry["launch_value"] == saved:
                self.tree.selection_set(str(index))
                self.tree.focus(str(index))
                self.on_tree_select()
                break
        self.status_var.set(f"Saved target '{saved}' to {self.config_path}.")
        return True

    def launch_process(self, command: list[str], message: str) -> None:
        self.status_var.set("Launching xoreos...")
        env = os.environ.copy()
        runtime_dirs = get_runtime_dll_dirs(self.repo_root)
        if runtime_dirs:
            prepend = os.pathsep.join(str(path) for path in runtime_dirs)
            env["PATH"] = prepend + os.pathsep + env.get("PATH", "")
        try:
            subprocess.Popen(command, cwd=self.repo_root, env=env)
        except OSError as exc:
            messagebox.showerror("Launch failed", str(exc))
            self.status_var.set("Launch failed.")
            return
        self.status_var.set(message)

    def launch_direct(self) -> None:
        try:
            result = self.validate_current_path(auto_only=True)
            if result["state"] != "ready":
                raise ValueError("The selected folder is incomplete. Fix validation issues before launching.")
            self.prefer_saved_target = False
            command = self.build_command(use_saved_target=False, require_exe=True)
        except ValueError as exc:
            messagebox.showerror("Cannot launch xoreos", str(exc))
            return
        self.launch_process(command, "xoreos started with the selected game folder.")

    def save_and_launch(self) -> None:
        if self.remember_var.get():
            if not self.save_target():
                return
        try:
            command = self.build_command(use_saved_target=self.command_should_use_target(), require_exe=True)
        except ValueError as exc:
            messagebox.showerror("Cannot launch xoreos", str(exc))
            return
        self.launch_process(command, "xoreos started.")


def main() -> int:
    root = tk.Tk()
    try:
        style = ttk.Style(root)
        if os.name == "nt" and "vista" in style.theme_names():
            style.theme_use("vista")
    except tk.TclError:
        pass
    app = XoreosLauncherApp(root)
    app.update_preview()
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
