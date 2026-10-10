"""Send bounded cheats through synchronized inputs in an X4 development match.

Requires MMX4_COOP_NETPLAY_DEBUG=ON on both peers. It never launches a process
or writes guest RAM. Keep this helper separate from ordinary playtest evidence.
"""
import argparse
import struct
import time
from coop_debug import Runtime


def packet(sequence, command, first=0, second=0, third=0, x=0, y=0):
    if not (1 <= sequence <= 31 and 1 <= command <= 6 and
            0 <= first <= 255 and 0 <= second <= 255 and 0 <= third <= 7 and
            -32768 <= x < 32768 and -32768 <= y < 32768):
        raise ValueError('Invalid development command arguments')
    x &= 65535
    y &= 65535
    words = [0x5A0 | sequence, command | (first << 3), second | (third << 8),
             x & 2047, (x >> 11) | ((y & 63) << 5), y >> 6]
    checksum = 2166136261
    for word in words:
        for byte in (word & 255, word >> 8):
            checksum = ((checksum ^ byte) * 16777619) & 0xFFFFFFFF
    words.append((checksum ^ (checksum >> 11) ^ (checksum >> 22)) & 2047)
    return [(index << 11) | word for index, word in enumerate(words)]


class NetplayDevelopment:
    def __init__(self, host_port, guest_port):
        self.peers = [Runtime(host_port), Runtime(guest_port)]
        self.addresses = [peer.diagnostic() for peer in self.peers]
        for peer, address in zip(self.peers, self.addresses):
            status = peer.request('netplay_status')
            if not status['active'] or status['rollback'] or status['players'] != 2:
                raise RuntimeError('Requires a connected two-player delay-sync match')
            if struct.unpack('<I', peer.read(address + 0x50, 4))[0] != 0x44455631:
                raise RuntimeError('This build does not enable synchronized development cheats')
        self.sequence = struct.unpack('<I', self.peers[0].read(self.addresses[0] + 0x4C, 4))[0]

    def _wait(self, offset, value, timeout=10):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if all(struct.unpack('<I', peer.read(address + offset, 4))[0] == value
                   for peer, address in zip(self.peers, self.addresses)):
                return
            time.sleep(.02)
        raise TimeoutError(f'Both peers did not acknowledge development word {value:#x}')

    def fixture(self, command, first=0, second=0, third=0, *, x=0, y=0):
        self.sequence = self.sequence % 31 + 1
        try:
            self.peers[1].request('set_input', layer='host', buttons='0xffff')
            for word in packet(self.sequence, command, first, second, third, x, y):
                held = (word & 1) | ((word & 0x3FFE) << 2) | 6
                self.peers[0].request('set_input', layer='host', buttons=hex((~held) & 65535))
                self._wait(0x48, word)
            self._wait(0x4C, self.sequence)
            self._wait(0x38, self.sequence)
            results = [struct.unpack('<I', peer.read(address + 0x54, 4))[0]
                       for peer, address in zip(self.peers, self.addresses)]
            if results != [1, 1]:
                raise RuntimeError(f'Development command rejected at native boundary: {results}')
        finally:
            self.peers[0].request('set_input', layer='host', buttons='0xffff')

    def teleport(self, seat, x, y):
        self.fixture(4, seat, x=x, y=y)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ports', type=int, nargs=2, required=True, metavar=('HOST', 'GUEST'))
    parser.add_argument('command', choices=('stage', 'checkpoint', 'health', 'invulnerable', 'teleport', 'enemy-health'))
    parser.add_argument('values', type=int, nargs='+')
    args = parser.parse_args()
    runtime = NetplayDevelopment(*args.ports)
    if args.command == 'teleport':
        runtime.teleport(*args.values)
    else:
        runtime.fixture({'stage': 1, 'health': 2, 'checkpoint': 3,
                         'invulnerable': 5, 'enemy-health': 6}[args.command], *args.values)
    print('Both peers acknowledged synchronized development command', runtime.sequence)


if __name__ == '__main__':
    main()
