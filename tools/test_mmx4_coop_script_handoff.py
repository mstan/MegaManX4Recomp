"""Owned, isolated original-disc Jungle door/conversation qualification."""
import argparse
import json
import time
from pathlib import Path

from coop_debug import Runtime
from test_mmx4_coop_doors import run_case
from test_mmx4_coop_netplay import change_config, prepare_peer, reserve_ports, sha256_file


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--report-dir', type=Path, required=True)
    parser.add_argument('--campaign', type=int, choices=(0, 1), required=True)
    parser.add_argument('--owner', type=int, choices=(0, 1))
    parser.add_argument('--survivor', action='store_true')
    args = parser.parse_args()
    args.game_root = Path(__file__).resolve().parents[1]
    args.config = args.game_root / 'game.toml'
    args.exe, args.bios, args.disc = args.exe.resolve(), args.bios.resolve(), args.disc.resolve()
    args.cameras, args.frontend, args.delay, args.seed = 'unified', 'headless-opengl', 2, 0
    output = args.report_dir.resolve()
    output.mkdir(parents=True, exist_ok=False)
    results = []
    for survivor, owner in ([(args.survivor, args.owner)] if args.owner is not None else
                            [(False, 0), (False, 1), (True, 0), (True, 1)]):
        case = output / f'owner-{owner}-survivor-{int(survivor)}'
        case.mkdir()
        ports, reservations = reserve_ports()
        peer = prepare_peer(args, case, 0, ports, False, 0)
        config = peer.directory / 'game.toml'
        source = change_config(config.read_text(encoding='utf-8'), 'video', 'aspect_ratio', '4:3')
        config.write_text(source, encoding='utf-8')
        state = peer.directory / 'mods/state.toml'
        state.write_text(state.read_text(encoding='utf-8').replace('enabled = false', 'enabled = true'),
                         encoding='utf-8')
        peer.argv = peer.argv[:peer.argv.index('--netplay')] + ['--debug-port', str(peer.tcp)]
        record = dict(executable_sha256=sha256_file(args.exe), argv=peer.argv)
        runtime = Runtime(peer.tcp)
        try:
            for reservation in reservations:
                reservation.close()
            reservations = []
            peer.launch()
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                try:
                    runtime.request('mod_counters')
                    break
                except OSError:
                    time.sleep(.1)
            runtime.boot(campaign=args.campaign)
            record.update(run_case(runtime, args.campaign, owner, survivor, case))
            (case / 'result.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
            record['native_text'] = runtime.read(0x8013BC28, 0x18).hex()
            record['native_pad'] = runtime.read(0x80166C08, 6).hex()
            record['mod_counters'] = runtime.request('mod_counters')
            record['runtime_stats'] = runtime.request('dispatch_stats')
        except Exception as error:
            if 'status' in record:
                record['diagnostic_error'] = f'{type(error).__name__}: {error}'
                if record['status'] == 'passed':
                    record['status'] = 'failed'
            else:
                record.update(status='failed', error=f'{type(error).__name__}: {error}')
        finally:
            for reservation in reservations:
                reservation.close()
            peer.stop()
            results.append(record)
            (output / 'report.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({key: record.get(key) for key in ('name', 'status', 'error')}), flush=True)
        if record.get('status') != 'passed':
            return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
