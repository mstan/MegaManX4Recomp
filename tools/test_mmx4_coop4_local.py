"""Bounded native local 3/4-player split check, with isolated muted profiles."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

from coop_debug import Runtime, capture_pair
from coop_private_window_input import PrivateWindowInput
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe', type=Path, required=True)
parser.add_argument('--disc', type=Path, required=True)
parser.add_argument('--report-dir', type=Path, required=True)
parser.add_argument('--players', type=int, choices=(3, 4), default=4)
parser.add_argument('--port', type=int, default=17941)
parser.add_argument('--free-cutscenes', action='store_true')
parser.add_argument('--game-config', type=Path)
parser.add_argument('--native-exe', type=Path)
args = parser.parse_args()
if os.name != 'nt':
    parser.error('Independent four-seat keyboard input currently requires Windows')
source = Path(__file__).resolve().parents[1]
exe = args.exe.resolve()
root = args.report_dir.resolve()
assert root.name.startswith('labels-production-qa'), 'Use an isolated labels-production-qa* report directory'
root.mkdir(parents=True, exist_ok=False)
for name in ('assets', 'mods', 'bios'):
    shutil.copytree(exe.parent / name, root / name)
for file in (exe, *exe.parent.glob('*.dll')):
    shutil.copy2(file, root / file.name)
state = 'format_version = 2\n[[feature]]\npackage_id = "mmx4.coop4"\nid = "coop"\nenabled = true\n'
if args.players == 3:
    state += '[feature.values]\np4 = "none"\n'
if args.free_cutscenes:
    state += '\n[[feature]]\npackage_id = "mmx4.coop-free-cutscenes"\nid = "free-cutscenes"\nenabled = true\n'
(root / 'mods/state.toml').write_text(state, encoding='utf-8')
config = (args.game_config or source / 'game.toml').read_text(encoding='utf-8').replace(
    'exe = "mmx4/SLUS_005.61"', 'exe = ' + json.dumps((args.native_exe or source / 'mmx4/SLUS_005.61').resolve().as_posix()))
(root / 'game.toml').write_text(config, encoding='utf-8')
(root / 'settings.toml').write_text(
    '[video]\nfullscreen = false\nwindow_width = 960\ninternal_resolution = "native"\ndynamic_resolution = false\nrender_thread = true\n[audio]\nvolume = 0\n[controller]\nmultitap = true\n' +
    ''.join(f'p{i}_device = "keyboard"\np{i}_mode = "digital"\n' for i in range(1, 5)), encoding='utf-8')
# Distinct physical keys pass through the runtime's normal SDL/controller
# routing. The development TCP pad overrides only cover the first two seats.
key_names = [
    dict(up='Up', right='Right', down='Down', left='Left', square='Z', cross='X', start='Return'),
    dict(up='W', right='D', down='S', left='A', square='Q', cross='E', start='F2'),
    dict(up='I', right='L', down='K', left='J', square='U', cross='O', start='F3'),
    dict(up='T', right='H', down='G', left='F', square='V', cross='B', start='F4'),
]
key_codes = [
    dict(up=38, right=39, down=40, left=37, square=90, cross=88, start=13),
    dict(up=87, right=68, down=83, left=65, square=81, cross=69, start=113),
    dict(up=73, right=76, down=75, left=74, square=85, cross=79, start=114),
    dict(up=84, right=72, down=71, left=70, square=86, cross=66, start=115),
]
buttons = ('up down left right cross circle square triangle l1 r1 l2 r2 l3 r3 start select '
           'ls_up ls_down ls_left ls_right rs_up rs_down rs_left rs_right').split()
(root / 'keybinds.ini').write_text('\n'.join(
    f'[player{seat+1}]\n' + ''.join(f'{key} = {keys.get(key, "None")}\n' for key in buttons)
    for seat, keys in enumerate(key_names)), encoding='utf-8')
env = {key: value for key, value in os.environ.items()
       if not key.upper().startswith(('PSX_', 'RNET_', 'LNG_'))}
for key, name in dict(TEMP='temp', TMP='temp', APPDATA='appdata', LOCALAPPDATA='local',
                      PSX_RESIDENT_CACHE='resident', PSX_OVERLAY_CACHE_DIR='overlay',
                      PSX_OVERLAY_CAPTURE_ROOT='captures', MESA_SHADER_CACHE_DIR='shaders').items():
    path = root / name
    path.mkdir(exist_ok=True)
    env[key] = str(path)
env.update(SDL_AUDIODRIVER='dummy', SDL_WINDOWS_RAW_KEYBOARD='0', MMX4_RECOMP_DATA_DIR=str(root),
           PSX_OVERLAY_AUTOCOMPILE_OFF='1', PSX_RENDER_PASS_VERIFY='1',
           PSX_DEBUG_TOGGLES='0', PSX_DYNRES='0', PSX_INTERNAL_RESOLUTION='native')
runtime = Runtime(args.port)
report = {'players': args.players, 'executable_sha256': hashlib.sha256(exe.read_bytes()).hexdigest(),
          'input': 'Distinct private keyboard bindings through SDL; owned hidden window only',
          'keybindings': key_names, 'checks': [], 'pictures': []}
keyboard = None


def seat_input(buttons=0xffff, seat=0):
    assert keyboard is not None and 0 <= seat < args.players
    for name, bit in dict(up=0x10, right=0x20, down=0x40, left=0x80,
                          start=8, cross=0x4000, square=0x8000).items():
        vk = key_codes[seat][name]
        wanted = not bool(buttons & bit)
        if (vk in keyboard.down) != wanted:
            keyboard.key(vk, wanted)


def save():
    (root / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


def sample(label):
    data = runtime.read(diagnostic, 0xdd0)
    header = struct.unpack_from('<6I', data)
    assert header[1] == args.players and not header[3], header
    players = []
    for seat in range(args.players):
        body = data[0x100 + seat * 256:0x1e4 + seat * 256]
        camera = data[0x500 + seat * 256:0x5fc + seat * 256]
        players.append(dict(character=body[2], state=body[4], pose=body[5], hp=body[0x5c] & 127,
                            x=struct.unpack_from('<i', body, 8)[0] / 65536,
                            y=struct.unpack_from('<i', body, 12)[0] / 65536,
                            camera_x=struct.unpack_from('<i', camera, 8)[0] / 65536))
    row = dict(label=label, header=header, players=players,
               menus=[list(data[0xd00+i*0x34:0xd34+i*0x34]) for i in range(args.players)],
               mode=list(runtime.read(0x801721c0, 2)))
    report['checks'].append(row)
    save()
    return row


def frames(count, timeout=30):
    start = struct.unpack('<I', runtime.read(diagnostic + 16, 4))[0]
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if struct.unpack('<I', runtime.read(diagnostic + 16, 4))[0] >= start + count:
            return
        time.sleep(.03)
    raise TimeoutError('Native simulation stopped')


def picture(label):
    path = root / (label + '.png')
    capture_pair(runtime, path)
    with Image.open(path) as original:
        image = original.convert('RGB')
    width, height = image.size
    canvas_width, canvas_height = ((height * 4 // 3, height) if width * 3 >= height * 4
                                   else (width, width * 3 // 4))
    x, y = (width-canvas_width)//2, (height-canvas_height)//2
    half_w, half_h = canvas_width//2, canvas_height//2
    panes = [(x, y, x+half_w, y+half_h),
             (x+half_w, y, x+canvas_width, y+half_h)]
    if args.players == 3:
        left = x + (canvas_width-half_w)//2
        panes.append((left, y+half_h, left+half_w, y+canvas_height))
        pillars = [(x, y+half_h, left, y+canvas_height),
                   (left+half_w, y+half_h, x+canvas_width, y+canvas_height)]
        assert all(image.crop(rect).getbbox() is None for rect in pillars), (label, pillars)
    else:
        panes += [(x, y+half_h, x+half_w, y+canvas_height),
                  (x+half_w, y+half_h, x+canvas_width, y+canvas_height)]
    for rect in panes:
        pane = image.crop(rect).convert('L')
        assert pane.histogram()[0] < pane.width*pane.height*.95, (label, rect)
    report['pictures'].append(dict(label=label, path=path.name, size=[width, height], panes=panes,
                                   centered_p3_pillars_black=args.players == 3))
    save()


def native_fixture(command, seat, value):
    # Offline native damage grace isolates controller ownership from enemy
    # knockback. Door/death qualification must run without this fixture.
    def word(address, value):
        for byte in range(4):
            runtime.write(address+byte, (value >> (byte*8)) & 255)
    seq = struct.unpack('<I', runtime.read(diagnostic+0x30, 4))[0]+1
    for offset, item in ((0x34, command), (0x38, seat), (0x3c, value)):
        word(diagnostic+offset, item)
    word(diagnostic+0x30, seq)
    deadline = time.monotonic()+10
    while struct.unpack('<I', runtime.read(diagnostic+0x44, 4))[0] != seq:
        assert time.monotonic() < deadline, 'Offline protection was not acknowledged'
        time.sleep(.03)
    report['input_fixture'] = 'Native damage grace; no door/death claim'


command = [str(root / exe.name), '--hidden-window', '--no-launcher', '--renderer', 'opengl',
           '--debug-port', str(args.port), '--game', str(root / 'game.toml'),
           '--bios', str(root / 'bios/openbios.bin'), '--disc', str(args.disc.resolve()),
           '--memcard-dir', str(root / 'saves')]
with (root / 'runtime.log').open('wb') as log:
    process = subprocess.Popen(command, cwd=root, env=env, stdout=log, stderr=log,
                               creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
    try:
        deadline = time.monotonic() + 100
        while True:
            assert process.poll() is None, 'Runtime exited'
            try:
                runtime.request('get_registers')
                break
            except OSError:
                if time.monotonic() > deadline:
                    raise
                time.sleep(.1)
        diagnostic = next(c['count'] for c in runtime.request('mod_counters')['counters']
                          if c['name'] == 'mmx4.coop4.diagnostic-address')
        runtime.request('turbo', enabled=1)
        runtime.boot(wait_for_coop=False)
        runtime.request('turbo', enabled=0)
        keyboard = PrivateWindowInput(process, root / exe.name)
        # Optional story checks use the same actual four-seat keyboard path.
        runtime.input = seat_input
        deadline = time.monotonic() + 30
        while True:
            row = sample('enrolling')
            if row['header'][2] and all(p['state'] == 1 and p['hp'] for p in row['players']):
                break
            if time.monotonic() > deadline:
                raise TimeoutError('Native intro did not enroll all seats')
            time.sleep(.15)
        frames(12)
        deadline = time.monotonic() + 20
        while any(p['pose'] < 2 for p in sample('arrival')['players']):
            if time.monotonic() > deadline:
                raise TimeoutError('Concurrent native arrivals did not finish')
            time.sleep(.05)
        picture('spawn')
        native_fixture(3, 0, (1 << args.players)-1)
        for seat in range(args.players):
            before = sample('before-seat-' + str(seat + 1))
            seat_input(0xffdf, seat)
            frames(20)
            seat_input(0xffff, seat)
            frames(6)
            after = sample('seat-' + str(seat + 1) + '-moved')
            assert after['players'][seat]['x'] > before['players'][seat]['x'] + 15, (seat, before, after)
            for other in range(args.players):
                if other != seat:
                    assert abs(after['players'][other]['x'] - before['players'][other]['x']) < 2, (seat, other)
            seat_input(0x7fff, seat)
            frames(8)
            seat_input(0xffff, seat)
            frames(8)
        picture('all-seats-moved')
        leader = args.players - 1
        seat_input(0xffdf, leader)
        frames(140)
        seat_input(0xffff, leader)
        frames(8)
        apart = sample('last-seat-leading')
        assert apart['players'][leader]['x'] - apart['players'][0]['x'] > 250, apart
        assert apart['players'][leader]['camera_x'] - apart['players'][0]['camera_x'] > 200, apart
        assert all(p['hp'] for p in apart['players']), apart
        picture('independent-cameras')
        runtime.request('window_size', w=1280, h=720)
        frames(10)
        picture('resized')
        runtime.request('render_pass_refuse', on=1)
        try:
            time.sleep(.3)
            picture('refused-shared-fallback')
        finally:
            runtime.request('render_pass_refuse', on=0)
        frames(8)
        for seat in range(args.players):
            native_fixture(4, seat, 16)
        seat_input(0xfff7, leader)
        time.sleep(.2)
        seat_input(0xffff, leader)
        deadline = time.monotonic() + 8
        while runtime.read(0x801721c0, 2) != bytes([6, 2]):
            if time.monotonic() > deadline:
                raise TimeoutError('Pause did not open')
            time.sleep(.05)
        deadline = time.monotonic() + 30
        while True:
            paused = sample('private-pause-initializing')
            if all(m[4] == 1 for m in paused['menus']):
                break
            if time.monotonic() > deadline:
                raise TimeoutError('Private menus did not initialize')
            time.sleep(.05)
        picture('private-pause')
        # X starts in the weapon column; an unupgraded Zero starts on
        # the horizontal tank row. Use an available native neighbor.
        for seat in range(args.players):
            before = sample('menu-before-' + str(seat+1))
            seat_input(0xffdf if before['players'][seat]['character'] else 0xffbf, seat)
            time.sleep(.15)
            seat_input(0xffff, seat)
            time.sleep(.15)
            after = sample('menu-after-' + str(seat+1))
            assert after['menus'][seat][0x14] != before['menus'][seat][0x14], (seat, before, after)
            for other in range(args.players):
                if other != seat:
                    assert after['menus'][other][0x14] == before['menus'][other][0x14], (seat, other)
        picture('independent-menu-cursors')
        deadline = time.monotonic() + 30
        while runtime.read(0x801721c0, 2) != bytes([6, 0]):
            if time.monotonic() > deadline:
                raise TimeoutError('Pause did not close')
            seat_input(0xfff7)
            time.sleep(.2)
            seat_input()
            time.sleep(.3)
        frames(8)
        picture('resumed')
        stats = runtime.request('render_pass_stats')
        counters = {c['name']: c['count'] for c in runtime.request('mod_counters')['counters']}
        grid = runtime.request('stereo_stats')
        report.update(render_stats=stats, counters=counters, grid=grid,
                      presents=runtime.request('gl_present_ring', n=20))
        assert counters.get('mmx4.coop4.local-grid-committed', 0) > 10, counters
        assert counters.get('mmx4.renderer.activated') == 1, counters
        assert all(counters.get(f'mmx4.renderer.layer{i}-tiles', 0) > 0 for i in range(3)), counters
        assert not counters.get('mmx4.renderer.arena-failed', 0), counters
        assert runtime.request('gpu_state')['ws']['nw_extra'] == 0
        assert not counters.get('mmx4.coop4.local-view-state-leak', 0), counters
        assert stats['verify_checks'] > 0 and not any(stats[k] for k in
            ('verify_mismatch', 'aborted', 'watchdog', 'vram_leaks')), stats
        expected_mode = 4 if args.players == 3 else 3
        assert grid['published']['mode'] == expected_mode and grid['published']['valid'], grid
        for event in report['presents']['events']:
            _, _, w, h = event[5]
            assert abs(w * 6 - h * 8) <= 8, event
        if args.free_cutscenes:
            from coop_four_cutscene_scenario import four_cutscene
            report['cutscene'] = four_cutscene(runtime, diagnostic, args.players, root)
        report['passed'] = True
        save()
        print('Native local split passed:', args.players, 'players', flush=True)
    except BaseException as error:
        try:
            sample('failure')
            report['render_stats'] = runtime.request('render_pass_stats')
        except (OSError, RuntimeError):
            pass
        report.update(passed=False, error=repr(error))
        save()
        raise
    finally:
        if keyboard and process.poll() is None:
            keyboard.release_all()
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=15)
