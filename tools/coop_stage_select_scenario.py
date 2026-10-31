"""Native board/intro/spawn checks after Continue; Pillow compares intro panes."""
import struct
import time
import json

from coop_debug import capture_pair


def stage_select(s):
    r = s.r
    assert r.read(0x801721c0, 2) == bytes((3, 4)), 'Use --card-seed to reach the native board'
    time.sleep(.3)

    def actors():
        pool = r.read(0x80173ca0, 0x1800)
        return [dict(address=0x80173ca0 + i, subtype=pool[i + 2],
                     x=struct.unpack_from('<i', pool, i + 8)[0] / 65536,
                     table=struct.unpack_from('<I', pool, i + 0x30)[0],
                     sequence=pool[i + 0x17],
                     duration=pool[i + 0x44],
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
    assert len(initial) == 2 + 2 * int(expected), (initial, play.hex())
    pictures = [picture('stage-select-both-heroes')]
    companion_views_distinct = None
    if expected and s.capture_composed:
        from PIL import Image, ImageChops
        with Image.open(pictures[0]) as image:
            width, height = image.size
            if width > height * 2:
                pane = width // 2
                # Compare only the companion area, below the top portraits.
                bounds = (pane * 265 // 320, height // 3, pane, height * 2 // 3)
                left = image.crop(bounds)
                right = image.crop((bounds[0] + pane, bounds[1], width, bounds[3]))
                companion_views_distinct = ImageChops.difference(left, right).getbbox() is not None
                assert not companion_views_distinct, 'P1/P2 companion order differs between screens'
    for operation in ('save', 'load'):
        generation = r.request('savestate_status')['generation']
        r.request('savestate', op=operation, slot=0)
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            state = r.request('savestate_status')
            if state['generation'] > generation and not state['pending']:
                assert state['last_ok'], state
                break
            time.sleep(.03)
        else:
            raise TimeoutError((operation, state))
    assert [(v['address'], v['subtype']) for v in actors()] == [
        (v['address'], v['subtype']) for v in initial]
    # P2 must not acquire campaign/stage-choice ownership from its local image.
    r.input(0xffbf, 1)
    time.sleep(.12)
    r.release()
    assert r.read(0x801721c3, 1)[0] == play[3]
    if play[3] == 8:
        r.input(0xffef, 0)
        time.sleep(.12)
    # Storm Owl's introduction reproduces the recycled-particle regression.
    for unused in range(2):
        r.input(0xffdf, 0)
        time.sleep(.12)
        r.release()
        time.sleep(.12)
    advanced = set()
    iris_selected = not expected
    r.input(0xbfff, 0)
    try:
        deadline = time.monotonic() + .45
        while time.monotonic() < deadline:
            current = {actor['address']: actor for actor in actors()}
            for hero in heroes:
                now = current.get(hero['address'])
                if now and now['animation'] != hero['animation']:
                    advanced.add(hero['address'])
            iris_selected |= any(v['subtype'] == 1 and v['table'] == 0x8010e55c
                                 and v['sequence'] in (0, 1) and any(
                                     old['address'] == v['address'] and
                                     (old['duration'] != v['duration'] or old['animation'] != v['animation'])
                                     for old in initial)
                                 for v in current.values())
            time.sleep(.02)
        pictures.append(picture('stage-select-animation'))
    finally:
        r.release()
    assert len(advanced) == 2, (heroes, advanced)
    assert iris_selected, 'Iris native animation clock stopped during selection'
    intro = None
    spawn = []
    deadline = time.monotonic() + 60
    while time.monotonic() < deadline:
        mode = r.read(0x801721c0, 4)
        if mode[:2] == bytes((3, 11)) and mode[2] >= 1 and intro is None:
            from PIL import Image, ImageChops
            candidate = picture('stage-introduction')
            with Image.open(candidate) as image:
                width, height = image.size
                a = image.crop((0, 64, width // 2, height))
                # A black fade cannot establish that the intro is correct.
                visible = sum(max(pixel) > 64 and max(pixel)-min(pixel) > 24
                              for pixel in a.convert('RGB').getdata())
                if visible > 200:
                    intro = candidate
                    pictures.append(intro)
                    if s.capture_composed and width > height * 2:
                        b = image.crop((width // 2, 64, width, height))
                        assert ImageChops.difference(a, b).getbbox() is None, 'Intro panes differ'
        if mode[0] == 6:
            bodies = r.players()
            for body in bodies:
                if body[0] and body[3]:
                    table = struct.unpack_from('<I', body, 0x30)[0]
                    assert table not in (0x8010e514, 0x8010e538), 'Board pose leaked into gameplay'
            spawn.append([body.hex() for body in bodies])
            if all(b[0] and b[3] and b[4] == 1 and b[5] >= 2 for b in bodies):
                break
        time.sleep(.02)
    else:
        raise TimeoutError('Board-to-introduction-to-spawn did not complete')
    assert intro is not None, 'No native boss-introduction screen observed'
    r.wait_playable(20, advance_dialogue=True)
    pictures.append(picture('stage-spawn'))
    assert r.read(0x80172203, 1)[0] == s.campaign
    (s.output / 'spawn-frames.json').write_text(json.dumps(spawn))
    return dict(actors=initial, both_native_animations=True, iris_selection_animation=iris_selected,
                companion_views_distinct=companion_views_distinct, p1_stage_choice=True,
                native_companion_admission=bool(expected), intro_panes_equal=True,
                spawn_board_pose_leak=False, spawn_samples=len(spawn), pictures=pictures)
