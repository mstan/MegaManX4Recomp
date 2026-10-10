"""Bounded original-disc regression scenarios using hidden private copies.

Requires a development build with TCP tools. Stage/checkpoint/teleport/health
fixtures only set up scenarios; controller input drives the reported behavior.
Never launches a visible window or uses a player's settings or memory cards.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time
from coop_debug import Runtime
from test_mmx4_coop_doors import snapshot


class Scenarios:
    def __init__(self, runtime, output, campaign, victory_seat=1,clears=16):
        self.r, self.output, self.campaign = runtime, output, campaign
        self.victory_seat=victory_seat
        self.expected_clears=clears
        self.events = []
        self.watch_type=None
        self.watch_enemy=None
        self.first_boss=None

    def sample(self, label, capture=False):
        row = dict(label=label, **snapshot(self.r))
        # The co-op body mirror stops outside gameplay; shared dispatcher
        # fields remain canonical and must be read live for reward/title UI.
        shared=self.r.read(0x801721c0,0x26)
        row['mode']=list(shared[:4])
        row['stage'],row['section']=shared[12:14]
        row['checkpoint']=shared[0x1d]
        row['blocked']=self.r.read(0x801721dc,1)[0]
        mirror=self.r.read(self.r.diagnostic(),0x6d0)
        row['native_health']=[list(mirror[at+0x5c:at+0x5f]) for at in (0x300,0x100)]
        row['vehicle_health']=[mirror[at+0x5c] for at in (0x620,0x200)]
        row['campaign_health']=mirror[0x445]
        row['freeze']=mirror[0x410]
        row['warning']=self.r.read(0x80141bdc,1)[0]
        row['menu']=list(self.r.read(0x801754a4,2))
        row['personal_locks']=[[mirror[at+j] for j in (0x67,0xbc,0xc0,0xc3,0xc4)]
                               for at in (0x300,0x100)]
        row['orb_capture']=[mirror[at+0xba] for at in (0x300,0x100)]
        row['bounds']=list(struct.unpack('<4h',self.r.read(0x801419d4,8)))
        if self.watch_enemy is not None:
            row['enemies']=[p for p in self.enemies() if p['type']==self.watch_enemy]
        self.events.append(row)
        if self.watch_type is not None and self.first_boss is None:
            # Allocated slots retain previous actor fields until native init.
            self.first_boss=next((p for p in self.enemies() if p['type']==self.watch_type and
                                  p['state'][0]!=0 and p['hp']>1),None)
        if capture:
            self.r.screenshot(self.output / (label + '.png'))
        return row

    def enemies(self):
        pool=self.r.read(0x8013bed0,48*0x9c)
        result=[]
        for i in range(48):
            p=pool[i*0x9c:(i+1)*0x9c]
            if p[0]:
                result.append(dict(slot=i,type=p[1],hp=p[0x5c]&127,state=list(p[4:7]),
                                   timer=struct.unpack_from('<H',p,0x7c)[0],
                                   captured=p[0x8d],capture_timer=struct.unpack_from('<H',p,0x8a)[0],
                                   x=struct.unpack_from('<i',p,8)[0]/65536,
                                   y=struct.unpack_from('<i',p,12)[0]/65536))
        return result

    def enemy_activation(self):
        self.watch_enemy=12
        cases=[]
        for seat in (1,0):
            self.load(1,0)
            self.r.teleport_both(500,427)
            self.r.teleport(seat,540,427)
            self.wait('plant-idle-'+str(seat),lambda s:any(
                p['type']==12 and abs(p['x']-704)<4 and p['state'][:2]==[1,2] and p['timer']>0
                for p in self.enemies()),5)
            actor=next(p for p in self.enemies() if p['type']==12 and
                       abs(p['x']-704)<4 and p['state'][:2]==[1,2] and p['timer']>0)
            self.r.input(0xffdf,seat)
            try:
                row=self.wait('plant-triggered-'+str(seat),lambda s:any(
                    # Native phase3 immediately dispatches 8004BCC8, which
                    # enters phase4 before the completed-frame observation.
                    p['slot']==actor['slot'] and p['type']==12 and p['state'][1]==4
                    for p in self.enemies()),3)
            finally:self.r.release()
            # The predicate reads the enemy after the body mirror. Refresh
            # the completed player frame after releasing the movement input.
            row=self.sample('plant-trigger-confirmed-'+str(seat))
            triggered=next(p for p in self.enemies() if p['slot']==actor['slot'])
            assert abs(row['players'][seat^1]['x']-triggered['x'])>=144,row
            assert abs(row['players'][seat]['x']-triggered['x'])<144,row
            assert all(p['hp']>0 for p in row['players']),row
            self.sample('native-plant-activation-'+str(seat),True)
            cases.append(dict(seat=seat,before=actor,after=triggered,players=row['players']))
        self.watch_enemy=None
        return dict(native_distance_activation=cases)

    def wait(self, label, predicate, timeout=15, confirm=False):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            row = self.sample(label)
            if predicate(row):
                return row
            if confirm and (row['blocked'] or row['mode'][0] in (7,9,11)):
                for seat in (0, 1):
                    self.r.input(0xbfff, seat)
                time.sleep(.03)
                self.r.release()
            time.sleep(.02)
        raise TimeoutError(f'{label}: {row}')

    def load(self, stage, section, checkpoint=None):
        self.r.release()
        if self.r.read(0x801721c0,1)[0] in (3,9):
            deadline=time.monotonic()+30
            while time.monotonic()<deadline:
                if self.r.read(0x801721c0,1)[0]==6:break
                self.r.input(0xbfff,0);time.sleep(.06)
                self.r.release();time.sleep(.06)
            else:raise TimeoutError('Native stage selection did not enter gameplay')
            self.r.wait_playable(30,advance_dialogue=True)
        self.r.fixture(1, stage, section, self.campaign)
        self.r.wait_playable(45, advance_dialogue=True)
        if checkpoint is not None:
            self.r.fixture(3, checkpoint)
            self.r.wait_playable(45, advance_dialogue=True)

    def jet(self):
        self.load(5, 0)
        counts = []
        for _ in range(20):
            actors = self.r.read(0x80142f98, 32 * 0x30)
            counts.append(sum(actors[i * 0x30] != 0 and actors[i * 0x30 + 1] == 27
                              for i in range(32)))
            time.sleep(.02)
        mirror = self.r.read(self.r.diagnostic(), 0x6d0)
        pointers = [struct.unpack_from('<I', mirror, offset + 0xa0)[0] for offset in (0x620, 0x200)]
        counters = self.r.request('mod_counters')['counters']
        shared = sum(c['count'] for c in counters if c['name'] == 'mmx4.coop.shared-ride-ready')
        assert max(counts) <= 1 and shared and pointers[0] == pointers[1] != 0, (counts, shared, pointers)
        self.sample('jet-shared-ready', True)
        before = self.sample('jet-before-death')
        self.r.fixture(2, 1, 0x80)
        self.wait('jet-p2-dead', lambda s: s['players'][1]['state'] == 3, 15)
        self.r.fixture(1, 5, 1, self.campaign)
        self.wait('jet-section-transfer', lambda s: s['stage'] == 5 and s['section'] == 1 and
                  s['mode'][:2] == [6, 0] and s['players'][0]['state'] == 1 and
                  s['players'][1]['state'] == 3, 45)
        time.sleep(.4)
        row = self.sample('jet-section-corpse', True)
        vehicle = self.r.read(self.r.diagnostic() + 0x200, 0xb0)
        assert row['players'][1]['hp'] == 0 and row['players'][1]['state'] == 3
        assert not vehicle[0] and not vehicle[3]
        assert row['players'][0]['hp'] > 0 and row['lives'] == before['lives']
        return dict(ready_peak=max(counts), ready_pointers=pointers, shared_callbacks=shared,
                    corpse_stays_dead=True, shared_retry_lives=row['lives'])

    def door(self, owner, label):
        self.r.input(0xffdf, owner)
        try:
            self.wait(label + '-entered', lambda s: s['players'][owner]['door'] != 0, 8)
        finally:
            self.r.release()
        row = self.wait(label + '-returned', lambda s:
                        not s['blocked'] and all(p['active'] and p['visible'] and
                        p['state'] == 1 and p['action'] == 2 and p['grounded'] and
                        not p['door'] and not p['script'] for p in s['players']), 25, True)
        assert all(p['hp'] > 0 for p in row['players'])
        assert abs(row['players'][0]['y'] - row['players'][1]['y']) <= 1
        self.sample(label + '-finished', True)
        return row

    def cyber(self):
        self.load(6, 1, 1)
        self.r.teleport_both(568, 2507)
        self.r.teleport(1, 600, 2506 if self.campaign == 0 else 2507)
        self.door(1, 'cyber-p2-outer')
        self.door(1, 'cyber-p2-inner')
        return dict(outer=True, inner=True, passenger_alive=True)

    def intro(self):
        self.load(0, 1, 2)
        self.r.teleport_both(4208, 443)
        self.r.input(0xffdf, 1)
        try:
            row = self.wait('intro-p2-led-script', lambda s:
                            s['blocked'] or any(p['door'] or p['script'] for p in s['players']), 12)
        finally:
            self.r.release()
        self.sample('intro-p2-led-entry', True)
        self.wait('intro-script-released', lambda s: not s['blocked'] and
                  all(not p['door'] and not p['script'] and p['hp'] > 0 for p in s['players']), 30, True)
        return dict(p2_first_approach=True, native_arena_script_released=True,
                    note='Scripted arena approach; not a separate C4 door qualification')

    def combat(self):
        self.intro()
        # Zero's campaign has a Mac encounter before the dragon arena.
        # Wait for the native dragon, rather than treating a script gap as
        # the start of the fight.
        deadline=time.monotonic()+20
        try:
            while time.monotonic()<deadline:
                boss=next((p for p in self.enemies() if p['type']==8 and
                           p['state'][0] and p['hp']>1),None)
                if boss:
                    break
                row=self.sample('intro-find-dragon')
                for seat in (0,1):
                    self.r.input(0xbfff if row['blocked'] else 0xffdf,seat)
                time.sleep(.05)
                self.r.release()
                time.sleep(.03)
            else:
                raise TimeoutError('Native dragon arena did not initialize')
        finally:
            self.r.release()
        self.wait('arena-ordinary-play',lambda s:not s['blocked'] and not s['freeze'] and
                  not s['warning'] and all(p['active'] and p['visible'] and
                  p['state']==1 and p['action']==2 for p in s['players']),15,True)
        enemies=self.enemies()
        boss=next(p for p in enemies if p['type']==8 and p['hp']>0)
        self.r.input(0xfff7,1);time.sleep(.1);self.r.release()
        self.wait('p2-personal-pause',lambda s:s['mode'][:2]==[6,2] and s['menu'][0]==1 and s['menu'][1]<3,5)
        time.sleep(.05)
        self.r.input(0xfff7,1);time.sleep(.1);self.r.release()
        self.wait('p2-personal-resume',lambda s:s['mode'][:2]==[6,0],5)
        self.r.fixture(6,boss['slot'],16)
        before=next(p for p in self.enemies() if p['slot']==boss['slot'])
        counters=self.r.request('mod_counters')['counters']
        start=sum(c['count'] for c in counters if c['name']=='mmx4.coop.p2-enemy-hit')
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            target=next((p for p in self.enemies() if p['slot']==boss['slot']),None)
            if target:
                self.r.teleport_both(int(target['x'])-48,int(target['y'])+24)
            self.r.input(0x7fff,1);time.sleep(1.0 if self.campaign else .08)
            self.r.release();time.sleep(.08)
            enemies=self.enemies()
            after=next((p for p in enemies if p['slot']==boss['slot']),None)
            counters=self.r.request('mod_counters')['counters']
            count=sum(c['count'] for c in counters if c['name']=='mmx4.coop.p2-enemy-hit')
            if count>start and (after is None or after['hp']<before['hp']):
                self.sample('p2-post-pause-native-enemy-hit',True)
                return dict(character=1-self.campaign,before=before,after=after,p2_hit_callbacks=count-start)
        raise AssertionError(f'P2 attacks did not damage native enemy after pause: {before}/{after}')

    def jet_boss(self):
        cases=[]
        for owner in (0,1):
            self.watch_type=None;self.first_boss=None
            self.load(5,1,1)
            before=self.sample('jet-door-spawn-'+str(owner),True)
            height=int(before['players'][0]['y'])
            self.r.teleport_both(16848,height)
            self.r.teleport(owner,16936,height)
            self.watch_type=56
            self.door(owner,'jet-owner-'+str(owner))
            boss=self.first_boss
            assert boss is not None,(owner,self.enemies())
            # The native shared boss entrance (80070514) flies to a position
            # derived from camera bounds; authored Y138 is not its fight Y.
            assert 17024<=boss['x']<=17344 and 0<=boss['y']<=384,boss
            cases.append(dict(owner=owner,first_boss=boss,authored_spawn=[17146,138],
                              players=self.sample('jet-boss-live-'+str(owner))['players'],
                              camera_bounds=self.sample('jet-boss-bounds-'+str(owner))['bounds']))
        self.watch_type=None
        # The active boss flies between samples; compare the native room
        # rather than asynchronous positions from different flight frames.
        assert cases[0]['camera_bounds']==cases[1]['camera_bounds'],cases
        return dict(owners=cases,both_bosses_in_native_room=True)

    def spider_visual(self):
        self.load(1,0)
        start=self.sample('spider-start',True)
        probes=[]
        for x in (400,416,432,448,464):
            self.r.teleport_both(x,394)
            self.r.teleport(1,x+24,393 if self.campaign==0 else 394)
            time.sleep(.08)
            row=self.sample('spider-water-'+str(x),True)
            pool=self.r.read(0x8013e510,32*0x70)
            effects=[]
            for i in range(32):
                p=pool[i*0x70:(i+1)*0x70]
                if p[0] and p[1]==7:
                    effects.append(dict(slot=i,variant=p[2],visible=p[3],state=p[4],
                                        x=struct.unpack_from('<i',p,8)[0]/65536,
                                        y=struct.unpack_from('<i',p,12)[0]/65536))
            probes.append(dict(players=row['players'],effects=effects))
        for seat in (0,1):
            assert any(all(any(e['visible'] and abs(e['x']-(p['players'][seat]['x']+side))<1 and
                              abs(e['y']-p['players'][seat]['y'])<1 for e in p['effects'])
                           for side in (-8,8)) for p in probes),('Missing native water pair',seat)
        # Reload to restore the native camera-area bounds at the start.
        self.load(1,0)
        time.sleep(.15)
        peak=0
        self.r.input(0xdfdf,1)
        try:
            for i in range(12):
                trails=self.r.read(self.r.diagnostic()+0x500,3*0x60)
                active=sum(bool(trails[n*0x60] and trails[n*0x60+3]) for n in range(3))
                if active>peak:
                    peak=active
                    self.sample('p2-dash-trails-'+str(i),True)
                time.sleep(.015)
        finally:
            self.r.release()
        assert peak>0,('No visible P2 dash trails',self.sample('dash-missing'))
        return dict(water_probes=probes,p2_visible_dash_trails=peak)

    def cyber_missiles(self):
        self.cyber()
        self.wait('cyber-fight-live',lambda s:not s['blocked'] and not s['warning'] and
                  any(p['type']==64 and p['state'][0] and p['hp']>1 for p in self.enemies()),20,True)
        # Opposite sides distinguish a P2 homing turn from an attack that
        # happens to travel towards both overlapping players.
        self.r.teleport(0,1250,2507 if self.campaign==0 else 2506)
        self.r.teleport(1,1056,2506 if self.campaign==0 else 2507)
        rows=[]
        observed_types={}
        fight=[]
        deadline=time.monotonic()+25
        while time.monotonic()<deadline:
            first,body=self.r.players()
            p1x,p1y=struct.unpack_from('<ii',first,8)
            x,y=struct.unpack_from('<ii',body,8)
            pool=self.r.read(0x8013f328,32*0x9c)
            for i in range(32):
                p=pool[i*0x9c:(i+1)*0x9c]
                if p[0]:observed_types[p[1]]=observed_types.get(p[1],0)+1
                if p[0] and p[1]==42 and p[4]==1 and p[5]==0:
                    mx,my=struct.unpack_from('<ii',p,8)
                    vx,vy=struct.unpack_from('<ii',p,0x20)
                    rows.append(dict(slot=i,x=mx/65536,y=my/65536,vx=vx/65536,vy=-vy/65536,
                                     p1=[p1x/65536,p1y/65536],p2=[x/65536,y/65536],
                                     angle=struct.unpack_from('<I',p,0x84)[0],
                                     turn_delay=struct.unpack_from('<h',p,0x88)[0]))
            def towards(p,seat):
                return (p[seat][0]-p['x'])*p['vx']+(p[seat][1]-p['y'])*p['vy']
            approaching=[p for p in rows if p['turn_delay']==0 and
                         towards(p,'p2')>0 and towards(p,'p1')<0]
            if len(rows)>=25 and approaching:
                break
            if len(fight)<5 or len(fight)%40==0:
                fight.append(dict(enemies=self.enemies(),gates=list(self.r.read(self.r.diagnostic()+0x410,13))))
            else:
                fight.append(None)
            time.sleep(.02)
        self.sample('cyber-p2-homing-missiles',True)
        (self.output/'cyber-missiles.json').write_text(json.dumps(dict(missiles=rows,types=observed_types,
                            fight=[p for p in fight if p]),indent=2))
        assert rows,'Cyber Peacock did not emit native homing missiles in the bounded fight'
        assert approaching,('Native missiles never turn towards living P2',rows)
        assert all(p[0x5c]&127 for p in self.r.players())
        return dict(native_missile_samples=len(rows),approaching_p2_away_from_p1=len(approaching),both_alive=True)

    def diagnostics(self):
        self.r.input(0xffdf,0);self.r.input(0x7fff,1)
        try:
            time.sleep(.06)
            image=self.r.request('host_osd_shot',path=str(self.output/'native-input-osd.png'))
            assert image['height']>=40,image
        finally:
            self.r.release()
        time.sleep(.06)
        self.r.request('host_osd_shot',path=str(self.output/'native-input-osd-idle.png'))
        return dict(host_input_bitmap=image,default_mod_active=True)

    def audio(self):
        self.load(1,0)
        time.sleep(.2)
        stats=self.r.request('audio_stats')
        start=stats['taps'][0]['frames']
        before=self.r.request('spu_events',count=256)['total']
        self.r.input(0xbfff,0)
        time.sleep(.03)
        self.r.input(0xbfff,1)
        time.sleep(.09)
        self.r.release()
        time.sleep(.12)
        events=[e for e in self.r.request('spu_events',count=256)['events'] if e['seq']>=before]
        wav=self.r.request('audio_wav',path=str(self.output/'native-two-jumps.wav'),tap=0,start=str(start))
        (self.output/'native-two-jumps.json').write_text(json.dumps(dict(events=events,wav=wav),indent=2))
        on=[e for e in events if e['kind']=='KEYON']
        assert any(e['v']<24 for e in on) and any(24<=e['v']<48 for e in on),events
        overlap=[]
        for second in on:
            if not 24<=second['v']<48:continue
            first=next((e for e in on if e['v']==second['v']-24 and e['frame']<second['frame']),None)
            end=next((e for e in events if e['v']==second['v']-24 and e['kind']=='END_STOP' and
                      e['frame']>second['frame']),None)
            if first and end and not any(e['v']==first['v'] and e['kind']=='KEYOFF' and
                                         first['frame']<=e['frame']<=end['frame'] for e in events):
                overlap.append(dict(p1_voice=first['v'],p2_voice=second['v'],
                                    p1_start=first['frame'],p2_start=second['frame'],p1_end=end['frame']))
        assert overlap,('No uninterrupted same-logical-channel native overlap',events)
        return dict(native_hardware_keyons=sum(e['v']<24 for e in on),
                    native_private_keyons=sum(24<=e['v']<48 for e in on),overlap=overlap,wav=wav)

    def cyber_orb(self):
        self.watch_enemy=36
        self.load(6,0)
        # Let P2's invulnerability expire at the safe spawn before any orb
        # is loaded. Waiting beside moving orbs would let them leave the view.
        self.r.fixture(5,1)
        time.sleep(2.2)
        # Settle the camera before crossing400. Moving straight from camera96
        # to X480 makes the native intro particle offscreen and culls its timer.
        self.r.teleport_both(240,203)
        self.r.input(0xbfdf,0);self.r.input(0xbfdf,1)
        time.sleep(.2)
        self.r.input(0xffdf,0);self.r.input(0xffdf,1)
        time.sleep(.2)
        self.r.release();time.sleep(.1)
        self.r.teleport_both(480,203)
        self.wait('cyber-assessment-entered',lambda s:s['freeze']!=0,5)
        self.wait('cyber-assessment-controls',lambda s:not s['freeze'] and
                  not any(s['personal_locks'][1]),25,True)
        self.wait('cyber-orbs-visible',lambda s:any(p['type']==36 and p['state'][0]==1
                  for p in self.enemies()),8)
        orb=min((p for p in self.enemies() if p['type']==36 and p['state'][0]==1),
                key=lambda p:abs(p['x']-651)+abs(p['y']-191))
        self.r.teleport(0,int(orb['x'])-40,int(orb['y'])+24)
        self.r.teleport(1,int(orb['x']),int(orb['y'])+24)
        captured=self.wait('cyber-orb-captured-p2',lambda s:s['orb_capture'][1]!=0 and any(
            p['type']==36 and p['state'][1]==3 for p in self.enemies()),8)
        counters=self.r.request('mod_counters')['counters']
        before=sum(c['count'] for c in counters if c['name']=='mmx4.coop.departures')
        self.r.input(0xfffe,1)
        try:
            time.sleep(1.7)
            self.sample('cyber-orb-select-held',True)
        finally:
            self.r.release()
        counters=self.r.request('mod_counters')['counters']
        after=sum(c['count'] for c in counters if c['name']=='mmx4.coop.departures')
        assert before==after,('P2 withdrew after orb capture',captured,before,after)
        self.r.fixture(5,3)
        self.watch_enemy=None
        return dict(native_orb_capture=True,withdrawal_while_captured=after-before,
                    capture_locks=captured['personal_locks'][1])

    def cyber_course(self):
        cases=[]
        # The native spawn list800F41D0 has six assessment checkpoints.
        # Odd checkpoints are hubs: endpoint768, no introductory stopwatch.
        # Hub5 advances into section1; hub1 advances to checkpoint2.
        for checkpoint,owner in ((1,1),(5,0)):
            self.load(6,0,checkpoint)
            self.wait('cyber-hub-controls',lambda s:not s['freeze'] and
                      not s['blocked'] and all(not any(p) for p in s['personal_locks']),10)
            counters=self.r.request('mod_counters')['counters']
            count=lambda name:sum(c['count'] for c in counters if c['name']==name)
            before_departures=count('mmx4.coop.script-departures')
            before_arrivals=count('mmx4.coop.script-arrivals')
            # Native-burn-40's bounded hub traversal established these safe
            # platforms:565/955 and659/923 (Zero's feet are one pixel higher).
            # Use those observed coordinates and let only the designated
            # owner cross768; moving both together makes P1 enter first.
            self.r.teleport_both(565,955)
            owner_height=923 if (self.campaign^owner)==0 else 922
            self.r.teleport(owner,659,owner_height)
            self.r.input(0xffdf,owner)
            try:
                self.wait('cyber-course-owner-entered',lambda s:s['players'][owner]['script']!=0,8)
            finally:self.r.release()
            deadline=time.monotonic()+30
            transitioned=False
            while time.monotonic()<deadline:
                row=self.sample('cyber-course-handoff')
                counters=self.r.request('mod_counters')['counters']
                if row['section']!=0 or row['checkpoint']!=checkpoint:transitioned=True
                if not transitioned:
                    assert count('mmx4.coop.script-arrivals')==before_arrivals,(
                        'Passenger returned before native course changed',row)
                if transitioned and row['mode'][:2]==[6,0] and not row['blocked'] and not row['freeze'] and all(
                    p['active'] and p['visible'] and p['state']==1 and p['action']==2 and
                    p['grounded'] and p['hp']>0 and not p['script'] for p in row['players']):break
                time.sleep(.02)
            else:raise TimeoutError(f'Cyber course handoff did not complete: {row}')
            assert count('mmx4.coop.script-departures')>before_departures,row
            assert row['section']==(1 if checkpoint==5 else 0),row
            assert row['checkpoint']==(0 if checkpoint==5 else 2),row
            self.sample('cyber-hub-'+str(checkpoint)+'-complete',True)
            cases.append(dict(checkpoint=checkpoint,owner=owner,after=row,
                              passenger_arrivals=count('mmx4.coop.script-arrivals')-before_arrivals))
        return dict(native_assessment_handoffs=cases)

    def spider_hole(self):
        self.load(1,0,4)
        self.r.teleport_both(5680,394)
        self.r.teleport(1,5736,394)
        row=self.wait('spider-p2-first-secret-drop',lambda s:s['bounds'][0]==6048 and
                      s['bounds'][2]==256,8)
        assert row['players'][0]['x']<5712,row
        assert 5712<=row['players'][1]['x']<=5760 and row['players'][1]['hp']>0,row
        self.sample('spider-secret-hole-open',True)
        self.r.teleport_both(5992,394)
        self.r.teleport(1,6040,394)
        row=self.wait('spider-p2-secret-room-camera',lambda s:s['bounds'][1]==6048,8)
        assert all(p['hp']>0 for p in row['players']),row
        self.sample('spider-secret-room-open',True)
        return dict(p2_drop_opens_secret_camera=True,p2_room_entry_updates_camera=True,
                    camera_bounds=row['bounds'])

    def victory(self):
        self.load(5,1,1)
        height=int(self.sample('jet-victory-setup')['players'][0]['y'])
        self.r.teleport_both(16848,height)
        self.r.teleport(self.victory_seat,16936,height)
        self.door(self.victory_seat,'jet-victory-door')
        self.wait('jet-live-fight',lambda s:not s['blocked'] and not s['warning'] and
                  not s['freeze'],20,True)
        boss=next(p for p in self.enemies() if p['type']==56 and p['hp']>0)
        self.r.fixture(6,boss['slot'],1)
        before=self.r.read(self.r.diagnostic()+0x459,1)[0]
        deadline=time.monotonic()+25
        while time.monotonic()<deadline:
            target=next((p for p in self.enemies() if p['type']==56),None)
            if target is None or target['state'][0]==2:
                break
            # Jet can fly beyond the room while choosing an attack. Keep
            # fixture bodies inside the observed room, clear of its door.
            self.r.teleport_both(max(17088,min(17296,int(target['x'])-48)),
                                 max(160,min(283,int(target['y'])+24)))
            self.r.input(0x7fff,self.victory_seat)
            time.sleep(1.0 if (self.campaign^self.victory_seat)==0 else .09)
            self.r.release();time.sleep(.07)
        else:
            raise TimeoutError('Native P2 attack did not defeat reduced-health Jet Stingray')
        self.r.release()
        counters=self.r.request('mod_counters')['counters']
        arrivals=sum(c['count'] for c in counters if c['name']=='mmx4.coop.script-arrivals')
        row=self.wait('jet-victory-reward',lambda s:s['mode'][0]==9,35,True)
        self.sample('jet-victory-completion',True)
        cleared=self.r.read(0x801721c0+0x59,1)[0]
        assert cleared&0x10 and cleared&before==before,(before,cleared)
        cache=list(self.r.read(0x800f1d90,8))
        assert cache[0]==self.campaign and cache[4]==cleared,('Continue cache lost native boss clear',cache,cleared)
        counters=self.r.request('mod_counters')['counters']
        after=sum(c['count'] for c in counters if c['name']=='mmx4.coop.script-arrivals')
        assert after==arrivals,('Passenger returned during victory',arrivals,after)
        return dict(native_boss_kill_seat=self.victory_seat,clears_before=before,clears_after=cleared,
                    passenger_returns_during_victory=after-arrivals,mode=row['mode'],continue_cache=cache)

    def save_loop(self):
        win=self.victory()
        return self.finish_save(win,16)

    def second_save_loop(self):
        # Start from a cloned card produced by native Jet save_loop. Native
        # Continue loads it; then defeat a different Maverick through attacks.
        before=self.r.read(0x801721c0+0x59,1)[0]
        assert before==16,before
        self.load(1,1,5)
        self.r.fixture(5,3)
        self.door(self.victory_seat,'spider-second-clear-door')
        self.wait('spider-fight-live',lambda s:not s['blocked'] and not s['freeze'] and
                  not s['warning'] and any(p['state'][0] and p['hp']>=32 and p['x']>=4624
                                           for p in self.enemies()),20,True)
        boss=next(p for p in self.enemies() if p['hp']>=32 and p['x']>=4624)
        self.r.fixture(6,boss['slot'],1)
        bounds=self.sample('spider-second-clear-room')['bounds']
        deadline=time.monotonic()+25
        while time.monotonic()<deadline:
            target=next((p for p in self.enemies() if p['slot']==boss['slot'] and
                         p['type']==boss['type']),None)
            if target is None or target['state'][0]==2:break
            self.r.teleport_both(max(bounds[1]+48,min(bounds[0]+288,int(target['x'])-48)),
                                 max(bounds[3]+32,min(bounds[2]+220,int(target['y'])+24)))
            self.r.input(0x7fff,self.victory_seat)
            time.sleep(1.0 if (self.campaign^self.victory_seat)==0 else .09)
            self.r.release();time.sleep(.07)
        else:raise TimeoutError('Native attack did not defeat reduced-health Web Spider')
        self.wait('spider-second-clear-reward',lambda s:s['mode'][0]==9,35,True)
        cleared=self.r.read(0x801721c0+0x59,1)[0]
        cache=list(self.r.read(0x800f1d90,8))
        assert cleared==17 and cache[0]==self.campaign and cache[4]==17,(cleared,cache)
        return self.finish_save(dict(native_boss_type=boss['type'],native_boss_kill_seat=self.victory_seat,
                                     clears_before=before,clears_after=cleared,continue_cache=cache),17)

    def stage_reentry(self):
        # Re-enter the stage just defeated from the actual frontend. load()
        # first reaches native ordinary gameplay before any developer load.
        assert self.r.read(0x801721c0,2)==bytes([3,4])
        clears=self.r.read(0x801721c0+0x59,1)[0]
        self.load(1,0)
        row=self.sample('same-stage-frontend-reentry-alive',True)
        assert row['campaign']==self.campaign and row['mode'][:2]==[6,0],row
        assert all(p['active'] and p['state']==1 and p['hp']>0 for p in row['players']),row
        assert self.r.read(0x801721c0+0x59,1)[0]==clears,row
        return dict(same_stage_reentry_both_alive=True,preserved_clears=clears,players=row['players'])

    def finish_save(self,win,clears):
        deadline=time.monotonic()+45
        try:
            while time.monotonic()<deadline:
                row=self.sample('native-save-to-stage-select')
                if row['mode'][:2]==[3,4]:break
                self.r.input(0xbfff,0);time.sleep(.06)
                self.r.release();time.sleep(.06)
            else:raise TimeoutError('Native save/continue UI did not reach stage selection')
        finally:self.r.release()
        self.sample('native-stage-select-jet-clear',True)
        indices=self.r.read(0x800f4758,8)
        palette=struct.unpack('<I',self.r.read(0x1f800028,4))[0]
        for boss in range(8):
            if not clears&(1<<boss):continue
            colors=struct.unpack('<16H',self.r.read(palette+indices[boss]*32,32))
            assert all((c&31)==((c>>5)&31)==((c>>10)&31) for c in colors),('Cleared portrait not grey',boss,colors)
        cards=[]
        for path in (self.output/'saves').glob('card*.mcd'):
            data=path.read_bytes()
            for block in range(1,16):
                if data[block*128]==0x51 and data[block*128+10:block*128+22]==b'BASLUS-00561':
                    record=list(data[block*8192+512:block*8192+520])
                    cards.append(dict(card=path.name,record=record))
        assert any(c['record'][0]==self.campaign and c['record'][4]==clears for c in cards),('Native save missing campaign/clears',cards)
        return dict(victory=win,cleared_portraits_grey=True,native_card_records=cards)

    def reload_save(self):
        play=self.r.read(0x801721c0,0x64)
        assert play[:2]==bytes([3,4]) and play[0x43]==self.campaign and play[0x59]==self.expected_clears
        palette=struct.unpack('<I',self.r.read(0x1f800028,4))[0]
        indices=self.r.read(0x800f4758,8)
        for boss in range(8):
            if not self.expected_clears&(1<<boss):continue
            colors=struct.unpack('<16H',self.r.read(palette+indices[boss]*32,32))
            assert all((c&31)==((c>>5)&31)==((c>>10)&31) for c in colors),(boss,colors)
        self.r.screenshot(self.output/'cold-reload-grey-jet.png')
        return dict(cold_native_card_reload=True,campaign=play[0x43],clears=play[0x59],cleared_portraits_grey=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--game', type=Path, default=Path('game.toml'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--port', type=int, default=17881)
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0)
    parser.add_argument('--victory-seat',type=int,choices=(0,1),default=1)
    parser.add_argument('--card-seed',type=Path,help='Private native cards to clone and Continue from data1')
    parser.add_argument('--clears',type=lambda p:int(p,0),default=16,help='Expected boss bits on cold native card load')
    parser.add_argument('--cases', nargs='+', choices=('jet', 'cyber', 'intro','combat','jet_boss',
                        'spider_visual','cyber_missiles','spider_hole','victory','diagnostics','audio',
                        'cyber_orb','cyber_course','save_loop','second_save_loop','stage_reentry','reload_save','enemy_activation'), default=['jet', 'cyber', 'intro'])
    args = parser.parse_args()
    for name in ('exe', 'bios', 'disc', 'game', 'output'):
        setattr(args, name, getattr(args, name).resolve())
    args.output.mkdir(parents=True, exist_ok=False)
    if args.card_seed:
        (args.output/'saves').mkdir()
        for name in ('card1.mcd','card2.mcd'):
            shutil.copy2(args.card_seed/name,args.output/'saves'/name)
    for name in ('assets', 'mods'):
        shutil.copytree(args.exe.parent / name, args.output / name)
    executable = args.output / args.exe.name
    shutil.copy2(args.exe, executable)
    for dll in args.exe.parent.glob('*.dll'):
        shutil.copy2(dll, args.output / dll.name)
    (args.output / 'mods/state.toml').write_text('format_version = 2\n\n[[feature]]\n'
        'package_id = "mmx4.coop"\nid = "coop"\nenabled = true\n'
        '[feature.values]\ncameras = "unified"\n\n[[feature]]\n'
        'package_id = "mmx4.enhancement.widescreen"\nid = "widescreen"\nenabled = false\n')
    config = args.game.read_text().replace('exe = "mmx4/SLUS_005.61"',
        'exe = ' + json.dumps((args.game.parent / 'mmx4/SLUS_005.61').as_posix()))
    config = config.replace('internal_resolution = "1080p"', 'internal_resolution = "native"')
    (args.output / 'game.toml').write_text(config)
    env = {key: value for key, value in os.environ.items()
           if not key.upper().startswith(('PSX_', 'RNET_', 'MMX4_DIAGNOSTIC_'))}
    env['PATH'] = r'C:\msys64\mingw64\bin;' + env.get('PATH', '')
    env['SDL_AUDIODRIVER']='dummy'
    argv = [str(executable), '--headless-opengl', '--no-launcher', '--debug-port', str(args.port),
            '--game', str(args.output / 'game.toml'), '--bios', str(args.bios), '--disc', str(args.disc),
            '--memcard-dir', str(args.output / 'saves')]
    runtime = Runtime(args.port)
    scenarios = Scenarios(runtime, args.output, args.campaign,args.victory_seat,args.clears)
    report = dict(campaign=args.campaign, cases=[], argv=argv)
    with (args.output / 'runtime.log').open('wb') as log:
        process = subprocess.Popen(argv, cwd=args.output, env=env, stdout=log, stderr=log,
                                   creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
        report['pid'] = process.pid
        try:
            for _ in range(80):
                try:
                    runtime.request('get_registers')
                    break
                except OSError:
                    time.sleep(.1)
            if args.card_seed:runtime.boot_continue(args.campaign,args.clears)
            else:runtime.boot(campaign=args.campaign)
            folder=next((args.output/'saves/diagnostics').iterdir())
            shutil.copytree(folder,args.output/'input-only-prefix')
            if not args.card_seed:runtime.fixture(5, 3)
            for name in args.cases:
                result = dict(name=name, status='running')
                report['cases'].append(result)
                result.update(getattr(scenarios, name)(), status='passed')
                print(json.dumps(result), flush=True)
        except Exception as error:
            report['error'] = f'{type(error).__name__}: {error}'
            if report['cases']:
                report['cases'][-1]['status'] = 'failed'
            print(report['error'], flush=True)
            try:
                scenarios.sample('failure', True)
            except Exception:
                pass
            raise
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=5)
            (args.output / 'report.json').write_text(json.dumps(report, indent=2))
            (args.output / 'events.json').write_text(json.dumps(scenarios.events, indent=2))


if __name__ == '__main__':
    main()
