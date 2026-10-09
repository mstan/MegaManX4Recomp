"""Small source-only checks for portable defaults and title address contracts."""
from pathlib import Path
import unittest
import tomllib

ROOT = Path(__file__).resolve().parents[1]


def config(path):
    with path.open("rb") as source:
        return tomllib.load(source)


class ReleaseConfigTest(unittest.TestCase):
    def test_two_player_launcher_contract(self):
        # This selects both the visible Netplay entry point and the P2
        # Controls row; compiling netplay support alone does not expose them.
        for path in (ROOT / "game.toml", ROOT / "packaging/release/game.toml"):
            with self.subTest(path=path):
                self.assertEqual(config(path)["game"]["players"], 2)

    def test_coop_camera_choices(self):
        package = config(ROOT / "mods/preloaded/packages/mmx4.coop/0.0.1/manifest.toml")
        cameras = next(option for option in package["option"] if option["id"] == "cameras")
        self.assertEqual(cameras["feature"], "coop")
        self.assertEqual(cameras["default"], "unified")
        self.assertEqual([(choice["value"], choice["label"]) for choice in cameras["choice"]],
                         [("unified", "Unified"), ("split", "Split (Experimental)")])

    def test_display_defaults(self):
        for path in (ROOT / "game.toml", ROOT / "packaging/release/game.toml"):
            with self.subTest(path=path):
                video = config(path)["video"]
                self.assertEqual(video["renderer"], "opengl")
                self.assertEqual(video["internal_resolution"], "1080p")
                self.assertEqual(video["resolution_reference_lines"], 240)
                self.assertNotIn("supersampling", video)

    def test_coop_hud_choices(self):
        package = config(ROOT / "mods/preloaded/packages/mmx4.coop/0.0.1/manifest.toml")
        layout = next(option for option in package["option"] if option["id"] == "hud_layout")
        self.assertEqual(layout["feature"], "coop")
        self.assertEqual(layout["default"], "stacked")
        self.assertEqual([(choice["value"], choice["label"]) for choice in layout["choice"]],
                         [("stacked", "Stacked"), ("side_by_side", "Side by side")])

    def test_portable_player_contract_matches_development(self):
        dev = config(ROOT / "game.toml")
        player = config(ROOT / "packaging/release/game.toml")
        for table in ("widescreen", "controller", "audio"):
            with self.subTest(table=table):
                self.assertEqual(player[table], dev[table])
        for key in ("id", "load_address", "entry_pc", "text_size", "stack_base"):
            self.assertEqual(player["game"][key], dev["game"][key])
        self.assertNotIn("overlay_autocompile_cmd", player["runtime"])
        self.assertNotIn("audit", player)


if __name__ == "__main__":
    unittest.main()
