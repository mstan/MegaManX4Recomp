"""Native board checks after Continue from a private card; images need visual review."""
import struct
import time

from coop_debug import capture_pair


def stage_select(s):
    r = s.r
    assert r.read(0x801721c0, 2) == bytes((3, 4)), 'Use --card-seed to reach the native board'
    time.sleep(.3)

    def actors():
        pool = r.read(0x80173ca0, 0x1800)
        return [dict(address=0x80173ca0 + i, subtype=pool[i + 2],
                     x=struct.unpack_from('<i', pool, i + 8)[0] / 65536,
                     animation=struct.unpack_from('<I', pool, i + 0x34)[0])
                for i in range(0, len(pool), 0x60)
                if pool[i:i + 2] == bytes((0x41, 10))]

    def picture(name):
        path = s.output / (name + '.png')
        if s.capture_composed:
            capture_pair(r, path)
        else:
            r.screenshot(path)
        return str(path)

    initial = actors()
    heroes = [actor for actor in initial if actor['subtype'] == 0]
    assert len(heroes) == 2, initial
    play = r.read(0x801721c0, 0x64)
    assert play[0x43] == s.campaign
    expected = play[0x5f] < (7 if s.campaign else 9) and play[0x0e] != (5 if s.campaign else 10)
    assert len(initial) == 2 + int(expected), (initial, play.hex())
    pictures = [picture('stage-select-both-heroes')]
    # P2 must not acquire campaign/stage-choice ownership from its local image.
    r.input(0xffbf, 1)
    time.sleep(.12)
    r.release()
    assert r.read(0x801721c3, 1)[0] == play[3]
    if play[3] == 8:
        r.input(0xffef, 0)
        time.sleep(.12)
        r.release()
        time.sleep(.12)
    advanced = set()
    r.input(0xbfff, 0)
    try:
        deadline = time.monotonic() + .45
        while time.monotonic() < deadline:
            current = {actor['address']: actor for actor in actors()}
            for hero in heroes:
                now = current.get(hero['address'])
                if now and now['animation'] != hero['animation']:
                    advanced.add(hero['address'])
            time.sleep(.02)
        pictures.append(picture('stage-select-animation'))
    finally:
        r.release()
    assert len(advanced) == 2, (heroes, advanced)
    r.wait_playable(45, advance_dialogue=True)
    assert r.read(0x80172203, 1)[0] == s.campaign
    return dict(actors=initial, both_native_animations=True, p1_stage_choice=True,
                native_companion_admission=bool(expected), pictures=pictures)
