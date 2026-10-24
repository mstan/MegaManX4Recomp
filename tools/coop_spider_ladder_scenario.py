"""Web Spider's native final part1 ladder across a distant partner death."""
import json
import time
from coop_fortress_followup_scenario import actors, cameras, retain


def spider_ladder_survivor(s,kill_partner=True):
    r=s.r;lead=s.victory_seat;dead=lead^1
    s.load(1,0,2)
    r.fixture(5,1<<lead)
    # Native area4 opens the shaft's vertical follow at X5552..5568.
    # Cross it with input before moving to the last ladder; jumping directly
    # from the checkpoint to the upper ledge would bypass this prerequisite.
    r.teleport(lead,5543,1402)
    s.wait('spider-shaft-approach-floor',lambda v:v['players'][lead]['grounded'],5)
    deadline=time.monotonic()+20
    while time.monotonic()<deadline:
        if any(a['pool']=='controller' and a['type']==0 and a['flags'][2]==4 for a in actors(s)):break
        time.sleep(.07)
    else:raise AssertionError('Shaft camera-area actor was not reached by the view')
    r.input(0x9fdf,lead)
    try:s.wait('spider-shaft-admitted',lambda v:v['players'][lead]['x']>=5570,8)
    finally:r.release()
    assert cameras(s)[lead][0x2a//2]==256,'Shaft camera area was bypassed'
    # Observed recording: the last upper ladder is X5736, Y426..600.
    # Setup stops below its upper camera return strip. Up must climb through
    # that strip and admit the original upper enclosure before P1 dies.
    r.teleport(lead,5736,590)
    r.input(0xffef,lead)
    try:s.wait('spider-ladder-upper-platform',lambda v:v['players'][lead]['y']<=430,15)
    finally:r.release()
    deadline=time.monotonic()+20
    while time.monotonic()<deadline:
        view=cameras(s)[lead]
        if view[0x28//2]==256 and view[0x2a//2]==256 and view[7]==256:break
        time.sleep(.07)
    else:raise AssertionError('Upper ladder camera enclosure was not admitted')
    before=retain(s,'spider-ladder-before-death')
    assert before['players'][lead]['hp'] and before['players'][lead]['y']<=440,before
    assert {a['flags'][2] for a in before['actors'] if a['pool']=='controller' and a['type']==0}>={5,8}, \
        'Upper ladder camera-area actors were not reached'
    # An observed void on the waiting player's lower route. No camera,
    # bounds, scene flags or success predicates are modified by setup.
    if kill_partner:
        r.teleport(dead,2373,1538)
        s.wait('spider-ladder-partner-dead',lambda v:not v['players'][dead]['hp'],10)
        retain(s,'spider-ladder-after-death')
    r.input(0xffbf,lead);rows=[]
    try:
        deadline=time.monotonic()+10
        while time.monotonic()<deadline:
            row=s.sample('spider-ladder-down-'+str(len(rows)))
            row['views']=cameras(s);rows.append(row)
            p=row['players'][lead]
            if not p['hp'] or p['y']>=590:break
            time.sleep(.07)
    finally:
        r.release()
        (s.output/'spider-ladder-descent.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
    after=retain(s,'spider-ladder-descended')
    p=after['players'][lead]
    assert p['hp'] and p['y']>=590,('Unsafe Web Spider ladder descent',after)
    assert after['views'][lead][7]>before['views'][lead][7],('Upper enclosure did not release',before,after)
    assert bool(after['players'][dead]['hp'])!=kill_partner and after['lives']==before['lives'],after
    # Re-entering the upper strip must still select the higher camera area.
    r.input(0xffef,lead)
    try:s.wait('spider-ladder-reclimbed',lambda v:v['players'][lead]['y']<=430,15)
    finally:r.release()
    final=retain(s,'spider-ladder-returned')
    assert final['players'][lead]['hp'] and bool(final['players'][dead]['hp'])!=kill_partner,final
    return dict(leader=lead,character=p['character'],descent_y=p['y'],hp=p['hp'],
                camera_before=before['views'][lead][7],camera_after=after['views'][lead][7],
                native_return=True,partner_killed=kill_partner,partner_state_preserved=True)
