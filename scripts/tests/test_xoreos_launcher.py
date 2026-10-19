"""Tests for scripts/xoreos_launcher_gui.py. Run: python3 -m unittest discover -s scripts/tests"""

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
from unittest import mock

SCRIPT = Path(__file__).resolve().parent.parent / "xoreos_launcher_gui.py"
spec = importlib.util.spec_from_file_location("xoreos_launcher_gui", SCRIPT)
launcher = importlib.util.module_from_spec(spec)
sys.modules["xoreos_launcher_gui"] = launcher
spec.loader.exec_module(launcher)


def make_tree(root: Path, entries: list[str]) -> Path:
    """Create files, and directories for entries ending in '/'."""
    for entry in entries:
        path = root / entry
        if entry.endswith("/"):
            path.mkdir(parents=True, exist_ok=True)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"")
    return root


KOTOR = ["swkotor.exe", "chitin.key", "dialog.tlk", "data/", "lips/", "modules/", "rims/", "texturepacks/swpc_tex_gui.erf", "override/"]
KOTOR2 = ["swkotor2.exe", "chitin.key", "dialog.tlk", "data/", "lips/", "modules/", "texturepacks/", "streamvoice/"]


class TempDirTest(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._temp.name)

    def tearDown(self) -> None:
        self._temp.cleanup()


class CheckGameFolderTest(TempDirTest):
    def test_complete_kotor_is_ready(self) -> None:
        folder = make_tree(self.tmp / "swkotor", KOTOR)
        check = launcher.check_game_folder(str(folder))
        self.assertEqual(check.state, "ready")
        self.assertEqual(check.game, "kotor")

    def test_texture_pack_in_subfolder_is_enough(self) -> None:
        # Regression: the old launcher wanted swpc_tex_gui.erf in the top folder
        folder = make_tree(self.tmp / "game", ["swkotor.exe", "chitin.key", "dialog.tlk", "data/", "lips/", "modules/", "rims/", "texturepacks/"])
        self.assertEqual(launcher.check_game_folder(str(folder)).state, "ready")

    def test_names_match_case_insensitively(self) -> None:
        folder = make_tree(self.tmp / "game", ["SWKotOR.EXE", "Chitin.key", "DIALOG.TLK", "Data/", "LIPS/", "Modules/", "Rims/", "TexturePacks/"])
        self.assertEqual(launcher.check_game_folder(str(folder)).state, "ready")

    def test_kotor2_folder_named_swkotor2_is_kotor2(self) -> None:
        # Regression: "swkotor" is a substring of "swkotor2"
        folder = make_tree(self.tmp / "swkotor2", KOTOR2)
        check = launcher.check_game_folder(str(folder))
        self.assertEqual(check.game, "kotor2")
        self.assertEqual(check.state, "ready")
        self.assertTrue(any("not playable" in line for line in check.details))

    def test_missing_data_is_a_warning(self) -> None:
        folder = make_tree(self.tmp / "game", ["swkotor.exe", "chitin.key"])
        check = launcher.check_game_folder(str(folder))
        self.assertEqual(check.state, "warning")
        self.assertTrue(check.launchable)
        self.assertTrue(any("modules/" in line and "data/" in line for line in check.details))

    def test_data_without_program_explains_why(self) -> None:
        folder = make_tree(self.tmp / "game", ["chitin.key", "dialog.tlk", "modules/"])
        check = launcher.check_game_folder(str(folder))
        self.assertEqual(check.state, "error")
        self.assertTrue(any("swkotor.exe" in line for line in check.details))

    def test_subfolder_suggests_game_folder(self) -> None:
        folder = make_tree(self.tmp / "swkotor", KOTOR)
        check = launcher.check_game_folder(str(folder / "modules"))
        self.assertEqual(check.state, "error")
        self.assertEqual(Path(check.suggestion), folder)

    def test_library_folder_suggests_the_game_inside(self) -> None:
        folder = make_tree(self.tmp / "common" / "swkotor", KOTOR)
        make_tree(self.tmp / "common" / "Other Game", ["game.exe"])
        check = launcher.check_game_folder(str(self.tmp / "common"))
        self.assertEqual(Path(check.suggestion), folder)

    def test_choosing_the_program_file_suggests_its_folder(self) -> None:
        folder = make_tree(self.tmp / "swkotor", KOTOR)
        check = launcher.check_game_folder(str(folder / "swkotor.exe"))
        self.assertEqual(Path(check.suggestion), folder)

    def test_missing_and_empty(self) -> None:
        self.assertEqual(launcher.check_game_folder("").state, "empty")
        self.assertEqual(launcher.check_game_folder(str(self.tmp / "nope")).title, "Folder not found")
        self.assertEqual(launcher.check_game_folder(str(self.tmp)).state, "error")

    def test_mac_app_bundle(self) -> None:
        folder = make_tree(self.tmp / "mac", ["Knights of the Old Republic.app/"])
        self.assertEqual(launcher.check_game_folder(str(folder)).game, "kotor")


class ConfigFileTest(TempDirTest):
    def setUp(self) -> None:
        super().setUp()
        self.conf = self.tmp / "xoreos.conf"

    def test_save_new_target(self) -> None:
        launcher.save_target(self.conf, "kotor", {"path": "/games/swkotor", "fullscreen": "false"})
        targets = launcher.read_targets(self.conf)
        self.assertEqual([t.name for t in targets], ["kotor"])
        self.assertEqual(targets[0].path, "/games/swkotor")

    def test_update_keeps_other_keys_comments_and_sections(self) -> None:
        self.conf.write_text(
            "[xoreos]\nwidth=800\n\n# My game\n[KotOR]\npath=/old\nvolume_music=0.3\nlang=en # comment\n\n[other]\npath=/x\n",
            encoding="utf-8")
        launcher.save_target(self.conf, "kotor", {"path": "/new", "width": "1920"})
        text = self.conf.read_text(encoding="utf-8")
        self.assertIn("volume_music=0.3", text)
        self.assertIn("# My game", text)
        self.assertIn("[other]\npath=/x", text)
        self.assertEqual(text.count("[KotOR]"), 1)
        self.assertNotIn("[kotor]", text)
        targets = {t.name: t for t in launcher.read_targets(self.conf)}
        self.assertEqual(targets["KotOR"].values["path"], "/new")
        self.assertEqual(targets["KotOR"].values["width"], "1920")
        self.assertEqual(targets["KotOR"].values["lang"], "en")
        self.assertTrue((self.tmp / "xoreos.conf.bak").exists())

    def test_xoreos_section_is_not_a_target(self) -> None:
        self.conf.write_text("[xoreos]\npath=/x\n", encoding="utf-8")
        self.assertEqual(launcher.read_targets(self.conf), [])

    def test_refuses_values_the_format_cannot_store(self) -> None:
        for bad in ["/games/#1/swkotor", "/a\nb"]:
            with self.assertRaises(ValueError):
                launcher.save_target(self.conf, "kotor", {"path": bad})
        self.assertFalse(self.conf.exists())

    def test_brackets_and_equals_in_values_round_trip(self) -> None:
        path = "/games/Star Wars [GOG]/a=b"
        launcher.save_target(self.conf, "kotor", {"path": path})
        self.assertEqual(launcher.read_targets(self.conf)[0].path, path)

    def test_refuses_bad_names(self) -> None:
        for bad in ["xoreos", "", "a/b", "ko[tor]"]:
            with self.assertRaises(ValueError):
                launcher.save_target(self.conf, bad, {"path": "/x"})

    def test_remove_target(self) -> None:
        self.conf.write_text("[xoreos]\nwidth=800\n\n[kotor]\npath=/a\n\n[kotor2]\npath=/b\n", encoding="utf-8")
        self.assertTrue(launcher.remove_target(self.conf, "kotor"))
        self.assertEqual([t.name for t in launcher.read_targets(self.conf)], ["kotor2"])
        self.assertIn("width=800", self.conf.read_text(encoding="utf-8"))
        self.assertFalse(launcher.remove_target(self.conf, "missing"))

    def test_suggest_target_name(self) -> None:
        targets = [launcher.Target("kotor", {"path": str(self.tmp / "a")})]
        self.assertEqual(launcher.suggest_target_name("kotor", str(self.tmp / "a"), targets), "kotor")
        self.assertEqual(launcher.suggest_target_name("kotor", str(self.tmp / "b"), targets), "kotor_2")
        self.assertEqual(launcher.suggest_target_name("kotor2", str(self.tmp / "b"), targets), "kotor2")


class FindingTest(TempDirTest):
    def test_steam_library_paths(self) -> None:
        vdf = '"libraryfolders"\n{\n\t"0"\n\t{\n\t\t"path"\t\t"D:\\\\SteamLibrary"\n\t}\n\t"1" { "path" "/mnt/games" }\n}'
        self.assertEqual(launcher.steam_library_paths(vdf), [Path("D:\\SteamLibrary"), Path("/mnt/games")])

    @unittest.skipIf(os.name == "nt", "Linux/macOS Steam layout")
    def test_find_installs_from_steam_library(self) -> None:
        home = self.tmp / "home"
        steam = home / ".local" / "share" / "Steam"
        library = self.tmp / "library"
        make_tree(steam / "steamapps", ["libraryfolders.vdf"])
        (steam / "steamapps" / "libraryfolders.vdf").write_text(f'"libraryfolders" {{ "1" {{ "path" "{library}" }} }}', encoding="utf-8")
        make_tree(library / "steamapps" / "common" / "swkotor", KOTOR)
        make_tree(library / "steamapps" / "common" / "Portal", ["portal.exe"])
        saved = make_tree(self.tmp / "saved", KOTOR2)
        targets = [launcher.Target("kotor2", {"path": str(saved)})]
        with mock.patch.object(Path, "home", return_value=home), mock.patch.object(sys, "platform", "linux"):
            installs = launcher.find_installs(targets)
        self.assertEqual([(i.source, Path(i.path).name) for i in installs], [("Saved", "saved"), ("Steam", "swkotor")])

    def test_find_engines_prefers_newest(self) -> None:
        name = "xoreos.exe" if os.name == "nt" else "xoreos"
        old = make_tree(self.tmp, [f"build-old/bin/{name}"]) / "build-old" / "bin" / name
        new = make_tree(self.tmp, [f"builds/preset/bin/Release/{name}"]) / "builds" / "preset" / "bin" / "Release" / name
        make_tree(self.tmp, ["build-empty/bin/"])
        for path in [old, new]:
            path.chmod(0o755)
        os.utime(old, (time.time() - 100, time.time() - 100))
        self.assertEqual(launcher.find_engines(self.tmp), [new, old])


class CommandTest(unittest.TestCase):
    def test_saved_target_command(self) -> None:
        self.assertEqual(launcher.build_command("xoreos", target="kotor"), ["xoreos", "kotor", "--errordialog=false"])

    def test_one_off_command(self) -> None:
        options = launcher.LaunchOptions(1920, 1080, True, True)
        command = launcher.build_command("xoreos", path="/g/sw kotor", options=options, remember=False)
        self.assertEqual(command, ["xoreos", "--path=/g/sw kotor", "--errordialog=false", "--width=1920", "--height=1080",
                                   "--fullscreen=true", "--skipvideos=true", "--saveconf=false"])
        self.assertEqual(launcher.quote_command(command[:2]), 'xoreos "--path=/g/sw kotor"')

    def test_needs_a_game(self) -> None:
        with self.assertRaises(ValueError):
            launcher.build_command("xoreos")

    def test_parse_resolution(self) -> None:
        self.assertEqual(launcher.parse_resolution(" 1920 x 1080 "), (1920, 1080))
        for bad in ["big", "100x100", "1920"]:
            with self.assertRaises(ValueError):
                launcher.parse_resolution(bad)

    def test_error_excerpt_starts_at_first_error(self) -> None:
        output = "Status: a\nStatus: b\nERROR: Unable to detect the game\nERROR: more\n"
        self.assertEqual(launcher.error_excerpt(output), "ERROR: Unable to detect the game\nERROR: more")
        self.assertEqual(launcher.error_excerpt("only status\n"), "only status")


if __name__ == "__main__":
    unittest.main()
