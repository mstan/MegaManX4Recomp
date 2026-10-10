"""Verify synchronized cheats and native door handoffs in two hidden peers.

Both peers must use the separate MMX4_COOP_NETPLAY_DEBUG build. Private saves,
loopback ports and owned PIDs follow the normal netplay harness. No RAM writes.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import time
from coop_netplay_debug import NetplayDevelopment
from test_mmx4_coop_netplay import Exercise, prepare_peer, reserve_ports, change_config, ValidationError


def tolerate_loading_reads(peer):
    """Read-only TCP requests may time out while native linking/loading owns
    the emulation thread. Keep actual exit/desync failures and mutation errors
    immediate; never retry or relax a simulation command or watchdog policy.
    """
    original=peer._request
    def request(cmd, *, allow_error=False, **fields):
        deadline=time.monotonic()+12
        while True:
            try:
                return original(cmd,allow_error=allow_error,**fields)
            except ValidationError as error:
                if cmd not in ('netplay_status','read_ram','mod_counters') or \
                        'emu busy or frozen' not in str(error) or \
                        not peer.running() or time.monotonic()>=deadline:
                    raise
                time.sleep(.05)
    peer._request=request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--config', type=Path, default=Path('game.toml'))
    parser.add_argument('--game-root', type=Path, default=Path.cwd())
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cameras', choices=('unified', 'split'), default='unified')
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0)
    parser.add_argument('--doors', action='store_true', help='Also traverse the P2-led Cyber door')
    parser.set_defaults(latency=0, jitter=0, seed=714, delay=2,
                        frontend='headless-opengl', boot_timeout=100, phase_timeout=35)
    args = parser.parse_args()
    for name in ('exe', 'bios', 'disc', 'config', 'game_root', 'output'):
        setattr(args, name, getattr(args, name).resolve())
    args.output.mkdir(parents=True, exist_ok=False)
    ports, reservations = reserve_ports()
    report = dict(status='running', development=True, ports=ports, campaign=args.campaign,
                  cameras=args.cameras, boot_modes=[], checkpoints=[], observations=[], controls=[])
    peers = []
    exercise = None
    try:
        session = int(time.time()) & 0xFFFFFFFF
        peers = [prepare_peer(args, args.output, seat, ports, False, session)
                 for seat in (0, 1)]
        for peer in peers:
            tolerate_loading_reads(peer)
            peer.environment['PATH'] = r'C:\msys64\mingw64\bin;' + os.environ.get('PATH', '')
            peer.environment['SDL_AUDIODRIVER'] = 'dummy'
            config=peer.directory/'game.toml'
            config.write_text(change_config(config.read_text(),'video','internal_resolution','native'))
        for reservation in reservations:
            reservation.close()
        reservations = []
        for peer in peers:
            peer.launch()
        report['pids'] = [peer.process.pid for peer in peers]
        exercise = Exercise(args, peers, report)
        exercise.boot(args.campaign)
        debug = NetplayDevelopment(ports[2], ports[3])
        debug.fixture(5, 3)
        debug.teleport(1, 160, 136)
        exercise.wait_ticks(12)
        exercise.wait_observation('P2 synchronized teleport', lambda s:
                                  150 <= s['p2']['x'] <= 170 and s['p2']['hp'] > 0)
        tick = min(peer.status()['tick'] for peer in peers)
        exercise.checkpoint('teleport/invulnerability CRC', tick, min_digests=2)
        debug.fixture(1, 6, 1, args.campaign)
        exercise.wait_observation('Cyber stage loaded', lambda s:
                                  s['stage'] == 6 and s['section'] == 1 and s['enrolled'] and
                                  s['mode'] == 6 and s['minor'] == 0 and
                                  not s['pause_suppressed'] and s['p2']['hp'] > 0 and
                                  all(s[p]['grounded'] and s[p]['action'] == 2 for p in ('p1', 'p2')))
        debug.fixture(3, 1)
        exercise.wait_observation('Cyber checkpoint loaded', lambda s:
                                  s['stage'] == 6 and s['enrolled'] and s['p1']['y'] > 2400 and
                                  not s['pause_suppressed'] and s['p2']['hp'] > 0)
        tick = min(peer.status()['tick'] for peer in peers)
        exercise.checkpoint('native load/checkpoint CRC', tick, min_digests=2)
        debug.fixture(2, 1, 24)
        exercise.wait_observation('P2 synchronized health', lambda s: s['p2']['hp'] == 24)
        debug.fixture(2, 1, 32)
        if args.doors:
            # Keep both inside Unified's 240-pixel separation bound. Door tile
            # centers are solid; use the observed native floor before the door.
            debug.teleport(0, 568, 2507 if args.campaign == 0 else 2506)
            debug.teleport(1, 600, 2506 if args.campaign == 0 else 2507)
            peers[1].input(0x20)
            try:
                exercise.wait_observation('P2 native Cyber door entered', lambda s:
                                          s['p2']['door'] != 0)
            finally:
                peers[1].input(0)
            exercise.wait_observation('P2 native Cyber door returned', lambda s:
                                      all(s[p]['active'] and s[p]['hp'] > 0 and
                                          s[p]['state'] == 1 and s[p]['grounded']
                                          for p in ('p1', 'p2')) and not s['pause_suppressed'])
            tick = min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('P2 door/handoff CRC', tick, min_digests=2)
        debug.fixture(1, 5, 0, args.campaign)
        exercise.wait_observation('Jet riders enrolled', lambda s:
                                  s['mode'] == 6 and s['minor'] == 0 and s['stage'] == 5 and s['enrolled'] and
                                  all(s[p]['active'] and s[p]['hp'] > 0 and
                                      s[p]['vehicle'] == 255 for p in ('p1', 'p2')))
        counts = []
        for peer in peers:
            actors = peer.read(0x80142F98, 32 * 0x30)
            counts.append(sum(actors[i * 0x30] != 0 and actors[i * 0x30 + 1] == 0x1B
                              for i in range(32)))
        report['jet_ready_actors'] = counts
        shared = []
        for peer in peers:
            counters = peer.request('mod_counters')['counters']
            shared.append(sum(c['count'] for c in counters if c['name'] == 'mmx4.coop.shared-ride-ready'))
        report['jet_shared_ready_callbacks'] = shared
        if any(count > 1 for count in counts) or not all(shared):
            raise AssertionError(f'Expected one shared READY presentation per peer: {counts}/{shared}')
        tick = min(peer.status()['tick'] for peer in peers)
        exercise.checkpoint('Jet shared Ready CRC', tick, min_digests=2)
        report['status'] = 'passed'
        print('Development netplay cheats: both peers agree after teleport and native loads', flush=True)
    except BaseException as error:
        report.update(status='failed', error=f'{type(error).__name__}: {error}')
        print(report['error'], flush=True)
        for peer in peers:
            try:
                report.setdefault('failure_states', []).append(peer.observe())
            except Exception:
                pass
        raise
    finally:
        if exercise:
            exercise.pool.shutdown(wait=True)
        for peer in peers:
            peer.stop()
        for reservation in reservations:
            reservation.close()
        (args.output / 'report.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
