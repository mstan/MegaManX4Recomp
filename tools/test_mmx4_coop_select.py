"""Check foot withdrawal/rejoin and bike withdrawal prohibition in a private run.

Requires an already-running co-op development build and original-disc boot.
Only native stage fixtures and controller inputs are written. HP, ammo, actor
positions and vehicle state are observed, never seeded by this check.
"""
import argparse
import json
from pathlib import Path
import time

from coop_debug import Runtime
from test_mmx4_coop_doors import snapshot


def ticks(runtime):
    for item in runtime.request('mod_counters')['counters']:
        if item['name'] == 'mmx4.coop.player-ticks':
            return item['count']
    return 0


def run_case(runtime, campaign, stage, output):
    record = dict(campaign=campaign, stage=stage,
                  method=('native load fixture; Select hold during bike sequence'
                          if stage == 5 else
                          'native load fixture; Select hold/release/tap'))
    try:
        runtime.release()
        runtime.fixture(1, stage, 0, campaign)
        runtime.wait_playable(60, advance_dialogue=True)
        record['before'] = snapshot(runtime)
        before = runtime.players()[1]
        record['ammo_before'] = list(before[0xa8:0xb8])
        initial_ticks = ticks(runtime)
        start = time.monotonic()
        runtime.input(0xfffe, 1)
        if stage == 5:
            deadline = start + 10
            while time.monotonic() < deadline:
                current = snapshot(runtime)
                if not all(p['active'] and p['hp'] for p in current['players']):
                    raise AssertionError('Select removed a rider during the bike sequence')
                if ticks(runtime) - initial_ticks >= 110:
                    break
                time.sleep(.025)
            else:
                raise TimeoutError('Bike sequence stopped advancing')
            record['after_hold'] = current
            record['hold_player_ticks_observed'] = ticks(runtime) - initial_ticks
            if any(b[0xc5] != 0xff for b in runtime.players()):
                raise AssertionError('Select unmounted a Ride Chaser')
            runtime.release()
            runtime.screenshot(output / f'campaign-{campaign}-stage-{stage}-hold-blocked.png')
            record['status'] = 'passed'
            return record
        deadline = start + 10
        while time.monotonic() < deadline:
            current = snapshot(runtime)
            if not current['players'][1]['active']:
                break
            time.sleep(.025)
        else:
            raise TimeoutError('P2 did not withdraw after Select hold')
        record['out'] = current
        record['hold_seconds_observed'] = time.monotonic() - start
        record['hold_player_ticks_observed'] = ticks(runtime) - initial_ticks
        if record['hold_player_ticks_observed'] < 80:
            raise AssertionError('P2 departed substantially before 90 native ticks')
        after = runtime.players()[1]
        if after[0x5c] != before[0x5c] or after[0xa8:0xb8] != before[0xa8:0xb8]:
            raise AssertionError('Withdrawal changed private HP or ammo')
        runtime.release()
        time.sleep(.15)
        # Move the remaining player using native controls, before rejoining.
        runtime.input(0xffdf, 0)
        time.sleep(.35)
        runtime.release()
        record['remaining_player_moves'] = snapshot(runtime)
        join_ticks = ticks(runtime)
        runtime.input(0xfffe, 1)
        time.sleep(.15)
        runtime.release()
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            current = snapshot(runtime)
            player = current['players'][1]
            if player['active'] and player['visible'] and player['state'] == 1 and ticks(runtime) > join_ticks:
                break
            time.sleep(.05)
        else:
            raise TimeoutError('P2 did not return to native control')
        record['returned'] = current
        after = runtime.players()[1]
        record['ammo_after'] = list(after[0xa8:0xb8])
        if after[0x5c] != before[0x5c] or after[0xa8:0xb8] != before[0xa8:0xb8]:
            raise AssertionError('Rejoin changed private HP or ammo')
        if not current['players'][0]['hp'] or current['lives'] != record['before']['lives']:
            raise AssertionError('Withdrawal/rejoin damaged the shared run')
        runtime.screenshot(output / f'campaign-{campaign}-stage-{stage}-returned.png')
        record['status'] = 'passed'
    except Exception as error:
        record['status'] = 'failed'
        record['error'] = str(error)
        try:
            record['failure_state'] = snapshot(runtime)
        except Exception as diagnostic_error:
            record['diagnostic_error'] = str(diagnostic_error)
    finally:
        runtime.release()
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=14725)
    parser.add_argument('--campaign', type=int, choices=(0, 1), required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    runtime = Runtime(args.port)
    results = []
    for stage in (1, 5):
        record = run_case(runtime, args.campaign, stage, output)
        results.append(record)
        (output / 'results.json').write_text(json.dumps(results, indent=2))
        print(json.dumps(record), flush=True)
        if record['status'] != 'passed':
            raise SystemExit(1)


if __name__ == '__main__':
    main()
