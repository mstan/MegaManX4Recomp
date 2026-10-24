"""Bounded native 3/4-seat LAN probe. No simulation RAM writes online."""
import concurrent.futures,json,os,shutil,struct,subprocess,sys,time
from pathlib import Path
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--exe',type=Path,required=True)
parser.add_argument('--disc',type=Path,required=True)
parser.add_argument('--bios',type=Path)
parser.add_argument('--game-root',type=Path,default=Path(__file__).resolve().parents[1])
parser.add_argument('--report-dir',type=Path,required=True)
parser.add_argument('--players',type=int,choices=(3,4),default=4)
parser.add_argument('--roster',choices=('default','all-x'),default='default')
parser.add_argument('--tcp-base',type=int,default=17950)
parser.add_argument('--udp-base',type=int,default=18080)
options=parser.parse_args()
source=options.game_root.resolve();executable=options.exe.resolve();build=executable.parent
bios=(options.bios or build/'bios/openbios.bin').resolve();disc=options.disc.resolve()
for path in (executable,bios,disc):
    if not path.is_file():parser.error('File is missing: '+str(path))
sys.path.insert(0,str(source/'tools'))
from coop_debug import Runtime,capture_pair
from test_mmx4_coop_netplay import parse_crc_log
count=options.players;alternate=options.roster=='all-x'
if alternate and count!=4:parser.error('The all-X roster uses four seats')
root=options.report_dir.resolve();root.mkdir(parents=True,exist_ok=False)
pool=concurrent.futures.ThreadPoolExecutor(max_workers=count)
peers=[];processes=[];logs=[];dirs=[];addresses=[];report={'players':count,'checks':[]}
def save():
    (root/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
def all_request(command,**kw):
    return list(pool.map(lambda r:r.request(command,**kw),peers))
def input(seat,held=0):
    return peers[seat].request('set_input',layer='host',buttons=hex((~held)&65535),lx=128,ly=128,rx=128,ry=128)
def statuses():
    states=all_request('netplay_status')
    for state in states:
        assert state['active']==1 and state['rollback']==0 and state['players']==count,state
        assert state['desync']==0,state
    return states
def wait_ticks(ticks,timeout=60):
    start=min(s['tick'] for s in statuses());deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        states=statuses()
        if min(s['tick'] for s in states)>=start+ticks:return states
        time.sleep(.05)
    raise TimeoutError(('no simulation progress',start,states))
def observe(r,addr):
    b=r.read(addr,0xd00);rows=[]
    for seat in range(count):
        body=b[0x100+seat*256:0x1e4+seat*256];cam=b[0x500+seat*256:0x5fc+seat*256]
        rows.append({'char':body[2],'state':body[4],'pose':body[5],
            'x':struct.unpack_from('<i',body,8)[0]/65536,'y':struct.unpack_from('<i',body,12)[0]/65536,
            'hp':body[0x5c]&127,'armor':body[0xa7],'arm':body[0xb8],
            'camera':[struct.unpack_from('<i',cam,8)[0]/65536,struct.unpack_from('<i',cam,12)[0]/65536]})
    return {'header':struct.unpack_from('<6I',b),'actors':rows,'mode':list(r.read(0x801721c0,2))}
def observations():return list(pool.map(lambda pair:observe(*pair),zip(peers,addresses)))
def checkpoint(label):
    states=statuses();crcs=[parse_crc_log(d/'runtime.log') for d in dirs]
    common=set(crcs[0])
    for crc in crcs[1:]:common&=set(crc)
    for tick in common:assert len({crc[tick] for crc in crcs})==1,('CRC disagreement',tick,[c[tick] for c in crcs])
    assert len(common)>=3,('insufficient matching core/mod CRCs',label,len(common))
    row={'label':label,'statuses':states,'observations':observations(),'matched_crc_count':len(common),'through_tick':max(common)}
    report['checks'].append(row);save();print(label,'CRC samples',len(common),'through',max(common),flush=True);return row
try:
    for seat in range(count):
        folder=root/('p'+str(seat+1));folder.mkdir(exist_ok=True);dirs.append(folder)
        for name in ('assets','mods','bios'):shutil.copytree(build/name,folder/name,dirs_exist_ok=True)
        for p in [executable,*build.glob('*.dll')]:shutil.copy2(p,folder/p.name)
        state='format_version = 2\n[[feature]]\npackage_id = "mmx4.coop4"\nid = "coop"\nenabled = true\n'
        if count==3:state+='[feature.values]\np4 = "none"\n'
        elif alternate:state+='[feature.values]\np1 = "zero"\np2 = "x"\np3 = "force_x"\np4 = "ultimate_x"\n'
        (folder/'mods/state.toml').write_text(state,encoding='utf-8',newline='\n')
        config=(source/'game.toml').read_text(encoding='utf-8').replace('exe = "mmx4/SLUS_005.61"','exe = '+json.dumps((source/'mmx4/SLUS_005.61').as_posix()))
        (folder/'game.toml').write_text(config,encoding='utf-8',newline='\n')
        (folder/'settings.toml').write_text('[video]\nfullscreen = false\nwindow_width = 640\ninternal_resolution = "native"\n[audio]\nvolume = 0\n',encoding='utf-8',newline='\n')
        env=os.environ.copy()
        for key,sub in dict(TEMP='temp',TMP='temp',APPDATA='appdata',LOCALAPPDATA='local',PSX_RESIDENT_CACHE='resident',PSX_OVERLAY_CACHE_DIR='overlay',PSX_OVERLAY_CAPTURE_ROOT='captures',MESA_SHADER_CACHE_DIR='shaders',__GL_SHADER_DISK_CACHE_PATH='shaders').items():
            p=folder/sub;p.mkdir(exist_ok=True);env[key]=str(p)
        env.update(SDL_AUDIODRIVER='dummy',MMX4_RECOMP_DATA_DIR=str(folder),PSX_OVERLAY_AUTOCOMPILE_OFF='1',PSX_RENDER_PASS_VERIFY='1',PSX_NET_TRANSPORT='lan',PSX_NET_MODE='delay',PSX_NET_SLOTS=str(count))
        args=[str(folder/executable.name),'--hidden-window','--no-launcher','--renderer','opengl','--debug-port',str(options.tcp_base+seat),
            '--game',str(folder/'game.toml'),'--bios',str(bios),'--disc',str(disc),
            '--memcard-dir',str(folder/'saves'),'--netplay','--net-slot',str(seat),'--net-input-player','0','--net-bind','127.0.0.1:'+str(options.udp_base+seat),
            '--net-delay','2','--net-session-id','4404']
        if seat:args+=['--net-peer','127.0.0.1:'+str(options.udp_base)]
        log=(folder/'runtime.log').open('wb');logs.append(log)
        processes.append(subprocess.Popen(args,cwd=folder,env=env,stdout=log,stderr=log,creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0))
        peers.append(Runtime(options.tcp_base+seat))
    deadline=time.monotonic()+120
    for i,r in enumerate(peers):
        while time.monotonic()<deadline:
            assert processes[i].poll() is None,('peer exited',i)
            try:r.request('netplay_status');break
            except OSError:time.sleep(.1)
        addresses.append(next(c['count'] for c in r.request('mod_counters')['counters'] if c['name']=='mmx4.coop4.diagnostic-address'))
        input(i)
    print('All peers started',flush=True)
    deadline=time.monotonic()+240;pulse=0;previous=None
    while time.monotonic()<deadline:
        states=statuses();modes=[tuple(r.read(0x801721c0,2)) for r in peers]
        if modes!=previous:print('native modes',modes,'ticks',[s['tick'] for s in states],flush=True);previous=modes
        if all(mode==(6,0) for mode in modes):
            input(0);wait_ticks(20)
            obs=observations()
            if all(o['header'][2] and all(p['hp'] and p['state']==1 for p in o['actors']) for o in obs):break
            continue
        input(0,0x80 | (8 if pulse%2==0 else 0x4000));wait_ticks(12)
        input(0);wait_ticks(10);pulse+=1
    else:raise TimeoutError('Native boot did not enroll all seats')
    checkpoint('intro-enrolled')
    actors=observations()[0]['actors']
    assert [p['char'] for p in actors]==([1,0,0,0] if alternate else [0,1,0,1][:count]),actors
    if alternate:assert [(p['armor'],p['arm']) for p in actors[2:]]==[(15,1),(15,2)],actors
    for seat in range(count):
        before=observations()[0]
        input(seat,0x20);wait_ticks(20);input(seat);wait_ticks(6)
        after=observations()[0]
        assert after['actors'][seat]['x']>before['actors'][seat]['x']+15,('seat did not move',seat,before,after)
        for other in range(count):
            if other!=seat:assert abs(after['actors'][other]['x']-before['actors'][other]['x'])<2,('input leaked',seat,other,before,after)
        input(seat,0x8000);wait_ticks(8);input(seat);wait_ticks(8)
        checkpoint('seat-'+str(seat+1)+'-move-attack')
    # Hold and release the native fire button separately for every X. This
    # exercises deferred charge effects with normal/Force/Ultimate banks live.
    for seat,actor in enumerate(actors):
        if actor['char']==0:
            input(seat,0x8000);wait_ticks(110);input(seat);wait_ticks(25)
            checkpoint('seat-'+str(seat+1)+'-charged-shot')
    # Separate the last seat from the idle group using ordinary movement.
    # No actor is teleported across a trigger for this check.
    leader=count-1;before=observations()[0]
    input(leader,0x20);wait_ticks(140);input(leader);wait_ticks(8)
    separated=checkpoint('last-seat-leads')
    after=separated['observations'][0]
    assert after['actors'][leader]['x']-after['actors'][0]['x']>250,after
    assert after['actors'][leader]['camera'][0]-after['actors'][0]['camera'][0]>200,after
    for i,r in enumerate(peers):capture_pair(r,dirs[i]/'own-view.png')
    report['render_stats']=all_request('render_pass_stats');report['counters']=all_request('mod_counters')
    assert all(s['verify_mismatch']==0 and s['aborted']==0 for s in report['render_stats']),report['render_stats']
    for counters in report['counters']:
        values={row['name']:row['count'] for row in counters['counters']}
        assert values.get('mmx4.coop4.local-view-committed',0)>10,values
        assert values.get('mmx4.coop4.local-view-state-leak',0)==0,values
        for seat,actor in enumerate(actors):
            if actor['char']==0:assert values.get('mmx4.coop4.p'+str(seat+1)+'-active-shot-ticks',0)>0,values
    checkpoint('final');report['passed']=True;save();print('Native network qualification passed',flush=True)
except Exception as error:
    report['error']=str(error);save();raise
finally:
    for process in processes:
        if process.poll() is None:process.terminate()
    for process in processes:process.wait(timeout=15)
    for log in logs:log.close()
    pool.shutdown()
