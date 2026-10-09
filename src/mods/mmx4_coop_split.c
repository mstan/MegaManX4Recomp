#include "mmx4_coop_internal.h"
#include "mod_memory.h"
#include <string.h>

#define COOP_ID "mmx4.coop"
#define CAMERA 0x801419B0u
#define CAMERA_BYTES 0xFCu
#define VISITS_OFFSET (16u+2u*CAMERA_BYTES)
#define STATE_BYTES (VISITS_OFFSET+MMX4_COOP_PLACEMENTS/8u)

static uint32_t state;
static unsigned scanning,bounds_call,supplemental,spatial_call;

static void copy_bytes(uint32_t destination,uint32_t source,unsigned size) {
    for(unsigned offset=0;offset<size;++offset)
        psx_mod_write_byte(destination+offset,psx_mod_read_byte(source+offset));
}
static void read_bytes(uint32_t source,uint8_t *bytes,unsigned size) {
    for(unsigned offset=0;offset<size;++offset)bytes[offset]=psx_mod_read_byte(source+offset);
}
static void write_bytes(uint32_t destination,const uint8_t *bytes,unsigned size) {
    for(unsigned offset=0;offset<size;++offset)psx_mod_write_byte(destination+offset,bytes[offset]);
}
static int available(void) {
    return state && mmx4_coop_split_views() && mmx4_coop_ready() &&
        (!mmx4_coop_projected() || mmx4_coop_combat_actor_context() || spatial_call) &&
        psx_mod_read_byte(MMX4_PLAY)==6 &&
        !psx_mod_local_view_scope();
}
void mmx4_coop_split_reset(void) {
    if(!state || mmx4_coop_projected() || psx_mod_local_view_scope())return;
    for(unsigned offset=0;offset<STATE_BYTES;offset+=4)psx_mod_write_word(state+offset,0);
    scanning=bounds_call=supplemental=spatial_call=0;
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
    if(owner>=0)return (unsigned)owner;
    if(!mmx4_coop_alive(seat))return mmx4_coop_alive(seat^1u)?seat^1u:0;
    return seat;
}
int mmx4_coop_split_camera_copy(unsigned seat,uint8_t layers[0xFC]) {
    if(!state || !mmx4_coop_split_views() || seat>1 || !psx_mod_read_word(state))return 0;
    seat=mmx4_coop_split_view_seat(seat);
    read_bytes(state+16u+seat*CAMERA_BYTES,layers,CAMERA_BYTES);
    return 1;
}
static void initialize_cameras(void) {
    if(psx_mod_read_word(state))return;
    copy_bytes(state+16u,CAMERA,CAMERA_BYTES);
    copy_bytes(state+16u+CAMERA_BYTES,CAMERA,CAMERA_BYTES);
    psx_mod_write_word(state,1);
}
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]) {
    if(!available() || owner>1 || mmx4_coop_lifecycle_script_owner()>=0 ||
       psx_mod_read_byte(MMX4_PLAY+0x10) || psx_mod_read_byte(MMX4_PLAY+0x1C))return 0;
    initialize_cameras();read_bytes(CAMERA,canonical,CAMERA_BYTES);
    copy_bytes(CAMERA,state+16u+owner*CAMERA_BYTES,CAMERA_BYTES);
    return 1;
}
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed) {
    copy_bytes(state+16u+owner*CAMERA_BYTES,CAMERA,CAMERA_BYTES);
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
        copy_bytes(CAMERA,state+16u+owner*CAMERA_BYTES,CAMERA_BYTES);
    psx_mod_write_word(state+8u,owner+1u);
}
void mmx4_coop_split_camera(CPUState *cpu,unsigned owner) {
    if(!available())return;
    initialize_cameras();
    uint8_t canonical[CAMERA_BYTES];read_bytes(CAMERA,canonical,CAMERA_BYTES);
    mmx4_coop_call(cpu,0x80027D40u,0,0);
    mmx4_coop_call(cpu,0x800281E8u,0,0);
    copy_bytes(state+16u+owner*CAMERA_BYTES,CAMERA,CAMERA_BYTES);
    psx_mod_write_word(state+8u,owner+1u);
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    unsigned other=owner^1u;
    /* PLAY+10 also freezes the world while a native pickup counts health.
     * That pause must not copy room bounds or clamp the distant partner. */
    if(mmx4_coop_lifecycle_script_owner()>=0 ||
       psx_mod_read_byte(MMX4_PLAY+0x1C) || !mmx4_coop_alive(other)) {
        copy_bytes(state+16u+other*CAMERA_BYTES,state+16u+owner*CAMERA_BYTES,CAMERA_BYTES);
        mmx4_coop_lifecycle_camera_bounds(cpu,owner);
        return;
    }
    uint32_t shadow=state+16u+other*CAMERA_BYTES;
    /* Current/target limits, dead zones and authored room metadata belong
     * to the individual view. Shared scripts synchronize whole records above. */
    for(unsigned offset=0;offset<8u;++offset)psx_mod_write_byte(shadow+offset,canonical[offset]);
    copy_bytes(CAMERA,shadow,CAMERA_BYTES);
    if(other)mmx4_coop_enter_second();
    mmx4_coop_call(cpu,0x80027850u,0,0);
    mmx4_coop_call(cpu,0x80027D40u,0,0);
    mmx4_coop_call(cpu,0x800281E8u,0,0);
    copy_bytes(shadow,CAMERA,CAMERA_BYTES);
    if(other)mmx4_coop_leave_second();
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    psx_mod_counter_add("mmx4.coop.split-camera-ticks",1);
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
    if(spatial_call || !available() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u)return 0;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    unsigned kind=address==0x800B56F4u?0u:6u;
    if(psx_mod_read_byte(actor+1u)!=kind)return 0;
    initialize_cameras();
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
    unsigned target=scene>=0?(unsigned)scene:mmx4_coop_view_nearest(
        eligible[0].active || eligible[1].active?eligible:actors,
        (int16_t)psx_mod_read_half(actor+10u),(int16_t)psx_mod_read_half(actor+14u),0);
    uint8_t canonical[CAMERA_BYTES];
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
    if(!kind && scene<0 && !psx_mod_read_byte(MMX4_PLAY+0x10) && !psx_mod_read_byte(MMX4_PLAY+0x1C)) {
        parameters=psx_mod_read_word(actor+0x18u);
        for(unsigned seat=0;seat<2;++seat)if(in_camera_area(parameters,actors[seat])) {
            apply_camera_area(parameters,state+16u+seat*CAMERA_BYTES);
            if(seat==(unsigned)(!mmx4_coop_alive(0)))apply_camera_area(parameters,CAMERA);
        }
    }
    psx_mod_counter_add(target?"mmx4.coop.p2-spatial-stage":"mmx4.coop.p1-spatial-stage",1);
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
    return mmx4_coop_view_regions(views,actors,mmx4_coop_lifecycle_script_owner(),
        psx_mod_widescreen_x_margin(),regions);
}
static uint32_t placement_table(void) {
    int stage=(int8_t)psx_mod_read_byte(MMX4_PLAY+12);
    int section=(int8_t)psx_mod_read_byte(MMX4_PLAY+13);
    if(stage<0 || stage>=15 || section<0 || section>1)return 0;
    uint32_t table=psx_mod_read_word(0x800F43C8u+(unsigned)stage*8u+(unsigned)section*4u);
    return (table&7u) || table<0x80010000u || table>=0x80200000u?0:table;
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
    if((psx_mod_read_byte(state+VISITS_OFFSET+index/8u)&(1u<<(index&7u))) ||
       (supplemental && (psx_mod_read_byte(record)&0x70u)) ||
       (supplemental && !(category<3u ||
            (category==4u && psx_mod_read_byte(record+1u)==4u)))) {
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
    int owner=mmx4_coop_lifecycle_script_owner();scanning=1;
    uint32_t result=0;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        copy_bytes(CAMERA,state+16u+seat*CAMERA_BYTES,CAMERA_BYTES);
        result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    }
    write_bytes(CAMERA,canonical,CAMERA_BYTES);
    supplemental=1;
    for(unsigned region=0;region<count;++region)scan_rectangle(cpu,regions[region]);
    supplemental=scanning=0;
    psx_mod_counter_add("mmx4.coop.split-placement-scans",count);
    return mmx4_coop_finish(cpu,result);
}
static int actor_bounds(CPUState *cpu,uint32_t address) {
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
    int owner=mmx4_coop_lifecycle_script_owner();unsigned visible=0;
    uint32_t result=draw?0:1;bounds_call=1;
    for(unsigned seat=0;seat<2;++seat) {
        if(!actors[seat].active || (owner>=0 && owner!=(int)seat))continue;
        copy_bytes(CAMERA,state+16u+seat*CAMERA_BYTES,CAMERA_BYTES);
        uint32_t candidate=mmx4_coop_call(cpu,native,actor,cpu->gpr[5]);
        if(draw) {
            if(psx_mod_read_byte(actor+3u)) {visible=1;result=candidate;}
        }else if(!candidate)result=0;
    }
    if(draw)psx_mod_write_byte(actor+3u,(uint8_t)visible);
    write_bytes(CAMERA,canonical,CAMERA_BYTES);bounds_call=0;
    return mmx4_coop_finish(cpu,result);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_split) {
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B56F4u,spatial_stage_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B6A0Cu,spatial_stage_actor);
    psx_mod_register_function_entry_plugin(COOP_ID,0x80028DB4u,reset_placements);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80028E24u,scan_players);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800293E8u,placement_allocate);
    psx_mod_register_instruction_plugin(COOP_ID,0x80029284u,0xA2620000u,placement_spawned);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B1E8u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B160u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B318u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B288u,actor_bounds);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002B3C0u,actor_bounds);
}
