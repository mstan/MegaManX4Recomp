"""Load original stages through the development plugin's native load fixture.

Requires an already-running private OpenGL development build, original disc,
and an active gameplay session. This is stage bring-up evidence, not a claim
of campaign completion. The fixture is disabled during netplay/release builds.
"""
import argparse
import json
from pathlib import Path
import struct
import time
from coop_debug import Runtime


def actor_summary(body):
    x, y = struct.unpack_from('<ii', body, 8)
    return dict(active=body[0], character=body[2], visible=body[3], state=body[4],
                action=body[5], x=x/65536, y=y/65536, hp=body[0x5c]&127,
                vehicle=body[0xc5], input=struct.unpack_from('<H', body, 0x7c)[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=14525)
    parser.add_argument('--stages', default='0,1,2,3,4,5,6,7,8,9,10,11,12')
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0,
                        help='Use a fresh native boot for each campaign')
    parser.add_argument('--section', type=int, choices=(0, 1), default=0)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--boot', action='store_true')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    runtime = Runtime(args.port)
    if args.boot:
        runtime.boot(campaign=args.campaign)
    results = []
    failed = False
    try:
        for campaign in (args.campaign,):
            for stage in map(int, args.stages.split(',')):
                result = dict(campaign=campaign, stage=stage, section=args.section,
                              method='native-load fixture; short independent-input probe')
                try:
                    runtime.release()
                    runtime.fixture(1, stage, args.section, campaign)
                    players = runtime.wait_playable(90, advance_dialogue=True)
                    result['spawn'] = [actor_summary(p) for p in players]
                    if [p[2] for p in players] != [campaign, campaign ^ 1]:
                        raise AssertionError('Incorrect character pairing')
                    if players[0][0xc5] == 0xff and players[1][0xc5] != 0xff:
                        raise AssertionError('Native Ride Chaser spawn did not mount both riders')
                    time.sleep(.2)
                    before = runtime.players()
                    runtime.input(0xbfdf, 1)  # Right + Cross/jump, P2 only.
                    time.sleep(.25)
                    held = runtime.players()
                    runtime.release()
                    result['held'] = [actor_summary(p) for p in held]
                    if held[1][8:16] == before[1][8:16] and not held[1][0x7c:0x82].strip(b'\0'):
                        raise AssertionError('P2 showed neither movement nor input')
                    name = f'campaign-{campaign}-stage-{stage:02d}-{args.section}'
                    runtime.screenshot(args.output/(name+'.png'))
                    result['screenshot'] = name+'.png'
                    result['status'] = 'loaded-and-input-observed'
                except Exception as error:
                    result['status'] = 'failed'
                    result['error'] = str(error)
                    failed = True
                finally:
                    runtime.release()
                    results.append(result)
                    (args.output/'results.json').write_text(json.dumps(results, indent=2))
                    print(json.dumps(result), flush=True)
                if result['status'] == 'failed':
                    # A failed native load cannot safely seed the next case.
                    raise SystemExit(1)
    finally:
        runtime.release()
    raise SystemExit(1 if failed else 0)


if __name__ == '__main__':
    main()
