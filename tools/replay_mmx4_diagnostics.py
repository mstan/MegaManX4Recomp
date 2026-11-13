"""Replay an offline X4 diagnostic recording in a private hidden process.

Use the recording's original build, BIOS, disc, game config and settings.
Starting cards and mod choices come from the recording. Segments created by
save-state loads or rewind also restore their complete baseline.pst. The runtime verifies
the build/mod identity and each native-input state hash. Network recordings
and developer teleport/HP/stage commands are not supported for playback.
The final input interval is dropped because an interrupted recording may have
lost its trailing co-op reads. Personal cards/settings are never overwritten.
On Linux, use the native executable and payload extracted from the original
AppImage. Direct AppImage execution is rejected because AppRun seeds user data.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time
import tomllib


def native_executable(path):
    executable = path.resolve(strict=True)
    with executable.open('rb') as source:
        header = source.read(11)
    if (executable.suffix.lower() == '.appimage' or
            (header[:4] == b'\x7fELF' and header[8:11] in (b'AI\x01', b'AI\x02'))):
        raise ValueError(
            'Direct AppImage replay is unsupported: AppRun would seed user data. '
            'Extract the original image with --appimage-extract, then use '
            '--exe squashfs-root/usr/bin/MegaManX4Recomp '
            '--game squashfs-root/usr/share/megamanx4recomp/game.toml')
    return executable


def copy_native_libraries(executable, output):
    # Dereference each soname link into the private directory. A link back to
    # the extracted mount or original installation would defeat isolation.
    roots = [executable.parent]
    if executable.parent.name == 'bin':
        roots.append(executable.parent.parent / 'lib')
    destination = output / 'lib'
    for root in roots:
        for library in root.glob('*.so*'):
            if not library.is_file():
                continue
            destination.mkdir(exist_ok=True)
            target = destination / library.name
            if not target.exists():
                shutil.copy2(library, target, follow_symlinks=True)


def replay_environment(output, trace, source_env):
    env = {key: value for key, value in source_env.items()
           if not key.startswith(('PSX_', 'RNET_', 'LNG_', 'MMX4_DIAGNOSTIC_', 'MMX4_RECOMP_'))
           and key not in ('APPIMAGE', 'APPDIR', 'RECOMP_APPIMAGE_PATH', 'LD_LIBRARY_PATH')}
    env['MMX4_RECOMP_DATA_DIR'] = str(output)
    env['MMX4_DIAGNOSTIC_REPLAY'] = str(trace)
    # Use copied libraries before normal system libraries, without inheriting
    # dependencies from another AppImage mount or a developer installation.
    env['LD_LIBRARY_PATH'] = os.pathsep.join((str(output / 'lib'), str(output)))
    if (output / 'diagnostic-baseline.pst').is_file():
        env['PSX_LOAD_SLOT'] = '0'
    return env


def prepare(args):
    recording = args.recording.resolve()
    if recording.is_dir():
        recording /= 'inputs.csv'
    metadata=recording.parent/'session.json'
    session=json.loads(metadata.read_text(encoding='utf-8')) if metadata.exists() else {}
    if session.get('netplay'):
        raise ValueError('Netplay recording: useful for inspection, but offline replay is not supported')
    baseline=None
    if 'baseline' in session:
        if session['baseline']!='baseline.pst':
            raise ValueError('Unsupported diagnostic baseline filename')
        baseline=recording.parent/'baseline.pst'
        if not baseline.is_file() or not 0<baseline.stat().st_size<=64*1024*1024:
            raise ValueError('Recording has no complete restored-state baseline')
    with recording.open('rb') as source:
        if source.readline().rstrip(b'\r\n') != b'# mmx4-input-v2':
            raise ValueError('Expected a version 2 diagnostic recording')
        for prefix in (b'# execution ', b'# plan ', b'# build '):
            if not source.readline().startswith(prefix):
                raise ValueError('Incomplete diagnostic identity header')
        last_native, count = 0, 0
        while True:
            offset = source.tell()
            line = source.readline()
            if not line:
                break
            if line.startswith(b'N '):
                last_native, count = offset, count + 1
    if count < 2:
        raise ValueError('Recording has no complete input interval')
    executable = native_executable(args.exe)
    game = (args.game or executable.parent / 'game.toml').resolve(strict=True)
    config = game.read_text(encoding='utf-8')
    game_values = tomllib.loads(config)['game']
    section = re.search(r'^\[game\]\s*$.*?(?=^\[|\Z)', config, re.M | re.S)
    if not section:
        raise ValueError('Game configuration has no [game] table')
    text = section.group()
    for key in ('exe', 'disc'):
        path = args.disc.resolve() if key == 'disc' else Path(game_values[key])
        if not path.is_absolute():
            path = game.parent / path
        value = json.dumps(path.resolve().as_posix(), ensure_ascii=False)
        text = re.sub(r'^(' + key + r'\s*=\s*).+$', lambda m: m[1] + value, text, count=1, flags=re.M)
    config = config[:section.start()] + text + config[section.end():]
    cards = []
    for slot in (1, 2):
        card, absent = recording.parent / f'card{slot}.mcd', recording.parent / f'card{slot}.absent'
        if card.exists() == absent.exists():
            raise ValueError(f'Expected one starting card or absence marker for slot {slot}')
        if card.exists() and card.stat().st_size != 131072:
            raise ValueError(f'Invalid starting card size in slot {slot}')
        cards.append(card if card.exists() else None)
    state = recording.parent / 'mods-state.toml'
    if not state.is_file():
        raise ValueError('Recording is missing its starting mod choices')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    target = output / executable.name
    shutil.copy2(executable, target)
    # Windows release dependencies live beside the executable. An isolated
    # replay must work without relying on the developer's compiler PATH.
    for dependency in executable.parent.glob('*.dll'):
        shutil.copy2(dependency,output/dependency.name)
    copy_native_libraries(executable, output)
    for name in ('assets', 'mods', 'bios', 'overlay_toolchain'):
        # Extracted AppImages place the ELF under usr/bin and the title
        # resources beside the explicitly supplied payload game.toml.
        for root in (executable.parent, game.parent):
            source = root / name
            if source.is_dir():
                shutil.copytree(source, output / name)
                break
    (output / 'mods').mkdir(exist_ok=True)
    shutil.copy2(state, output / 'mods/state.toml')
    (output / 'game.toml').write_text(config, encoding='utf-8')
    settings_path = args.settings or (recording.parent / 'settings.toml'
        if (recording.parent / 'settings.toml').exists() else executable.parent / 'settings.toml')
    settings = settings_path.read_text(encoding='utf-8') if settings_path.exists() else ''
    settings = re.sub(r'^\[memcard\]\s*$.*?(?=^\[|\Z)', '', settings, flags=re.M | re.S)
    settings += '\n[memcard]\n' + ''.join(f'enable{i+1} = {str(card is not None).lower()}\n' for i, card in enumerate(cards))
    (output / 'settings.toml').write_text(settings, encoding='utf-8')
    (output / 'saves').mkdir()
    for i, card in enumerate(cards):
        if card:
            shutil.copy2(card, output / f'saves/card{i+1}.mcd')
    if baseline is not None:
        entry=int(game_values['entry_pc'],0)
        if not 0<entry<=0xffffffff:raise ValueError('Invalid game entry PC')
        # The runtime chooses the BIOS token by checksum, rather than filename.
        # Both private locations let the original BIOS select its own slot.
        for token in ('openbios','scph1001'):
            directory=output/'saves'/token;directory.mkdir()
            shutil.copy2(baseline,directory/f'state_{entry:08X}_slot00.pst')
        shutil.copy2(baseline,output/'diagnostic-baseline.pst')
    trace = output / 'replay-inputs.csv'
    with recording.open('rb') as source, trace.open('xb') as dest:
        remaining = last_native
        while remaining:
            chunk = source.read(min(remaining, 1024 * 1024))
            if not chunk:
                raise ValueError('Recording changed while preparing replay')
            dest.write(chunk)
            remaining -= len(chunk)
    return output, target, trace


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('recording', type=Path, help='Session directory or inputs.csv')
    parser.add_argument('--exe', type=Path, required=True, help='Original native X4 build')
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--game', type=Path)
    parser.add_argument('--settings', type=Path, help='Settings used when recording')
    parser.add_argument('--output', type=Path, required=True, help='New isolated directory')
    parser.add_argument('--timeout', type=float, default=120)
    parser.add_argument('--debug-port', type=int, help='Developer build TCP inspection port')
    parser.add_argument('--hidden-window', action='store_true',
                        help='Hidden drawable for composed Split-view inspection')
    args = parser.parse_args()
    bios, disc = args.bios.resolve(strict=True), args.disc.resolve(strict=True)
    output, target, trace = prepare(args)
    env = replay_environment(output, trace, os.environ)
    baseline = output / 'diagnostic-baseline.pst'
    if baseline.is_file():
        env['MMX4_DIAGNOSTIC_BASELINE'] = str(baseline)
    argv = [str(target), '--hidden-window' if args.hidden_window else '--headless-opengl',
            '--no-launcher', '--game', str(output / 'game.toml'),
            '--bios', str(bios), '--disc', str(disc), '--memcard-dir', str(output / 'saves')]
    if args.debug_port:
        argv += ['--debug-port', str(args.debug_port)]
    log_path = output / 'replay.log'
    flags = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
    result = 1
    with log_path.open('wb') as log:
        process = subprocess.Popen(argv, cwd=output, env=env, stdout=log, stderr=log, creationflags=flags)
        deadline = time.monotonic() + args.timeout
        try:
            while time.monotonic() < deadline:
                text = log_path.read_text(encoding='utf-8', errors='replace')
                if 'mmx4.diagnostics: input replay complete (' in text:
                    if not baseline.is_file() or 'mmx4.diagnostics: replay baseline restored' in text:
                        result = 0
                    break
                if 'mmx4.diagnostics: replay stopped at input ' in text or process.poll() is not None:
                    break
                time.sleep(.1)
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=15)
    print(('Replay verified. ' if result == 0 else 'Replay failed or timed out. ') + f'Log: {log_path}')
    raise SystemExit(result)


if __name__ == '__main__':
    main()
