"""Exercise original Jungle boss-door traversal in a private development run.

Requires an already running co-op executable and original disc. Use a fresh
controller-only boot for each campaign. The bounded native load/checkpoint
fixture seeds the antechamber; optional player fatal requests test survivors.
No actor positions, door state, boss HP or completion flags are written.
"""
import argparse
import json
from pathlib import Path
import struct
import time

from coop_debug import Runtime


def body_state(body):
    x, y = struct.unpack_from('<ii', body, 8)
    return dict(active=body[0], visible=body[3], character=body[2], state=body[4], action=body[5],
                hp=body[0x5c] & 127, x=x/65536, y=y/65536,
                door=body[0xc4], script=body[0xc0])


def snapshot(runtime):
    raw = runtime.read(runtime.diagnostic(), 0x464)
    if struct.unpack_from('<I', raw, 0x1c)[0] != 1:
        raise RuntimeError('This check requires canonical diagnostic snapshots')
    if struct.unpack_from('<I', raw, 12)[0]:
        raise RuntimeError('Co-op reported a failure')
    play = raw[0x400:0x464]
    return dict(frame=runtime.request('mod_counters')['frame'],
                mode=list(play[:4]), stage=play[12], section=play[13],
                checkpoint=play[0x1d], campaign=play[0x43], lives=play[0x44],
                blocked=play[0x1c],
                players=[body_state(raw[0x300:0x3e4]),
                         body_state(raw[0x100:0x1e4])],
                camera=list(struct.unpack('<12h', runtime.read(0x801419b0, 24))))


def run_case(runtime, campaign, owner, survivor, output):
    name = f'campaign-{campaign}-owner-{owner}-survivor-{int(survivor)}'
    record = dict(name=name, owner=owner, survivor=survivor,
                  method='native checkpoint fixture; controller-only traversal')
    events = []
    try:
        runtime.release()
        runtime.fixture(1, 1, 1, campaign)
        runtime.wait_playable(60, advance_dialogue=True)
        runtime.fixture(3, 5)
        runtime.wait_playable(60, advance_dialogue=True)
        if survivor:
            runtime.fixture(2, owner ^ 1, 0x80)
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                state = snapshot(runtime)
                if state['players'][owner ^ 1]['state'] == 3:
                    break
                time.sleep(.05)
            else:
                raise TimeoutError('Original partner death did not finish')
        record['before'] = snapshot(runtime)
        before_lives = record['before']['lives']
        runtime.input(0xffdf, owner)
        entered = False
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            state = snapshot(runtime)
            events.append(state)
            player = state['players'][owner]
            if not player['hp'] or player['state'] >= 2:
                raise AssertionError('Door owner died during traversal')
            if player['door']:
                entered = True
                runtime.release()
            if entered and not player['door']:
                break
            time.sleep(.05)
        else:
            raise TimeoutError('Native door did not enter and finish')
        runtime.release()
        # Partner transport is committed on the next native player tick.
        time.sleep(.2)
        after = snapshot(runtime)
        record['after'] = after
        if after['campaign'] != campaign or after['lives'] != before_lives:
            raise AssertionError('Door altered campaign identity or shared lives')
        partner = after['players'][owner ^ 1]
        if survivor:
            if partner['hp'] or partner['state'] != 3:
                raise AssertionError('Door revived the fallen partner')
        # The next native boss conversation must finish before another load;
        # changing loader mode during that script leaves its gates unfinished.
        deadline = time.monotonic() + 60
        while time.monotonic() < deadline:
            state = snapshot(runtime)
            if not state['blocked'] and not state['players'][owner]['script']:
                break
            runtime.input(0xbfff, owner)
            time.sleep(.1)
            runtime.release()
            time.sleep(.1)
        else:
            raise TimeoutError('Native boss conversation did not release play')
        record['after_conversation'] = snapshot(runtime)
        if not survivor:
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                state = snapshot(runtime)
                partner = state['players'][owner ^ 1]
                if partner['active'] and partner['visible']:
                    break
                time.sleep(.05)
            else:
                raise TimeoutError('Script passenger did not return')
            record['after_return'] = state
            if not partner['hp'] or abs(partner['x'] - state['players'][owner]['x']) > 40:
                raise AssertionError('Living partner did not return beside the owner')
        runtime.screenshot(output / (name + '.png'))
        record['status'] = 'passed'
    except Exception as error:
        record['status'] = 'failed'
        record['error'] = str(error)
        try:
            record['failure_state'] = snapshot(runtime)
            runtime.screenshot(output / (name + '-failed.png'))
        except Exception as diagnostic_error:
            record['diagnostic_error'] = str(diagnostic_error)
    finally:
        runtime.release()
        (output / (name + '-events.json')).write_text(json.dumps(events, indent=2))
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=14725)
    parser.add_argument('--campaign', type=int, choices=(0, 1), required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--boot', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    runtime = Runtime(args.port)
    if args.boot:
        runtime.boot(campaign=args.campaign)
    results = []
    for survivor, owner in ((False, 0), (False, 1), (True, 0), (True, 1)):
        record = run_case(runtime, args.campaign, owner, survivor, output)
        results.append(record)
        (output / 'results.json').write_text(json.dumps(results, indent=2))
        print(json.dumps(record), flush=True)
        if record['status'] != 'passed':
            raise SystemExit(1)


if __name__ == '__main__':
    main()
