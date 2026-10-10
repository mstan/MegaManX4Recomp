import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from prepare_mmx4_playtest_save import card_image


class PlaytestSaveTest(unittest.TestCase):
    def setUp(self):
        base = 0x80010000
        self.binary = bytearray(0x800F2160 - base + 0x800)
        struct.pack_into('<I', self.binary, 0x18, base)
        signature_offset = 0x8001005C - base + 0x800
        self.binary[signature_offset:signature_offset + 12] = b'BASLUS-00561'
        self.button_masks = tuple(1 << (15 - index) for index in range(16))
        controls_offset = 0x800EE430 - base + 0x800
        struct.pack_into('<16H', self.binary, controls_offset, *self.button_masks)

    def test_native_save_load_retains_movement_and_attack_bindings(self):
        for campaign in (0, 1):
            with self.subTest(campaign=campaign):
                card = card_image(self.binary, campaign)
                record = card[8192 + 512:8192 + 554]
                loaded_masks = struct.unpack_from('<16H', record, 8)
                for button_index in (2, 6, 10, 14):
                    pressed = self.button_masks[button_index]
                    translated = sum(1 << index for index, mask in enumerate(loaded_masks)
                                     if pressed & mask)
                    self.assertEqual(translated, 1 << button_index)
                self.assertEqual(record[:8], bytes((campaign, 32, 0, 0, 255, 5, 0, 0)))
                self.assertEqual(record[40:], bytes(2))
                self.assertEqual(len(card), 131072)

    def test_generator_refuses_to_replace_an_existing_card(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            executable = directory / 'original.exe'
            executable.write_bytes(self.binary)
            cards = directory / 'cards'
            cards.mkdir()
            existing = cards / 'card2.mcd'
            existing.write_bytes(b'existing player save')
            result = subprocess.run(
                [sys.executable, str(ROOT / 'tools/prepare_mmx4_playtest_save.py'),
                 '--exe', str(executable), '--output', str(cards)],
                capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('cards already exist', result.stderr)
            self.assertEqual(existing.read_bytes(), b'existing player save')
            self.assertFalse((cards / 'card1.mcd').exists())


if __name__ == '__main__':
    unittest.main()
