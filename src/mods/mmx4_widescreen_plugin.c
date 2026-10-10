#include "mod_capcom_bg2d.h"
#include "cpu_state.h"
#include "mod_memory.h"
#include "mod_visible_placements.h"
#include "mod_netplay.h"
#include "mmx4_coop_internal.h"
#include <string.h>

#define LAYERS 0x801419b0u
#define PKG "mmx4.enhancement.widescreen"
static uint32_t arena, object_begin, placement_state;
#define PLACEMENT_COUNT 4096u
#define PLACEMENT_STATE_BYTES (32u+PLACEMENT_COUNT/8u)
static int object_pending, object_screen, supplemental_scan;
static uint32_t view_object_begin;
static int view_object_pending,view_object_screen,view_supplemental_scan;
void mmx4_widescreen_view_save(void) {
    view_object_begin=object_begin;view_object_pending=object_pending;
    view_object_screen=object_screen;view_supplemental_scan=supplemental_scan;
}
void mmx4_widescreen_view_restore(void) {
    object_begin=view_object_begin;object_pending=view_object_pending;
    object_screen=view_object_screen;supplemental_scan=view_supplemental_scan;
}
static const PSXCapcomBackground background = {0x8013e2f0u,0x8013bd50u,3u,8u,0x7900u};

/* Original layer function tables are800F3140/800F3164. Their horizontal
 * helpers80027EE8..80027FA4 and80028390..8002844C use arithmetic shifts.
 * Preserve that integer mapping and signed rounding in the shared view. */
static int scroll_mapping(unsigned mode, int x, int *value) {
    switch (mode) {
    case 1: case 4: *value=psx_capcom_floor_div(x,2); return 1;
    case 2: *value=x; return 1;
    case 3: *value=psx_capcom_floor_div(x,4); return 1;
    case 6: *value=psx_capcom_floor_div(x,2)+psx_capcom_floor_div(x,4); return 1;
    case 7: *value=x+psx_capcom_floor_div(x,4); return 1;
    case 8: *value=x+psx_capcom_floor_div(x,2); return 1;
    default: return 0;
    }
}
static void mmx4_bg_begin(CPUState *cpu, uint32_t address) {
    (void)address;
    gpu_ws_bg2d_begin_view_layer(cpu->gpr[4],psx_mod_read_word(0x1f800108u),6u);
}
static void mmx4_bg_end(CPUState *cpu, uint32_t address) {
    (void)address;
    unsigned layer=cpu->gpr[4]; uint32_t packet=psx_mod_read_word(0x1f800108u);
    gpu_ws_bg2d_end_view_layer(layer,packet);
    WsViewAnchor view;
    if (layer>=3) return;
    if (!gpu_ws_bg2d_get_view(layer,&view)) {
        /* The wide anchor is unavailable at native 4:3. Split's local pass
         * still needs authored tiles: the shared 32x32 ring follows the
         * canonical player and cannot represent a distant partner's view. */
        if(!mmx4_coop_split_views() || !psx_mod_local_view_scope())return;
        view=(WsViewAnchor){0,16,0,0,0};
    }
    uint32_t b=LAYERS+layer*0x54u;
    int sx=(int16_t)psx_mod_read_half(b+10),sy=(int16_t)psx_mod_read_half(b+14);
    if (layer) {
        WsViewAnchor fg; int camera=(int16_t)psx_mod_read_half(LAYERS+10),now,left;
        unsigned mode=psx_mod_read_byte(b+4);
        if (gpu_ws_bg2d_get_view(0,&fg) && scroll_mapping(mode,camera,&now) &&
            sx==(int16_t)(now+(int16_t)psx_mod_read_half(b+0x40)) &&
            scroll_mapping(mode,camera-fg.left,&left))
            view=psx_capcom_scroll_view(fg,now,left);
    }
    if(mmx4_coop_split_views() && psx_mod_local_view_scope() &&
       view.left<=0 && view.right<=0)view.right=16;
    unsigned width=psx_mod_read_byte(0x80172224u),stride=psx_mod_read_half(0x8013bd48u);
    PSXCapcomTileMap map={psx_mod_read_word(0x1f800004u),psx_mod_read_word(0x1f800008u),
        psx_mod_read_word(0x1f80000cu),width,width?stride/width:0,stride,layer,
        psx_mod_read_byte(b+0x4du),psx_mod_read_byte(b+0x4eu)};
    int packets=psx_capcom_background_render(&background,arena,psx_mod_read_word(0x1f800000u),
        &map,sx,sy,view,packet,NULL);
    if (packets>=0) psx_mod_counter_add("mmx4.renderer.tiles",(uint32_t)packets);
    else psx_mod_counter_add("mmx4.renderer.invalid-layer",1);
}
static void finish_objects(uint32_t end) {
    if (object_pending && end>=object_begin && end-object_begin<=1000u*40u) {
        for (uint32_t p=object_begin;p<end;p+=40u) {
            psx_mod_tag_world_primitive(p,!object_screen);
            if (object_screen) gpu_ws_tag_hud_prim(p,0);
        }
    }
    object_pending=0;
}
static void mmx4_object_begin(CPUState *cpu, uint32_t address) {
    uint32_t packet=psx_mod_read_word(0x1f800100u);finish_objects(packet);
    unsigned selector=address==0x80024334u?0x14u:0x37u;
    object_screen=(int8_t)psx_mod_read_byte(cpu->gpr[4]+selector)<0;
    object_begin=packet;object_pending=1;
}
static void mmx4_object_end(CPUState *cpu, uint32_t address) {
    (void)cpu;(void)address;finish_objects(psx_mod_read_word(0x1f800100u));
}
static void mmx4_actor_bounds(CPUState *cpu, uint32_t address) {
    (void)address;
    if ((int8_t)psx_mod_read_byte(cpu->gpr[4]+0x14u)>=0)
        cpu->gpr[5]+=(uint32_t)psx_mod_widescreen_x_margin();
}
static int mmx4_fixed_draw(CPUState *cpu, uint32_t address) {
    if (psx_mod_widescreen_x_margin()<=0 || (int8_t)psx_mod_read_byte(cpu->gpr[4]+0x14u)<0)return 0;
    cpu->gpr[5]=address==0x8002b288u?32u:96u;
    cpu->gpr[6]=address==0x8002b288u?32u:80u;
    psx_dispatch_call(cpu,0x8002b318u,cpu->gpr[31]);return 1;
}
static int mmx4_fixed_lifetime(CPUState *cpu, uint32_t address) {
    (void)address;
    if (psx_mod_widescreen_x_margin()<=0 || (int8_t)psx_mod_read_byte(cpu->gpr[4]+0x14u)<0)return 0;
    cpu->gpr[5]=64;cpu->gpr[6]=64;
    psx_dispatch_call(cpu,0x8002b1e8u,cpu->gpr[31]);return 1;
}
static void mmx4_placement_reset(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(placement_state)for(unsigned i=0;i<PLACEMENT_STATE_BYTES;i+=4)
        psx_mod_write_word(placement_state+i,0);
}
static int placement_index(uint32_t record,unsigned *index) {
    uint32_t base=psx_mod_read_word(placement_state);
    if(!base || record<base || ((record-base)&7u) || record-base>=PLACEMENT_COUNT*8u)return 0;
    *index=(record-base)/8u;return 1;
}
static int placement_is_trackable(uint32_t record) {
    unsigned category=psx_mod_read_byte(record+3u);
    /* Original category4/type4 handler800C081C installs the destructible
     * door's native hitbox and health. Its artwork is already part of the
     * authored map, so widening only its rendering leaves shots passing
     * through until the old scan finally creates the collision actor. */
    return category<3u || (category==4u && psx_mod_read_byte(record+1u)==4u);
}
static void mmx4_placement_spawned(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(mmx4_coop_split_views())return;
    /* Guarded original successful-allocation latch, not an allocation attempt.
     * Failed allocations must remain eligible on a later native scan. */
    if(!placement_state || !cpu->gpr[17] || !placement_is_trackable(cpu->gpr[19]) ||
       !placement_index(cpu->gpr[19],&index))return;
    uint32_t p=placement_state+32u+index/8u;
    psx_mod_write_byte(p,psx_mod_read_byte(p)|(1u<<(index&7u)));
    psx_mod_counter_add("mmx4.renderer.placement-visited",1);
}
static int mmx4_extra_placement(CPUState *cpu, uint32_t address) {
    (void)address;
    if(mmx4_coop_split_views())return 0;
    if (cpu->gpr[31]!=0x8002916cu)return 0;
    /* The original scanner's s2 points at record+3; s3 retains its latch.
     * Categories0..2 are ordinary enemy/item allocators. Scripted categories
     * remain native. Intro type31 is an authored enemy, not a script trigger. */
    uint32_t record=cpu->gpr[18];unsigned category=psx_mod_read_byte(record);
    if (placement_is_trackable(record-3u)) {
        unsigned index;
        if(placement_state && psx_mod_widescreen_x_margin()>0 && placement_index(record-3u,&index) &&
           (psx_mod_read_byte(placement_state+32u+index/8u)&(1u<<(index&7u)))) {
            cpu->gpr[2]=0;psx_mod_counter_add("mmx4.renderer.respawn-suppressed",1);return 1;
        }
        if(supplemental_scan)psx_mod_counter_add("mmx4.renderer.extra-enemy",1);
        return 0;
    }
    if(!supplemental_scan)return 0;
    cpu->gpr[2]=0;return 1;
}
static int signed_bound(int x) { return x<-32768?-32768:x>32767?32767:x; }
static unsigned split_cameras(uint8_t cameras[2][0xFC]) {
    if(!mmx4_coop_split_views() || !mmx4_coop_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope())return 0;
    int owner=mmx4_coop_lifecycle_script_owner();unsigned count=0;
    for(unsigned seat=0;seat<2;++seat)
        if(mmx4_coop_alive(seat) && (owner<0 || owner==(int)seat) &&
           mmx4_coop_split_camera_copy(seat,cameras[count]))++count;
    return count;
}
/* Intro controller category3/type7 scans a separate twelve-record table,
 * 8010B464, through800B6EB4. Reuse that scanner and its successful-allocation
 * latches/pool, including the original vertical window. The bounded full-view
 * scan also notices a stationary window resize; the native controller scans
 * only when a layer scrolls. Beam geometry and animation stay native. */
static void mmx4_searchlight_scan(CPUState *cpu, uint32_t address) {
    (void)address;int margin=psx_mod_widescreen_x_margin();
    uint8_t cameras[2][0xFC];unsigned count=split_cameras(cameras);
    if(psx_mod_local_view_scope() || (margin<=0 && !count))return;
    CPUState saved=*cpu;
    for(unsigned view=0;view<(count?count:1u);++view)for(unsigned layer=0;layer<3;++layer) {
        uint32_t b=LAYERS+layer*0x54u;
        int x=(int16_t)psx_mod_read_half(b+10),y=(int16_t)psx_mod_read_half(b+14);
        if(count) {
            const uint8_t *shadow=cameras[view]+layer*0x54u;
            x=(int16_t)((uint16_t)shadow[10]|(uint16_t)shadow[11]<<8);
            y=(int16_t)((uint16_t)shadow[14]|(uint16_t)shadow[15]<<8);
        }
        *cpu=saved;cpu->gpr[29]-=32u;
        uint32_t arg=cpu->gpr[29]+16u,old=psx_mod_read_word(arg);
        cpu->gpr[4]=(uint32_t)signed_bound(x-48-margin);
        cpu->gpr[5]=(uint32_t)signed_bound(x+368+margin);
        cpu->gpr[6]=(uint32_t)signed_bound(y-48);
        cpu->gpr[7]=(uint32_t)signed_bound(y+288);
        psx_mod_write_word(arg,saved.gpr[4]);cpu->gpr[31]=0x800b6cb4u;
        psx_dispatch_call(cpu,0x800b6eb4u,cpu->gpr[31]);
        psx_mod_write_word(arg,old);
        if(cpu->muldiv_ts_done>saved.muldiv_ts_done)saved.muldiv_ts_done=cpu->muldiv_ts_done;
        if(cpu->gte_ts_done>saved.gte_ts_done)saved.gte_ts_done=cpu->gte_ts_done;
    }
    *cpu=saved;psx_mod_counter_add("mmx4.renderer.searchlight-scan",1);
}
static int mmx4_searchlight_visible(CPUState *cpu, uint32_t address) {
    (void)address;int margin=psx_mod_widescreen_x_margin();
    uint8_t cameras[2][0xFC];unsigned count=split_cameras(cameras);
    uint32_t actor=cpu->gpr[4];int layer=(int8_t)psx_mod_read_byte(actor+0x37u);
    if((margin<=0 && !count) || layer<0 || layer>=3)return 0;
    /* Original800D4024 tests the origin and beam centre against its extents,
     * with uint16 wrap. Expand only those X tests; preserve both Y tests and
     * the original return value convention. State2 retires an invisible beam,
     * so widening spawning alone would immediately destroy fringe beams. */
    int x0=(int16_t)psx_mod_read_half(actor+0x16u);
    int x1=(int16_t)psx_mod_read_half(actor+0x1eu);
    int y0=(int16_t)psx_mod_read_half(actor+0x1au);
    int y1=(int16_t)psx_mod_read_half(actor+0x32u);
    uint32_t b=LAYERS+(unsigned)layer*0x54u;
    cpu->gpr[2]=0;
    for(unsigned view=0;view<(count?count:1u);++view) {
        Mmx4CoopView camera={(int16_t)psx_mod_read_half(b+10),(int16_t)psx_mod_read_half(b+14)};
        if(count) {
            const uint8_t *shadow=cameras[view]+(unsigned)layer*0x54u;
            camera.camera_x=(int16_t)((uint16_t)shadow[10]|(uint16_t)shadow[11]<<8);
            camera.camera_y=(int16_t)((uint16_t)shadow[14]|(uint16_t)shadow[15]<<8);
        }
        if(mmx4_coop_view_beam_contains(camera,(int16_t)psx_mod_read_half(actor+10),
            (int16_t)psx_mod_read_half(actor+14),x0,x1,y0,y1,margin))cpu->gpr[2]=1;
    }
    if(cpu->gpr[2])psx_mod_counter_add("mmx4.renderer.searchlight-visible",1);
    return 1;
}
static void mmx4_scan_view(CPUState *cpu, uint32_t address) {
    (void)address;int margin=psx_mod_widescreen_x_margin();
    if(mmx4_coop_split_views())return;
    if(!placement_state)return;
    if (margin<=0 || !psx_mod_read_byte(LAYERS)) { psx_mod_write_word(placement_state+4u,0);return; }
    int x=(int16_t)psx_mod_read_half(LAYERS+10),y=(int16_t)psx_mod_read_half(LAYERS+14);
    int stage=(int8_t)psx_mod_read_byte(0x801721ccu),variant=(int8_t)psx_mod_read_byte(0x801721cdu);
    uint32_t table=psx_mod_read_word(0x800f43c8u+(uint32_t)(stage*8+variant*4));
    if((table&7u) || table<0x80010000u || table>=0x80200000u)return;
    if(table!=psx_mod_read_word(placement_state)) {
        mmx4_placement_reset(cpu,address);psx_mod_write_word(placement_state,table);
    }
    PSXPlacementRect now={signed_bound(x-48-margin),signed_bound(x+368+margin),
        signed_bound(y-48),signed_bound(y+288)},old={
        (int32_t)psx_mod_read_word(placement_state+8),(int32_t)psx_mod_read_word(placement_state+12),
        (int32_t)psx_mod_read_word(placement_state+16),(int32_t)psx_mod_read_word(placement_state+20)};
    /* A killed placement stays visited while it is in the extended activation
     * area. Once it leaves, original respawn behavior is restored. Bitmap and
     * previous view live in snapshot/rollback memory, including save reloads. */
    for(unsigned i=0;i<PLACEMENT_COUNT && table+i*8u+8u<=0x80200000u;++i) {
        uint32_t record=table+i*8u,p=placement_state+32u+i/8u;unsigned mask=1u<<(i&7u);
        if(psx_mod_read_byte(record+3u)==255u)break;
        int inside=psx_placement_contains(now,(int16_t)psx_mod_read_half(record+4u),
            (int16_t)psx_mod_read_half(record+6u));
        if(inside && placement_is_trackable(record) && (psx_mod_read_byte(record)&1u))
            psx_mod_write_byte(p,psx_mod_read_byte(p)|mask);
        else if((psx_mod_read_byte(p)&mask) && !inside)
            psx_mod_write_byte(p,psx_mod_read_byte(p)&~mask);
    }
    PSXPlacementRect strips[4];unsigned count=psx_placement_exposed(old,now,
        psx_mod_read_word(placement_state+4u)!=0,strips);
    psx_mod_write_word(placement_state+4u,1);
    psx_mod_write_word(placement_state+8u,(uint32_t)now.left);psx_mod_write_word(placement_state+12u,(uint32_t)now.right);
    psx_mod_write_word(placement_state+16u,(uint32_t)now.top);psx_mod_write_word(placement_state+20u,(uint32_t)now.bottom);
    CPUState saved=*cpu;
    for (unsigned side=0;side<count;++side) {
        *cpu=saved;cpu->gpr[29]-=32u;uint32_t arg=cpu->gpr[29]+16u,old=psx_mod_read_word(arg);
        cpu->gpr[4]=(uint32_t)strips[side].left;cpu->gpr[5]=(uint32_t)strips[side].right;
        cpu->gpr[6]=(uint32_t)strips[side].top;cpu->gpr[7]=(uint32_t)strips[side].bottom;
        psx_mod_write_word(arg,0u);cpu->gpr[31]=0x80028fa8u;
        supplemental_scan=1;psx_dispatch_call(cpu,0x80028fecu,cpu->gpr[31]);supplemental_scan=0;
        psx_mod_write_word(arg,old);
        if (cpu->muldiv_ts_done>saved.muldiv_ts_done)saved.muldiv_ts_done=cpu->muldiv_ts_done;
        if (cpu->gte_ts_done>saved.gte_ts_done)saved.gte_ts_done=cpu->gte_ts_done;
    }
    *cpu=saved;
}
static void mmx4_widescreen_activate(void) {
    arena=psx_capcom_background_activate(3u);if(!arena)return;
    placement_state=psx_mod_memory_alloc(PLACEMENT_STATE_BYTES,4);
    gpu_ws_bg2d_configure(LAYERS,0x801441c8u,0x80172224u,0x8013bd48u,32u,3u,0x54u,1000u);
    gpu_ws_bg2d_set_parent_links(0);
    gpu_ws_set_view_anchor(LAYERS+10u,LAYERS+0x1eu,LAYERS+0x1cu,LAYERS);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x80026aa0u,mmx4_bg_begin);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x80026894u,mmx4_bg_end);
    static const uint32_t renderers[]={0x80024334u,0x80024920u,0x80024b9cu};
    for(unsigned i=0;i<3;++i)(void)psx_mod_register_function_entry_plugin("mmx4.widescreen",renderers[i],mmx4_object_begin);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x80024260u,mmx4_object_end);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x8002b1e8u,mmx4_actor_bounds);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x8002b318u,mmx4_actor_bounds);
    (void)psx_mod_register_function_filter_plugin("mmx4.widescreen",0x8002b288u,mmx4_fixed_draw);
    (void)psx_mod_register_function_filter_plugin("mmx4.widescreen",0x8002b3c0u,mmx4_fixed_draw);
    (void)psx_mod_register_function_filter_plugin("mmx4.widescreen",0x8002b160u,mmx4_fixed_lifetime);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x80028e24u,mmx4_scan_view);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x80028db4u,mmx4_placement_reset);
    (void)psx_mod_register_instruction_plugin("mmx4.widescreen",0x80029284u,0xa2620000u,mmx4_placement_spawned);
    (void)psx_mod_register_function_filter_plugin("mmx4.widescreen",0x800293e8u,mmx4_extra_placement);
    (void)psx_mod_register_function_entry_plugin("mmx4.widescreen",0x800b6c9cu,mmx4_searchlight_scan);
    (void)psx_mod_register_function_filter_plugin("mmx4.widescreen",0x800d4024u,mmx4_searchlight_visible);
    if(psx_mod_netplay_is_active()) {
        int aspect=psx_mod_netplay_aspect();
        (void)psx_mod_set_fixed_display_aspect(aspect==2?21u:aspect==1?16u:4u,aspect?9u:3u);
        return;
    }
    char aspect[16];
    if(!psx_mod_option_value(PKG,"widescreen","aspect",aspect,sizeof aspect))strcpy(aspect,"adaptive");
    if(!strcmp(aspect,"16:9"))(void)psx_mod_set_fixed_display_aspect(16u,9u);
    else if(!strcmp(aspect,"21:9"))(void)psx_mod_set_fixed_display_aspect(21u,9u);
    else if(!strcmp(aspect,"32:9"))(void)psx_mod_set_fixed_display_aspect(32u,9u);
    else { (void)psx_mod_set_fixed_display_aspect(16u,9u);(void)psx_mod_set_adaptive_display_aspect(0u,0u); }
}

PSX_MOD_CONSTRUCTOR(mmx4_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.widescreen", mmx4_widescreen_activate);
}
