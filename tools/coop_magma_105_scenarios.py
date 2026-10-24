"""Native Volcano quake and authored pillar-sequence regression controls."""
import json
import struct
import time
from coop_debug import capture_pair
from coop_fortress_followup_scenario import actors,cameras


def pillar_tiles(r):
    layout,blocks=struct.unpack('<II',r.read(0x1f800004,8))
    width=r.read(0x80172224,1)[0]
    tiles=[]
    for y in range(624,721,16):
        for x in range(160,225,16):
            screen=r.read(layout+(y//256)*width+x//256,1)[0]
            address=blocks+screen*512+((y//16)&15)*32+((x//16)&15)*2
            tiles.append(struct.unpack('<H',r.read(address,2))[0])
    return tiles


def magma_pillars(s,qualify=True):
    results=[]
    for leader in (1,0):
        s.load(4,1);s.r.fixture(5,3)
        before=pillar_tiles(s.r);rows=[]
        # Protect the setup above the authored pillars. These position-only
        # fixtures cannot arm a controller, emit a fireball or edit terrain.
        for n in range(60):
            s.r.teleport(leader,640,400)
            row=s.sample(f'pillars-{leader}-{n}');row['actors']=actors(s)
            rows.append(row)
            if n in (0,30,59):capture_pair(s.r,s.output/f'pillars-{leader}-{n}.png')
            time.sleep(.035)
        after=pillar_tiles(s.r)
        sequence=[a for row in rows for a in row['actors'] if a['pool']=='controller' and a['type']==34]
        early=[a for row in rows for a in row['actors'] if a['pool']=='secondary' and
               a['type']==33 and a['x'] in (192,320,672)]
        result=dict(leader=leader,armed=any(a['flags'][4] for a in sequence),
                    fireball_x=sorted(set(a['x'] for a in early)),
                    changed_tiles=sum(a!=b for a,b in zip(before,after)),
                    before_tiles=before,after_tiles=after,
                    minimum_hp=[min(row['players'][seat]['hp'] for row in rows) for seat in (0,1)])
        (s.output/f'pillars-{leader}.json').write_text(json.dumps(dict(result=result,rows=rows),indent=2))
        if qualify:
            assert result['armed'] and result['changed_tiles']==16 and result['fireball_x']==[192,320,672],result
            assert all(result['minimum_hp']),result
        results.append(result)
    return dict(pillars=results,observation_only=not qualify)


def _magma_quake(s,qualify=True):
    leader=s.victory_seat;nearby=leader^1
    s.load(4,0);s.r.fixture(5,3)
    s.r.input(0xffdf,leader)
    try:s.wait('quake-leading-walk',lambda row:row['players'][leader]['x']>=1000,16)
    finally:s.r.release()
    rows=[]
    state=next(c['count'] for c in s.r.request('mod_counters')['counters'] if c['name']=='mmx4.coop.split-state')
    for n in range(90):
        row=s.sample('quake-'+str(n));row['views']=cameras(s)
        row['shake']=[s.r.read(state+16+seat*252+0x34,20).hex() for seat in (0,1)]
        row['actors']=actors(s);rows.append(row)
        if n in (0,30,60,89):capture_pair(s.r,s.output/f'quake-{n}.png')
        time.sleep(.025)
    (s.output/'quake-rows.json').write_text(json.dumps(rows,indent=2))
    timers=sorted(set(bytes.fromhex(row['shake'][nearby])[2] for row in rows if row['views'][nearby][26]&16))
    origins=sorted(set(row['views'][nearby][5] for row in rows))
    result=dict(leader=leader,native_timers=timers,camera_x=origins,
                minimum_hp=[min(row['players'][seat]['hp'] for row in rows) for seat in (0,1)])
    if qualify:assert len(timers)>=3 and len(origins)>1 and all(result['minimum_hp']),result
    return result


def magma_quake(s,qualify=True):
    output=s.output;seat=s.victory_seat;results=[]
    try:
        for leader in (1,0):
            s.victory_seat=leader;s.output=output/f'quake-leader-{leader}';s.output.mkdir()
            results.append(_magma_quake(s,qualify))
    finally:s.output=output;s.victory_seat=seat
    return dict(local_quakes=results)


def magma_slope_rumble(s,qualify=True):
    """First-slope shared rolling rumble with native damage left enabled.

    God mode changes this reproduction: it keeps early boulders alive beside
    the leading hero instead of letting them retire after hitting the waiting
    partner. Stage/HP setup alone is permitted here; input drives the descent.
    """
    results=[]
    state=next(c['count'] for c in s.r.request('mod_counters')['counters']
               if c['name']=='mmx4.coop.split-state')
    for leader in (1,0):
        s.load(4,0);s.r.fixture(5,0)
        for seat in (0,1):s.r.fixture(2,seat,48)
        rows=[];deadline=time.monotonic()+20;stopped=None
        s.r.input(0xdfdf,leader) # Native Right + Dash; partner remains at spawn.
        try:
            while time.monotonic()<deadline:
                row=s.sample(f'slope-rumble-{leader}-{len(rows)}')
                views=s.r.read(state+16,504)
                row['views']=[list(struct.unpack('<42h',views[v*252:v*252+0x54])) for v in (0,1)]
                row['shake']=[views[v*252+0x34:v*252+0x48].hex() for v in (0,1)]
                rows.append(row)
                if row['players'][leader]['x']>=1160 and stopped is None:
                    s.r.release();stopped=time.monotonic()
                    capture_pair(s.r,s.output/f'slope-rumble-leader-{leader}.png')
                if stopped is not None and time.monotonic()-stopped>1.5:break
                time.sleep(.025)
        finally:s.r.release()
        (s.output/f'slope-rumble-{leader}.json').write_text(json.dumps(rows,indent=2))
        descent=[r for r in rows if 400<=r['players'][leader]['x']<1100]
        timers=sorted(set(bytes.fromhex(r['shake'][leader])[2] for r in descent
                          if r['views'][leader][26]&16))
        result=dict(leader=leader,reached_bottom=stopped is not None,descent_samples=len(descent),
                    shaking_samples=sum(bool(r['views'][leader][26]&16) for r in descent),
                    native_timers=timers,
                    waiting_partner_hit=min(r['players'][leader^1]['hp'] for r in rows)<48,
                    minimum_hp=[min(r['players'][v]['hp'] for r in rows) for v in (0,1)])
        if qualify:
            assert result['reached_bottom'] and len(descent)>=5 and len(timers)>=3,result
            assert result['waiting_partner_hit'] and all(result['minimum_hp']),result
        results.append(result)
    return dict(slope_rumbles=results,observation_only=not qualify)
