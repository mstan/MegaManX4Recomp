"""Prepared local Complete Story review; private profiles and saved observations.

The six approaches bypass prior clears, never boss victory/movie-complete flags.
Uses the existing damage-multiplier package, not enemy-HP writes.
"""
import argparse
import hashlib
import json
import queue
from pathlib import Path
import threading
import time

from coop_debug import Runtime, capture_pair
from test_mmx4_complete_story import StoryCheck, PLAY


CASES = [
    dict(id='zero-colonel', campaign=1, slot=1, title='Zero: Colonel + Colonel movie',
         steps='Resume with Enter or Options. Hit Colonel with Z / Square. Advance '
         'dialogue with X / Cross. Watch the Colonel movie (Start can skip), then '
         'answer the save prompts. Expect the stage board, still Zero\'s campaign.'),
    dict(id='x-fortress', campaign=0, slot=2, title='X: Double, then Iris + aftermath movie',
         steps='Resume, move Right into the boss doors, then advance dialogue. '
         'Defeat Double; the next encounter loads normally. Move Right again and '
         'fight Iris. Open her shield before hitting the exposed core; invulnerable '
         'phases still apply. Expect the Iris aftermath movie once, then normal play.'),
    dict(id='x-sigma', campaign=0, slot=3, title='X: Zero/Sigma flashback before Sigma',
         steps='Resume and move Right. Advance the native Sigma dialogue with X / '
         'Cross. Expect the Zero/Sigma flashback once (Start can skip), followed by '
         'normal stage loading, boss introduction and recovered controls.'),
    dict(id='x-colonel', campaign=0, slot=4, title='X: Colonel + added Colonel movie',
         steps='Resume. Shoot Colonel with Z / Square. Advance dialogue. Expect '
         'the added Colonel movie once, then native save prompts and the stage '
         'board, still X\'s campaign.'),
    dict(id='zero-fortress', campaign=1, slot=5, title='Zero: Iris, then Double',
         steps='Resume, move Right through the doors and advance dialogue. Fight '
         'Iris, opening her shield before hitting her core. After the second '
         'encounter loads, move Right again and defeat Double. Expect the original '
         'Iris aftermath movie once, then normal play. No third encounter.'),
    dict(id='zero-sigma', campaign=1, slot=6, title='Zero: existing Sigma movie route',
         steps='Resume and move Right. Advance Sigma dialogue. Expect the original '
         'Zero/Sigma flashback once, then normal loading, introduction and controls.'),
]


def state(r, operation, slot):
    generation = r.request('savestate_status')['generation']
    r.request('savestate', op=operation, slot=slot)
    deadline = time.monotonic() + 35
    while time.monotonic() < deadline:
        status = r.request('savestate_status')
        if status['generation'] != generation and not status['pending']:
            if not status['last_ok']:
                raise RuntimeError('Snapshot operation failed: ' + str(status))
            return
        time.sleep(.05)
    raise TimeoutError('Snapshot operation did not finish; skip the movie and retry.')


def pause(s):
    s.r.release()
    if s.r.read(PLAY, 2) != bytes([6, 2]):
        s.pulse(0xfff7, 0, .13)
    s.wait('review-paused', lambda q: q['mode'][:2] == [6, 2], 15)
    time.sleep(.5)


def resume(s):
    # Simultaneous menus can leave either hero owning an open panel. Close
    # each with separate Start edges; one owner's input does not close both.
    for attempt in range(8):
        if s.r.read(PLAY, 2) == bytes([6, 0]):
            return
        s.pulse(0xfff7, attempt % 2, .15)
        time.sleep(.4)
    raise RuntimeError('Native pause panels did not close')


def prepare(r, profile, campaign):
    r.request('turbo', enabled=1)
    sleep = time.sleep
    time.sleep = lambda duration: sleep(duration * .15)
    try:
        r.boot(timeout=180, campaign=campaign)
    finally:
        time.sleep = sleep
    s = StoryCheck(r, profile, campaign, True, True, [])
    r.request('turbo', enabled=1)
    prepared = []
    try:
        # Native four-clear route -> actual Colonel admission and dialogue.
        r.write(PLAY + 0x59, 15)
        r.write(PLAY + 0x5f, 3)
        s.word(PLAY, 3)
        s.wait('memorial-playable', lambda q: q['stage'] == 9 and s.playable(q), 120, True)
        r.fixture(5, 3)
        for seat in (0, 1):
            r.input(0xffdf, seat)
        s.wait('colonel-admission', lambda q: any(
            e['type'] == 69 and e['hp'] == 48 for e in q['enemies']), 45, True)
        r.release()
        s.wait('colonel-controls', s.playable, 60, True)
        case = next(c for c in CASES if c['campaign'] == campaign and c['id'].endswith('colonel'))
        r.request('turbo', enabled=0)
        pause(s)
        state(r, 'save', case['slot'])
        capture_pair(r, profile / (case['id'] + '-ready.png'))
        prepared.append(dict(case=case, observed=s.sample('prepared-' + case['id'])))

        for kind, stage, checkpoint in (('fortress', 11, 1), ('sigma', 12, 10)):
            # Resume the native pause before using the ordinary stage loader.
            resume(s)
            s.wait('review-resumed', s.playable, 20)
            r.write(PLAY + 0x59, 255)
            r.request('turbo', enabled=1)
            s.load(stage, 0, checkpoint)
            r.fixture(5, 3)
            r.request('turbo', enabled=0)
            case = next(c for c in CASES if c['campaign'] == campaign and c['id'].endswith(kind))
            pause(s)
            state(r, 'save', case['slot'])
            capture_pair(r, profile / (case['id'] + '-ready.png'))
            prepared.append(dict(case=case, observed=s.sample('prepared-' + case['id'])))
        (profile / ('prepared-' + str(campaign) + '.json')).write_text(json.dumps(prepared, indent=2))
    finally:
        r.request('turbo', enabled=0)
        r.release()


def verify_states(r, profile):
    observations = []
    # Cross-campaign restoration must restore the native and host co-op roster.
    for case in CASES:
        state(r, 'load', case['slot'])
        s = StoryCheck(r, profile, case['campaign'], True, True, [])
        row = s.sample('restored-' + case['id'])
        assert row['mode'][:2] == [6, 2] and row['campaign'] == case['campaign'], row
        assert [p['character'] for p in row['players']] == [case['campaign'], case['campaign'] ^ 1], row
        assert all(p['active'] and p['hp'] == 32 for p in row['players']), row
        observations.append(row)
    state(r, 'load', CASES[0]['slot'])
    (profile / 'state-restore-check.json').write_text(json.dumps(observations, indent=2))


def verify_damage(r, profile):
    """Only native inputs damage Colonel: no enemy-HP or victory writes."""
    results = []
    for seat in (0, 1):
        state(r, 'load', CASES[0]['slot'])
        s = StoryCheck(r, profile, 1, True, True, [])
        resume(s)
        s.wait('damage-review-resumed', s.playable, 20)
        observed = []
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline:
            row = s.sample('one-shot-seat-' + str(seat))
            bosses = [e for e in row['enemies'] if e['type'] == 69]
            if not bosses:
                raise AssertionError(('Colonel disappeared before observed native defeat', row))
            boss = bosses[0]
            if not observed or observed[-1] != boss['hp']:
                observed.append(boss['hp'])
            if boss['hp'] == 0 and boss['state'][0] == 2:
                assert observed == [48, 0], observed
                results.append(dict(seat=seat + 1, character=row['players'][seat]['character'],
                                    hp_changes=observed, native_defeat=True, enemy_hp_writes=False))
                break
            player = row['players'][seat]
            direction = 0xffdf if boss['x'] >= player['x'] else 0xff7f
            s.pulse(direction, seat, .1 if abs(boss['x'] - player['x']) > 60 else .025)
            s.pulse(0x7fff, seat, .12)
            time.sleep(.1)
        else:
            raise AssertionError(('Native one-shot damage not established', seat, observed))
    state(r, 'load', CASES[0]['slot'])
    (profile / 'one-shot-check.json').write_text(json.dumps(results, indent=2))
    return results


def panel(r, profile, *, harness=None):
    import tkinter as tk
    from tkinter import ttk
    identity = hashlib.sha256((profile / 'MegaManX4Recomp.exe').read_bytes()).hexdigest()
    path = profile / 'review-notes.json'
    records = json.loads(path.read_text()) if path.exists() else {}
    current = records.setdefault(identity, {})
    root = tk.Tk()
    if harness is not None:
        root.withdraw()
    root.title('Mega Man X4 - Complete Story QA')
    root.geometry('730x650')
    frame = ttk.Frame(root, padding=14)
    frame.pack(fill='both', expand=True)
    selected = tk.StringVar(value=CASES[0]['title'])
    steps = tk.StringVar()
    status = tk.StringVar(value='Zero / Colonel is paused and ready. Click the game, then Enter/Options.')
    controls = tk.StringVar(value='Physical input check pending: while paused, briefly hold Right on '
        'keyboard, release, then Right on the DualSense. The live pad readings below must be separate.')
    pads = tk.StringVar(value='Waiting for live input readings...')
    ttk.Label(frame, text='Complete Story enabled | 2P Separate cameras | Damage x255 | QA invulnerability',
              wraplength=690).pack(fill='x')
    ttk.Combobox(frame, textvariable=selected, values=[c['title'] for c in CASES],
                 state='readonly').pack(fill='x', pady=10)
    ttk.Label(frame, textvariable=steps, wraplength=690).pack(fill='x', pady=6)
    ttk.Label(frame, text='P1 keyboard: arrows move, Z fire/saber, X jump/confirm, S dash, '
              'Enter pause/skip. P2 DualSense: D-pad/stick, Square fire/saber, Cross jump/confirm, '
              'Circle dash, Options pause. Click the game for keyboard input.', wraplength=690).pack(fill='x', pady=8)
    ttk.Label(frame, textvariable=controls, wraplength=690).pack(fill='x', pady=4)
    ttk.Label(frame, textvariable=pads, wraplength=690).pack(fill='x', pady=4)
    note = tk.Text(frame, height=6, wrap='word')
    note.pack(fill='both', expand=True, pady=8)
    note.insert('1.0', current.get(CASES[0]['id'], {}).get('notes', ''))
    active_note = CASES[0]['id']
    loaded_case = CASES[0]['id']

    def save():
        path.write_text(json.dumps(records, indent=2) + '\n')

    def choice():
        return next(c for c in CASES if c['title'] == selected.get())

    def refresh(*_):
        nonlocal active_note
        previous = current.setdefault(active_note, dict(result='Unrated'))
        previous['notes'] = note.get('1.0', 'end').strip()
        save()
        case = choice()
        active_note = case['id']
        steps.set(case['steps'])
        note.delete('1.0', 'end')
        note.insert('1.0', current.get(case['id'], {}).get('notes', ''))
        status.set('Selected test. Use Start / Restart to load its paused approach.')

    def record(result):
        case = choice()
        if case['id'] != loaded_case:
            status.set('Load the selected approach before recording its result.')
            return
        current[case['id']] = dict(result=result, notes=note.get('1.0', 'end').strip(),
                                   recorded_utc=time.time())
        save()
        status.set(case['title'] + ': ' + result + ' saved locally.')

    busy = False
    requests = queue.Queue()
    events = queue.Queue()
    stopped = threading.Event()

    # A single worker owns every socket operation, including live pad polling.
    # Tk must keep processing events even if the game stops answering TCP.
    def connection_worker():
        while not stopped.is_set():
            try:
                case = requests.get(timeout=.3)
            except queue.Empty:
                case = None
            try:
                if case is not None:
                    state(r, 'load', case['slot'])
                    r.release()
                    events.put(('loaded', case['id']))
                else:
                    events.put(('pads', (r.request('pad_status'), r.read(PLAY, 2))))
            except Exception as error:
                events.put(('load-error' if case is not None else 'connection-error', str(error)))
    threading.Thread(target=connection_worker, daemon=True).start()

    def restart():
        nonlocal busy
        if busy:
            return
        busy = True
        case = choice()
        status.set('Restoring ' + case['title'] + '...')
        requests.put(case)

    seen = set()
    samples = []
    def poll():
        nonlocal busy, loaded_case
        while True:
            try:
                event, data = events.get_nowait()
            except queue.Empty:
                break
            if event == 'loaded':
                busy = False
                loaded_case = data
                status.set('Approach restored and paused. Click the game, then Enter / Options.')
                continue
            if event == 'load-error':
                busy = False
                status.set('Restore failed; the panel remains available. ' + data)
                continue
            if event == 'connection-error':
                pads.set('Game is not answering yet. The selector and notes remain available.')
                continue
            pad, mode = data
            values = tuple(int(pad['slot' + str(i)]['buttons'], 16) for i in (0, 1))
            pads.set(f'Live pads: P1 {values[0]:04X} | P2 {values[1]:04X} | '
                     f'DualSense port {"connected" if pad["slot1"]["connected"] else "DISCONNECTED"}')
            if pad['override'] == -1 and pad['override_p2'] == -1 and mode == bytes([6, 2]):
                if values in ((0xffdf, 0xffff), (0xffff, 0xffdf)):
                    seen.add(0 if values[0] == 0xffdf else 1)
                    samples.append(dict(buttons=list(values), overrides=[-1, -1], time_utc=time.time()))
                    if seen == {0, 1}:
                        controls.set('Verified this run: separate physical Right presses reached only P1 and only P2.')
                        current['physical_controls'] = dict(result='Pass', samples=samples[-8:])
                        save()
            if not pad['slot1']['connected']:
                controls.set('Connect the DualSense for P2, then verify separate Right presses while paused.')
        root.after(50, poll)

    row = ttk.Frame(frame)
    row.pack(fill='x', pady=8)
    ttk.Button(row, text='Start / Restart selected', command=restart).pack(side='left')
    for result in ('Pass', 'Fail', 'Deferred'):
        ttk.Button(row, text=result, command=lambda value=result: record(value)).pack(side='left', padx=4)
    ttk.Label(frame, textvariable=status, wraplength=690).pack(fill='x', pady=6)
    ttk.Label(frame, text='Notes stay in this QA folder. Prepared approaches skip earlier campaign clears; '
              'the fights, movies and continuations are native. Close the game normally when done.',
              wraplength=690).pack(fill='x')
    selected.trace_add('write', refresh)
    steps.set(CASES[0]['steps'])
    def close():
        stopped.set()
        current.setdefault(active_note, dict(result='Unrated'))['notes'] = note.get('1.0', 'end').strip()
        save()
        root.destroy()
    root.protocol('WM_DELETE_WINDOW', close)
    if harness is not None:
        harness(root, dict(restart=restart, selected=selected, status=status, close=close))
    root.after(50, poll)
    root.mainloop()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', type=Path, required=True)
    parser.add_argument('--port', type=int, default=18741)
    parser.add_argument('--prepare', type=int, choices=(0, 1))
    parser.add_argument('--verify-states', action='store_true')
    args = parser.parse_args()
    r = Runtime(args.port, isolated_inputs=args.prepare is not None,
                timeout=15 if args.prepare is not None or args.verify_states else 1.5)
    if args.prepare is not None:
        prepare(r, args.profile, args.prepare)
    elif args.verify_states:
        verify_states(r, args.profile)
    else:
        panel(r, args.profile)


if __name__ == '__main__':
    main()
