"""Native intro gate, followed by an explicit frozen-scene controller fixture."""
import json
import struct
import time
from coop_debug import capture_pair


def four_cutscene(runtime, diagnostic, count, output):
    events = []

    def sample(label):
        data = runtime.read(diagnostic, 0xd00)
        header = struct.unpack_from('<6I', data)
        assert header[1] == count and not header[3], header
        players = []
        for seat in range(count):
            body = data[0x100+seat*256:0x1e4+seat*256]
            players.append(dict(hp=body[0x5c]&127, state=body[4], pose=body[5],
                x=struct.unpack_from('<i', body, 8)[0]/65536,
                y=struct.unpack_from('<i', body, 12)[0]/65536,
                script=body[0xc0], door=body[0xc4]))
        row = dict(label=label, header=header, players=players,
                   freeze=struct.unpack_from('<I', data, 0x20)[0],
                   play=list(runtime.read(0x801721c0, 0x20)))
        events.append(row)
        return row

    def word(address, value):
        for byte in range(4):
            runtime.write(address+byte, (value>>(byte*8))&255)

    def fixture(command, seat, value):
        sequence = struct.unpack('<I', runtime.read(diagnostic+0x30, 4))[0]+1
        for offset, data in ((0x34,command), (0x38,seat), (0x3c,value), (0x40,0)):
            word(diagnostic+offset, data)
        word(diagnostic+0x30, sequence)
        deadline = time.monotonic()+8
        while struct.unpack('<I', runtime.read(diagnostic+0x44, 4))[0] != sequence:
            assert time.monotonic()<deadline, 'Offline setup did not commit'
            time.sleep(.03)
        assert struct.unpack('<I', runtime.read(diagnostic+0x48, 4))[0] == 1

    saved_freeze = None
    try:
        for seat in range(count):
            runtime.input(0xffff, seat)
        # Remove the movement-ownership probe's protection before loading.
        # HP127 below cushions ordinary damage; native crush/death stays real.
        fixture(3, 0, 0)
        runtime.write(0x801721dd, 2)
        runtime.write(0x801721de, 0)
        runtime.write(0x801721c0, 5)
        deadline = time.monotonic()+45
        while True:
            row = sample('checkpoint-arrival')
            if row['play'][:2] == [6,0] and row['header'][2] and not row['play'][0x1c] and \
                    row['players'][0]['x']>3000 and all(p['state']==1 and p['pose']>=2 for p in row['players']):
                break
            assert time.monotonic()<deadline, row
            time.sleep(.05)
        for seat in range(count):
            fixture(2, seat, 127)
            runtime.input(0x7fdf, seat)
        runtime.request('turbo', enabled=1)
        deadline = time.monotonic()+120
        crossed = False
        while True:
            row = sample('native-door-approach')
            assert all(p['hp'] for p in row['players']), row
            if row['players'][0]['x']>=4600:
                runtime.request('turbo', enabled=0)
            crossed |= any(p['door'] for p in row['players'])
            if crossed and row['players'][0]['script'] and not any(p['door'] for p in row['players']):
                break
            assert time.monotonic()<deadline, row
            time.sleep(.05)
        capture_pair(runtime, output/'cutscene-after-native-door.png')
        before = sample('before-explicit-freeze')
        # This is a deliberate frozen-world setup, not a scene-success flag.
        # The native scene remains active; only non-owner movement is tested.
        saved_freeze = runtime.read(0x801721d0, 1)[0]
        runtime.write(0x801721d0, 1)
        deadline = time.monotonic()+15
        while True:
            after = sample('frozen-scene-movement')
            assert all(p['hp'] for p in after['players']), after
            if after['header'][4]>before['header'][4]+1:
                assert after['freeze'] == 1 and after['players'][0]['script'], after
            if after['freeze'] == 1 and all(after['players'][seat]['x']>before['players'][seat]['x']+20 for seat in range(1,count)):
                break
            assert time.monotonic()<deadline, after
            time.sleep(.05)
        assert abs(after['players'][0]['x']-before['players'][0]['x'])<2, after
        capture_pair(runtime, output/'cutscene-frozen-movement.png')
        runtime.write(0x801721d0, saved_freeze)
        saved_freeze = None
        for seat in range(count):
            runtime.input(0xffff, seat)
        # Let the actual story controller release the gate; no completion
        # byte is written. Confirm belongs to its native P1 scene owner.
        deadline = time.monotonic()+180
        first_release_frame=after['header'][4]
        while True:
            released = sample('native-scene-release')
            # A real section load clears the extra-seat mirrors until native
            # enrollment. Empty loading contexts are not dead participants.
            if released['header'][2]:
                assert all(p['hp'] for p in released['players']), released
            if released['play'][:2]==[6,0] and released['header'][2] and not released['play'][0x1c] and \
                    all(p['state']==1 and p['pose']>=2 and not p['script'] and not p['door'] for p in released['players']):
                break
            assert time.monotonic()<deadline and released['header'][4]-first_release_frame<2400, released
            runtime.input(0xbfff, 0)
            time.sleep(.12)
            runtime.input(0xffff, 0)
            time.sleep(.12)
        capture_pair(runtime, output/'cutscene-native-release.png')
        stats = runtime.request('render_pass_stats')
        assert stats['verify_checks'] and not any(stats[k] for k in
            ('verify_mismatch','aborted','watchdog','vram_leaks')), stats
        return dict(passed=True, native_gate=True, frozen_phase='explicit offline setup',
                    before=before, after=after, released=released, render_stats=stats,
                    native_section_transition=released['play'][13]!=before['play'][13])
    finally:
        if saved_freeze is not None:
            runtime.write(0x801721d0, saved_freeze)
        for seat in range(count):
            runtime.input(0xffff, seat)
        runtime.request('turbo', enabled=0)
        (output/'cutscene-events.json').write_text(json.dumps(events, indent=2), encoding='utf-8')
