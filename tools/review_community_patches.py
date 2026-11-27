"""Native review selector for a prepared, private current-build patch bundle.

Preparation and the normal game-image picker supply all assets. The selector
starts the first visible game only when its owner presses Open scenario.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import queue
import socket
import subprocess
import threading
import time


def stamp():
    return datetime.now(timezone.utc).isoformat(timespec='seconds')


def atomic_json(path, value):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value, indent=2), encoding='utf-8')
    temporary.replace(path)


class ReviewGame:
    def __init__(self, root):
        self.root = root.resolve()
        self.manifest = json.loads((self.root / 'review-manifest.json').read_text())
        self.process = None
        self.log = None
        self.case = None
        self.paused = False

    def exchange(self, command, fields):
        with socket.create_connection(('127.0.0.1', self.manifest['port']), timeout=10) as connection:
            connection.sendall((json.dumps(dict(id=1, cmd=command, **fields)) + '\n').encode())
            return json.loads(connection.makefile('rb').readline())

    def command(self, command, **fields):
        before = None
        if command == 'host_pause' and fields.get('on') in (0, 1):
            before = self.command('host_pause', on=-1)['depth']
        elif command == 'savestate':
            before = self.command('savestate_status')['generation']
        while True:
            try:
                result = self.exchange(command, fields)
            except (OSError, ValueError):
                if command == 'quit' and self.process:
                    self.process.wait()
                    return dict(ok=True)
                if not self.process or self.process.poll() is not None:
                    raise
                # A transport wait does not end healthy preparation. Observe
                # non-idempotent completion before considering a retry.
                if command == 'host_pause' and before is not None:
                    target = before + 1 if fields['on'] else max(0, before - 1)
                    if self.command('host_pause', on=-1)['depth'] == target:
                        return dict(ok=True, depth=target)
                elif command == 'savestate':
                    status = self.command('savestate_status')
                    if status['pending'] or status['generation'] != before:
                        return dict(ok=True)
                time.sleep(.1)
                continue
            if result.get('ok'):
                return result
            if result.get('err') == 'emu busy or frozen' and self.process and self.process.poll() is None:
                time.sleep(.05)
                continue
            raise RuntimeError(str(result))

    def state(self):
        result = self.command('read_ram', addr='0x801721c0', len=100)
        data = bytes.fromhex(result['hex'])
        body = bytes.fromhex(self.command('read_ram', addr='0x801418c8', len=96)['hex'])
        observed = dict(mode=list(data[:4]), stage=data[12], section=data[13], checkpoint=data[29],
                        campaign=data[0x43], character=body[2], hp=body[0x5c] & 127)
        boss = int.from_bytes(data[0x20:0x24], 'little')
        if data[0x24] and 0x8013bed0 <= boss < 0x8013dc10:
            actor = bytes.fromhex(self.command('read_ram', addr=hex(boss), len=96)['hex'])
            observed.update(boss_type=actor[1], boss_hp=actor[0x5c] & 127)
        return observed

    def pause(self):
        if self.process and self.process.poll() is None and not self.paused:
            self.command('host_pause', on=1)
            self.paused = True

    def play(self):
        if self.paused:
            self.command('clear_input')
            self.command('clear_input', port=2)
            self.command('host_pause', on=0)
            self.paused = False

    def snapshot(self, operation, slot):
        # Host pause is a dedicated gameplay pause. Snapshot polling needs the
        # execution thread at a safe boundary, then we restore that pause.
        was_paused = self.paused
        if was_paused:
            self.command('host_pause', on=0)
            self.paused = False
        try:
            self.command('set_input', buttons='0xffff', port=1)
            self.command('set_input', buttons='0xffff', port=2)
            generation = self.command('savestate_status')['generation']
            self.command('savestate', op=operation, slot=slot)
            while True:
                if self.process.poll() is not None:
                    raise RuntimeError('The review game exited; see its log.')
                status = self.command('savestate_status')
                if status['generation'] != generation and not status['pending']:
                    if not status['last_ok']:
                        raise RuntimeError('Current-build scenario snapshot was refused.')
                    break
                time.sleep(.05)
            if operation == 'load':
                # Restore acknowledgement precedes the next composed frame.
                # Let the restored scene render before applying host pause.
                first = self.command('mod_counters')['frame']
                while self.command('mod_counters')['frame'] - first < 4:
                    if self.process.poll() is not None:
                        raise RuntimeError('The review game exited before the scene rendered.')
                    time.sleep(.02)
        finally:
            if was_paused:
                self.pause()

    def close(self):
        if self.process and self.process.poll() is None:
            # The endpoint is reserved for this selector's child; never attach
            # to an existing game or stop an owner's unrelated session.
            self.command('quit')
            self.process.wait()
        self.process = None
        if self.log:
            self.log.close()
            self.log = None
        self.paused = False

    def open(self, case):
        if self.case and self.case['profile'] != case['profile']:
            self.close()
        self.case = case
        profile = self.root / case['profile']
        executable = profile / self.manifest['executable']
        if hashlib.sha256(executable.read_bytes()).hexdigest() != self.manifest['binary_sha256']:
            raise RuntimeError('The review executable changed; prepare new current-build states.')
        if self.process is None or self.process.poll() is not None:
            with socket.socket() as reservation:
                reservation.bind(('127.0.0.1', self.manifest['port']))
            env = {key: value for key, value in os.environ.items()
                   if not key.upper().startswith(('PSX_', 'RNET_', 'MMX4_', 'SDL_'))}
            env['PATH'] = 'C:/msys64/mingw64/bin;' + env.get('PATH', '')
            env['MMX4_RECOMP_DATA_DIR'] = str(profile)
            env['PSX_STARVATION_TIMEOUT_US'] = '0'
            temporary = self.root / 'tmp'
            temporary.mkdir(exist_ok=True)
            env['TEMP'] = env['TMP'] = str(temporary)
            self.log = (profile / ('owner-' + stamp().replace(':', '-') + '.log')).open('wb')
            argv = [str(executable), '--no-launcher', '--game', str(profile / 'game.toml'),
                    '--bios', self.manifest['bios'], '--disc', self.manifest['disc'],
                    '--memcard-dir', str(profile / 'saves'), '--debug-port', str(self.manifest['port'])]
            self.process = subprocess.Popen(argv, cwd=profile, env=env, stdin=subprocess.DEVNULL,
                                            stdout=self.log, stderr=subprocess.STDOUT,
                                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
            while True:
                if self.process.poll() is not None:
                    raise RuntimeError('The review game exited during startup; see its log.')
                try:
                    self.command('get_registers')
                    break
                except (OSError, ValueError):
                    time.sleep(.1)
            self.pause()
        self.snapshot('load', case['slot'])
        self.pause()
        observed = self.state()
        if (observed['stage'] != case['stage'] or observed['section'] != case['section'] or
                observed['mode'][0] != 6 or
                observed['character'] != case['character'] or
                observed['campaign'] != case['character'] or
                observed['hp'] != case.get('expected_hp', observed['hp']) or
                observed.get('boss_type') != case.get('expected_boss_type', observed.get('boss_type'))):
            raise RuntimeError('Prepared scenario loaded the wrong scene: ' + str(observed))
        atomic_json(self.root / 'owner-session.json', dict(pid=self.process.pid, case=case['id'],
                    paused=True, observed=observed, opened=stamp()))

    def capture(self, case):
        destination = self.root / 'captures'
        destination.mkdir(exist_ok=True)
        path = destination / (case['id'] + '-' + stamp().replace(':', '-') + '.png')
        sequence = self.command('present_shot_seq')['seq']
        self.command('present_shot', path=str(path))
        while self.command('present_shot_seq')['seq'] == sequence:
            if self.process.poll() is not None:
                raise RuntimeError('The review game exited before its capture completed.')
            time.sleep(.05)
        return str(path)


def panel(root):
    import tkinter as tk
    from tkinter import ttk, messagebox
    game = ReviewGame(root)
    notes_path = game.root / 'review-observations.json'
    notes = json.loads(notes_path.read_text()) if notes_path.exists() else dict(history=[])
    window = tk.Tk()
    window.title('Mega Man X4 - Community patch QA')
    window.geometry('760x720')
    window.minsize(700, 650)
    body = ttk.Frame(window, padding=16)
    body.pack(fill='both', expand=True)
    ttk.Label(body, text='Mega Man X4 community patches', font=('Segoe UI', 16, 'bold')).pack(anchor='w')
    ttk.Label(body, text='Patch design and artwork: flipstrike and hasungkee.').pack(anchor='w', pady=(4, 10))
    selected = tk.StringVar(value=game.manifest['scenarios'][0]['title'])
    choices = ttk.Combobox(body, textvariable=selected, state='readonly',
                          values=[case['title'] for case in game.manifest['scenarios']])
    choices.pack(fill='x')
    steps = tk.StringVar()
    expected = tk.StringVar()
    history = tk.StringVar()
    status = tk.StringVar(value='Ready. Open scenario starts your review game paused, in a normal window with normal audio.')
    ttk.Label(body, textvariable=steps, wraplength=710, justify='left').pack(anchor='w', pady=(12, 7))
    ttk.Label(body, textvariable=expected, wraplength=710, justify='left').pack(anchor='w', pady=7)
    controls = ttk.Frame(body)
    controls.pack(fill='x', pady=10)
    events = queue.Queue()
    busy = [False]

    def case():
        return next(row for row in game.manifest['scenarios'] if row['title'] == selected.get())

    def action(function):
        if busy[0]:
            return
        busy[0] = True
        choices.configure(state='disabled')
        status.set('Preparing the selected scene...')
        def worker():
            try:
                function()
                events.put(('ok', 'Ready; game paused.' if game.paused else 'Playing. Pause before changing scenes or recording a result.'))
            except Exception as error:
                events.put(('error', str(error)))
        threading.Thread(target=worker, daemon=True).start()

    def open_selected():
        selected_case = case()
        action(lambda: game.open(selected_case))
    for label, command in [('Open scenario', open_selected), ('Play', lambda: action(game.play)),
                           ('Pause', lambda: action(game.pause)), ('Restart', open_selected)]:
        ttk.Button(controls, text=label, command=command).pack(side='left', padx=(0, 7))
    def reference():
        name = case().get('reference')
        if not name:
            status.set('This check uses the expected behavior described above.')
            return
        picture = tk.PhotoImage(file=str(game.root / name))
        preview = tk.Toplevel(window)
        preview.title('Supplied patch reference')
        label = ttk.Label(preview, image=picture)
        label.image = picture
        label.pack(padx=12, pady=12)
    ttk.Button(controls, text='Reference', command=reference).pack(side='left')
    ttk.Label(body, text='Notes for this individual check:').pack(anchor='w', pady=(8, 3))
    text = tk.Text(body, height=8, wrap='word', font=('Segoe UI', 11))
    text.pack(fill='both', expand=True)

    def save_draft(*_):
        notes.setdefault('drafts', {})[case()['id']] = text.get('1.0', 'end-1c')
        atomic_json(notes_path, notes)

    current = [None]
    def describe(*_):
        if current[0]:
            notes.setdefault('drafts', {})[current[0]] = text.get('1.0', 'end-1c')
        row = case()
        if game.case and game.case['id'] != row['id'] and game.process and game.process.poll() is None:
            action(game.pause)
        current[0] = row['id']
        steps.set(row['steps'])
        expected.set('Expected: ' + row['expected'])
        text.delete('1.0', 'end')
        text.insert('1.0', notes.get('drafts', {}).get(row['id'], ''))
        previous = [item for item in notes['history'] if item['case'] == row['id']]
        history.set('Not reviewed yet.' if not previous else 'Latest result: ' + previous[-1]['result'] + ' | All earlier observations retained.')
        atomic_json(notes_path, notes)

    def record(verdict):
        if busy[0]:
            return
        row = case()
        observation = dict(case=row['id'], result=verdict, notes=text.get('1.0', 'end-1c'),
                           recorded=stamp(), binary_sha256=game.manifest['binary_sha256'])
        if game.case and game.case['id'] == row['id'] and game.process and game.process.poll() is None:
            try:
                game.pause()
                observation['scene'] = game.state()
                observation['capture'] = game.capture(row)
            except Exception as error:
                observation['capture_note'] = str(error)
        elif verdict == 'Pass':
            status.set('Open and review this scenario before recording a Pass.')
            return
        notes['history'].append(observation)
        save_draft()
        describe()
        status.set(row['title'] + ': ' + verdict + ' saved.')

    verdicts = ttk.Frame(body)
    verdicts.pack(fill='x', pady=12)
    for verdict in ('Pass', 'Bug found', 'Deferred'):
        ttk.Button(verdicts, text=verdict, command=lambda value=verdict: record(value)).pack(side='left', padx=(0, 8))
    ttk.Label(body, textvariable=history, wraplength=710).pack(anchor='w', pady=5)
    ttk.Label(body, textvariable=status, foreground='#175680', wraplength=710).pack(anchor='w', pady=7)
    ttk.Label(body, text=game.manifest['controls'], wraplength=710, foreground='#555555').pack(anchor='w', pady=6)
    ttk.Label(body, text='Private saves and settings. Closing this panel closes only its own review game.',
              wraplength=710, foreground='#555555').pack(anchor='w')
    choices.bind('<<ComboboxSelected>>', describe)

    def poll():
        try:
            kind, message = events.get_nowait()
            busy[0] = False
            choices.configure(state='readonly')
            status.set(message)
            if kind == 'error':
                messagebox.showerror('Community patch review', message)
        except queue.Empty:
            pass
        window.after(100, poll)

    def close():
        if busy[0]:
            status.set('A scenario operation is still running. Wait for it to finish before closing.')
            return
        save_draft()
        try:
            game.close()
        except Exception as error:
            status.set('Could not close the review game: ' + str(error))
            return
        window.destroy()
    window.protocol('WM_DELETE_WINDOW', close)
    describe()
    poll()
    window.mainloop()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', type=Path, required=True)
    arguments = parser.parse_args()
    panel(arguments.bundle)
