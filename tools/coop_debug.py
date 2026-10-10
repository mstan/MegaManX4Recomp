"""Private, running X4 development build control. No automatic process launch.

Boot navigation uses controller input only, never game state writes. Graphics
use the live aspect's GL capture; canonical crops omit native-wide HUD anchors.
"""
import argparse
import json
from pathlib import Path
import socket
import struct
import time


class Runtime:
    def __init__(self, port=14525):
        self.port = port

    def request(self, cmd, **fields):
        with socket.create_connection(('127.0.0.1', self.port), timeout=15) as conn:
            conn.sendall((json.dumps(dict(id=1, cmd=cmd, **fields)) + '\n').encode())
            result = json.loads(conn.makefile().readline())
        if not result.get('ok'):
            raise RuntimeError(result)
        return result

    def read(self, address, size):
        return bytes.fromhex(self.request('read_ram', addr=hex(address), len=size)['hex'])

    def write(self, address, value):
        return self.request('write_ram', addr=hex(address), val=hex(value))

    def input(self, buttons=0xffff, seat=0):
        return self.request('set_input', buttons=hex(buttons), port=seat+1)

    def release(self):
        self.request('clear_input')
        self.request('clear_input', port=2)

    def teleport(self, seat, x, y):
        """Offline developer build only: move at the native world boundary.

        Use coordinates from an authored checkpoint/observed safe platform.
        Teleporting into solid tiles or past room triggers can still be lethal.
        """
        if seat not in (0, 1) or not all(-32768 <= v < 32768 for v in (x, y)):
            raise ValueError('Seat must be 0/1 and coordinates signed world pixels')
        address = self.diagnostic()
        for offset, value in ((0x40, int(x * 65536)), (0x44, int(y * 65536))):
            for byte in range(4):
                self.write(address + offset + byte, (value >> (byte * 8)) & 255)
        self.fixture(4, seat)

    def teleport_both(self, x, y):
        """Move both living seats together at one native dispatcher boundary."""
        if not all(-32768 <= v < 32768 for v in (x, y)):
            raise ValueError('Coordinates must be signed world pixels')
        address = self.diagnostic()
        for offset, value in ((0x40, int(x * 65536)), (0x44, int(y * 65536))):
            for byte in range(4):
                self.write(address + offset + byte, (value >> (byte * 8)) & 255)
        self.fixture(4, 2)

    def screenshot(self, path):
        # A canonical VRAM crop omits anchored HUD at native-wide aspects.
        state = self.request('gpu_state')
        command = 'screenshot_wide_hires' if state.get('ws', {}).get('nw_extra', 0) else 'screenshot_hires'
        return self.request(command, path=str(Path(path).resolve()))

    def diagnostic(self):
        counters = self.request('mod_counters')['counters']
        for c in counters:
            if c['name'] == 'mmx4.coop.diagnostic-address':
                return c['count']
        raise RuntimeError('Co-op plugin has not activated')

    def fixture(self, command, first=0, second=0, third=0):
        address = self.diagnostic()
        sequence = struct.unpack('<I', self.read(address+0x30, 4))[0]+1
        for offset, value in enumerate((command, first, second, third)):
            self.write(address+0x34+offset, value)
        # Development runs use fewer than 256 requests; one byte commits all
        # arguments at the native gameplay dispatcher, between world updates.
        if sequence > 255:
            raise RuntimeError('Restart the private QA process after 255 fixtures')
        self.write(address+0x30, sequence)
        deadline = time.monotonic()+10
        while time.monotonic() < deadline:
            if struct.unpack('<I', self.read(address+0x38, 4))[0] == sequence:
                result=struct.unpack('<I',self.read(address+0x54,4))[0]
                if result==2:
                    raise RuntimeError(f'Native fixture rejected command {command}: {first}, {second}, {third}')
                return
            time.sleep(.05)
        raise TimeoutError('Fixture was not acknowledged in native gameplay')

    def players(self):
        # One completed projection boundary; a raw PLAYER read may catch P2
        # temporarily occupying the game's native singleton during a callback.
        snapshot = self.read(self.diagnostic(), 0x3e4)
        if struct.unpack_from('<I', snapshot, 0x1c)[0] == 1:
            return snapshot[0x300:0x3e4], snapshot[0x100:0x1e4]
        return self.read(0x801418c8, 0xe4), snapshot[0x100:0x1e4]

    def wait_playable(self, timeout=45, advance_dialogue=False):
        deadline = time.monotonic()+timeout
        address = self.diagnostic()
        while time.monotonic() < deadline:
            mode = self.read(0x801721c0, 0x20)
            diagnostic = struct.unpack('<5I', self.read(address, 20))
            if diagnostic[3]:
                raise RuntimeError('Co-op plugin reported a failure')
            players = self.players()
            if mode[0] == 6 and not mode[0x1c] and diagnostic[2] and all(
                    p[0] and p[4] == 1 and p[5] >= 2 for p in players):
                if advance_dialogue:
                    self.release()
                return players
            if advance_dialogue and mode[0] == 6 and mode[0x1c]:
                # Native boss conversations require the campaign owner's
                # confirm button before the game releases player control.
                self.input(0xbfff)
                time.sleep(.1)
                self.release()
            time.sleep(.1)
        raise TimeoutError(f'Native stage did not become playable: {mode.hex()}')

    def boot(self, timeout=90, campaign=0):
        deadline = time.monotonic() + timeout
        previous = None
        iteration = 0
        try:
            while time.monotonic() < deadline:
                state = self.read(0x801721c0, 4)
                if state != previous:
                    print('native mode:', state.hex(), flush=True)
                    previous = state
                if state[0] == 6:
                    self.release()
                    players = self.wait_playable()
                    if [p[2] for p in players] != [campaign, campaign ^ 1]:
                        raise AssertionError('Native character selection chose the wrong campaign')
                    return
                # Original 800297D8 selects Zero with Right before processing
                # confirmation. Keeping it held also covers short select windows.
                buttons = 0xfff7 if iteration % 2 == 0 else 0xbfff
                self.input(buttons & (0xffdf if campaign else 0xffff))
                time.sleep(.6)
                self.release()
                time.sleep(.3)
                iteration += 1
            raise TimeoutError('Input-only navigation did not reach gameplay')
        finally:
            self.release()

    def boot_continue(self,campaign,clears=16,timeout=100):
        """Load data1 from a cloned native card, stopping at stage selection."""
        deadline=time.monotonic()+timeout
        def pulse(buttons):
            self.input(buttons);time.sleep(.15);self.release();time.sleep(.15)
        def wait(label,predicate,intro=False):
            previous=None
            while time.monotonic()<deadline:
                front=self.read(0x80173c70,0x20);play=self.read(0x801721c0,0x64)
                key=list(front[:3])+list(play[:2])
                if key!=previous:print(label,key,flush=True);previous=key
                if predicate(front,play) and not self.read(0x80141bdc,1)[0]:return play
                if intro and front[0]!=6:pulse(0xfff7)
                elif label=='loaded stage selection' and play[:2]==bytes([3,9]):pulse(0xbfff)
                else:time.sleep(.06)
            raise TimeoutError(label)
        try:
            wait('main menu',lambda f,p:f[:2]==bytes([6,1]),True)
            pulse(0xffbf)
            assert self.read(0x80173c72,1)[0]==1,'Continue not selected'
            pulse(0xbfff)
            wait('Continue card choice',lambda f,p:f[:2]==bytes([7,3]))
            pulse(0xbfff)
            wait('card slot choice',lambda f,p:p[:2]==bytes([0,1]))
            pulse(0xbfff)
            wait('saved data list',lambda f,p:p[:2]==bytes([0,3]))
            time.sleep(1.5)
            pulse(0xbfff)
            wait('saved data confirmation',lambda f,p:p[:2]==bytes([0,4]))
            pulse(0xbfff)
            play=wait('loaded stage selection',lambda f,p:p[:2]==bytes([3,4]))
            assert play[0x43]==campaign and play[0x59]==clears,(campaign,clears,list(play[0x43:0x60]))
            return play
        finally:self.release()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=14525)
    parser.add_argument('--boot', action='store_true')
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0)
    parser.add_argument('--screenshot', type=Path)
    args = parser.parse_args()
    runtime = Runtime(args.port)
    if args.boot:
        runtime.boot(campaign=args.campaign)
    if args.screenshot:
        print(runtime.screenshot(args.screenshot))
    print(runtime.request('mod_counters'))
    address = runtime.diagnostic()
    print('diagnostic', hex(address), struct.unpack('<5I', runtime.read(address, 20)))


if __name__ == '__main__':
    main()
