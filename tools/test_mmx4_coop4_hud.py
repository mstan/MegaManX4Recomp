"""Native 3/4P damage-trail and health-color checks in hidden, muted copies.

Offline fixtures change health and damage grace only in this test's process.
They do not test damage collision, death, doors or campaign progression.
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
import zlib

from coop_debug import Runtime, capture_pair


def png_pixels(path):
    data=path.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n'
    at=8;packed=bytearray()
    while at<len(data):
        size=struct.unpack_from('>I',data,at)[0];kind=data[at+4:at+8];chunk=data[at+8:at+8+size];at+=size+12
        if kind==b'IHDR':
            width,height,depth,color,_,_,interlace=struct.unpack('>IIBBBBB',chunk)
            assert depth==8 and color in(2,6) and not interlace
        elif kind==b'IDAT':packed.extend(chunk)
    channels=3 if color==2 else 4;stride=width*channels;raw=zlib.decompress(packed)
    previous=bytearray(stride);rows=[]
    for y in range(height):
        at=y*(stride+1);kind=raw[at];row=bytearray(raw[at+1:at+1+stride]);assert kind<=4
        for i in range(stride):
            a=row[i-channels] if i>=channels else 0;b=previous[i];c=previous[i-channels] if i>=channels else 0
            if kind==1:predict=a
            elif kind==2:predict=b
            elif kind==3:predict=(a+b)//2
            elif kind==4:
                p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c)
                predict=a if pa<=pb and pa<=pc else b if pb<=pc else c
            else:predict=0
            row[i]=(row[i]+predict)&255
        rows.append(row);previous=row
    return width,height,channels,rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--disc',type=Path,required=True)
    p.add_argument('--report-dir',type=Path,required=True)
    p.add_argument('--players',type=int,choices=(3,4),default=4)
    p.add_argument('--force-x',action='store_true')
    p.add_argument('--port',type=int,default=17958)
    a=p.parse_args();source=Path(__file__).resolve().parents[1]
    exe=a.exe.resolve();root=a.report_dir.resolve();root.mkdir(parents=True,exist_ok=False)
    for name in ('assets','mods','bios'):shutil.copytree(exe.parent/name,root/name)
    for f in (exe,*exe.parent.glob('*.dll')):shutil.copy2(f,root/f.name)
    values='[feature.values]\n'+('p4 = "none"\n' if a.players==3 else '')+('p3 = "force_x"\n' if a.force_x else '')
    (root/'mods/state.toml').write_text('format_version = 2\n[[feature]]\npackage_id = "mmx4.coop4"\nid = "coop"\nenabled = true\n'+values,encoding='utf-8')
    config=(source/'game.toml').read_text(encoding='utf-8').replace('exe = "mmx4/SLUS_005.61"','exe = '+json.dumps((source/'mmx4/SLUS_005.61').as_posix()))
    (root/'game.toml').write_text(config,encoding='utf-8')
    (root/'settings.toml').write_text('[video]\nfullscreen = false\nwindow_width = 960\ninternal_resolution = "native"\nrender_thread = true\n[audio]\nvolume = 0\n',encoding='utf-8')
    env={k:v for k,v in os.environ.items() if not k.upper().startswith(('PSX_','LNG_','RNET_'))}
    for key,name in [('TEMP','temp'),('TMP','temp'),('APPDATA','appdata'),('LOCALAPPDATA','local')]:
        folder=root/name;folder.mkdir(exist_ok=True);env[key]=str(folder)
    env.update(SDL_AUDIODRIVER='dummy',MMX4_RECOMP_DATA_DIR=str(root),PSX_OVERLAY_AUTOCOMPILE_OFF='1',PSX_RENDER_PASS_VERIFY='1',PSX_DYNRES='0',PSX_INTERNAL_RESOLUTION='native',PSX_DEBUG_TOGGLES='0')
    command=[str(root/exe.name),'--hidden-window','--no-launcher','--renderer','opengl','--debug-port',str(a.port),'--game',str(root/'game.toml'),'--bios',str(root/'bios/openbios.bin'),'--disc',str(a.disc.resolve()),'--memcard-dir',str(root/'saves')]
    r=Runtime(a.port);report=dict(binary_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),players=a.players,force_x=a.force_x,checks=[],passed=False)
    def save():(root/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    with (root/'runtime.log').open('wb') as log:
        process=subprocess.Popen(command,cwd=root,env=env,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            deadline=time.monotonic()+120
            while True:
                assert process.poll() is None,'Private runtime exited'
                try:r.request('get_registers');break
                except OSError:
                    assert time.monotonic()<deadline;time.sleep(.1)
            r.request('turbo',enabled=1);r.boot(wait_for_coop=False);r.request('turbo',enabled=0)
            diagnostic=next(c['count'] for c in r.request('mod_counters')['counters'] if c['name']=='mmx4.coop4.diagnostic-address')
            def frames(count):
                start=int.from_bytes(r.read(diagnostic+16,4),'little');deadline=time.monotonic()+90
                while int.from_bytes(r.read(diagnostic+16,4),'little')<start+count:
                    assert time.monotonic()<deadline,'Native simulation stopped';time.sleep(.02)
            def fixture(command,seat,value):
                seq=int.from_bytes(r.read(diagnostic+0x30,4),'little')+1
                for off,item in [(0x34,command),(0x38,seat),(0x3c,value),(0x30,seq)]:
                    for byte in range(4):r.write(diagnostic+off+byte,(item>>(byte*8))&255)
                deadline=time.monotonic()+20
                while int.from_bytes(r.read(diagnostic+0x44,4),'little')!=seq:
                    assert time.monotonic()<deadline;time.sleep(.02)
                assert int.from_bytes(r.read(diagnostic+0x48,4),'little')==1
            def sample(label):
                data=r.read(diagnostic,0xe00)
                assert int.from_bytes(data[4:8],'little')==a.players and not int.from_bytes(data[12:16],'little')
                bodies=[data[0x100+s*256:0x1e4+s*256] for s in range(a.players)]
                row=dict(label=label,frame=int.from_bytes(data[16:20],'little'),hp=[b[0x5c]&127 for b in bodies],trail=[b[0x5d] for b in bodies],delay=[b[0x5f] for b in bodies])
                report['checks'].append(row);save();return row
            def picture(label,hp,red_seat=None):
                path=root/(label+'.png');capture_pair(r,path)
                w,h,c,rows=png_pixels(path);sx=w/640;sy=h/480;bars=[]
                for seat,value in enumerate(hp):
                    col=seat%2 if seat<2 or a.players==4 else 0;line=int(seat>=2)
                    x=int(col*w/2+23.5*sx);ys=range(int(line*h/2+48*sy),int(line*h/2+79*sy))
                    counts=dict(green=0,red=0,critical=0)
                    for y in ys:
                        rr,gg,bb=rows[y][x*c:x*c+3]
                        counts['green']+=rr<32 and gg>160 and bb<110
                        counts['red']+=rr>140 and gg<64 and bb<64
                        counts['critical']+=rr>200 and gg>160 and bb<140
                    bars.append(counts)
                    fill=counts['critical'] if value<10 else counts['green']
                    assert abs(fill-value*sy)<=2,(label,seat,value,counts)
                    if red_seat is None:assert counts['red']==0,(label,seat,counts)
                    elif seat==red_seat:assert counts['red']>0,(label,seat,counts)
                report['checks'].append(dict(label=label+'-pixels',bars=bars));save()
            frames(70);fixture(3,0,(1<<a.players)-1)
            initial=sample('full-health');assert initial['hp']==[32]*a.players
            picture('full-health',[32]*a.players)
            values=[29,14,24,15][:a.players]
            for seat,value in enumerate(values):fixture(2,seat,value)
            frames(96);settled=sample('independent-health-settled')
            assert settled['hp']==values and settled['trail']==values,settled
            picture('independent-health-settled',values)
            fixture(2,1,8);frames(1);damaged=sample('p2-damage-trail')
            assert damaged['hp'][1]==8 and damaged['trail'][1]>8,damaged
            damaged_values=list(values);damaged_values[1]=8
            picture('p2-damage-trail',damaged_values,red_seat=1)
            frames(40);settled=sample('p2-critical-settled')
            assert settled['hp']==damaged_values and settled['trail']==damaged_values,settled
            picture('p2-critical-settled',damaged_values)
            fixture(2,2,4);frames(96);damaged_values[2]=4
            row=sample('p3-critical-settled');assert row['hp']==row['trail']==damaged_values,row
            picture('p3-critical-settled',damaged_values)
            fixture(2,1,32);frames(2);damaged_values[1]=32
            row=sample('p2-healed');assert row['hp']==row['trail']==damaged_values,row
            picture('p2-healed',damaged_values)
            # No private camera callback may be needed to advance a seat's
            # trail. Force the shared fallback while changing only P2.
            r.request('render_pass_refuse',on=1)
            try:
                fixture(2,1,16);frames(80);damaged_values[1]=16
                row=sample('private-views-refused');assert row['hp']==row['trail']==damaged_values,row
            finally:r.request('render_pass_refuse',on=0)
            frames(4);picture('private-views-restored',damaged_values)
            counters={c['name']:c['count'] for c in r.request('mod_counters')['counters']}
            stats=r.request('render_pass_stats');report.update(counters=counters,render_stats=stats)
            assert counters.get('mmx4.coop4.hud-state-ticks',0)>100
            assert counters.get('mmx4.renderer.activated')==1 and not counters.get('mmx4.renderer.arena-failed',0)
            assert stats['verify_checks']>100 and not any(stats[k] for k in ['verify_mismatch','aborted','watchdog','vram_leaks'])
            report['passed']=True;save();print(json.dumps(dict(passed=True,players=a.players,force_x=a.force_x,restore_checks=stats['verify_checks'])))
        except Exception as error:
            report['error']=repr(error);save()
            try:capture_pair(r,root/'failure.png')
            except Exception:pass
            raise
        finally:
            try:r.request('quit')
            except (OSError,RuntimeError,ValueError):pass
            if process.poll() is None:process.terminate()
            process.wait(timeout=15)


if __name__=='__main__':main()
