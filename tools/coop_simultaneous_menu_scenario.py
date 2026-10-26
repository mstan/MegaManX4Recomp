"""Native paired-menu QA. Fixtures prepare inventory; controller input manages it.

Works with the offline Runtime or the synchronized NetplayRuntime adapter.
No fixture sets a cursor, heals a character or installs the selected weapon.
"""
import json
import time
import struct
import zlib


def visible_halves(path,min_visible=.05):
    """Reject empty composed menus, excluding the input overlay and bars."""
    raw=path.read_bytes();assert raw[:8]==b'\x89PNG\r\n\x1a\n'
    pos=8;packed=bytearray()
    while pos<len(raw):
        length=struct.unpack_from('>I',raw,pos)[0];kind=raw[pos+4:pos+8];data=raw[pos+8:pos+8+length];pos+=length+12
        if kind==b'IHDR':
            width,height,depth,color,_,_,interlace=struct.unpack('>IIBBBBB',data)
            assert depth==8 and color in(2,6)and interlace==0
        elif kind==b'IDAT':packed.extend(data)
    channels=3 if color==2 else 4;stride=width*channels;pixels=zlib.decompress(packed);previous=bytearray(stride)
    counts=[0,0];samples=[0,0]
    for y in range(height):
        at=y*(stride+1);kind=pixels[at];row=bytearray(pixels[at+1:at+1+stride]);assert kind<=4
        if kind:
            for i in range(stride):
                a=row[i-channels]if i>=channels else 0;b=previous[i];c=previous[i-channels]if i>=channels else 0
                if kind==1:predict=a
                elif kind==2:predict=b
                elif kind==3:predict=(a+b)//2
                else:
                    p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c)
                    predict=a if pa<=pb and pa<=pc else b if pb<=pc else c
                row[i]=(row[i]+predict)&255
        if height//5<=y<height*4//5 and y%8==0:
            for x in range(0,width,8):
                half=int(x>=width//2);samples[half]+=1
                counts[half]+=max(row[x*channels:x*channels+3])>16
        previous=row
    ratios=[a/b for a,b in zip(counts,samples)]
    assert min(ratios)>min_visible,('Composed menu is empty',path,ratios)
    return ratios


def simultaneous_menus(s, capture=None):
    r=s.r
    r.fixture(5,3);s.load(1,0);r.fixture(5,3)
    r.fixture(7,s.campaign^1,0,1);r.fixture(6,252,16,2)
    for seat in (0,1):r.fixture(2,seat,16)
    x=s.campaign;zero=x^1;events=[]
    peers=getattr(getattr(r,'development',None),'peers',[r])
    render_before=[p.request('render_pass_stats')for p in peers]
    captures=[]

    def frames(count):
        start=struct.unpack('<I',r.read(0x80141bd8,4))[0];deadline=time.monotonic()+20
        while ((struct.unpack('<I',r.read(0x80141bd8,4))[0]-start)&0xffffffff)<count:
            if time.monotonic()>deadline:raise TimeoutError('Native menu clock stopped')
            time.sleep(.015)

    def pulse(mask,seat):
        r.input(mask,seat);frames(6);r.release();frames(6)

    def shot(label):
        if capture:
            for path in capture(label):
                captures.append(dict(path=str(path),visible_halves=visible_halves(path,.01 if label.startswith('campaign-')else .05)))
        else:
            s.sample(label)
            sequence=r.request('present_shot_seq')['seq']
            r.request('present_shot',path=str(s.output/(label+'-pair.png')))
            deadline=time.monotonic()+12
            while r.request('present_shot_seq')['seq']==sequence:
                if time.monotonic()>deadline:raise TimeoutError('Composed menu capture pending')
                time.sleep(.03)
            path=s.output/(label+'-pair.png')
            captures.append(dict(path=str(path),visible_halves=visible_halves(path)))

    def opened(seat):
        pulse(0xfff7,seat)
        s.wait('paired-menu-open',lambda q:q['mode'][:2]==[6,2] and
               q['menu'][0]==1 and q['menu'][1]<3,20)
        deadline=time.monotonic()+30
        while r.read(0x80141bdc,1)[0]:
            assert time.monotonic()<deadline,'Native menu opening fade did not finish'
            time.sleep(.03)

    def resumed(seat):
        pulse(0xfff7,seat)
        return s.wait('paired-menu-resume',lambda q:q['mode'][:2]==[6,0],20)

    opened(1);shot('paired-menus-open')
    # Native X starts on Buster; one Down selects Lightning Web. Zero starts
    # on the first acquired E-tank. Verify real selected weapons after resume.
    pulse(0xffbf,x);pulse(0xbfff,x);shot('x-weapon-selected')
    selected=r.read(0x801754b4,1)[0]
    r.input(0xbfff,zero);frames(4);r.release();frames(2)
    # LAN host input crosses the configured delay before simulation sees it.
    # Retain each chord long enough to apply, rather than replacing it early.
    step=6 if hasattr(r,'development')else 2
    r.input(0xffb7,x)  # Start+Down during native E-tank use.
    frames(step)
    assert list(r.read(0x801721c0,2))==[6,2],'Tank use allowed unpause'
    if not x:assert r.read(0x801754b4,1)[0]!=selected,'Tank use blocked partner navigation'
    r.input(0xbff7,x);frames(step)  # Start+Cross selects another weapon.
    changed=r.players()[x][0x93]
    assert changed!=1 and r.players()[zero][0x5c]&127<32,('No equipment change during healing',changed)
    # Keep Start held while returning to Lightning Web, so neither a tank
    # finishing between inputs nor a held Start can resume the shared world.
    r.input(0xffe7,x);frames(step)  # Start+Up.
    r.input(0xbff7,x);frames(step)
    assert r.players()[x][0x93]==1,'Partner weapon confirmation was blocked'
    r.input(0xfff7,x);frames(80)
    assert list(r.read(0x801721c0,2))==[6,2],'Held Start resumed after healing'
    r.release();frames(2)
    shot('zero-tank-completed');row=resumed(x)
    bodies=r.players()
    assert bodies[zero][0x5c]&127==32 and bodies[x][0x5c]&127==16,row
    assert bodies[x][0x93]==1 and bodies[zero][0x93]==0,[b[0x93]for b in bodies]
    events.append(dict(healer=zero,hp=[b[0x5c]&127 for b in bodies],weapons=[b[0x93]for b in bodies]))

    # Reopening resets X's cursor to the selected weapon (index2). Four Down
    # edges reach tank0 through the original weapon grid; Zero can navigate
    # independently before confirmation. No E-tank option was enabled by QA.
    opened(0)
    for _ in range(4):pulse(0xffbf,x)
    pulse(0xffbf,zero);shot('independent-tank-selection')
    r.input(0xbfff,x);frames(4);r.release();frames(2)
    r.input(0xfff7,zero);frames(8)
    assert list(r.read(0x801721c0,2))==[6,2],'Partner resumed during X tank use'
    r.release();frames(80)
    row=resumed(zero);bodies=r.players()
    assert all((b[0x5c]&127)==32 for b in bodies),row
    events.append(dict(healer=x,hp=[b[0x5c]&127 for b in bodies]))
    shot('both-management-results')
    # Each prepared tank contains exactly 16 HP. Spend a third 16 HP refill
    # across the inventories: a shared two-tank pool would now be empty.
    # This proves the default menu option actually
    # requires private contents despite QA leaving separate_etanks false.
    r.fixture(2,zero,16);opened(zero)
    pulse(0xbfff,zero);frames(80)
    row=resumed(x);bodies=r.players()
    assert all((b[0x5c]&127)==32 for b in bodies),('Private third tank missing',row)
    events.append(dict(healer=zero,third_16_hp_refill=True,hp=[b[0x5c]&127 for b in bodies]))
    snapshot_restored=False
    if not hasattr(r,'development'):
        opened(0);selected=r.read(0x801754b4,1)[0]
        def state_operation(op):
            generation=r.request('savestate_status')['generation']
            r.request('savestate',op=op,slot=1);deadline=time.monotonic()+25
            while time.monotonic()<deadline:
                status=r.request('savestate_status')
                if status['generation']!=generation and not status['pending']:
                    assert status['last_ok']and status['last_op']==op,status
                    return
                time.sleep(.03)
            raise TimeoutError('Menu snapshot '+op+' pending')
        state_operation('save');pulse(0xffbf,0);state_operation('load')
        assert list(r.read(0x801721c0,2))==[6,2]
        assert r.read(0x801754b4,1)[0]==selected,'Personal menu snapshot lost its cursor'
        shot('paused-snapshot-restored');resumed(1);snapshot_restored=True
    # Equipment must be usable after the shared exit, not only selected in UI.
    def x_projectiles():
        if x:
            raw=r.read(r.diagnostic()+0x480,16*8);stride=8
        else:
            raw=r.read(0x801406f8,16*0x9c);stride=0x9c
        return sum(bool(raw[i])for i in range(0,len(raw),stride))
    before=x_projectiles();r.input(0x7fff,x);after=before
    for _ in range(15):
        frames(2);after=x_projectiles()
        if after>before:break
    r.release()
    assert after>before,('Selected X weapon did not fire after pause',before,after)
    shot('selected-weapon-fired')
    # Either seat can open shared Quit; its initiator controls confirmation.
    opened(1);pulse(0xfffe,1)
    s.wait('p2-quit-modal',lambda q:q['menu']==[1,6],20)
    s.wait('p2-quit-ready',lambda q:q['menu_detail'][4:8]==[1,6,2,1] and not q['warning'],20)
    shot('p2-quit-modal')
    pulse(0xbfff,0)
    assert list(r.read(0x801721c0,2))==[6,2] and r.read(0x801754a5,1)[0]==6
    pulse(0xbfff,1)
    s.wait('p2-quit-cancelled',lambda q:q['menu'][0]==1 and q['menu'][1]<3,20)
    pulse(0xfffe,0)
    s.wait('campaign-quit-modal',lambda q:q['menu']==[1,6],20)
    s.wait('campaign-quit-ready',lambda q:q['menu_detail'][4:8]==[1,6,2,1] and not q['warning'],20)
    shot('campaign-quit-modal')
    pulse(0xffb7,1)
    assert list(r.read(0x801721c0,2))==[6,2] and r.read(0x801754a5,1)[0]==6
    pulse(0xbfff,0) # Native default is Continue.
    s.wait('campaign-quit-cancelled',lambda q:q['menu'][0]==1 and q['menu'][1]<3,20)
    resumed(1)
    opened(0)
    option_path=[]
    for _ in range(16):
        cursor=r.read(0x801754b4,1)[0];option_path.append(cursor)
        if cursor==14:break
        pulse(0xffbf if cursor<10 or cursor==13 else 0xffdf,0)
    assert r.read(0x801754b4,1)[0]==14,('Native Options entry unreachable',option_path)
    pulse(0xbfff,0)
    s.wait('campaign-options-modal',lambda q:q['menu']==[1,5],20)
    frames(50);shot('campaign-options-modal')
    pulse(0xfff7,1)
    assert list(r.read(0x801721c0,2))==[6,2] and r.read(0x801754a5,1)[0]==5
    pulse(0xfff7,0)
    s.wait('campaign-options-returned',lambda q:q['menu'][0]==1 and q['menu'][1]<3,20)
    resumed(1)
    # A surviving P2 can manage and resume without applying equipment to a
    # dead campaign owner. Damage is a fixture; death and menu exit are native.
    r.fixture(2,0,0);frames(30);opened(1)
    pulse(0xfff7,0)
    assert list(r.read(0x801721c0,2))==[6,2],'Dead P1 resumed the team'
    resumed(1)
    assert r.players()[0][0x5c]&127==0,'Menu revived the dead campaign owner'
    render_after=[p.request('render_pass_stats')for p in peers]
    for before,after in zip(render_before,render_after):
        for key in('verify_mismatch','aborted','watchdog','vram_leaks'):
            assert after[key]==before[key],('Menu render transaction failed',key,before,after)
        assert after['verify_checks']>before['verify_checks'],after
    result=dict(native=True,campaign=s.campaign,two_character_menus=True,
        independent_weapon_and_tank_management=True,tank_blocks_unpause=True,
        partner_equipment_during_healing=True,held_start_not_queued=True,
        either_player_resumes=True,required_private_etanks=True,paused_snapshot_restored=snapshot_restored,
        prepared_energy_per_tank=16,prepared_energy_per_seat=32,total_native_healing=48,
        p2_quit_confirmation=True,campaign_confirmation_serialized=True,shared_options_returned=True,surviving_p2_menu=True,
        native_weapon_fired_after_resume=True,render_pass=render_after,captures=captures,events=events)
    (s.output/'simultaneous-menu-results.json').write_text(json.dumps(result,indent=2))
    return result
