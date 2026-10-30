"""Bounded original-disc Complete Story checks in isolated, muted profiles.

Fixtures bypass prior campaign clears and position an approach, never set a
boss-dead/movie-done/next-scene result. Native controls deliver the final hit.
Requires a development runtime with TCP tools. Does not use owner saves.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import shlex
import socket
import struct
import subprocess
import sys
import time

from coop_debug import Runtime, capture_pair
from test_mmx4_coop_doors import body_state

PLAY = 0x801721C0
PLAYER = 0x801418C8


class StoryCheck:
    def __init__(self, runtime, output, campaign, coop, complete, natural):
        self.r, self.output, self.campaign = runtime, output, campaign
        self.coop, self.complete, self.natural = coop, complete, natural
        self.rows, self.movies = [], []

    def word(self, address, value):
        for i in range(4):
            self.r.write(address + i, (value >> (8*i)) & 255)

    def sample(self, label):
        play=self.r.read(PLAY,0x64)
        if self.coop:
            mirror=self.r.read(self.r.diagnostic(),0x6d0)
            bodies=[mirror[at:at+0xe4] for at in (0x300,0x100)]
            canonical=mirror[0x400:0x464]
            if play[0]==6 and canonical[12:14]==play[12:14]:
                # The counterpart briefly projects its private inventory over
                # PLAY. The completed mirror retains the campaign owner.
                play=play[:0x43]+canonical[0x43:]+play[0x64:]
        else:
            bodies=[self.r.read(PLAYER,0xe4)]
        pool=self.r.read(0x8013BED0,48*0x9c)
        enemies=[]
        for i in range(48):
            b=pool[i*0x9c:(i+1)*0x9c]
            if b[0]:
                enemies.append(dict(slot=i,type=b[1],variant=b[2],hp=b[0x5c]&127,
                    state=list(b[4:8]),x=struct.unpack_from('<i',b,8)[0]/65536,
                    y=struct.unpack_from('<i',b,12)[0]/65536))
        row=dict(label=label,mode=list(play[:4]),stage=play[12],section=play[13],
            progress=play[0x5f],clears=play[0x59],campaign=play[0x43],lives=play[0x44],
            checkpoint=play[0x1d],blocked=play[0x1c],warning=self.r.read(0x80141bdc,1)[0],
            players=[body_state(b) for b in bodies],enemies=enemies,
            menu=list(self.r.read(0x801754a0,8)))
        self.rows.append(row)
        return row

    def pulse(self, buttons, seat=0, duration=.08):
        self.r.input(buttons,seat);time.sleep(duration);self.r.release()

    def confirm(self):
        for seat in range(2 if self.coop else 1):self.r.input(0xbfff,seat)
        time.sleep(.08);self.r.release()

    @staticmethod
    def needs_confirm(row):
        # PLAY+1C remains set on some frontend transitions. It only denotes
        # a dialogue lock in gameplay; confirming a board initialization can
        # otherwise select a stage before the neutral-board assertion sees it.
        return (row['mode'][0]==6 and row['blocked']) or row['mode'][0] in (7,9,11,12) or row['mode'][:2] in ([3,1],[3,9])

    def state(self, operation):
        from coop_fmv_transition_scenario import state_operation
        state_operation(self.r,operation)

    def protect(self):
        if self.coop:
            self.r.fixture(5,3)
        else:
            # Native invulnerability only; no boss state is changed here.
            self.r.write(PLAYER+0x61,127)
            self.r.write(PLAYER+0x5c,32)

    def capture(self, name):
        if self.coop:
            capture_pair(self.r,self.output/(name+'.png'))
        else:
            self.r.screenshot(self.output/(name+'.png'))

    def wait(self, label, predicate, timeout=60, confirm=False):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            row=self.sample(label)
            if predicate(row):return row
            if confirm and self.needs_confirm(row):
                self.confirm()
            time.sleep(.06)
        raise AssertionError((label,row))

    @staticmethod
    def playable(row):
        return row['mode'][:2]==[6,0] and not row['blocked'] and not row['warning'] and all(
            b['active'] and b['hp'] and b['state']==1 and b['action']==2 and
            not b['script'] and not b['door'] for b in row['players'])

    def position(self, seat, x, y):
        if self.coop:
            self.r.teleport(seat,x,y)
        else:
            # Multiple byte writes must not race physics/collision. Use the
            # original pause menu as a quiescent setup boundary, then resume.
            self.pulse(0xfff7)
            self.wait('position-paused',lambda q:q['mode'][:2]==[6,2],12)
            self.wait('position-menu-ready',lambda q:q['menu'][4]==1 and q['menu'][5]<3,20)
            time.sleep(.3)
            for at,value in ((8,int(x*65536)),(12,int(y*65536)),
                (0x18,int(x*65536)),(0x1c,int(y*65536)),(0x20,0),(0x24,0),(0x28,0)):
                self.word(PLAYER+at,value)
            for at in (0x89,0x70,0x71):self.r.write(PLAYER+at,0)
            self.pulse(0xfff7)
            self.wait('position-resumed',lambda q:q['mode'][:2]==[6,0],12)

    def load(self, stage, section=0, checkpoint=0):
        if self.coop:
            from test_mmx4_coop_regressions import Scenarios
            s=Scenarios(self.r,self.output,self.campaign)
            s.load(stage,section,checkpoint)
        else:
            self.r.release()
            self.r.write(PLAY+12,stage);self.r.write(PLAY+13,section)
            self.word(PLAY,4)
            self.wait('solo-native-load',self.playable,90,True)
            if checkpoint:
                self.r.write(PLAY+0x1d,checkpoint);self.r.write(PLAY+0x1e,0)
                self.word(PLAY,5)
        self.wait('loaded-stage',self.playable,90,True)
        self.protect()

    def movie(self, number, continuation, *, confirm=False):
        # The exact native movie id has its own working byte, discovered by
        # the normal skip resolver. The trusted entry counters identify these
        # added clips even if no automatic skip profile is configured.
        before=self.r.request('fmv_state')
        baseline=before['mdec_decode_count'];started=time.monotonic();saw_xa=False
        seen=False;skipped=False;captures=False;rows=[]
        while time.monotonic()-started<300:
            state=self.r.request('fmv_state');row=self.sample('movie-'+str(number))
            decoded=state['mdec_decode_count']-baseline
            saw_xa|=bool(state['xa_stream_active'])
            seen|=decoded>30 and saw_xa
            rows.append(dict(elapsed=time.monotonic()-started,fmv=state,mode=row['mode']))
            if seen and not captures:
                # FMVs use native packed RGB888 scanout, unsupported by the
                # high-resolution 15-bit gameplay capture. This path decodes
                # the actual display bytes without overwriting packed VRAM.
                try:
                    self.r.request('screenshot_file',path=str(self.output/f'movie-{number}.png'))
                    captures=True
                except RuntimeError as error:
                    if 'display disabled' not in str(error):raise
            if seen and continuation(row):
                break
            if seen and number not in self.natural and not skipped:
                self.pulse(0xfff7);skipped=True
            elif confirm and self.needs_confirm(row):
                self.confirm()
            time.sleep(.12)
        else:raise AssertionError(('Movie did not reach its continuation',number,row,rows[-1]))
        counts={c['name']:c['count'] for c in self.r.request('mod_counters')['counters']}
        key={7:'colonel',8:'iris',9:'sigma'}[number]
        if self.complete:
            # Debug co-op wraps 18000 in a guarded guest call: plugin entry
            # callbacks can run twice while the native movie runs once. Its
            # existing native-body observer is authoritative for this mode.
            if self.coop:
                assert counts.get(f'mmx4.qa.movie{number}.native-starts')==1,counts
                assert counts.get(f'mmx4.qa.movie{number}.native-returns')==1,counts
            else:
                assert counts.get('mmx4.story.'+key+'-movie')==1,counts
        assert decoded>30 and saw_xa,('No real video/audio',number,rows[-1])
        result=dict(movie=number,natural=number in self.natural,skipped=skipped,
            decoded=decoded,xa=saw_xa,continuation=row,counters=counts)
        self.movies.append(result)
        (self.output/f'movie-{number}-timeline.json').write_text(json.dumps(rows,indent=2))
        return row

    def defeat(self, kind):
        lowered=set();start=time.monotonic()
        while time.monotonic()-start<65:
            row=self.sample('native-hit-'+str(kind))
            foes=[e for e in row['enemies'] if e['type']==kind]
            if any(e['hp']==0 and e['state'][0]==2 for e in foes):return row
            if not foes:raise AssertionError(('Boss disappeared before native defeat',kind,row))
            # Iris's armor is not the damageable core. Native saber hits first
            # expose that core; lower only its HP, then land a real final hit.
            boss=next((e for e in foes if e['variant']),foes[0])
            if (kind!=66 or boss['variant']) and (boss['slot'],boss['variant']) not in lowered:
                self.r.write(0x8013bed0+boss['slot']*0x9c+0x5c,1)
                lowered.add((boss['slot'],boss['variant']))
            seat=(self.campaign if kind==73 else self.campaign^1) if self.coop else 0
            character=(self.campaign^seat) if self.coop else self.campaign
            if kind==69:
                # This small, flat arena needs no position fixture at all.
                # Let native movement/collision and Colonel's own approach
                # deliver a grounded saber or charged-buster hit.
                self.protect()
                player=row['players'][seat]
                direction=0xffdf if boss['x']>=player['x'] else 0xff7f
                self.pulse(direction,seat,.08 if abs(boss['x']-player['x'])>55 else .02)
                self.pulse(0x7fff,seat,.12 if character else 1.4)
                time.sleep(.2)
                continue
            if not self.coop and character and kind in (66,73):
                # Pausing/repositioning around Iris's fast native movement
                # makes short-range targets stale by the time the menu shuts.
                # Use the normal acquired double-jump/spin and native pursuit.
                self.protect()
                player=row['players'][0]
                direction=0xffdf if boss['x']>=player['x'] else 0xff7f
                self.pulse(direction & 0x3fff,0,.16)
                time.sleep(.15)
                continue
            x=max(30,boss['x']-(60 if not character else 24))
            y=boss['y']
            if kind in (66,73):x=max(2080,min(2340,x));y=min(442 if character else 443,y)
            self.position(seat,x,y);self.protect()
            self.pulse(0xffdf,seat,.03)
            self.pulse(0x7fff,seat,1.4 if not character else .12)
            time.sleep(.2)
        raise AssertionError(('Native attacks did not defeat boss',kind,row))

    def memorial(self):
        self.r.release()
        # Exactly four prior clears are the fixture. Native HQ dialogue,
        # Memorial loader, Colonel encounter and completion remain under test.
        self.r.write(PLAY+0x59,15);self.r.write(PLAY+0x5f,3);self.word(PLAY,3)
        if not self.complete and self.campaign:
            row=self.movie(7,lambda q:q['mode'][:2]==[3,4],confirm=True)
            assert row['progress']==4 and row['campaign']==1,row
            assert not any(q['stage']==9 and q['mode'][0]==6 for q in self.rows)
            return dict(stock_zero_fmv_only=True,returned=row)
        self.wait('memorial-playable',lambda q:q['stage']==9 and self.playable(q),120,True)
        self.protect()
        for seat in range(2 if self.coop else 1):self.r.input(0xffdf,seat)
        self.wait('colonel-admission',lambda q:any(e['hp']==48 and e['state'][0]==1 for e in q['enemies']),45,True)
        self.r.release();self.wait('colonel-controls',self.playable,60,True)
        self.capture('memorial-controls')
        self.state('save')
        row=self.sample('memorial-boss')
        bosses=[e for e in row['enemies'] if e['hp']==48 and e['state'][0]==1]
        assert len(bosses)==1,('Expected native Memorial Colonel',row)
        kind=bosses[0]['type'];before=row['lives']
        self.defeat(kind)
        if not self.complete:
            row=self.wait('stock-memorial-return',lambda q:q['mode'][:2]==[3,4],90)
            assert row['campaign']==0 and row['progress']==4,row
            return dict(stock_x_fight_only=True,returned=row)
        row=self.movie(7,lambda q:q['mode'][:2]==[3,4],confirm=True)
        assert row['campaign']==self.campaign and row['progress']==4 and row['clears']==15,row
        assert row['lives']==before,row
        self.capture('memorial-return-board')
        return dict(colonel_type=kind,returned=row)

    def fortress(self):
        # This encounter follows all eight Mavericks in the original route.
        # Prepare those earlier abilities as well as the authored checkpoint.
        self.r.write(PLAY+0x59,255)
        self.load(11,0,1)
        self.r.input(0xffdf,0)
        if self.coop:self.r.input(0xffdf,1)
        first=66 if self.campaign else 73
        for index,kind in enumerate((first,73 if first==66 else 66)):
            self.wait('fortress-boss-'+str(kind),lambda q:any(e['type']==kind and e['hp']==48 for e in q['enemies']),120,True)
            self.r.release();self.wait('fortress-controls',self.playable,60,True)
            self.capture('fortress-round-'+str(index))
            self.defeat(kind)
            if not index:
                self.wait('second-encounter-load',lambda q:q['mode'][0]==4 or
                    (q['mode'][0]==6 and q['players'][0]['x']<2000),90,True)
                self.wait('second-door-controls',self.playable,60,True)
                self.state('save');self.state('load')
                if self.coop:
                    self.r.input(0xffdf,0);self.r.input(0xffdf,1)
                else:self.r.input(0xffdf)
        row=self.movie(8,lambda q:q['stage']==11 and q['section']==1 and self.playable(q),confirm=True)
        assert row['campaign']==self.campaign,row
        self.capture('fortress-return-controls')
        return dict(rounds=[first,73 if first==66 else 66],returned=row)

    def sigma(self):
        # Bypass the refight gauntlet only. Native door and dialogue trigger
        # the mode10 movie; no movie-complete or boss admission flag is set.
        self.r.write(PLAY+0x59,255)
        self.load(12,0,10)
        for seat in range(2 if self.coop else 1):self.position(seat,1920,2506)
        self.r.input(0xffdf)
        if self.coop:self.r.input(0xffdf,1)
        self.wait('sigma-scene-admitted',lambda q:q['stage']==12 and q['checkpoint']==10 and q['blocked'],40)
        self.r.release()
        row=self.movie(9,lambda q:q['stage']==12 and q['section']==1 and q['mode'][0]==6,confirm=True)
        self.wait('sigma-control-recovery',self.playable,75,True)
        self.capture('sigma-return-controls')
        assert row['campaign']==self.campaign,row
        return dict(returned=row)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe',type=Path,required=True)
    ap.add_argument('--disc',type=Path,required=True)
    ap.add_argument('--game',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--campaign',type=int,choices=(0,1),default=1)
    ap.add_argument('--coop',action='store_true')
    ap.add_argument('--stock-story',action='store_true')
    ap.add_argument('--software',action='store_true',help='CPU rendering for a compatible headless Windows target')
    ap.add_argument('--recompiler',type=Path,help='Pinned native emitter for a source-tree QA profile')
    ap.add_argument('--native-cc',type=Path,help='Native cache compiler for that emitter')
    ap.add_argument('--cases',nargs='+',choices=('memorial','fortress','sigma'),default=['memorial'])
    ap.add_argument('--natural',type=int,nargs='*',choices=(7,8,9),default=[])
    ap.add_argument('--port',type=int,default=18731)
    args=ap.parse_args()
    if args.stock_story and args.cases!=['memorial']:
        ap.error('The stock-story control covers Memorial Hall only')
    out=args.output;out.mkdir(exist_ok=False,parents=True)
    for name in ('assets','mods','bios'):shutil.copytree(args.exe.parent/name,out/name)
    shutil.copy2(args.exe,out/args.exe.name)
    for dll in args.exe.parent.glob('*.dll'):shutil.copy2(dll,out/dll.name)
    (out/'mods/state.toml').write_text('format_version = 2\n[[feature]]\npackage_id = "mmx4.coop"\nid = "coop"\nenabled = '+str(args.coop).lower()+'\n'
        '[feature.values]\nlocal_cameras = "split"\nboss_doors = "either"\nboth_story_bosses = false\n'
        '\n[[feature]]\npackage_id = "mmx4.complete-story"\nid = "complete-story"\nenabled = '+str(not args.stock_story).lower()+'\n'
        '\n[[feature]]\npackage_id = "mmx4.diagnostics"\nid = "inputs"\nenabled = true\n')
    if args.software:
        assert not args.coop,'The co-op profile requires OpenGL'
        with (out/'mods/state.toml').open('a') as state:
            state.write('\n[[feature]]\npackage_id = "mmx4.enhancement.widescreen"\nid = "widescreen"\nenabled = false\n')
    cfg=args.game.read_text(encoding='utf-8').replace('internal_resolution = "1080p"','internal_resolution = "native"')
    if args.recompiler:
        framework=args.game.parent/'psxrecomp-v4'
        helper=framework/'tools/compile_overlays.py'
        assert args.recompiler.is_file() and helper.is_file()
        command=[sys.executable,str(helper),'--game-toml',str(out/'game.toml'),
            '--recompiler',str(args.recompiler),'--runtime-include',str(framework/'runtime/include'),'--cps']
        if args.native_cc:
            assert args.native_cc.is_file()
            command+=['--gcc',str(args.native_cc)]
        cfg='\n'.join(line for line in cfg.splitlines() if not line.startswith('overlay_autocompile_cmd ='))
        quoted=subprocess.list2cmdline(command) if os.name=='nt' else shlex.join(command)
        cfg=cfg.replace('[runtime]','[runtime]\noverlay_autocompile_cmd = '+json.dumps(quoted),1)
    if args.software:cfg=cfg.replace('renderer = "opengl"','renderer = "software"')
    (out/'game.toml').write_text(cfg,encoding='utf-8')
    env={k:v for k,v in os.environ.items() if not k.upper().startswith(('PSX_','RNET_','MMX4_DIAGNOSTIC_'))}
    env['SDL_AUDIODRIVER']='dummy'
    env['PATH']='C:/msys64/mingw64/bin;'+env.get('PATH','')
    argv=[str(out/args.exe.name),'--headless' if args.software else '--hidden-window','--no-launcher','--debug-port',str(args.port),
        '--disc',str(args.disc),'--game',str(out/'game.toml'),'--bios',str(out/'bios/openbios.bin'),'--memcard-dir',str(out/'saves')]
    with socket.socket() as sock:sock.bind(('127.0.0.1',args.port))
    r=Runtime(args.port,isolated_inputs=True)
    s=StoryCheck(r,out,args.campaign,args.coop,not args.stock_story,args.natural)
    report=dict(argv=argv,campaign=args.campaign,coop=args.coop,complete=not args.stock_story,
        binary_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),cases=[],prerequisites_bypassed=True,
        harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    with (out/'runtime.log').open('wb') as log:
        p=subprocess.Popen(argv,cwd=out,env=env,stdout=log,stderr=log,
            creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
        try:
            end=time.monotonic()+100
            while True:
                try:r.request('get_registers');break
                except OSError:assert p.poll() is None and time.monotonic()<end;time.sleep(.1)
            r.request('turbo',enabled=1)
            sleep=time.sleep;time.sleep=lambda t:sleep(t*.15)
            try:r.boot(campaign=args.campaign,wait_for_coop=args.coop,timeout=180)
            finally:time.sleep=sleep;r.request('turbo',enabled=0)
            s.wait('boot-gameplay',s.playable,45,True)
            for name in args.cases:
                result=dict(case=name,status='running');report['cases'].append(result)
                result.update(getattr(s,name)(),status='passed')
                print(json.dumps(result),flush=True)
            report['status']='passed'
        except Exception as error:
            report['error']=repr(error);report['status']='failed'
            if report['cases']:report['cases'][-1]['status']='failed'
            raise
        finally:
            try:r.release();r.request('quit')
            except Exception:pass
            try:p.wait(timeout=10)
            except subprocess.TimeoutExpired:p.terminate();p.wait(timeout=5)
            (out/'observations.json').write_text(json.dumps(s.rows,indent=2))
            report['movies']=s.movies
            (out/'report.json').write_text(json.dumps(report,indent=2))


if __name__=='__main__':main()
