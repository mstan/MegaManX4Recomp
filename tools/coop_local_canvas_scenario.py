"""Native offline 8:3 layout and Jungle tile-ring regression qualification."""
import json
import time


def local_canvas(self):
    r = self.r
    reports = {}
    pictures = []

    def capture(label):
        path = self.output / (label + '.png')
        before = r.request('present_shot_seq')['seq']
        r.request('present_shot', path=str(path))
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            result = r.request('present_shot_seq')
            if result['seq'] != before:
                assert result['wrote'] and path.is_file(), result
                pictures.append(str(path))
                return
            time.sleep(.03)
        raise TimeoutError('No composed frame: ' + label)

    def canvas(label, history=20):
        data = r.request('gl_present_ring', n=history)
        assert data['events'], label
        for event in data['events']:
            _, _, width, height = event[5]
            assert width > 0 and height > 0 and abs(width * 3 - height * 8) <= 8, (
                label, event)
        reports[label] = data

    # Boot's canonical menus and CPU/movie presents must already use 8:3,
    # before gameplay has submitted any independent camera pair.
    canvas('startup', 4096)
    r.request('window_size', w=1280, h=720)
    self.load(1, 0)
    r.fixture(5, 3)
    r.teleport_both(56, 139)
    self.wait('canvas-start-grounded', lambda v: all(
        p['hp'] and p['grounded'] for p in v['players']), 10)
    time.sleep(.5)
    capture('canvas-near')
    canvas('near')
    # P2 stays on the opening platform. With the shared retail tile ring,
    # moving P1 down to the waterfall tears apart that stationary platform.
    r.teleport(0, 500, 394)
    apart = self.wait('canvas-p1-ahead', lambda v: all(
        p['hp'] and p['grounded'] for p in v['players']) and
        v['players'][0]['x'] - v['players'][1]['x'] > 400, 10)
    time.sleep(.5)
    capture('canvas-p1-ahead')
    canvas('apart')
    counters = {c['name']: c['count'] for c in r.request('mod_counters')['counters']}
    assert counters.get('mmx4.renderer.tiles', 0), 'Authored-map renderer was not loaded'
    assert counters.get('mmx4.coop.local-pair-committed', 0), counters

    r.request('render_pass_refuse', on=1)
    try:
        time.sleep(.5)
        capture('canvas-refused-pair')
        canvas('refused')
    finally:
        r.request('render_pass_refuse', on=0)
    time.sleep(.5)
    r.input(0xfff7, 0)
    time.sleep(.15)
    r.release()
    self.wait('canvas-paused', lambda v: v['mode'][:2] == [6, 2], 10)
    time.sleep(.5)
    capture('canvas-pause')
    canvas('pause')
    assert r.request('stereo_stats')['published']['mode'] == 2
    deadline = time.monotonic() + 30
    while True:
        menu=list(r.read(0x801754a4,2))
        if menu[0]==1 and menu[1]<3:
            break
        if time.monotonic()>deadline:
            raise TimeoutError('Native pause did not become interactive')
        time.sleep(.03)
    r.input(0xfff7,0)
    time.sleep(.15)
    r.release()
    deadline = time.monotonic() + 30
    while r.read(0x801721c0, 2) != bytes([6, 0]):
        if time.monotonic() > deadline:
            raise TimeoutError('Native pause did not close')
        time.sleep(.03)
    time.sleep(.5)
    capture('canvas-resumed')
    canvas('resumed')
    r.request('window_size', w=960, h=720)
    time.sleep(.5)
    capture('canvas-resized')
    canvas('resized', 5)
    stats = r.request('render_pass_stats')
    assert stats['verify_checks'] > 0 and stats['verify_mismatch'] == 0, stats
    assert stats['aborted'] == stats['watchdog'] == stats['vram_leaks'] == 0, stats
    assert not counters.get('mmx4.coop.local-view-state-leak', 0), counters
    (self.output / 'canvas-presents.json').write_text(json.dumps(reports, indent=2))
    return dict(apart=apart, pictures=pictures, render_pass=stats,
                counters=counters, present_samples={k: len(v['events']) for k, v in reports.items()})
