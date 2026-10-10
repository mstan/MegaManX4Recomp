import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class CoopHookRegistrationTest(unittest.TestCase):
    def test_guest_dispatch_address_is_in_runtime_function_aperture(self):
        source = (ROOT / 'src/mods/mmx4_coop_combat.c').read_text(encoding='utf-8')
        address = int(re.search(r'#define ACTOR_DISPATCH (0x[0-9A-Fa-f]+)u', source)[1], 16)
        self.assertGreaterEqual(address & 0x1FFFFFFF, 0x0F000000)
        self.assertLess(address & 0x1FFFFFFF, 0x10000000)
        self.assertEqual(address & 3, 0)

    def test_function_addresses_have_one_handler_per_plugin(self):
        registrations = {}
        pattern = re.compile(
            r'psx_mod_register_function_(?:entry|filter)_plugin\s*\('
            r'\s*(?:ID|COOP_ID)\s*,\s*(0x[0-9a-fA-F]+|[A-Z][A-Z_0-9]*)[uU]?'
            r'\s*,\s*(\w+)\s*\)')
        for source in (ROOT / 'src/mods').glob('mmx4_coop_*.c'):
            content = source.read_text(encoding='utf-8')
            constants = dict(re.findall(
                r'^#define\s+(\w+)\s+(0x[0-9a-fA-F]+)[uU]?\s*$', content, re.M))
            for match in pattern.finditer(content):
                value = constants.get(match.group(1), match.group(1))
                address = int(value, 16) & 0x1FFFFFFF
                handler = f'{source.name}:{match.group(2)}'
                self.assertNotIn(address, registrations,
                                 f'Duplicate mod hook at {address:#x}: '
                                 f'{registrations.get(address)} and {handler}')
                registrations[address] = handler
        self.assertGreater(len(registrations), 20)
        self.assertTrue(registrations[0x27850].endswith(':camera_update'))
        self.assertTrue(registrations[0x21340].endswith(':second_tick'))
        self.assertTrue(registrations[0x35240].endswith(':reset_player'))
        self.assertTrue(registrations[0xEA80C].endswith(':present_local_view'))


if __name__ == '__main__':
    unittest.main()
