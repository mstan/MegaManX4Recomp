"""Small source-only checks for portable defaults and title address contracts."""
from pathlib import Path
import unittest
import tomllib

ROOT = Path(__file__).resolve().parents[1]


def config(path):
    with path.open("rb") as source:
        return tomllib.load(source)


class ReleaseConfigTest(unittest.TestCase):
    def test_original_disc_is_required_even_without_mods(self):
        expected = ["3ceab06ac99add4035f912188bcbf5b16c02056f46a33d38ff0c8dbce6cb613b"]
        expected_mount = ["9bec883757c28ac6642f4e9a7952b3ef39cc519b4ad14dde500e32f9ea13bf64"]
        for path in (ROOT / "game.toml", ROOT / "packaging/release/game.toml"):
            with self.subTest(path=path):
                self.assertEqual(config(path)["game"]["required_disc_sha256"], expected)
                self.assertEqual(config(path)["game"]["required_disc_fps"], expected_mount)

    def test_experimental_four_player_launcher_capacity(self):
        # Advertise controls for all seats; the selected session profile still
        # limits ordinary co-op to two and optional-P4 rosters to three.
        for path in (ROOT / "game.toml", ROOT / "packaging/release/game.toml"):
            with self.subTest(path=path):
                self.assertEqual(config(path)["game"]["players"], 4)

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

    def test_recompiler_hook_contract_matches_development(self):
        # Keep the shipped codegen metadata aligned with the trusted hooks
        # used to generate the resident game code. Order and hex case do not
        # change the address contract.
        dev = config(ROOT / "game.toml")["recompiler"]
        player = config(ROOT / "packaging/release/game.toml")["recompiler"]
        for key in ("mod_function_entry_funcs", "mod_instruction_sites"):
            with self.subTest(key=key):
                self.assertSetEqual(
                    {f"0x{int(address, 0):08X}" for address in player[key]},
                    {f"0x{int(address, 0):08X}" for address in dev[key]},
                    f"Release {key} differs from the development hook contract",
                )


if __name__ == "__main__":
    unittest.main()
