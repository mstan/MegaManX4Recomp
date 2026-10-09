"""Inspect the real native launcher's declared camera-option room offers."""
import argparse
import copy
import hashlib
import json
import subprocess
import time
from pathlib import Path

from test_mmx4_coop_netplay import prepare_peer, reserve_ports, require, sha256_file


def qualify_launch_options(args, output, offers):
    """Exercise the actual launcher commit with local recorded room messages.

    This checks the engine's room-to-launch path without contacting a lobby
    service or starting an Internet match. Valid cases stop at tick-0 admission.
    """
    results = []
    for name, choice, corruption in (('host-unified', 'unified', None),
            ('host-split', 'split', None), ('bad-fingerprint', 'split', 'fingerprint'),
            ('undeclared-option', 'split', 'option')):
        case = output / name
        case.mkdir()
        args.cameras = 'split' if choice == 'unified' else 'unified'
        ports, reservations = reserve_ports()
        peer = prepare_peer(args, case, 1, ports, False, 1296987000)
        state = peer.directory / 'mods/state.toml'
        previous = sha256_file(state)
        caps = copy.deepcopy(next(row['answer']['start']['match_caps'] for row in offers
                                  if row['choice'] == choice))
        caps.update(relay='host', input_delay=2, session_bios='openbios', guest_memcard=False)
        if corruption == 'fingerprint':
            caps['mod_plan_fp'] = '0' * 64
        elif corruption == 'option':
            caps['mods'][0]['feats'] = 'coop=cameras~invalid'
            canonical = 'psx-trusted-settings-v1\nmmx4.coop\n0.0.1\ncoop\ncameras=invalid\n'
            caps['mod_plan_fp'] = hashlib.sha256(canonical.encode()).hexdigest()
        common = dict(lobby_id='private-camera-qualification', session_id=1296987000,
            host_player_id='private-host', player_count=2, max_slots=2,
            host_endpoint=f'127.0.0.1:{ports[0]}', guest_endpoint=f'127.0.0.1:{ports[1]}',
            match_caps=caps, slots=[dict(slot=0,player_id='private-host',ready=True),
                                   dict(slot=1,player_id='private-guest',ready=True)])
        record = dict(handoff_player_id='private-guest', handoff_server_ip='127.0.0.1',
            handoff_seat=dict(common,op='joined',local_slot=1),
            handoff_room=dict(common,op='lobby_update',all_ready=True),
            handoff_launch=dict(common,op='launch',transport='sfu',
                                relay_endpoint=f'127.0.0.1:{ports[0]}'))
        launch = peer.directory / 'launch.json'
        launch.write_text(json.dumps(record), encoding='utf-8')
        peer.environment['RECOMP_NETPLAY_LAUNCH'] = str(launch)
        peer.argv = peer.argv[:peer.argv.index('--netplay')] + ['--launcher','--debug-port',str(peer.tcp)]
        status = Path(str(launch) + '.status')
        result = dict(name=name, host_cameras=choice, offline_cameras=args.cameras,
                      argv=peer.argv, status='running')
        try:
            for reservation in reservations:
                reservation.close()
            reservations = []
            peer.launch()
            deadline = time.monotonic() + 20
            while not status.exists() and peer.running() and time.monotonic() < deadline:
                time.sleep(.1)
            require(status.is_file(), f'Native launcher produced no handoff result: {peer.log}')
            result['handoff_result'] = json.loads(status.read_text(encoding='utf-8'))
            accepted = result['handoff_result'].get('ok') is True
            require(accepted == (corruption is None),
                    f'Native launcher incorrectly accepted/rejected {name}: {result}')
            result['source_state_unchanged'] = sha256_file(state) == previous
            require(result['source_state_unchanged'], 'Host settings replaced persisted offline preferences')
            if accepted:
                deadline = time.monotonic() + 10
                while time.monotonic() < deadline:
                    text = peer.log.read_text(errors='replace')
                    if 'netplay waiting for peer START + tick-0 admit' in text:
                        break
                    require(peer.running(), f'Accepted native launch exited early: {peer.log}')
                    time.sleep(.1)
                else:
                    raise AssertionError('Accepted native launch never reached the network admission gate')
                result['native_profile_commit'] = 'title netplay profile mmx4-coop' in text
                require(result['native_profile_commit'], 'Native title profile did not commit before admission')
            result['status'] = 'passed'
        finally:
            peer.stop()
            for reservation in reservations:
                reservation.close()
            result['returncode'] = peer.process.poll() if peer.process else None
            result['log_tail'] = peer.log.read_text(errors='replace').splitlines()[-12:]
            results.append(result)
            (output / 'launch-checks.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--report-dir', type=Path, required=True)
    parser.add_argument('--launch-checks', action='store_true')
    args = parser.parse_args()
    args.game_root = Path(__file__).resolve().parents[1]
    args.config = args.game_root / 'game.toml'
    args.exe, args.bios, args.disc = args.exe.resolve(), args.bios.resolve(), args.disc.resolve()
    args.frontend, args.delay, args.seed = 'headless-opengl', 2, 0
    output = args.report_dir.resolve()
    output.mkdir(parents=True, exist_ok=False)
    report = dict(status='running', executable_sha256=sha256_file(args.exe), offers=[],
                  scope='Actual native room-offer publisher; not room-start adoption or transport qualification')
    try:
        for choice in ('unified', 'split'):
            args.cameras = choice
            case = output / choice
            case.mkdir()
            ports, reservations = reserve_ports()
            peer = prepare_peer(args, case, 0, ports, False, 0)
            for reservation in reservations:
                reservation.close()
            state = peer.directory / 'mods/state.toml'
            previous = sha256_file(state)
            request = peer.directory / 'identity.json'
            request.write_text(json.dumps(dict(handoff_query='identity', handoff_disc=str(args.disc))),
                               encoding='utf-8')
            argv = peer.argv[:peer.argv.index('--netplay')] + ['--netplay-query', str(request)]
            with peer.log.open('wb') as log:
                process = subprocess.Popen(argv, cwd=peer.directory, env=peer.environment,
                                           stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                                           creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
                try:
                    code = process.wait(timeout=45)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    process.wait(timeout=5)
                    raise
            require(code == 0, f'Native room offer query failed for {choice}: {peer.log}')
            answer = json.loads(Path(str(request) + '.answer').read_text(encoding='utf-8'))
            offer = dict(choice=choice, answer=answer, argv=argv,
                         source_state_unchanged=sha256_file(state) == previous)
            report['offers'].append(offer)
            require(answer.get('disc_ok') is True and 'start' in answer,
                    f'Native publisher did not produce an eligible start offer: {answer}')
            caps = answer['start']['match_caps']
            package = next((item for item in caps.get('mods', []) if item['id'] == 'mmx4.coop'), None)
            require(package is not None and package['feats'] == f'coop=cameras~{choice}',
                    f'Native publisher omitted the declared camera selection: {caps}')
            canonical = f"psx-trusted-settings-v1\nmmx4.coop\n{package['ver']}\ncoop\ncameras={choice}\n"
            expected = hashlib.sha256(canonical.encode()).hexdigest()
            require(caps.get('mod_plan_fp') == expected,
                    f'Native publisher settings identity is missing or inconsistent: {caps}')
            require(offer['source_state_unchanged'], 'Room query altered persisted offline selections')
        require(report['offers'][0]['answer']['start']['match_caps']['mod_plan_fp'] !=
                report['offers'][1]['answer']['start']['match_caps']['mod_plan_fp'],
                'Unified and Split room offers did not identify different agreed settings')
        if args.launch_checks:
            report['launch_checks'] = qualify_launch_options(args, output, report['offers'])
            report['scope'] = ('Actual native offer publisher and recorded-room launcher/profile commit; '
                               'valid cases reach tick-0 admission; not a live Internet lobby test')
        report['status'] = 'passed'
    except Exception as error:
        report.update(status='failed', error=f'{type(error).__name__}: {error}')
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: report.get(key) for key in ('status', 'error')}), flush=True)
    return 0 if report['status'] == 'passed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
