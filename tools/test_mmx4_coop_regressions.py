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
import tomllib
from coop_debug import Runtime
from test_mmx4_coop_doors import snapshot


class Scenarios:
    def spider_ladder_pair(self):
        from coop_spider_ladder_scenario import spider_ladder_survivor
        return spider_ladder_survivor(self,kill_partner=False)
    def spider_ladder_survivor(self):
        from coop_spider_ladder_scenario import spider_ladder_survivor
        return spider_ladder_survivor(self)
    def magma_pillars(self):
        from coop_magma_105_scenarios import magma_pillars
        return magma_pillars(self)
    def magma_quake(self):
        from coop_magma_105_scenarios import magma_quake
        return magma_quake(self)
    def sigma_ladder_survivor(self):
        from coop_fortress_followup_scenario import sigma_ladder_survivor
        return sigma_ladder_survivor(self)
    def intro_bees(self):
        from coop_intro_bees_scenario import intro_bees
        return intro_bees(self)
    def magma_giga_death(self):
        from coop_fortress_followup_scenario import magma_giga_death
        return magma_giga_death(self)
    def fmv_transitions(self):
        from coop_fmv_transition_scenario import fmv_transitions
        return fmv_transitions(self)
    def magma_distant_entry(self):
        from coop_fortress_followup_scenario import magma_distant_entry
        return magma_distant_entry(self)
    def sigma1_doors(self):
        from coop_fortress_followup_scenario import sigma1_doors
        return sigma1_doors(self)
    def sigma2_lasers(self):
        from coop_fortress_followup_scenario import sigma2_lasers
        return sigma2_lasers(self)

    def fortress_probe(self):
        from coop_fortress_followup_scenario import fortress_probe
        return fortress_probe(self)

    def intro_dragon_snapshot(self):
        from coop_intro_dragon_scenario import intro_dragon_views
        return intro_dragon_views(self,restore=True)

    def magma_idle_resolution(self):
        from coop_magma_effects_scenario import magma_idle_resolution
        return magma_idle_resolution(self)

    def mushroom_lift_crush(self):
        from coop_mushroom_lift_scenario import mushroom_lift_crush
        return mushroom_lift_crush(self)

    def magma_effects_field(self):
        from coop_magma_effects_scenario import magma_effects
        return magma_effects(self,field=True)

    def magma_effects(self):
        from coop_magma_effects_scenario import magma_effects
        return magma_effects(self)

    def intro_dragon_views(self):
        from coop_intro_dragon_scenario import intro_dragon_views
        return intro_dragon_views(self)

    def intro_effect_corridor(self):
        from coop_opening_102_scenarios import intro_effect_corridor
        return intro_effect_corridor(self)

    def intro_distant_entry(self):
        from coop_opening_102_scenarios import intro_distant_entry
        return intro_distant_entry(self)

    def intro_structure_shake(self):
        from coop_opening_102_scenarios import intro_structure_shake
        return intro_structure_shake(self)

    def intro_closing_shake(self):
        from coop_opening_102_scenarios import intro_closing_shake
        return intro_closing_shake(self)

    def p2_pause_quit(self):
        from coop_opening_102_scenarios import p2_pause_quit
        return p2_pause_quit(self)

    def p2_pause_escape(self):
        from coop_opening_102_scenarios import p2_pause_escape
        return p2_pause_escape(self)


    def slash_midboss_entry(self):
        from coop_stage_followup_scenarios import slash_midboss_entry
        return slash_midboss_entry(self)

    def slash_cannon_origins(self):
        from coop_stage_followup_scenarios import slash_cannon_origins
        return slash_cannon_origins(self)

    def owl_cannon_origins(self):
        from coop_stage_followup_scenarios import owl_cannon_origins
        return owl_cannon_origins(self)

    def mushroom_midboss_exit(self):
        from coop_stage_followup_scenarios import mushroom_midboss_exit
        return mushroom_midboss_exit(self)

    def mushroom_lift_high_jump(self):
        from coop_mushroom_lift_scenario import mushroom_lift_high_jump
        return mushroom_lift_high_jump(self)

    def intro_first_drop(self):
        from coop_mushroom_followup_scenarios import intro_first_drop
        return intro_first_drop(self)

    def mushroom_survivor_colors(self):
        from coop_mushroom_followup_scenarios import mushroom_survivor_colors
        return mushroom_survivor_colors(self)

    def marine_dismount(self):
        from coop_marine_dismount_scenario import marine_dismount
        return marine_dismount(self)

    def sigma_entry(self):
        from coop_sigma_entry_scenario import sigma_entry
        return sigma_entry(self)

    def personal_menus(self):
        from coop_personal_menu_scenario import personal_menus
        return personal_menus(self)

    def frost_shaft(self):
        from coop_frost_shaft_scenario import frost_shaft
        return frost_shaft(self)

    def local_canvas(self):
        from coop_local_canvas_scenario import local_canvas
        return local_canvas(self)

    def story_bosses(self):
        from coop_story_scenarios import story_bosses
        return story_bosses(self)

    def boss_balance(self):
        from coop_balance_scenarios import boss_balance
        return boss_balance(self)

    def spider_balance(self):
        from coop_balance_scenarios import boss_balance
        return boss_balance(self, spider=True)

    def balance_saved_control(self):
        from coop_balance_saved_scenario import balance_saved_control
        return balance_saved_control(self)

    def balance_saved_slashes(self):
        from coop_balance_saved_scenario import balance_saved_slashes
        return balance_saved_slashes(self)

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
        row['plugin_failed']=struct.unpack_from('<I',mirror,12)[0]
        assert not row['plugin_failed'],('Co-op renderer disabled',row)
        row['native_health']=[list(mirror[at+0x5c:at+0x5f]) for at in (0x300,0x100)]
        row['vehicle_health']=[mirror[at+0x5c] for at in (0x620,0x200)]
        row['campaign_health']=mirror[0x445]
        row['freeze']=mirror[0x410]
        row['warning']=self.r.read(0x80141bdc,1)[0]
        row['menu']=list(self.r.read(0x801754a4,2))
        row['menu_detail']=list(self.r.read(0x801754a0,0x30))
        row['frontend']=list(self.r.read(0x80173c70,8))
        row['personal_locks']=[[mirror[at+j] for j in (0x67,0xbc,0xc0,0xc3,0xc4)]
                               for at in (0x300,0x100)]
        row['orb_capture']=[mirror[at+0xba] for at in (0x300,0x100)]
        row['camera_clip']=list(struct.unpack('<6h',self.r.read(0x801419cc,12)))
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
                                   weakness=hex(struct.unpack_from('<I',p,0x58)[0]),reaction=p[0x63],
                                   timer=struct.unpack_from('<H',p,0x7c)[0],
                                   captured=p[0x8d],capture_timer=struct.unpack_from('<H',p,0x8a)[0],
                                   x=struct.unpack_from('<i',p,8)[0]/65536,
                                   y=struct.unpack_from('<i',p,12)[0]/65536))
        return result

    def spider_capture(self):
        from coop_spider_capture_scenario import spider_capture
        return spider_capture(self)

    def storm_survivor_exit(self):
        """Actual Storm Owl7 exit; teleport only to the observed final floor."""
        self.load(7,0)
        self.r.fixture(5,2)
        self.r.fixture(2,0,0x80)
        self.wait('owl-p1-dead',lambda v:v['players'][0]['state']==3,12)
        self.r.teleport(1,6310,380)
        self.wait('owl-final-floor',lambda v:v['players'][1]['grounded'],8)
        before=self.sample('owl-exit-before',True)
        self.r.input(0xffdf,1)
        try:
            self.wait('owl-native-transition',lambda v:v['section']==1,15)
        finally:
            self.r.release()
        after=self.wait('owl-section2',lambda v:v['stage']==7 and v['section']==1 and
            v['mode'][:2]==[6,0] and v['players'][1]['state']==1 and
            v['players'][1]['action']==2 and v['players'][1]['hp']>0,30)
        after=self.wait('owl-teammate-revived',lambda v:all(
            p['hp']and p['state']==1 and p['visible']for p in v['players']),20)
        assert after['lives']==before['lives'],after
        self.sample('owl-exit-after',True)
        return dict(before=before,after=after)

    def storm_team_exit(self):
        """A genuine P1 stage departure must still advance with P2 alive."""
        self.load(7,0)
        self.r.fixture(5,2)
        self.r.teleport_both(6310,380)
        self.wait('owl-team-final-floor',lambda v:all(p['grounded'] for p in v['players']),8)
        before=self.sample('owl-team-exit-before',True)
        self.r.input(0xffdf,0)
        try:
            self.wait('owl-team-native-transition',lambda v:v['section']==1,15)
        finally:
            self.r.release()
        after=self.wait('owl-team-section2',lambda v:v['stage']==7 and v['section']==1 and
            v['mode'][:2]==[6,0] and all(p['state']==1 and p['action']==2 and
                                      p['hp']>0 for p in v['players']),30)
        assert after['lives']==before['lives'],after
        self.sample('owl-team-exit-after',True)
        return dict(before=before,after=after)

    def storm_exit_race(self):
        """P2 crosses the real final gap, including during P1's death.

        The older survivor case waits for state3 before entering and cannot
        expose a death dispatcher suspended by the native forward fade.
        """
        cases=[]
        for dying in (False,True):
            self.load(7,0);self.r.fixture(5,3)
            # Observed safe final ledge, before the6352 native exit test.
            # Both fit Unified's separation bound; movement crosses the gap.
            self.r.teleport_both(6240,426)
            before=self.sample('owl-race-before-'+str(dying),True)
            if dying:self.r.fixture(2,0,0)
            tick=lambda:struct.unpack('<I',self.r.read(self.r.diagnostic()+4,4))[0]
            first=tick();deadline=time.monotonic()+5
            self.r.input(0x9fdf,1)
            try:
                while tick()-first<4:
                    if time.monotonic()>=deadline:raise TimeoutError('Owl jump world clock stopped')
                    time.sleep(.005)
                self.r.input(0xdfdf,1)
                self.wait('owl-race-native-section',lambda v:v['section']==1,15)
            finally:self.r.release()
            after=self.wait('owl-race-both-return',lambda v:v['stage']==7 and v['section']==1 and
                v['mode'][:2]==[6,0] and not v['freeze'] and all(p['active'] and p['visible'] and
                p['hp'] and p['state']==1 and p['action']==2 for p in v['players']),40)
            assert after['lives']==before['lives'],(before,after)
            self.sample('owl-race-after-'+str(dying),True)
            cases.append(dict(p1_dying=dying,before=before,after=after))
        return dict(native_exit_races=cases)

    def storm_midboss_exit(self):
        """Defeat the original Core, then let P2 follow P1 out of its room.

        Setup uses the existing synchronized enemy-health fixture. A native
        attack supplies the final hit; the original death animation, camera
        release and map edits must complete without further state writes.
        This scenario works with Runtime and NetplayRuntime.
        """
        cases = []
        for owner in (0, 1):
            self.load(7, 1)
            self.r.fixture(5, 3)
            self.r.fixture(7, self.campaign ^ 1, 0, 1)
            self.r.teleport_both(2150, 939)
            self.r.teleport(1, 2170, 938)
            try:
                self.r.input(0xffdf, 0)
                self.r.input(0xffdf, 1)
                self.wait('owl-core-approach', lambda v:
                    all(p['x'] >= 2490 and p['state'] == 1 and not p['door']
                        for p in v['players']) and not v['blocked'] and
                    any(e['type'] == 70 and e['hp'] > 0 and e['state'][0] == 1
                        for e in self.enemies()), 30)
                self.r.release()
                # The finishing player stays nearest during the death update.
                # P2 therefore also exercises the projected actor context.
                self.r.input(0xff7f, owner ^ 1)
                self.wait('owl-core-attacker-nearest', lambda v:
                    v['players'][owner]['x'] - v['players'][owner ^ 1]['x'] >= 24, 8)
                self.r.release()
                core = next(e for e in self.enemies() if e['type'] == 70 and e['hp'] > 0)
                self.sample('owl-core-live-' + str(owner), True)
                self.r.fixture(6, core['slot'], 1)
                deadline = time.monotonic() + 30
                while time.monotonic() < deadline:
                    if not any(e['type'] == 70 and e['hp'] > 0 and e['state'][0] == 1
                               for e in self.enemies()):
                        break
                    self.r.input(0xbfff, owner)
                    time.sleep(.10)
                    self.r.input(0x7fff, owner)
                    time.sleep(.10)
                    self.r.input(0xffff, owner)
                    time.sleep(.20)
                else:
                    raise TimeoutError('No native final hit on Generaid Core')
                self.r.release()
                self.wait('owl-core-retired', lambda v:
                    not any(e['type'] == 70 for e in self.enemies()), 20)
                self.sample('owl-core-retired-' + str(owner), True)
                self.r.input(0xffdf, 0)
                self.wait('owl-p1-clears-core-room', lambda v:
                    v['players'][0]['x'] > 2600, 12)
                self.r.release()
                self.sample('owl-p1-ahead-core-' + str(owner), True)
                # Unified constrains separation. Keep the leader walking
                # while P2 follows, rather than demand an impossible gap.
                self.r.input(0xffdf, 0)
                self.r.input(0xffdf, 1)
                self.wait('owl-p2-clears-core-room', lambda v:
                    v['players'][1]['x'] > 2780, 12)
                self.r.release()
                after = self.wait('owl-core-both-idle', lambda v:
                    not v['blocked'] and not v['freeze'] and all(
                        p['active'] and p['visible'] and p['hp'] and
                        p['state'] == 1 and p['action'] == 2 and
                        not p['door'] and not p['script'] for p in v['players']), 12)
                self.sample('owl-core-followed-' + str(owner), True)
                cases.append(dict(final_hit_owner=owner, after=after))
            finally:
                self.r.release()
        return dict(native_midboss_exits=cases)

    def x_armor(self):
        """Render each shared armor upgrade on X in either roster.

        The permanent flags are a developer setup, not capsule acquisition.
        Captures qualify the artwork; the private producer counter guards
        the missing native armor pass when X is the second player.
        """
        self.load(1,0)
        # Same observed safe ledge used by enemy_activation; x320 is solid.
        self.r.teleport_both(500,427)
        self.r.teleport(1,556,427)
        self.wait('armor-both-grounded',lambda s:all(p['grounded'] for p in s['players']),8)
        original=self.r.read(0x80172207,2)
        cases=[]
        def armor_count():
            return sum(c['count'] for c in self.r.request('mod_counters')['counters']
                       if c['name']=='mmx4.coop.x-armor-primitives')
        try:
            for flags,arms in ((0,0),(8,0),(1,0),(2,0),(4,1),(4,2),(15,2)):
                before=armor_count()
                self.r.write(0x80172207,flags)
                self.r.write(0x80172208,arms)
                # P1 receives these bytes at native stage initialization.
                # A raw PLAYER write can hit a temporary P2 projection and
                # be discarded when P1's body is restored.
                if not self.campaign:
                    self.load(1,0)
                    self.r.teleport_both(500,427)
                    self.r.teleport(1,556,427)
                    self.wait('armor-p1-grounded-%02x-%d'%(flags,arms),
                              lambda s:all(p['grounded'] for p in s['players']),8)
                time.sleep(.25)
                row=self.sample('x-armor-%02x-%d'%(flags,arms),True)
                body=self.r.players()[self.campaign]
                assert body[0xa7]==flags and body[0xb8]==arms,(flags,arms,body.hex())
                after=armor_count()
                if self.campaign and flags:assert after>before,'Private X armor pass emitted no primitives'
                if not self.campaign:assert after==before,'P1 armor emitted twice'
                assert row['campaign']==self.campaign and all(p['hp']>0 for p in row['players']),row
                cases.append(dict(flags=flags,arms=arms,private_armor_primitives=after-before))
        finally:
            self.r.write(0x80172207,original[0]);self.r.write(0x80172208,original[1])
            if not self.campaign:
                self.load(1,0)
        return dict(x_seat=self.campaign,armor_cases=cases)

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

    def local_pair(self):
        from coop_local_pair_scenario import local_pair
        return local_pair(self)

    def jet_intro(self, capture=None):
        self.r.fixture(1,5,0,self.campaign)
        deadline=time.monotonic()+45;entered=None;ready_peak=0
        rider_phases=[set(),set()];maximum_separation=0;release_frames=[None,None]
        while time.monotonic()<deadline:
            play=self.r.read(0x801721c0,0x20)
            if play[0]==6 and play[12]==5:
                mirror=self.r.read(self.r.diagnostic(),0x6d0)
                bodies=[mirror[at:at+0xe4] for at in (0x300,0x100)]
                bikes=[mirror[at:at+0xb0] for at in (0x620,0x200)]
                native_frame=struct.unpack_from('<I',mirror,4)[0]
                for seat,bike in enumerate(bikes):
                    if bike[0] and bike[4:6]==bytes([1,1]) and release_frames[seat] is None:
                        release_frames[seat]=native_frame
                if all(b[0] and b[1]==0 and b[4]==1 and b[5]<=1 for b in bikes):
                    xs=[struct.unpack_from('<i',body,8)[0]for body in bodies]
                    maximum_separation=max(maximum_separation,abs(xs[0]-xs[1])/65536)
                # The bike can advance correctly while its rider stays in
                # pose0. Check each actual rider throughout the entrance;
                # enrollment and READY counts alone missed this regression.
                for seat,(body,bike) in enumerate(zip(bodies,bikes)):
                    if not (body[0] and body[3] and bike[0] and bike[4:6]==bytes([1,0])):
                        continue
                    phase=bike[6]
                    # P1's controller precedes its bike: a freshly requested
                    # pose (substate6=0) is consumed on the following tick.
                    if phase not in (1,2,3,4) or not body[6]:continue
                    expected=0x47 if phase in (1,2) else 0x4b
                    assert body[0x17]==body[0xd4]==expected,(
                        'Wrong Jet rider pose',seat,phase,body[0x17],body[0xd4],expected)
                    if phase not in rider_phases[seat]:
                        rider_phases[seat].add(phase)
                        label='jet-rider-%d-phase-%d'%(seat+1,phase)
                        self.events.append(dict(label=label,pose=body[0x17],frame=body[0x47]))
                        if capture:capture(label)
                actors=self.r.read(0x80142f98,32*0x30)
                ready_peak=max(ready_peak,sum(actors[i*0x30]!=0 and actors[i*0x30+1]==27 for i in range(32)))
                if entered is None and play[0x1c] and all(b[0] and b[4:6]==bytes([1,0]) and b[6] in (1,2) for b in bikes):
                    assert all(b[4:6]==bytes([1,18]) and b[0xc5]==255 for b in bodies)
                    xs=[struct.unpack_from('<i',b,8)[0] for b in bodies]
                    assert xs[1]<xs[0],('P2 should enter behind P1',xs)
                    pointers=[struct.unpack_from('<I',b,0xa0)[0] for b in bikes]
                    assert pointers[0]==pointers[1]!=0,pointers
                    entered=dict(separation=(xs[0]-xs[1])/65536,ready_actor=pointers[0])
                    self.sample('jet-concurrent-entrance',True)
                if not play[0x1c] and all(b[0] and b[4]==1 and b[5]==1 for b in bikes) and all(b[6] for b in bodies):break
            time.sleep(.025)
        else:raise TimeoutError('Jet entrance did not release both bikes')
        assert entered and ready_peak<=1,(entered,ready_peak)
        assert all({1,2}.issubset(phases) for phases in rider_phases),rider_phases
        assert all(frame is not None for frame in release_frames),release_frames
        assert abs(release_frames[1]-release_frames[0])<=32,('Jet entrance release delay',release_frames)
        assert all(body[0x17]==body[0xd4]==0x41 for body in bodies),[b.hex() for b in bodies]
        self.sample('jet-concurrent-controls',True)
        return dict(**entered,ready_peak=ready_peak,both_native_entrances=True,
                    rider_phases=[sorted(phases) for phases in rider_phases],
                    maximum_intro_separation=maximum_separation,release_frames=release_frames)

    def jet(self):
        entrance=self.jet_intro()
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
        return dict(entrance=entrance,ready_peak=max(counts), ready_pointers=pointers, shared_callbacks=shared,
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

    def boss_door_policy(self):
        from test_mmx4_coop_doors import run_case
        cases=[]
        for survivor,owner in ((False,0),(False,1),(True,0),(True,1)):
            case=run_case(self.r,self.campaign,owner,survivor,self.output,self.boss_doors=='both')
            cases.append(case)
            assert case['status']=='passed',case
        return dict(policy=self.boss_doors,doors=cases)

    def cyber(self):
        self.load(6, 1, 1)
        self.r.teleport_both(568, 2507)
        self.r.teleport(1, 600, 2506 if self.campaign == 0 else 2507)
        self.door(1, 'cyber-p2-outer')
        self.door(1, 'cyber-p2-inner')
        return dict(outer=True, inner=True, passenger_alive=True)

    def cyber_moving(self):
        cases=[]
        for owner in (1,0):
            self.load(6,1,1)
            self.r.teleport_both(568,2507)
            self.r.teleport(owner,600,2507)
            self.r.input(0xffdf,owner)
            try:
                self.wait('cyber-moving-entered-'+str(owner),
                          lambda s:s['players'][owner]['door']!=0,8)
                self.wait('cyber-moving-door-finished-'+str(owner),
                          lambda s:not s['players'][owner]['door'] and not s['blocked'],20,True)
                deadline=time.monotonic()+4
                sample=0
                while time.monotonic()<deadline:
                    # Keep walking inside the room while the partner arrives.
                    self.r.input(0xffdf if sample%2 else 0xff7f,owner)
                    time.sleep(.15)
                    row=self.sample('cyber-moving-arrival-'+str(owner),sample%4==0)
                    assert all(p['hp']>0 for p in row['players']),row
                    if all(p['active'] and p['visible'] and p['state']==1 and
                           not p['door'] and not p['script'] for p in row['players']) and \
                       row['players'][owner^1]['action']==2:
                        cases.append(dict(owner=owner,arrival=row))
                        break
                    sample+=1
                else:raise AssertionError(('Partner did not arrive with moving leader',row))
            finally:self.r.release()
        return dict(moving_leaders=cases)

    def handoff_dash(self):
        cases=[]
        for dash_seat in (1,0):
            self.load(6,1,1)
            self.r.teleport_both(568,2507);self.r.teleport(1,600,2507)
            frames=[]
            for door_index in (0,1):
                self.r.input(0xffdf,1)
                try:
                    self.wait('handoff-dash-entered-%d-%d'%(dash_seat,door_index),lambda s:s['players'][1]['door'],8)
                    self.wait('handoff-dash-released-%d-%d'%(dash_seat,door_index),lambda s:
                        not s['players'][1]['door'] and not s['blocked'],20,True)
                finally:self.r.release()
                for frame in range(24):
                    # Dash throughout the passenger arrival, alternating
                    # direction inside the native boss antechamber.
                    direction=0xffdf if frame%8<4 else 0xff7f
                    self.r.input(direction & 0xdfff,dash_seat);time.sleep(.035)
                    self.r.release();time.sleep(.035)
                    frames.append(self.sample('handoff-dash-%d-%d-%d'%(dash_seat,door_index,frame),True))
                self.wait('handoff-dash-arrived-%d-%d'%(dash_seat,door_index),lambda s:
                    not s['blocked'] and all(p['active'] and p['visible'] and p['hp'] and
                    p['state']==1 and p['action']>=2 and not p['door'] and not p['script']
                    for p in s['players']),20,True)
            row=self.wait('handoff-dash-both-ready-'+str(dash_seat),lambda s:
                not s['blocked'] and all(p['active'] and p['visible'] and p['hp'] and
                p['state']==1 and p['action']>=2 and not p['door'] and not p['script']
                for p in s['players']),10,True)
            self.sample('handoff-dash-finished-'+str(dash_seat),True)
            assert any(f['players'][dash_seat]['action'] in (12,13) for f in frames),('No native dash',dash_seat)
            timings=[]
            for active in (False,True):
                start=self.r.request('get_registers')['frame'];begin=time.monotonic()
                while time.monotonic()-begin<2:
                    if active:self.r.input((0xffdf if int((time.monotonic()-begin)*4)%2 else 0xff7f)&0xdfff,dash_seat)
                    time.sleep(.05);self.r.release();time.sleep(.05)
                end=self.r.request('get_registers')['frame'];elapsed=time.monotonic()-begin
                timings.append(dict(dashing=active,frames=end-start,seconds=elapsed,fps=(end-start)/elapsed,
                    phase=self.r.request('phase_profile',window=2)))
            cases.append(dict(dash_seat=dash_seat,frames=len(frames),ready=row['players'],timings=timings))
        return dict(p2_door_both_dash_seats=cases)

    def cyber_flip_probe(self):
        owner=getattr(self,'flip_owner',1)
        self.load(6,1)
        self.r.fixture(5,0)
        diag=self.r.diagnostic()
        self.wait('cyber-flip-native-invulnerability-cleared',lambda s:
            not self.r.read(diag+0x161,1)[0] and not self.r.read(diag+0x361,1)[0],5)
        self.r.teleport_both(850,971);self.r.teleport(owner,656,900)
        rows=[]
        for frame in range(20):
            row=self.sample('cyber-flip-probe-%d-%d'%(owner,frame),True)
            assert all(p['hp']>0 for p in row['players']),('Unsafe flip setup',row['players'])
            row['flip']=self.r.read(0x801721c0+0x26,1)[0]
            row['pools']={}
            for name,address,stride,count in (('general',0x80165a30,0x8c,32),('particles',0x80173ca0,0x60,64)):
                data=self.r.read(address,stride*count)
                row['pools'][name]=[dict(slot=i,type=p[1],state=list(p[:8]),
                    x=struct.unpack_from('<i',p,8)[0]/65536,y=struct.unpack_from('<i',p,12)[0]/65536,
                    raw=p.hex()) for i in range(count) if (p:=data[i*stride:(i+1)*stride])[0]]
            rows.append(row)
            self.r.input(0xbfff if frame%5==0 else 0xffff,owner);time.sleep(.08);self.r.release()
        (self.output/('cyber-flip-probe-%d.json'%owner)).write_text(json.dumps(rows,indent=2),encoding='utf-8')
        self.r.fixture(5,3)
        return dict(owner=owner,frames=len(rows),flips=sorted({r['flip'] for r in rows}),ready=rows[-1]['players'])

    def cyber_flip(self):
        cases=[]
        for owner in (1,0):
            self.flip_owner=owner
            result=self.cyber_flip_probe()
            assert result['flips']==[1],result
            assert all(p['active'] and p['visible'] and p['grounded'] and p['hp']>0
                       for p in result['ready']),result
            cases.append(result)
        return dict(native_shared_flip_switches=cases)

    def snow_base_survivor(self):
        self.watch_enemy=18
        self.r.request('wtrace_range',lo='0x0014192f',hi='0x00141930')
        self.r.request('wtrace_add',lo='0x001721dc',hi='0x001721dd')
        self.r.request('wtrace_clear')
        self.load(2,0,4)
        self.wait('snow-base-last-platform',lambda s:all(p['grounded'] and p['hp']>0 for p in s['players']),10)
        self.r.fixture(2,0,0x80)
        before=self.wait('snow-base-p1-dead-p2-alive',lambda s:s['players'][0]['state']==3 and
            not s['players'][0]['hp'] and s['players'][1]['hp']>0,10)
        self.sample('snow-base-surviving-p2-before-door',True)
        (self.output/'snow-base-before-door-writes.json').write_text(json.dumps(
            self.r.request('wtrace_dump',count=4096),indent=2),encoding='utf-8')
        self.r.input(0xffdf,1)
        try:
            self.wait('snow-base-p2-miniboss-entry',lambda s:any(e['type']==18 and e['hp']>1
                for e in self.enemies()),25,True)
        finally:self.r.release()
        self.wait('snow-base-miniboss-controls-ready',lambda s:not s['blocked'] and not s['warning'] and
            s['players'][1]['action']>=2 and not s['players'][1]['script'] and not s['players'][1]['door'],25,True)
        boss=next(e for e in self.enemies() if e['type']==18 and e['hp']>1)
        self.sample('snow-base-miniboss-before-native-death',True)
        self.r.fixture(6,boss['slot'],1)
        for attempt in range(8):
            target=next((e for e in self.enemies() if e['type']==18 and e['hp']>0),None)
            if not target:break
            self.r.teleport(1,int(target['x'])-40,2235)
            self.r.input(0xbfff,1);time.sleep(.18)
            self.r.input(0x7fff,1);time.sleep(.12);self.r.release();time.sleep(.15)
        # The actual native hit/death, not a fabricated section flag, owns
        # the forward transition.
        self.r.input(0xffdf,1)
        try:
            after=self.wait('snow-base-p2-forward-section',lambda s:s['stage']==2 and s['section']==1 and
                s['mode'][:2]==[6,0] and s['players'][1]['active'] and s['players'][1]['visible'] and
                s['players'][1]['state']==1 and s['players'][1]['action']>=2 and not s['blocked'],35,True)
        finally:self.r.release()
        after=self.wait('snow-base-p2-section2-grounded',lambda s:s['stage']==2 and s['section']==1 and
            not s['blocked'] and s['players'][1]['grounded'] and s['players'][1]['action']==2 and
            not s['personal_locks'][1][0],8)
        assert after['players'][0]['hp']==0 and after['players'][1]['hp']>0,after
        assert after['lives']==before['lives'],after
        self.sample('snow-base-surviving-p2-section2',True)
        self.watch_enemy=None
        return dict(native_p2_section_transfer=True,before=before['players'],after=after['players'],lives=after['lives'])

    def frost_late_path(self):
        """The later outdoor ice bridge has a different P1-only trigger."""
        self.load(2,0,0)
        self.r.fixture(5,3)
        before=self.sample('frost-late-path-start')
        self.r.teleport(1,3246,361)  # Observed safe ledge before the gap.
        rows=[]
        deadline=time.monotonic()+8
        while time.monotonic()<deadline:
            row=self.sample('frost-late-path-p2-ahead')
            data=self.r.read(0x80142F98,32*0x30)
            row['controllers']=[list(b[1:7]) for i in range(32)
                if (b:=data[i*0x30:(i+1)*0x30])[0] and b[1]==21 and b[2]==3]
            data=self.r.read(0x80165A30,32*0x8C)
            row['path_platforms']=sum(bool(b[0] and b[1]==17) for i in range(32)
                if (b:=data[i*0x8C:(i+1)*0x8C]))
            rows.append(row)
            assert row['players'][0]['x']<3168 and row['players'][1]['hp']>0,row
            if row['path_platforms']:break
            time.sleep(.08)
        (self.output/'frost-late-path.json').write_text(json.dumps(rows,indent=2))
        assert any(c[4]>0 for r in rows for c in r['controllers']),('Bridge never armed',rows)
        assert rows[-1]['path_platforms']>0,('No native ice-path platforms',rows[-1])
        return dict(p1_remains_behind=True,p2_activates_native_bridge=True,
                    before=before['players'],after=rows[-1]['players'],
                    platforms=rows[-1]['path_platforms'])

    def section_health(self):
        self.load(1,0)
        self.r.fixture(2,1,8)
        before=self.sample('section-low-p2-health',True)
        assert before['players'][1]['hp']==8,before
        lives=before['lives']
        self.load(1,1)
        after=self.sample('section-refilled-p2-health',True)
        maximum=self.r.read(self.r.diagnostic()+0x446,1)[0]
        assert after['players'][1]['hp']==maximum,(maximum,after)
        assert after['lives']==lives,after
        return dict(before=before['players'],after=after['players'],maximum=maximum,lives=lives)

    def weapon_cycle(self):
        """Reuse projectile slots across every weapon, then charge the buster.

        The old private renderer interpreted a static buster's retained sprite
        pointer as compressed artwork and disabled the entire co-op session.
        Only the initial unlock/setup uses a fixture; native R1 selects each
        subsequent weapon and Square creates every actual projectile.
        """
        self.load(1,0)
        seat=self.campaign  # X is P1 in X's campaign, P2 in Zero's.
        self.r.fixture(7,seat,0,1)
        checks=[]
        def state(label):
            raw=self.r.read(self.r.diagnostic(),0x3e4)
            frame,enrolled,failed=struct.unpack_from('<3I',raw,4)
            bodies=(raw[0x300:0x3e4],raw[0x100:0x1e4])
            assert enrolled and not failed,('Co-op disabled',label,frame,failed)
            assert all(b[0] and b[3] and b[0x5c]&127 for b in bodies),('Partner lost',label)
            return frame,bodies
        def advance(label,frames,charged=False):
            start,_=state(label);deadline=time.monotonic()+20
            observed=[]
            while time.monotonic()<deadline:
                frame,bodies=state(label)
                if charged:
                    # P2's completed projection mirror owns its shot headers.
                    # Only X is firing, so a type18 in the native pool is P1's.
                    stride=8 if seat else 0x9c
                    address=self.r.diagnostic()+0x480 if seat else 0x801406f8
                    raw=self.r.read(address,16*stride)
                    for i in range(16):
                        shot=raw[i*stride:(i+1)*stride]
                        if shot[0]&0x60 and shot[1]==0x12 and shot[3]:
                            observed.append(dict(frame=frame,slot=i,header=shot[:8].hex(),
                                retained_table=hex(struct.unpack_from('<I',shot,0x38)[0]) if not seat else None))
                            if len(observed)==1:self.r.screenshot(self.output/(label+'.png'))
                if frame-start>=frames:
                    if charged:
                        assert observed,('No native full-charge projectile',label)
                        return observed
                    return bodies
                time.sleep(.04)
            raise TimeoutError(('Native attack did not advance',label))
        try:
            for cycle in range(2):
                for weapon in range(9):
                    label=f'weapon-cycle-{seat}-{cycle}-{weapon}'
                    _,bodies=state(label)
                    assert bodies[seat][0x93]==weapon,('Native R1 selection',label,bodies[seat][0x93])
                    self.r.input(0x7fff,seat);advance(label+'-charge',140)
                    self.r.release();shots=advance(label+'-release',45,charged=weapon==0)
                    checks.append(dict(cycle=cycle,weapon=weapon,frame=state(label)[0],
                        charged_buster=shots if weapon==0 else None))
                    self.r.input(0xf7ff,seat);advance(label+'-next',4)
                    self.r.release();advance(label+'-settle',4)
                self.r.screenshot(self.output/f'weapon-cycle-{seat}-{cycle}.png')
        finally:self.r.release()
        (self.output/'weapon-cycle.json').write_text(json.dumps(checks,indent=2)+'\n')
        return dict(x_seat=seat,native_weapon_selections=True,attacks=len(checks),partner_retained=True)

    def dash_effects(self):
        self.load(1,0)
        self.r.teleport_both(500,427);self.r.teleport(1,580,427)
        self.wait('dash-effects-grounded',lambda s:all(p['grounded'] for p in s['players']),5)
        rows=[]
        for seat in (1,0):
            self.r.input(0xdfdf,seat)
            try:
                for frame in range(14):
                    row=self.sample('dash-effects-%d-%d'%(seat,frame),True)
                    data=self.r.read(0x8013e510,32*0x70)
                    row['effects']=[]
                    for slot in range(32):
                        e=data[slot*0x70:(slot+1)*0x70]
                        if e[0]:row['effects'].append(dict(slot=slot,type=e[1],state=list(e[4:7]),
                            x=struct.unpack_from('<i',e,8)[0]/65536,y=struct.unpack_from('<i',e,12)[0]/65536,
                            parent=hex(struct.unpack_from('<I',e,0x50)[0]),priority=e[0x16],
                            assembly=hex(struct.unpack_from('<I',e,0x3c)[0])))
                    rows.append(row);time.sleep(.015)
            finally:self.r.release()
        (self.output/'dash-effects.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
        return dict(frames=len(rows),types=sorted({e['type'] for row in rows for e in row['effects']}))

    def zero_elemental(self):
        self.load(1,0)
        zero=self.campaign^1
        self.r.fixture(7,zero,0,1)
        self.r.teleport_both(56,139);self.r.teleport(zero,112,139)
        self.wait('elemental-both-grounded',lambda s:all(p['grounded'] for p in s['players']),5)
        rows=[]
        for move in ('fire','ice'):
            self.r.teleport(zero,112,139)
            if move=='ice':
                self.r.input(0xbfff,zero);time.sleep(.2)
            # Zero's techniques use Triangle; Square is the ordinary saber
            # combo/air spin and does not reproduce the elemental report.
            self.r.input(0xefef if move=='fire' else 0xefbf,zero)
            try:
                for frame in range(12):
                    row=self.sample('zero-%s-%d'%(move,frame),True)
                    data=self.r.read(0x8013e510,32*0x70)
                    row['effects']=[dict(slot=i,type=p[1],state=list(p[:8]),
                        x=struct.unpack_from('<i',p,8)[0]/65536,y=struct.unpack_from('<i',p,12)[0]/65536,
                        parent=hex(struct.unpack_from('<I',p,0x50)[0]),sprites=hex(struct.unpack_from('<I',p,0x38)[0]),
                        assembly=hex(struct.unpack_from('<I',p,0x3c)[0]))
                        for i in range(32) if (p:=data[i*0x70:(i+1)*0x70])[0]]
                    data=self.r.read(0x80173ca0,64*0x60)
                    row['particles']=[dict(slot=i,type=p[1],state=list(p[:8]),
                        x=struct.unpack_from('<i',p,8)[0]/65536,y=struct.unpack_from('<i',p,12)[0]/65536,
                        sprites=hex(struct.unpack_from('<I',p,0x38)[0]),assembly=hex(struct.unpack_from('<I',p,0x3c)[0]))
                        for i in range(64) if (p:=data[i*0x60:(i+1)*0x60])[0]]
                    rows.append(row);time.sleep(.015)
            finally:self.r.release()
            self.wait('elemental-finished-'+move,lambda s:s['players'][zero]['grounded'] and
                s['players'][zero]['state']==1 and s['players'][zero]['action']==2,6)
        (self.output/'zero-elemental.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
        return dict(zero_seat=zero,frames=len(rows),actions=sorted({r['players'][zero]['action'] for r in rows}),
            effect_types=sorted({e['type'] for r in rows for e in r['effects']}),
            particle_types=sorted({p['type'] for r in rows for p in r['particles']}))

    def fire_weakness(self):
        self.cyber()
        self.wait('weakness-cyber-fight-live',lambda s:not s['blocked'] and not s['warning'] and
            any(e['type']==64 and e['hp']>1 for e in self.enemies()),20,True)
        # Cyber's native update8007D838 enters reaction phase5 for attack
        # types3/35/63 (returns4/36/64). X uses Soul Body, Zero Ryuuenjin.
        # Enemy+63 == -1 denotes immunity/shot consumption, not a weakness.
        self.r.fixture(7,1,3 if self.campaign else 0,1)
        boss=next(e for e in self.enemies() if e['type']==64 and e['hp']>1)
        before=boss['hp'];rows=[]
        for attempt in range(10):
            target=next((e for e in self.enemies() if e['type']==64 and e['hp']>0),None)
            if target is None:break
            # Enter from the observed floor, then the native fire technique
            # reaches the boss vertically. P1 stays outside attack range.
            # The boss can hover left of the safe floor behind the doorway.
            # A setup teleport there crushes the body against the closed gate.
            x=max(1040,min(1240,int(target['x'])))
            self.r.teleport(1,x,2507)
            self.r.input(0xff7f if target['x']<x else 0xffdf,1)
            time.sleep(.015);self.r.release()
            self.r.input(0x7fff if self.campaign else 0xefef,1);time.sleep(.12);self.r.release()
            for frame in range(10):
                row=self.sample('p2-fire-weakness-%d-%d'%(attempt,frame),frame%3==0)
                row['bosses']=[e for e in self.enemies() if e['type']==64]
                shots=self.r.read(self.r.diagnostic()+0x480,16*8)
                row['shot_types']=[shots[i*8+1] for i in range(16) if shots[i*8]]
                rows.append(row);time.sleep(.025)
            if any(e['state'][1]==5 for r in rows for e in r['bosses']):break
        (self.output/'fire-weakness.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
        damage=min((e['hp'] for r in rows for e in r['bosses']),default=before)
        assert damage<before,('P2 fire did no damage',before,damage)
        assert any(e['state'][1]==5 for r in rows for e in r['bosses']),('No native weakness reaction',before,damage)
        return dict(before_hp=before,after_hp=damage,native_weakness_reaction=True,campaign=self.campaign)

    def initial_arrivals(self):
        from mmx4_arrival_probe import Probe
        output=self.output/'initial-arrivals';output.mkdir()
        probe=Probe(self.r,output,self.campaign,True)
        error=None
        try:
            for stage,section in ((1,0),(6,0),(6,1)):
                probe.initial(stage,section,'first')
            return dict(concurrent_native_arrivals=[{k:c[k] for k in
                ('stage','section','concurrent_counter_delta','concurrent_beams_observed',
                 'initial_gap_pixels','native_dash_observed','dash_during_partner_beam')}
                for c in probe.cases],horizontal_gap=24)
        except Exception as exc:
            error=str(exc);raise
        finally:probe.report(error)

    def web_walljump(self):
        from mmx4_web_probe import Probe
        output=self.output/'web-walljump';output.mkdir()
        probe=Probe(self.r,output,self.campaign)
        try:
            for side in ('left','right'):
                probe.walljump(side)
            for seat in (probe.x_seat,probe.zero_seat):
                probe.top_contact(seat)
            return dict(native_zero_web_walljump=True,campaign=self.campaign,cases=len(probe.cases))
        finally:
            self.r.release()
            (output/'stage-interactions.json').write_text(json.dumps(
                dict(cases=probe.cases,events=probe.events),indent=2),encoding='utf-8')

    def stationary_arrivals(self):
        from mmx4_arrival_probe import Probe
        output=self.output/'stationary-arrivals';output.mkdir()
        probe=Probe(self.r,output,self.campaign,True)
        error=None
        try:
            for stage,section in ((1,0),(6,0),(6,1)):
                probe.initial(stage,section,exercise_movement=False)
            return dict(native_concurrent_arrivals=True,scenes=3,horizontal_gap=24,
                        standing_both=True,campaign=self.campaign)
        except Exception as exc:
            error=str(exc);raise
        finally:probe.report(error)

    def intro_arrivals(self):
        from mmx4_arrival_probe import Probe
        output=self.output/'intro-arrivals';output.mkdir()
        probe=Probe(self.r,output,self.campaign,True)
        error=None
        self.r.fixture(5,3)
        try:
            for seat in (1,0):probe.intro(seat)
            return dict(native_p2_intro_entry=True,campaign=self.campaign,
                        dash_seats=[dict(seat=c['dash_seat'],
                            arrival_overlap=c['native_dash_during_partner_beam'],
                            post_arrival_dash=c['post_arrival_native_dash'],
                            timings=c['timings']) for c in probe.cases])
        except Exception as exc:
            error=str(exc);raise
        finally:
            self.r.fixture(5,0)
            probe.report(error)

    def train_background(self):
        from mmx4_web_probe import Probe
        output=self.output/'train-background';output.mkdir()
        probe=Probe(self.r,output,self.campaign)
        try:
            for owner in (1,0):probe.train_background(owner,2)
            return dict(native_split_train_background=True,owners=2)
        finally:
            self.r.release();self.r.fixture(5,0)
            (output/'stage-interactions.json').write_text(json.dumps(
                dict(cases=probe.cases,events=probe.events),indent=2),encoding='utf-8')

    def dialogue_native(self):
        from mmx4_dialogue_probe import slash_dialogue
        result=slash_dialogue(self)
        return dict(campaign=self.campaign,native_blue_dialogue=True,owners=len(result['cases']))

    def volcano_probe(self):
        self.load(4,1,3)
        self.sample('volcano-native-checkpoint-3',True)
        actors=self.r.read(0x80142f98,32*0x30)
        stage_actors=[]
        for slot in range(32):
            actor=actors[slot*0x30:(slot+1)*0x30]
            if actor[0]:
                stage_actors.append(dict(slot=slot,type=actor[1],data=actor.hex()))
        return dict(players=self.sample('volcano-start')['players'],
                    enemies=self.enemies(),stage_actors=stage_actors)

    def ride_armor(self, capture=None):
        """Board the authored Raiden, attack, eject and reload its checkpoint.

        Fixtures select an original spawn and place P2 above the native armor;
        actual overlap, controls, boarding/ejection and actors remain native.
        """
        self.load(4,1,3)
        approach=self.sample('ride-armor-authored-boss-approach')['players'][0]
        self.load(4,1,2)
        def vehicle(seat):
            return self.r.read(self.r.diagnostic()+(0x200 if seat else 0x620),0xb0)
        def shot(label):
            self.sample(label,True)
            if capture:capture(label)
        world=vehicle(0)
        assert world[0] and world[1]==1 and world[4:6]==bytes([1,0]),world.hex()
        x,y=struct.unpack_from('<ii',world,8);hp=world[0x5c]&127
        assert hp>0 and not self.r.players()[0][0xc5]
        self.r.teleport(1,x//65536,y//65536-70)
        deadline=time.monotonic()+12
        while time.monotonic()<deadline:
            armor=vehicle(1);body=self.r.players()[1]
            if body[0xc5] and armor[0] and armor[1]==1 and armor[5]==1:break
            time.sleep(.025)
        else:raise TimeoutError('P2 did not complete native Ride Armor boarding')
        assert not vehicle(0)[0] and armor[0x97]&0x40
        shot('ride-armor-boarded')
        before=struct.unpack_from('<i',armor,8)[0]
        try:
            self.r.input(0xffdf,1);time.sleep(.45)
        finally:self.r.release()
        armor=vehicle(1)
        assert struct.unpack_from('<i',armor,8)[0]>before and armor[0x5c]&127==hp
        attacks=set()
        for pulse in range(3):
            self.r.input(0x7fff,1)
            deadline=time.monotonic()+.22
            while time.monotonic()<deadline:
                pool=self.r.read(self.r.diagnostic()+0x480,16*8)
                for at in range(0,len(pool),8):
                    actor=pool[at:at+8]
                    if actor[0] and 59<=actor[1]<=64:attacks.add(actor[1])
                time.sleep(.01)
            self.r.release();time.sleep(.07)
        shot('ride-armor-attack')
        assert attacks,('Native armor attack actors missing',vehicle(1).hex())
        # Retail admits the eject edge in idle/walk. A Jump pressed during
        # the last punch is consumed; release and finish the native attack.
        deadline=time.monotonic()+6
        while time.monotonic()<deadline:
            if vehicle(1)[5]==1:break
            time.sleep(.025)
        else:raise TimeoutError('Ride Armor attack did not return to idle')
        self.r.release();time.sleep(.12)
        try:
            self.r.input(0xbfef,1)
            deadline=time.monotonic()+6
            while time.monotonic()<deadline:
                if not self.r.players()[1][0xc5]:break
                time.sleep(.015)
            else:raise TimeoutError('Up+Jump did not eject P2 from Ride Armor')
        finally:self.r.release()
        world=vehicle(0)
        assert world[0] and world[1]==1 and not world[0x97]&0x40 and world[0x5c]&127==hp
        assert not vehicle(1)[0] and not self.r.players()[0][0xc5]
        shot('ride-armor-native-eject')
        # Mount a fresh authored checkpoint actor, then have the partner
        # enter Dragoon's original arena approach. Its native scene must
        # dismount the transported rider before starting the incoming beam.
        self.load(4,1,2)
        world=vehicle(0);x,y=struct.unpack_from('<ii',world,8)
        self.r.teleport(1,x//65536,y//65536-70)
        deadline=time.monotonic()+12
        while time.monotonic()<deadline:
            if self.r.players()[1][0xc5] and vehicle(1)[5]==1:break
            time.sleep(.025)
        else:raise TimeoutError('Second native armor boarding did not complete')
        # Keep both within the authored approach so Unified's midpoint
        # camera can actually activate the native arena actors. Skipping P1
        # thousands of pixels ahead alone bypasses those activation strips.
        self.r.teleport(1,int(approach['x'])-24,int(approach['y']))
        self.r.teleport(0,int(approach['x']),int(approach['y']))
        try:
            self.r.input(0xffdf,0)
            self.wait('ride-armor-native-partner-handoff',lambda row:
                not self.r.players()[1][0xc5] and any(
                    p['door'] or p['script'] or p['action']==0 for p in row['players']),15)
        finally:self.r.release()
        self.wait('ride-armor-partner-returned-on-foot',lambda row:
            not row['blocked'] and all(p['active'] and p['visible'] and p['hp']>0 and
                not p['door'] and not p['script'] and p['action']>=2 for p in row['players']),35,True)
        assert not self.r.players()[1][0xc5]
        shot('ride-armor-partner-scene-on-foot')
        self.load(6,1,1)
        assert all(not b[0xc5] for b in self.r.players())
        assert all(not vehicle(seat)[0] for seat in (0,1))
        shot('ride-armor-stage-cleanup')
        return dict(native_armor_mount=True,attack_types=sorted(attacks),
                    native_ejection=True,world_armor_hp=hp,native_partner_scene_dismount=True,
                    new_stage_on_foot=True)

    def volcano_entry(self):
        cases=[]
        for owner in (1,0):
            self.load(4,1,3)
            self.sample('volcano-before-drop-'+str(owner),True)
            self.r.input(0xffdf,owner)
            try:
                deadline=time.monotonic()+10
                captured=0
                while time.monotonic()<deadline:
                    row=self.sample('volcano-drop-'+str(owner),captured%8==0)
                    assert row['players'][owner]['hp']>0,('Leader died during native boss approach',row)
                    if row['players'][owner]['door'] or row['players'][owner]['script'] or \
                       (row['players'][owner]['grounded'] and row['players'][owner]['y']>520):
                        break
                    captured+=1
                    time.sleep(.04)
                else:raise TimeoutError(('Leader did not reach lower arena',row))
            finally:self.r.release()
            arrived=self.wait('volcano-partner-arrived-'+str(owner),lambda s:not s['blocked'] and
                all(p['active'] and p['visible'] and p['hp']>0 and
                    not p['door'] and not p['script'] and p['grounded'] and p['y']>520
                    for p in s['players']),20,True)
            self.sample('volcano-safe-landing-'+str(owner),True)
            self.r.input(0xffdf,owner)
            try:
                self.wait('volcano-native-boss-'+str(owner),lambda s:any(
                    e['hp']>=32 and e['state'][0] for e in self.enemies()),10)
            finally:self.r.release()
            self.wait('volcano-fight-live-'+str(owner),lambda s:not s['blocked'] and not s['warning'] and
                all(p['active'] and p['visible'] and p['hp']>0 and p['state']==1 and
                    p['action']==2 and not p['door'] and not p['script']
                    for p in s['players']),25,True)
            self.sample('volcano-fight-ready-'+str(owner),True)
            cases.append(dict(owner=owner,arrival=arrived,enemies=self.enemies()))
        return dict(native_approaches=cases,both_alive=True)

    def teleport_fall(self):
        """A setup teleport must retain native gravity without synthetic inputs."""
        cases=[]
        # Jungle can reinitialize acceleration in its native fall path; Frost's
        # idle destination exposed the lost +2C value. Keep both native setups.
        for stage,checkpoint,positions in((1,0,(500,556)),(2,1,(2213,2253))):
            self.load(stage,0,checkpoint);self.r.fixture(5,3)
            self.wait('teleport-fall-native-ready',lambda v:all(p['hp'] and p['visible'] and
                p['state']==1 and p['action']==2 and not p['script'] for p in v['players']),15)
            for seat,x in enumerate(positions):self.r.teleport(seat,x,300)
            label='teleport-fall-'+str(stage)
            before=self.sample(label+'-start',True)
            after=self.wait(label+'-falling',lambda v:all(p['hp'] and p['y']>340
                for p in v['players']),8)
            grounded=self.wait(label+'-landed',lambda v:all(p['hp'] and p['grounded'] and
                p['action']==2 for p in v['players']),8)
            self.sample(label+'-grounded',True)
            cases.append(dict(stage=stage,checkpoint=checkpoint,before=before['players'],
                falling=after['players'],landed=grounded['players']))
        return dict(native_falls=cases)

    def intro_platform_leader(self):
        """Either hero crosses a real falling-platform trigger in Intro part2."""
        results=[]
        for owner in (0,1):
            self.load(0,1,1);self.r.fixture(5,3)
            self.wait('intro-platform-arrivals-'+str(owner),lambda v:not v['blocked'] and
                all(p['active'] and p['visible'] and p['hp'] and p['state']==1 and
                    p['action']==2 and not p['script'] and not p['door'] for p in v['players']),15)
            # Follow the original raised floor from checkpoint1. Avoid
            # transplanting a lower corridor's443 root into its solid tiles.
            self.r.input(0xffdf,0);self.r.input(0xffdf,1)
            try:
                self.wait('intro-platform-approach-'+str(owner),lambda v:
                    max(p['x'] for p in v['players'])>=1872,8)
            finally:self.r.release()
            def platform():
                pool=self.r.read(0x80165a30,32*0x8c)
                for slot in range(32):
                    body=pool[slot*0x8c:(slot+1)*0x8c]
                    if body[0] and body[1:3]==bytes((1,5)):
                        return dict(address=hex(0x80165a30+slot*0x8c),state=list(body[4:7]),
                            x=struct.unpack_from('<i',body,8)[0]/65536,
                            y=struct.unpack_from('<i',body,12)[0]/65536,body=body.hex())
            self.wait('intro-platform-loaded-'+str(owner),lambda _:platform() is not None,8)
            before=platform();assert before['state'][0:2]==[1,0],before
            self.sample('intro-platform-before-'+str(owner),True)
            self.r.input(0xffdf,owner)
            try:
                self.wait('intro-platform-native-crossing-'+str(owner),lambda v:v['players'][owner]['x']>=2010,8)
            finally:self.r.release()
            after=self.sample('intro-platform-after-'+str(owner),True)
            deadline=time.monotonic()+3
            while time.monotonic()<deadline:
                moved=platform()
                if moved and moved['y']>before['y']+8:break
                time.sleep(.025)
            result=dict(owner=owner,before=before,after=moved,players=after['players'])
            results.append(result)
            (self.output/'intro-platform-evidence.json').write_text(json.dumps(results,indent=2))
            assert all(p['hp'] and p['visible'] for p in after['players']),after
            assert moved and moved['y']>before['y']+8,('Native platform did not drop for leader',result)
        return dict(native_platforms=results)

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

    def intro_entry(self):
        self.load(0,1,2)
        # Native gate is ahead of4200.4000 stopped P2 at the shared spacing
        # limit before reaching it, so it did not exercise boss entry.
        self.r.teleport(0,4200,443)
        self.watch_enemy=8
        before=self.sample('intro-native-checkpoint-before-entry',True)
        assert not before['blocked'] and not any(p['script'] for p in before['players']),before
        deadline=time.monotonic()+45
        try:
            while time.monotonic()<deadline:
                row=self.sample('intro-native-p2-traversal')
                if any(e['type']==8 and e['hp']>1 for e in self.enemies()):break
                # Zero campaign has a Mac conversation before the dragon.
                # Complete that native scene, then continue P2's approach.
                if row['blocked']:
                    self.r.input(0xbfff,0);self.r.input(0xbfff,1)
                    time.sleep(.03);self.r.release()
                else:
                    # Mac's script returns Zero to4112. Restore the safe
                    # pre-dragon fixture after that completed encounter so
                    # the240px shared spacing limit permits P2-only entry.
                    if self.campaign==1 and row['players'][0]['x']<4200 and \
                       row['players'][0]['action']==2 and not row['players'][0]['script']:
                        self.r.teleport(0,4200,442)
                    self.r.input(0xffdf,1)
                time.sleep(.02)
            else:raise TimeoutError('P2 native intro traversal did not reach dragon')
        finally:self.r.release()
        self.sample('intro-native-entry-state',True)
        self.wait('intro-native-dragon-ready',lambda s:not s['blocked'] and not s['warning'] and
                  any(e['type']==8 and e['hp']>1 for e in self.enemies()) and
                  all(p['active'] and p['visible'] and p['hp']>0 and p['state']==1 and
                      p['action']==2 and not p['door'] and not p['script']
                      for p in s['players']),25,True)
        self.sample('intro-native-dragon-fight',True)
        return dict(native_p2_controller_traversal=True,both_in_dragon_fight=True,
                    p2_owns_scene=any(x['players'][1]['script'] for x in self.events
                                     if x['label']=='intro-native-p2-traversal'))

    def trail_probe(self,stage=1):
        self.load(stage,0)
        def dash(label):
            self.r.input(0xdfdf,1)
            peak=0
            try:
                for i in range(20):
                    b=self.r.read(self.r.diagnostic()+0x500,3*0x60)
                    active=sum(bool(b[n*0x60] and b[n*0x60+3]) for n in range(3))
                    if active>peak:
                        peak=active;self.sample(label+'-'+str(i),True)
                    time.sleep(.008)
            finally:self.r.release()
            return dict(visible_peak=peak,actors=b.hex())
        diag=self.r.diagnostic()
        first_palette=struct.unpack('<I',self.r.read(0x1f800028,4))[0]
        private_base=diag-0xc0000+(self.campaign^1)*0x60000
        body=self.r.read(diag+0x100,0xe4)
        assert private_base<=struct.unpack_from('<I',body,0x38)[0]<private_base+0x34000
        palettes=[self.r.read(a+0x1a0,96).hex() for a in (first_palette,private_base+0x41000)]
        color=struct.unpack_from('<H',bytes.fromhex(palettes[1]),2)[0]
        red,blue=color&31,(color>>10)&31
        correct_color=red>blue if self.campaign==0 else blue>red
        before=dash('trail-before-warp')
        (self.output/'trail-before.json').write_text(json.dumps(dict(palettes=palettes,before=before),indent=2))
        assert before['visible_peak']>0,before
        self.wait('trail-before-withdraw',lambda s:s['players'][1]['action']==2 and
                  s['players'][1]['grounded'],5)
        self.r.input(0xfffe,1)
        try:self.wait('trail-withdrawn',lambda s:not s['players'][1]['active'],6)
        finally:self.r.release()
        time.sleep(.1)
        self.r.input(0xfffe,1);time.sleep(.08);self.r.release()
        self.wait('trail-returned',lambda s:s['players'][1]['active'] and
                  s['players'][1]['action']==2 and s['players'][1]['grounded'],8)
        after=dash('trail-after-warp')
        (self.output/'trail-probe.json').write_text(json.dumps(dict(
            palettes=palettes,before=before,after=after),indent=2))
        assert after['visible_peak']>0,('Native trails lost after warp',after)
        assert correct_color,('P2 native dash color belongs to other character',palettes)
        return dict(before=before['visible_peak'],after=after['visible_peak'],palettes=palettes)

    def post_victory_trails(self):
        assert self.r.read(0x801721c0,2)==bytes([3,4]),'Run directly after save_loop'
        assert self.r.read(0x801721c0+0x59,1)[0]&16
        result=self.trail_probe(stage=8)
        return dict(native_jet_victory_to_train=True,**result)

    def open_airdash(self):
        self.load(1,0)
        self.r.fixture(5,0)
        self.r.write(0x801418c8+0x61,0)
        clears=self.r.read(0x801721c0+0x59,1)[0]
        self.r.write(0x801721c0+0x59,clears|16)
        self.r.write(0x801418c8+0xb9,clears|16)
        time.sleep(.8)
        zero_seat=self.campaign^1
        captures=[]
        for cycle in range(3):
            self.r.teleport_both(56,139)
            self.r.teleport(zero_seat,88,138)
            self.wait('open-airdash-grounded-'+str(cycle),lambda s:all(
                p['grounded'] and p['action']==2 for p in s['players']),6)
            self.r.input(0xbfdf,zero_seat);time.sleep(.15)
            self.r.release();time.sleep(.01)
            self.r.input(0xdfdf,zero_seat)
            try:
                for frame in range(8):
                    captures.append(self.sample('open-airdash-%d-%d'%(cycle,frame),True))
                    time.sleep(.008)
            finally:self.r.release()
            time.sleep(.3)
        actions=list(dict.fromkeys(s['players'][zero_seat]['action'] for s in captures))
        assert 13 in actions,('Native Zero air dash was not exercised',actions)
        assert all(all(p['visible'] and p['hp']>0 for p in s['players']) for s in captures)
        return dict(native_zero_airdash_actions=actions,zero_seat=zero_seat,captures=len(captures))

    def train_visual(self):
        self.load(8,0)
        self.r.fixture(5,0)
        self.r.write(0x801418c8+0x61,0)
        # Jet Stingray's native campaign bit gives Zero the air dash.
        clears=self.r.read(0x801721c0+0x59,1)[0]
        self.r.write(0x801721c0+0x59,clears|16)
        time.sleep(.2)
        self.sample('train-airdash-before',True)
        captures=[]
        for cycle in range(3):
            self.r.input(0xbfdf,1);time.sleep(.08)
            self.r.release();time.sleep(.02)
            self.r.input(0xdfdf,1)
            try:
                for i in range(8):
                    captures.append(self.sample('train-airdash-%d-%d'%(cycle,i),True))
                    time.sleep(.008)
            finally:self.r.release()
            time.sleep(.2)
        self.sample('train-airdash-after',True)
        uploads=self.r.request('a0_history')
        (self.output/'train-airdash-uploads.json').write_text(json.dumps(uploads,indent=2))
        counters=self.r.request('mod_counters')['counters']
        return dict(airdash_actions=list(dict.fromkeys(s['players'][1]['action'] for s in captures)),
                    uploads=uploads['count'],primary_private_draws=sum(c['count'] for c in counters
                     if c['name']=='mmx4.coop.primary-private-character-draws'))

    def door_death(self):
        cases=[]
        for owner in (1,0):
            self.load(6,1,1)
            self.r.teleport_both(568,2507)
            self.r.teleport(owner,600,2507)
            self.r.input(0xffdf,owner)
            try:self.wait('door-death-entered-'+str(owner),lambda s:s['players'][owner]['door'],8)
            finally:self.r.release()
            self.r.request('wtrace_range',lo='0x141924',hi='0x141925')
            lives=self.r.read(0x801721c0+0x44,1)[0]
            before_counters=self.r.request('mod_counters')['counters']
            count=lambda counters:sum(c['count'] for c in counters if c['name']==
                                      'mmx4.coop.p%d-death-sequences'%(owner+1))
            before=count(before_counters)
            self.r.fixture(2,owner,0x80)
            self.sample('door-death-fatal-'+str(owner),True)
            row=self.wait('door-death-survivor-released-'+str(owner),lambda s:
                s['players'][owner]['hp']==0 and s['players'][owner^1]['active'] and
                s['players'][owner^1]['visible'] and s['players'][owner^1]['action']==2 and
                not s['blocked'] and not any(p['door'] or p['script'] for p in s['players']),15,True)
            self.sample('door-death-survivor-safe-'+str(owner),True)
            assert row['players'][owner^1]['hp']>0 and row['lives']==lives,row
            after=count(self.r.request('mod_counters')['counters'])
            assert after-before==1,('Repeated native death sequence',before,after)
            cases.append(dict(owner=owner,players=row['players'],native_death_sequences=after-before))
        return dict(native_death_during_door=cases)

    def train_return(self):
        self.load(8,0)
        self.wait('train-return-start',lambda s:all(p['action']==2 and p['grounded']
                  for p in s['players']) and not s['blocked'],10)
        self.r.input(0xfffe,1)
        try:self.wait('train-withdrawn',lambda s:not s['players'][1]['active'],6)
        finally:self.r.release()
        time.sleep(.1)
        self.r.input(0xfffe,1);time.sleep(.08);self.r.release()
        row=self.wait('train-return-finished',lambda s:s['players'][1]['active'] and
                      s['players'][1]['action']==2 and s['players'][1]['grounded'],10)
        assert all(p['hp']>0 for p in row['players']),row
        self.sample('train-native-return-safe',True)
        return dict(native_train_return=True,players=row['players'])

    def train_deaths(self):
        cases=[]
        for checkpoint in (0,2):
            self.load(8,0,checkpoint)
            start=self.wait('train-deaths-grounded-'+str(checkpoint),lambda s:all(
                p['action']==2 and p['grounded'] for p in s['players']) and not s['blocked'],10)
            self.r.fixture(2,1,0x80)
            self.wait('train-deaths-p2-gone-'+str(checkpoint),lambda s:s['players'][1]['state']==3,12)
            assert self.sample('train-survivor-safe')['players'][0]['hp']>0
            self.r.fixture(2,0,0x80)
            row=self.wait('train-team-retry-'+str(checkpoint),lambda s:s['mode'][:2]==[6,0] and
                s['lives']==start['lives']-1 and all(p['active'] and p['visible'] and p['hp']>0 and
                p['state']==1 and p['action']==2 and p['grounded'] for p in s['players']),35,True)
            self.sample('train-team-retry-visible-'+str(checkpoint),True)
            cases.append(dict(checkpoint=checkpoint,lives=row['lives'],players=row['players']))
        return dict(native_train_death_retries=cases)

    def train_boss(self):
        cases=[]
        for owner in (1,0):
            self.load(8,1,3)
            # Checkpoint is inside the last carriage. Its platform continues
            # at X9360; seed the approach past the carriage wall. The native
            # boss gate at X9472 must still be crossed by controller input.
            self.r.teleport_both(9360,395)
            self.r.write(0x801721c0+0x2e,0) # Native first-visit boss dialogue policy.
            start=self.wait('train-boss-grounded-'+str(owner),lambda s:all(
                p['grounded'] and p['hp']>0 for p in s['players']),10)
            self.sample('train-boss-before-'+str(owner),True)
            self.r.input(0xffdf,owner)
            try:
                entry=self.wait('train-boss-dialogue-'+str(owner),lambda s:s['blocked'] or
                    any(p['script'] for p in s['players']),15)
            finally:self.r.release()
            self.sample('train-boss-dialogue-visible-'+str(owner),True)
            self.wait('train-boss-text-phase-'+str(owner),lambda s:any(e['type']==54 and
                e['state']==[1,9,6] for e in self.enemies()) and self.r.read(0x8013bc34,1)[0],15)
            for panel_frame in range(4):
                time.sleep(.4)
                self.sample('train-boss-text-panel-%d-%d'%(owner,panel_frame),True)
            row=self.wait('train-boss-fight-'+str(owner),lambda s:not s['blocked'] and not s['freeze'] and
                not s['warning'] and any(e['type']==54 and e['hp']>1 for e in self.enemies()) and
                all(p['active'] and p['visible'] and p['state']==1 and p['action']==2 and
                    p['hp']>0 and not p['script'] and not p['door'] for p in s['players']),25,True)
            self.sample('train-boss-both-visible-'+str(owner),True)
            boss=next(e for e in self.enemies() if e['type']==54 and e['hp']>1)
            self.r.fixture(6,boss['slot'],127) # Keep repeated visual attack probes in the fight.
            for seat in (1,0):
                counters=self.r.request('mod_counters')['counters']
                counts=lambda cs:{c['name']:c['count'] for c in cs if 'pause-world' in c['name']}
                before=counts(counters)
                self.r.input(0xfff7,seat);time.sleep(.1);self.r.release()
                self.wait('train-boss-pause-'+str(owner)+'-'+str(seat),lambda s:
                    s['mode'][:2]==[6,2] and s['menu'][0]==1 and s['menu'][1]<3,6)
                self.sample('train-boss-menu-'+str(owner)+'-'+str(seat),True)
                self.r.input(0xfff7,seat);time.sleep(.1);self.r.release()
                self.wait('train-boss-resume-'+str(owner)+'-'+str(seat),lambda s:s['mode'][:2]==[6,0],6)
                for frame in range(12):
                    self.r.input(0x7fff,1);time.sleep(.09);self.r.release()
                    time.sleep(.3)
                    self.sample('train-boss-resumed-'+str(owner)+'-'+str(seat)+'-'+str(frame),True)
                after=counts(self.r.request('mod_counters')['counters'])
                assert all(after.get(name,0)>before.get(name,0) for name in
                    ('mmx4.coop.pause-world-player-draws','mmx4.coop.pause-world-hud-draws')),('Pause world omitted P2',before,after)
            cases.append(dict(owner=owner,start=start['players'],entry=entry['players'],ready=row['players']))
        return dict(native_slash_beast_entries=cases)

    def bike_probe(self):
        self.load(5,0)
        diag=self.r.diagnostic()
        def grounded_bikes(s):
            data=self.r.read(diag,0x6d0)
            return all(data[at+4:at+6]==bytes([1,1]) and data[at+0x70]&8 for at in (0x620,0x200))
        row=self.wait('bike-both-initialized-grounded',grounded_bikes,10)
        x,y=int(row['players'][0]['x']),int(row['players'][0]['y'])
        self.r.teleport_both(x,y);self.r.teleport(1,x-96,y)
        time.sleep(.06)
        frames=[]
        def capture(label):
            data=self.r.read(diag,0x6d0)
            def actor(at):
                p=data[at:at+0xb0]
                return dict(state=list(p[:8]),priority=p[0x16],pose=p[0x46],frame=p[0x47],
                    grounded=bool(p[0x70]&8),controls=struct.unpack_from('<H',p,0x8c)[0],
                    x=struct.unpack_from('<i',p,8)[0]/65536,y=struct.unpack_from('<i',p,12)[0]/65536,
                    sprites=hex(struct.unpack_from('<I',p,0x38)[0]),assembly=hex(struct.unpack_from('<I',p,0x3c)[0]))
            row=dict(label=label,players=[actor(0x300),actor(0x100)],bikes=[actor(0x620),actor(0x200)])
            data=self.r.read(0x8013f328,32*0x9c)
            row['exhaust']=[dict(slot=i,state=list(p[:8]),priority=p[0x16],
                x=struct.unpack_from('<i',p,8)[0]/65536,y=struct.unpack_from('<i',p,12)[0]/65536)
                for i in range(32) if (p:=data[i*0x9c:(i+1)*0x9c])[0] and p[1]==57]
            frames.append(row);self.sample(label,True)
        capture('bike-before-dash')
        for seat in (1,0):
            self.r.input(0xdfff,seat)
            try:
                for frame in range(8):
                    capture('bike-dash-%d-%d'%(seat,frame));time.sleep(.012)
            finally:self.r.release()
            time.sleep(.06)
        (self.output/'bike-frames.json').write_text(json.dumps(frames,indent=2),encoding='utf-8')
        for seat in (1,0):
            assert any(row['label'].startswith('bike-dash-'+str(seat)) and
                       row['bikes'][seat]['state'][5] in (2,5) for row in frames),('Bike ignored Dash',seat,
                [(row['label'],row['bikes'][seat]['state'][5]) for row in frames])
        return dict(native_bike_frames=len(frames),initial=frames[0],dash=frames[-1])

    def arrival_death(self):
        self.load(6,1,1)
        self.r.teleport_both(568,2507);self.r.teleport(0,600,2507)
        self.r.input(0xffdf,0)
        try:
            self.wait('arrival-death-door-start',lambda s:s['players'][0]['door'],8)
            incoming=self.wait('arrival-death-incoming',lambda s:not s['players'][0]['door'] and
                s['players'][1]['active'] and s['players'][1]['visible'] and
                s['players'][1]['action']==0,15)
        finally:self.r.release()
        self.r.fixture(2,0,0x80)
        row=self.wait('arrival-death-survivor-ready',lambda s:s['players'][0]['hp']==0 and
                      s['players'][1]['active'] and s['players'][1]['action']==2 and
                      s['players'][1]['hp']>0,10)
        assert row['players'][1]['x']>=688 and row['players'][1]['y']>=2480,row
        self.sample('arrival-death-kept-new-room',True)
        return dict(native_leader_death_during_arrival=True,incoming=incoming,players=row['players'])

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

    def cyber_course_leader(self):
        """Finish a timed course as P2, including its native assessment.

        Hub-only handoff coverage omits the targeting graphic that produces
        the timed course's completion signal. Keep P1 behind the endpoint.
        """
        self.load(6,0);self.r.fixture(5,3)
        def assessment_ready(_):
            pool=self.r.read(0x80173ca0,64*0x60)
            return any(pool[i*0x60] and pool[i*0x60+1]==34 and
                pool[i*0x60+4:i*0x60+6]==bytes((1,2)) for i in range(64))
        # An instantaneous P1 teleport can cull the initial native analysis
        # graphic before the camera catches up, invalidating the setup itself.
        # Move through observed36 route samples in bounded steps instead; let
        # the native camera/graphic advance between them. No event or grading
        # state is written. Coordinates are Zero's; X's floor origin is +1Y.
        route=((131,162),(193,133),(259,166),(321,142),(387,147),(452,154),
            (515,162),(577,134),(643,144),(705,141),(771,63),(836,170),
            (897,134),(965,132),(1025,163),(1088,202),(1154,202),(1216,152),
            (1280,74),(1346,59),(1408,125),(1474,186),(1539,174),(1601,122),
            (1667,130),(1712,154))
        tick=lambda:struct.unpack('<I',self.r.read(self.r.diagnostic()+4,4))[0]
        assessment_started=False
        for x,y in route:
            self.r.teleport_both(x,y+(self.campaign==0))
            self.r.teleport(1,x,y+(self.campaign==1))
            first=tick();deadline=time.monotonic()+5
            while tick()-first<10:
                if time.monotonic()>=deadline:raise TimeoutError('Cyber setup world clock stopped')
                time.sleep(.005)
            if x>=400 and not assessment_started:
                # The actual native entry threshold is400, not stage load.
                # Keep this first platform through its opening scan animation.
                self.wait('cyber-course-initial-assessment-ready',assessment_ready,15)
                assessment_started=True
        # Last safe platform observed in the supplied36 recording, before
        # the native1792 endpoint. P1 remains within Unified's normal bounds.
        self.r.teleport(1,1744,154+(self.campaign==1))
        before=self.sample('cyber-course-p2-before',True)
        self.r.input(0xffdf,1)
        try:
            self.wait('cyber-course-p2-assessed',lambda v:v['players'][1]['script']!=0,12)
            self.wait('cyber-course-native-checkpoint',lambda v:v['checkpoint']in(1,2),20)
        finally:self.r.release()
        after=self.wait('cyber-course-p2-both-return',lambda v:v['stage']==6 and
            v['section']==0 and v['checkpoint']in(1,2) and v['mode'][:2]==[6,0] and
            not v['freeze'] and all(p['active'] and p['visible'] and p['hp'] and
                p['state']==1 and p['action']==2 for p in v['players']),40)
        assert after['lives']==before['lives'],(before,after)
        start_x=[p['x']for p in after['players']]
        for seat in (0,1):self.r.input(0xffdf,seat)
        time.sleep(.2);self.r.release()
        moved=self.sample('cyber-course-p2-after',True)
        assert all(p['x']>start_x[seat] and p['hp'] for seat,p in enumerate(moved['players'])),moved
        return dict(before=before,after=after,movement=moved)

    def cyber_indicator(self):
        """Observe the one native course controller and both drawn indicators."""
        self.load(6,0);self.r.fixture(5,3)
        def marker():
            pool=self.r.read(0x80173ca0,64*0x60)
            for i in range(64):
                b=pool[i*0x60:(i+1)*0x60]
                if b[0] and b[1]==34 and b[4:6]==bytes((1,2)):
                    return 0x80173ca0+i*0x60,b
            return None
        route=((131,162),(193,133),(259,166),(321,142),(387,147),(452,154))
        opening=[]
        for x,y in route:
            self.r.teleport_both(x,y+(self.campaign==0))
            self.r.teleport(1,x,y+(self.campaign==1))
            start=struct.unpack('<I',self.r.read(self.r.diagnostic()+4,4))[0]
            deadline=time.monotonic()+5
            while struct.unpack('<I',self.r.read(self.r.diagnostic()+4,4))[0]-start<10:
                if time.monotonic()>=deadline:raise TimeoutError('Cyber setup clock stopped')
                time.sleep(.01)
            if x>=400:
                self.r.teleport(1,515,162+(self.campaign==1))
                pool=self.r.read(0x80173ca0,64*0x60)
                opening=[dict(address=hex(0x80173ca0+i*0x60),body=b.hex())
                    for i in range(64) if (b:=pool[i*0x60:(i+1)*0x60])[0] and b[1]==34]
                self.sample('cyber-indicator-opening',True)
        self.wait('cyber-indicator-free-course',lambda v:marker() is not None and
            not v['freeze'] and not v['blocked'],15)
        self.r.teleport(1,515,162+(self.campaign==1))
        address,body=marker()
        before=self.sample('cyber-indicator-before',True)
        ghost=self.r.diagnostic()+0x800
        ghost_before=self.r.read(ghost,0x60)
        # Observers only: these rings must never advance grading or completion.
        peer=self.r.peers[0] if hasattr(self.r,'peers') else self.r
        peer.request('fntrace_clear')
        for target in(0x800ce894,0x80024334):peer.request('fntrace_arm',target=hex(target))
        time.sleep(.6)
        after=self.sample('cyber-indicator-after',True)
        counters={c['name']:c['count']for c in self.r.request('mod_counters')['counters']}
        evidence=dict(before=before,after=after,opening=opening,marker=hex(address),
            marker_before=body.hex(),marker_after=self.r.read(address,0x60).hex(),
            ghost_before=ghost_before.hex(),ghost_after=self.r.read(ghost,0x60).hex(),
            counters=counters,functions=peer.request('fntrace_dump',count=8192))
        (self.output/'cyber-indicator-evidence.json').write_text(json.dumps(evidence,indent=2))
        assert all(p['hp'] and p['visible'] for p in after['players']),after
        assert counters.get('mmx4.coop.p2-analysis-draws',0)>0,'P2 course indicator was never drawn'
        assert evidence['ghost_before']==evidence['ghost_after'],'Draw actor leaked into saved guest state'
        assert not counters.get('mmx4.coop.local-view-state-leak',0),counters
        updates={}
        for entry in evidence['functions']['entries']:
            if entry['target']=='0x800CE894':updates[entry['frame']]=updates.get(entry['frame'],0)+1
        assert len(updates)>=5 and max(updates.values())==1,updates
        return dict(marker=hex(address),players=after['players'],
                    analysis_draws=counters['mmx4.coop.p2-analysis-draws'],
                    native_update_frames=len(updates),max_updates_per_frame=max(updates.values()))

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
            before_pairs=count('mmx4.coop.concurrent-return-arrivals')
            pair_beam=None
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
                if transitioned and row['mode'][:2]==[6,0] and pair_beam is None and all(
                    p['active'] and p['visible'] and p['state']==1 and p['action']==0
                    for p in row['players']):
                    pair_beam=self.sample('cyber-paired-return-'+str(checkpoint),True)
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
            pairs=count('mmx4.coop.concurrent-return-arrivals')-before_pairs
            if checkpoint==1:
                assert pairs>0 and pair_beam,('Native direct return did not arrive together',pairs,row)
            cases.append(dict(checkpoint=checkpoint,owner=owner,after=row,
                              passenger_arrivals=count('mmx4.coop.script-arrivals')-before_arrivals,
                              concurrent_return_arrivals=pairs,pair_beam=pair_beam))
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

    def dragoon_survivor_reward(self):
        """A dead campaign owner still controls the post-victory reward/save UI."""
        self.watch_enemy=65
        self.load(4,1,3)
        self.r.fixture(5,3)
        self.r.teleport_both(5350,620)
        self.wait('dragoon-native-fight',lambda v:any(
            e['type']==65 and e['hp']>0 and e['state'][0]==1 for e in self.enemies())
            and not v['blocked'] and not v['warning'],30,True)
        self.r.fixture(2,0,0x80)
        self.wait('dragoon-p1-dead',lambda v:v['players'][0]['state']==3 and
                  v['players'][0]['hp']==0,12)
        boss=next(e for e in self.enemies() if e['type']==65 and e['hp']>0)
        self.r.fixture(6,boss['slot'],1)
        deadline=time.monotonic()+30
        try:
            while time.monotonic()<deadline:
                target=next((e for e in self.enemies() if e['type']==65),None)
                if target is None or target['state'][0]==2:break
                # The observed arena floor ends before the lava at either side.
                self.r.teleport(1,max(5184,min(5440,int(target['x'])-48)),651)
                self.r.input(0xffdf,1);time.sleep(.05);self.r.release()
                self.r.input(0x7fff,1);time.sleep(.25);self.r.release();time.sleep(.12)
            else:raise TimeoutError('Native P2 attack did not defeat HP1 Dragoon')
            win=self.sample('dragoon-p2-native-victory',True)
            assert win['players'][0]['hp']==0,'Victory revived dead P1'
            deadline=time.monotonic()+35
            while time.monotonic()<deadline:
                row=self.sample('dragoon-dialogue-to-reward')
                if row['mode'][0]==7:break
                self.r.input(0xbfff,1);time.sleep(.09);self.r.release();time.sleep(.11)
            else:raise TimeoutError('Dragoon victory did not reach weapon reward')
        finally:self.r.release()
        self.sample('dragoon-reward-before-p1-input')
        # Only P1 advances the reward and native memory-card UI. An old P2
        # story-pad edge must not steal control after gameplay ends.
        result=self.finish_save(win,8)
        assert self.r.read(0x801721c0+0x43,1)[0]==self.campaign
        cache=list(self.r.read(0x800f1d90,8))
        assert cache[0]==self.campaign and cache[4]==8,cache
        self.watch_enemy=None
        return dict(result,p1_dead_p2_victory=True,p1_reward_input=True,
                    continue_cache=cache,campaign=self.campaign)

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

    def portrait_exit(self):
        """Quit a live stage and use native Continue with current data."""
        clears=self.r.read(0x801721c0+0x59,1)[0]
        assert clears, 'Run after save_loop or with a cloned native save'
        self.load(1,0)
        def pulse(buttons):
            self.r.input(buttons,0);time.sleep(.15)
            self.r.release();time.sleep(.2)
        pulse(0xfff7)
        self.wait('portrait-exit-pause',lambda s:s['mode'][:2]==[6,2] and s['menu'][0]==1 and s['menu'][1]<3,5)
        pulse(0xfffe)
        self.wait('portrait-exit-quit-dialogue',lambda s:s['menu'][1]==6,5)
        time.sleep(.7)
        self.sample('portrait-exit-quit-choice',True)
        pulse(0xffbf)
        pulse(0xbfff)
        # Original Quit has a second No/Yes confirmation. Confirm Yes, then
        # skip the title animation using Start, as on a cold native boot.
        time.sleep(.4)
        pulse(0xffbf)
        pulse(0xbfff)
        deadline=time.monotonic()+15
        while time.monotonic()<deadline:
            front=self.r.read(0x80173c70,0x20)
            if front[:2]==bytes([6,1]):break
            if front[0]<6:pulse(0xfff7)
            else:time.sleep(.05)
        else:raise TimeoutError(('Native Quit did not reach main menu',self.sample('portrait-exit-quit-stuck')))
        self.sample('portrait-exit-main-menu',True)
        cache=list(self.r.read(0x800f1d90,8))
        assert cache[0]==self.campaign and cache[4]==clears,('Quit changed Continue data',cache,clears)
        # Main menu defaults to New Game; Continue defaults to current data
        # when this same process still has an active campaign.
        pulse(0xffbf)
        pulse(0xbfff)
        deadline=time.monotonic()+12
        while time.monotonic()<deadline:
            front=self.r.read(0x80173c70,0x20)
            if front[0]==7 and front[1] in (1,3):break
            time.sleep(.05)
        else:raise TimeoutError('Native Continue choice did not open')
        self.sample('portrait-exit-current-data-choice',True)
        routes=[]
        deadline=time.monotonic()+20
        while time.monotonic()<deadline:
            row=self.sample('portrait-exit-current-data')
            if not routes or routes[-1]!=row['mode'][:2]:routes.append(row['mode'][:2])
            if row['mode'][:2]==[3,4]:break
            if row['mode'][0]==0 and row['mode'][1]!=0:
                raise AssertionError(('Entered card load instead of current data',routes))
            pulse(0xbfff)
        else:raise TimeoutError('Continue current data did not reach stage selection')
        self.sample('portrait-exit-returned-board',True)
        actual=self.r.read(0x801721c0+0x59,1)[0]
        assert actual==clears,('Stage exit lost clears',clears,actual,cache)
        palette=struct.unpack('<I',self.r.read(0x1f800028,4))[0]
        indices=self.r.read(0x800f4758,8)
        colors_by_boss={}
        for boss in range(8):
            if not clears&(1<<boss):continue
            colors=struct.unpack('<16H',self.r.read(palette+indices[boss]*32,32))
            colors_by_boss[boss]=colors
            assert all((c&31)==((c>>5)&31)==((c>>10)&31) for c in colors),('Stage-exit portrait recolored',boss,colors)
        return dict(native_quit_continue_current=True,clears=clears,continue_cache=cache,
                    routes=routes,portrait_colors=colors_by_boss)

    def portrait_escape(self):
        clears=self.r.read(0x801721c0+0x59,1)[0]
        assert clears&16,'Run with a native Jet Stingray clear'
        self.load(5,1,1)
        def pulse(buttons):
            self.r.input(buttons,0);time.sleep(.12);self.r.release();time.sleep(.15)
        pulse(0xfff7)
        self.wait('portrait-escape-pause',lambda s:s['mode'][:2]==[6,2] and
                  s['menu'][0]==1 and s['menu'][1]<3,5)
        self.sample('portrait-escape-menu',True)
        choices=[]
        for _ in range(12):
            selected=self.r.read(0x801754b4,1)[0]
            choices.append(selected)
            if selected==13:break
            pulse(0xffbf if selected<10 else 0xffdf)
        else:raise AssertionError(('Native Escape Device not selectable',choices))
        self.sample('portrait-escape-device-selected',True)
        pulse(0xbfff)
        deadline=time.monotonic()+65
        while time.monotonic()<deadline:
            row=self.sample('portrait-escape-board')
            if row['mode'][:2]==[3,4]:break
            if row['mode'][0] in (12,9) or row['mode'][:2]==[3,9]:pulse(0xbfff)
            else:time.sleep(.05)
        else:raise TimeoutError(('Native escape/save UI did not return to board',row))
        self.sample('portrait-escape-returned-board',True)
        actual=self.r.read(0x801721c0+0x59,1)[0]
        assert actual==clears,('Escape lost campaign progress',clears,actual)
        palette=struct.unpack('<I',self.r.read(0x1f800028,4))[0]
        indices=self.r.read(0x800f4758,8)
        colors=struct.unpack('<16H',self.r.read(palette+indices[4]*32,32))
        assert all((c&31)==((c>>5)&31)==((c>>10)&31) for c in colors),('Escape recolored Jet portrait',colors)
        return dict(native_escape_device=True,choices=choices,clears=actual,portrait_colors=colors,mode=row['mode'])

    def finish_save(self,win,clears):
        deadline=time.monotonic()+45
        try:
            while time.monotonic()<deadline:
                row=self.sample('native-save-to-stage-select')
                if row['mode'][:2]==[3,4]:break
                if row['mode'][0]==3 and row['mode'][1]!=9:
                    # Let the board finish building with neutral controls.
                    # Rapid headless execution can otherwise confirm a stage
                    # between samples and skip the state we intend to inspect.
                    time.sleep(.02);continue
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
    parser.add_argument('--settings',type=Path,help='Use supplied video settings, preserving the QA boot and split layout')
    parser.add_argument('--diagnostic-watchdog-seconds',type=int,choices=(120,),
                        help='Diagnostic-only extended host-stall deadline; never a release setting')
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--game', type=Path, default=Path('game.toml'))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--port', type=int, default=17881)
    parser.add_argument('--campaign', type=int, choices=(0, 1), default=0)
    parser.add_argument('--victory-seat',type=int,choices=(0,1),default=1)
    parser.add_argument('--card-seed',type=Path,help='Private native cards to clone and Continue from data1')
    parser.add_argument('--clears',type=lambda p:int(p,0),default=16,help='Expected boss bits on cold native card load')
    parser.add_argument('--hold-on-failure',type=int,default=0,
                        help='Keep only this owned hidden process alive for bounded TCP investigation')
    parser.add_argument('--cameras',choices=('unified','split'),default='unified')
    parser.add_argument('--local-cameras',choices=('unified','split'),default='unified')
    parser.add_argument('--hidden-window',action='store_true',help='Hidden drawable for composed presentation checks')
    parser.add_argument('--quick-boot',action='store_true',help='Use turbo only for input-only boot navigation')
    parser.add_argument('--custom-renderer',action='store_true')
    parser.add_argument('--boss-doors',choices=('either','both'),default='either')
    parser.add_argument('--both-story-bosses',action='store_true')
    parser.add_argument('--boss-scaling',action='store_true')
    parser.add_argument('--separate-etanks',action='store_true')
    parser.add_argument('--ultimate-armor',action='store_true',help='Enable the original X code for Nova checks')
    parser.add_argument('--fmv-media',type=Path,help='Verified movie pack or original Japanese disc')
    parser.add_argument('--fmv-language',choices=('none','en','es'),default='en')
    parser.add_argument('--fmv-ids',type=int,nargs='+',default=[2,3,4,5],choices=range(2,11))
    parser.add_argument('--fmv-skip',action='store_true',help='Exercise native Start after video/audio begin')
    parser.add_argument('--fmv-both',action='store_true',help='Test natural completion and native skipping independently')
    parser.add_argument('--fmv-turbo',action='store_true',help='Fast-forward movie observation without skipping native completion')
    parser.add_argument('--fmv-opening',action='store_true',help='Observe a neutral cold-boot opening before navigating Game Start')
    parser.add_argument('--sigma-entry-cases', nargs='+',
                        choices=('p2-survivor', 'p1-survivor', 'both'),
                        default=['p2-survivor', 'p1-survivor', 'both'])
    parser.add_argument('--individual-pausing',action='store_true',help='Use the legacy one-owner menu instead of independent simultaneous menus')
    parser.add_argument('--boss-cooldowns',choices=('per_player','shared'),default='per_player')
    parser.add_argument('--cases', nargs='+', choices=('spider_ladder_pair','spider_ladder_survivor','magma_pillars','magma_quake','sigma_ladder_survivor','intro_bees','magma_giga_death','fmv_transitions','magma_distant_entry','sigma1_doors','sigma2_lasers','fortress_probe','intro_dragon_snapshot','magma_idle_resolution','mushroom_lift_crush','magma_effects_field','magma_effects','intro_dragon_views','intro_effect_corridor','intro_distant_entry','intro_structure_shake','intro_closing_shake','p2_pause_quit','p2_pause_escape','slash_midboss_entry','slash_cannon_origins','owl_cannon_origins','mushroom_midboss_exit','personal_menus','local_pair','local_canvas','jet','jet_intro','bike_probe', 'cyber','handoff_dash','section_health','weapon_cycle','dash_effects','zero_elemental','fire_weakness','initial_arrivals','stationary_arrivals','intro_arrivals','web_walljump','train_background','dialogue_native','cyber_flip_probe','cyber_flip','snow_base_survivor','frost_late_path', 'intro','intro_entry','trail_probe','post_victory_trails','open_airdash','train_visual','door_death','train_return','train_deaths','train_boss','arrival_death','combat','jet_boss',
                        'mushroom_lift_high_jump','intro_first_drop','mushroom_survivor_colors','marine_dismount','sigma_entry','spider_balance','boss_balance','story_bosses','boss_door_policy','spider_visual','cyber_missiles','spider_hole','victory','dragoon_survivor_reward','diagnostics','audio',
                        'cyber_orb','cyber_course','cyber_course_leader','cyber_indicator','cyber_moving','volcano_probe','ride_armor','volcano_entry','save_loop','second_save_loop','stage_reentry','portrait_exit','portrait_escape','reload_save','enemy_activation','x_armor','spider_capture','storm_survivor_exit','storm_team_exit','storm_exit_race','storm_midboss_exit','intro_platform_leader','teleport_fall','balance_saved_control','balance_saved_slashes','frost_shaft'), default=['jet', 'cyber', 'intro'])
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
    # A build with a bundled BIOS verifies the adjacent image before honoring
    # the CLI selection. Preserve it in isolated runs, just like the assets.
    if (args.exe.parent/'bios').is_dir():
        shutil.copytree(args.exe.parent/'bios',args.output/'bios')
    executable = args.output / args.exe.name
    shutil.copy2(args.exe, executable)
    for dll in args.exe.parent.glob('*.dll'):
        shutil.copy2(dll, args.output / dll.name)
    (args.output / 'mods/state.toml').write_text('format_version = 2\n\n[[feature]]\n'
        'package_id = "mmx4.coop"\nid = "coop"\nenabled = true\n'
        '[feature.values]\nlocal_cameras = "'+args.local_cameras+'"\ncameras = "'+args.cameras+'"\nboss_doors = "'+args.boss_doors+'"\n'
        'both_story_bosses = '+str(args.both_story_bosses).lower()+'\n'
        'boss_scaling = '+str(args.boss_scaling).lower()+'\n'
        'separate_etanks = '+str(args.separate_etanks).lower()+'\n'
        'ultimate_armor_x = '+str(args.ultimate_armor).lower()+'\n'
        'simultaneous_menus = '+str(not args.individual_pausing).lower()+'\n'
        'boss_cooldowns = "'+args.boss_cooldowns+'"\n\n[[feature]]\n'
        'package_id = "mmx4.enhancement.widescreen"\nid = "widescreen"\nenabled = '+str(args.custom_renderer).lower()+'\n\n[[feature]]\n'
        'package_id = "mmx4.diagnostics"\nid = "inputs"\nenabled = true\n')
    if args.fmv_media:
        resource={'none':'japanese-disc','en':'english-movies','es':'spanish-movies'}[args.fmv_language]
        with (args.output/'mods/state.toml').open('a',encoding='utf-8') as state:
            state.write('\n[[feature]]\npackage_id = "mmx4.japanese-fmvs"\nid = "japanese-fmvs"\nenabled = true\n'
                '[feature.values]\nsubtitles = '+json.dumps(args.fmv_language)+'\n'
                '[feature.resources]\n'+resource+' = '+json.dumps(args.fmv_media.resolve().as_posix())+'\n')
    config = args.game.read_text(encoding="utf-8").replace('exe = "mmx4/SLUS_005.61"',
        'exe = ' + json.dumps((args.game.parent / 'mmx4/SLUS_005.61').as_posix()))
    config = config.replace('internal_resolution = "1080p"', 'internal_resolution = "native"')
    (args.output / 'game.toml').write_text(config,encoding='utf-8',newline='\n')
    if args.settings:
        video=tomllib.loads(args.settings.read_text(encoding='utf-8'))['video']
        # Settings also contain BIOS/boot, rewind, cards and owner devices.
        # Copy only presentation fields; the QA setup retains those contracts.
        for key in ('bios_hle','fast_boot','aspect_ratio','rewind','rewind_depth','rewind_interval'):
            video.pop(key,None)
        video['fullscreen']=0
        (args.output/'settings.toml').write_text('[video]\n'+''.join(
            key+' = '+json.dumps(value)+'\n' for key,value in video.items()),encoding='utf-8')
    env = {key: value for key, value in os.environ.items()
           if not key.upper().startswith(('PSX_', 'RNET_', 'MMX4_DIAGNOSTIC_'))}
    env['PATH'] = r'C:\msys64\mingw64\bin;' + env.get('PATH', '')
    env['SDL_AUDIODRIVER']='dummy'
    if args.diagnostic_watchdog_seconds:
        env['PSX_STARVATION_TIMEOUT_US']=str(args.diagnostic_watchdog_seconds*1000000)
    if any(case in args.cases for case in ('local_pair','local_canvas','personal_menus')):
        env['PSX_RENDER_PASS_VERIFY']='1'
    argv = [str(executable), '--hidden-window' if args.hidden_window else '--headless-opengl', '--no-launcher', '--debug-port', str(args.port),
            '--game', str(args.output / 'game.toml'), '--bios', str(args.bios), '--disc', str(args.disc),
            '--memcard-dir', str(args.output / 'saves')]
    runtime = Runtime(args.port)
    scenarios = Scenarios(runtime, args.output, args.campaign,args.victory_seat,args.clears)
    scenarios.capture_composed=args.hidden_window
    scenarios.boss_doors=args.boss_doors
    scenarios.both_story_bosses=args.both_story_bosses
    scenarios.boss_scaling=args.boss_scaling
    scenarios.boss_cooldowns=args.boss_cooldowns
    scenarios.separate_etanks=args.separate_etanks
    scenarios.sigma_entry_cases=args.sigma_entry_cases
    scenarios.ultimate_armor=args.ultimate_armor
    scenarios.fmv_ids=args.fmv_ids
    scenarios.fmv_skip=args.fmv_skip
    scenarios.fmv_both=args.fmv_both
    scenarios.fmv_turbo=args.fmv_turbo
    scenarios.simultaneous_menus=not args.individual_pausing
    report = dict(campaign=args.campaign, cases=[], argv=argv,
                  diagnostic_watchdog_seconds=args.diagnostic_watchdog_seconds)
    with (args.output / 'runtime.log').open('wb') as log:
        process = subprocess.Popen(argv, cwd=args.output, env=env, stdout=log, stderr=log,
                                   creationflags=subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0)
        report['pid'] = process.pid
        try:
            ready_deadline=time.monotonic()+(180 if args.diagnostic_watchdog_seconds else 100)
            while time.monotonic()<ready_deadline:
                if process.poll() is not None:raise RuntimeError('Owned runtime exited before debug readiness')
                try:
                    runtime.request('get_registers')
                    break
                except OSError:
                    time.sleep(.1)
            else:raise TimeoutError('Owned runtime debug server did not become ready')
            if args.fmv_opening:
                from coop_fmv_transition_scenario import opening_transition
                report['opening']=opening_transition(runtime,args.output,args.fmv_turbo)
            if args.card_seed:runtime.boot_continue(args.campaign,args.clears)
            else:
                if args.quick_boot:
                    runtime.request('turbo',enabled=1)
                    sleep=time.sleep;time.sleep=lambda t:sleep(t*.15)
                    try:runtime.boot(timeout=180,campaign=args.campaign)
                    finally:time.sleep=sleep;runtime.request('turbo',enabled=0)
                else:runtime.boot(timeout=180 if args.diagnostic_watchdog_seconds else 90,campaign=args.campaign)
            folder=next(path for path in (args.output/'saves/diagnostics').iterdir()
                        if path.is_dir() and path.name.startswith('session-'))
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
                (args.output/'failure-enemies.json').write_text(json.dumps(scenarios.enemies(),indent=2))
                (args.output/'failure-writes.json').write_text(json.dumps(runtime.request('wtrace_dump',count=4096),indent=2))
            except Exception:
                pass
            if args.hold_on_failure:
                print('Owned hidden failure process retained for TCP inspection',flush=True)
                hold_deadline=time.monotonic()+max(0,min(1800,args.hold_on_failure))
                while process.poll() is None and time.monotonic()<hold_deadline:
                    time.sleep(.25)
            raise
        finally:
            if process.poll() is None:
                try:runtime.release();runtime.request('quit')
                except (OSError,RuntimeError):pass
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.terminate()
                    process.wait(timeout=5)
            (args.output / 'report.json').write_text(json.dumps(report, indent=2))
            (args.output / 'events.json').write_text(json.dumps(scenarios.events, indent=2))


if __name__ == '__main__':
    main()
