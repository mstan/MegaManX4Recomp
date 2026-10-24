#include "mmx4_coop_internal.h"
#include "mmx4_coop_memory.h"
#include "mod_memory.h"
#include <string.h>

#define COOP_ID "mmx4.coop"
#define CAMERA 0x801419B0u
#define CAMERA_BYTES 0xFCu
#define TRAIN_POOL 0x80169CB8u
#define TRAIN_SLOTS 4u
#define TRAIN_ACTOR_BYTES 0x30u
#define VISITS_OFFSET (16u+2u*CAMERA_BYTES)
#define STAIRS_OFFSET (VISITS_OFFSET+MMX4_COOP_PLACEMENTS/8u)
#define JUNGLE_OFFSET (STAIRS_OFFSET+2u*0x30u)
#define INTRO_OWNER_OFFSET (JUNGLE_OFFSET+2u*TRAIN_SLOTS*TRAIN_ACTOR_BYTES)
#define STATE_BYTES (INTRO_OWNER_OFFSET+48u*4u)

static uint32_t state;
static unsigned scanning,bounds_call,supplemental,spatial_call,background_call;
static unsigned mushroom_camera_call;
static unsigned destructible_camera_call;
static unsigned lift_camera_call;
static unsigned jungle_background_call;
static unsigned frost_camera_call;
static uint32_t background_actor,background_dx;
static unsigned background_parallax;
static unsigned train_scene_call;
static unsigned intro_explosion_call,intro_shake_call,intro_explosion_published;
static unsigned intro_camera_call;
static unsigned volcano_call,volcano_fireball_call;
static void initialize_cameras(void);

int mmx4_coop_split_snapshot_ready(void) {
    return !(scanning || bounds_call || supplemental || spatial_call || mushroom_camera_call || destructible_camera_call || lift_camera_call || jungle_background_call || frost_camera_call || background_call ||
        background_actor || background_dx || background_parallax || train_scene_call ||
        intro_explosion_call || intro_shake_call || intro_explosion_published || intro_camera_call ||
        volcano_call || volcano_fireball_call);
}
int mmx4_coop_split_snapshot(Mmx4SnapshotIO *s) {
    /* Camera, placement and controller bytes use the guest-memory section. */
    mmx4_snapshot_fixed(s,state);
    return s->ok;
}

static void copy_camera(uint32_t destination,uint32_t source) {
    /* All callers copy one complete camera between disjoint contexts. */
    uint8_t bytes[CAMERA_BYTES];
    mmx4_coop_capture(source,bytes,sizeof bytes);
    mmx4_coop_project(destination,bytes,sizeof bytes);
}
static void read_bytes(uint32_t source,uint8_t *bytes,unsigned size) {
    mmx4_coop_capture(source,bytes,size);
}
static void write_bytes(uint32_t destination,const uint8_t *bytes,unsigned size) {
    mmx4_coop_project(destination,bytes,size);
}
void mmx4_coop_split_local_explosion_palette(CPUState *cpu,const uint8_t camera[0xFC]) {
    /* BBC14's type28 controller centres its explosions at CAMERA+(160,128)
     * in BBDF4, but BBEA4 applies red to the shared palette. Intro part2 also
     * has B5960's type2/subtype0 red pulse (0..3) around the native view.
     * Remove those local effects from a distant independent viewport.
     * This helper runs inside the local-render RAM/VRAM transaction: native
     * colors, upload flags and working palettes restore with that scope. */
    if(!state || !mmx4_coop_split_views() || !mmx4_coop_ready() || mmx4_coop_projected() ||
       !psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+1) || psx_mod_read_byte(MMX4_PLAY+12) ||
       psx_mod_read_byte(MMX4_PLAY+0x0F) || psx_mod_read_byte(MMX4_PLAY+0x10) ||
       psx_mod_read_byte(0x80175E9Cu) || !psx_mod_read_half(0x80175EA0u) ||
       psx_mod_read_half(0x80175EA0u)>31 ||
       psx_mod_read_half(0x80141BE6u) || psx_mod_read_half(0x8013E1BEu))return;
    int x=(int16_t)((uint16_t)camera[10]|(uint16_t)camera[11]<<8)+160;
    int y=(int16_t)((uint16_t)camera[14]|(uint16_t)camera[15]<<8)+128;
    int margin=psx_mod_widescreen_x_margin();
    unsigned sources=0,pulse=0;
    for(unsigned i=0;i<32;++i) {
        uint32_t actor=0x80142F98u+i*0x30u;
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+4)!=1)continue;
        if(psx_mod_read_byte(actor+1)==2 && !psx_mod_read_byte(actor+2) &&
           psx_mod_read_byte(MMX4_PLAY+13)==1 && psx_mod_read_half(0x80175EA0u)<=3)
            pulse=1;
        if(psx_mod_read_byte(actor+1)!=28)continue;
        ++sources;
        int dx=(int16_t)psx_mod_read_half(actor+10)-x;
        int dy=(int16_t)psx_mod_read_half(actor+14)-y;
        if(dx>-320-2*margin && dx<320+2*margin && dy>-240 && dy<240)return;
    }
    if(!sources && pulse) {
        /* The looping palette actor's XY is its allocation position, not
         * the region it lights. Its global pulse belongs to the native
         * camera being drawn before this private viewport is substituted. */
        int dx=(int16_t)psx_mod_read_half(CAMERA+10)+160-x;
        int dy=(int16_t)psx_mod_read_half(CAMERA+14)+128-y;
        if(dx>-320-2*margin && dx<320+2*margin && dy>-240 && dy<240)return;
        sources=1;
    }
    if(!sources)return; /* Other red flashes and ordinary fades remain native. */
    psx_mod_write_half(0x80175EA0u,0);
    psx_mod_write_byte(0x80166BB0u,(uint8_t)(psx_mod_read_byte(0x80166BB0u)|4u));
    mmx4_coop_call(cpu,0x8002A484u,0,0); /* Rebuild the unlit working palette. */
    mmx4_coop_call(cpu,0x80016004u,0,0); /* Upload within the private GPU pass. */
    psx_mod_counter_add("mmx4.coop.split-distant-explosion-flash-removed",1);
}
static int available(void) {
    return state && mmx4_coop_split_views() && mmx4_coop_ready() &&
        (!mmx4_coop_projected() || mmx4_coop_combat_actor_context() || spatial_call) &&
        psx_mod_read_byte(MMX4_PLAY)==6 &&
        !psx_mod_local_view_scope();
}
int mmx4_coop_split_sprite_outside(uint32_t actor) {
    if(!mmx4_coop_split_views() || !mmx4_coop_ready() ||
       psx_mod_read_byte(MMX4_PLAY)!=6)return 0;
    unsigned layer=psx_mod_read_byte(actor+0x14u);
    if(layer>=3)return 0; /* Screen-relative UI and scripted graphics. */
    uint32_t camera=CAMERA+layer*0x54u;
    int x=(int16_t)(psx_mod_read_half(actor+10)-psx_mod_read_half(camera+10));
    int y=(int16_t)(psx_mod_read_half(actor+14)-psx_mod_read_half(camera+14));
    int margin=psx_mod_widescreen_x_margin();
    /* 80024334 and the armor emitter use signed byte assembly offsets and
     * 16px tiles, including horizontal flips. Keep a conservative 144px
     * border, then reject before the GPU truncates coordinates to 11 bits.
     * An actor can remain live/drawable for the other camera; neither its
     * simulation visibility flag nor its lifetime may be changed here. */
    return x < -144-margin || x > 320+144+margin || y < -144 || y > 240+144;
}
static int draw_armor(CPUState *cpu,uint32_t address) {
    /* One native armor hook must retain both camera culling and rider order.
     * A second registration for this plugin replaces the first callback. */
    if(mmx4_coop_split_sprite_outside(cpu->gpr[4]))return mmx4_coop_finish(cpu,0);
    return mmx4_coop_render_rider_armor(cpu,address);
}
static int intro_boss(uint32_t actor) {
    return actor>=0x8013BED0u && actor<0x8013BED0u+48u*0x9Cu &&
        !((actor-0x8013BED0u)%0x9Cu) && psx_mod_read_byte(actor) &&
        psx_mod_read_byte(actor+1)==0;
}
static uint32_t intro_owner_slot(uint32_t actor) {
    return state+INTRO_OWNER_OFFSET+(actor-0x8013BED0u)/0x9Cu*4u;
}
static uint32_t intro_camera(uint32_t boss) {
    unsigned owner=state?psx_mod_read_word(intro_owner_slot(boss)):0;
    return owner && owner<=2 ? state+16u+(owner-1u)*CAMERA_BYTES : CAMERA;
}
static void intro_projectile_origin(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[16],parent=cpu->gpr[17];
    if(!available() || psx_mod_read_byte(MMX4_PLAY+12) || psx_mod_read_byte(MMX4_PLAY+13) ||
       actor<0x8013F328u || actor>=0x801406A8u || (actor-0x8013F328u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1) ||
       psx_mod_read_word(actor+0x7C)!=parent || !intro_boss(parent) ||
       psx_mod_read_byte(parent+0x14u)!=255)return;
    /* Native994E0's background shots add CAMERA to their screen-relative
     * parent. Match the body's recorded origin without another AI update. */
    cpu->gpr[3]=psx_mod_read_word(intro_camera(parent)+(address==0x80099634u?8u:12u));
    psx_mod_counter_add("mmx4.coop.intro-shot-view-origins",1);
}
static int intro_boss_camera(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(intro_camera_call || !available() || !intro_boss(actor) ||
       psx_mod_read_byte(MMX4_PLAY+12) || psx_mod_read_byte(MMX4_PLAY+13))return 0;
    initialize_cameras();
    if(address==0x80041DF0u) {
        /* Capture the participant while XY is still in world space. The
         * following background phase has screen coordinates, so choosing a
         * hero later would mistake the waiting player for its owner. Refresh
         * on every world departure, including a reused native actor slot. */
        Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
        unsigned owner=mmx4_coop_view_nearest(actors,
            (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
        psx_mod_write_word(intro_owner_slot(actor),owner+1u);
    }
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,sizeof canonical);
    copy_camera(CAMERA,intro_camera(actor));
    intro_camera_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    intro_camera_call=0;
    write_bytes(CAMERA,canonical,sizeof canonical);
    psx_mod_counter_add("mmx4.coop.intro-dragon-view-origins",1);
    return mmx4_coop_finish(cpu,result);
}
static void project_intro_sprite(uint32_t actor,uint32_t boss) {
    if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+0x14u)!=255)return;
    /* Native80041DF0/8004205C change this boss between screen and world
     * coordinates; type10 attachments inherit the same layer. Rebase only
     * in the private draw transaction, using the recorded participant's view.
     * Integer halfwords match the native emitter without fractional carry. */
    uint32_t camera=intro_camera(boss);
    psx_mod_write_half(actor+10,psx_mod_read_half(actor+10)+psx_mod_read_half(camera+10));
    psx_mod_write_half(actor+14,psx_mod_read_half(actor+14)+psx_mod_read_half(camera+14));
    psx_mod_write_byte(actor+0x14u,0);
    psx_mod_counter_add("mmx4.coop.split-intro-sprite-projected",1);
}
void mmx4_coop_split_project_intro_boss(void) {
    if(!psx_mod_local_view_scope() || !mmx4_coop_split_views() || !mmx4_coop_ready() ||
       psx_mod_read_byte(MMX4_PLAY)!=6 || psx_mod_read_byte(MMX4_PLAY+12) ||
       psx_mod_read_byte(MMX4_PLAY+13))return;
    for(unsigned i=0;i<48;++i) {
        uint32_t actor=0x8013BED0u+i*0x9Cu;
        if(intro_boss(actor))project_intro_sprite(actor,actor);
    }
    for(unsigned i=0;i<32;++i) {
        uint32_t actor=0x8013E510u+i*0x70u;
        if(psx_mod_read_byte(actor+1)==10 && intro_boss(psx_mod_read_word(actor+0x50)))
            project_intro_sprite(actor,psx_mod_read_word(actor+0x50));
    }
}
void mmx4_coop_split_reset(void) {
    if(!state || mmx4_coop_projected() || psx_mod_local_view_scope())return;
    for(unsigned offset=0;offset<STATE_BYTES;offset+=4)psx_mod_write_word(state+offset,0);
    scanning=bounds_call=supplemental=spatial_call=mushroom_camera_call=destructible_camera_call=lift_camera_call=jungle_background_call=background_call=0;
    background_actor=background_dx=background_parallax=frost_camera_call=train_scene_call=0;
    intro_explosion_call=intro_shake_call=intro_explosion_published=0;
    intro_camera_call=0;
    volcano_call=volcano_fireball_call=0;
}
int mmx4_coop_split_activate(void) {
    state=mmx4_coop_split_views()?psx_mod_memory_alloc(STATE_BYTES,4):0;
    mmx4_coop_split_reset();
    if(state)psx_mod_counter_add("mmx4.coop.split-state",state);
    return !mmx4_coop_split_views() || state!=0;
}
uint32_t mmx4_coop_split_digest(uint32_t seed) {
    if(state)for(unsigned offset=0;offset<STATE_BYTES;++offset)
        seed=(seed^psx_mod_read_byte(state+offset))*16777619u;
    return seed;
}
void mmx4_coop_split_actors(Mmx4CoopViewActor actors[2]) {
    unsigned projected=(unsigned)(mmx4_coop_projected()!=0);
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t *body=seat?mmx4_coop_second_body():mmx4_coop_first_body();
        int16_t world_x=seat==projected?(int16_t)psx_mod_read_half(MMX4_PLAYER+10):
            (int16_t)((uint16_t)body[10]|(uint16_t)body[11]<<8);
        int16_t world_y=seat==projected?(int16_t)psx_mod_read_half(MMX4_PLAYER+14):
            (int16_t)((uint16_t)body[14]|(uint16_t)body[15]<<8);
        actors[seat]=(Mmx4CoopViewActor){world_x,world_y,(unsigned)mmx4_coop_alive(seat)};
    }
}
unsigned mmx4_coop_split_view_seat(unsigned seat) {
    if(seat>1)seat=0;
    int owner=mmx4_coop_lifecycle_script_owner();
    if(owner>=0 && !mmx4_coop_lifecycle_paired_door())return (unsigned)owner;
    if(!mmx4_coop_alive(seat))return mmx4_coop_alive(seat^1u)?seat^1u:0;
    return seat;
}
int mmx4_coop_split_camera_copy(unsigned seat,uint8_t layers[0xFC]) {
    if(!state || !mmx4_coop_split_views() || seat>1)return 0;
    /* Initial arrivals render before the first enabled camera tick. Use the
     * authored starting camera without mutating snapshot state in a draw. */
    if(!psx_mod_read_word(state)) {read_bytes(CAMERA,layers,CAMERA_BYTES);return 1;}
    seat=mmx4_coop_split_view_seat(seat);
    read_bytes(state+16u+seat*CAMERA_BYTES,layers,CAMERA_BYTES);
    return 1;
}
static void initialize_cameras(void) {
    if(psx_mod_read_word(state))return;
    copy_camera(state+16u,CAMERA);
    copy_camera(state+16u+CAMERA_BYTES,CAMERA);
    psx_mod_write_word(state,1);
}
static void cannon_laser_origin(CPUState *cpu,uint32_t address) {
    unsigned type=address==0x8009D104u?20u:36u;
    unsigned value_register=address==0x800A3C00u?3u:2u;
    unsigned stage=psx_mod_read_byte(MMX4_PLAY+12);
    uint32_t beam=cpu->gpr[16],parent=cpu->gpr[17];
    if(!available() || (stage!=7 && stage!=8 && !(stage==11 && type==36u)) || beam<0x8013F328u ||
       beam>=0x801406A8u || (beam-0x8013F328u)%0x9Cu ||
       !psx_mod_read_byte(beam) || psx_mod_read_byte(beam+1)!=type ||
       psx_mod_read_word(beam+0x7C)!=parent || parent<0x8013BED0u ||
       parent>=0x8013DC10u || (parent-0x8013BED0u)%0x9Cu ||
       !psx_mod_read_byte(parent))return;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    if(!actors[0].active && !actors[1].active)return;
    unsigned owner=mmx4_coop_view_nearest(actors,
        (int16_t)psx_mod_read_half(parent+10),(int16_t)psx_mod_read_half(parent+14),0);
    initialize_cameras();
    /* Native9D100 and A3B04 clamp two cannon beam families against CAMERA.x,
     * dragging distant emitters into P1's view. A3BFC also derives a culling
     * extent from that origin. Sigma2 reuses the same type36 beams as the
     * earlier cannons. Supply the actual emitter's nearest view to
     * each loaded value; native collision, animation and lifetime run once,
     * retaining authored world coordinates in either facing direction. */
    cpu->gpr[value_register]=(uint32_t)(int32_t)(int16_t)
        psx_mod_read_half(state+16u+owner*CAMERA_BYTES+10u);
    psx_mod_counter_add("mmx4.coop.cannon-laser-view-origins",1);
}
static int intro_effect_near(uint32_t camera,int x,int y,int half_height) {
    int dx=x-((int16_t)psx_mod_read_half(camera+10)+160);
    int dy=y-((int16_t)psx_mod_read_half(camera+14)+128);
    int margin=psx_mod_widescreen_x_margin();
    return dx>-320-2*margin && dx<320+2*margin && dy>-240-half_height && dy<240+half_height;
}
static void copy_shake_command(uint32_t destination,uint32_t source,unsigned axis) {
    unsigned bit=axis?1u:16u;
    static const unsigned offsets[]={0x36,0x3A,0x3C,0x3E,0x45};
    psx_mod_write_half(destination+0x34,(uint16_t)(psx_mod_read_half(destination+0x34)|bit));
    for(unsigned i=0;i<5;++i)
        psx_mod_write_byte(destination+offsets[i]+axis,psx_mod_read_byte(source+offsets[i]+axis));
}
static int volcano_rolling_window(uint32_t camera) {
    int center=(int16_t)psx_mod_read_half(camera+10)+160;
    for(unsigned slot=0;slot<32;++slot) {
        uint32_t emitter=0x80142F98u+slot*0x30u;
        if(!psx_mod_read_byte(emitter) || psx_mod_read_byte(emitter+1)!=33 ||
           psx_mod_read_byte(emitter+2)!=0x10 || psx_mod_read_byte(emitter+4)!=1)continue;
        /* BC9F8/BCA88 admit the rightward rolling sequence from its authored
         * X+385..X+1216 window. This is the environmental rumble's audience,
         * even when its remaining boulders have not caught that view yet. */
        int dx=center-(int16_t)psx_mod_read_half(emitter+10);
        if(dx>384 && dx<=1216)return 1;
    }
    return 0;
}
static int intro_shake(CPUState *cpu,uint32_t address) {
    if(intro_shake_call || !state || !mmx4_coop_split_views() || !mmx4_coop_ready() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6)return 0;
    unsigned stage=psx_mod_read_byte(MMX4_PLAY+12);
    if(stage!=0 && stage!=4)return 0;
    uint32_t caller=cpu->gpr[31];
    unsigned explosion=caller==0x800BBD08u || caller==0x800BBD50u || caller==0x800BBE58u;
    unsigned impact=caller==0x800BE6ACu || caller==0x800BE778u ||
        caller==0x800BEF58u || caller==0x800BF104u || caller==0x800BF2D0u ||
        caller==0x800C0750u || caller==0x800C07F8u;
    if(stage==4) {
        explosion=0;
        impact=caller==0x800A0858u || caller==0x800A0CB8u || caller==0x800A0D6Cu ||
            caller==0x800A0E04u || caller==0x800A1438u || caller==0x800A14C0u;
    }
    if(!explosion && !impact)return 0;
    int x=(int16_t)psx_mod_read_half(CAMERA+10)+160;
    int y=(int16_t)psx_mod_read_half(CAMERA+14)+128;
    unsigned rolling_rumble=0;
    if(impact) {
        uint32_t actor=cpu->gpr[16];
        if(actor<0x80010000u || actor>0x801FF000u || !psx_mod_read_byte(actor))return 0;
        if(stage==4 && (actor<0x8013F328u || actor>=0x801406A8u ||
           (actor-0x8013F328u)%0x9Cu || psx_mod_read_byte(actor+1)!=33))return 0;
        x=(int16_t)psx_mod_read_half(actor+10);y=(int16_t)psx_mod_read_half(actor+14);
        /* A0CB0 is part1's recurring rolling-ground rumble, issued on every
         * grounded tick after the native quake-bit guard. It is not an
         * impact at the ball's current XY. A fast hero can be far ahead of
         * the remaining balls, especially after earlier balls hit the hero
         * waiting at spawn. Retain this shared native environmental command
         * in views inside the emitter's authored activation window;
         * falling/pillar impacts keep their locality. */
        rolling_rumble=stage==4 && !psx_mod_read_byte(MMX4_PLAY+13) &&
            caller==0x800A0CB8u && psx_mod_read_byte(actor+4)==4 &&
            !(psx_mod_read_byte(actor+0x15)&0x40u);
    }
    /* Subtype0 issues shake for the complete closing wall, whose authored
     * rows extend432px below its emitter. Test that full footprint, or a
     * hero at the shaft floor is wrongly considered distant from the top. */
    int half_height=(caller==0x800C0750u || caller==0x800C07F8u)?216:0;
    y+=half_height;
    initialize_cameras();
    unsigned axis=address==0x80028BACu;
    unsigned bit=axis?1u:16u;
    const unsigned offsets[]={0x36,0x3A,0x3C,0x3E,0x45};
    uint8_t previous[5];
    unsigned flags=psx_mod_read_half(CAMERA+0x34);
    for(unsigned i=0;i<5;++i)previous[i]=psx_mod_read_byte(CAMERA+offsets[i]+axis);
    intro_shake_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    intro_shake_call=0;
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        if(!mmx4_coop_alive(seat) || (!intro_effect_near(shadow,x,y,half_height) &&
           !(rolling_rumble && volcano_rolling_window(shadow))))continue;
        /* Independent views may already be partway through this rumble.
         * Preserve their native phase instead of restarting the amplitude
         * toggle whenever another view admits the shared command. */
        if(rolling_rumble && (psx_mod_read_half(shadow+0x34)&bit))continue;
        copy_shake_command(shadow,CAMERA,axis);
        if(intro_explosion_call || volcano_fireball_call)intro_explosion_published|=1u<<(seat*2u+axis);
        psx_mod_counter_add(stage==4?(seat?"mmx4.coop.p2-volcano-shakes":"mmx4.coop.p1-volcano-shakes"):
            (seat?"mmx4.coop.p2-intro-shakes":"mmx4.coop.p1-intro-shakes"),1);
    }
    /* The world event runs once. Only its native shake command is published;
     * room limits, camera origins and the other shake axis remain personal. */
    if(!intro_effect_near(CAMERA,x,y,half_height) &&
       !(rolling_rumble && volcano_rolling_window(CAMERA))) {
        psx_mod_write_half(CAMERA+0x34,(uint16_t)((psx_mod_read_half(CAMERA+0x34)&~bit)|(flags&bit)));
        for(unsigned i=0;i<5;++i)psx_mod_write_byte(CAMERA+offsets[i]+axis,previous[i]);
    }
    return mmx4_coop_finish(cpu,result);
}
static int intro_explosions(CPUState *cpu,uint32_t address) {
    if(intro_explosion_call || !available() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY+12) || psx_mod_read_byte(MMX4_PLAY+13))return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u ||
       psx_mod_read_byte(actor+1)!=28)return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    unsigned owner=!actors[0].active || (actors[1].active && actors[1].world_x>actors[0].world_x);
    uint8_t canonical[CAMERA_BYTES];
    int own=owner && mmx4_coop_split_scene_camera_begin(owner,canonical);
    unsigned canonical_owner=psx_mod_read_word(state+8u)==2u;
    intro_explosion_published=0;intro_explosion_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    intro_explosion_call=0;
    if(own) {
        mmx4_coop_split_scene_camera_end(owner,canonical,0);
        /* Restoring P1's camera must retain this event's newly published
         * nearby shake. Copy only commands actually issued this tick, or an
         * older shadow timer would keep restarting after every scope. The
         * canonical camera may also belong to a surviving P2. */
        for(unsigned axis=0;axis<2;++axis)
            if(intro_explosion_published&(1u<<(canonical_owner*2u+axis)))
                copy_shake_command(CAMERA,state+16u+canonical_owner*CAMERA_BYTES,axis);
    }
    intro_explosion_published=0;
    return mmx4_coop_finish(cpu,result);
}
static int volcano_context(void) {
    return state && mmx4_coop_split_views() && mmx4_coop_ready() &&
        !psx_mod_local_view_scope() && psx_mod_read_byte(MMX4_PLAY)==6 &&
        psx_mod_read_byte(MMX4_PLAY+12)==4;
}
static int volcano_controller(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(volcano_call || !volcano_context() || actor<0x80142F98u || actor>=0x80143598u ||
       (actor-0x80142F98u)%0x30u || psx_mod_read_byte(actor+1)!=33)return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    if(!actors[0].active && !actors[1].active)return 0;
    int x=(int16_t)psx_mod_read_half(actor+10),y=(int16_t)psx_mod_read_half(actor+14);
    unsigned subtype=psx_mod_read_byte(actor+2);
    unsigned owner=mmx4_coop_view_nearest(actors,x,y,0);
    if((subtype&0xF0u)!=0x20u) {
        owner=!actors[0].active || (actors[1].active && actors[1].world_x>actors[0].world_x);
        /* Keep an authored emitter available to a trailing living hero
         * while the other has already left its native activation window. */
        for(unsigned seat=0;seat<2;++seat) {
            int dx=actors[seat].world_x-x;
            int eligible=(subtype&0xF0u)==0 ? dx>0 && dx<=944 :
                (subtype&15u)?dx<=-385 && dx>=-959:dx>384 && dx<=1216;
            if(actors[seat].active && eligible)owner=seat;
        }
    }
    unsigned projected=(unsigned)(mmx4_coop_projected()!=0);
    if(projected && owner!=projected)return 0;
    initialize_cameras();uint8_t canonical[CAMERA_BYTES];
    read_bytes(CAMERA,canonical,sizeof canonical);
    copy_camera(CAMERA,state+16u+owner*CAMERA_BYTES);
    volcano_call=1;
    if(owner!=projected)mmx4_coop_enter_second();
    /* One original timer/emission pass. Allocation provenance already lives
     * in combat's serialized secondary-owner table; no XY-based guess is
     * possible for the falling branch's freshly allocated zeroed body. */
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(owner!=projected)mmx4_coop_leave_second();
    volcano_call=0;write_bytes(CAMERA,canonical,sizeof canonical);
    psx_mod_counter_add("mmx4.coop.volcano-emitter-view-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static int volcano_fireball(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(volcano_fireball_call || !volcano_context() || actor<0x8013F328u || actor>=0x801406A8u ||
       (actor-0x8013F328u)%0x9Cu || psx_mod_read_byte(actor+1)!=33)return 0;
    unsigned owner=mmx4_coop_secondary_allocation_owner(actor);
    /* Only falling initialization has no world position. Once placed, a
     * rolling/erupting ball may be beside the other hero. Keeping its remote
     * allocator's camera can make impact code test that distant view's
     * clear quake bit and restart the nearby view's quake every tick. */
    if(psx_mod_read_byte(actor+4) || !owner || owner>2 || !mmx4_coop_alive(owner-1u)) {
        Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
        owner=mmx4_coop_view_nearest(actors,(int16_t)psx_mod_read_half(actor+10),
            (int16_t)psx_mod_read_half(actor+14),0)+1u;
    }
    initialize_cameras();uint8_t canonical[CAMERA_BYTES];
    read_bytes(CAMERA,canonical,sizeof canonical);
    copy_camera(CAMERA,state+16u+(owner-1u)*CAMERA_BYTES);
    intro_explosion_published=0;volcano_fireball_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    volcano_fireball_call=0;write_bytes(CAMERA,canonical,sizeof canonical);
    unsigned canonical_owner=psx_mod_read_word(state+8u)==2u;
    for(unsigned axis=0;axis<2;++axis)
        if(intro_explosion_published&(1u<<(canonical_owner*2u+axis)))
            copy_shake_command(CAMERA,state+16u+canonical_owner*CAMERA_BYTES,axis);
    intro_explosion_published=0;
    psx_mod_counter_add("mmx4.coop.volcano-fireball-view-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static uint32_t byte_word(const uint8_t *bytes,unsigned offset) {
    return (uint32_t)bytes[offset]|(uint32_t)bytes[offset+1]<<8|
        (uint32_t)bytes[offset+2]<<16|(uint32_t)bytes[offset+3]<<24;
}
static uint32_t signed_half(uint32_t value) {
    /* Retail sra retains the sign bit, including negative odd fixed units. */
    return (value>>1)|(value&0x80000000u);
}
static void train_panorama_wrap(uint32_t camera) {
    unsigned first=psx_mod_read_byte(camera+0x4Du);
    unsigned last=psx_mod_read_byte(camera+0x4Eu);
    unsigned width=psx_mod_read_byte(0x80172224u);
    if(psx_mod_read_byte(camera+4)!=5 || first>last || last>=width)return;
    int x=(int16_t)psx_mod_read_half(camera+10);
    /* Retail273F8..274CC resets the scrolling plane's integer origin to
     * the opposite authored screen. Canonical draws retain that reset;
     * private draws roll it back, so an ever-increasing saved origin would
     * reset to the same screen on every subsequent draw and appear frozen.
     * Retain the same wrap in the simulation's saved plane. Fractional X,
     * previous origins, foreground geometry and the train clock stay native. */
    if(x>=(int)(last+1u)*256)
        psx_mod_write_half(camera+10,(uint16_t)(first*256u));
    else if(first && x<=(int)(first-1u)*256)
        psx_mod_write_half(camera+10,(uint16_t)(last*256u));
}
static void mushroom_midboss_camera_unlock(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[address==0x8005D7C4u?18:4];
    unsigned phase=address==0x8005D7C4u?2u:3u;
    if(!available() || !psx_mod_read_word(state) ||
       psx_mod_read_byte(MMX4_PLAY+12)!=3 || psx_mod_read_byte(MMX4_PLAY+13) ||
       actor<0x8013BED0u || actor>=0x8013DC10u || (actor-0x8013BED0u)%0x9Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=33 ||
       psx_mod_read_byte(actor+4)!=phase || psx_mod_read_byte(actor+0x5C) ||
       cpu->gpr[2]<4352u || cpu->gpr[2]>4608u)return;
    /* Tentoroid's original death opens the left room limit two pixels per
     * tick at5D7C4, then finishes at4352 at5D80C. Publish those same target
     * writes to saved views still in its4608 enclosure. Otherwise P2 stays
     * clamped at4616 after a native kill while living P1 can leave normally.
     * Each camera keeps its own origin and native limit easing. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        unsigned minimum=psx_mod_read_half(shadow+0x26);
        if(psx_mod_read_half(shadow+0x24)!=4608 || minimum<4352 || minimum>4608)continue;
        psx_mod_write_half(shadow+0x26,(uint16_t)cpu->gpr[2]);
        psx_mod_counter_add("mmx4.coop.split-mushroom-camera-unlocks",1);
    }
}
static void owl_midboss_camera_unlock(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[16];
    if(!available() || !psx_mod_read_word(state) ||
       psx_mod_read_byte(MMX4_PLAY+12)!=7 || psx_mod_read_byte(MMX4_PLAY+13)!=1 ||
       actor<0x8013BED0u || actor>=0x8013BED0u+48u*0x9Cu ||
       (actor-0x8013BED0u)%0x9Cu || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=70 || psx_mod_read_byte(actor+4)!=2 ||
       psx_mod_read_byte(actor+0x5C) || cpu->gpr[2]!=2560u)return;
    /* Retail891FC/89204 releases Generaid Core's2240 enclosure by writing
     * both current and target maxX=2560. Its one shared death update only
     * changes CAMERA; a saved partner view otherwise retains the old wall.
     * Publish exactly those two writes to views still in that enclosure.
     * Origins, other limits, actors and native death/tile edits stay intact. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        if(psx_mod_read_half(shadow+0x24)!=2240 ||
           psx_mod_read_half(shadow+0x26)!=2240)continue;
        psx_mod_write_half(shadow+0x1C,(uint16_t)cpu->gpr[2]);
        psx_mod_write_half(shadow+0x24,(uint16_t)cpu->gpr[2]);
        psx_mod_counter_add("mmx4.coop.split-midboss-camera-unlocks",1);
    }
}
static int destructible_camera_unlock(CPUState *cpu,uint32_t address) {
    if(destructible_camera_call || !available() ||
       psx_mod_read_byte(MMX4_PLAY+12) || psx_mod_read_byte(MMX4_PLAY+13))return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80165A30u || actor>=0x80166BB0u || (actor-0x80165A30u)%0x8Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=4 ||
       psx_mod_read_byte(actor+2))return 0;
    initialize_cameras();destructible_camera_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    /* Retail C0D98 retires the intro's destroyed type4/subtype0 door and
     * writes 4544 to the current/target maximum X at CAMERA+1C/+24. The
     * shared map opens for both players, but independent camera snapshots
     * otherwise retain the old 2976 limit and trap P2 after the opening.
     * Native 2174C dispatches 32 8C-byte slots before the camera tick at
     * 211D8, so both shadows must receive the release before that tick.
     * Publish exactly those native writes; all other room bounds, origins,
     * and the one map/palette/actor finalization remain native. */
    for(unsigned seat=0;seat<2;++seat)if(mmx4_coop_alive(seat)) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        psx_mod_write_half(shadow+0x1C,psx_mod_read_half(CAMERA+0x1C));
        psx_mod_write_half(shadow+0x24,psx_mod_read_half(CAMERA+0x24));
    }
    destructible_camera_call=0;
    psx_mod_counter_add("mmx4.coop.split-destructible-camera-unlocks",1);
    return mmx4_coop_finish(cpu,result);
}
void mmx4_coop_split_dragoon_camera_release(uint32_t actor) {
    if(!available() || !psx_mod_read_word(state) ||
       psx_mod_read_byte(MMX4_PLAY+12)!=12 || psx_mod_read_byte(MMX4_PLAY+13) ||
       psx_mod_read_byte(MMX4_PLAY+0x1D)!=2 ||
       actor<0x8013BED0u || actor>=0x8013BED0u+48u*0x9Cu ||
       (actor-0x8013BED0u)%0x9Cu || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=65 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5)!=3 ||
       psx_mod_read_half(CAMERA+0x24)!=432 ||
       psx_mod_read_half(CAMERA+0x26)!=272 ||
       psx_mod_read_byte(CAMERA+0x48)!=4)return;
    /* Retail7E400/7E424/7E430 writes target min272/max432 and follow step4
     * after36B18 has ended the shared intro. P2's saved view still has the
     * closed432..432 enclosure; a projected owner can lose the writes for
     * both views. Publish exactly these targets/tuning to that enclosure.
     * Keep origins, current limits, vertical rooms and native easing intact;
     * hidden arrivals also need the release when they become visible again. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        if(psx_mod_read_half(shadow+0x24)!=432 ||
           psx_mod_read_half(shadow+0x26)!=432 ||
           psx_mod_read_half(shadow+0x28)!=psx_mod_read_half(CAMERA+0x28) ||
           psx_mod_read_half(shadow+0x2A)!=psx_mod_read_half(CAMERA+0x2A))continue;
        psx_mod_write_half(shadow+0x24,psx_mod_read_half(CAMERA+0x24));
        psx_mod_write_half(shadow+0x26,psx_mod_read_half(CAMERA+0x26));
        psx_mod_write_byte(shadow+0x48,psx_mod_read_byte(CAMERA+0x48));
        psx_mod_counter_add("mmx4.coop.split-dragoon-camera-releases",1);
    }
}
static int mushroom_lift_camera(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(lift_camera_call)return 0;
    if(!available())return mmx4_coop_lifecycle_lift_start(cpu,address);
    if(psx_mod_read_byte(MMX4_PLAY+12)!=3 ||
       actor<0x80165A30u || actor>=0x80166BB0u || (actor-0x80165A30u)%0x8Cu ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=14)return 0;
    initialize_cameras();
    uint8_t before[CAMERA_BYTES];read_bytes(CAMERA,before,sizeof before);
    lift_camera_call=1;
    uint32_t result=mmx4_coop_lifecycle_lift_start(cpu,address)?cpu->gpr[2]:
        mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    /* Native C3B40 releases the lift's vertical targets from2048 to720.
     * C3BA4/C3C9C subsequently change the current limits at its top. Apply
     * those authored writes to every view in this shaft, once, without
     * repeating the lift, panels, sound, checkpoint or collision update.
     * Each camera keeps its own origin and ordinary native follow. A view
     * in another horizontal room retains its independent limits. */
    static const unsigned horizontal[]={0x1C,0x1E,0x24,0x26};
    static const unsigned vertical[]={0x20,0x22,0x28,0x2A,0x2C};
    for(unsigned seat=0;seat<2;++seat)if(mmx4_coop_alive(seat)) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        unsigned same_room=1;
        for(unsigned i=0;i<4;++i)if(psx_mod_read_half(shadow+horizontal[i])!=
            (uint16_t)(before[horizontal[i]]|(uint16_t)before[horizontal[i]+1]<<8))same_room=0;
        if(!same_room)continue;
        for(unsigned i=0;i<5;++i) {
            unsigned offset=vertical[i];
            uint16_t old=(uint16_t)(before[offset]|(uint16_t)before[offset+1]<<8);
            uint16_t next=psx_mod_read_half(CAMERA+offset);
            if(next!=old)psx_mod_write_half(shadow+offset,next);
        }
        if(before[0x47]!=psx_mod_read_byte(CAMERA+0x47))
            psx_mod_write_byte(shadow+0x47,psx_mod_read_byte(CAMERA+0x47));
    }
    lift_camera_call=0;
    psx_mod_counter_add("mmx4.coop.split-lift-camera",1);
    return mmx4_coop_finish(cpu,result);
}
static int train_foreground(CPUState *cpu,uint32_t address) {
    if(background_call!=1 || cpu->gpr[4]!=background_actor)return 0;
    /* Full native800D964C adds foreground displacement only on this branch.
     * Observe it at the actual dispatch, after the native phase selector.
     * A stopped train's +7 transition does not add parallax on that tick. */
    if(psx_mod_read_byte(background_actor+7) && !psx_mod_read_byte(MMX4_PLAYER+0xBC)) {
        background_parallax=1;
        background_dx=psx_mod_read_word(CAMERA+8)-psx_mod_read_word(CAMERA+20);
    }
    background_call=2;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    background_call=1;
    return mmx4_coop_finish(cpu,result);
}
static int moving_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    unsigned storm=address==0x800DA298u;
    unsigned kind=storm?5u:address==0x800D8648u?0u:2u;
    if(!storm && !train_scene_call && !background_call && mmx4_coop_ready() &&
       !mmx4_coop_projected() && !psx_mod_local_view_scope() &&
       psx_mod_read_byte(MMX4_PLAY)==6 && psx_mod_read_byte(MMX4_PLAY+12)==8 &&
       actor>=TRAIN_POOL && actor<TRAIN_POOL+TRAIN_SLOTS*TRAIN_ACTOR_BYTES &&
       !((actor-TRAIN_POOL)%TRAIN_ACTOR_BYTES) && psx_mod_read_byte(actor+1)==kind) {
        /* D8BB4/D993C select phases from PLAYER.x. D8E94/D9B08 then
         * start real camera locks, bomb scenes and train stop/start events.
         * Keep this one controller with the progressing hero and retain
         * that owner through the subsequent native command completion. */
        train_scene_call=1;
        int handled=mmx4_coop_lifecycle_train(cpu,address);
        train_scene_call=0;
        if(handled) {
            /* A P2-owned admission temporarily installs its camera. The
             * nested world update publishes native scrolling to both saved
             * views, but an uncommitted scene restores the old canonical
             * camera afterward. Keep that canonical view's newly published
             * scenery, or its next camera snapshot erases the scroll every
             * frame. Foreground limits/origins and scene ownership still
             * follow the lifecycle transaction; never rerun the controller. */
            if(state && mmx4_coop_split_views()) {
                unsigned previous=psx_mod_read_word(state+8u);
                if(previous==1 || previous==2) {
                    uint32_t shadow=state+16u+(previous-1u)*CAMERA_BYTES;
                    uint8_t scenery[2u*0x54u];
                    read_bytes(shadow+0x54u,scenery,sizeof scenery);
                    write_bytes(CAMERA+0x54u,scenery,sizeof scenery);
                }
            }
            return handled;
        }
    }
    unsigned train_view=train_scene_call && state && mmx4_coop_split_views() &&
        mmx4_coop_ready() && !psx_mod_local_view_scope();
    if(background_call || (!available() && !train_view) ||
       (mmx4_coop_projected() && !train_view) ||
       psx_mod_read_byte(MMX4_PLAY+12)!=(storm?7u:8u) || actor<TRAIN_POOL ||
       actor>=TRAIN_POOL+TRAIN_SLOTS*TRAIN_ACTOR_BYTES ||
       (actor-TRAIN_POOL)%TRAIN_ACTOR_BYTES ||
       psx_mod_read_byte(actor+1)!=kind)return 0;
    initialize_cameras();
    uint8_t before[CAMERA_BYTES],after[CAMERA_BYTES];
    read_bytes(CAMERA,before,CAMERA_BYTES);
    unsigned storm_live=storm && psx_mod_read_byte(actor+4)<=1;
    background_actor=actor;background_dx=background_parallax=0;background_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    read_bytes(CAMERA,after,CAMERA_BYTES);
    unsigned previous=psx_mod_read_word(state+8u);
    unsigned owner=train_view && mmx4_coop_projected()?1u:previous?previous-1u:0u;
    /* QA87 traces D9218 a0=80169CB8, ra=80021BDC. Native80021B34 and its
     * allocator8002B048 agree: four30-byte actors, table800F2AB4, kinds0/2.
     * The train's mode5 helpers cannot recreate these shared actor writes.
     * They run after the current player camera snapshot in80021158.
     * Apply their writes to both saved views, never to canonical RAM again.
     * 0/2 add +18 speed and its arithmetic half. Only D9218's inspected
     * 800D964C branch also adds foreground motion; replace that contribution
     * with the other view's own displacement when the branch actually runs.
     * Storm Owl's kind5 controller uses the same pool/timing, but DA358's
     * foreground contribution uses signed truncation instead of sra. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        uint32_t seat_dx=psx_mod_read_word(shadow+8)-psx_mod_read_word(shadow+20);
        for(unsigned layer=0;layer<3;++layer) {
            unsigned base=layer*0x54u;
            for(unsigned offset=0;offset<0x54u;++offset) {
                if((offset>=8u && offset<16u) || (offset>=20u && offset<28u))continue;
                if(before[base+offset]!=after[base+offset])
                    psx_mod_write_byte(shadow+base+offset,after[base+offset]);
            }
            /* Shared authored limits/shake metadata are copied above, while
             * each foreground's current/previous origins stay independent. */
            if(!layer)continue;
            for(unsigned axis=8;axis<=12;axis+=4) {
                uint32_t delta=byte_word(after,base+axis)-byte_word(before,base+axis);
                uint32_t value=psx_mod_read_word(shadow+base+axis)+delta;
                if(background_parallax && seat!=owner && axis==8) {
                    uint32_t native_part=layer==1?background_dx:signed_half(background_dx);
                    uint32_t seat_part=layer==1?seat_dx:signed_half(seat_dx);
                    value+=seat_part-native_part;
                }
                if(storm_live && axis==8 && seat!=owner) {
                    /* DA358 adds the shared aircraft speed and each view's
                     * foreground displacement/2 (near) or /4 (far). Retail
                     * divides previous-current with truncation toward zero;
                     * preserve its negative odd fixed-unit rounding exactly.
                     * The one controller also advances stage events: never
                     * rerun it for the other camera. */
                    uint32_t native_part=byte_word(before,20)-byte_word(before,8);
                    uint32_t seat_part=0u-seat_dx;
                    for(unsigned n=0;n<layer;++n) {
                        native_part=signed_half(native_part+(native_part>>31));
                        seat_part=signed_half(seat_part+(seat_part>>31));
                    }
                    value+=native_part-seat_part;
                }
                psx_mod_write_word(shadow+base+axis,value);
            }
            if(!storm)train_panorama_wrap(shadow+base);
        }
    }
    /* A P2 scene transaction saves its installed camera once more on exit.
     * Retain the same completed wrap there, or that save would replace P2's
     * corrected shadow with the unwrapped native controller result again. */
    if(!storm && train_view)for(unsigned layer=1;layer<3;++layer)
        train_panorama_wrap(CAMERA+layer*0x54u);
    background_call=background_parallax=0;background_actor=background_dx=0;
    psx_mod_counter_add(storm?"mmx4.coop.split-storm-background-ticks":
                               "mmx4.coop.split-train-background-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static int jungle_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(jungle_background_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+12)!=1 || psx_mod_read_byte(MMX4_PLAY+13)!=1 ||
       actor<TRAIN_POOL || actor>=TRAIN_POOL+TRAIN_SLOTS*TRAIN_ACTOR_BYTES ||
       (actor-TRAIN_POOL)%TRAIN_ACTOR_BYTES || psx_mod_read_byte(actor+1)!=1 ||
       psx_mod_read_byte(actor+4)>1)return 0;
    if(!mmx4_coop_split_views()) {
        /* This controller selects scenery directly from PLAYER's X/Y.
         * Unified camera helpers already follow a surviving P2, but cannot
         * correct this selection from the dead canonical body's old room. */
        if(mmx4_coop_alive(0) || !mmx4_coop_alive(1) ||
           mmx4_coop_lifecycle_script_owner()>=0)return 0;
        jungle_background_call=1;mmx4_coop_enter_second();
        uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
        mmx4_coop_leave_second();jungle_background_call=0;
        psx_mod_counter_add("mmx4.coop.unified-survivor-jungle-background",1);
        return mmx4_coop_finish(cpu,result);
    }
    if(!available())return 0;
    unsigned slot=(actor-TRAIN_POOL)/TRAIN_ACTOR_BYTES;
    if(mmx4_coop_lifecycle_script_owner()>=0) {
        /* Shared scenes replace complete camera records. Their mode/offset
         * may belong to the other region, so initialize each private phase
         * again when ordinary independent follow resumes. */
        for(unsigned seat=0;seat<2;++seat)
            psx_mod_write_byte(state+JUNGLE_OFFSET+(seat*TRAIN_SLOTS+slot)*TRAIN_ACTOR_BYTES,0);
        return 0;
    }
    initialize_cameras();
    uint8_t controller[TRAIN_ACTOR_BYTES],canonical[CAMERA_BYTES];
    read_bytes(actor,controller,sizeof controller);
    unsigned previous=psx_mod_read_word(state+8u),owner=previous?previous-1u:0u;
    jungle_background_call=1;
    /* Native D91A8 selects the scenery phase from player X/Y. The canonical
     * pool still runs once, with its current living camera owner. */
    if(owner)mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(owner)mmx4_coop_leave_second();
    read_bytes(CAMERA,canonical,sizeof canonical);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t local=state+JUNGLE_OFFSET+(seat*TRAIN_SLOTS+slot)*TRAIN_ACTOR_BYTES;
        if(!mmx4_coop_alive(seat)) {
            /* Its camera follows the survivor while dead; discard the
             * preceding independent region phase before any revival. */
            psx_mod_write_byte(local,0);continue;
        }
        if(!psx_mod_read_byte(local) || !controller[4]) {
            write_bytes(local,controller,sizeof controller);
            /* A late enrollment must initialize this view's mode/offset,
             * even if the canonical controller already reached state 1. */
            psx_mod_write_byte(local+4,0);
        }
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        copy_camera(CAMERA,shadow);
        if(seat)mmx4_coop_enter_second();
        /* D8ED4's live states write only actor phase bytes and layer 1's
         * mode, offsets and origins (D8FDC..D9194). Keeping those phase bytes
         * per seat preserves its native one-tick region transitions. No
         * animation, palette, collision or shared stage callback is repeated.
         * In particular mode 5's ordinary camera helper cannot update this
         * layer: it otherwise stays at the opening forest over distant rooms. */
        mmx4_coop_call(cpu,address,local,0);
        if(seat)mmx4_coop_leave_second();
        copy_camera(shadow,CAMERA);
        psx_mod_counter_add(seat?"mmx4.coop.p2-jungle-background":
                                "mmx4.coop.p1-jungle-background",1);
    }
    write_bytes(CAMERA,canonical,sizeof canonical);
    jungle_background_call=0;
    return mmx4_coop_finish(cpu,result);
}
static int frost_snow_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(background_call || !available() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY+12)!=2 ||
       actor<0x80142F98u || actor>=0x80143598u ||
       (actor-0x80142F98u)%0x30u || psx_mod_read_byte(actor+1)!=9)return 0;
    initialize_cameras();
    uint32_t snow=CAMERA+2u*0x54u;
    uint8_t before[0x54],after[0x54];
    read_bytes(snow,before,sizeof before);background_call=3;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    read_bytes(snow,after,sizeof after);
    /* Native B75C8 owns one weather clock. B7764/B7934 add fractional wind
     * and wrap the vertical snow origin. Mode5's camera helper is a no-op,
     * so the noncanonical view otherwise keeps its initial frozen snow.
     * The origin encodes the shared weather atlas phase, independently of
     * foreground position. Publish its exact native value to both views,
     * including absolute phase resets; do not repeat the timer or update. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES+2u*0x54u;
        for(unsigned offset=0;offset<sizeof before;++offset) {
            if((offset>=8u && offset<16u) || (offset>=20u && offset<28u))continue;
            if(before[offset]!=after[offset])psx_mod_write_byte(shadow+offset,after[offset]);
        }
        for(unsigned axis=8;axis<=12;axis+=4)
            psx_mod_write_word(shadow+axis,byte_word(after,axis));
    }
    background_call=0;
    psx_mod_counter_add("mmx4.coop.split-snow-background-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
/* These small native controllers run after the player-camera snapshots.
 * Mode5's ordinary camera helper does not reproduce their animation writes. */
static int fortress_background(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    unsigned scroll=address==0x800DA878u,kind=scroll?7u:6u;
    if(background_call || !available() || mmx4_coop_projected() ||
       actor<TRAIN_POOL || actor>=TRAIN_POOL+TRAIN_SLOTS*TRAIN_ACTOR_BYTES ||
       (actor-TRAIN_POOL)%TRAIN_ACTOR_BYTES || psx_mod_read_byte(actor+1)!=kind)return 0;
    initialize_cameras();
    uint32_t layer=CAMERA+0x54u;
    uint8_t before[0x54],after[0x54];
    read_bytes(layer,before,sizeof before);
    unsigned running=psx_mod_read_byte(actor+4)<=1;
    background_call=4;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    read_bytes(layer,after,sizeof after);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t foreground=state+16u+seat*CAMERA_BYTES;
        uint32_t shadow=foreground+0x54u;
        for(unsigned offset=0;offset<sizeof before;++offset) {
            if((offset>=8u && offset<16u) || (offset>=20u && offset<28u))continue;
            if(before[offset]!=after[offset])psx_mod_write_byte(shadow+offset,after[offset]);
        }
        if(scroll && running) {
            /* DA90C adds +14 speed plus half this view's foreground
             * displacement (signed division toward zero), and follows its
             * own Y. Never transplant the other player's parallax origin. */
            uint32_t dx=psx_mod_read_word(foreground+8)-psx_mod_read_word(foreground+20);
            uint32_t half=signed_half(dx+(dx>>31));
            psx_mod_write_word(shadow+8,psx_mod_read_word(shadow+8)+
                psx_mod_read_word(actor+0x14)+half);
            psx_mod_write_word(shadow+12,psx_mod_read_word(foreground+12));
        }else if(!scroll && (before[10]!=after[10] || before[11]!=after[11])) {
            /* DA818 advances a shared four-cell atlas every eight ticks.
             * It writes only the integer X halfword and the dirty flag. */
            psx_mod_write_half(shadow+10,(uint16_t)(after[10]|(uint16_t)after[11]<<8));
        }
    }
    background_call=0;
    psx_mod_counter_add(scroll?"mmx4.coop.split-spaceport-background-ticks":
                               "mmx4.coop.split-fortress-background-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]) {
    if(!available() || owner>1 || (mmx4_coop_lifecycle_script_owner()>=0 &&
       !mmx4_coop_lifecycle_paired_door()) ||
       psx_mod_read_byte(MMX4_PLAY+0x10) ||
       (psx_mod_read_byte(MMX4_PLAY+0x1C) && !mmx4_coop_lifecycle_nova_active()))return 0;
    /* Nova's personal attack lock does not move the other hero's camera.
     * Door eligibility still runs natively, including while Both waits;
     * its trailing visibility query needs that waiting hero's real view. */
    initialize_cameras();read_bytes(CAMERA,canonical,CAMERA_BYTES);
    copy_camera(CAMERA,state+16u+owner*CAMERA_BYTES);
    return 1;
}
void mmx4_coop_split_door_camera_bounds(unsigned seats,const uint16_t bounds[4]) {
    if(!available())return;
    initialize_cameras();
    /* Publish the opening door's enclosure once. Subsequent camera-area
     * crossings belong to each hero, including a retreat down the stairs. */
    unsigned current=psx_mod_read_word(state+8u);
    for(unsigned seat=0;seat<2;++seat)if(seats&(1u<<seat))
        for(unsigned i=0;i<4;++i) {
            psx_mod_write_half(state+16u+seat*CAMERA_BYTES+0x1Cu+i*2u,bounds[i]);
            psx_mod_write_half(state+16u+seat*CAMERA_BYTES+0x24u+i*2u,bounds[i]);
            if(current==seat+1u) {
                psx_mod_write_half(CAMERA+0x1Cu+i*2u,bounds[i]);
                psx_mod_write_half(CAMERA+0x24u+i*2u,bounds[i]);
            }
        }
}
void mmx4_coop_split_commit_scene_camera(unsigned owner) {
    if(!state || owner>1 || !mmx4_coop_split_views() || !mmx4_coop_ready() ||
       psx_mod_local_view_scope())return;
    initialize_cameras();
    /* The native scene has admitted its owner, even if WARNING has already
     * frozen ordinary camera follow. Begin its shared pan in that real view. */
    copy_camera(CAMERA,state+16u+owner*CAMERA_BYTES);
    psx_mod_write_word(state+8u,owner+1u);
    psx_mod_counter_add("mmx4.coop.intro-entry-camera-commits",1);
}
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed) {
    copy_camera(state+16u+owner*CAMERA_BYTES,CAMERA);
    if(committed)psx_mod_write_word(state+8u,owner+1u);
    else write_bytes(CAMERA,canonical,CAMERA_BYTES);
}
void mmx4_coop_split_camera_prepare(unsigned owner) {
    if(!available() || owner>1)return;
    initialize_cameras();
    unsigned previous=psx_mod_read_word(state+8u);
    /* A surviving P2 must start its native camera update in its own room,
     * rather than inherit the fallen P1's origins and clamping targets. */
    if(previous && previous!=owner+1u && mmx4_coop_lifecycle_script_owner()<0 &&
       !psx_mod_read_byte(MMX4_PLAY+0x10) && !psx_mod_read_byte(MMX4_PLAY+0x1C))
        copy_camera(CAMERA,state+16u+owner*CAMERA_BYTES);
    psx_mod_write_word(state+8u,owner+1u);
}
void mmx4_coop_split_flip_cameras(void) {
    if(!available() || !psx_mod_read_word(state))return;
    for(unsigned seat=0;seat<2;++seat)for(unsigned layer=0;layer<3;++layer) {
        uint32_t camera=state+16u+seat*CAMERA_BYTES+layer*0x54u;
        uint32_t axis=layer?0x09000000u:0x07000000u;
        for(unsigned at=12;at<=24;at+=12)
            psx_mod_write_word(camera+at,axis-psx_mod_read_word(camera+at));
        if(!layer) {
            /* C4F40 also calls C5210, which replaces the room's vertical
             * follow/limit pairs. Mirroring XY alone leaves P2 clamped to
             * the old floor after the world has turned over, crushing a
             * grounded partner on the very first flipped camera update.
             * Keep each view's horizontal limits and position independent. */
            static const unsigned offsets[]={0x20,0x22,0x28,0x2A};
            for(unsigned i=0;i<4;++i)psx_mod_write_half(camera+offsets[i],
                psx_mod_read_half(CAMERA+offsets[i]));
        }else {
            /* C5210 shifts the authored parallax Y origin by 128/192.
             * Apply its delta to each view's own origin, once per flip. */
            int delta=layer==1?128:192;
            if(!psx_mod_read_byte(MMX4_PLAY+0x26))delta=-delta;
            psx_mod_write_half(camera+0x42,psx_mod_read_half(camera+0x42)+delta);
        }
        psx_mod_write_byte(camera+0x4C,1);
    }
}
static void mushroom_lift_scroll(const uint8_t canonical[CAMERA_BYTES]) {
    if(psx_mod_read_byte(MMX4_PLAY+12)!=3 || psx_mod_read_byte(MMX4_PLAY+13)!=1)return;
    static const unsigned horizontal[]={0x1C,0x1E,0x24,0x26};
    for(unsigned i=0;i<4;++i) {
        unsigned at=horizontal[i];
        if(psx_mod_read_half(CAMERA+at)!=(uint16_t)(canonical[at]|
            (uint16_t)canonical[at+1]<<8))return;
    }
    for(unsigned slot=0;slot<32;++slot) {
        uint32_t actor=0x80165A30u+slot*0x8Cu;
        if(!psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=14 ||
           psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5)!=1)continue;
        /* C3BA4 owns one forced ascent, including its camera-dependent top
         * events. Personal high jumps otherwise change a private camera's
         * current limits and accumulate >100px of drift from that world
         * scroll. Retain the shared vertical rail after native player follow,
         * before deriving parallax. Horizontal views and rider physics remain
         * independent. Native phase2 releases this rail at the lift's top. */
        psx_mod_write_word(CAMERA+12,byte_word(canonical,12));
        psx_mod_write_word(CAMERA+24,byte_word(canonical,24));
        static const unsigned vertical[]={0x20,0x22,0x28,0x2A};
        for(unsigned i=0;i<4;++i) {
            unsigned at=vertical[i];
            psx_mod_write_half(CAMERA+at,(uint16_t)(canonical[at]|
                (uint16_t)canonical[at+1]<<8));
        }
        psx_mod_counter_add("mmx4.coop.split-lift-scroll",1);
        return;
    }
}
void mmx4_coop_split_camera(CPUState *cpu,unsigned owner) {
    if(!available())return;
    initialize_cameras();
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    mmx4_coop_call(cpu,0x80027D40u,0,0);
    mmx4_coop_call(cpu,0x800281E8u,0,0);
    copy_camera(state+16u+owner*CAMERA_BYTES,CAMERA);
    psx_mod_write_word(state+8u,owner+1u);
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    unsigned other=owner^1u;
    /* PLAY+10 also freezes the world while a native pickup counts health.
     * That pause must not copy room bounds or clamp the distant partner. */
    unsigned paired=mmx4_coop_lifecycle_paired_door();
    if((mmx4_coop_lifecycle_script_owner()>=0 && !paired) ||
       (psx_mod_read_byte(MMX4_PLAY+0x1C) && !paired && !mmx4_coop_lifecycle_nova_active()) || !mmx4_coop_alive(other)) {
        copy_camera(state+16u+other*CAMERA_BYTES,state+16u+owner*CAMERA_BYTES);
        mmx4_coop_lifecycle_camera_bounds(cpu,owner);
        return;
    }
    uint32_t shadow=state+16u+other*CAMERA_BYTES;
    /* The opening door publishes its enclosure once. Recopying these limits
     * each frame also copies a trailing hero's later return-area bounds,
     * which can clamp the partner back through the open doorway. */
    /* Current/target limits, dead zones and authored room metadata belong
     * to the individual view. Shared scripts synchronize whole records above. */
    for(unsigned offset=0;offset<8u;++offset)psx_mod_write_byte(shadow+offset,canonical[offset]);
    copy_camera(CAMERA,shadow);
    if(other)mmx4_coop_enter_second();
    mmx4_coop_call(cpu,0x80027850u,0,0);
    mushroom_lift_scroll(canonical);
    mmx4_coop_call(cpu,0x80027D40u,0,0);
    mmx4_coop_call(cpu,0x800281E8u,0,0);
    copy_camera(shadow,CAMERA);
    if(other)mmx4_coop_leave_second();
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    psx_mod_counter_add("mmx4.coop.split-camera-ticks",1);
    if(paired)psx_mod_counter_add("mmx4.coop.split-paired-door-camera-ticks",1);
}
static int ram_range(uint32_t address,unsigned bytes) {
    return address>=0x80010000u && address<0x80200000u && bytes<=0x80200000u-address;
}
static int in_camera_area(uint32_t parameters,Mmx4CoopViewActor actor) {
    return actor.active && ram_range(parameters,10u) &&
        actor.world_x<=(int)psx_mod_read_half(parameters) &&
        actor.world_x>=(int)psx_mod_read_half(parameters+2u) &&
        actor.world_y<=(int)psx_mod_read_half(parameters+4u) &&
        actor.world_y>=(int)psx_mod_read_half(parameters+6u);
}
static void apply_camera_area(uint32_t parameters,uint32_t destination) {
    /* Retail 800B5798: region bounds then IDs in 8010AE74's (variable,value)
     * table, resolved through 8010AF9C. Only camera variables are per view;
     * the one original callback applies any shared variables once. */
    psx_mod_write_byte(destination+0x47u,2);
    for(unsigned index=4;index<36 && ram_range(parameters+index*2u,2);++index) {
        unsigned id=psx_mod_read_half(parameters+index*2u);if(!id)break;
        if(id>74u)break;
        uint32_t pair=0x8010AE74u+(id-1u)*4u;
        unsigned variable=psx_mod_read_half(pair);if(variable>=256u)break;
        uint32_t target=psx_mod_read_word(0x8010AF9Cu+variable*4u);
        if(target>=CAMERA && target+2u<=CAMERA+CAMERA_BYTES && !(target&1u))
            psx_mod_write_half(destination+target-CAMERA,psx_mod_read_half(pair+2u));
    }
}
static int spatial_stage_actor(CPUState *cpu,uint32_t address) {
    if(spatial_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_read_byte(MMX4_PLAY)!=6 || psx_mod_local_view_scope())return 0;
    int split=mmx4_coop_split_views();
    if(split && !available())return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u)return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    unsigned kind=address==0x800B56F4u?0u:6u;
    if(psx_mod_read_byte(actor+1u)!=kind)return 0;
    if(split)initialize_cameras();
    uint32_t parameters=psx_mod_read_byte(actor+4u)?psx_mod_read_word(actor+0x18u):0;
    Mmx4CoopViewActor eligible[2]={actors[0],actors[1]};
    for(unsigned seat=0;seat<2;++seat) {
        if(!kind && ram_range(parameters,10u))eligible[seat].active=(unsigned)in_camera_area(parameters,actors[seat]);
        if(kind)eligible[seat].active=actors[seat].active &&
            ((psx_mod_read_byte(actor+2u)&0xF0u)?
                actors[seat].world_y>=(int16_t)psx_mod_read_half(actor+14u):
                actors[seat].world_x>=(int16_t)psx_mod_read_half(actor+10u));
    }
    int scene=mmx4_coop_lifecycle_script_owner();
    if(!kind && mmx4_coop_lifecycle_paired_door())scene=-1;
    unsigned target=scene>=0?(unsigned)scene:mmx4_coop_view_nearest(
        eligible[0].active || eligible[1].active?eligible:actors,
        (int16_t)psx_mod_read_half(actor+10u),(int16_t)psx_mod_read_half(actor+14u),0);
    uint8_t canonical[CAMERA_BYTES];
    int16_t old_bounds[4];
    for(unsigned i=0;i<4;++i)old_bounds[i]=(int16_t)psx_mod_read_half(CAMERA+0x24u+i*2u);
    int own_camera=target && mmx4_coop_split_scene_camera_begin(target,canonical);
    spatial_call=1;
    uint8_t campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
    if(target) {mmx4_coop_enter_second();psx_mod_write_byte(MMX4_PLAY+0x43,campaign);}
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(target) {
        psx_mod_write_byte(MMX4_PLAY+0x43,psx_mod_read_byte(MMX4_PLAYER+2));
        mmx4_coop_leave_second();
    }
    spatial_call=0;
    if(own_camera)mmx4_coop_split_scene_camera_end(target,canonical,0);
    if(!split && !kind && scene<0 && actors[target].active && actors[target^1u].active) {
        int16_t bounds[4];
        for(unsigned i=0;i<4;++i)bounds[i]=(int16_t)psx_mod_read_half(CAMERA+0x24u+i*2u);
        int narrowed=bounds[0]<old_bounds[0] || bounds[1]>old_bounds[1] ||
            bounds[2]<old_bounds[2] || bounds[3]>old_bounds[3];
        Mmx4CoopViewActor other=actors[target^1u];
        if(narrowed && bounds[0]>=bounds[1] && bounds[2]>=bounds[3] &&
           (other.world_x<bounds[1] || other.world_x>bounds[0]+320 ||
            other.world_y<bounds[3] || other.world_y>bounds[2]+256))
            mmx4_coop_lifecycle_room_entry(target);
    }
    if(split && !kind && scene<0 && !psx_mod_read_byte(MMX4_PLAY+0x10) && !psx_mod_read_byte(MMX4_PLAY+0x1C)) {
        parameters=psx_mod_read_word(actor+0x18u);
        for(unsigned seat=0;seat<2;++seat)if(in_camera_area(parameters,actors[seat])) {
            apply_camera_area(parameters,state+16u+seat*CAMERA_BYTES);
            if(seat==(unsigned)(!mmx4_coop_alive(0)))apply_camera_area(parameters,CAMERA);
        }
    }
    psx_mod_counter_add(target?"mmx4.coop.p2-spatial-stage":"mmx4.coop.p1-spatial-stage",1);
    return mmx4_coop_finish(cpu,result);
}
static void mushroom_stair_views(CPUState *cpu,uint32_t actor,unsigned section) {
    if(!available() || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(MMX4_PLAY+1) || psx_mod_read_byte(MMX4_PLAY+0x10) ||
       psx_mod_read_byte(MMX4_PLAY+0x1C))return;
    initialize_cameras();
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,sizeof canonical);
    unsigned owner=psx_mod_read_word(state+8u);
    owner=owner?owner-1u:0u;
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t animation=state+STAIRS_OFFSET+seat*0x30u;
        /* Retail B7528's first region is the rotating staircase, in either
         * section. Only B7140's camera/animation helper is repeated here:
         * B7394/B7440/B74EC still commit shared checkpoints exactly once. */
        if(!actors[seat].active || actors[seat].world_x>=
           (int16_t)psx_mod_read_half(section?0x8010BE1Cu:0x8010B4D0u)) {
            psx_mod_write_byte(animation+6,0);continue;
        }
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        copy_camera(CAMERA,shadow);
        mmx4_coop_call(cpu,section?0x800BA644u:0x800B7140u,animation,0);
        copy_camera(shadow,CAMERA);
        if(seat==owner) {
            canonical[4]=psx_mod_read_byte(CAMERA+4);
            read_bytes(CAMERA+0x54u+8,canonical+0x54u+8,4);
            canonical[0x54u+0x4C]=psx_mod_read_byte(CAMERA+0x54u+0x4C);
        }
        psx_mod_counter_add(seat?"mmx4.coop.p2-stair-animation":"mmx4.coop.p1-stair-animation",1);
    }
    write_bytes(CAMERA,canonical,sizeof canonical);
}
static int mushroom_stage_camera(CPUState *cpu,uint32_t address) {
    if(mushroom_camera_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+12)!=3 || mmx4_coop_lifecycle_script_owner()>=0 ||
       !mmx4_coop_alive(1))return 0;
    uint32_t actor=cpu->gpr[4];
    unsigned section=address==0x800BA57Cu;
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u ||
       psx_mod_read_byte(actor+1)!=(section?22u:8u))return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    unsigned returning=!section && psx_mod_read_byte(MMX4_PLAY+0x1D)>=2;
    unsigned second_leads=!actors[0].active || (returning?actors[0].world_x>actors[1].world_x:
                                                       actors[0].world_x<actors[1].world_x);
    if(!second_leads && !mmx4_coop_split_views())return 0;
    /* Retail B7528 selects the spiral/room phase from player X/Y. B7394
     * advances to the lift checkpoint at X3536; B7440/B74EC advance the
     * leftward return at its top. These routines read player position but
     * write only shared camera/script fields. Supply the progressing seat
     * for one native update; no player controller or world loop is repeated. */
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8),y=psx_mod_read_word(MMX4_PLAYER+12);
    const uint8_t *body=mmx4_coop_second_body();
    uint32_t second_x=(uint32_t)body[8]|(uint32_t)body[9]<<8|
        (uint32_t)body[10]<<16|(uint32_t)body[11]<<24;
    uint32_t second_y=(uint32_t)body[12]|(uint32_t)body[13]<<8|
        (uint32_t)body[14]<<16|(uint32_t)body[15]<<24;
    if(second_leads) {
        psx_mod_write_word(MMX4_PLAYER+8,second_x);psx_mod_write_word(MMX4_PLAYER+12,second_y);
    }
    mushroom_camera_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    mushroom_camera_call=0;
    psx_mod_write_word(MMX4_PLAYER+8,x);psx_mod_write_word(MMX4_PLAYER+12,y);
    mushroom_stair_views(cpu,actor,section);
    if(second_leads)psx_mod_counter_add("mmx4.coop.p2-mushroom-stage",1);
    return mmx4_coop_finish(cpu,result);
}
static unsigned eligible_regions(PSXPlacementRect regions[2]) {
    Mmx4CoopView views[2];Mmx4CoopViewActor actors[2];
    initialize_cameras();mmx4_coop_split_actors(actors);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        views[seat]=(Mmx4CoopView){(int16_t)psx_mod_read_half(shadow+10),
            (int16_t)psx_mod_read_half(shadow+14)};
    }
    int owner=mmx4_coop_lifecycle_paired_door()?-1:mmx4_coop_lifecycle_script_owner();
    return mmx4_coop_view_regions(views,actors,owner,
        psx_mod_widescreen_x_margin(),regions);
}
static uint32_t placement_table(void) {
    int stage=(int8_t)psx_mod_read_byte(MMX4_PLAY+12);
    int section=(int8_t)psx_mod_read_byte(MMX4_PLAY+13);
    if(stage<0 || stage>=15 || section<0 || section>1)return 0;
    uint32_t table=psx_mod_read_word(0x800F43C8u+(unsigned)stage*8u+(unsigned)section*4u);
    /* Native records have an eight-byte stride relative to their table.
     * The table itself is only four-byte aligned (Sigma1: 800F848C). */
    return (table&3u) || table<0x80010000u || table>=0x80200000u?0:table;
}
static int placement_index(uint32_t record,unsigned *index) {
    uint32_t table=psx_mod_read_word(state+4u);
    if(!table || record<table || ((record-table)&7u) ||
       record-table>=MMX4_COOP_PLACEMENTS*8u)return 0;
    *index=(record-table)/8u;return 1;
}
static void reset_placements(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;mmx4_coop_split_reset();
}
static void placement_spawned(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(!available() || !cpu->gpr[17] || !placement_index(cpu->gpr[19],&index))return;
    uint32_t byte=state+VISITS_OFFSET+index/8u;
    psx_mod_write_byte(byte,psx_mod_read_byte(byte)|(1u<<(index&7u)));
    psx_mod_counter_add("mmx4.coop.split-placement-visited",1);
}
static int placement_allocate(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(!available() || cpu->gpr[31]!=0x8002916Cu ||
       !placement_index(cpu->gpr[19],&index))return 0;
    uint32_t record=cpu->gpr[19];unsigned category=psx_mod_read_byte(record+3u);
    if(supplemental && !mmx4_coop_view_prefetch_allowed(
       psx_mod_read_byte(MMX4_PLAY+12),psx_mod_read_byte(MMX4_PLAY+13),
       category,psx_mod_read_byte(record+1),psx_mod_read_half(CAMERA+0x26))) {
        psx_mod_counter_add("mmx4.coop.jet-prefetch-waits",1);
        return mmx4_coop_finish(cpu,0);
    }
    unsigned refight_portal=category==4u && psx_mod_read_byte(record+1u)==27u &&
        psx_mod_read_byte(MMX4_PLAY+12)==12 && !psx_mod_read_byte(MMX4_PLAY+13);
    unsigned volcano_emitter=category==3u && psx_mod_read_byte(record+1u)==33u &&
        psx_mod_read_byte(MMX4_PLAY+12)==4;
    if((psx_mod_read_byte(state+VISITS_OFFSET+index/8u)&(1u<<(index&7u))) ||
       (supplemental && (psx_mod_read_byte(record)&0x70u)) ||
       (supplemental && !(category<3u ||
            (category==4u && (psx_mod_read_byte(record+1u)==4u ||
                             psx_mod_read_byte(record+1u)==8u)) || refight_portal || volcano_emitter))) {
        psx_mod_counter_add("mmx4.coop.split-respawn-suppressed",1);
        return mmx4_coop_finish(cpu,0);
    }
    return 0;
}
static void scan_rectangle(CPUState *cpu,PSXPlacementRect region) {
    CPUState saved=*cpu;
    cpu->gpr[29]-=32u;
    uint32_t argument=cpu->gpr[29]+16u,previous=psx_mod_read_word(argument);
    cpu->gpr[4]=(uint32_t)region.left;cpu->gpr[5]=(uint32_t)region.right;
    cpu->gpr[6]=(uint32_t)region.top;cpu->gpr[7]=(uint32_t)region.bottom;
    psx_mod_write_word(argument,0);
    cpu->gpr[31]=0x80028FA8u;
    psx_dispatch_call(cpu,0x80028FECu,cpu->gpr[31]);
    psx_mod_write_word(argument,previous);
    if(cpu->muldiv_ts_done>saved.muldiv_ts_done)saved.muldiv_ts_done=cpu->muldiv_ts_done;
    if(cpu->gte_ts_done>saved.gte_ts_done)saved.gte_ts_done=cpu->gte_ts_done;
    *cpu=saved;
}
static int scan_players(CPUState *cpu,uint32_t address) {
    if(scanning || !available())return 0;
    uint32_t table=placement_table();if(!table)return 0;
    if(table!=psx_mod_read_word(state+4u)) {
        for(unsigned offset=VISITS_OFFSET;offset<STATE_BYTES;offset+=4)
            psx_mod_write_word(state+offset,0);
        psx_mod_write_word(state+4u,table);
    }
    PSXPlacementRect regions[2];unsigned count=eligible_regions(regions);
    for(unsigned index=0;index<MMX4_COOP_PLACEMENTS && table+index*8u+8u<=0x80200000u;++index) {
        uint32_t record=table+index*8u;
        if(psx_mod_read_byte(record+3u)==255u)break;
        int in_region=mmx4_coop_view_contains(regions,count,
            (int16_t)psx_mod_read_half(record+4u),(int16_t)psx_mod_read_half(record+6u));
        uint32_t byte=state+VISITS_OFFSET+index/8u;unsigned mask=1u<<(index&7u);
        if(!in_region)psx_mod_write_byte(byte,psx_mod_read_byte(byte)&~mask);
        else if(psx_mod_read_byte(record)&1u)psx_mod_write_byte(byte,psx_mod_read_byte(byte)|mask);
    }
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    int owner=mmx4_coop_lifecycle_paired_door()?-1:mmx4_coop_lifecycle_script_owner();scanning=1;
    uint32_t result=0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        copy_camera(CAMERA,state+16u+seat*CAMERA_BYTES);
        result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    }
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    /* A paired doorway can finish with the next door already inside a
     * stationary view, missing retail's moving-edge spawn strips. Include
     * unflagged General8 doors in the bounded full-view scan above; native
     * eligibility still owns admission, and shared visits prevent copies.
     * Other scripted General actors retain directional-only activation. */
    supplemental=1;
    for(unsigned region=0;region<count;++region)scan_rectangle(cpu,regions[region]);
    supplemental=scanning=0;
    psx_mod_counter_add("mmx4.coop.split-placement-scans",count);
    return mmx4_coop_finish(cpu,result);
}
static int actor_bounds(CPUState *cpu,uint32_t address) {
    if(address==0x8002B160u && mmx4_coop_lifecycle_assessment_bounds(cpu,address))return 1;
    uint32_t actor=cpu->gpr[4];
    if(bounds_call || !available() || (int8_t)psx_mod_read_byte(actor+0x14u)<0)return 0;
    uint32_t native=address;
    unsigned draw=address!=0x8002B1E8u && address!=0x8002B160u;
    if(address==0x8002B160u) {native=0x8002B1E8u;cpu->gpr[5]=cpu->gpr[6]=64;}
    if(address==0x8002B288u || address==0x8002B3C0u) {
        native=0x8002B318u;cpu->gpr[5]=address==0x8002B288u?32:96;
        cpu->gpr[6]=address==0x8002B288u?32:80;
    }
    unsigned layer=psx_mod_read_byte(actor+0x14u);if(layer>=3)return 0;
    initialize_cameras();
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    int owner=mmx4_coop_lifecycle_paired_door()?-1:mmx4_coop_lifecycle_script_owner();
    unsigned visible=0;
    uint32_t result=draw?0:1;bounds_call=1;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        copy_camera(CAMERA,state+16u+seat*CAMERA_BYTES);
        uint32_t candidate=mmx4_coop_call(cpu,native,actor,cpu->gpr[5]);
        if(draw) {
            if(psx_mod_read_byte(actor+3u)) {visible=1;result=candidate;}
        }else if(!candidate)result=0;
    }
    if(draw)psx_mod_write_byte(actor+3u,(uint8_t)visible);
    write_bytes(CAMERA,canonical,CAMERA_BYTES);bounds_call=0;
    return mmx4_coop_finish(cpu,result);
}
static unsigned frost_camera_phase(int x,int y) {
    /* 800B8E74's signed position test against the original stage table.
     * The upper route uses phases0/1; the lower shaft uses phases2..5. */
    unsigned phase=y<1280?0u:2u,limit=y<1280?2u:5u;
    while(phase<limit && x>=(int16_t)psx_mod_read_half(0x8010B6C4u+phase*2u))++phase;
    return y<1280 && phase==2u?1u:phase;
}
static int frost_stage_camera(CPUState *cpu,uint32_t address) {
    if(frost_camera_call || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       psx_mod_read_byte(MMX4_PLAY+12)!=2 || psx_mod_read_byte(MMX4_PLAY+13) ||
       mmx4_coop_lifecycle_script_owner()>=0 || !mmx4_coop_alive(1))return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u ||
       psx_mod_read_byte(actor+1)!=15)return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    /* B8E74 chooses the shared stage-camera phase from PLAYER's position.
     * Run this controller once for the living leader, including a surviving
     * P2. Its original state machine retains the world/layer progression. */
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8),y=psx_mod_read_word(MMX4_PLAYER+12);
    unsigned leader=!actors[0].active || actors[1].world_x>actors[0].world_x;
    if(leader) {
        const uint8_t *body=mmx4_coop_second_body();
        psx_mod_write_word(MMX4_PLAYER+8,byte_word(body,8));
        psx_mod_write_word(MMX4_PLAYER+12,byte_word(body,12));
    }
    frost_camera_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    frost_camera_call=0;
    psx_mod_write_word(MMX4_PLAYER+8,x);psx_mod_write_word(MMX4_PLAYER+12,y);
    if(leader)psx_mod_counter_add("mmx4.coop.p2-frost-stage-camera",1);
    if(available() && psx_mod_read_word(state)) {
        /* B8C00/B8C70/B8CE0/B8D50/B8DCC install the bottom follow
         * threshold only on phase entry. Each view must use its own phase:
         * a partner still outside the shaft keeps its normal dead zone.
         * Never replay the controller (phase5 changes shared world state),
         * and retain each camera's XY, limits and other follow settings. */
        static const uint16_t bottom[]={192,128,192,112,192};
        for(unsigned seat=0;seat<2;++seat)if(actors[seat].active) {
            unsigned phase=frost_camera_phase(actors[seat].world_x,actors[seat].world_y);
            if(phase<5u) {
                psx_mod_write_half(state+16u+seat*CAMERA_BYTES+0x2Eu,bottom[phase]);
                if(psx_mod_read_word(state+8u)==seat+1u)
                    psx_mod_write_half(CAMERA+0x2Eu,bottom[phase]);
            }
        }
    }
    return mmx4_coop_finish(cpu,result);
}
static void frost_shaft_camera_expand(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!available() || !psx_mod_read_word(state) || psx_mod_local_view_scope() ||
       psx_mod_read_byte(MMX4_PLAY+12)!=2 || psx_mod_read_byte(MMX4_PLAY+13) ||
       cpu->gpr[2]!=5728u)return;
    uint32_t old=psx_mod_read_half(CAMERA+0x24u);
    if(old!=5312u)return;
    /* Native B8D64 opens the later shaft's right-hand landing. A saved
     * partner view otherwise stays at5312 and clamps their body to5624,
     * above the pit. Release the matching current/target maximum together;
     * each view keeps its origin, vertical limits and native follow. */
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=state+16u+seat*CAMERA_BYTES;
        if(psx_mod_read_half(shadow+0x1Cu)==old && psx_mod_read_half(shadow+0x24u)==old) {
            psx_mod_write_half(shadow+0x1Cu,(uint16_t)cpu->gpr[2]);
            psx_mod_write_half(shadow+0x24u,(uint16_t)cpu->gpr[2]);
            psx_mod_counter_add("mmx4.coop.frost-shaft-camera-unlocks",1);
        }
    }
}
static int frost_ice_range(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!available() || psx_mod_read_byte(MMX4_PLAY+12)!=2 ||
       psx_mod_read_byte(MMX4_PLAY+13))return 0;
    initialize_cameras();
    /* B9E54 is a pure proximity query for the opening ice tile sequences.
     * Use its exact unsigned16 extents for each living view. The shared
     * cycle keeps its native clock, allocation mask and tile-edit queue. */
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    uint32_t point=cpu->gpr[4];
    int owner=mmx4_coop_lifecycle_script_owner();
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        unsigned layer=seat?mmx4_coop_second_body()[0x14]:
            psx_mod_read_byte(MMX4_PLAYER+0x14);
        if(layer>=3)continue;
        uint32_t camera=state+16u+seat*CAMERA_BYTES+layer*0x54u;
        uint16_t x=(uint16_t)(psx_mod_read_half(point)-psx_mod_read_half(camera+10)+640u);
        uint16_t y=(uint16_t)(psx_mod_read_half(point+2)-psx_mod_read_half(camera+14)+480u);
        if(x<1600u && y<1200u)return mmx4_coop_finish(cpu,1);
    }
    return mmx4_coop_finish(cpu,0);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_split) {
    psx_mod_register_instruction_plugin(COOP_ID,0x80099634u,0x8E240008u,intro_projectile_origin);
    psx_mod_register_instruction_plugin(COOP_ID,0x80099650u,0x00000000u,intro_projectile_origin);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800BC748u,volcano_controller);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800A03B8u,volcano_fireball);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80041DF0u,intro_boss_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8004205Cu,intro_boss_camera);
    psx_mod_register_instruction_plugin(COOP_ID,0x8009D104u,0x8623000Au,cannon_laser_origin);
    psx_mod_register_instruction_plugin(COOP_ID,0x800A3B08u,0x8623000Au,cannon_laser_origin);
    psx_mod_register_instruction_plugin(COOP_ID,0x800A3C00u,0x8622000Au,cannon_laser_origin);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80028B68u,intro_shake);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80028BACu,intro_shake);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800BBC14u,intro_explosions);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B8AF8u,frost_stage_camera);
    psx_mod_register_instruction_plugin(COOP_ID,0x800B8D64u,0xA42219D4u,frost_shaft_camera_expand);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B9E54u,frost_ice_range);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800257BCu,draw_armor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B7078u,mushroom_stage_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800BA57Cu,mushroom_stage_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D8648u,moving_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D9218u,moving_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800DA298u,moving_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D964Cu,train_foreground);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800C0D98u,destructible_camera_unlock);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800C3880u,mushroom_lift_camera);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D8ED4u,jungle_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B75C8u,frost_snow_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800DA7C0u,fortress_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800DA878u,fortress_background);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B56F4u,spatial_stage_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B6A0Cu,spatial_stage_actor);
    psx_mod_register_function_entry_plugin(COOP_ID,0x80028DB4u,reset_placements);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80028E24u,scan_players);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800293E8u,placement_allocate);
    psx_mod_register_instruction_plugin(COOP_ID,0x80029284u,0xA2620000u,placement_spawned);
    psx_mod_register_instruction_plugin(COOP_ID,0x8005D7C4u,0xA42219D6u,mushroom_midboss_camera_unlock);
    psx_mod_register_instruction_plugin(COOP_ID,0x8005D80Cu,0xA42219D6u,mushroom_midboss_camera_unlock);
    psx_mod_register_instruction_plugin(COOP_ID,0x80089204u,0xA42219D4u,owl_midboss_camera_unlock);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B1E8u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B160u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B318u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B288u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B3C0u,actor_bounds);
}
