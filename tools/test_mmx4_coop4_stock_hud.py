"""Native 3P/4P stock-charge HUD ownership with distinct real keyboard inputs.

Windows hidden-drawable QA only. A legal roster supplies the stock-charge arm;
setup fixtures protect and align the players. Counts are never written: normal
SDL keyboard input produces and consumes every reported stored shot.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

from coop_debug import Runtime,capture_pair
from coop_private_window_input import PrivateWindowInput
from test_mmx4_coop4_hud import png_pixels

FORMS=('x','force_x','ultimate_x','zero','black_zero','none')
SQUARE_KEYS=('Z','C','V','B')
BUTTONS=('up','down','left','right','cross','circle','square','triangle','l1','r1','l2','r2',
         'l3','r3','start','select','ls_up','ls_down','ls_left','ls_right',
         'rs_up','rs_down','rs_left','rs_right')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--disc',type=Path,required=True)
    p.add_argument('--game',type=Path,required=True)
    p.add_argument('--report-dir',type=Path,required=True)
    p.add_argument('--roster',nargs=4,choices=FORMS,required=True)
    p.add_argument('--port',type=int,default=18011)
    a=p.parse_args()
    count=3 if a.roster[3]=='none' else 4
    assert 'none' not in a.roster[:3] and len(set(a.roster))==4,a.roster
    assert 'force_x' in a.roster,'A stock-charge Force Armor X is required'
    force=a.roster.index('force_x')
    exe=a.exe.resolve();root=a.report_dir.resolve();config_source=a.game.resolve()
    assert root.name.startswith('labels-production-qa'),root
    root.mkdir(parents=True,exist_ok=False)
    for name in ('assets','mods','bios'):shutil.copytree(exe.parent/name,root/name)
    for item in (exe,*exe.parent.glob('*.dll')):shutil.copy2(item,root/item.name)
    for name in ('provenance.json',exe.stem+'.execution.json'):
        if (exe.parent/name).is_file():shutil.copy2(exe.parent/name,root/name)
    (root/'mods/state.toml').write_text('format_version = 2\n[[feature]]\npackage_id = "mmx4.coop4"\n'
        'id = "coop"\nenabled = true\n[feature.values]\n'+''.join(
            'p%d = %s\n'%(i+1,json.dumps(form)) for i,form in enumerate(a.roster)),encoding='utf-8')
    config=config_source.read_text(encoding='utf-8').replace('exe = "mmx4/SLUS_005.61"',
        'exe = '+json.dumps((config_source.parent/'mmx4/SLUS_005.61').as_posix()))
    (root/'game.toml').write_text(config,encoding='utf-8')
    (root/'settings.toml').write_text('[video]\nfullscreen = false\nwindow_width = 960\n'
        'internal_resolution = "native"\nrender_thread = true\n[audio]\nvolume = 0\n'
        '[controller]\nmultitap = true\n'+''.join(
            'p%d_device = "keyboard"\np%d_mode = "digital"\n'%(seat+1,seat+1) for seat in range(4)),encoding='utf-8')
    bindings=[]
    for seat in range(4):
        values={name:'None' for name in BUTTONS};values['square']=SQUARE_KEYS[seat]
        if seat==0:values.update(start='Return',cross='X',up='Up',down='Down',left='Left',right='Right')
        bindings.append('[player%d]\n'%(seat+1)+''.join(name+' = '+value+'\n' for name,value in values.items()))
    (root/'keybinds.ini').write_text('\n'.join(bindings),encoding='utf-8')
    env={k:v for k,v in os.environ.items() if not k.upper().startswith(('PSX_','LNG_','RNET_'))}
    for key,name in [('TEMP','temp'),('TMP','temp'),('APPDATA','appdata'),('LOCALAPPDATA','local')]:
        folder=root/name;folder.mkdir(exist_ok=True);env[key]=str(folder)
    env.update(SDL_AUDIODRIVER='dummy',SDL_WINDOWS_RAW_KEYBOARD='0',MMX4_RECOMP_DATA_DIR=str(root),
        PSX_OVERLAY_AUTOCOMPILE_OFF='1',PSX_RENDER_PASS_VERIFY='1',PSX_DYNRES='0',
        PSX_INTERNAL_RESOLUTION='native',PSX_DEBUG_TOGGLES='0')
    argv=[str(root/exe.name),'--hidden-window','--no-launcher','--renderer','opengl','--debug-port',str(a.port),
        '--game',str(root/'game.toml'),'--bios',str(root/'bios/openbios.bin'),'--disc',str(a.disc.resolve()),
        '--memcard-dir',str(root/'saves')]
    report=dict(binary_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),roster=a.roster,
        players=count,force_seat=force,argv=argv,checks=[],passed=False,
        input_method='Distinct per-seat keyboard bindings; window-local PostMessage to owned hidden process')
    if (root/'provenance.json').is_file():
        report['provenance']=json.loads((root/'provenance.json').read_text(encoding='utf-8-sig'))
    r=Runtime(a.port);keys=None;diagnostic=None
    def save():(root/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    def sample(label):
        data=r.read(diagnostic,0xe00);header=struct.unpack_from('<6I',data)
        assert header[0]==0x34503458 and header[1]==count and header[2] and not header[3],header
        bodies=[data[0x100+seat*0x100:0x1e4+seat*0x100] for seat in range(count)]
        cameras=[]
        for seat in range(count):
            camera=data[0x500+seat*0x100:0x5fc+seat*0x100]
            cameras.append([[struct.unpack_from('<i',camera,layer*0x54+8)[0]/65536,
                             struct.unpack_from('<i',camera,layer*0x54+12)[0]/65536] for layer in range(3)])
        pool=r.read(0x80173ca0,64*0x60);actors=[]
        for slot in range(64):
            body=pool[slot*0x60:(slot+1)*0x60]
            if body[0] and body[1]==0x36:
                actors.append(dict(address=hex(0x80173ca0+slot*0x60),visible=body[3],state=body[4],
                    frame=body[0x47],assembly=hex(struct.unpack_from('<I',body,0x3c)[0]),
                    xy=[struct.unpack_from('<h',body,10)[0],struct.unpack_from('<h',body,14)[0]]))
        row=dict(label=label,frame=header[4],mode=list(r.read(0x801721c0,0x20)),
            characters=[body[2] for body in bodies],hp=[body[0x5c]&127 for body in bodies],
            state=[list(body[4:7]) for body in bodies],stocks=[body[0xa6] for body in bodies],
            armor=[list((body[0xa7],body[0xb8])) for body in bodies],
            positions=[[struct.unpack_from('<i',body,8)[0]/65536,struct.unpack_from('<i',body,12)[0]/65536] for body in bodies],
            charge_bytes=[body[0x8c:0xa0].hex() for body in bodies],cameras=cameras,actors=actors)
        assert row['characters']==[int(form in ('zero','black_zero')) for form in a.roster[:count]],row
        assert all(row['hp']),row
        report['checks'].append(row);save();return row
    def frames(amount,label):
        start=int.from_bytes(r.read(diagnostic+16,4),'little');deadline=time.monotonic()+90
        while int.from_bytes(r.read(diagnostic+16,4),'little')<start+amount:
            if time.monotonic()>=deadline:raise TimeoutError(label)
            time.sleep(.02)
        return sample(label)
    def fixture(command,seat,x,y=0):
        seq=int.from_bytes(r.read(diagnostic+0x30,4),'little')+1
        for off,value in [(0x34,command),(0x38,seat),(0x3c,x),(0x40,y),(0x30,seq)]:
            for byte in range(4):r.write(diagnostic+off+byte,(value>>(byte*8))&255)
        deadline=time.monotonic()+20
        while int.from_bytes(r.read(diagnostic+0x44,4),'little')!=seq:
            if time.monotonic()>=deadline:raise TimeoutError('Setup fixture not acknowledged')
            time.sleep(.02)
        assert int.from_bytes(r.read(diagnostic+0x48,4),'little')==1,(command,seat,x,y)
    def picture(label,stocked):
        path=root/(label+'.png');capture_pair(r,path)
        width,height,channels,rows=png_pixels(path)
        assert abs(width*3-height*4)<=4,(width,height)
        sx=width/640;sy=height/480;crops=[];rectangles=[]
        for seat in range(count):
            left=seat%2*width/2 if count==4 or seat<2 else width/4
            top=int(seat>=2)*height/2
            # Y=96 still includes the final X/Zero portrait row. The stock
            # assembly begins at 97, below the character-specific artwork.
            box=(int(left+7*sx),int(top+97*sy),int(left+43*sx),int(top+128*sy))
            rectangles.append(box)
            crop=[]
            for y in range(box[1],box[3]):
                for x in range(box[0],box[2]):crop.append(tuple(rows[y][x*channels:x*channels+3]))
            crops.append(crop)
        reference=next(seat for seat in range(count) if seat!=force)
        diffs=[sum(a!=b for a,b in zip(crop,crops[reference])) for crop in crops]
        row=dict(label=label+'-pixels',path=str(path),stocked=stocked,rectangles=rectangles,
            reference_seat=reference,different_pixels=diffs)
        report['checks'].append(row);save()
        # All setup cameras coincide. In the stock-only area, the same world
        # must appear in every pane; only Force X's four icons may differ.
        assert all(value==0 for seat,value in enumerate(diffs) if seat!=force),row
        if stocked:assert diffs[force]>150,row
        else:assert diffs[force]==0,row
        return row
    def hold_square(seat,amount,label):
        keys.key(ord(SQUARE_KEYS[seat]),True)
        try:return frames(amount,label)
        finally:keys.key(ord(SQUARE_KEYS[seat]),False)

    with (root/'runtime.log').open('wb') as log:
        process=subprocess.Popen(argv,cwd=root,env=env,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW)
        report['pid']=process.pid;save()
        try:
            deadline=time.monotonic()+120
            while True:
                assert process.poll() is None,'Private runtime exited'
                try:r.request('get_registers');break
                except OSError:
                    if time.monotonic()>=deadline:raise TimeoutError('Debug server did not start')
                    time.sleep(.1)
            keys=PrivateWindowInput(process,root/exe.name)
            r.request('turbo',enabled=1)
            try:r.boot(campaign=int(a.roster[0] in ('zero','black_zero')),wait_for_coop=False)
            finally:r.request('turbo',enabled=0);r.release()
            diagnostic=next(c['count'] for c in r.request('mod_counters')['counters'] if c['name']=='mmx4.coop4.diagnostic-address')
            deadline=time.monotonic()+90
            while True:
                raw=r.read(diagnostic,24);mode=r.read(0x801721c0,0x20)
                if int.from_bytes(raw[8:12],'little') and mode[0:2]==bytes((6,0)) and not mode[28]:break
                if time.monotonic()>=deadline:raise TimeoutError('Four-player enrollment did not settle')
                time.sleep(.05)
            frames(70,'native-enrolled');fixture(3,0,(1<<count)-1)
            spawn=sample('spawn-protected')
            for seat in range(count):fixture(1,seat,96,int(spawn['positions'][seat][1]))
            empty=frames(70,'aligned-empty')
            assert empty['stocks']==[0]*count,empty
            assert all(camera==empty['cameras'][0] for camera in empty['cameras']),empty
            assert len(empty['actors'])==sum(form in ('x','force_x','ultimate_x') for form in a.roster[:count]),empty
            assert empty['armor'][force]==[15,1],empty
            picture('stock-empty',False)
            hold_square(force,210,'force-native-charge-held')
            charged=frames(12,'force-native-four-stocks')
            expected=[0]*count;expected[force]=4
            assert charged['stocks']==expected,charged
            assert sum(bool(actor['visible']) for actor in charged['actors'])==1,charged
            assert next(actor for actor in charged['actors'] if actor['visible'])['frame']==27,charged
            picture('stock-four',True)
            # Charging each other legal X form, or swinging either Zero form,
            # must not borrow or consume Force X's stock or duplicate its HUD.
            for seat in range(count):
                if seat==force:continue
                hold_square(seat,120,'control-seat-%d-held'%(seat+1))
                control=frames(16,'control-seat-%d-released'%(seat+1))
                assert control['stocks']==expected,control
                picture('stock-control-seat-%d'%(seat+1),True)
            # Refuse private render transactions while normal input consumes a
            # stock. Canonical fallback contains P1's HUD only; an extra seat's
            # X indicator must be hidden there, then return in its own pane.
            before={item['name']:item['count'] for item in r.request('mod_counters')['counters']}
            r.request('render_pass_refuse',on=1)
            try:
                hold_square(force,3,'force-fire-refused')
                refused=frames(15,'force-three-stocks-refused')
                expected[force]=3
                assert refused['stocks']==expected,refused
                r.screenshot(root/'stock-refused-canonical.png')
                capture_pair(r,root/'stock-refused-composed.png')
                after={item['name']:item['count'] for item in r.request('mod_counters')['counters']}
                if force:
                    key='mmx4.coop4.stored-charge-hud-canonical-hidden'
                    assert after.get(key,0)>before.get(key,0),(before,after)
                report['refusal_counters']={'before':before,'after':after};save()
            finally:r.request('render_pass_refuse',on=0)
            restored=frames(8,'force-three-stocks-restored')
            assert restored['stocks']==expected,restored
            picture('stock-after-refusal',True)
            for remaining in (2,1,0):
                hold_square(force,3,'force-fire-%d'%remaining)
                consumed=frames(15,'force-stock-%d'%remaining)
                expected[force]=remaining
                assert consumed['stocks']==expected,consumed
            frames(8,'force-stock-empty');picture('stock-consumed',False)
            counters={item['name']:item['count'] for item in r.request('mod_counters')['counters']}
            stats=r.request('render_pass_stats')
            assert counters.get('mmx4.coop4.stored-charge-hud-owner-draws',0)>0,counters
            assert counters.get('mmx4.coop4.stored-charge-hud-other-view-hidden',0)>0,counters
            if force:assert counters.get('mmx4.coop4.stored-charge-hud-owner-ticks',0)>0,counters
            assert not counters.get('mmx4.coop4.local-view-state-leak',0),counters
            assert stats['verify_checks']>100 and not any(stats[key] for key in ('verify_mismatch','aborted','watchdog','vram_leaks')),stats
            report.update(counters=counters,render_stats=stats,stereo=r.request('stereo_stats'),passed=True);save()
            print(json.dumps(dict(passed=True,roster=a.roster,force_seat=force,restore_checks=stats['verify_checks'])))
        except Exception as error:
            report['error']=repr(error);save()
            try:capture_pair(r,root/'failure.png')
            except Exception:pass
            raise
        finally:
            if keys:
                try:keys.release_all()
                except Exception:pass
            try:r.request('quit')
            except (OSError,RuntimeError,ValueError):pass
            if process.poll() is None:process.terminate()
            process.wait(timeout=15)


if __name__=='__main__':main()
