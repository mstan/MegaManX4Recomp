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
        deadline=time.monotonic()+45
        while True:
            try:
                return original(cmd,allow_error=allow_error,**fields)
            except ValidationError as error:
                if cmd not in ('netplay_status','read_ram','mod_counters') or \
                        'emu busy or frozen' not in str(error) or \
                        not peer.running() or time.monotonic()>=deadline:
                    raise
                time.sleep(.05)
            except TimeoutError:
                # A read can also miss the socket deadline during native
                # loading. Retrying read-only queries cannot double-apply a
                # developer command; retain the same bounded overall deadline.
                if cmd not in ('netplay_status','read_ram','mod_counters') or \
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
    parser.add_argument('--jet-only',action='store_true',help='Check both native Jet rider entrance animations and composed frames')
    parser.add_argument('--personal-menus-only',action='store_true',help='Check character menus, tank healing and native weapon selection')
    parser.add_argument('--weapon-awards-only',action='store_true',help='Native award initializer, personal awards and shared P1 save flow')
    parser.add_argument('--separate-etanks',action='store_true',help='Enable private E-tank contents in both test peers')
    parser.add_argument('--individual-pausing',action='store_true',help='Use legacy one-owner pausing')
    parser.add_argument('--boss-return-only',action='store_true',help='Check partner arrival during boss-fight movement, jump and Nova')
    parser.add_argument('--train-only',action='store_true',help='Qualify both Slash Split background panes and CRCs')
    parser.add_argument('--latch-only',action='store_true',help='Qualify the native Mushroom latch and matching peer state')
    parser.add_argument('--stairs-only',action='store_true',help='Qualify independent Mushroom stair animation with the partner ahead')
    parser.add_argument('--nova-only',action='store_true',help='Qualify native Nova Strike camera independence')
    parser.add_argument('--dragon-only',action='store_true',help='Inspect intro dragon placement in two independent views')
    parser.add_argument('--wrap-only',action='store_true',help='Inspect distant player sprites around the 2048px GPU boundary')
    parser.add_argument('--jungle-exit-only',action='store_true',help='Check a distant P2-led Jungle section exit')
    parser.add_argument('--jungle-first-dead',action='store_true',help='With --jungle-exit-only, keep genuinely dead P1 dead')
    parser.add_argument('--expect-sprite-wrap',action='store_true',help='Capture the old build without the distant-sprite correction')
    parser.add_argument('--expect-duplicated-dragon',action='store_true',help='Capture the old screen-relative intro boss')
    parser.add_argument('--frontend',choices=('headless-opengl','hidden-window'),default='headless-opengl')
    parser.add_argument('--boot-timeout',type=float,default=180,help='Bounded cold-runtime and native boot allowance')
    parser.add_argument('--phase-timeout',type=float,default=45)
    parser.add_argument('--expect-camera-fallback',action='store_true',help='Require the unfixed Nova camera failure in an A/B baseline')
    parser.set_defaults(latency=0, jitter=0, seed=714, delay=2,
                        frontend='headless-opengl')
    args = parser.parse_args()
    if args.nova_only or args.boss_return_only:args.ultimate_armor=True
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
            # Development packets deliberately use controller chords. In a
            # hidden SDL window the host save-menu chord otherwise consumes
            # a packet and pauses the match. Change only these private peers.
            (peer.directory/'settings.toml').write_text(
                '[hotkeys]\nrewind_pad = 0\nsave_state_menu_pad = 0\n'
                'fast_forward_pad = 0\nfast_forward_toggle_pad = 0\n')
        for reservation in reservations:
            reservation.close()
        reservations = []
        for peer in peers:
            peer.launch()
        report['pids'] = [peer.process.pid for peer in peers]
        exercise = Exercise(args, peers, report)
        exercise.boot(args.campaign)
        debug = NetplayDevelopment(ports[2], ports[3])
        if args.weapon_awards_only:
            from mmx4_netplay_runtime import NetplayRuntime
            from test_mmx4_coop_regressions import Scenarios
            scenario=Scenarios(NetplayRuntime(debug),args.output,args.campaign,clears=0)
            scenario.award_online=True
            scenario.save_root=peers[0].directory/'saves'
            def capture(label):
                for peer in peers:
                    path=args.output/('%s-p%d.png'%(label,peer.seat+1))
                    sequence=peer.request('present_shot_seq')['seq']
                    peer.request('present_shot',path=str(path))
                    deadline=time.monotonic()+12
                    while peer.request('present_shot_seq')['seq']==sequence:
                        if time.monotonic()>deadline:raise TimeoutError('Peer award capture pending')
                        time.sleep(.03)
            scenario.award_capture=capture
            try:
                report['weapon_awards']=scenario.award_presentations()
                tick=min(peer.status()['tick'] for peer in peers)
                exercise.checkpoint('Native weapon awards and shared save',tick,min_digests=2)
                report['render']=[peer.request('render_pass_stats') for peer in peers]
                assert all(v['verify_checks']>0 and not v['verify_mismatch'] for v in report['render']),report['render']
                report['status']='passed'
            finally:
                (args.output/'scenario-events.json').write_text(json.dumps(scenario.events,indent=2))
            return
        if args.personal_menus_only:
            from mmx4_netplay_runtime import NetplayRuntime
            from test_mmx4_coop_regressions import Scenarios
            from coop_personal_menu_scenario import personal_menus
            scenario=Scenarios(NetplayRuntime(debug),args.output,args.campaign)
            scenario.separate_etanks=args.separate_etanks
            scenario.simultaneous_menus=not args.individual_pausing
            def capture(label):
                paths=[]
                for seat,peer in enumerate(peers):
                    path=args.output/('%s-p%d.png'%(label,seat+1))
                    sequence=peer.request('present_shot_seq')['seq']
                    peer.request('present_shot',path=str(path))
                    deadline=time.monotonic()+12
                    while peer.request('present_shot_seq')['seq']==sequence:
                        if time.monotonic()>deadline:raise TimeoutError('Peer menu capture pending')
                        time.sleep(.03)
                    paths.append(path)
                return paths
            try:
                report['personal_menus']=personal_menus(scenario,capture)
                tick=min(peer.status()['tick'] for peer in peers)
                exercise.checkpoint('Character menus and E-tank ownership',tick,min_digests=2)
                report['status']='passed'
            finally:
                (args.output/'scenario-events.json').write_text(json.dumps(scenario.events,indent=2))
            return
        if args.boss_return_only:
            from mmx4_netplay_runtime import NetplayRuntime
            from test_mmx4_coop_regressions import Scenarios
            from coop_boss_return_scenario import boss_return
            scenario=Scenarios(NetplayRuntime(debug),args.output,args.campaign)
            def capture(label):
                for seat,peer in enumerate(peers):
                    peer.request('present_shot',path=str(args.output/('%s-p%d.png'%(label,seat+1))))
            try:
                report['boss_return']=boss_return(scenario,capture)
                tick=min(peer.status()['tick'] for peer in peers)
                exercise.checkpoint('Boss-fight uninterrupted partner arrivals',tick,min_digests=2)
                report['status']='passed'
            finally:
                (args.output/'scenario-events.json').write_text(json.dumps(scenario.events,indent=2))
            return
        if args.jet_only:
            from mmx4_netplay_runtime import NetplayRuntime
            from test_mmx4_coop_regressions import Scenarios
            scenario=Scenarios(NetplayRuntime(debug),args.output,args.campaign)
            def capture(label):
                # Capture the composed private texture banks, not raw VRAM.
                for seat,peer in enumerate(peers):
                    peer.request('present_shot',path=str(args.output/('%s-p%d.png'%(label,seat+1))))
            try:
                report['jet_entrance']=scenario.jet_intro(capture)
                tick=min(peer.status()['tick'] for peer in peers)
                exercise.checkpoint('Jet rider entrances',tick,min_digests=2)
                report['status']='passed'
            finally:
                (args.output/'scenario-events.json').write_text(json.dumps(scenario.events,indent=2))
            return
        if args.jungle_exit_only:
            from mmx4_netplay_runtime import NetplayRuntime
            from test_mmx4_coop_regressions import Scenarios
            from coop_jungle_exit_scenario import jungle_exit
            scenario=Scenarios(NetplayRuntime(debug),args.output,args.campaign)
            try:
                report['jungle_exit']=jungle_exit(scenario,args.jungle_first_dead)
                tick=min(peer.status()['tick'] for peer in peers)
                exercise.checkpoint('Jungle part2 arrival',tick,min_digests=2)
                report['status']='passed'
            finally:
                (args.output/'scenario-events.json').write_text(json.dumps(scenario.events,indent=2))
            return
        if args.wrap_only:
            from mmx4_netplay_runtime import NetplayRuntime
            runtime=NetplayRuntime(debug)
            runtime.fixture(5,3)
            runtime.fixture(1,1,0,args.campaign)
            runtime.wait_playable(45,advance_dialogue=True)
            # The recording is on the lower route after the waterfall. The
            # original checkpoint loads those camera bounds; teleporting
            # directly from checkpoint0 is clamped before the room loads.
            runtime.fixture(3,2)
            runtime.wait_playable(45,advance_dialogue=True)
            report['wrap_samples']=samples=[]
            for reverse in (0,1):
                runtime.teleport(reverse,1917,1467)
                runtime.teleport(reverse^1,3965,1338)
                exercise.wait_ticks(520)
                row=exercise.split_world_observation('wrap-'+str(reverse));samples.append(row)
                for peer in peers:
                    if args.frontend=='hidden-window':
                        peer.request('present_shot',path=str(args.output/f'wrap-{reverse}-peer{peer.seat}.png'))
                assert all(s['players'][p]['hp']>0 for s in row['peers'] for p in ('p1','p2')),row
                assert all(abs(s['cameras'][0]['x']-s['cameras'][1]['x'])>1800 for s in row['peers']),row
                for s in row['peers']:
                    culled=s['counters'].get('mmx4.coop.split-distant-sprite-culled',0)
                    assert bool(culled)!=args.expect_sprite_wrap,('Unexpected distant-sprite guard',culled)
                exercise.wait_ticks(10)
                if args.frontend=='hidden-window':
                    for peer in peers:
                        assert peer.request('present_shot_seq')['wrote'], 'Composed capture failed'
                        assert (args.output/f'wrap-{reverse}-peer{peer.seat}.png').is_file()
                tick=min(peer.status()['tick'] for peer in peers)
                exercise.checkpoint('Distant player wrap '+str(reverse),tick,min_digests=2)
            exercise.split_presentation_probe()
            report.update(status='passed')
            return
        if args.dragon_only:
            from mmx4_netplay_runtime import NetplayRuntime
            runtime=NetplayRuntime(debug)
            runtime.fixture(5,3)
            # Enter before the first encounter so its camera-relative phases
            # and placement triggers run in their original order.
            runtime.teleport(0,1200,427)
            exercise.wait_ticks(160)
            report['dragon_samples']=samples=[]
            attacks=0
            runtime.input(0xffdf,0)
            try:
                for i in range(160):
                    row=exercise.split_world_observation('dragon-'+str(i))
                    row['pools']=[]
                    for base,stride,count in ((0x8013bed0,0x9c,48),(0x8013e510,0x70,32),
                                               (0x8013f328,0x9c,32),(0x80173ca0,0x60,64),
                                               (0x80169cb8,0x30,4)):
                        data=runtime.read(base,stride*count)
                        for j in range(count):
                            body=data[j*stride:(j+1)*stride]
                            if body[0] and body[3]:row['pools'].append(dict(address=hex(base+j*stride),body=body.hex()))
                    samples.append(row)
                    screen_attack=any(0x8013bed0<=int(a['address'],16)<0x8013bed0+48*0x9c and
                        bytes.fromhex(a['body'])[1]==0 and bytes.fromhex(a['body'])[0x14]==255 and
                        bytes.fromhex(a['body'])[5] in (1,4) for a in row['pools'])
                    if screen_attack:attacks+=1
                    if args.frontend=='hidden-window' and (i%6==0 or screen_attack):
                        for peer in peers:
                            peer.request('present_shot',path=str(args.output/f'dragon-{i}-peer{peer.seat}.png'))
                    exercise.wait_ticks(16)
                    if attacks>=3:break
            finally:runtime.release()
            flybys=[row for row in samples if any(
                0x8013bed0<=int(a['address'],16)<0x8013bed0+48*0x9c and
                bytes.fromhex(a['body'])[1]==0 and bytes.fromhex(a['body'])[0x14]==255
                for a in row['pools'])]
            assert flybys,'Native screen-relative intro boss phase did not run'
            assert attacks>=3,'Native background attack was not captured'
            for seat in (0,1):
                projected=samples[-1]['peers'][seat]['counters'].get('mmx4.coop.split-intro-sprite-projected',0)
                assert bool(projected)!=args.expect_duplicated_dragon,('Intro projection guard',seat,projected)
                assert all(row['peers'][seat]['players']['p2']['x']==224 for row in flybys)
            exercise.split_presentation_probe()
            tick=min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('Intro dragon separated cameras',tick,min_digests=2)
            report.update(status='passed')
            return
        if args.nova_only:
            from mmx4_netplay_runtime import NetplayRuntime
            runtime=NetplayRuntime(debug)
            runtime.fixture(5,3)
            runtime.fixture(7,args.campaign,0,2)
            runtime.fixture(1,1,0,args.campaign)
            exercise.wait_observation('two native initial arrivals',lambda s:s['stage']==1 and
                s['mode']==6 and s['minor']==0 and s['enrolled'] and
                all(s[p]['state']==1 and s[p]['action']==0 for p in ('p1','p2')))
            arrival_counts=[]
            for peer in peers:
                counts={c['name']:c['count'] for c in peer.request('mod_counters')['counters']}
                arrival_counts.append(counts.get('mmx4.coop.local-view-committed',0))
            exercise.wait_ticks(6)
            report['arrival_views']=[]
            for peer,before_count in zip(peers,arrival_counts):
                counts={c['name']:c['count'] for c in peer.request('mod_counters')['counters']}
                after_count=counts.get('mmx4.coop.local-view-committed',0)
                report['arrival_views'].append(dict(seat=peer.seat,before=before_count,after=after_count))
                if not args.expect_camera_fallback:
                    assert after_count>before_count,('Arrival fell back to unified HUD',report['arrival_views'])
            runtime.wait_playable(45,advance_dialogue=True)
            exercise.split_presentation_probe()
            runtime.teleport(args.campaign,500,427);runtime.teleport(args.campaign^1,56,139)
            exercise.wait_ticks(60)
            exercise.wait_observation('two living grounded Nova participants',lambda s:
                all(s[p]['state']==1 and s[p]['hp']>0 and s[p]['grounded'] for p in ('p1','p2')))
            def capture(label):
                p=runtime.players()
                counters={c['name']:c['count'] for c in peers[0].request('mod_counters')['counters']}
                state=counters['mmx4.coop.split-state']
                layers=runtime.read(state+16,2*0xfc)
                play=runtime.read(0x801721c0,0x64)
                row=dict(label=label,players=[b.hex() for b in p],lock=play[0x1c],freeze=play[0x10],
                         cameras=[struct.unpack_from('<h',layers,i*0xfc+10)[0] for i in (0,1)],
                         rendered=[{c['name']:c['count'] for c in peer.request('mod_counters')['counters']
                                    if c['name'].startswith('mmx4.coop.local-view')}for peer in peers])
                report['observations'].append(row);return row
            before=capture('before-nova')
            report['nova_samples']=samples=[]
            # Native default R2 is Nova Strike; only the armor/progress setup is a fixture.
            runtime.input(0xfdff,args.campaign)
            try:
                for i in range(50):
                    row=capture('nova-'+str(i));samples.append(row)
                    if i in (2,8,18):
                        for peer in peers:peer.request('screenshot_hires',path=str(args.output/f'nova-{i}-peer{peer.seat}.png'))
                    exercise.wait_ticks(2)
            finally:runtime.release()
            attacks=[s for s in samples if bytes.fromhex(s['players'][args.campaign])[5]==0x22]
            assert attacks,'Native Nova Strike was not performed'
            assert all(all(bytes.fromhex(p)[4]==1 and bytes.fromhex(p)[0x5c]&127 for p in s['players'])for s in attacks),'Participant died during the fixture'
            merged=any(s['cameras'][0]==s['cameras'][1] for s in attacks)
            assert merged==args.expect_camera_fallback,('Unexpected Nova camera result',[(s['lock'],s['cameras'])for s in attacks])
            if not args.expect_camera_fallback:
                partner=args.campaign^1
                assert all(s['cameras'][partner]==before['cameras'][partner] for s in attacks),'Nova moved the idle partner camera'
                for seat in (0,1):
                    key='mmx4.coop.local-view-committed'
                    assert attacks[-1]['rendered'][seat][key]>attacks[0]['rendered'][seat][key],'Private rendering stopped during Nova'
            tick=min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('Nova Strike expected baseline failure' if args.expect_camera_fallback else
                                'Nova Strike independent cameras',tick,min_digests=2)
            report.update(status='passed')
            return
        if args.stairs_only:
            if args.cameras!='split':raise ValueError('Stair camera qualification requires --cameras split')
            from mmx4_netplay_runtime import NetplayRuntime
            runtime=NetplayRuntime(debug)
            runtime.fixture(5,3)
            cases=[];report['stairs']=cases
            for section in (0,1):
                for moving in (1,0):
                    runtime.fixture(1,3,section,args.campaign)
                    runtime.wait_playable(45,advance_dialogue=True)
                    runtime.teleport(moving^1,1570,322)
                    runtime.teleport(moving,800,500)
                    exercise.wait_ticks(30)
                    samples=[]
                    case=dict(section=section,moving=moving,samples=samples);cases.append(case)
                    runtime.input(0xffdf,moving)
                    try:
                        for i in range(20):
                            peer=peers[moving]
                            counters=peer.request('mod_counters')['counters']
                            state=next(c['count'] for c in counters if c['name']=='mmx4.coop.split-state')
                            layers=peer.read(state+16+moving*0xfc,0xfc)
                            samples.append(dict(x=struct.unpack_from('<h',layers,10)[0],
                                                phase=struct.unpack_from('<h',layers,0x54+10)[0],
                                                camera_mode=layers[4],background_mode=layers[0x54+4],
                                                players=[b.hex() for b in runtime.players()]))
                            if i in (0,10,19):
                                peer.request('screenshot_hires',path=str(args.output/f'stairs-{section}-{moving}-{i}.png'))
                            exercise.wait_ticks(6)
                    finally:runtime.release()
                    assert max(s['x'] for s in samples)-min(s['x'] for s in samples)>64,('Stair camera did not move',case)
                    assert len({s['phase'] for s in samples})==3,('Stair animation stopped',case)
                    tick=min(peer.status()['tick'] for peer in peers)
                    exercise.checkpoint(f'Mushroom stairs section{section} moving{moving}',tick,min_digests=2)
            report.update(status='passed')
            print('Independent Mushroom stairs: both seats and sections animate with matching peer CRCs',flush=True)
            return
        if args.latch_only:
            from mmx4_netplay_runtime import NetplayRuntime
            runtime=NetplayRuntime(debug)
            debug.fixture(5,3)
            runtime.fixture(1,3,1,args.campaign)
            report['stage_skip_overlays']=[peer.request('overlay_status') for peer in peers]
            for seat,overlay in enumerate(report['stage_skip_overlays']):
                if overlay['save_menu_open']:
                    print('Development packet opened the host save menu on peer',seat,flush=True)
                    runtime.input(0xdfff,seat)
                    time.sleep(.15)
            runtime.release()
            runtime.wait_playable(45,advance_dialogue=True)
            runtime.fixture(3,3)
            runtime.wait_playable(45,advance_dialogue=True)
            def latch(label):
                observations=[]
                for seat,peer in enumerate(peers):
                    pool=peer.read(0x80165A30,32*0x8c)
                    found=[]
                    for index in range(32):
                        actor=pool[index*0x8c:(index+1)*0x8c]
                        if actor[0] and actor[1]==13 and struct.unpack_from('<h',actor,14)[0]==488:
                            found.append(dict(state=actor[4],action=actor[5]))
                    if len(found)!=1:raise AssertionError((label,seat,found))
                    observations.append(found[0])
                    debug.peers[seat].screenshot(args.output/(label+'-peer'+str(seat)+'.png'))
                report['observations'].append(dict(label=label,latches=observations))
                return observations
            runtime.teleport(1,6114,592)
            runtime.teleport(0,6152,452)
            exercise.wait_ticks(6)
            waiting=latch('partner-below')
            if any(p!={'state':1,'action':0} for p in waiting):
                raise AssertionError(('Latch closed over the visible partner',waiting))
            tick=min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('Mushroom visible-partner wait CRC',tick,min_digests=2)
            runtime.teleport_both(6152,452)
            exercise.wait_ticks(32)
            closed=latch('both-clear')
            if any(p['state']!=2 and p['action']!=1 for p in closed):
                raise AssertionError(('Latch failed to close after both clear',closed))
            tick=min(peer.status()['tick'] for peer in peers)
            exercise.checkpoint('Mushroom native closure CRC',tick,min_digests=2)
            report.update(status='passed',native_latch=True)
            print('Netplay: native latch waits for the visible partner and closes with matching peer CRCs',flush=True)
            return
        if args.train_only:
            if args.cameras!='split':raise ValueError('Train scenery qualification requires --cameras split')
            from mmx4_netplay_runtime import NetplayRuntime
            from mmx4_web_probe import Probe
            runtime=NetplayRuntime(debug)
            output=args.output/'train-background';output.mkdir()
            probe=Probe(runtime,output,args.campaign)
            trace_targets=(0x800D8648,0x800D9218,0x800D964C,0x80021A20,0x8002196C)
            for target in trace_targets:peers[0].request('fntrace_arm',target=hex(target))
            peers[0].request('wtrace_arm',lo='0x801419B0',hi='0x80141AAC')
            try:
                for owner in (1,0):
                    probe.train_background(owner,2)
                    captures=[]
                    for frame in (0,1):
                        for peer in peers:
                            shot=output/f'train-owner-{owner}-peer-{peer.seat}-frame-{frame}.png'
                            peer.request('screenshot_hires',path=str(shot))
                            captures.append(str(shot))
                        time.sleep(.2)
                    probe.cases[-1]['both_peer_captures']=captures
                    tick=min(peer.status()['tick'] for peer in peers)
                    exercise.checkpoint('native Slash backgrounds owner '+str(owner),tick,min_digests=2)
            finally:
                runtime.release()
                traces={hex(target):peers[0].request('fntrace_dump',
                    target_lo=hex(target),target_hi=hex(target+4),count=128)
                    for target in trace_targets}
                traces['camera_writes']=peers[0].request('wtrace_dump',count=128,newest=1,
                    addr_lo='0x80141A04',addr_hi='0x80141AAC')
                (output/'native-traces.json').write_text(json.dumps(traces,indent=2))
                (output/'stage-interactions.json').write_text(json.dumps(
                    dict(cases=probe.cases,events=probe.events),indent=2))
            report.update(status='passed',train_background_owners=2)
            print('Split netplay: both native Slash backgrounds advance with matching peer CRCs',flush=True)
            return
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
                report.setdefault('failure_overlays', []).append(peer.request('overlay_status'))
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
