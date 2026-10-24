/* Ownership-boundary tests with a small deterministic native-call model.
 * These prove projection/lifecycle bookkeeping, not original game behavior;
 * the original executable stage/menu/vehicle matrix remains required. */
#include "mmx4_coop_internal.h"
void mmx4_coop_bosses_new_stage(void) {}
void mmx4_coop_bosses_stage_initialization(void) {}
uint32_t mmx4_coop_counterpart_story_text(void) {return 0x9F046000u;}
void mmx4_coop_balance_reset(void) {}
void mmx4_cheats_enforce_current(void) {} /* This fixture has no character codes. */
#include "ws_view_anchor.h"
#include <stdio.h>
int gpu_ws_bg2d_get_view(unsigned layer, struct WsViewAnchor *view) {
    (void)layer;*view=(WsViewAnchor){0};return 0;
}
int psx_mod_netplay_is_active(void) {return 0;}
int psx_mod_netplay_aspect(void) {return 0;}
static unsigned local_view;
int psx_mod_local_view_scope(void) {return (int)local_view;}
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint8_t ram[0x200000],p2[0xE4],p1_backup[0xE4];
static uint8_t p2_vehicle[0xB0],p1_vehicle_backup[0xB0];
static unsigned projected,ready=1,world_calls,observed_campaign,collect_calls[2];
static unsigned pause_commit,pause_exit,world_draws;
static unsigned personal_model,personal_native_calls[2],personal_draws[2];
static unsigned personal_exit_model,menu_asset_calls[2],late_menu_assets;
static unsigned assessment_calls[2];
static unsigned assessment_cull;
static unsigned reward_calls,save_calls,observed_menu_select;
static uint8_t saved_palette_dirty;
static unsigned stage_palette_phase,palette_swap_model,palette_uploads;
static uint32_t saved_palette_resource;
static uint16_t palette_uploaded;
static uint16_t palette_uploaded_world;
static uint32_t stage_animation_destination;
static unsigned stage_animation_frame;
#define PALETTE_SOURCE 0x801F4000u
#define SECOND_PALETTE_SOURCE 0x801F4100u
#define WORKING_PALETTE_COLOR 0x80141F72u
#define STAGE_PALETTE_OFFSET 0x9A0u
static int split_views;
static int both_door_players;
static int private_etanks_option;
static int simultaneous_menus_option;
int psx_mod_current_option_value(const char *key,char *value,uint32_t cap) {
    assert(cap>=6);
    if(!strcmp(key,"simultaneous_menus"))strcpy(value,simultaneous_menus_option?"true":"false");
    else {assert(!strcmp(key,"separate_etanks"));strcpy(value,private_etanks_option?"true":"false");}
    return 1;
}
int mmx4_coop_both_door_players(void) {return both_door_players;}
static unsigned stage_script_calls,scene_calls,observed_controls;
static unsigned stage_camera_model,stage_camera_commits,stage_camera_y;
static unsigned both_door_crossings;
static unsigned door_map_queries;
static uint32_t door_map_kind;
static unsigned canonical_portal_solids;
static unsigned latch_calls,latch_owner,latch_y;
static unsigned boundary_calls[2];
static unsigned dialogue_calls,dialogue_edge;
static unsigned dialogue_init_calls,dialogue_init_character,stage_script_projected;
static unsigned initial_arrival_model,initial_control_calls[2],incoming_calls[2];
static unsigned natural_arrivals,direct_arrivals,terrain_calls[2];
static unsigned direct_initializer_calls[2];
static unsigned surface_calls[2];
static unsigned ice_contact_model,ice_contact_calls[2];
static uint8_t terrain_contact;
static int narrow_return_x;
static unsigned flip_queries[2],flip_hits[2],flip_world_calls,flip_second_calls,flip_camera_calls;
static uint8_t p2_solid_bits;
static unsigned portal_hits[2],portal_admissions,portal_continuations[2];
static uint16_t inputs[2];
typedef struct { uint32_t address;PSXModFunctionFilterCallback filter; } Filter;
static Filter filters[64];static unsigned filter_count;
static PSXModFunctionEntryCallback stage_completed_predicate;
int psx_mod_register_instruction_plugin(const char *id,uint32_t address,uint32_t expected,
    PSXModFunctionEntryCallback callback) {
    (void)id;
    if(address==0x80020018u)stage_completed_predicate=callback;
    assert((address==0x800DA44Cu && expected==0x284218D0u) ||
           (address==0x800B9F74u && expected==0x2442FD5Fu) ||
           (address==0x800BA39Cu && expected==0x28420C60u) ||
           (address==0x800C0574u && expected==0x00041100u) ||
           (address==0x800BEEF4u && expected==0x00041100u) ||
           (address==0x800D0580u && expected==0x14620006u) ||
           (address==0x80081250u && expected==0x14620005u) ||
           (address==0x80020018u && expected==0x14620007u));return 1;
}
uint8_t psx_mod_read_byte(uint32_t a) {return ram[a&0x1FFFFFu];}
void psx_mod_write_byte(uint32_t a,uint8_t v) {ram[a&0x1FFFFFu]=v;}
uint16_t psx_mod_read_half(uint32_t a) {
    return (uint16_t)(psx_mod_read_byte(a)|(uint16_t)psx_mod_read_byte(a+1)<<8);
}
void psx_mod_write_half(uint32_t a,uint16_t v) {
    psx_mod_write_byte(a,(uint8_t)v);psx_mod_write_byte(a+1,(uint8_t)(v>>8));
}
uint32_t psx_mod_read_word(uint32_t a) {
    return (uint32_t)psx_mod_read_half(a)|(uint32_t)psx_mod_read_half(a+2)<<16;
}
void psx_mod_write_word(uint32_t a,uint32_t v) {
    psx_mod_write_half(a,(uint16_t)v);psx_mod_write_half(a+2,(uint16_t)(v>>16));
}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t a,PSXModFunctionFilterCallback f) {
    (void)id;assert(filter_count<64);
    for(unsigned i=0;i<filter_count;++i)assert(((filters[i].address^a)&0x1FFFFFFFu)!=0);
    filters[filter_count++]=(Filter){a,f};return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,PSXModFunctionEntryCallback f) {
    (void)id;(void)a;(void)f;return 1;
}
void psx_mod_counter_add(const char *name,uint32_t amount) {
    if(!strcmp(name,"mmx4.coop.concurrent-stage-arrivals"))natural_arrivals+=amount;
    if(!strcmp(name,"mmx4.coop.concurrent-return-arrivals"))direct_arrivals+=amount;
    if(!strcmp(name,"mmx4.coop.both-door-crossings"))both_door_crossings+=amount;
}
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat) {
    (void)cpu;++menu_asset_calls[seat];
    if(psx_mod_read_byte(0x801754A4u)==2 && psx_mod_read_byte(0x801754A5u)>=2)
        ++late_menu_assets;
}
static unsigned scene_camera_commits,scene_camera_owner;
void mmx4_coop_split_door_camera_bounds(unsigned seats,const uint16_t bounds[4]) {
    (void)seats;(void)bounds;
}
void mmx4_coop_split_commit_scene_camera(unsigned owner) {
    ++scene_camera_commits;scene_camera_owner=owner;
}
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]) {
    if(stage_camera_model && split_views) {
        assert(owner==1);memset(canonical,0,0xFC);
        psx_mod_write_half(0x801419BEu,736);return 1;
    }
    (void)owner;(void)canonical;return 0;
}
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed) {
    if(stage_camera_model) {assert(owner==1 && committed);++stage_camera_commits;}
    (void)owner;(void)canonical;(void)committed;
}
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat) {
    return seat?p2_solid_bits:psx_mod_read_byte(actor+0x72);
}
int mmx4_coop_ready(void) {return ready;}
int mmx4_coop_split_views(void) {return split_views;}
int mmx4_coop_projected(void) {return (int)projected;}
uint8_t *mmx4_coop_second_body(void) {return p2;}
uint8_t *mmx4_coop_first_body(void) {return p1_backup;}
uint8_t *mmx4_coop_second_vehicle(void) {return p2_vehicle;}
uint8_t *mmx4_coop_first_vehicle(void) {return p1_vehicle_backup;}
void mmx4_coop_clear_current_attacks(void) {}
void mmx4_coop_split_flip_cameras(void) {++flip_camera_calls;}
void mmx4_coop_flip_second(void) {
    ++flip_second_calls;
    for(unsigned at=12;at<=28;at+=16) {
        uint32_t y=(uint32_t)p2[at]|(uint32_t)p2[at+1]<<8|(uint32_t)p2[at+2]<<16|(uint32_t)p2[at+3]<<24;
        y=0x08000000u-y;
        for(unsigned i=0;i<4;++i)p2[at+i]=(uint8_t)(y>>(i*8));
    }
}
uint16_t mmx4_coop_input(unsigned seat) {return inputs[seat];}
int mmx4_coop_finish(CPUState *cpu,uint32_t v) {cpu->gpr[2]=v;return 1;}
int mmx4_coop_combat_canonical_call(CPUState *cpu,uint32_t address,
    PSXModFunctionFilterCallback callback) {return callback(cpu,address);}
int mmx4_coop_alive(unsigned seat) {
    if(mmx4_coop_lifecycle_hidden(seat))return 0;
    uint8_t *body=seat?p2:p1_backup;
    if((!projected && !seat) || (projected && seat))
        return psx_mod_read_byte(MMX4_PLAYER) &&
            (psx_mod_read_byte(MMX4_PLAYER+0x5C)&127u) &&
            psx_mod_read_byte(MMX4_PLAYER+4)<2;
    return body[0] && (body[0x5C]&127u) && body[4]<2;
}
void mmx4_coop_enter_second(void) {
    assert(!projected);
    saved_palette_dirty=psx_mod_read_byte(0x80166BB0u);
    if(palette_swap_model) {
        saved_palette_resource=psx_mod_read_word(0x1F800028u);
        psx_mod_write_word(0x1F800028u,SECOND_PALETTE_SOURCE);
    }
    for(unsigned i=0;i<sizeof p2;++i) {
        p1_backup[i]=psx_mod_read_byte(MMX4_PLAYER+i);
        psx_mod_write_byte(MMX4_PLAYER+i,p2[i]);
    }
    projected=1;mmx4_coop_lifecycle_project();
    for(unsigned i=0;i<sizeof p2_vehicle;++i) {
        p1_vehicle_backup[i]=psx_mod_read_byte(MMX4_VEHICLE+i);
        psx_mod_write_byte(MMX4_VEHICLE+i,p2_vehicle[i]);
    }
}
void mmx4_coop_leave_second(void) {
    assert(projected);mmx4_coop_lifecycle_restore();
    for(unsigned i=0;i<sizeof p2;++i) {
        p2[i]=psx_mod_read_byte(MMX4_PLAYER+i);
        psx_mod_write_byte(MMX4_PLAYER+i,p1_backup[i]);
    }
    projected=0;
    psx_mod_write_byte(0x80166BB0u,saved_palette_dirty);
    if(palette_swap_model)psx_mod_write_word(0x1F800028u,saved_palette_resource);
    for(unsigned i=0;i<sizeof p2_vehicle;++i) {
        p2_vehicle[i]=psx_mod_read_byte(MMX4_VEHICLE+i);
        psx_mod_write_byte(MMX4_VEHICLE+i,p1_vehicle_backup[i]);
    }
}
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    uint32_t old4=cpu->gpr[4],old5=cpu->gpr[5],old2=cpu->gpr[2],result=0;
    cpu->gpr[4]=a0;cpu->gpr[5]=a1;
    for(unsigned i=0;i<filter_count;++i)if(filters[i].address==address && filters[i].filter(cpu,address)) {
        result=cpu->gpr[2];goto done;
    }
    if(ice_contact_model && address==0x8002C160u) {
        ++ice_contact_calls[projected];
        result=psx_mod_read_half(MMX4_PLAYER+10)==psx_mod_read_half(a0+10);
        goto done;
    }
    switch(address) {
    case 0x800C36E8:
        ++latch_calls;latch_owner=projected;
        latch_y=psx_mod_read_half(cpu->gpr[6]+14);
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        psx_mod_write_byte(a1+0x10,1);
        psx_mod_write_byte(a0+5,1);break;
    case 0x80027BE4: {
        ++boundary_calls[projected];
        int camera_x=(int16_t)psx_mod_read_half(a0+10);
        int left=(int16_t)psx_mod_read_half(a0+0x1E);
        int right=(int16_t)psx_mod_read_half(a0+0x1C);
        int player_x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10);
        if(camera_x<=left && player_x-8<left)
            psx_mod_write_half(MMX4_PLAYER+10,(uint16_t)(left+8));
        if(camera_x>right && player_x+8>=right+320)
            psx_mod_write_half(MMX4_PLAYER+10,(uint16_t)(right+312));
        break;
    }
    case 0x800350A4:
        psx_mod_write_byte(MMX4_PLAYER+0x46,2);break;
    case 0x80035048:
        psx_mod_write_byte(MMX4_PLAYER+5,1);psx_mod_write_byte(MMX4_PLAYER+6,0);break;
    case 0x80035848:
        ++incoming_calls[projected];
        psx_mod_write_byte(MMX4_PLAYER+5,0);psx_mod_write_byte(MMX4_PLAYER+6,0);
        psx_mod_write_byte(MMX4_PLAYER+0x67,255);psx_mod_write_byte(MMX4_PLAYER+0x17,1);
        psx_mod_write_word(MMX4_PLAYER+0x24,0xFFF80000u);break;
    case 0x800358A4: {
        ++direct_initializer_calls[projected];
        unsigned y=psx_mod_read_byte(MMX4_PLAY+0x0D)?2507u:715u;
        if(psx_mod_read_byte(MMX4_PLAY+0x0C)==3)y=171u;
        y-=psx_mod_read_byte(MMX4_PLAYER+2)!=0;
        psx_mod_write_word(MMX4_PLAYER+12,y<<16);
        mmx4_coop_call(cpu,0x80035EA4u,MMX4_PLAYER,0);
        psx_mod_write_byte(MMX4_PLAYER+5,2);psx_mod_write_byte(MMX4_PLAYER+6,0);
        psx_mod_write_byte(MMX4_PLAY+0x1C,0);
        break;
    }
    case 0x80035EA4:
        psx_mod_write_word(MMX4_PLAYER+0x68,
            psx_mod_read_byte(MMX4_PLAYER+2)?0x800F8BC8u:0x800F8BC4u);break;
    case 0x800B60BC: {
        ++surface_calls[projected];
        if(!psx_mod_read_byte(a0+4)) {
            psx_mod_write_byte(a0+4,1);psx_mod_write_word(a0+0x14,0);break;
        }
        int32_t movement=(int32_t)psx_mod_read_word(MMX4_PLAYER+0x20);
        if(!movement)movement=(int32_t)psx_mod_read_word(a0+0x14)/2;
        psx_mod_write_word(a0+0x14,(uint32_t)movement);
        psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(MMX4_PLAYER+8)+(uint32_t)movement);
        break;
    }
    case 0x800343A4:
        /* Native idle entry must replace an outgoing beam's action0/1. */
        psx_mod_write_byte(MMX4_PLAYER+5,2);psx_mod_write_byte(MMX4_PLAYER+6,0);
        break;
    case 0x8002C614:
        ++terrain_calls[projected];
        psx_mod_write_byte(MMX4_PLAYER+0x70,
            narrow_return_x && (int16_t)psx_mod_read_half(MMX4_PLAYER+10)==narrow_return_x?
            13:terrain_contact);break;
    case 0x8002D7E4:
    case 0x8002D724:
        ++door_map_queries;
        if(address==0x8002D7E4u)psx_mod_write_half(MMX4_PLAYER+0x6C,(uint16_t)(a1&15u));
        result=door_map_kind;break;
    case 0x80031540:
        if(!psx_mod_read_byte(MMX4_PLAYER+6))psx_mod_write_byte(MMX4_PLAYER+6,1);
        else {psx_mod_write_byte(MMX4_PLAYER+4,3);psx_mod_write_byte(MMX4_PLAYER+3,0);}
        break;
    case 0x80031410:
        psx_mod_write_byte(MMX4_PLAYER+4,1);psx_mod_write_byte(MMX4_PLAYER+5,2);
        psx_mod_write_byte(MMX4_PLAYER+6,0);psx_mod_write_byte(MMX4_PLAYER+0x46,0);
        break;
    case 0x8001FF8C:
        cpu->gpr[3]=psx_mod_read_byte(MMX4_PLAYER+4);
        assert(stage_completed_predicate);stage_completed_predicate(cpu,0x80020018u);
        if(psx_mod_read_half(0x80166C0Cu)&0x800u)psx_mod_write_byte(MMX4_PLAY+1,2);
        else if(cpu->gpr[3]==3)psx_mod_write_byte(MMX4_PLAY+1,1);
        else ++world_calls;
        break;
    case 0x80021158:
        ++world_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);break;
    case 0x800CE894:
        ++assessment_calls[projected];
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        /* The actual native graphic reads PLAYER as its target and later
         * signals parent+16. The native before/after probe checks its real
         * motion, camera cull and completion; this model checks ownership. */
        psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8));
        if(assessment_cull) {
            uint32_t saved_ra=cpu->gpr[31];cpu->gpr[31]=0x800CEA8Cu;
            if(mmx4_coop_call(cpu,0x8002B160u,a0,0))psx_mod_write_byte(a0,0);
            else psx_mod_write_byte(psx_mod_read_word(a0+0x50)+0x16,1);
            cpu->gpr[31]=saved_ra;
        }
        break;
    case 0x8002B160:
        /* The real registered Split dispatcher owns this address. It must
         * call the lifecycle exception, rather than a duplicate registration. */
        result=mmx4_coop_lifecycle_assessment_bounds(cpu,address)?cpu->gpr[2]:assessment_cull;
        break;
    case 0x80023D68:
        ++world_draws;assert(!projected);
        assert(psx_mod_read_byte(MMX4_PLAY+0x43)==psx_mod_read_byte(MMX4_PLAYER+2));
        break;
    case 0x80021D20:
        ++dialogue_calls;dialogue_edge=psx_mod_read_half(0x80166C0Cu);
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);assert(!projected);break;
    case 0x8002217C:
        ++dialogue_init_calls;assert(!projected);
        dialogue_init_character=psx_mod_read_byte(MMX4_PLAYER+2);
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        psx_mod_write_byte(0x80166BB0u,(uint8_t)(psx_mod_read_byte(0x80166BB0u)|1u));
        break;
    case 0x800C4EC8:
        ++flip_queries[projected];
        if(flip_hits[projected])psx_mod_write_byte(a0+5,1);
        break;
    case 0x800C4F40:
        ++flip_world_calls;
        for(unsigned at=12;at<=28;at+=16)
            psx_mod_write_word(MMX4_PLAYER+at,0x08000000u-psx_mod_read_word(MMX4_PLAYER+at));
        break;
    case 0x8002FD70:
        if(personal_model) {
            psx_mod_write_byte(0x801754A4u,0);psx_mod_write_byte(0x801754A5u,2);
            psx_mod_write_byte(0x801754B4u,psx_mod_read_byte(MMX4_PLAYER+2)?10:0);
            psx_mod_write_byte(0x8013B818u,psx_mod_read_byte(MMX4_PLAYER+0x93));
            mmx4_coop_call(cpu,0x800129A4u,8,0);
        }
        break;
    case 0x80023D90:
        ++personal_draws[projected];break;
    case 0x800129F0:case 0x800129A4:
        if(personal_exit_model)psx_mod_write_byte(0x80141BDCu,1);
        break;
    case 0x8002FCAC:
        if(personal_model) {
            const uint32_t menu=0x801754A0u;
            ++personal_native_calls[projected];
            unsigned main=psx_mod_read_byte(menu+4),sub=psx_mod_read_byte(menu+5);
            uint16_t edge=psx_mod_read_half(0x80166C0Cu);
            if(!main) {
                if(!sub)psx_mod_write_byte(menu+5,1);
                else if(sub==1)mmx4_coop_call(cpu,0x8002FD70u,menu,0);
                else {psx_mod_write_byte(menu+4,1);psx_mod_write_byte(menu+5,psx_mod_read_byte(MMX4_PLAYER+2)?1:0);}
            }else if(main==2) {
                if(personal_exit_model) {
                    if(!sub) {
                        mmx4_coop_call(cpu,0x800129F0u,8,0);psx_mod_write_byte(menu+5,1);
                    }else if(sub==1 && !psx_mod_read_byte(0x80141BDCu)) {
                        psx_mod_write_byte(0x80166BB0u,1);psx_mod_write_byte(menu+5,2);
                    }else if(sub==2) {
                        mmx4_coop_call(cpu,0x80031064u,menu,0);
                        mmx4_coop_call(cpu,0x800129A4u,8,0);psx_mod_write_byte(menu+5,3);
                    }else if(sub==3 && !psx_mod_read_byte(0x80141BDCu)) {
                        psx_mod_write_byte(MMX4_PLAY+1,0);
                        psx_mod_write_byte(menu+4,0);psx_mod_write_byte(menu+5,0);
                    }
                    mmx4_coop_call(cpu,sub<2?0x80023D90u:0x80023D68u,0,0);
                    break;
                }
                /* Commit once per hero, but the world remains shared. */
                mmx4_coop_call(cpu,0x80031064u,menu,0);
                psx_mod_write_byte(MMX4_PLAY+1,0);
                psx_mod_write_byte(menu+4,0);psx_mod_write_byte(menu+5,0);
            }else if(sub==3 || sub==4) {
                uint32_t tank=MMX4_PLAY+(sub==3?0x5C:0x5E);
                unsigned energy=psx_mod_read_byte(tank)&127u;
                unsigned hp=psx_mod_read_byte(MMX4_PLAYER+0x5C)&127u;
                if(energy && hp<psx_mod_read_byte(MMX4_PLAY+0x46)) {
                    psx_mod_write_byte(tank,(uint8_t)(0x80u|energy-1));
                    psx_mod_write_byte(MMX4_PLAYER+0x5C,(uint8_t)(hp+1));
                }else psx_mod_write_byte(menu+5,1);
            }else if(sub==5 && (edge&0x800))psx_mod_write_byte(menu+5,2);
            else if(edge==0x40 && (sub==6 || (sub==2 && psx_mod_read_byte(menu+0x14)==13))) {
                psx_mod_write_byte(menu+4,2);
                if(sub==2) {psx_mod_write_byte(menu+5,0);psx_mod_write_word(menu+0x30,1);}
            }
            else if(edge==0x40 && sub==2 && psx_mod_read_byte(menu+0x14)==14)
                psx_mod_write_byte(menu+5,5);
            else if(edge==0x40 && sub==1)psx_mod_write_byte(menu+5,psx_mod_read_byte(menu+0x14)==12?4:3);
            else if(edge==0x4000)psx_mod_write_byte(menu+0x14,(uint8_t)(psx_mod_read_byte(menu+0x14)+2));
            else if(edge==0x40)psx_mod_write_byte(MMX4_PLAYER+0x93,2);
            if(edge&0x100)psx_mod_write_byte(menu+5,6);
            mmx4_coop_call(cpu,0x80023D90u,0,0);
            break;
        }
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        observed_menu_select=psx_mod_read_half(0x80166C0Cu)&0x100u;
        if(pause_commit) {
            psx_mod_write_byte(MMX4_PLAYER+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAYER+0x5C)+1));
            psx_mod_write_byte(MMX4_PLAY+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAY+0x5C)-1));
        }
        if(pause_exit) {
            /* Native 80031014 requests a palette upload, 80031064 updates
             * the world, and 80031130 draws it after changing PLAY+1. */
            psx_mod_write_byte(0x80166BB0u,
                (uint8_t)(psx_mod_read_byte(0x80166BB0u)|1u));
            mmx4_coop_call(cpu,0x80021158u,0,0);
            psx_mod_write_byte(MMX4_PLAY+1,0);
            mmx4_coop_call(cpu,0x80023D68u,0,0);
        }else {
            mmx4_coop_call(cpu,0x80023D68u,0,0);
        }
        break;
    case 0x80031064:
        if(personal_model && !psx_mod_read_word(0x801754D0u)) {
            psx_mod_write_byte(MMX4_PLAYER+0xBE,1);
            mmx4_coop_call(cpu,0x80021158u,0,0);
        }
        break;
    case 0x800C00BC:
        ++collect_calls[projected?1:0];
        if(psx_mod_read_word(MMX4_PLAYER+8)==psx_mod_read_word(a0+8)) {
            psx_mod_write_byte(a0+4,2);psx_mod_write_byte(a0+0x80,2);
            psx_mod_write_byte(MMX4_PLAY+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAY+0x5C)+1));
            mmx4_coop_call(cpu,0x800C03BCu,1,0);
        }
        break;
    case 0x800BF730:
        if(psx_mod_read_byte(a0+4)==2) {
            psx_mod_write_byte(MMX4_PLAYER+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAYER+0x5C)+1));
            uint8_t n=(uint8_t)(psx_mod_read_byte(a0+0x80)-1);
            psx_mod_write_byte(a0+0x80,n);if(!n) {
                psx_mod_write_byte(a0+4,3);mmx4_coop_call(cpu,0x800C03BCu,0,0);
            }
        }
        break;
    case 0x800C03BC:
        for(unsigned i=0x10;i<0x18;++i)psx_mod_write_byte(MMX4_PLAY+i,(uint8_t)a0);
        break;
    case 0x80035A6C:
        for(unsigned i=0x12;i<=0x1A;++i)psx_mod_write_byte(MMX4_PLAY+i,1);
        psx_mod_write_byte(MMX4_PLAY+0x1C,1);psx_mod_write_byte(MMX4_PLAYER+4,3);
        break;
    case 0x80033BC8:
        if(psx_mod_read_byte(MMX4_PLAYER+0x5C)==128) {
            psx_mod_write_byte(MMX4_PLAYER+0x5C,0);
            psx_mod_write_byte(MMX4_PLAYER+4,2);
            psx_mod_write_byte(MMX4_PLAY+0x1C,1);
        }
        break;
    case 0x8001FA24:
        ++reward_calls;
        psx_mod_write_byte(MMX4_PLAY+0x59,(uint8_t)(psx_mod_read_byte(MMX4_PLAY+0x59)|
            (1u<<(psx_mod_read_byte(MMX4_PLAY+0x26)-1u))));
        psx_mod_write_byte(MMX4_PLAYER+0xB9,(uint8_t)(psx_mod_read_byte(MMX4_PLAYER+0xB9)|
            (1u<<(psx_mod_read_byte(MMX4_PLAY+0x26)-1u))));
        psx_mod_write_word(MMX4_PLAY,9);
        for(unsigned at=0x26;at<0x36;++at)psx_mod_write_byte(MMX4_PLAY+at,0);
        break;
    case 0x8001C07C:
    case 0x8001C3E8: {
        ++save_calls;assert(!projected);
        /* Original native record offsets: character, maxHP, additional
         * upgrade, armor, completed stages, story and heart/tank flags. */
        static const unsigned fields[]={0x43,0x46,0x48,0x47,0x59,0x5F,0x5A,0x5B};
        uint32_t record=address==0x8001C3E8?0x800F1D90u:0x80168000u;
        for(unsigned i=0;i<8;++i)psx_mod_write_byte(record+i,psx_mod_read_byte(MMX4_PLAY+fields[i]));
        break;
    }
    case 0x800C6EDC:
        psx_mod_write_byte(MMX4_PLAY+0x47,2);psx_mod_write_byte(MMX4_PLAYER+0xA7,2);break;
    case 0x800BCE48:
        assert(!mmx4_coop_lifecycle_snapshot_ready());
        ++stage_script_calls;
        if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)>576)psx_mod_write_byte(a0+4,1);
        break;
    case 0x8002166C:
        ++stage_script_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        stage_script_projected=projected;
        if(stage_camera_model) {
            stage_camera_y=psx_mod_read_half(0x801419BEu);
            psx_mod_write_half(0x801419D8u,2048);break;
        }
        if(stage_animation_frame) {
            /* Native kind3 B5DCC caches its destination from SCRATCH+28;
             * B5C60 copies the next palette frame and requests dirty bit1.
             * Cover both an existing destination and initialization after
             * the campaign player has died. */
            if(!stage_animation_destination)
                stage_animation_destination=psx_mod_read_word(0x1F800028u)+STAGE_PALETTE_OFFSET;
            psx_mod_write_half(stage_animation_destination,(uint16_t)stage_animation_frame);
            psx_mod_write_byte(0x80166BB0u,(uint8_t)(psx_mod_read_byte(0x80166BB0u)|1u));
            break;
        }
        if(stage_palette_phase) {
            /* Actual type37 BD5AC/BD5CC writes: two white ticks, then zero
             * RGB additions plus dirty |=5 to rebuild the shared palette. */
            unsigned white=stage_palette_phase==1;
            psx_mod_write_half(0x80175EA0u,(uint16_t)(white?31:0));
            psx_mod_write_half(0x80141BE6u,(uint16_t)(white?0x3E0:0));
            psx_mod_write_half(0x8013E1BEu,(uint16_t)(white?0x7C00:0));
            if(!white)psx_mod_write_byte(0x80166BB0u,
                (uint8_t)(psx_mod_read_byte(0x80166BB0u)|(stage_palette_phase==2?5:1)));
            break;
        }
        if(psx_mod_read_byte(MMX4_PLAY)==3)assert(!projected);
        mmx4_coop_call(cpu,0x80036AE4u,0x15,0x40);
        psx_mod_write_byte(MMX4_PLAY+0x1D,9);break;
    case 0x8002A484: {
        /* Native zero-addition early exit and dirty-bit4 rebuild gate.
         * One representative nonzero color is enough to expose a retained
         * white working palette; resource selection stays observable. */
        unsigned additions=psx_mod_read_half(0x80175EA0u)|psx_mod_read_half(0x80141BE6u)|
                           psx_mod_read_half(0x8013E1BEu);
        if(!additions && !(psx_mod_read_byte(0x80166BB0u)&4u))break;
        assert(!projected);
        uint16_t base=psx_mod_read_half(psx_mod_read_word(0x1F800028u)+2);
        psx_mod_write_half(WORKING_PALETTE_COLOR,(uint16_t)(additions?0x7FFF:base));
        psx_mod_write_byte(0x80166BB0u,2);break;
    }
    case 0x80016004:
        if(psx_mod_read_byte(0x80166BB0u)&3u) {
            ++palette_uploads;palette_uploaded=psx_mod_read_byte(0x80166BB0u)&1u?
                psx_mod_read_half(psx_mod_read_word(0x1F800028u)+2):
                psx_mod_read_half(WORKING_PALETTE_COLOR);
            palette_uploaded_world=psx_mod_read_half(psx_mod_read_word(0x1F800028u)+STAGE_PALETTE_OFFSET);
        }
        psx_mod_write_byte(0x80166BB0u,0);break;
    case 0x800C3880:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        /* Original C3A7C's initial lift gate. P1's completed death has
         * action zero; a living P2 must supply this read without revival. */
        if(psx_mod_read_byte(MMX4_PLAYER+5))psx_mod_write_byte(a0+6,1);
        break;
    case 0x800D8648:
    case 0x800D9218:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        psx_mod_write_word(a0+0x18,psx_mod_read_word(MMX4_PLAYER+8));
        break;
    case 0x800D9C84:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)>=7009) {
            mmx4_coop_call(cpu,0x80036AE4u,0x14,0);
            psx_mod_write_byte(a0,0);
        }
        break;
    case 0x800D2A38:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        /* Native D2420 reads the entrant before issuing its scene command.
         * A later callback must read that same body's completion marker. */
        if(!psx_mod_read_byte(a0+6)) {
            if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)>=2321) {
                psx_mod_write_byte(a0+6,1);
                mmx4_coop_call(cpu,0x80036AE4u,0x14,0x40);
            }
        }else if(psx_mod_read_byte(MMX4_PLAYER+0xC0)==0xFF) {
            mmx4_coop_call(cpu,0x80036B18u,0,0);
            psx_mod_write_byte(a0+4,3);
        }
        break;
    case 0x800C2BE0:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(psx_mod_read_byte(a0+0x72)&8u) {
            psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(a0+8));
            psx_mod_write_word(MMX4_PLAYER+12,psx_mod_read_word(a0+12));
            mmx4_coop_call(cpu,0x80036AE4u,0x14,0x40);
            psx_mod_write_byte(a0+5,1);
        }
        if(!mmx4_coop_lifecycle_defer_portal_solid(a0))
            mmx4_coop_call(cpu,0x8002E184u,a0,0);
        break;
    case 0x800C7970:
        assert(a1==MMX4_PLAYER);return portal_hits[projected];
    case 0x800C7164:
        if(!psx_mod_read_byte(a0+5)) {
            if(mmx4_coop_call(cpu,0x800C7970u,a0,MMX4_PLAYER)) {
                ++portal_admissions;
                psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(a0+8));
                mmx4_coop_call(cpu,0x80036AE4u,0x14,0x40);
                psx_mod_write_byte(a0+5,1);
            }
        }else if(!psx_mod_read_byte(MMX4_PLAYER+0xC0)) {
            ++portal_continuations[projected];psx_mod_write_byte(a0+5,2);
        }
        if(!mmx4_coop_lifecycle_defer_portal_solid(a0))
            mmx4_coop_call(cpu,0x8002E184u,a0,0);
        break;
    case 0x8002E184:
        if(projected) {fprintf(stderr,"portal solid ran inside P2 scene\n");exit(1);}
        ++canonical_portal_solids;
        break;
    case 0x800BD654:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(psx_mod_read_word(MMX4_PLAYER+8)>=0x1000000u) {
            mmx4_coop_call(cpu,0x80036AE4u,0x15,0x40);
            psx_mod_write_byte(MMX4_PLAY+0x1D,8);
        }
        break;
    case 0x800C165C:
        mmx4_coop_call(cpu,0x80036AE4u,0x15,0x40);
        psx_mod_write_byte(a0+4,4);break;
    case 0x800C169C:
        /* Original Jungle variant0 finishes after X6336. */
        if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)>6336) {
            psx_mod_write_byte(MMX4_PLAY+0x0F,0x40);
            psx_mod_write_byte(a0,0);
        }
        break;
    case 0x800D0374:
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(!psx_mod_read_byte(a0+5)) {
            /* D0548 starts the outgoing walk; D05B0 later reads the same
             * singleton's completion byte. No synthetic completion here. */
            mmx4_coop_call(cpu,0x80036AE4u,0x15,0);
            psx_mod_write_byte(a0+5,1);
        }else if(psx_mod_read_byte(MMX4_PLAYER+0xC0)==255)
            psx_mod_write_byte(a0+5,2);
        break;
    case 0x800D1248:
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8)+352u*65536u);
        psx_mod_write_byte(a0+4,1);break;
    case 0x800D1284: {
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        uint16_t target=(uint16_t)(psx_mod_read_half(MMX4_PLAYER+10)-48);
        /* Original D12C0 initializes this boundary; D14F8 waits for it. */
        if(!psx_mod_read_byte(a0+4)) {
            psx_mod_write_half(0x801419D4u,target);
            psx_mod_write_byte(a0+4,1);
        }else if(psx_mod_read_half(0x801419BAu)==target)
            psx_mod_write_byte(a0+5,1);
        break;
    }
    case 0x80085F8C:
    case 0x80088D30:
        psx_mod_write_byte(a0+5,(uint8_t)(psx_mod_read_byte(a0+5)+1));
        cpu->gpr[31]=0x80085FE4u;
        mmx4_coop_call(cpu,0x80036AE4u,0x15,0);
        break;
    case 0x80052218:
        /* The Frost admission retires non-door General actors and enters
         * its introduction substate before starting the shared action. */
        for(unsigned i=0;i<32;++i)if(psx_mod_read_byte(0x80165A31u+i*0x8Cu)!=8)
            psx_mod_write_byte(0x80165A30u+i*0x8Cu,0);
        psx_mod_write_byte(a0+6,1);cpu->gpr[31]=0x8005228Cu;
        mmx4_coop_call(cpu,0x80036AE4u,0x15,0);
        break;
    case 0x80036AE4:
        psx_mod_write_byte(MMX4_PLAYER+0xC0,1);psx_mod_write_byte(MMX4_PLAY+0x1C,1);break;
    case 0x80036B18:
        psx_mod_write_byte(MMX4_PLAYER+0xC0,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);break;
    case 0x800311EC:
        observed_controls=psx_mod_read_half(MMX4_PLAYER+0x7C);
        if(both_door_players) {
            if(psx_mod_read_byte(MMX4_PLAYER+0xC4)==1) {
                psx_mod_write_byte(MMX4_PLAYER+0xC4,255);
                psx_mod_write_byte(MMX4_PLAYER+5,0x13);
            }else if(!psx_mod_read_byte(MMX4_PLAYER+0xC4) &&
                     psx_mod_read_byte(MMX4_PLAYER+5)==0x13)
                psx_mod_write_byte(MMX4_PLAYER+5,2);
            if(!psx_mod_read_byte(MMX4_PLAYER+0xC4) &&
               !psx_mod_read_byte(MMX4_PLAYER+0xC0) &&
               !psx_mod_read_byte(MMX4_PLAY+0x0F) && (observed_controls&0x20u)) {
                unsigned dash=(observed_controls&0x400u)!=0;
                psx_mod_write_byte(MMX4_PLAYER+5,(uint8_t)(dash?0x0Du:
                    (observed_controls&0x8000u)?8u:4u));
                psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(MMX4_PLAYER+8)+
                    (dash?8u:4u)*65536u);
            }
        }
        if(initial_arrival_model && psx_mod_read_byte(MMX4_PLAYER+4)==1 &&
           !psx_mod_read_byte(MMX4_PLAYER+5)) {
            if(initial_arrival_model==2 && psx_mod_read_byte(MMX4_PLAYER+0xBE)) {
                /* Native 80031434..6C postpones all grounding checks until
                 * the authored geometry countdown reaches zero. A normal
                 * collision box in this modeled ceiling kills the body. */
                unsigned wait=psx_mod_read_byte(MMX4_PLAYER+0xBE);
                if(psx_mod_read_word(MMX4_PLAYER+0x68))
                    psx_mod_write_byte(MMX4_PLAYER+0x5C,0);
                psx_mod_write_byte(MMX4_PLAYER+0xBE,(uint8_t)(wait-1));
                if(wait==1)mmx4_coop_call(cpu,0x80035EA4u,MMX4_PLAYER,0);
                psx_mod_write_word(MMX4_PLAYER+12,
                    psx_mod_read_word(MMX4_PLAYER+12)+8u*65536u);
                break;
            }
            /* Model the distinct native touchdown and animation-completion
             * boundaries; leader input cannot advance this private body. */
            ++initial_control_calls[projected];
            psx_mod_write_byte(MMX4_PLAYER+6,1);psx_mod_write_byte(MMX4_PLAYER+0x67,0);
            for(unsigned i=0;i<3;++i)psx_mod_write_byte(0x801419F4u+i*0x54u,1);
            if(initial_control_calls[projected]>=3) {
                psx_mod_write_byte(MMX4_PLAYER+5,2);psx_mod_write_byte(MMX4_PLAYER+6,0);
                psx_mod_write_byte(MMX4_PLAY+0x1C,0);
            }
        }
        break;
    case 0x800C1E7C:
        result=psx_mod_read_word(MMX4_PLAYER+8)==psx_mod_read_word(a0+8);break;
    case 0x800C1D90:
        psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(MMX4_PLAYER+8)+0xA400u);
        if(psx_mod_read_half(0x801419BAu)==psx_mod_read_half(0x801419D6u)) {
            psx_mod_write_byte(MMX4_PLAYER+0xC4,0);psx_mod_write_byte(a0+5,4);
            mmx4_coop_call(cpu,0x80035EA4u,MMX4_PLAYER,0);
        }
        break;
    case 0x800C1994:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(!psx_mod_read_byte(a0+5) &&
           mmx4_coop_call(cpu,0x800C1E7Cu,a0,0)) {
            psx_mod_write_byte(MMX4_PLAYER+0xC4,1);psx_mod_write_byte(a0+5,1);
        }else if(both_door_players && psx_mod_read_byte(a0+5)==1) {
            if((int8_t)psx_mod_read_byte(MMX4_PLAYER+0xC4)<0) {
                psx_mod_write_byte(MMX4_PLAY+0x10,1);psx_mod_write_byte(MMX4_PLAY+0x12,1);
                psx_mod_write_word(MMX4_PLAYER+0x68,0);psx_mod_write_byte(a0+5,2);
            }
        }else if(both_door_players && psx_mod_read_byte(a0+5)==2) {
            if(psx_mod_read_byte(a0+0x45)>=2) {
                psx_mod_write_byte(MMX4_PLAY+0x10,0);psx_mod_write_byte(MMX4_PLAY+0x12,0);
                psx_mod_write_half(0x801419D4u,(uint16_t)(psx_mod_read_half(a0+10)+0x50));
                psx_mod_write_half(0x801419D6u,psx_mod_read_half(a0+10));
                psx_mod_write_byte(a0+5,3);
            }
        }else if(both_door_players && psx_mod_read_byte(a0+5)==3) {
            uint32_t old_a2=cpu->gpr[6];cpu->gpr[6]=MMX4_PLAYER;
            mmx4_coop_call(cpu,0x800C1D90u,a0,MMX4_PLAY);cpu->gpr[6]=old_a2;
        }else if(psx_mod_read_byte(a0+5)==3) {
            psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(MMX4_PLAYER+8)+0x10000u);
            psx_mod_write_byte(MMX4_PLAYER+0xC4,0);psx_mod_write_byte(a0+5,4);
        }
        break;
    default: break;
    }
done:
    cpu->gpr[4]=old4;cpu->gpr[5]=old5;cpu->gpr[2]=old2;return result;
}

int mmx4_coop_armor_mounted(void) {
    return ready && psx_mod_read_byte(0x801418C8u+0xC5) &&
        psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+1)==1 &&
        (psx_mod_read_byte(MMX4_VEHICLE+0x97)&0x40);
}
void mmx4_coop_armor_dismount(CPUState *cpu,unsigned seat) {
    (void)cpu;
    if(seat>1 || local_view)return;
    uint8_t *body=seat==projected?ram+(0x801418C8u&0x1FFFFFu):seat?p2:p1_backup;
    uint8_t *vehicle=seat==projected?ram+(MMX4_VEHICLE&0x1FFFFFu):seat?p2_vehicle:p1_vehicle_backup;
    if(body[0xC5] && vehicle[0] && vehicle[1]==1 && (vehicle[0x97]&0x40)) {
        body[0xC5]=0;vehicle[0x97]&=(uint8_t)~0x40u;
    }
}
#include "../src/mods/mmx4_coop_lifecycle.c"
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(8*i));
}
static void reset(void) {
    stage_camera_model=stage_camera_commits=stage_camera_y=0;
    stage_palette_phase=palette_swap_model=palette_uploads=0;palette_uploaded=0;
    stage_animation_destination=stage_animation_frame=0;palette_uploaded_world=0;
    both_door_players=0;
    both_door_crossings=0;
    door_map_queries=0;door_map_kind=0x38;
    CPUState cpu={0};memset(ram,0,sizeof ram);memset(p2,0,sizeof p2);
    memset(p2_vehicle,0,sizeof p2_vehicle);
    fresh_game(&cpu,0);separate_etanks=simultaneous_menus=0;projected=local_view=0;inputs[0]=inputs[1]=0;
    world_calls=observed_campaign=pause_commit=pause_exit=world_draws=0;
    personal_model=0;memset(personal_native_calls,0,sizeof personal_native_calls);
    personal_exit_model=late_menu_assets=0;memset(menu_asset_calls,0,sizeof menu_asset_calls);
    memset(personal_draws,0,sizeof personal_draws);
    memset(assessment_calls,0,sizeof assessment_calls);
    assessment_cull=0;
    stage_script_calls=scene_calls=observed_controls=p2_solid_bits=canonical_portal_solids=0;
    memset(boundary_calls,0,sizeof boundary_calls);
    dialogue_calls=dialogue_edge=dialogue_init_calls=dialogue_init_character=stage_script_projected=0;
    initial_arrival_model=0;
    memset(initial_control_calls,0,sizeof initial_control_calls);
    memset(incoming_calls,0,sizeof incoming_calls);
    memset(terrain_calls,0,sizeof terrain_calls);
    memset(direct_initializer_calls,0,sizeof direct_initializer_calls);
    memset(surface_calls,0,sizeof surface_calls);
    natural_arrivals=direct_arrivals=0;terrain_contact=8;narrow_return_x=0;
    collect_calls[0]=collect_calls[1]=0;ready=1;split_views=0;
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+3,1);
    psx_mod_write_byte(MMX4_PLAYER+4,1);psx_mod_write_byte(MMX4_PLAYER+0x5C,16);
    psx_mod_write_byte(MMX4_PLAYER+5,2);
    p2[0]=p2[3]=p2[4]=1;p2[5]=2;p2[2]=1;p2[0x5C]=12;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+0x46,32);
    psx_mod_write_byte(MMX4_PLAY+0x44,2);
    mmx4_coop_enter_second();mmx4_coop_leave_second();
}
static int personal_floor_momentum(CPUState *cpu) {
    reset();const uint32_t actor=0x80142F98u;
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,5);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+0x14,100u<<16);psx_mod_write_word(PLAYER+8,1000u<<16);
    uint8_t canonical[0x30];read_bytes(actor,canonical,sizeof canonical);
    mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,2000u<<16);
    psx_mod_write_word(PLAYER+0x20,1u<<16);
    mmx4_coop_lifecycle_surface(cpu);mmx4_coop_lifecycle_surface(cpu);
    CHECK(psx_mod_read_word(PLAYER+8)==2001u<<16 && surface_calls[1]==2);
    uint8_t after[0x30];read_bytes(actor,after,sizeof after);CHECK(!memcmp(canonical,after,sizeof after));
    psx_mod_write_word(PLAYER+0x20,0);mmx4_coop_lifecycle_surface(cpu);
    CHECK(psx_mod_read_word(PLAYER+8)==(2001u<<16)+32768u);
    unsigned calls=surface_calls[1];
    /* The surviving P2's shared script pass cannot apply P1's floor again. */
    mmx4_coop_call(cpu,0x800B60BCu,actor,0);CHECK(surface_calls[1]==calls);
    local_view=1;mmx4_coop_lifecycle_surface(cpu);local_view=0;CHECK(surface_calls[1]==calls);
    psx_mod_write_byte(PLAY+0x15,1);mmx4_coop_lifecycle_surface(cpu);
    CHECK(surface_calls[1]==calls);psx_mod_write_byte(PLAY+0x15,0);
    mmx4_coop_leave_second();CHECK(psx_mod_read_word(PLAYER+8)==1000u<<16);
    Mmx4SnapshotIO count={NULL,NULL,0,0,MMX4_SNAPSHOT_COUNT,1};CHECK(mmx4_coop_lifecycle_snapshot(&count));
    uint8_t *saved=malloc(count.pos);CHECK(saved);uint32_t bytes=count.pos;
    Mmx4SnapshotIO io={saved,NULL,bytes,0,MMX4_SNAPSHOT_WRITE,1};CHECK(mmx4_coop_lifecycle_snapshot(&io));
    uint32_t digest=mmx4_coop_lifecycle_digest(2166136261u);memset(surface_second,0,sizeof surface_second);
    io=(Mmx4SnapshotIO){NULL,saved,bytes,0,MMX4_SNAPSHOT_READ,1};CHECK(mmx4_coop_lifecycle_snapshot(&io));
    CHECK(mmx4_coop_lifecycle_digest(2166136261u)==digest);free(saved);
    mmx4_coop_lifecycle_reset();CHECK(!surface_valid[0]);return 0;
}
static int colonel_palette_refreshes(CPUState *cpu) {
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned survivor=0;survivor<3;++survivor) {
        reset();split_views=1;palette_swap_model=1;
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        if(survivor==1){psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
        if(survivor==2){p2[4]=3;p2[0x5C]=0;}
        psx_mod_write_word(0x1F800028u,PALETTE_SOURCE);
        uint16_t base=(uint16_t)(0x1234u+campaign*0x400u);
        psx_mod_write_half(PALETTE_SOURCE+2,base);
        psx_mod_write_half(SECOND_PALETTE_SOURCE+2,0x2E7);
        stage_palette_phase=1;mmx4_coop_call(cpu,0x8002166Cu,0,0);
        CHECK(stage_script_calls==1 && stage_script_projected==(survivor==1));
        CHECK(!projected && psx_mod_read_word(0x1F800028u)==PALETTE_SOURCE);
        mmx4_coop_call(cpu,0x8002A484u,0,0);mmx4_coop_call(cpu,0x80016004u,0,0);
        CHECK(palette_uploads==1 && palette_uploaded==0x7FFF);
        stage_palette_phase=2;mmx4_coop_call(cpu,0x8002166Cu,0,0);
        CHECK(stage_script_calls==2 && (psx_mod_read_byte(0x80166BB0u)&4u));
        CHECK(!psx_mod_read_half(0x80175EA0u) && !psx_mod_read_half(0x80141BE6u) &&
              !psx_mod_read_half(0x8013E1BEu));
        CHECK(!projected && psx_mod_read_word(0x1F800028u)==PALETTE_SOURCE);
        mmx4_coop_call(cpu,0x8002A484u,0,0);mmx4_coop_call(cpu,0x80016004u,0,0);
        CHECK(palette_uploads==2 && palette_uploaded==base);
        CHECK(psx_mod_read_half(WORKING_PALETTE_COLOR)==base && !psx_mod_read_byte(0x80166BB0u));
        CHECK(psx_mod_read_byte(PLAY+0x43)==campaign);
        CHECK((survivor==1)==!mmx4_coop_alive(0) && (survivor==2)==!mmx4_coop_alive(1));
    }
    /* Shared stage palette uploads retain bit1 as well as flash rebuild4.
     * Ordinary private character projections keep their prior ownership. */
    reset();palette_swap_model=1;psx_mod_write_word(0x1F800028u,PALETTE_SOURCE);
    psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    stage_palette_phase=3;psx_mod_write_byte(0x80166BB0u,2);
    mmx4_coop_call(cpu,0x8002166Cu,0,0);CHECK(psx_mod_read_byte(0x80166BB0u)==3);
    CHECK(psx_mod_read_word(0x1F800028u)==PALETTE_SOURCE && !projected);
    psx_mod_write_byte(0x80166BB0u,2);
    mmx4_coop_enter_second();psx_mod_write_byte(0x80166BB0u,7);mmx4_coop_leave_second();
    CHECK(psx_mod_read_byte(0x80166BB0u)==2 && psx_mod_read_word(0x1F800028u)==PALETTE_SOURCE);
    return 0;
}
static int survivor_palette_animation(CPUState *cpu) {
    for(unsigned campaign=0;campaign<2;++campaign)
    for(unsigned split=0;split<2;++split)
    for(unsigned late=0;late<2;++late) {
        reset();split_views=(int)split;palette_swap_model=1;
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_word(0x1F800028u,PALETTE_SOURCE);
        psx_mod_write_half(SECOND_PALETTE_SOURCE+2,0x2E7);
        psx_mod_write_half(SECOND_PALETTE_SOURCE+STAGE_PALETTE_OFFSET,0x4567);
        if(!late)stage_animation_destination=PALETTE_SOURCE+STAGE_PALETTE_OFFSET;
        psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
        for(unsigned frame=1;frame<=4;++frame) {
            stage_animation_frame=0x1200u+frame;
            mmx4_coop_call(cpu,0x8002166Cu,0,0);
            CHECK(stage_script_calls==frame && stage_script_projected && !projected);
            CHECK(stage_animation_destination==PALETTE_SOURCE+STAGE_PALETTE_OFFSET);
            CHECK(psx_mod_read_half(PALETTE_SOURCE+STAGE_PALETTE_OFFSET)==stage_animation_frame);
            CHECK(psx_mod_read_byte(0x80166BB0u)&1u);
            CHECK(psx_mod_read_word(0x1F800028u)==PALETTE_SOURCE);
            CHECK(psx_mod_read_half(SECOND_PALETTE_SOURCE+2)==0x2E7);
            CHECK(psx_mod_read_half(SECOND_PALETTE_SOURCE+STAGE_PALETTE_OFFSET)==0x4567);
            mmx4_coop_call(cpu,0x8002A484u,0,0);
            mmx4_coop_call(cpu,0x80016004u,0,0);
            CHECK(palette_uploads==frame && palette_uploaded_world==stage_animation_frame);
            CHECK(!psx_mod_read_byte(0x80166BB0u));
            CHECK(psx_mod_read_byte(PLAY+0x43)==campaign && mmx4_coop_alive(1));
        }
    }
    return 0;
}
static int open_both_door(CPUState *cpu,uint32_t actor) {
    reset();both_door_players=1;
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,8);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+8,512u<<16);psx_mod_write_word(actor+0x68,0x800FCC58u);
    psx_mod_write_byte(0x800FCC5Au,8);
    psx_mod_write_byte(0x800FCC5Bu,32);
    for(unsigned character=0;character<2;++character) {
        psx_mod_write_byte(0x800F8BC4u+character*4u+2u,8);
        psx_mod_write_byte(0x800F8BC4u+character*4u+3u,16);
    }
    psx_mod_write_half(0x801419D6u,128);psx_mod_write_half(0x801419BAu,128);
    psx_mod_write_half(0x801419D4u,608);
    psx_mod_write_word(PLAYER+8,512u<<16);psx_mod_write_byte(PLAYER+0x89,8);
    mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
    mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,512u<<16);
    psx_mod_write_byte(PLAYER+0x89,8);mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
    mmx4_coop_leave_second();
    mmx4_coop_call(cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==1 && !warp_pending && !warp_phase);
    uint32_t partner_box=body_word(p2,0x68);
    CHECK(p2[0xC4]==255);
    mmx4_coop_enter_second();mmx4_coop_call(cpu,0x800311ECu,PLAYER,0);
    mmx4_coop_leave_second();
    CHECK(p2[5]==2 && body_word(p2,0x68)==partner_box && p2[0xC4]==255);
    mmx4_coop_call(cpu,0x800311ECu,PLAYER,0);
    mmx4_coop_call(cpu,0x800C1994u,actor,0);
    CHECK(psx_mod_read_byte(actor+5)==2 && !psx_mod_read_word(PLAYER+0x68));
    psx_mod_write_byte(actor+0x45,2);mmx4_coop_call(cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==2 && psx_mod_read_byte(actor+5)==3);
    CHECK(psx_mod_read_half(0x801419D6u)==128 && both_door.target_min==512);
    CHECK(psx_mod_read_word(PLAYER+0x68) && psx_mod_read_byte(PLAYER+0xC4)==255);
    CHECK(psx_mod_read_word(PLAYER+8)==512u<<16 && body_word(p2,8)==512u<<16);
    return 0;
}
static void door_control(CPUState *cpu,unsigned seat,uint16_t input) {
    if(seat)mmx4_coop_enter_second();
    psx_mod_write_half(PLAYER+0x7C,input);
    psx_mod_write_half(PLAYER+0x7E,input);psx_mod_write_half(PLAYER+0x80,input);
    mmx4_coop_call(cpu,0x800311ECu,PLAYER,0);
    if(seat)mmx4_coop_leave_second();
}
static void prepare_initial_arrival(void) {
    reset();
    psx_mod_write_byte(PLAY+0x1C,1);psx_mod_write_byte(PLAY+0x1E,1);
    psx_mod_write_byte(PLAY+0x1F,1);psx_mod_write_byte(PLAYER+5,0);
    psx_mod_write_byte(PLAYER+0x67,255);
    psx_mod_write_word(PLAYER+8,72u<<16);psx_mod_write_word(PLAYER+12,202u<<16);
    p2[10]=96;p2[14]=202;
}
static void prepare_direct_spawn(CPUState *cpu,unsigned campaign) {
    reset();
    psx_mod_write_byte(PLAY+0x0C,6);psx_mod_write_byte(PLAY+0x1D,2);
    psx_mod_write_byte(PLAY+0x1C,1);psx_mod_write_byte(PLAY+0x1E,255);
    psx_mod_write_byte(PLAY+0x1F,1);psx_mod_write_byte(PLAYER+5,0);
    psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);
    psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
    cpu->gpr[31]=0x80035834u;cpu->gpr[4]=PLAYER;
    direct_spawn(cpu,0x800358A4u);
    /* Original358A4 at Cyber6/0 checkpoint2 replaces authoredY512 with
     * X715/Zero714, installs the character box and releases PLAY+1C. P2's
     * ordinary enrollment starts with that same Y; X/P2 can remain one pixel
     * above its floor until its own original direct initializer runs. */
    psx_mod_write_word(PLAYER+8,72u<<16);
    psx_mod_write_word(PLAYER+12,(715u-campaign)<<16);
    psx_mod_write_byte(PLAYER+5,2);psx_mod_write_byte(PLAY+0x1C,0);
    psx_mod_write_byte(PLAYER+0xBE,3);
    mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
    mmx4_coop_enter_second();
    psx_mod_write_word(PLAYER+8,96u<<16);
    psx_mod_write_word(PLAYER+12,(715u-campaign)<<16);
    psx_mod_write_byte(PLAYER+0xBE,3);
    mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
    terrain_contact=(uint8_t)(campaign?0:8);
    mmx4_coop_call(cpu,0x8002C614u,PLAYER,0);terrain_contact=8;
    mmx4_coop_leave_second();
    for(unsigned layer=0;layer<3;++layer)psx_mod_write_byte(0x801419F4u+layer*0x54u,1);
    psx_mod_write_half(0x801419B0u+0x1C,1888);
}
static int personal_tank_inventory(void) {
    reset();private_etanks_option=1;mmx4_coop_lifecycle_activate();
    CHECK(separate_etanks);inventory_initialized=0;
    psx_mod_write_half(PLAY+0x5A,0x1000);
    psx_mod_write_byte(PLAY+0x5C,0x90);
    mmx4_coop_enter_second();CHECK(psx_mod_read_byte(PLAY+0x5C)==0x90);
    psx_mod_write_byte(PLAY+0x5C,0x88);
    /* A tank acquired by the guest is available to the campaign owner too;
     * hearts, maxHP, lives, W-energy and story progress never become private. */
    psx_mod_write_half(PLAY+0x5A,0x3001);
    psx_mod_write_byte(PLAY+0x5D,0x84);psx_mod_write_byte(PLAY+0x5E,7);
    mmx4_coop_leave_second();
    CHECK(psx_mod_read_byte(PLAY+0x5C)==0x90 && psx_mod_read_byte(PLAY+0x5D)==0x80);
    CHECK(psx_mod_read_half(PLAY+0x5A)==0x3001 && psx_mod_read_byte(PLAY+0x5E)==7);
    psx_mod_write_byte(PLAY+0x5C,0x8E);
    mmx4_coop_enter_second();CHECK(psx_mod_read_byte(PLAY+0x5C)==0x88);
    CHECK(psx_mod_read_byte(PLAY+0x5D)==0x84);mmx4_coop_leave_second();
    CPUState cpu={0};inputs[1]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    inputs[1]=0;pause_commit=1;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);pause_commit=0;
    CHECK(p2[0x5C]==13 && psx_mod_read_byte(PLAYER+0x5C)==16);
    CHECK(second_inventory.etanks[0]==0x87 && psx_mod_read_byte(PLAY+0x5C)==0x8E);
    /* Guest energy is authoritative in rollback and render transactions. */
    Mmx4SnapshotIO count={NULL,NULL,0,0,MMX4_SNAPSHOT_COUNT,1};
    CHECK(mmx4_coop_lifecycle_snapshot(&count));uint32_t bytes=count.pos;
    uint8_t *saved=malloc(bytes);CHECK(saved);
    Mmx4SnapshotIO io={saved,NULL,bytes,0,MMX4_SNAPSHOT_WRITE,1};CHECK(mmx4_coop_lifecycle_snapshot(&io));
    uint32_t digest=mmx4_coop_lifecycle_digest(1);
    second_inventory.etanks[0]=0x81;CHECK(digest!=mmx4_coop_lifecycle_digest(1));
    io=(Mmx4SnapshotIO){NULL,saved,bytes,0,MMX4_SNAPSHOT_READ,1};
    CHECK(mmx4_coop_lifecycle_snapshot(&io) && digest==mmx4_coop_lifecycle_digest(1));
    mmx4_coop_lifecycle_view_save();second_inventory.etanks[0]=0;
    mmx4_coop_lifecycle_view_restore();CHECK(digest==mmx4_coop_lifecycle_digest(1));
    separate_etanks=0;io=(Mmx4SnapshotIO){NULL,saved,bytes,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_lifecycle_snapshot(&io));free(saved);
    private_etanks_option=0;mmx4_coop_lifecycle_activate();CHECK(!separate_etanks);
    reset();return 0;
}

static int simultaneous_menu_exit(void) {
    CPUState cpu={0};
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode) {
        reset();simultaneous_menus=personal_active=personal_initialized=menu_active=1;
        personal_eligible=3;personal_model=personal_exit_model=1;split_views=mode;
        memset(personal_menu,0,sizeof personal_menu);
        for(unsigned seat=0;seat<2;++seat)personal_menu[seat].native[4]=1;
        psx_mod_write_byte(PLAY+1,2);psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        inputs[1]=START;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(mmx4_coop_lifecycle_personal_menus() && !mmx4_coop_lifecycle_resuming_world());
        CHECK(personal_menu[0].native[5]==1 && personal_menu[1].native[5]==1);
        psx_mod_write_byte(0x80169D7Cu,64);psx_mod_write_byte(0x80169D8Cu,128);
        CHECK(mmx4_coop_lifecycle_menu_fade(0x80166C10u)==64);
        CHECK(mmx4_coop_lifecycle_menu_fade(0x80166CB0u)==128);
        inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(!world_calls && !late_menu_assets);
        psx_mod_write_byte(0x80141BDCu,0);mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_menu[0].native[5]==2 && personal_menu[1].native[5]==2);
        CHECK(mmx4_coop_lifecycle_personal_menus() && !mmx4_coop_lifecycle_resuming_world());
        CHECK(mmx4_coop_lifecycle_menu_fade(0x80166C10u)==255);
        unsigned assets=menu_asset_calls[0]+menu_asset_calls[1];
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(!mmx4_coop_lifecycle_personal_menus() && mmx4_coop_lifecycle_resuming_world());
        CHECK(personal_active && psx_mod_read_byte(PLAY+1)==2 && world_calls==1);
        CHECK(!mmx4_coop_lifecycle_draw_menu(&cpu,0) && !mmx4_coop_lifecycle_draw_menu(&cpu,1));
        for(unsigned i=0;i<3;++i)mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(world_calls==1 && !late_menu_assets && assets==menu_asset_calls[0]+menu_asset_calls[1]);
        psx_mod_write_byte(0x80141BDCu,0);mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(!personal_active && !mmx4_coop_lifecycle_resuming_world() && !psx_mod_read_byte(PLAY+1));
        CHECK(world_calls==1 && !projected && !late_menu_assets);
        CHECK(!mmx4_coop_lifecycle_menu_fade(0x80166CB0u));
    }
    reset();return 0;
}

static int simultaneous_menu_ownership(void) {
    CPUState cpu={0};
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
      for(unsigned opener=0;opener<2;++opener) {
        reset();simultaneous_menus_option=1;private_etanks_option=0;
        mmx4_coop_lifecycle_activate();personal_model=1;split_views=(int)mode;
        CHECK(simultaneous_menus && separate_etanks);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_half(PLAY+0x5A,0x1000);psx_mod_write_byte(PLAY+0x5C,0x83);
        inputs[opener]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
        CHECK(menu_active && personal_active && personal_eligible==3);
        inputs[0]=inputs[1]=0;
        for(unsigned i=0;i<4;++i)mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_initialized && personal_menu[0].native[4]==1 && personal_menu[1].native[4]==1);
        CHECK(!world_calls && !personal_draws[0] && !personal_draws[1]);
        /* Both cursors advance independently in a single simulation frame. */
        inputs[0]=inputs[1]=0x4000;
        unsigned initial[2]={personal_menu[0].native[0x14],personal_menu[1].native[0x14]};
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_menu[0].native[0x14]==initial[0]+2 && personal_menu[1].native[0x14]==initial[1]+2);
        inputs[0]=inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        unsigned x=campaign,zero=x^1u;
        personal_menu[x].native[5]=0;personal_menu[x].native[0x14]=2;
        inputs[x]=0x40;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK((x?p2[0x93]:psx_mod_read_byte(PLAYER+0x93))==2);
        CHECK((zero?p2[0x93]:psx_mod_read_byte(PLAYER+0x93))==0);
        inputs[x]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        /* Healing wins over the other player's same-frame Start. */
        personal_menu[zero].native[5]=1;personal_menu[zero].native[0x14]=10;
        inputs[x]=START;inputs[zero]=0x40;
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_busy(zero) && personal_active && !world_calls);
        unsigned selected=personal_menu[x].native[0x14];
        /* The partner can navigate and confirm equipment while Start stays
         * held. Healing continues and cannot queue a later world resume. */
        inputs[x]=START|0x4000;inputs[zero]=START;
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_busy(zero) && personal_menu[x].native[0x14]==selected+2);
        if(x)p2[0x93]=0;else psx_mod_write_byte(PLAYER+0x93,0);
        inputs[x]=START|0x40;
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_busy(zero) && (x?p2[0x93]:psx_mod_read_byte(PLAYER+0x93))==2);
        inputs[0]=inputs[1]=START;
        for(unsigned i=0;i<5;++i) {
            mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
            CHECK(personal_active && personal_menu[x].native[0x14]==selected+2 && !world_calls);
        }
        CHECK(!personal_busy(zero));
        CHECK((zero?p2[0x5C]:psx_mod_read_byte(PLAYER+0x5C))==(zero?15:19));
        CHECK((zero?second_inventory.etanks[0]:psx_mod_read_byte(PLAY+0x5C))==0x80);
        CHECK((x?second_inventory.etanks[0]:psx_mod_read_byte(PLAY+0x5C))==0x83);
        /* Paused snapshots restore both cursors and the independent inventories. */
        Mmx4SnapshotIO count={NULL,NULL,0,0,MMX4_SNAPSHOT_COUNT,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&count));uint32_t bytes=count.pos;
        uint8_t *saved=malloc(bytes);CHECK(saved);
        Mmx4SnapshotIO io={saved,NULL,bytes,0,MMX4_SNAPSHOT_WRITE,1};CHECK(mmx4_coop_lifecycle_snapshot(&io));
        uint32_t digest=mmx4_coop_lifecycle_digest(1);
        personal_menu[1].native[0x14]^=1;CHECK(digest!=mmx4_coop_lifecycle_digest(1));
        io=(Mmx4SnapshotIO){NULL,saved,bytes,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&io) && digest==mmx4_coop_lifecycle_digest(1));free(saved);
        /* A held Start through healing does not resume. A fresh edge does. */
        CHECK(personal_active);inputs[0]=inputs[1]=0;
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        inputs[opener^1u]=START;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(!personal_active && !menu_active && !projected && world_calls==1);
        CHECK(psx_mod_read_byte(PLAY+0x43)==campaign && mmx4_coop_lifecycle_can_tick());
      }
    /* Simultaneous tank requests serialize, including the one shared W pool. */
    for(unsigned w=0;w<2;++w) {
        reset();simultaneous_menus=separate_etanks=personal_active=personal_initialized=menu_active=1;
        personal_eligible=3;personal_model=1;psx_mod_write_byte(PLAY+1,2);
        psx_mod_write_byte(PLAY+(w?0x5E:0x5C),0x90);second_inventory.etanks[0]=0x90;
        for(unsigned seat=0;seat<2;++seat) {
            personal_menu[seat].native[4]=1;personal_menu[seat].native[5]=1;
            personal_menu[seat].native[0x14]=(uint8_t)(w?12:10);
            inputs[seat]=START|0x40;
        }
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_busy(0) && !personal_busy(1));
        inputs[0]=inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        inputs[1]=0x40;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_busy(0) && !personal_busy(1) && personal_active);
    }
    /* Options remain usable during a partner heal. Tick the healer under
     * shared frontends, but prevent Escape/Quit from bypassing the exit lock. */
    reset();simultaneous_menus=separate_etanks=personal_active=personal_initialized=menu_active=1;
    personal_eligible=3;personal_model=1;psx_mod_write_byte(PLAY+1,2);
    personal_menu[0].native[4]=personal_menu[1].native[4]=1;
    personal_menu[0].native[5]=2;personal_menu[0].native[0x14]=14;
    personal_menu[1].native[5]=3;second_inventory.etanks[0]=0x90;
    inputs[0]=0x40;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[5]==5 && personal_busy(1));
    unsigned energy=second_inventory.etanks[0]&127u;
    inputs[0]=START;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[5]==2 && personal_active && personal_busy(1));
    CHECK((second_inventory.etanks[0]&127u)==energy-1);
    personal_menu[0].native[0x14]=13;inputs[0]=0x40;
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[4]==1 && personal_active && !world_calls);
    inputs[0]=SELECT;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[5]==6 && personal_busy(1));
    energy=second_inventory.etanks[0]&127u;
    inputs[0]=0x40;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[4]==1 && personal_active && !world_calls);
    CHECK((second_inventory.etanks[0]&127u)==energy-1);
    /* P2 opens the shared Quit frontend; native Escape eligibility is shared. */
    reset();simultaneous_menus=separate_etanks=personal_active=personal_initialized=menu_active=1;
    personal_eligible=3;personal_model=1;psx_mod_write_byte(PLAY+1,2);
    for(unsigned seat=0;seat<2;++seat)personal_menu[seat].native[4]=1;
    inputs[1]=SELECT;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[5]==6 && personal_modal_owner==1);
    CHECK(personal_menu[1].native[5]!=6 && !projected);
    {
        Mmx4SnapshotIO count={NULL,NULL,0,0,MMX4_SNAPSHOT_COUNT,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&count));
        uint8_t *saved=malloc(count.pos);CHECK(saved);
        Mmx4SnapshotIO io={saved,NULL,count.pos,0,MMX4_SNAPSHOT_WRITE,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&io));
        uint32_t digest=mmx4_coop_lifecycle_digest(1);
        personal_modal_owner=0;CHECK(digest!=mmx4_coop_lifecycle_digest(1));
        io=(Mmx4SnapshotIO){NULL,saved,count.pos,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&io) && personal_modal_owner==1);
        CHECK(digest==mmx4_coop_lifecycle_digest(1));free(saved);
    }
    /* P1 cannot confirm P2's popup, including while both menus are visible. */
    inputs[0]=0x40;inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[4]==1 && personal_active);
    inputs[0]=0;inputs[1]=0x40;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_menu[0].native[4]==2 && !projected);
    personal_menu[0].native[4]=1;personal_menu[0].native[5]=0;
    inputs[0]=0;
    inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    personal_menu[1].native[0x14]=13;inputs[1]=0x40;
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);CHECK(!p2[0x93]);
    /* A campaign modal is drawn once and neither guest navigation nor a
     * simultaneous Start can dismiss it or resume the shared world. */
    personal_menu[0].native[5]=6;inputs[0]=START;inputs[1]=START|0x4000;
    unsigned guest_calls=personal_native_calls[1],draws=personal_draws[0];
    unsigned guest_cursor=personal_menu[1].native[0x14];
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(personal_active && personal_native_calls[1]==guest_calls &&
        personal_menu[1].native[0x14]==guest_cursor && personal_draws[0]==draws+1);
    personal_menu[0].native[5]=0;inputs[0]=inputs[1]=0;
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    mmx4_coop_lifecycle_view_save();personal_rendering=1;
    mmx4_coop_lifecycle_view_restore();
    CHECK(!personal_rendering && mmx4_coop_lifecycle_snapshot_ready());
    personal_eligible=2;inputs[0]=START;inputs[1]=0;
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);CHECK(personal_active);
    inputs[0]=0;inputs[1]=START;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(!personal_active && !psx_mod_read_byte(PLAYER+0xBE) && p2[0xBE]==1 && world_calls==1);
    /* P2 Escape uses the canonical transition even with P1 dead. Retail
     * availability is mirrored and the other personal menu cannot exit twice. */
    for(unsigned eligible=0;eligible<2;++eligible)for(unsigned dead=0;dead<2;++dead) {
        reset();simultaneous_menus=separate_etanks=personal_active=personal_initialized=menu_active=1;
        personal_eligible=dead?2:3;personal_model=1;psx_mod_write_byte(PLAY+1,2);
        for(unsigned seat=0;seat<2;++seat) {
            personal_menu[seat].native[4]=1;personal_menu[seat].native[5]=2;
        }
        personal_menu[0].native[0x23]=(uint8_t)eligible;
        personal_menu[1].native[0x14]=13;
        inputs[1]=0x40;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
        CHECK(personal_menu[1].native[0x23]==eligible && !projected);
        CHECK(personal_menu[0].native[4]==(eligible?2:1));
        CHECK(personal_menu[0].native[0x30]==eligible && !world_calls);
        CHECK(!personal_menu[1].native[0x30]);
        CHECK(!psx_mod_read_byte(PLAYER+0xBE) && !p2[0xBE]);
        if(eligible) {
            inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
            CHECK(!psx_mod_read_byte(PLAYER+0xBE) && !p2[0xBE] && !world_calls);
        }
    }
    simultaneous_menus_option=0;private_etanks_option=0;
    reset();return 0;
}

static int marine_chaser_dismount(void) {
    CPUState cpu={0};
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned split=0;split<2;++split) {
        reset();split_views=split;
        psx_mod_write_byte(PLAY+12,5);psx_mod_write_byte(PLAY+13,1);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(MMX4_VEHICLE,1);p2_vehicle[0]=1;
        psx_mod_write_byte(MMX4_VEHICLE+4,2);p2_vehicle[4]=2;
        psx_mod_write_word(PLAYER+12,120u<<16);put32(p2+12,119u<<16);
        p2[0xC5]=255;
        cpu.gpr[31]=0x8003BA84u;
        mmx4_coop_call(&cpu,0x80036AE4u,0x17,0x40);
        CHECK(psx_mod_read_byte(PLAYER+0xC0)==1);
        CHECK(!script_active && !warp_pending && !warp_phase);
        CHECK(mmx4_coop_lifecycle_can_tick() && !mmx4_coop_lifecycle_hidden(1));
        CHECK(p2[0xC5]==255 && body_word(p2,12)==119u<<16);
        /* P2's own vehicle must now eject its rider, rather than being hidden
         * and carried to P1's later on-foot position. */
        p2[0xC5]=0;mmx4_coop_enter_second();
        mmx4_coop_call(&cpu,0x80036AE4u,0x17,0x40);mmx4_coop_leave_second();
        CHECK(p2[0xC0]==1 && !script_active && !warp_pending);
        cpu.gpr[31]=0x8003BAC4u;
        psx_mod_write_byte(MMX4_VEHICLE+4,3);
        mmx4_coop_call(&cpu,0x80036B18u,MMX4_VEHICLE,0);
        CHECK(!psx_mod_read_byte(PLAYER+0xC0) && p2[0xC0]==1);
        p2_vehicle[4]=3;mmx4_coop_enter_second();
        mmx4_coop_call(&cpu,0x80036B18u,MMX4_VEHICLE,0);mmx4_coop_leave_second();
        CHECK(!p2[0xC0] && !script_active && !warp_pending && !warp_phase);
        CHECK(psx_mod_read_byte(PLAY+0x43)==campaign && !projected);
    }
    /* Other scene commands and vehicle contexts keep their shared carry. */
    for(unsigned excluded=0;excluded<6;++excluded) {
        reset();psx_mod_write_byte(PLAY+12,5);psx_mod_write_byte(PLAY+13,1);
        psx_mod_write_byte(MMX4_VEHICLE,1);psx_mod_write_byte(MMX4_VEHICLE+4,2);
        cpu.gpr[31]=0x8003BA84u;
        if(excluded==0)psx_mod_write_byte(PLAY+12,6);
        if(excluded==1)psx_mod_write_byte(PLAY+13,0);
        if(excluded==2)psx_mod_write_byte(MMX4_VEHICLE+1,1);
        if(excluded==3)psx_mod_write_byte(MMX4_VEHICLE,0);
        if(excluded==4)cpu.gpr[31]=0x800C1678u;
        mmx4_coop_call(&cpu,0x80036AE4u,excluded==5?0x15:0x17,0x40);
        CHECK(script_active && warp_pending);
    }
    return 0;
}

int main(void) {
    CHECK(!simultaneous_menu_exit());
    CHECK(!simultaneous_menu_ownership());
    CHECK(!personal_tank_inventory());
    CPUState cpu={0};reset();
    /* Hub return pads include Slash's wall-adjacent spawn. Enrollment may
     * prefer +24 elsewhere; a refight return must wait for the existing
     * standing-box/floor probe before making either counterpart visible. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned checkpoint=11;checkpoint<=18;++checkpoint) {
        reset();psx_mod_write_byte(PLAY+12,12);psx_mod_write_byte(PLAY+0x1D,(uint8_t)checkpoint);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_word(PLAYER+8,640u<<16);psx_mod_write_word(PLAYER+12,180u<<16);
        psx_mod_write_byte(PLAYER+0x89,8);
        mmx4_coop_call(&cpu,0x80035EA4u,PLAYER,0);
        put32(p2+8,664u<<16);put32(p2+12,180u<<16);
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(warp_phase==1 && !p2[0] && !p2[3] && !incoming_calls[1]);
        CHECK(psx_mod_read_word(PLAYER+8)==640u<<16 && psx_mod_read_byte(PLAYER+0x5C)==16);
        terrain_contact=12;
        for(unsigned tick=0;tick<3;++tick)warp_tick(&cpu);
        CHECK(warp_phase==1 && !p2[3] && p2[0x5C]==12 && !incoming_calls[1]);
        terrain_contact=8;
        for(unsigned tick=0;tick<3;++tick)warp_tick(&cpu);
        CHECK(warp_phase==2 && p2[0] && p2[3] && incoming_calls[1]==1);
        CHECK(body_word(p2,8)==640u<<16 && p2[0x70]==8 && p2[0x89]==8 && p2[0x5C]==12);
        CHECK(!projected && psx_mod_read_byte(PLAY+0x43)==campaign);
    }
    /* Bike entry may enroll under READY, but never resurrect a carried death
     * or treat an ordinary checkpoint/story lock as this initial entrance. */
    reset();psx_mod_write_byte(PLAY+0x0C,5);psx_mod_write_byte(PLAY+0x1C,1);
    psx_mod_write_byte(PLAYER+5,18);psx_mod_write_byte(PLAYER+0xC5,255);
    psx_mod_write_byte(MMX4_VEHICLE,1);psx_mod_write_byte(MMX4_VEHICLE+4,1);
    psx_mod_write_byte(MMX4_VEHICLE+6,1);psx_mod_write_word(MMX4_VEHICLE+0xA0,0x80142F98u);
    CHECK(!mmx4_coop_lifecycle_can_enroll());
    psx_mod_write_word(MMX4_VEHICLE+0x20,4u<<16);
    CHECK(mmx4_coop_lifecycle_can_enroll());
    carry_dead=1;CHECK(!mmx4_coop_lifecycle_can_enroll());carry_dead=0;
    carry_first_dead=1;CHECK(!mmx4_coop_lifecycle_can_enroll());carry_first_dead=0;
    departed=1;CHECK(!mmx4_coop_lifecycle_can_enroll());departed=0;
    psx_mod_write_byte(PLAY+0x1D,1);CHECK(!mmx4_coop_lifecycle_can_enroll());
    psx_mod_write_byte(PLAY+0x1D,0);psx_mod_write_byte(MMX4_VEHICLE+5,1);
    CHECK(!mmx4_coop_lifecycle_can_enroll());
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();latch_calls=0;
        uint32_t fan=0x80165A30u;
        psx_mod_write_byte(PLAY+12,3);psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+4,3);
        psx_mod_write_half(PLAYER+14,2507);p2[14]=100;p2[15]=0;
        psx_mod_write_byte(fan,1);psx_mod_write_byte(fan+1,13);psx_mod_write_byte(fan+4,1);
        cpu.gpr[6]=PLAYER;
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);
        CHECK(latch_calls==1 && latch_owner==1 && latch_y==100);
        CHECK(observed_campaign==campaign && psx_mod_read_byte(PLAY+0x43)==campaign);
        CHECK(!projected && !psx_mod_read_byte(PLAYER+0x5C));
        CHECK(psx_mod_read_half(PLAYER+14)==2507 && p2[14]==100);
        CHECK(psx_mod_read_byte(PLAY+0x10)==1 && psx_mod_read_byte(fan+5)==1);
        psx_mod_write_byte(fan+5,0);p2[0x5C]=0;
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);
        CHECK(latch_calls==2 && latch_owner==0 && latch_y==2507);
    }
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();latch_calls=0;const uint32_t fan=0x80165A30u;
        psx_mod_write_byte(PLAY+12,3);psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(fan,1);psx_mod_write_byte(fan+1,13);psx_mod_write_byte(fan+4,1);
        psx_mod_write_half(fan+14,120);psx_mod_write_word(fan+0x68,0x800F0000u);
        psx_mod_write_byte(0x800F0003u,16);psx_mod_write_byte(0x800F0023u,16);
        put32(p2+8,100u<<16);put32(p2+12,200u<<16);cpu.gpr[6]=PLAYER;
        /* A transiently missing P2 box must not close a hatch above them. */
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);
        CHECK(!latch_calls && !psx_mod_read_byte(fan+5) && !psx_mod_read_byte(PLAY+0x10));
        put32(p2+0x68,0x800F0020u);
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);CHECK(!latch_calls);
        put32(p2+12,80u<<16);
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);CHECK(latch_calls==1);
    }
    reset();
    /* The ordinary unlocked enrollment policy is retained. READY has not
     * begun the native beam yet, and genuine scene locks remain ineligible. */
    CHECK(mmx4_coop_lifecycle_can_enroll());
    psx_mod_write_byte(PLAY+0x0C,3);psx_mod_write_byte(PLAY+0x1D,1);
    psx_mod_write_byte(PLAY+0x1E,255);psx_mod_write_byte(PLAYER+0x89,0);
    CHECK(!mmx4_coop_lifecycle_can_enroll());
    psx_mod_write_byte(PLAYER+0x70,8);CHECK(!mmx4_coop_lifecycle_can_enroll());
    psx_mod_write_byte(PLAYER+0x89,8);CHECK(mmx4_coop_lifecycle_can_enroll());
    reset();
    psx_mod_write_byte(PLAY+0x1C,1);CHECK(!mmx4_coop_lifecycle_can_enroll());
    for(unsigned gate=0;gate<22;++gate) {
        prepare_initial_arrival();CHECK(mmx4_coop_lifecycle_can_enroll());
        switch(gate) {
        case 0:psx_mod_write_byte(PLAY,5);break;
        case 1:psx_mod_write_byte(PLAY+1,1);break;
        case 2:psx_mod_write_byte(PLAY+0x1C,2);break;
        case 3:psx_mod_write_byte(PLAY+0x1E,0);break;
        case 4:psx_mod_write_byte(PLAY+0x1F,0);break;
        case 5:psx_mod_write_byte(PLAYER,0);break;
        case 6:psx_mod_write_byte(PLAYER+3,0);break;
        case 7:psx_mod_write_byte(PLAYER+4,2);break;
        case 8:psx_mod_write_byte(PLAYER+5,2);break;
        case 9:psx_mod_write_byte(PLAYER+0x67,1);break;
        case 10:psx_mod_write_byte(PLAYER+0x5C,0);break;
        case 11:psx_mod_write_byte(PLAYER+0xC0,1);break;
        case 12:psx_mod_write_byte(PLAYER+0xC3,1);break;
        case 13:psx_mod_write_byte(PLAYER+0xC4,1);break;
        case 14:psx_mod_write_byte(PLAYER+0xC5,255);break;
        case 15:psx_mod_write_byte(PLAYER+0xD9,1);break;
        case 16:psx_mod_write_byte(PLAY+0x0F,1);break;
        case 17:psx_mod_write_byte(PLAY+0x10,1);break;
        case 18:script_active=1;break;
        case 19:carry_dead=1;break;
        case 20:carry_first_dead=1;break;
        case 21:departed=carry_departure=1;break;
        }
        CHECK(!mmx4_coop_lifecycle_can_enroll());
    }
    /* Both characters begin their own native beam once, at their initialized
     * positions. P1 walking/dashing after touchdown cannot hide or restart
     * P2's remaining animation; P2 can also finish before P1 releases +1C. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned finish_first=0;finish_first<2;++finish_first) {
        prepare_initial_arrival();
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        initial_arrival_model=1;
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(initial_arrival && !warp_phase && !mmx4_coop_lifecycle_hidden(1));
        CHECK(natural_arrivals==1 && !direct_arrivals);
        CHECK(p2[5]==0 && p2[0x67]==255 && p2[0x17]==1 && incoming_calls[1]==1);
        CHECK(body_word(p2,8)==96u<<16 && body_word(p2,12)==202u<<16);
        CHECK(body_word(p2,8)==psx_mod_read_word(PLAYER+8)+(24u<<16));
        CHECK(psx_mod_read_byte(PLAYER+5)==0 && psx_mod_read_byte(PLAYER+0x67)==255);
        for(unsigned tick=0;tick<3;++tick) {
            if(tick && !finish_first) {
                psx_mod_write_byte(PLAYER+5,(uint8_t)(tick==1?4:13));
                psx_mod_write_byte(PLAYER+0x67,0);psx_mod_write_byte(PLAYER+0x89,8);
                psx_mod_write_byte(PLAY+0x1C,0);
            }
            uint8_t first_body[0xE4];
            for(unsigned i=0;i<sizeof first_body;++i)first_body[i]=psx_mod_read_byte(PLAYER+i);
            mmx4_coop_lifecycle_tick(&cpu);
            CHECK(mmx4_coop_lifecycle_can_tick() && !warp_phase && p2[0] && p2[3]);
            mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x800311ECu,PLAYER,0);mmx4_coop_leave_second();
            CHECK(initial_control_calls[1]==tick+1 && incoming_calls[1]==1);
            CHECK(psx_mod_read_byte(PLAY+0x1C)==(finish_first || !tick));
            for(unsigned i=0;i<sizeof first_body;++i)CHECK(first_body[i]==psx_mod_read_byte(PLAYER+i));
            for(unsigned layer=0;layer<3;++layer)CHECK(!psx_mod_read_byte(0x801419F4u+layer*0x54u));
            CHECK(body_word(p2,8)==96u<<16 && body_word(p2,12)==202u<<16);
            CHECK(initial_arrival==(tick<2));
        }
        CHECK(p2[5]==2 && !p2[0x67] && !warp_phase && !projected);
        CHECK(p2[0x5C]==12 && psx_mod_read_byte(PLAYER+0x5C)==16);
    }
    /* Cyber 6/1 begins at Y768 with an authored eight-tick geometry wait;
     * a delayed enrollment retains the remaining wait at its current Y.
     * The counterpart uses its own character box when native movement
     * enables collision, including an enrollment after the wait expired. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned remaining=0;remaining<=8;++remaining) {
        prepare_initial_arrival();
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAYER+0xBE,(uint8_t)remaining);
        psx_mod_write_word(PLAYER+8,328u<<16);
        uint32_t x=352u<<16;
        for(unsigned i=0;i<4;++i)p2[8+i]=(uint8_t)(x>>(8*i));
        uint32_t y=(768u+(8u-remaining)*8u)<<16;
        psx_mod_write_word(PLAYER+12,y);
        for(unsigned i=0;i<4;++i)p2[12+i]=(uint8_t)(y>>(8*i));
        /* Model an already initialized ordinary box, as the old enrollment
         * path installed before entering the native initial beam. */
        p2[0x68]=0xC4;p2[0x69]=0x8B;p2[0x6A]=0x0F;p2[0x6B]=0x80;
        initial_arrival_model=2;mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(p2[0xBE]==remaining && psx_mod_read_byte(PLAYER+0xBE)==remaining);
        CHECK(body_word(p2,8)==x && psx_mod_read_word(PLAYER+8)==328u<<16);
        CHECK(body_word(p2,12)==y && body_word(p2,0x68)==
            (remaining?0u:(campaign?0x800F8BC4u:0x800F8BC8u)));
        for(unsigned tick=0;tick<remaining;++tick) {
            mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x800311ECu,PLAYER,0);mmx4_coop_leave_second();
            CHECK(p2[0xBE]==remaining-tick-1 && p2[0x5C]==12);
            CHECK(p2[5]==0 && p2[6]==0 && p2[0x67]==255 && initial_arrival);
            CHECK(body_word(p2,12)==y+(tick+1)*8u*65536u);
            CHECK(body_word(p2,8)==x && psx_mod_read_word(PLAYER+8)==328u<<16);
            CHECK(body_word(p2,0x68)==(tick+1==remaining?
                (campaign?0x800F8BC4u:0x800F8BC8u):0u));
            CHECK(psx_mod_read_byte(PLAYER+0xBE)==remaining && psx_mod_read_word(PLAYER+12)==y);
        }
        CHECK(!p2[0xBE] && body_word(p2,12)==832u<<16 && incoming_calls[1]==1);
        for(unsigned tick=0;tick<3;++tick) {
            mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x800311ECu,PLAYER,0);mmx4_coop_leave_second();
        }
        CHECK(!initial_arrival && p2[5]==2 && !p2[0x67] && p2[0x5C]==12);
    }
    prepare_initial_arrival();mmx4_coop_lifecycle_enrolled(&cpu);
    psx_mod_write_byte(PLAY+0x1C,0);p2[0x67]=0;inputs[1]=SELECT;
    for(unsigned tick=0;tick<100;++tick)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(initial_arrival && !departed && !warp_phase);
    mmx4_coop_lifecycle_reset();CHECK(!initial_arrival);
    /* The native direct-init caller is the one-shot authority. A private
     * projection cannot overwrite that marker; unrelated/story entries,
     * frontend scopes, and normal policies never authorize another P1 beam. */
    for(unsigned gate=0;gate<12;++gate) {
        prepare_direct_spawn(&cpu,0);CHECK(direct_initial_spawn);
        direct_initial_spawn=0;
        psx_mod_write_byte(PLAY+0x1C,1);psx_mod_write_byte(PLAYER+5,0);
        switch(gate) {
        case 0:cpu.gpr[31]=0x80035838u;break;
        case 1:cpu.gpr[4]=PLAYER+0x100;break;
        case 2:psx_mod_write_byte(PLAY+0x0C,12);break;
        case 3:psx_mod_write_byte(PLAY+0x0C,1);break;
        case 4:psx_mod_write_byte(PLAY+0x1E,0);break;
        case 5:psx_mod_write_byte(PLAY+0x1F,0);break;
        case 6:psx_mod_write_byte(PLAY+0x1C,0);break;
        case 7:psx_mod_write_byte(PLAY,5);break;
        case 8:psx_mod_write_byte(PLAY+1,1);break;
        case 9:mmx4_coop_enter_second();break;
        case 10:local_view=1;break;
        case 11:psx_mod_write_byte(PLAY+0x0C,3);psx_mod_write_byte(PLAY+0x1D,0);break;
        }
        direct_spawn(&cpu,0x800358A4u);CHECK(!direct_initial_spawn);
        if(projected)mmx4_coop_leave_second();
    }
    prepare_direct_spawn(&cpu,0);mmx4_coop_enter_second();
    direct_spawn(&cpu,0x800358A4u);CHECK(direct_initial_spawn);mmx4_coop_leave_second();
    uint32_t marker_digest=mmx4_coop_lifecycle_digest(2166136261u);
    direct_initial_spawn=0;
    CHECK(mmx4_coop_lifecycle_digest(2166136261u)!=marker_digest);
    prepare_direct_spawn(&cpu,0);psx_mod_write_byte(PLAY+0x0C,3);
    psx_mod_write_byte(PLAY+0x1D,1);psx_mod_write_byte(PLAY+0x1C,1);
    direct_spawn(&cpu,0x800358A4u);CHECK(direct_initial_spawn);
    for(unsigned boundary=0;boundary<5;++boundary) {
        prepare_direct_spawn(&cpu,0);CHECK(direct_initial_spawn);
        switch(boundary) {
        case 0:fresh_game(&cpu,0);break;
        case 1:mmx4_coop_lifecycle_reset();break;
        case 2:stage_selection(&cpu,0);break;
        case 3:warp_pending=1;warp_begin(&cpu);break;
        case 4:stage_initialization(&cpu,0);break;
        }
        CHECK(!direct_initial_spawn);
    }
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned finish_first=0;finish_first<2;++finish_first) {
        prepare_direct_spawn(&cpu,campaign);CHECK(direct_pair_candidate());
        uint32_t first_x=psx_mod_read_word(PLAYER+8),first_y=psx_mod_read_word(PLAYER+12);
        uint32_t second_x=body_word(p2,8),second_y=(714u+campaign)<<16;
        uint32_t first_box=psx_mod_read_word(PLAYER+0x68),second_box=body_word(p2,0x68);
        initial_arrival_model=1;mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(initial_arrival && !direct_initial_spawn && !warp_phase && !projected);
        CHECK(incoming_calls[0]==1 && incoming_calls[1]==1 && direct_arrivals==1 && !natural_arrivals);
        CHECK(!direct_initializer_calls[0] && direct_initializer_calls[1]==1);
        CHECK(psx_mod_read_byte(PLAY+0x1C)==1);
        CHECK(!psx_mod_read_byte(PLAYER+0xBE) && !p2[0xBE]);
        CHECK(psx_mod_read_word(PLAYER+0x68)==first_box && body_word(p2,0x68)==second_box);
        CHECK(psx_mod_read_word(PLAYER+8)==first_x && body_word(p2,8)==second_x);
        CHECK(psx_mod_read_word(PLAYER+12)==first_y && body_word(p2,12)==second_y);
        CHECK(second_x==first_x+(24u<<16) && !mmx4_coop_lifecycle_hidden(1));
        CHECK(boundary_calls[1]==1 && terrain_calls[1]==2);
        unsigned shared_released=0;
        for(unsigned pass=0;pass<2;++pass) {
            unsigned seat=pass?finish_first:(finish_first^1u);
            for(unsigned tick=0;tick<3;++tick) {
                if(seat)mmx4_coop_enter_second();
                mmx4_coop_call(&cpu,0x800311ECu,PLAYER,0);
                if(seat)mmx4_coop_leave_second();
                if(!seat && tick==2)shared_released=1;
                CHECK(psx_mod_read_byte(PLAY+0x1C)==!shared_released);
            }
        }
        CHECK(!initial_arrival && !psx_mod_read_byte(PLAY+0x1C));
        CHECK(psx_mod_read_byte(PLAYER+5)==2 && p2[5]==2);
        CHECK(!psx_mod_read_byte(PLAYER+0x67) && !p2[0x67]);
        CHECK(psx_mod_read_byte(PLAYER+0x5C)==16 && p2[0x5C]==12);
        CHECK(psx_mod_read_word(PLAYER+8)==first_x && body_word(p2,8)==second_x);
        CHECK(psx_mod_read_word(PLAYER+12)==first_y && body_word(p2,12)==second_y);
    }
    /* A bounds clamp uses native terrain at the resulting position. Fresh
     * terrain +70, not a stale +89 ground bit, decides whether a pair is safe. */
    for(unsigned floor=0;floor<2;++floor) {
        prepare_direct_spawn(&cpu,0);
        mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,424u<<16);mmx4_coop_leave_second();
        psx_mod_write_half(0x801419B0u+10,101);psx_mod_write_half(0x801419B0u+0x1C,100);
        terrain_contact=(uint8_t)(floor?8:0);
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(body_word(p2,8)==412u<<16 && terrain_calls[1]==2 && boundary_calls[1]==1);
        CHECK(incoming_calls[0]==floor && incoming_calls[1]==1 && direct_arrivals==floor);
        CHECK(!direct_initial_spawn && initial_arrival==floor);
    }
    for(unsigned gate=0;gate<42;++gate) {
        prepare_direct_spawn(&cpu,0);
        switch(gate) {
        case 0:psx_mod_write_byte(PLAYER,0);break;
        case 1:psx_mod_write_byte(PLAYER+3,0);break;
        case 2:psx_mod_write_byte(PLAYER+4,3);break;
        case 3:psx_mod_write_byte(PLAYER+5,4);break;
        case 4:psx_mod_write_byte(PLAYER+0x5C,0);break;
        case 5:psx_mod_write_word(PLAYER+0x68,0);break;
        case 6:psx_mod_write_byte(PLAYER+0x67,255);break;
        case 7:psx_mod_write_byte(PLAYER+0xBC,1);break;
        case 8:psx_mod_write_byte(PLAYER+0xC0,1);break;
        case 9:psx_mod_write_byte(PLAYER+0xC3,1);break;
        case 10:psx_mod_write_byte(PLAYER+0xC4,1);break;
        case 11:psx_mod_write_byte(PLAYER+0xC5,255);break;
        case 12:psx_mod_write_byte(PLAYER+0xD9,1);break;
        case 13:p2[0]=0;break;
        case 14:p2[3]=0;break;
        case 15:p2[4]=3;break;
        case 16:p2[5]=4;break;
        case 17:p2[0x5C]=0;break;
        case 18:memset(p2+0x68,0,4);break;
        case 19:p2[0x67]=255;break;
        case 20:p2[0xBC]=1;break;
        case 21:p2[0xC0]=1;break;
        case 22:p2[0xC3]=1;break;
        case 23:p2[0xC4]=1;break;
        case 24:p2[0xC5]=255;break;
        case 25:p2[0xD9]=1;break;
        case 26:psx_mod_write_byte(PLAY+0x0F,1);break;
        case 27:psx_mod_write_byte(PLAY+0x10,1);break;
        case 28:psx_mod_write_byte(PLAY,5);break;
        case 29:psx_mod_write_byte(PLAY+1,1);break;
        case 30:psx_mod_write_byte(PLAY+0x1E,0);break;
        case 31:psx_mod_write_byte(PLAY+0x1F,0);break;
        case 32:psx_mod_write_byte(PLAY+0x1C,1);break;
        case 33:script_active=1;break;
        case 34:warp_pending=1;break;
        case 35:warp_phase=1;break;
        case 36:departed=1;break;
        case 37:carry_dead=1;break;
        case 38:carry_first_dead=1;break;
        case 39:carry_departure=1;break;
        case 40:terrain_contact=0;p2[0x89]=8;break;
        case 41:terrain_contact=12;break;
        }
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(!direct_initial_spawn && !initial_arrival && !incoming_calls[0] && !direct_arrivals);
        /* An ordinary later return cannot inherit the rejected marker. */
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+5,2);
        psx_mod_write_byte(PLAYER+0x67,0);psx_mod_write_byte(PLAY+0x1C,0);
        CHECK(!direct_pair_candidate());
    }
    reset();
    for(unsigned owner=0;owner<2;++owner) {
        reset();memset(flip_queries,0,sizeof flip_queries);memset(flip_hits,0,sizeof flip_hits);
        flip_world_calls=flip_second_calls=flip_camera_calls=0;
        psx_mod_write_byte(PLAY+0x0C,6);psx_mod_write_byte(PLAY+0x0D,1);
        const uint32_t actor=0x80165A30u;flip_hits[owner]=1;
        mmx4_coop_call(&cpu,0x800C4EC8u,actor,0);
        CHECK(psx_mod_read_byte(actor+5)==1 && flip_queries[0]==1 && flip_queries[1]==owner);
        CHECK(!projected && psx_mod_read_byte(PLAYER+0x5C)==16 && p2[0x5C]==12);
        psx_mod_write_word(PLAYER+12,971u<<16);psx_mod_write_word(PLAYER+28,(970u<<16)+123);
        p2[14]=0xCA;p2[15]=3; /* Y970. */
        uint8_t original[4];memcpy(original,p2+12,4);
        mmx4_coop_call(&cpu,0x800C4F40u,actor,0);
        CHECK(flip_world_calls==1 && flip_second_calls==1 && flip_camera_calls==1);
        CHECK(psx_mod_read_word(PLAYER+12)==1077u<<16 && body_word(p2,12)==1078u<<16);
        mmx4_coop_call(&cpu,0x800C4F40u,actor,0);
        CHECK(psx_mod_read_word(PLAYER+12)==971u<<16 && !memcmp(original,p2+12,4));
        CHECK(flip_world_calls==2 && flip_second_calls==2 && !projected);
    }
    reset();
    for(unsigned seat=0;seat<2;++seat)for(unsigned lock=0;lock<2;++lock)for(unsigned hidden=0;hidden<2;++hidden) {
        reset();psx_mod_write_byte(PLAY+0x1C,(uint8_t)lock);
        if(hidden) {warp_phase=2;warp_owner=seat;}
        if(seat)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER+0x5C,128);
        mmx4_coop_call(&cpu,0x80033BC8u,PLAYER,0);
        CHECK(psx_mod_read_byte(PLAYER+4)==2 && !psx_mod_read_byte(PLAYER+0x5C));
        CHECK(psx_mod_read_byte(PLAY+0x1C)==lock);
        CHECK(living_death_partner());
        if(seat)mmx4_coop_leave_second();
        CHECK(seat?psx_mod_read_byte(PLAYER+0x5C)==16:p2[0x5C]==12);
    }
    reset();p2[0x5C]=0;p2[4]=3;psx_mod_write_byte(PLAYER+0x5C,128);
    mmx4_coop_call(&cpu,0x80033BC8u,PLAYER,0);
    CHECK(psx_mod_read_byte(PLAY+0x1C)==1); /* Native team-wipe lock. */
    reset();
    Inventory original_second=second_inventory,original_saved=saved_inventory;
    uint32_t inventory_digest=mmx4_coop_lifecycle_digest(2166136261u);
    mmx4_coop_lifecycle_view_save();
    mmx4_coop_enter_second();psx_mod_write_byte(PLAY+0x49,7);mmx4_coop_leave_second();
    mmx4_coop_lifecycle_view_restore();
    CHECK(!memcmp(&original_second,&second_inventory,sizeof original_second));
    CHECK(!memcmp(&original_saved,&saved_inventory,sizeof original_saved));
    CHECK(mmx4_coop_lifecycle_digest(2166136261u)==inventory_digest);
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(0x801419F4u,1);
        psx_mod_write_half(0x801419B0u+0x1C,100);
        psx_mod_write_word(PLAYER+8,0x00081234u);
        mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,0xFFEC5678u);
        mmx4_coop_leave_second();
        mmx4_coop_lifecycle_camera_bounds(&cpu,0);
        CHECK(p2[10]==8 && p2[11]==0 && p2[8]==0x78 && p2[9]==0x56);
        CHECK(psx_mod_read_word(PLAYER+8)==0x00081234u);
        CHECK(!boundary_calls[0] && boundary_calls[1]==1 && !world_calls && !projected);
        psx_mod_write_half(0x801419B0u+10,101);
        mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,0x01C25678u);
        mmx4_coop_leave_second();mmx4_coop_lifecycle_camera_bounds(&cpu,0);
        CHECK(p2[10]==0x9C && p2[11]==1 && p2[8]==0x78 && p2[9]==0x56);
        CHECK(psx_mod_read_word(PLAYER+8)==0x00081234u);
    }
    reset();psx_mod_write_byte(0x801419F4u,1);
    psx_mod_write_word(PLAYER+8,0xFFEC1234u);
    mmx4_coop_lifecycle_camera_bounds(&cpu,1);
    CHECK(psx_mod_read_word(PLAYER+8)==0x00081234u);
    CHECK(boundary_calls[0]==1 && !boundary_calls[1] && !projected);
    reset();psx_mod_write_byte(0x801419F4u,1);p2[0xBC]=5;
    mmx4_coop_lifecycle_camera_bounds(&cpu,0);CHECK(!boundary_calls[1]);
    p2[0xBC]=0;p2[4]=3;p2[0x5C]=0;
    mmx4_coop_lifecycle_camera_bounds(&cpu,0);CHECK(!boundary_calls[1]);
    p2[4]=1;p2[0x5C]=12;warp_owner=0;warp_phase=2;
    mmx4_coop_lifecycle_camera_bounds(&cpu,0);CHECK(!boundary_calls[1]);
    warp_phase=0;psx_mod_write_byte(0x801419F4u,0);
    mmx4_coop_lifecycle_camera_bounds(&cpu,0);CHECK(!boundary_calls[1]);
    reset();
    /* Permanent upgrades belong to one campaign regardless of collector.
     * Projection must preserve a new heart, maxHP and shared tank energy. */
    psx_mod_write_byte(MMX4_PLAY+0x5C,0x89);psx_mod_write_half(MMX4_PLAY+0x5A,0x3000);
    mmx4_coop_enter_second();
    psx_mod_write_byte(MMX4_PLAY+0x46,34);psx_mod_write_byte(MMX4_PLAY+0x5A,4);
    psx_mod_write_byte(MMX4_PLAY+0x5C,0x88);mmx4_coop_lifecycle_project();
    mmx4_coop_leave_second();
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x46)==34);
    CHECK(psx_mod_read_half(MMX4_PLAY+0x5A)==0x3004);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x5C)==0x88);
    mmx4_coop_enter_second();CHECK(psx_mod_read_byte(MMX4_PLAY+0x46)==34);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x5A)==4);mmx4_coop_leave_second();
    uint32_t digest=mmx4_coop_lifecycle_digest(2166136261u);
    mmx4_coop_enter_second();mmx4_coop_leave_second();
    CHECK(digest==mmx4_coop_lifecycle_digest(2166136261u));
    mmx4_coop_enter_second();digest=mmx4_coop_lifecycle_digest(2166136261u);
    saved_inventory.hp=7;
    CHECK(digest!=mmx4_coop_lifecycle_digest(2166136261u));
    saved_inventory.hp=0;mmx4_coop_leave_second();
    /* Changing the selected campaign changes P2's character, with the same
     * permanent maxHP/heart progress and fresh transient inventory. */
    psx_mod_write_byte(MMX4_PLAY+0x43,1);mmx4_coop_enter_second();
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==0);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x46)==34);mmx4_coop_leave_second();

    reset();inputs[0]=inputs[1]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(menu_active && menu_owner==0 && psx_mod_read_byte(PLAY+1)==2);
    reset();inputs[1]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(menu_active && menu_owner==1 && psx_mod_read_byte(PLAY+1)==2);
    inputs[1]=0;pause_commit=1;psx_mod_write_byte(PLAY+0x5C,0x88);
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(p2[0x5C]==13 && psx_mod_read_byte(PLAYER+0x5C)==16);
    CHECK(psx_mod_read_byte(PLAY+0x5C)==0x87 && observed_campaign==1);
    CHECK(world_draws==1);
    psx_mod_write_byte(0x80166BB0u,2);
    pause_commit=0;pause_exit=1;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(!menu_active && !projected && world_calls==1 && observed_campaign==0);
    CHECK(world_draws==2 && psx_mod_read_byte(0x80166BB0u)==3);

    /* Shipped41 can retain the pause callback guard across Quit/Continue.
     * A fresh stage must admit P2's real menu again; a completed native menu
     * must not keep suppressing the partner's controller in gameplay. */
    reset();menu_active=menu_guard=1;menu_owner=1;
    mmx4_coop_lifecycle_reset();
    CHECK(!menu_active && !menu_guard && !menu_projecting);
    inputs[1]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(menu_active && menu_owner==1);
    inputs[1]=0;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(observed_campaign==1);
    pause_exit=1;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(!menu_active && mmx4_coop_lifecycle_can_tick());
    reset();menu_active=menu_guard=1;menu_owner=1;
    mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(!menu_active && !menu_guard && mmx4_coop_lifecycle_can_tick());

    /* A P2 collection heals P2 on later native actor ticks. */
    reset();uint32_t actor=0x8013D000u;
    psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+8,100);
    p2[8]=100;mmx4_coop_call(&cpu,0x800C00BCu,actor,0);
    CHECK(collect_calls[0]==1 && collect_calls[1]==1);
    mmx4_coop_call(&cpu,0x800BF730u,actor,0);
    CHECK(p2[0x5C]==13 && psx_mod_read_byte(PLAYER+0x5C)==16);
    mmx4_coop_call(&cpu,0x800BF730u,actor,0);CHECK(p2[0x5C]==14);
    CHECK(!pickup_record(actor,0));
    /* Split refills remain owned after the collector moves away. Each native
     * update heals once, with no world pause or unrelated scene lock changes.
     * Unified keeps the original whole-world refill pause. */
    static const unsigned health_kinds[]={0,1,5}; /* small, large, full */
    for(unsigned form=0;form<3;++form)
      for(unsigned split=0;split<2;++split)for(unsigned owner=0;owner<2;++owner) {
        reset();split_views=(int)split;
        psx_mod_write_byte(actor+0x7C,(uint8_t)health_kinds[form]);
        psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+8,owner?100:0);
        p2[8]=100;mmx4_coop_call(&cpu,0x800C00BCu,actor,0);
        CHECK(psx_mod_read_byte(PLAY+0x10)==!split);
        if(split)CHECK(pickup_record(actor,0) && pickup_record(actor,0)->owner==owner);
        psx_mod_write_word(PLAYER+8,200);p2[8]=0;
        for(unsigned i=0;i<2;++i)mmx4_coop_call(&cpu,0x800BF730u,actor,0);
        CHECK(psx_mod_read_byte(PLAYER+0x5C)==(owner?16:18));
        CHECK(p2[0x5C]==(owner?14:12));
        CHECK(!psx_mod_read_byte(PLAY+0x10) && !pickup_record(actor,0));
    }
    for(unsigned form=0;form<3;++form)
      for(unsigned owner=0;owner<2;++owner)for(unsigned die=0;die<2;++die) {
        reset();split_views=1;
        psx_mod_write_byte(actor+0x7C,(uint8_t)health_kinds[form]);
        for(unsigned i=0x10;i<0x18;++i)psx_mod_write_byte(PLAY+i,(uint8_t)(i+1));
        psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+8,owner?100:0);p2[8]=100;
        mmx4_coop_call(&cpu,0x800C00BCu,actor,0);
        if(die) {
            if(owner) {p2[0x5C]=0;p2[4]=3;}
            else {psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+4,3);}
        }
        for(unsigned i=0;i<2;++i)mmx4_coop_call(&cpu,0x800BF730u,actor,0);
        for(unsigned i=0x10;i<0x18;++i)CHECK(psx_mod_read_byte(PLAY+i)==i+1);
        if(die)CHECK(owner?!p2[0x5C]:!psx_mod_read_byte(PLAYER+0x5C));
    }
    reset();psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+8,100);p2[8]=100;
    mmx4_coop_call(&cpu,0x800C00BCu,actor,0);CHECK(!collect_calls[0] && collect_calls[1]==1);

    /* One corpse neither ends the stage nor freezes the survivor. */
    reset();psx_mod_write_byte(PLAYER+4,2);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_call(&cpu,0x80035A6Cu,PLAYER,0);
    CHECK(!psx_mod_read_byte(PLAY+0x12) && !psx_mod_read_byte(PLAY+0x1C));
    mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(!psx_mod_read_byte(PLAY+1) && world_calls==1);
    p2[4]=3;p2[0x5C]=0;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(psx_mod_read_byte(PLAY+1)==1);
    /* A healthy P1 passenger has finished its outgoing beam while P2
     * remains in the room. That state must not restart the whole stage. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned phase=1;phase<=3;phase+=2) {
        reset();psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAYER+4,3);warp_owner=1;warp_phase=phase;
        mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
        CHECK(!psx_mod_read_byte(PLAY+1) && world_calls==1);
        CHECK(psx_mod_read_byte(PLAYER+0x5C)==16 && p2[0x5C]==12);
    }
    /* A shared victory retains both bodies through the native poses and
     * waits for the longer character animation before stage termination. */
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();uint32_t boss=0x8013BED0u;
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAY+12,5);psx_mod_write_word(PLAY+0x20,boss);
        psx_mod_write_word(0x1F80004Cu,boss);
        psx_mod_write_byte(boss,1);psx_mod_write_byte(boss+1,56);psx_mod_write_byte(boss+4,2);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        cpu.gpr[4]=0x14;CHECK(paired_victory_scene(&cpu));
        departed=1;CHECK(!paired_victory_scene(&cpu));departed=0;
        psx_mod_write_byte(PLAYER+0x5C,0);CHECK(!paired_victory_scene(&cpu));psx_mod_write_byte(PLAYER+0x5C,16);
        p2[0x5C]=0;CHECK(!paired_victory_scene(&cpu));p2[0x5C]=12;
        psx_mod_write_byte(boss+1,70);CHECK(!paired_victory_scene(&cpu));psx_mod_write_byte(boss+1,56);
        psx_mod_write_byte(boss+0x5C,1);CHECK(!paired_victory_scene(&cpu));psx_mod_write_byte(boss+0x5C,0);
        mmx4_coop_call(&cpu,0x80036AE4u,0x14,0x40);
        CHECK(paired_victory && !warp_pending && !warp_phase);
        Mmx4SnapshotIO count={0};count.mode=MMX4_SNAPSHOT_COUNT;count.ok=1;
        CHECK(mmx4_coop_lifecycle_snapshot(&count));
        uint8_t *saved=malloc(count.pos);CHECK(saved);
        Mmx4SnapshotIO io={saved,NULL,count.pos,0,MMX4_SNAPSHOT_WRITE,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&io));paired_victory=0;
        io=(Mmx4SnapshotIO){NULL,saved,count.pos,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_lifecycle_snapshot(&io) && paired_victory);free(saved);
        psx_mod_write_byte(PLAY+0x0F,0x10);
        cpu.gpr[4]=PLAYER;CHECK(!partner_victory_pose(&cpu,0x80034F7Cu));
        mmx4_coop_enter_second();
        CHECK(partner_victory_pose(&cpu,0x80034F7Cu));
        CHECK(psx_mod_read_byte(PLAYER+5)==0x19 && !psx_mod_read_byte(PLAYER+6));
        mmx4_coop_leave_second();CHECK(p2[0] && p2[3] && p2[5]==0x19 && p2[0x5C]==12);
        psx_mod_write_byte(PLAYER+4,3);
        mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
        CHECK(!psx_mod_read_byte(PLAY+1) && world_calls==1);
        p2[4]=3;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
        CHECK(psx_mod_read_byte(PLAY+1)==1);
    }
    reset();paired_victory=1;fresh_game(&cpu,0);CHECK(!paired_victory);
    /* A completed stage-owner warp has state3 and positive HP; let native
     * stage termination advance, including with a hidden P2 passenger. */
    reset();psx_mod_write_byte(PLAYER+4,3);
    mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(psx_mod_read_byte(PLAY+1)==1);
    reset();psx_mod_write_byte(PLAYER+4,3);warp_owner=0;warp_phase=1;
    psx_mod_write_byte(PLAY+0x0F,1);
    mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(psx_mod_read_byte(PLAY+1)==1);
    /* A genuine forward exit may finish while P1's death is still in
     * state1/2. Only the healthy completed P2 beam satisfies this gate;
     * never project a body or advance on a catch-up/retry/withdrawal. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned gate=0;gate<12;++gate) {
        reset();psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+4,1);
        psx_mod_write_byte(PLAYER+5,17);psx_mod_write_byte(PLAY+0x0F,1);p2[4]=3;
        switch(gate) {
        case 1:psx_mod_write_byte(PLAYER+4,2);break;
        case 2:psx_mod_write_byte(PLAY+0x0F,0);break;
        case 3:psx_mod_write_byte(PLAY+0x0F,255);break;
        case 4:psx_mod_write_byte(PLAYER+0x5C,16);break;
        case 5:p2[0x5C]=0;break;
        case 6:p2[4]=1;break;
        case 7:departed=1;break;
        case 8:warp_owner=0;warp_phase=1;break;
        case 9:local_view=1;break;
        case 10:psx_mod_write_byte(PLAY,7);break;
        case 11:ready=0;break;
        }
        uint8_t original[0xE4];read_bytes(PLAYER,original,sizeof original);
        mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
        CHECK((psx_mod_read_byte(PLAY+1)==1)==(gate<2));
        CHECK(!memcmp(ram+(PLAYER&0x1FFFFFu),original,sizeof original) && !projected);
    }
    reset();psx_mod_write_byte(PLAY+12,7);psx_mod_write_byte(PLAYER+4,3);
    psx_mod_write_byte(PLAYER+0x5C,0);p2[10]=0xD1;p2[11]=0x18;
    cpu.gpr[2]=438;storm_survivor_exit(&cpu,0x800DA44Cu);
    CHECK(cpu.gpr[2]==6353);
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,12);
    cpu.gpr[2]=438;storm_survivor_exit(&cpu,0x800DA44Cu);CHECK(cpu.gpr[2]==6353);
    cpu.gpr[2]=6400;storm_survivor_exit(&cpu,0x800DA44Cu);CHECK(cpu.gpr[2]==6400);
    local_view=1;cpu.gpr[2]=438;storm_survivor_exit(&cpu,0x800DA44Cu);CHECK(cpu.gpr[2]==438);
    local_view=0;p2[0x5C]=0;
    storm_survivor_exit(&cpu,0x800DA44Cu);CHECK(cpu.gpr[2]==438);
    /* The Cyber assessment completion producer belongs to the hero who
     * entered the course portal. Keep its native update once and preserve
     * the campaign and P1 body; reject unrelated effects/scenes and hidden,
     * voluntarily withdrawn or dead P2. A survivor may own the course too. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
    for(unsigned gate=0;gate<17;++gate) {
        reset();const uint32_t effect=0x80173D00u,parent=0x80143028u;
        psx_mod_write_byte(PLAY+12,6);psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_word(PLAYER+8,72u<<16);put32(p2+8,1792u<<16);
        psx_mod_write_byte(effect,65);psx_mod_write_byte(effect+1,34);
        psx_mod_write_word(effect+0x50,parent);
        psx_mod_write_byte(parent,1);psx_mod_write_byte(parent+1,38);
        script_active=1;script_owner=1;script_serial=7;
        assessment_cull=mode;psx_mod_write_half(0x801419BAu,100);
        psx_mod_write_byte(effect+4,1);
        SceneOwner *record=scene_record(parent,1);record->owner=1;record->serial=7;
        switch(gate) {
        case 1:psx_mod_write_byte(PLAYER+0x5C,0);script_active=0;break;
        case 2:warp_owner=1;warp_phase=1;break;
        case 3:p2[4]=3;break;
        case 4:record->owner=0;break;
        case 5:record->serial=6;break;
        case 6:script_active=0;break;
        case 7:script_owner=0;break;
        case 8:psx_mod_write_byte(parent+1,37);break;
        case 9:psx_mod_write_word(effect+0x50,parent+1);break;
        case 10:psx_mod_write_byte(effect+1,33);break;
        case 11:psx_mod_write_byte(PLAY+13,1);break;
        case 12:local_view=1;break;
        case 13:departed=1;break;
        case 14:warp_owner=0;warp_phase=1;break;
        case 15:p2[0x5C]=0;break;
        case 16:psx_mod_write_byte(PLAY,7);break;
        }
        uint8_t original[0xE4];read_bytes(PLAYER,original,sizeof original);
        mmx4_coop_call(&cpu,0x800CE894u,effect,0);
        CHECK(assessment_calls[0]==(gate>=4) && assessment_calls[1]==(gate<4));
        CHECK(psx_mod_read_word(effect+8)==((gate<4?1792u:72u)<<16));
        CHECK(psx_mod_read_half(0x801419BAu)==100);
        CHECK((psx_mod_read_byte(effect)!=0)==(!mode || gate<4));
        CHECK(psx_mod_read_byte(parent+0x16)==(mode && gate<4));
        CHECK(!memcmp(ram+(PLAYER&0x1FFFFFu),original,sizeof original) && !projected);
        CHECK(observed_campaign==campaign && psx_mod_read_byte(PLAY+0x43)==campaign);
    }
    /* Owl's adjacent entry/release areas can observe different heroes: the
     * rear hero at2680 releases the minimum after the leader crossed2648.
     * The real boss introduction must own its authored enclosure, including
     * P2-led/survivor entries. Ordinary backtracking and other scenes retain
     * their native bounds. No current camera position is teleported. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
        for(unsigned owner=0;owner<2;++owner)for(unsigned survivor=0;survivor<2;++survivor) {
            reset();split_views=(int)mode;
            psx_mod_write_byte(PLAY+12,7);psx_mod_write_byte(PLAY+13,1);
            psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
            if(survivor) {
                if(owner)psx_mod_write_byte(PLAYER+0x5C,0);
                else p2[0x5C]=0;
            }
            psx_mod_write_half(0x801419D4u,2240);psx_mod_write_half(0x801419D6u,0);
            psx_mod_write_half(0x801419CEu,0);psx_mod_write_half(0x801419BAu,2300);
            if(owner)mmx4_coop_enter_second();
            cpu.gpr[31]=0x80075104u;
            mmx4_coop_call(&cpu,0x80036AE4u,0x14,0);
            if(owner)mmx4_coop_leave_second();
            CHECK(psx_mod_read_half(0x801419D6u)==2240);
            CHECK(psx_mod_read_half(0x801419D4u)==2240);
            CHECK(psx_mod_read_half(0x801419CEu)==0);
            CHECK(psx_mod_read_half(0x801419BAu)==2300);
        }
    for(unsigned test=0;test<8;++test) {
        reset();psx_mod_write_byte(PLAY+12,7);psx_mod_write_byte(PLAY+13,1);
        cpu.gpr[31]=0x80075104u;
        if(test==0)psx_mod_write_byte(PLAY+12,12); /* Boss rush. */
        if(test==1)psx_mod_write_byte(PLAY+13,0);
        if(test==2)cpu.gpr[31]+=4;
        if(test==3)local_view=1;
        if(test==4)ready=0;
        if(test==5)psx_mod_write_byte(PLAY,7);
        mmx4_coop_call(&cpu,0x80036AE4u,test==6?0x15:0x14,test==7?0x40:0);
        CHECK(psx_mod_read_half(0x801419D6u)==0);
    }
    /* Every stage/section's survivor pool must cull and write bounds using
     * its real camera before the first ordinary post-death camera update.
     * This includes Web Spider's return strips as well as Sigma's. The
     * canonical live-player and Unified paths retain their existing view. */
    for(unsigned stage=0;stage<14;++stage)for(unsigned section=0;section<2;++section)
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned test=0;test<4;++test) {
        reset();stage_camera_model=1;split_views=test!=1;
        psx_mod_write_byte(PLAY+12,(uint8_t)stage);
        psx_mod_write_byte(PLAY+13,(uint8_t)section);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAYER+0x5C,test==2?32:0);
        p2[0x5C]=(uint8_t)(test==3?0:32);
        psx_mod_write_half(0x801419BEu,2048);
        mmx4_coop_call(&cpu,0x8002166Cu,0,0);
        CHECK(stage_script_calls==1 && !projected);
        CHECK(stage_camera_y==(test?2048u:736u));
        CHECK(stage_camera_commits==(test==0));
        CHECK(psx_mod_read_half(0x801419D8u)==2048);
        CHECK(observed_campaign==campaign);
        CHECK(psx_mod_read_byte(PLAYER+0x5C)==(test==2?32:0));
        CHECK(p2[0x5C]==(test==3?0:32));
    }
    /* The part2 pillar sequence is distinct from ordinary fireball emitters.
     * Exercise the original signed X>576 admission and exclusion boundaries. */
    for(unsigned mode=0;mode<2;++mode)for(unsigned test=0;test<17;++test) {
        reset();split_views=(int)mode;psx_mod_write_byte(PLAY+12,4);psx_mod_write_byte(PLAY+13,1);
        uint32_t actor=0x80142F98u;cpu.gpr[4]=actor;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,34);
        uint32_t original_x=test==14?(576u<<16)|0x8000u:test==16?577u<<16:64u<<16;
        psx_mod_write_word(PLAYER+8,original_x);
        put32(p2+8,(test==1?576u:test==2?577u:640u)<<16);
        if(test==3)p2[0x5C]=0;
        if(test==4)projected=1;
        if(test==5)local_view=1;
        if(test==6)psx_mod_write_byte(PLAY+12,3);
        if(test==7)psx_mod_write_byte(PLAY+13,0);
        if(test==8)psx_mod_write_byte(PLAY+0x1D,1);
        if(test==9)psx_mod_write_byte(actor+4,1);
        if(test==10)psx_mod_write_byte(actor+1,33);
        if(test==11)ready=0;
        if(test==12)psx_mod_write_byte(PLAY,7);
        if(test==13)scene_guard=1;
        if(test==15)put32(p2+8,(576u<<16)|0xC000u);
        unsigned calls=stage_script_calls;
        int handled=volcano_platform_start(&cpu,0x800BCE48u);
        CHECK(handled==(test==0 || test==2 || test==14));
        CHECK(stage_script_calls==calls+(unsigned)handled);
        CHECK(psx_mod_read_word(PLAYER+8)==original_x);
        if(handled)CHECK(psx_mod_read_byte(actor+4)==1);
        if(test==13)scene_guard=0;
    }
    /* Either living hero can arm the original opening-ice cycle, including
     * both exact interval edges. The clock and shared world are untouched. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
        for(unsigned test=0;test<12;++test) {
            reset();split_views=mode;psx_mod_write_byte(PLAY+12,2);
            psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
            unsigned x=test==1?672u:test==2?673u:test==3?3167u:test==4?3168u:705u;
            p2[10]=(uint8_t)x;p2[11]=(uint8_t)(x>>8);cpu.gpr[2]=144;
            if(test==5)p2[0x5C]=0;
            if(test==6)projected=1;
            if(test==7)local_view=1;
            if(test==8)psx_mod_write_byte(PLAY+12,1);
            if(test==9)psx_mod_write_byte(PLAY+13,1);
            if(test==10)cpu.gpr[2]=900;
            if(test==11)ready=0;
            frost_ice_activation(&cpu,0x800B9F74u);
            unsigned wanted=test==0 || test==2 || test==3?x:test==10?900u:144u;
            CHECK(cpu.gpr[2]==wanted);
        }
    /* The later bridge is a separate signed X>=3168 predicate. Include the
     * exact boundary, dead/hidden seats, and presentation/projection guards. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
        for(unsigned test=0;test<15;++test) {
            reset();split_views=mode;psx_mod_write_byte(PLAY+12,2);
            psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
            int x=test==1?3167:test==2?3168:test==3?-1:3246;
            p2[10]=(uint8_t)x;p2[11]=(uint8_t)((uint16_t)x>>8);cpu.gpr[2]=144;
            if(test==4)p2[0x5C]=0;
            if(test==5)projected=1;
            if(test==6)local_view=1;
            if(test==7)psx_mod_write_byte(PLAY+12,1);
            if(test==8)psx_mod_write_byte(PLAY+13,1);
            if(test==9)cpu.gpr[2]=4000;
            if(test==10)ready=0;
            if(test==11)psx_mod_write_byte(PLAY,7);
            if(test==12)cpu.gpr[2]=(uint32_t)-1;
            if(test==13)psx_mod_write_byte(PLAYER+0x5C,0);
            if(test==14)p2[0]=0;
            frost_path_activation(&cpu,0x800BA39Cu);
            unsigned wanted=test==0 || test==2 || test==12 || test==13?
                (unsigned)x:test==9?4000u:144u;
            CHECK(cpu.gpr[2]==wanted);
        }
    reset();
    /* The collapse starts on contact, rather than a guessed expiry timer.
     * Check each native roster/camera mode and restrict the pure geometry
     * extension to the exact type17 caller; no other collision is widened. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
        for(unsigned test=0;test<13;++test) {
            reset();ice_contact_model=1;memset(ice_contact_calls,0,sizeof ice_contact_calls);
            split_views=mode;psx_mod_write_byte(PLAY+12,2);
            psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
            const uint32_t trigger=0x80165A30u;
            psx_mod_write_byte(trigger,1);psx_mod_write_byte(trigger+1,17);psx_mod_write_byte(trigger+4,1);
            psx_mod_write_half(trigger+10,3456);psx_mod_write_half(PLAYER+10,144);
            p2[10]=0x80;p2[11]=0x0D;
            cpu.gpr[4]=trigger;cpu.gpr[5]=PLAYER;cpu.gpr[31]=0x800C4204u;
            if(test==1)psx_mod_write_half(PLAYER+10,3456);
            if(test==2)p2[0x5C]=0;
            if(test==3)projected=1;
            if(test==4)local_view=1;
            if(test==5)ready=0;
            if(test==6)psx_mod_write_byte(PLAY+12,1);
            if(test==7)psx_mod_write_byte(PLAY+13,1);
            if(test==8)psx_mod_write_byte(trigger+1,18);
            if(test==9)cpu.gpr[31]=0x800C4208u;
            if(test==10)psx_mod_write_byte(trigger+4,2);
            if(test==11)cpu.gpr[4]=trigger+1;
            if(test==12)p2[10]=0;
            uint8_t body[0xE4];memcpy(body,ram+(PLAYER&0x1FFFFFu),sizeof body);
            unsigned was_projected=projected;
            int handled=frost_path_contact(&cpu,0x8002C160u);
            CHECK(handled==(test<2 || test==12));
            if(handled)CHECK(cpu.gpr[2]==(test<2));
            CHECK(projected==was_projected && !frost_path_contact_call);
            CHECK(!memcmp(body,ram+(PLAYER&0x1FFFFFu),sizeof body));
            CHECK(ice_contact_calls[1]==(test==0 || test==12));
            ice_contact_model=0;
        }
    /* The platform's native threshold still decides admission. Extend only
     * its loaded X for either a leading living P2 or a P2 survivor. */
    for(unsigned subtype=0;subtype<15;++subtype)for(unsigned test=0;test<16;++test) {
        reset();uint32_t platform=0x80165A30u+0x8Cu;
        psx_mod_write_byte(PLAY+12,0);psx_mod_write_byte(PLAY+13,1);
        psx_mod_write_byte(platform,1);psx_mod_write_byte(platform+1,1);
        psx_mod_write_byte(platform+2,(uint8_t)subtype);psx_mod_write_byte(platform+4,1);
        p2[10]=0xD1;p2[11]=7; /*2001, past subtype5's2000 threshold. */
        cpu.gpr[16]=platform;cpu.gpr[3]=1872;cpu.gpr[2]=0x12345678;
        switch(test) {
        case 1:psx_mod_write_byte(PLAYER+0x5C,0);cpu.gpr[3]=2500;break;
        case 2:cpu.gpr[3]=2010;break; /* P1 remains the farther live hero. */
        case 3:p2[0x5C]=0;break;
        case 4:ready=0;break;
        case 5:projected=1;break;
        case 6:local_view=1;break;
        case 7:psx_mod_write_byte(PLAY,7);break;
        case 8:psx_mod_write_byte(PLAY+12,1);break;
        case 9:psx_mod_write_byte(PLAY+13,0);break;
        case 10:cpu.gpr[16]=platform+1;break;
        case 11:psx_mod_write_byte(platform,0);break;
        case 12:psx_mod_write_byte(platform+1,2);break;
        case 13:psx_mod_write_byte(platform+2,15);break;
        case 14:psx_mod_write_byte(platform+4,2);break;
        case 15:psx_mod_write_byte(platform+5,1);break;
        }
        uint32_t before=cpu.gpr[3];uint8_t saved[0x8C];read_bytes(platform,saved,sizeof saved);
        intro_platform_activation(&cpu,0x800BEEF4u);
        CHECK(cpu.gpr[3]==(test<2?2001u:before));
        CHECK(cpu.gpr[2]==0x12345678);
        uint8_t after[0x8C];read_bytes(platform,after,sizeof after);CHECK(!memcmp(saved,after,sizeof saved));
        projected=0;local_view=0;
    }
    /* The recorded P1 corpse stays at Y394 while P2 reaches the crusher
     * floor at Y955. Only that survivor may satisfy the native Y predicate. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned ineligible=0;ineligible<9;++ineligible) {
        reset();uint32_t crusher=0x80165A30u+0x8Cu;
        psx_mod_write_byte(PLAY+12,0);psx_mod_write_byte(PLAY+13,1);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
        psx_mod_write_half(PLAYER+14,394);p2[14]=0xBB;p2[15]=3;
        psx_mod_write_byte(crusher,1);psx_mod_write_byte(crusher+1,3);psx_mod_write_byte(crusher+4,1);
        cpu.gpr[16]=crusher;cpu.gpr[3]=394;
        switch(ineligible) {
        case 1:psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,16);break;
        case 2:p2[0x5C]=0;p2[4]=2;break;
        case 3:psx_mod_write_byte(PLAY+12,7);break;
        case 4:psx_mod_write_byte(PLAY+13,0);break;
        case 5:psx_mod_write_byte(crusher+1,8);break;
        case 6:psx_mod_write_byte(crusher+4,2);break;
        case 7:cpu.gpr[16]=crusher+1;break;
        case 8:warp_phase=1;warp_owner=0;p2[0]=0;break;
        }
        intro_survivor_crusher(&cpu,0x800C0574u);
        CHECK(cpu.gpr[3]==(ineligible>1?394u:955u));
        CHECK(psx_mod_read_half(PLAYER+14)==394 &&
            psx_mod_read_byte(PLAYER+0x5C)==(ineligible==1?16:0));
    }
    reset();p2[4]=2;p2[0x5C]=0;mmx4_coop_enter_second();
    mmx4_coop_call(&cpu,0x80035A6Cu,PLAYER,0);mmx4_coop_leave_second();
    CHECK(!psx_mod_read_byte(PLAY+0x12) && !psx_mod_read_byte(PLAY+0x1C));

    reset();psx_mod_write_byte(PLAY+0x59,4);psx_mod_write_byte(PLAYER+0xB9,4);
    psx_mod_write_byte(PLAY+0x26,1);mmx4_coop_call(&cpu,0x8001FA24u,PLAY,0);
    CHECK(psx_mod_read_byte(PLAYER+0xB9)==5 && p2[0xB9]==5);
    CHECK(psx_mod_read_byte(0x800F1D94u)==5);
    psx_mod_write_byte(PLAYER+2,1);psx_mod_write_byte(PLAY+0x43,1);p2[2]=0;
    mmx4_coop_call(&cpu,0x800C6EDCu,actor,0);
    CHECK(p2[0xA7]==2 && !psx_mod_read_byte(PLAYER+0xA7));
    CHECK(psx_mod_read_byte(PLAY+0x47)==2);

    /* A P2-triggered completion updates canonical body, campaign, Continue
     * record and both ability copies once, in either character order. */
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();reward_calls=save_calls=0;
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAY+0x59,1);psx_mod_write_byte(PLAYER+0xB9,1);p2[0xB9]=1;
        psx_mod_write_byte(PLAY+0x26,5);
        mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x8001FA24u,PLAY,0);
        CHECK(projected && psx_mod_read_byte(PLAY+0x43)==(campaign^1u));
        mmx4_coop_leave_second();
        CHECK(psx_mod_read_byte(PLAY+0x43)==campaign);
        CHECK(psx_mod_read_byte(PLAY+0x59)==17 && psx_mod_read_byte(PLAYER+0xB9)==17 && p2[0xB9]==17);
        CHECK(psx_mod_read_byte(0x800F1D90u)==campaign && psx_mod_read_byte(0x800F1D94u)==17);
        mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x8001FA24u,PLAY,0);mmx4_coop_leave_second();
        CHECK(reward_calls==1 && save_calls==1);
        for(unsigned writer=0;writer<2;++writer) {
            psx_mod_write_byte(PLAY+0x46,36);psx_mod_write_byte(PLAY+0x47,3);
            psx_mod_write_byte(PLAY+0x5A,3);psx_mod_write_byte(PLAY+0x45,9);
            mmx4_coop_enter_second();psx_mod_write_byte(PLAY+0x45,4);
            mmx4_coop_call(&cpu,writer?0x8001C07Cu:0x8001C3E8u,0,0);
            CHECK(projected && psx_mod_read_byte(PLAY+0x45)==4);
            mmx4_coop_leave_second();
            uint32_t record=writer?0x80168000u:0x800F1D90u;
            CHECK(psx_mod_read_byte(record)==campaign && psx_mod_read_byte(record+1)==36);
            CHECK(psx_mod_read_byte(record+3)==3 && psx_mod_read_byte(record+4)==17);
            CHECK(psx_mod_read_byte(record+6)==3 && psx_mod_read_byte(PLAY+0x45)==9);
        }
        psx_mod_write_word(PLAY,6);psx_mod_write_byte(PLAY+1,2);
        menu_active=1;menu_owner=1;inputs[1]=SELECT;menu_previous[1]=0;
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);CHECK(observed_menu_select==SELECT);
        menu_owner=0;inputs[0]=SELECT;menu_previous[0]=0;
        mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);CHECK(observed_menu_select==SELECT);
    }

    /* A same-section checkpoint retains a dead passenger. */
    reset();mmx4_coop_lifecycle_reset();p2[4]=3;p2[0x5C]=0;
    mmx4_coop_lifecycle_reset();
    memset(p2,0,sizeof p2);p2[0]=p2[3]=p2[4]=1;p2[0x5C]=32;
    p2_vehicle[0]=p2_vehicle[3]=1;
    mmx4_coop_lifecycle_enrolled(&cpu);CHECK(p2[4]==3 && !p2[0x5C] && !p2[3]);
    CHECK(!p2_vehicle[0] && !p2_vehicle[3]);
    full_stage(&cpu,0);mmx4_coop_lifecycle_reset();
    p2[4]=1;p2[3]=1;p2[0x5C]=32;mmx4_coop_lifecycle_enrolled(&cpu);
    CHECK(p2[4]==1 && p2[0x5C]==32);
    /* Native section mode10 transitions through full loader mode4 too;
     * revive a dead P1 at the next section, after the native clear. */
    reset();mmx4_coop_lifecycle_reset();
    psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    psx_mod_write_byte(PLAY+0x0D,1);full_stage(&cpu,0);
    stage_initialization(&cpu,0);
    /* Original 8002A7D0 clears P1 before native 80035240; the callback
     * must retain the preceding death, not infer status from that storage. */
    psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_lifecycle_reset();
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
    psx_mod_write_byte(MMX4_VEHICLE,1);psx_mod_write_byte(MMX4_VEHICLE+3,1);
    p2[0x5C]=32; /* Native counterpart initializer has run. */
    mmx4_coop_lifecycle_enrolled(&cpu);
    CHECK(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0x5C)==32);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+3));
    CHECK(p2[0x5C]==32 && p2[4]==1);

    /* A living P1 must not turn into a corpse when the same native clear
     * runs during section transfer or same-assets checkpoint initialization. */
    for(unsigned full=0;full<2;++full) {
        reset();mmx4_coop_lifecycle_reset();
        if(full) {psx_mod_write_byte(PLAY+0x0D,1);full_stage(&cpu,0);}
        uint32_t before_capture=mmx4_coop_lifecycle_digest(2166136261u);
        stage_initialization(&cpu,0);
        CHECK(first_before_clear_valid && !first_before_clear_dead);
        CHECK(before_capture!=mmx4_coop_lifecycle_digest(2166136261u));
        psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
        mmx4_coop_lifecycle_reset();CHECK(!first_before_clear_valid);
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
        p2[0x5C]=32;
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0x5C)==32);
        CHECK(p2[4]==1 && p2[0x5C]==(full?32:12));
    }

    /* A healthy scripted OWNER also ends in native state3, without being
     * the hidden passenger. Native Jungle type5 arg0 requests PLAY0F=40
     * before section0->1. Neither owner may be carried as a death. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned full=0;full<2;++full) {
        reset();mmx4_coop_lifecycle_reset();
        psx_mod_write_byte(PLAY+0x0F,0x40);
        if(owner)p2[4]=3;else psx_mod_write_byte(PLAYER+4,3);
        if(full) {psx_mod_write_byte(PLAY+0x0D,1);full_stage(&cpu,0);}
        stage_initialization(&cpu,0);
        mmx4_coop_lifecycle_reset();
        CHECK(carry_resources && !carry_dead && !carry_first_dead);
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,16);
        p2[0]=p2[3]=p2[4]=1;p2[0x5C]=32;
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(p2[4]==1 && p2[0x5C]==(full?32:12));
        CHECK(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0x5C)==16);
    }
    for(unsigned seat=0;seat<2;++seat) {
        reset();
        if(seat)p2[4]=2;else psx_mod_write_byte(PLAYER+4,2);
        CHECK(seat_was_dead(seat));
        if(seat) {p2[4]=3;p2[0x5C]=0;}
        else {psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
        CHECK(seat_was_dead(seat));
        if(seat) {p2[4]=4;p2[0x5C]=12;}
        else {psx_mod_write_byte(PLAYER+4,4);psx_mod_write_byte(PLAYER+0x5C,16);}
        CHECK(!seat_was_dead(seat));
    }

    /* Frontend PLAYER clearing must not carry deaths/withdrawal into a
     * fresh entry of the same stage. Forward transfers above bypass it. */
    for(unsigned previous=0;previous<3;++previous) {
        reset();psx_mod_write_byte(PLAY+0x0D,1);mmx4_coop_lifecycle_reset();
        psx_mod_write_byte(PLAY+0x43,1);psx_mod_write_byte(PLAY+0x47,3);
        psx_mod_write_byte(PLAY+0x48,2);psx_mod_write_byte(PLAY+0x59,17);
        if(previous==1) {p2[4]=3;p2[0x5C]=0;}
        if(previous==2) {departed=1;p2[0]=p2[3]=0;}
        stage_selection(&cpu,0x8002E420u);
        CHECK(!stage_known && !first_before_clear_valid);
        psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
        psx_mod_write_byte(PLAY+0x0D,0);full_stage(&cpu,0);stage_initialization(&cpu,0);
        mmx4_coop_lifecycle_reset();
        CHECK(!carry_resources && !carry_first_dead && !carry_dead && !carry_departure);
        CHECK(psx_mod_read_byte(PLAY+0x43)==1 && psx_mod_read_byte(PLAY+0x47)==3);
        CHECK(psx_mod_read_byte(PLAY+0x48)==2 && psx_mod_read_byte(PLAY+0x59)==17);
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
        p2[0]=p2[3]=p2[4]=1;p2[0x5C]=32;
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0x5C)==32);
        CHECK(p2[0x5C]==32 && !departed);
    }

    /* Permanent upgrades follow P1; X's native weapon override stays local.
     * Native8003718C treats any nonzero A6 as a forced weapon13 selection. */
    reset();p2[0xA6]=0x55;p2[0xB8]=0x66;
    psx_mod_write_byte(PLAY+0x47,3);psx_mod_write_byte(PLAY+0x59,17);
    for(unsigned upgrade=1;upgrade<=2;++upgrade) {
        psx_mod_write_byte(PLAY+0x48,(uint8_t)upgrade);
        mmx4_coop_lifecycle_follow_progress(p2,0);
        CHECK(p2[0xA7]==3 && p2[0xB8]==upgrade && p2[0xB9]==17 && p2[0xA6]==0x55);
    }
    p2[0xA7]=0x88;p2[0xB8]=0x66;
    mmx4_coop_lifecycle_follow_progress(p2,1);
    CHECK(p2[0xA7]==0x88 && p2[0xB8]==0x66 && p2[0xA6]==0x55 && p2[0xB9]==17);

    /* An outgoing passenger uses the native finished-beam state 3 while
     * still having health. An area transfer must preserve that living seat. */
    for(unsigned owner=0;owner<2;++owner) {
        reset();mmx4_coop_lifecycle_reset();
        script_active=1;script_owner=owner;request_script_departure(owner);
        for(unsigned i=0;i<3;++i)mmx4_coop_lifecycle_tick(&cpu);
        CHECK(warp_phase==1);
        CHECK(owner?psx_mod_read_byte(PLAYER+4)==3:p2[4]==3);
        psx_mod_write_byte(PLAY+0x0D,1);full_stage(&cpu,0);stage_initialization(&cpu,0);
        mmx4_coop_lifecycle_reset();
        CHECK(carry_resources && !carry_dead && !carry_first_dead);
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,16);
        p2[0]=p2[3]=p2[4]=1;p2[0x5C]=32;
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(p2[0x5C]==32 && p2[4]==1);
        CHECK(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0x5C)==16);
    }
    reset();mmx4_coop_lifecycle_reset();departed=1;p2[0]=0;p2[4]=3;
    psx_mod_write_byte(PLAY+0x0D,1);full_stage(&cpu,0);stage_initialization(&cpu,0);
    mmx4_coop_lifecycle_reset();CHECK(carry_resources && carry_departure && !carry_dead);
    /* Forward sections revive dead teammates, even if an extra life was
     * collected before the transfer. Same-section checkpoints do not. */
    reset();mmx4_coop_lifecycle_reset();p2[4]=3;p2[0x5C]=0;
    psx_mod_write_byte(PLAY+0x44,3);psx_mod_write_byte(PLAY+0x0D,1);
    full_stage(&cpu,0);stage_initialization(&cpu,0);
    psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_lifecycle_reset();CHECK(carry_resources && !carry_dead && carry_refill);
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
    p2[4]=1;p2[3]=1;p2[0x5C]=32;mmx4_coop_lifecycle_enrolled(&cpu);
    CHECK(p2[4]==1 && p2[0x5C]==32);CHECK(psx_mod_read_byte(PLAYER+4)==1);

    /* The native stage11 slot spans two fortress levels: the story boss
     * (section0) and Sigma stage3 (section1). A dead teammate rejoins at
     * that new level. Ordinary forward sections revive either seat too,
     * preserving the survivor's resources and shared lives. */
    for(unsigned story=0;story<2;++story)for(unsigned dead=0;dead<2;++dead)
    for(unsigned forward=0;forward<2;++forward) {
        reset();psx_mod_write_byte(PLAY+0x0C,(uint8_t)(story?11:1));
        mmx4_coop_lifecycle_reset();
        if(dead) {p2[4]=3;p2[0x5C]=0;}
        else {psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
        psx_mod_write_byte(PLAY+0x0D,(uint8_t)forward);
        if(forward)full_stage(&cpu,0);
        stage_initialization(&cpu,0);
        psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
        mmx4_coop_lifecycle_reset();
        CHECK(carry_resources==!(story && forward));
        CHECK(carry_dead==(!forward && dead) && carry_first_dead==(!forward && !dead));
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,48);
        p2[0]=p2[3]=p2[4]=1;p2[0x5C]=48;
        mmx4_coop_lifecycle_enrolled(&cpu);
        if(forward) {
            CHECK(p2[0x5C]==48 && psx_mod_read_byte(PLAYER+0x5C)==48);
            CHECK(p2[4]<2 && psx_mod_read_byte(PLAYER+4)<2);
        }
        CHECK(psx_mod_read_byte(PLAY+0x44)==2);
    }

    /* Native forward-section refill policies0/-2 heal living P2 to the
     * initialized upgraded maximum, while -1 retains personal resources. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned policy=0;policy<3;++policy) {
        reset();psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        mmx4_coop_enter_second();mmx4_coop_leave_second();
        mmx4_coop_lifecycle_reset();p2[0xA8]=7;p2[0x93]=3;
        psx_mod_write_byte(PLAY+0x0D,1);
        psx_mod_write_byte(PLAY+0x1E,(uint8_t)(policy?253+policy:0));
        mmx4_coop_lifecycle_reset();
        p2[0]=p2[3]=p2[4]=1;p2[0x5C]=40;
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(p2[0x5C]==(policy==2?12:40));
        CHECK(p2[0x5D]==p2[0x5C] && p2[0x5E]==p2[0x5C]);
        CHECK(p2[0xA8]==7 && p2[0x93]==3);
        CHECK(psx_mod_read_byte(PLAY+0x44)==2);
        CHECK(!carry_refill);
    }

    /* Game-over Continue increases 0 to 2 after a team wipe; it is still a
     * team respawn. The native 0->255 underflow must not preserve corpses. */
    for(unsigned new_lives=2;new_lives<=255;new_lives+=253) {
        reset();psx_mod_write_byte(PLAY+0x44,0);mmx4_coop_lifecycle_reset();
        psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
        p2[4]=3;p2[0x5C]=0;psx_mod_write_byte(PLAY+0x44,(uint8_t)new_lives);
        stage_initialization(&cpu,0);
        psx_mod_write_byte(PLAYER+4,0);mmx4_coop_lifecycle_reset();
        CHECK(!carry_resources && !carry_first_dead && !carry_dead);
        psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
        p2[4]=p2[3]=1;p2[0x5C]=32;mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(psx_mod_read_byte(PLAYER+4)==1 && p2[4]==1 && p2[0x5C]==32);
    }

    reset();inputs[1]=SELECT;
    for(unsigned i=0;i<89;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!departed && mmx4_coop_alive(1));

    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(departed && !mmx4_coop_alive(1) && !mmx4_coop_lifecycle_can_tick());
    inputs[1]=0;mmx4_coop_lifecycle_tick(&cpu);
    inputs[1]=SELECT;mmx4_coop_lifecycle_tick(&cpu);CHECK(departed && rejoin_pending);
    psx_mod_write_byte(PLAYER+0x89,8);psx_mod_write_word(PLAYER+8,0x12340000u);
    psx_mod_write_word(PLAYER+12,0x34560000u);mmx4_coop_lifecycle_tick(&cpu);
    mmx4_coop_lifecycle_tick(&cpu); /* Native returning-pose completion. */
    CHECK(!departed && mmx4_coop_alive(1) && p2[0x5C]==12);
    CHECK(p2[10]==0x34 && p2[11]==0x12 && p2[14]==0x56 && p2[15]==0x34);
    CHECK(p2[0x15]==0x40 && p2_vehicle[0x15]==0x40);
    CHECK(!projected);

    /* A passenger starts the native incoming pose at the owner's landing
     * height, facing right. Preserve its camera index and P1's facing. */
    reset();p2[0x14]=2;p2[0x15]=p2_vehicle[0x15]=0;
    psx_mod_write_byte(PLAYER+0x15,0);
    position_return(&cpu,1,320u<<16,144u<<16,1,1,1,1,0);
    start_arrival(&cpu,1);
    CHECK(body_word(p2,12)==144u<<16 && body_word(p2,0x1C)==144u<<16);
    CHECK(p2[0x14]==2 && p2[0x15]==0x40 && p2_vehicle[0x15]==0x40);
    CHECK(!psx_mod_read_byte(PLAYER+0x15) && !projected);
    /* Opposite characters share the surface, not an identical body origin.
     * The native box centres differ by one pixel in either roster. */
    for(unsigned character=0;character<2;++character) {
        reset();p2[2]=(uint8_t)(character^1u);
        psx_mod_write_byte(PLAYER+2,(uint8_t)character);
        psx_mod_write_byte(0x800F8BC4u+1,2);psx_mod_write_byte(0x800F8BC4u+3,19);
        psx_mod_write_byte(0x800F8BC8u+1,3);psx_mod_write_byte(0x800F8BC8u+3,19);
        uint32_t box=character?0x800F8BC8u:0x800F8BC4u;
        position_return(&cpu,1,320u<<16,(171u-character)<<16,1,1,0,0,box);
        CHECK(body_word(p2,12)==(170u+character)<<16);
        CHECK(body_word(p2,0x1C)==body_word(p2,12));
        CHECK(psx_mod_read_byte(PLAYER+2)==character && !projected);
    }

    /* A real checkpoint transport can overtake a still-incoming passenger.
     * The global loader requires canonical state3 even when that P1 pose was
     * already hidden by the scene lock. Retain its cached return resources. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned phase=1;phase<=2;++phase)
    for(unsigned dead=0;dead<2;++dead) {
        reset();script_active=1;script_owner=warp_owner=owner;warp_phase=phase;
        warp_active=warp_visible=1;warp_origin_x=123u<<16;warp_origin_y=456u<<16;
        unsigned passenger=owner^1u;
        if(owner)p2[4]=3;else psx_mod_write_byte(PLAYER+4,3);
        if(passenger) {p2[0]=p2[3]=0;p2[4]=(uint8_t)(dead?2:1);p2[5]=p2[6]=0;}
        else {psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
            psx_mod_write_byte(PLAYER+4,(uint8_t)(dead?2:1));
            psx_mod_write_byte(PLAYER+5,0);psx_mod_write_byte(PLAYER+6,0);}
        psx_mod_write_byte(PLAY+0x0F,64);psx_mod_write_byte(PLAY+0x1C,1);
        warp_tick(&cpu);CHECK(warp_phase==(dead?phase:3u) && !projected);
        CHECK(warp_active==1 && warp_visible==1 && warp_origin_x==123u<<16 && warp_origin_y==456u<<16);
        if(!dead) {
            warp_tick(&cpu);warp_tick(&cpu);
            CHECK(warp_phase==1 && psx_mod_read_byte(PLAYER+4)==3 && p2[4]==3);
            CHECK((passenger?p2[0x5C]:psx_mod_read_byte(PLAYER+0x5C))==(passenger?12:16));
        }
    }

    /* A winner's state3 is a completed departure, not a dead owner to rescue.
     * Finish either passenger's outgoing beam during completion, so native
     * stage termination sees canonical state3; keep the passenger outside. */
    for(unsigned owner=0;owner<2;++owner) {
        reset();script_active=1;script_owner=owner;request_script_departure(owner);
        mmx4_coop_lifecycle_tick(&cpu);CHECK(warp_phase==3);
        psx_mod_write_byte(PLAY+0x0F,1);
        if(owner)p2[4]=3;else psx_mod_write_byte(PLAYER+4,3);
        mmx4_coop_lifecycle_tick(&cpu);CHECK(warp_phase==1);
        CHECK(psx_mod_read_byte(PLAYER+4)==3);
        CHECK(owner==0 || !psx_mod_read_byte(PLAYER));
        CHECK(owner==1 || !p2[0]);
        CHECK(p2[4]==3);
        mmx4_coop_lifecycle_tick(&cpu);CHECK(warp_phase==1);
    }

    /* Native room fades and victory actions cannot reopen the passenger. */
    for(unsigned gate=0;gate<3;++gate) {
        reset();warp_owner=0;warp_phase=1;warp_active=warp_visible=1;
        p2[0]=p2[3]=0;psx_mod_write_byte(PLAYER+0x89,8);
        if(gate==0)psx_mod_write_byte(PLAY+1,1);
        if(gate==1)psx_mod_write_byte(PLAY+0x0F,1);
        if(gate==2)psx_mod_write_byte(PLAYER+5,0x14);
        for(unsigned i=0;i<6;++i)mmx4_coop_lifecycle_tick(&cpu);
        CHECK(warp_phase==1 && !p2[0] && !p2[3]);
        psx_mod_write_byte(PLAY+1,0);psx_mod_write_byte(PLAY+0x0F,0);
        psx_mod_write_byte(PLAYER+5,2);psx_mod_write_word(PLAYER+12,144u<<16);
        for(unsigned i=0;i<3;++i)mmx4_coop_lifecycle_tick(&cpu);
        CHECK(warp_phase==2 && body_word(p2,12)==144u<<16);
        CHECK(p2[0x70]==8 && p2[0x89]==8); /* Grounded return must not take another descent step. */
        mmx4_coop_lifecycle_tick(&cpu);CHECK(!warp_phase && mmx4_coop_alive(1));
    }

    /* The leader's feet may fit where the other hero's standing box is
     * crushed. Native terrain must admit a nearby same-height landing;
     * otherwise keep the passenger hidden until a safe location exists. */
    for(unsigned owner=0;owner<2;++owner) {
        reset();warp_owner=owner;warp_phase=1;warp_active=warp_visible=1;
        unsigned passenger=owner^1u;
        if(passenger) {p2[0]=p2[3]=0;}
        else {psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);}
        if(owner)mmx4_coop_enter_second();
        psx_mod_write_word(PLAYER+8,640u<<16);psx_mod_write_word(PLAYER+12,180u<<16);
        psx_mod_write_byte(PLAYER+0x89,8);
        if(owner)mmx4_coop_leave_second();
        psx_mod_write_half(0x801419D6u,512);psx_mod_write_half(0x801419D4u,512);
        terrain_contact=12;narrow_return_x=640;
        for(unsigned i=0;i<3;++i)warp_tick(&cpu);
        CHECK(warp_phase==1 && !incoming_calls[passenger] && !projected);
        CHECK(passenger?!p2[0]:!psx_mod_read_byte(PLAYER));
        CHECK((passenger?body_word(p2,12):psx_mod_read_word(PLAYER+12))==180u<<16);
        CHECK((passenger?p2[0x5C]:psx_mod_read_byte(PLAYER+0x5C))==(passenger?12:16));
        terrain_contact=8;
        for(unsigned i=0;i<3;++i)warp_tick(&cpu);
        CHECK(warp_phase==2 && incoming_calls[passenger]==1 && !projected);
        CHECK((passenger?body_word(p2,8):psx_mod_read_word(PLAYER+8))==648u<<16);
        CHECK((passenger?body_word(p2,12):psx_mod_read_word(PLAYER+12))==180u<<16);
        warp_tick(&cpu);CHECK(!warp_phase && mmx4_coop_alive(passenger));
    }

    /* A grounded walking/dashing leader releases the arrival for either seat.
     * Native victory actions above remain gated through completion. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned action=4;action<=13;action+=9) {
        reset();warp_owner=owner;warp_phase=1;warp_active=warp_visible=1;
        if(owner) {
            psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
            p2[5]=(uint8_t)action;p2[0x89]=8;
        }else {
            p2[0]=p2[3]=0;psx_mod_write_byte(PLAYER+5,(uint8_t)action);
            psx_mod_write_byte(PLAYER+0x89,8);
        }
        for(unsigned i=0;i<3;++i)mmx4_coop_lifecycle_tick(&cpu);
        CHECK(warp_phase==2);
        mmx4_coop_lifecycle_tick(&cpu);
        CHECK(!warp_phase && mmx4_coop_alive(owner^1u));
    }

    /* A grounded mounted leader releases its on-foot partner at the native
     * Armor's floor. Personal phase1/no foot box are normal while riding. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned reject=0;reject<5;++reject) {
        reset();warp_owner=owner;warp_phase=1;warp_active=warp_visible=1;
        unsigned passenger=owner^1u;
        if(passenger)p2[0]=p2[3]=0;
        else {psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);}
        if(owner)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER+5,18);psx_mod_write_byte(PLAYER+0x67,1);
        psx_mod_write_byte(PLAYER+0xC5,255);psx_mod_write_byte(PLAYER+0x89,0);
        psx_mod_write_word(PLAYER+0x68,0);
        psx_mod_write_byte(MMX4_VEHICLE,0x41);psx_mod_write_byte(MMX4_VEHICLE+1,1);
        psx_mod_write_byte(MMX4_VEHICLE+4,1);psx_mod_write_byte(MMX4_VEHICLE+5,1);
        psx_mod_write_byte(MMX4_VEHICLE+0x97,0x40);psx_mod_write_byte(MMX4_VEHICLE+0xA8,8);
        psx_mod_write_word(MMX4_VEHICLE+8,640u<<16);psx_mod_write_word(MMX4_VEHICLE+12,180u<<16);
        psx_mod_write_word(MMX4_VEHICLE+0x68,0x800F9120u);
        psx_mod_write_byte(0x800F9123u,32);
        if(reject==1)psx_mod_write_byte(MMX4_VEHICLE+0xA8,0);
        if(reject==2)psx_mod_write_byte(MMX4_VEHICLE+1,0);
        if(reject==3)psx_mod_write_byte(MMX4_VEHICLE+0x97,0);
        if(reject==4)psx_mod_write_byte(PLAYER+0xC0,1);
        if(owner)mmx4_coop_leave_second();
        for(unsigned tick=0;tick<3;++tick)warp_tick(&cpu);
        CHECK(!projected && warp_phase==(reject?1u:2u));
        CHECK(incoming_calls[passenger]==!reject);
        if(!reject) {
            CHECK((passenger?body_word(p2,12):psx_mod_read_word(PLAYER+12))==212u<<16);
            CHECK((owner?p2[0xC5]:psx_mod_read_byte(PLAYER+0xC5))==255);
        }
    }

    /* Once grounded arrival is committed, the leader's jump/fall or Nova
     * must not hide the partner and restart the beam. Native +67 is also a
     * personal airborne phase, not evidence of a shared cutscene. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned attack=0;attack<2;++attack) {
        reset();warp_owner=owner;warp_active=warp_visible=1;
        start_arrival(&cpu,owner^1u);
        if(owner)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER+2,0);
        psx_mod_write_byte(PLAYER+5,(uint8_t)(attack?0x22:6));
        psx_mod_write_byte(PLAYER+0x67,(uint8_t)(attack?1:255));
        psx_mod_write_byte(PLAYER+0x89,0);
        if(owner)mmx4_coop_leave_second();
        psx_mod_write_byte(PLAY+0x1C,(uint8_t)attack);
        warp_tick(&cpu);
        CHECK(!warp_phase && mmx4_coop_alive(owner^1u));
        CHECK(psx_mod_read_byte(PLAYER+3) && p2[3] && !projected);
        CHECK(incoming_calls[owner^1u]==1);
        CHECK(psx_mod_read_byte(PLAY+0x1C)==attack);
        CHECK((owner?p2[5]:psx_mod_read_byte(PLAYER+5))==(attack?0x22:6));
    }
    /* Real shared scene locks still postpone an already-started arrival,
     * including when the leader was using Nova as a conversation begins. */
    for(unsigned gate=0;gate<5;++gate) {
        reset();warp_owner=0;start_arrival(&cpu,1);
        if(gate<3)psx_mod_write_byte(PLAYER+(gate==0?0xC0:gate==1?0xC3:0xC4),1);
        if(gate==3)psx_mod_write_byte(PLAY+0x1C,1);
        if(gate==4) {
            psx_mod_write_byte(PLAYER+5,0x22);psx_mod_write_byte(PLAY+0x1C,1);script_active=1;
        }
        warp_tick(&cpu);CHECK(warp_phase==1 && !p2[0] && !p2[3]);
    }
    /* A personal lock still blocks choosing an uncommitted landing point. */
    reset();warp_owner=0;warp_phase=1;warp_active=warp_visible=1;p2[0]=p2[3]=0;
    psx_mod_write_byte(PLAYER+0x89,8);psx_mod_write_byte(PLAYER+0x67,1);
    for(unsigned i=0;i<4;++i)warp_tick(&cpu);
    CHECK(warp_phase==1 && !p2[0] && !incoming_calls[1]);

    /* A death while the passenger is hidden must restore both the departure
     * position and a playable action. Keeping the outgoing action0/1 after
     * clearing warp_phase strands a living survivor in its beam dispatcher. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned phase=1;phase<=3;++phase) {
        reset();warp_owner=owner;warp_phase=phase;warp_active=warp_visible=1;
        warp_origin_x=640u<<16;warp_origin_y=180u<<16;
        unsigned passenger=owner^1u;
        if(owner)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER+0x5C,0x80);psx_mod_write_byte(PLAYER+4,2);
        if(owner)mmx4_coop_leave_second();
        if(passenger)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
        psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+5,0);
        if(passenger)mmx4_coop_leave_second();
        warp_tick(&cpu);
        CHECK(!warp_phase && !projected && mmx4_coop_alive(passenger));
        CHECK((passenger?p2[5]:psx_mod_read_byte(PLAYER+5))==2);
        CHECK((passenger?body_word(p2,8):psx_mod_read_word(PLAYER+8))==640u<<16);
        CHECK((passenger?body_word(p2,12):psx_mod_read_word(PLAYER+12))==180u<<16);
        CHECK(!mmx4_coop_alive(owner) && psx_mod_read_byte(PLAY+0x44)==2);
    }

    /* A genuine fatal hit on the passenger still belongs to native death;
     * origin recovery must not turn a corpse into an idle visible body. */
    reset();warp_phase=1;warp_owner=0;warp_active=warp_visible=1;
    psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+4,2);
    p2[0x5C]=0x80;p2[4]=2;p2[5]=1;
    warp_tick(&cpu);CHECK(!warp_phase && p2[4]==2 && p2[5]==1 && !mmx4_coop_alive(1));

    /* A changed departure area must pass native terrain admission before
     * the survivor becomes visible, including the wider opposite hero. */
    for(unsigned owner=0;owner<2;++owner) {
        reset();warp_owner=owner;warp_phase=1;warp_active=warp_visible=1;
        warp_origin_x=640u<<16;warp_origin_y=180u<<16;
        unsigned passenger=owner^1u;
        if(owner)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+4,2);
        if(owner)mmx4_coop_leave_second();
        terrain_contact=12;warp_tick(&cpu);
        CHECK(warp_phase==1 && !mmx4_coop_alive(passenger) && !projected);
        CHECK(passenger?!p2[3]:!psx_mod_read_byte(PLAYER+3));
        terrain_contact=8;warp_tick(&cpu);
        CHECK(!warp_phase && mmx4_coop_alive(passenger));
        CHECK((passenger?p2[5]:psx_mod_read_byte(PLAYER+5))==2);
        CHECK(!mmx4_coop_alive(owner) && psx_mod_read_byte(PLAY+0x44)==2);
    }

    /* Death during a committed arrival keeps the survivor in the new
     * room, rather than restoring the pre-door departure coordinates. */
    reset();warp_owner=0;warp_phase=1;warp_active=warp_visible=1;
    warp_origin_x=40u<<16;warp_origin_y=20u<<16;p2[0]=p2[3]=0;
    psx_mod_write_word(PLAYER+8,640u<<16);psx_mod_write_word(PLAYER+12,180u<<16);
    psx_mod_write_byte(PLAYER+0x89,8);
    for(unsigned i=0;i<3;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(warp_phase==2);
    psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+4,2);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!warp_phase && mmx4_coop_alive(1));
    CHECK(body_word(p2,8)==640u<<16 && body_word(p2,12)==180u<<16);

    /* A native door can lose its initiator during auto-walk. Transfer its
     * lock to the living partner and leave the corpse to native death. */
    for(unsigned dead=0;dead<2;++dead) {
        reset();script_active=1;script_owner=dead;script_serial=5;
        scenes[0]=(SceneOwner){actor,5,(uint8_t)dead};
        if(dead) {
            p2[0x5C]=0x80;p2[5]=0x13;p2[0xC3]=p2[0xC4]=0xFF;
            p2[10]=0x80;p2[11]=2;p2[14]=144;
        }else {
            psx_mod_write_byte(PLAYER+0x5C,0x80);psx_mod_write_byte(PLAYER+5,0x13);
            psx_mod_write_byte(PLAYER+0xC3,0xFF);psx_mod_write_byte(PLAYER+0xC4,0xFF);
            psx_mod_write_word(PLAYER+8,640u<<16);psx_mod_write_word(PLAYER+12,144u<<16);
        }
        mmx4_coop_lifecycle_tick(&cpu);
        CHECK(script_active && script_owner==(dead^1u));
        CHECK(scenes[0].owner==(dead^1u) && scenes[0].recovered);
        CHECK((dead?p2[4]:psx_mod_read_byte(PLAYER+4))==2);
        CHECK(!(dead?p2[0xC4]:psx_mod_read_byte(PLAYER+0xC4)));
        CHECK((dead?psx_mod_read_byte(PLAYER+0xC4):p2[0xC4])==1);
        CHECK(mmx4_coop_alive(dead^1u) && !mmx4_coop_alive(dead));
        CHECK((dead?psx_mod_read_word(PLAYER+8):body_word(p2,8))==640u<<16);
        CHECK((dead?psx_mod_read_word(PLAYER+12):body_word(p2,12))==144u<<16);
    }

    reset();script_active=1;script_owner=0;cpu.gpr[4]=PLAYER;
    psx_mod_write_byte(PLAYER+0xC4,1);
    scenes[0]=(SceneOwner){actor,1,0,0};
    CHECK(!hidden_terrain(&cpu,0x8002C614u)); /* Native door, not a recovery. */
    scenes[0].recovered=1;psx_mod_write_byte(PLAYER+0x70,12);
    psx_mod_write_byte(PLAYER+0x71,3);
    CHECK(hidden_terrain(&cpu,0x8002C614u));
    CHECK(psx_mod_read_byte(PLAYER+0x70)==8 && !psx_mod_read_byte(PLAYER+0x71));
    psx_mod_write_byte(PLAYER+0xC4,0);
    CHECK(!hidden_terrain(&cpu,0x8002C614u));

    /* Personal C4 doors do not acquire PLAY1C; general C0 scenes do.
     * A partner's native death must preserve either preexisting gate. */
    for(unsigned gate=0;gate<2;++gate) {
        reset();script_active=1;script_owner=0;
        psx_mod_write_byte(PLAY+0x1C,(uint8_t)gate);
        mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x80035A6Cu,PLAYER,0);
        mmx4_coop_leave_second();
        CHECK(psx_mod_read_byte(PLAY+0x1C)==gate);
    }

    /* Marine Base prohibits withdrawal for the whole bike sequence, even
     * during frames where native mounting fields temporarily clear. */
    reset();inputs[1]=SELECT;psx_mod_write_byte(PLAYER+0xC5,0xFF);
    psx_mod_write_byte(PLAY+0x0C,5);
    psx_mod_write_byte(PLAYER+0x89,0);p2[0xC5]=0xFF;
    p2_vehicle[0]=p2_vehicle[3]=1;p2_vehicle[0x5C]=23;
    psx_mod_write_byte(MMX4_VEHICLE,1);psx_mod_write_byte(MMX4_VEHICLE+3,1);
    psx_mod_write_byte(MMX4_VEHICLE+0x5C,31);
    for(unsigned i=0;i<90;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!departed && p2_vehicle[0]==1 && psx_mod_read_byte(MMX4_VEHICLE)==1);
    p2[0xC5]=0;psx_mod_write_byte(PLAYER+0xC5,0);
    for(unsigned i=0;i<100;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!departed && mmx4_coop_alive(1) && p2_vehicle[0x5C]==23);
    psx_mod_write_byte(PLAY+0x1D,1);
    for(unsigned i=0;i<90;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(departed); /* Foot control after the bike sequence remains eligible. */

    /* Personal native control locks cannot be escaped by Select. A queued
     * voluntary return also waits until fades/victory actually finish. */
    static const unsigned lock_offsets[]={0xC0,0xC3,0xC4,0x67,0xBC};
    for(unsigned k=0;k<sizeof lock_offsets/sizeof lock_offsets[0];++k) {
        reset();inputs[1]=SELECT;p2[lock_offsets[k]]=1;
        for(unsigned i=0;i<100;++i)mmx4_coop_lifecycle_tick(&cpu);
        CHECK(!departed && !select_ticks && mmx4_coop_alive(1));
    }
    reset();departed=rejoin_pending=1;psx_mod_write_byte(PLAYER+0x89,8);
    psx_mod_write_byte(PLAYER+5,0x13);
    for(unsigned i=0;i<4;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(departed && rejoin_pending && !warp_phase);
    psx_mod_write_byte(PLAYER+5,2);psx_mod_write_byte(PLAY+0x0F,1);
    mmx4_coop_lifecycle_tick(&cpu);CHECK(departed && !warp_phase);
    psx_mod_write_byte(PLAY+0x0F,0);mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!departed && warp_phase==2);

    reset();psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    inputs[1]=SELECT;for(unsigned i=0;i<100;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!departed && mmx4_coop_alive(1));

    /* Death during the incoming pose must leave the returning survivor
     * playable, rather than treating the temporary control lock as death. */
    reset();inputs[1]=SELECT;
    for(unsigned i=0;i<90;++i)mmx4_coop_lifecycle_tick(&cpu);
    inputs[1]=0;mmx4_coop_lifecycle_tick(&cpu);
    psx_mod_write_byte(PLAYER+0x89,8);inputs[1]=SELECT;
    mmx4_coop_lifecycle_tick(&cpu);CHECK(warp_phase==2);
    psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_call(&cpu,0x8001FF8Cu,0,0);
    CHECK(!warp_phase && mmx4_coop_alive(1) && p2[0x5C]==12 && world_calls==1);

    /* The Mushroom lift's initial native gate must use the sole survivor.
     * It must not revive P1, change campaign ownership, or update twice. */
    const uint32_t survivor_lift=0x80165A30u;
    reset();psx_mod_write_byte(MMX4_PLAY+12,3);psx_mod_write_byte(MMX4_PLAY+13,1);
    psx_mod_write_byte(survivor_lift,1);psx_mod_write_byte(survivor_lift+1,14);psx_mod_write_byte(survivor_lift+4,1);
    psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+5,0);
    psx_mod_write_byte(PLAYER+0x5C,0);p2[5]=2;
    scene_calls=0;cpu.gpr[4]=survivor_lift;
    CHECK(mmx4_coop_lifecycle_lift_start(&cpu,0x800C3880u));
    CHECK(scene_calls==1 && psx_mod_read_byte(survivor_lift+6)==1 && !projected);
    CHECK(!psx_mod_read_byte(PLAYER+5) && !psx_mod_read_byte(PLAYER+0x5C));
    CHECK(p2[5]==2 && p2[0x5C]==12 && observed_campaign==0);
    CHECK(!script_active && !warp_pending && !warp_phase);
    /* A corpse in another stage is not a Mushroom lift admission. */
    psx_mod_write_byte(survivor_lift+6,0);psx_mod_write_byte(MMX4_PLAY+12,1);
    CHECK(!mmx4_coop_lifecycle_lift_start(&cpu,0x800C3880u));
    CHECK(!psx_mod_read_byte(survivor_lift+6));

    /* Sigma's corridor admission and later scene completion belong to the
     * living entrant, preserving the corpse and the campaign identity. */
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();psx_mod_write_byte(PLAY+12,12);psx_mod_write_byte(PLAY+0x43,campaign);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
        psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
        psx_mod_write_word(PLAYER+8,1840u<<16);put32(p2+8,2356u<<16);
        cpu.gpr[4]=actor;scene_calls=0;
        mmx4_coop_call(&cpu,0x800D2A38u,actor,0);
        CHECK(scene_calls==1 && script_active && script_owner==1 && p2[0xC0]==1);
        CHECK(!psx_mod_read_byte(PLAYER+0xC0) && !psx_mod_read_byte(PLAYER+0x5C));
        CHECK(psx_mod_read_word(PLAYER+8)==1840u<<16 && observed_campaign==campaign);
        CHECK(psx_mod_read_byte(PLAY+0x43)==campaign && !projected);
        p2[0xC0]=0xFF;cpu.gpr[4]=actor;
        mmx4_coop_call(&cpu,0x800D2A38u,actor,0);
        CHECK(!script_active && !p2[0xC0] && !projected);
        CHECK(psx_mod_read_byte(PLAYER+4)==3 && !psx_mod_read_byte(PLAYER+0x5C));
    }
    reset();psx_mod_write_byte(PLAY+12,12);psx_mod_write_byte(actor,1);
    psx_mod_write_byte(actor+4,1);put32(p2+8,2356u<<16);cpu.gpr[4]=actor;
    mmx4_coop_call(&cpu,0x800D2A38u,actor,0);
    CHECK(!script_active); /* A living P1 retains the original admission. */
    psx_mod_write_word(PLAYER+8,2321u<<16);cpu.gpr[4]=actor;
    mmx4_coop_call(&cpu,0x800D2A38u,actor,0);CHECK(script_active && !script_owner);
    reset();cpu.gpr[4]=actor;
    CHECK(!scene_actor(&cpu,0x800D2A38u)); /* Other stages stay native. */
    psx_mod_write_byte(PLAY+12,12);psx_mod_write_byte(PLAY+13,1);
    CHECK(!scene_actor(&cpu,0x800D2A38u)); /* Later Sigma sections stay native. */
    psx_mod_write_byte(PLAY+13,0);local_view=1;
    CHECK(!scene_actor(&cpu,0x800D2A38u));local_view=0;

    /* P2 may trigger a native portal first. Its scene sees the original
     * campaign, while only P2 receives native body/action writes. */
    reset();psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+8,0x02000000u);psx_mod_write_word(actor+12,0x03000000u);
    p2_solid_bits=8;mmx4_coop_call(&cpu,0x800C2BE0u,actor,0);
    CHECK(canonical_portal_solids==1 && !projected && !projected_portal_solid);
    CHECK(scene_calls==1 && observed_campaign==0 && script_owner==1 && script_active);
    CHECK(!psx_mod_read_byte(actor+0x72) && p2[0xC0]==1);
    CHECK(!psx_mod_read_byte(PLAYER+0xC0) && p2[0x5C]==12);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(warp_phase==3 && mmx4_coop_lifecycle_hidden(0));
    mmx4_coop_lifecycle_tick(&cpu); /* Native departure finishes before hiding. */
    CHECK(!psx_mod_read_byte(PLAYER) && mmx4_coop_lifecycle_hidden(0));
    CHECK(psx_mod_read_word(PLAYER+8)==0);
    CHECK(psx_mod_read_byte(PLAYER+0x5C)==16);
    psx_mod_write_half(PLAYER+0x7C,0x2000);
    mmx4_coop_call(&cpu,0x800311ECu,0,0);CHECK(!observed_controls);
    mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x80036B18u,0,0);mmx4_coop_leave_second();
    CHECK(!script_active && !p2[0xC0] && psx_mod_read_byte(PLAY+0x43)==0);
    /* Chained dialogue holds the passenger outside until actual landing. */
    psx_mod_write_byte(PLAY+0x1C,1);p2[0x89]=8;
    mmx4_coop_lifecycle_tick(&cpu);CHECK(!psx_mod_read_byte(PLAYER));
    psx_mod_write_byte(PLAY+0x1C,0);mmx4_coop_lifecycle_tick(&cpu);
    mmx4_coop_lifecycle_tick(&cpu);mmx4_coop_lifecycle_tick(&cpu);
    CHECK(psx_mod_read_word(PLAYER+8)==0x02000000u && warp_phase==2);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!warp_phase && mmx4_coop_alive(0) && psx_mod_read_byte(PLAYER+0x5C)==16);

    /* A dead P1 stays dead while P2 advances native checkpoint scripts once. */
    reset();psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_call(&cpu,0x8002166Cu,0,0);
    CHECK(stage_script_calls==1 && observed_campaign==0 && psx_mod_read_byte(PLAY+0x1D)==9);
    CHECK(p2[0xC0]==1 && !psx_mod_read_byte(PLAYER+0xC0));
    CHECK(psx_mod_read_byte(PLAYER+4)==3 && !psx_mod_read_byte(PLAYER+0x5C));
    CHECK(!warp_pending && !projected && psx_mod_read_byte(PLAY+0x43)==0);
    mmx4_coop_call(&cpu,0x80036B18u,0,0);
    CHECK(!p2[0xC0] && !script_active && psx_mod_read_byte(PLAYER+4)==3);

    reset();psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    p2[11]=2;psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    mmx4_coop_call(&cpu,0x800BD654u,actor,0);
    CHECK(scene_calls==1 && observed_campaign==0 && psx_mod_read_byte(PLAY+0x1D)==8);
    CHECK(p2[0x5C]==12 && psx_mod_read_byte(PLAYER+4)==3 && !projected);

    /* Dragoon's act1 actor must observe completion from the body it sent
     * walking, even with a healthy trailing P1 in the other campaign. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned owner=0;owner<2;++owner) {
        reset();psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);
        second_inventory.character=p2[2]=(uint8_t)(campaign^1u);
        script_active=1;script_owner=owner;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
        mmx4_coop_call(&cpu,0x800D0374u,actor,0);
        CHECK(psx_mod_read_byte(actor+5)==1 && script_owner==owner);
        CHECK(observed_campaign==campaign && !projected);
        CHECK(psx_mod_read_byte(PLAYER+0xC0)==(owner?0:1) && p2[0xC0]==(owner?1:0));
        if(owner)p2[0xC0]=255;else psx_mod_write_byte(PLAYER+0xC0,255);
        mmx4_coop_call(&cpu,0x800D0374u,actor,0);
        CHECK(psx_mod_read_byte(actor+5)==2 && observed_campaign==campaign);
        CHECK(!projected && psx_mod_read_byte(PLAY+0x43)==campaign);
        CHECK((owner?psx_mod_read_byte(PLAYER+0xC0):p2[0xC0])==0);
    }

    /* Rescue actors use the committed winner, including a surviving P2.
     * A living but lagging P1 must not steal the scene either. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned scenario=0;scenario<3;++scenario) {
        reset();unsigned owner=scenario!=0;
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);
        second_inventory.character=p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_word(PLAYER+8,3936u<<16);
        mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,4544u<<16);mmx4_coop_leave_second();
        if(scenario==2) {psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
        script_active=1;script_owner=owner;
        uint32_t expected=owner?4544u:3936u;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,0);
        mmx4_coop_call(&cpu,0x800D1248u,actor,0);
        CHECK(psx_mod_read_word(actor+8)==(expected+352u)*65536u);
        psx_mod_write_byte(actor+4,0);
        mmx4_coop_call(&cpu,0x800D1284u,actor,0);
        CHECK(psx_mod_read_half(0x801419D4u)==expected-48u);
        psx_mod_write_half(0x801419BAu,(uint16_t)(expected-48u));
        mmx4_coop_call(&cpu,0x800D1284u,actor,0);
        CHECK(psx_mod_read_byte(actor+5)==1 && observed_campaign==campaign);
        CHECK(!projected && psx_mod_read_byte(PLAY+0x43)==campaign);
        CHECK(psx_mod_read_word(PLAYER+8)==3936u*65536u);
        if(scenario==2)CHECK(psx_mod_read_byte(PLAYER+4)==3 && !psx_mod_read_byte(PLAYER+0x5C));
    }

    /* Waterfall exit completion belongs to the body sent on the outgoing
     * walk. A dead P1 must not block it, and a subsequently leading partner
     * must not steal an already committed exit. */
    for(unsigned owner=0;owner<2;++owner) {
        reset();psx_mod_write_byte(actor,0x41);psx_mod_write_byte(actor+4,3);
        psx_mod_write_word(PLAYER+8,(owner?5039u:6320u)<<16);
        mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,(owner?6320u:5039u)<<16);mmx4_coop_leave_second();
        if(owner) {psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
        mmx4_coop_call(&cpu,0x800C165Cu,actor,0);
        CHECK(script_active && script_owner==owner && psx_mod_read_byte(actor+4)==4);
        mmx4_coop_call(&cpu,0x800C169Cu,actor,0);
        CHECK(!psx_mod_read_byte(PLAY+0x0F) && psx_mod_read_byte(actor));
        if(!owner)psx_mod_write_word(PLAYER+8,6360u<<16);
        mmx4_coop_enter_second();psx_mod_write_word(PLAYER+8,(owner?6360u:6400u)<<16);mmx4_coop_leave_second();
        mmx4_coop_call(&cpu,0x800C169Cu,actor,0);
        CHECK(psx_mod_read_byte(PLAY+0x0F)==0x40 && !psx_mod_read_byte(actor));
        CHECK(!projected && psx_mod_read_byte(PLAY+0x43)==0);
        if(owner)CHECK(psx_mod_read_byte(PLAYER+4)==3 && !psx_mod_read_byte(PLAYER+0x5C));
    }

    /* Boss doors use C4 directly, not the general C0 script command pair.
     * Independent P2 entry must serialize the native auto-walk once. */
    reset();psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+8,0x02000000u);p2[11]=2;
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(scene_calls==1 && script_active && script_owner==1 && p2[0xC4]==1);
    CHECK(mmx4_coop_lifecycle_script_owner()==1);
    CHECK(!psx_mod_read_byte(PLAYER+0xC4) && observed_campaign==0);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(warp_phase==3 && mmx4_coop_lifecycle_hidden(0));
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!psx_mod_read_byte(PLAYER) && psx_mod_read_word(PLAYER+8)==0);
    psx_mod_write_byte(actor+5,3);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(scene_calls==2 && !script_active && !p2[0xC4]);
    CHECK(mmx4_coop_lifecycle_script_owner()==-1);
    p2[0x89]=8;for(unsigned i=0;i<4;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(psx_mod_read_word(PLAYER+8)==0x02010000u);
    CHECK(!scene_record(actor,0));

    /* Both-player admission retains the idle native pass. Contact must be
     * simultaneous, not remembered after the first player walks away. */
    actor=0x80165A30u;
    for(unsigned leader=0;leader<2;++leader) {
        reset();both_door_players=1;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,8);psx_mod_write_byte(actor+4,1);
        if(leader)p2[11]=2;else psx_mod_write_word(PLAYER+8,0x02000000u);
        psx_mod_write_word(actor+8,0x02000000u);
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK(scene_calls==1 && !script_active && !p2[0xC4] && !psx_mod_read_byte(PLAYER+0xC4));
        CHECK(!door_wait && !psx_mod_read_byte(actor+5));
        p2[11]=2;psx_mod_write_word(PLAYER+8,0x02000000u);
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK(scene_calls==2 && script_active && !script_owner && psx_mod_read_byte(PLAYER+0xC4));
        CHECK(!door_wait && !projected);
    }
    /* Mounted contact queries borrow the foot box without ejecting a rider.
     * A remote or missing partner cannot commit a Both door; once both touch,
     * only the mounted hero dismounts and native admission starts on foot. */
    for(unsigned rider=0;rider<2;++rider) {
        reset();both_door_players=1;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,8);psx_mod_write_byte(actor+4,1);
        psx_mod_write_word(actor+8,0x02000000u);
        if(rider)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER+0xC5,255);psx_mod_write_word(PLAYER+0x68,0);
        psx_mod_write_byte(MMX4_VEHICLE,0x41);psx_mod_write_byte(MMX4_VEHICLE+1,1);
        psx_mod_write_byte(MMX4_VEHICLE+0x97,0x40);
        if(rider)mmx4_coop_leave_second();
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK((rider?p2[0xC5]:psx_mod_read_byte(PLAYER+0xC5))==255 && !script_active);
        if(rider)p2[11]=2;else psx_mod_write_word(PLAYER+8,0x02000000u);
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK((rider?p2[0xC5]:psx_mod_read_byte(PLAYER+0xC5))==255 && !script_active);
        CHECK((rider?body_word(p2,0x68):psx_mod_read_word(PLAYER+0x68))==0);
        p2[11]=2;psx_mod_write_word(PLAYER+8,0x02000000u);
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK(!(rider?p2[0xC5]:psx_mod_read_byte(PLAYER+0xC5)) && script_active && !projected);
    }
    /* Passenger transport must leave an ejected world armor at its original
     * position, including when the passenger is the campaign owner. */
    reset();
    psx_mod_write_byte(PLAYER+0xC5,255);
    psx_mod_write_byte(MMX4_VEHICLE,0x41);psx_mod_write_byte(MMX4_VEHICLE+1,1);
    psx_mod_write_byte(MMX4_VEHICLE+3,1);psx_mod_write_byte(MMX4_VEHICLE+0x97,0x40);
    psx_mod_write_word(MMX4_VEHICLE+8,0x12340000u);
    warp_pending=1;warp_owner=1;warp_begin(&cpu);
    CHECK(!psx_mod_read_byte(PLAYER+0xC5) && !warp_vehicle_active);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+3));
    position_return(&cpu,0,0x05000000u,0x03000000u,0,0,0,0,0);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_word(MMX4_VEHICLE+8)==0x12340000u);
    psx_mod_write_byte(MMX4_VEHICLE+1,0); /* Unmounted Chasers retain native clearing. */
    position_return(&cpu,0,0x05000000u,0x03000000u,0,0,0,0,0);
    CHECK(!psx_mod_read_byte(MMX4_VEHICLE) && !psx_mod_read_byte(MMX4_VEHICLE+3));
    for(unsigned absent=0;absent<2;++absent) {
        reset();both_door_players=1;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,8);psx_mod_write_byte(actor+4,1);
        if(absent) {p2[4]=3;p2[0x5C]=0;}
        else {psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK(script_active && script_owner==(absent^1u) && !door_wait);
        CHECK(!both_door.phase);
        if(!absent)mmx4_coop_enter_second();
        mmx4_coop_call(&cpu,0x800311ECu,PLAYER,0);
        if(!absent)mmx4_coop_leave_second();
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        psx_mod_write_byte(actor+0x45,2);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK(!script_active && !warp_phase && !warp_pending && !both_door_crossings);
        CHECK((absent?body_word(p2,8):psx_mod_read_word(PLAYER+8))==0);
        CHECK(!(absent?(p2[0x5C]&127u):(psx_mod_read_byte(PLAYER+0x5C)&127u)));
    }
    reset();both_door_players=1;departed=1;
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(script_active && !script_owner && !door_wait);

    /* Either seat can pause during free passage, retaining the open door
     * through the native menu fade/resume. Opening/intro and global gates
     * still reject Start; stale paired state cannot unlock a later scene. */
    for(unsigned seat=0;seat<2;++seat)for(unsigned phase=1;phase<=5;++phase) {
        CHECK(!open_both_door(&cpu,actor));both_door.phase=(uint8_t)phase;
        inputs[seat]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
        unsigned eligible=phase==2 || phase==3;
        CHECK(menu_active==eligible && (psx_mod_read_byte(PLAY+1)==2)==eligible);
        if(eligible) {
            CHECK(menu_owner==seat && script_active);
            inputs[seat]=0;pause_exit=1;
            mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);pause_exit=0;
            CHECK(!menu_active && !psx_mod_read_byte(PLAY+1));
            CHECK(both_door.phase==phase && script_active && !warp_pending && !warp_phase);
        }
    }
    for(unsigned gate=0;gate<6;++gate) {
        CHECK(!open_both_door(&cpu,actor));inputs[1]=START;
        if(gate<3)psx_mod_write_byte(PLAY+(unsigned[]){0x1C,0x10,0x0F}[gate],1);
        else if(gate==3)both_door.stage^=1u;
        else if(gate==4)psx_mod_write_byte(0x80141BDCu,1);
        else ++script_serial;
        mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);CHECK(!menu_active);
    }

    /* Both mode keeps both native bodies in the world. Opening releases the
     * owner's action19 into ordinary controls; physical box clearance, then
     * camera arrival, own closure. No controller or door pass supplies X. */
    CHECK(!open_both_door(&cpu,actor));
    /* Retail's open doorway remains a wall in the map. Exempt its interior
     * wall samples in both axes, retaining the floor at the bottom edge,
     * adjacent walls, closed phases and other tile classifications. */
    for(unsigned seat=0;seat<2;++seat) {
        if(seat)mmx4_coop_enter_second();
        cpu.gpr[6]=0;cpu.gpr[31]=0x8002CB9Cu;
        unsigned queries=door_map_queries;
        CHECK(!mmx4_coop_call(&cpu,0x8002D7E4u,PLAYER,515));
        CHECK(door_map_queries==queries+1 && psx_mod_read_half(PLAYER+0x6C)==3);
        CHECK(mmx4_coop_call(&cpu,0x8002D7E4u,PLAYER,503)==0x38);
        cpu.gpr[6]=32;CHECK(mmx4_coop_call(&cpu,0x8002D7E4u,PLAYER,512)==0x38);
        cpu.gpr[6]=0;cpu.gpr[31]=0x8002D360u;
        CHECK(!mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512)); /* Interior vertical sample. */
        cpu.gpr[6]=32;
        CHECK(mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512)==0x38); /* Aperture floor. */
        cpu.gpr[6]=0;
        cpu.gpr[31]=0x8002D624u;
        CHECK(!mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512));
        door_map_kind=0x21;CHECK(mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512)==0x21);
        door_map_kind=0x3E;CHECK(mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512)==0x3E);
        door_map_kind=0x3F;CHECK(mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512)==0x3F);
        door_map_kind=0x38;both_door.phase=1;
        CHECK(mmx4_coop_call(&cpu,0x8002D724u,PLAYER,512)==0x38);both_door.phase=2;
        if(seat)mmx4_coop_leave_second();
    }
    cpu.gpr[6]=cpu.gpr[31]=0;
    uint8_t paired_hp[2]={psx_mod_read_byte(PLAYER+0x5C),p2[0x5C]};
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==2 && !both_door_crossings);
    CHECK(psx_mod_read_word(PLAYER+8)==512u<<16 && body_word(p2,8)==512u<<16);
    for(unsigned tick=0;tick<3;++tick)door_control(&cpu,0,0x420);
    CHECK(observed_controls==0x420 && psx_mod_read_byte(PLAYER+5)==0x0D);
    CHECK(psx_mod_read_byte(PLAYER+0xC4)==255 && psx_mod_read_word(PLAYER+8)==536u<<16);
    uint32_t paired_x=psx_mod_read_word(PLAYER+8);
    for(unsigned tick=0;tick<3;++tick)mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==2 && psx_mod_read_word(PLAYER+8)==paired_x);
    CHECK(psx_mod_read_half(0x801419D6u)==128 && !both_door_crossings);
    for(unsigned tick=0;tick<6;++tick)door_control(&cpu,1,0x8020);
    CHECK(observed_controls==0x8020 && p2[5]==8 && body_word(p2,8)==536u<<16);
    CHECK(p2[0xC4]==255 && psx_mod_read_word(PLAYER+0x68) && body_word(p2,0x68));
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==3 && psx_mod_read_byte(actor+5)==3);
    CHECK(psx_mod_read_half(0x801419D6u)==512 && !both_door_crossings);
    psx_mod_write_half(0x801419BAu,511);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==3 && !both_door_crossings);
    psx_mod_write_half(0x801419BAu,609);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==3 && !both_door_crossings);
    psx_mod_write_half(0x801419BAu,560);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door.phase==4 && both_door_crossings==1 && psx_mod_read_byte(actor+5)==4);
    CHECK(!script_active && !psx_mod_read_byte(PLAYER+0xC4) && !p2[0xC4]);
    CHECK(psx_mod_read_word(PLAYER+8)==paired_x && body_word(p2,8)==paired_x);
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);mmx4_coop_lifecycle_tick(&cpu);
    CHECK(both_door_crossings==1 && !warp_pending && !warp_phase);
    CHECK(psx_mod_read_byte(PLAYER)==1 && psx_mod_read_byte(PLAYER+3)==1 && p2[0] && p2[3]);
    CHECK(psx_mod_read_byte(PLAYER+0x5C)==paired_hp[0] && p2[0x5C]==paired_hp[1]);
    CHECK(psx_mod_read_byte(PLAY+0x43)==0 && psx_mod_read_byte(PLAY+0x44)==2);

    /* The next retail boss conversation freezes the shared participants and
     * accepts its owner's text input without starting a passenger beam.
     * Release consumes this privilege; a later boss command works normally. */
    cpu.gpr[31]=0x80049084u;mmx4_coop_call(&cpu,0x80036AE4u,0x14,0x40);
    CHECK(script_active && both_door.phase==5 && !warp_pending && !warp_phase);
    door_control(&cpu,1,0x20);CHECK(!observed_controls && body_word(p2,8)==paired_x);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(psx_mod_read_byte(PLAYER) && p2[0] && !mmx4_coop_lifecycle_hidden(0) && !mmx4_coop_lifecycle_hidden(1));
    mmx4_coop_call(&cpu,0x80036B18u,0,0);
    CHECK(!both_door.phase && !script_active && !warp_pending);
    mmx4_coop_call(&cpu,0x80036AE4u,0x14,0x40);CHECK(warp_pending);
    cpu.gpr[31]=0;

    /* Colonel's WARNING must not cancel an in-progress physical crossing.
     * The native initializer advances its actor and starts action15 once;
     * both projections defer all of that until the door/camera are ready. */
    for(unsigned admission=0;admission<3;++admission)for(unsigned seat=0;seat<2;++seat) {
        CHECK(!open_both_door(&cpu,actor));
        uint32_t colonel=0x8013BED0u;
        static const uint32_t admissions[]={0x80085F8Cu,0x80088D30u,0x80052218u};
        uint32_t entry=admissions[admission];
        if(seat)mmx4_coop_enter_second();
        for(unsigned phase=1;phase<=3;++phase) {
            both_door.phase=(uint8_t)phase;
            CHECK(mmx4_coop_lifecycle_paired_door());
            mmx4_coop_call(&cpu,entry,colonel,0);
            CHECK(!psx_mod_read_byte(colonel+5) && !psx_mod_read_byte(colonel+6) && !warp_pending && !warp_phase);
            CHECK(both_door.phase==phase && !psx_mod_read_byte(PLAYER+0xC0));
        }
        both_door.phase=4;
        CHECK(!mmx4_coop_lifecycle_paired_door());
        mmx4_coop_call(&cpu,entry,colonel,0);
        CHECK(psx_mod_read_byte(colonel+(admission==2?6:5))==1 && psx_mod_read_byte(PLAYER+0xC0));
        CHECK(both_door.phase==5 && script_owner==seat && !warp_pending && !warp_phase);
        if(seat)mmx4_coop_leave_second();
    }
    reset();mmx4_coop_call(&cpu,0x80085F8Cu,0x8013BED0u,0);
    CHECK(psx_mod_read_byte(0x8013BED5u)==1 && warp_pending);
    cpu.gpr[31]=0;

    /* Intro act1 must not start its camera-relative auto-walk while the
     * paired door still owns the old scrolling minimum. */
    for(unsigned phase=1;phase<=4;++phase) {
        CHECK(!open_both_door(&cpu,actor));
        both_door.stage=0;both_door.section=0;
        psx_mod_write_byte(PLAY+12,0);psx_mod_write_byte(PLAY+13,0);
        both_door.phase=(uint8_t)phase;cpu.gpr[2]=cpu.gpr[3]=4848;
        paired_intro_camera(&cpu,0x800D0580u);
        CHECK((cpu.gpr[3]==4848)==(phase==4));
        ++script_serial;cpu.gpr[3]=4848;paired_intro_camera(&cpu,0x800D0580u);
        CHECK(cpu.gpr[3]==4848);
    }
    reset();cpu.gpr[2]=cpu.gpr[3]=4848;paired_intro_camera(&cpu,0x800D0580u);
    CHECK(cpu.gpr[3]==4848);

    /* Iris's camera wait accepts the completed crossing anywhere inside her
     * room, but cannot start her script while either body is in the doorway. */
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(!open_both_door(&cpu,actor));
        both_door.stage=11;psx_mod_write_byte(PLAY+0x0C,11);
        psx_mod_write_half(0x801419D6u,2032);psx_mod_write_half(0x801419D4u,2096);
        if(seat)mmx4_coop_enter_second();
        for(unsigned phase=1;phase<=4;++phase)for(unsigned x=2031;x<=2097;++x) {
            both_door.phase=(uint8_t)phase;cpu.gpr[2]=2032;cpu.gpr[3]=x;
            paired_iris_camera(&cpu,0x80081250u);
            CHECK((cpu.gpr[3]==2032)==(phase==4 && x>=2032 && x<=2096));
        }
        both_door.phase=4;cpu.gpr[3]=2096;psx_mod_write_half(0x801419D6u,2048);
        paired_iris_camera(&cpu,0x80081250u);CHECK(cpu.gpr[3]==2032);
        cpu.gpr[3]=2096;psx_mod_write_half(0x801419D4u,2100);
        paired_iris_camera(&cpu,0x80081250u);CHECK(cpu.gpr[3]==2096);
        psx_mod_write_half(0x801419D4u,2096);++script_serial;
        paired_iris_camera(&cpu,0x80081250u);CHECK(cpu.gpr[3]==2096);
        if(seat)mmx4_coop_leave_second();
    }
    reset();cpu.gpr[2]=2032;cpu.gpr[3]=2096;
    paired_iris_camera(&cpu,0x80081250u);CHECK(cpu.gpr[3]==2096);

    /* Death exempts the missing player. Even a dead door owner transfers its
     * gate to the survivor's own position, without the Either-mode rescue. */
    CHECK(!open_both_door(&cpu,actor));
    psx_mod_write_word(PLAYER+8,536u<<16);
    psx_mod_write_byte(PLAYER+0x5C,0x80);mmx4_coop_lifecycle_tick(&cpu);
    CHECK(both_door.owner==1 && script_owner==1 && !scenes[0].recovered);
    CHECK(body_word(p2,8)==512u<<16 && p2[0xC4]==255 && !psx_mod_read_byte(PLAYER+0xC4));
    for(unsigned tick=0;tick<3;++tick)door_control(&cpu,1,0x420);
    psx_mod_write_half(0x801419BAu,512);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(both_door_crossings==1 && !warp_phase && !warp_pending && !p2[0xC4]);
    CHECK(psx_mod_read_word(PLAYER+8)==536u<<16 && body_word(p2,8)==536u<<16);
    CHECK(!(psx_mod_read_byte(PLAYER+0x5C)&127u));
    for(unsigned absent=0;absent<2;++absent) {
        CHECK(!open_both_door(&cpu,actor));
        for(unsigned tick=0;tick<3;++tick)door_control(&cpu,0,0x420);
        if(absent)departed=1;else {p2[4]=3;p2[0x5C]=0;}
        psx_mod_write_half(0x801419BAu,512);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
        CHECK(both_door_crossings==1 && !warp_pending && !warp_phase);
        CHECK(body_word(p2,8)==512u<<16 && psx_mod_read_word(PLAYER+8)==536u<<16);
    }

    /* Rollback owns this entire protocol. Invalid snapshots do not mutate it;
     * aborts, loaders and unrelated scenes cannot leave a warp exemption. */
    CHECK(!open_both_door(&cpu,actor));
    uint8_t paired_snapshot[4096];BothDoor saved_pair=both_door;
    uint32_t paired_digest=mmx4_coop_lifecycle_digest(2166136261u);
    Mmx4SnapshotIO pair_io={paired_snapshot,NULL,sizeof paired_snapshot,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_lifecycle_snapshot(&pair_io));
    uint32_t pair_bytes=pair_io.pos;
    memset(&both_door,0,sizeof both_door);
    pair_io=(Mmx4SnapshotIO){NULL,paired_snapshot,pair_bytes,0,MMX4_SNAPSHOT_READ,1};
    CHECK(mmx4_coop_lifecycle_snapshot(&pair_io));
    CHECK(!memcmp(&saved_pair,&both_door,sizeof saved_pair));
    CHECK(mmx4_coop_lifecycle_digest(2166136261u)==paired_digest);
    uint32_t door_end=pair_bytes-32u*(1u+0x30u)-sizeof both_door.bounds;
    paired_snapshot[door_end-4]=6;
    pair_io=(Mmx4SnapshotIO){NULL,paired_snapshot,pair_bytes,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_lifecycle_snapshot(&pair_io) && both_door.phase==2);
    paired_snapshot[door_end-4]=2;paired_snapshot[door_end-18]^=1;
    pair_io=(Mmx4SnapshotIO){NULL,paired_snapshot,pair_bytes,0,MMX4_SNAPSHOT_VALIDATE,1};
    CHECK(!mmx4_coop_lifecycle_snapshot(&pair_io) && both_door.phase==2);
    psx_mod_write_byte(actor,0);mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!both_door.phase && !script_active && !psx_mod_read_byte(PLAYER+0xC4) && !p2[0xC4]);
    CHECK(!warp_phase && !warp_pending && psx_mod_read_word(PLAYER+0x68));
    CHECK(!open_both_door(&cpu,actor));stage_initialization(&cpu,0);
    CHECK(!both_door.phase);
    CHECK(!open_both_door(&cpu,actor));mmx4_coop_lifecycle_reset();CHECK(!both_door.phase);
    CHECK(!open_both_door(&cpu,actor));stage_selection(&cpu,0);CHECK(!both_door.phase);
    CHECK(!open_both_door(&cpu,actor));psx_mod_write_byte(actor+1,9);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!both_door.phase && !psx_mod_read_byte(PLAYER+0xC4) && !p2[0xC4]);
    CHECK(!open_both_door(&cpu,actor));
    mmx4_coop_enter_second();cpu.gpr[31]=0x800C1678u;
    mmx4_coop_call(&cpu,0x80036AE4u,0x15,0x40);mmx4_coop_leave_second();
    CHECK(!both_door.phase && !psx_mod_read_byte(PLAYER+0xC4) && !p2[0xC4] && warp_pending);
    CHECK(!open_both_door(&cpu,actor));
    both_door.phase=4;script_active=0;psx_mod_write_byte(PLAYER+0xC4,0);
    cpu.gpr[31]=0x800C1678u;mmx4_coop_call(&cpu,0x80036AE4u,0x15,0x40);
    CHECK(!both_door.phase && warp_pending);cpu.gpr[31]=0;

    /* P1 wins simultaneous eligibility; a corpse cannot enter or be revived. */
    reset();psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(scene_calls==1 && script_owner==0 && psx_mod_read_byte(PLAYER+0xC4)==1);
    CHECK(mmx4_coop_lifecycle_script_owner()==0);
    CHECK(!p2[0xC4]);
    reset();psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(scene_calls==1 && script_owner==1 && p2[0xC4]==1);
    CHECK(psx_mod_read_byte(PLAYER+4)==3 && !warp_pending);
    psx_mod_write_byte(actor+5,3);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(!script_active && psx_mod_read_byte(PLAYER+4)==3 && !p2[0xC4]);
    reset();script_active=script_owner=1;
    story_pad[0]=0x40;story_pad[1]=0;story_pad[2]=0x40;
    psx_mod_write_half(PAD,2);psx_mod_write_half(PAD+2,2);psx_mod_write_half(PAD+4,0);
    mmx4_coop_call(&cpu,0x80021D20u,0,0);
    CHECK(dialogue_calls==1 && dialogue_edge==0x40 && observed_campaign==0);
    CHECK(psx_mod_read_half(PAD)==2 && !psx_mod_read_half(PAD+4) && !projected);
    script_owner=0;mmx4_coop_call(&cpu,0x80021D20u,0,0);
    CHECK(dialogue_calls==2 && !dialogue_edge);
    script_active=0;mmx4_coop_call(&cpu,0x80021D20u,0,0);
    CHECK(dialogue_calls==3 && !dialogue_edge);

    /* Returning to stage selection clears P1's gameplay body. The native
     * frontend still updates its own stage-script pool; P2 has no campaign
     * and must not replace its palette/resources or acquire its story input. */
    reset();psx_mod_write_word(PLAY,3);psx_mod_write_byte(PLAYER+4,0);
    psx_mod_write_byte(PLAYER+0x5C,0);uint8_t front_p2[0xE4];memcpy(front_p2,p2,sizeof front_p2);
    mmx4_coop_call(&cpu,0x8002166Cu,0,0);
    CHECK(stage_script_calls==1 && !stage_script_projected && !projected && observed_campaign==0);
    CHECK(!script_active && !memcmp(front_p2,p2,sizeof front_p2));
    script_active=script_owner=1;story_pad[2]=0x40;psx_mod_write_half(PAD+4,0x800);
    mmx4_coop_call(&cpu,0x80021D20u,0,0);
    CHECK(dialogue_calls==1 && dialogue_edge==0x800);

    /* A surviving P2 can finish Dragoon with P1 dead. The weapon reward
     * (mode7) reuses the text/stage actor dispatchers, but must accept live
     * P1 input even with a stale P2-owned victory script and cached edge. */
    for(unsigned campaign=0;campaign<2;++campaign) {
        reset();psx_mod_write_word(PLAY,7);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
        script_active=script_owner=1;story_pad[0]=story_pad[2]=0x80;
        memcpy(front_p2,p2,sizeof front_p2);
        psx_mod_write_half(PAD,0x40);psx_mod_write_half(PAD+4,0x40);
        mmx4_coop_call(&cpu,0x8002166Cu,0,0);
        CHECK(stage_script_calls==1 && !stage_script_projected && !projected);
        CHECK(observed_campaign==campaign && !memcmp(front_p2,p2,sizeof front_p2));
        mmx4_coop_call(&cpu,0x80021D20u,0,0);
        CHECK(dialogue_calls==1 && dialogue_edge==0x40);
        CHECK(psx_mod_read_half(PAD)==0x40 && psx_mod_read_half(PAD+4)==0x40);
        CHECK(script_active && script_owner==1 && !projected);
    }

    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned story=0;story<2;++story) {
        reset();psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        uint8_t hp=p2[0x5C];mmx4_coop_enter_second();
        uint8_t projected_campaign=story?(uint8_t)campaign:(uint8_t)(campaign^1u);
        psx_mod_write_byte(PLAY+0x43,projected_campaign);
        mmx4_coop_call(&cpu,0x8002217Cu,16,255);
        CHECK(dialogue_init_calls==1 && observed_campaign==campaign && dialogue_init_character==campaign && projected);
        CHECK(psx_mod_read_byte(PLAY+0x43)==projected_campaign && psx_mod_read_byte(PLAYER+0x5C)==hp);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)(campaign^1u));mmx4_coop_leave_second();
        CHECK(psx_mod_read_byte(PLAY+0x43)==campaign && p2[0x5C]==hp);
        CHECK(psx_mod_read_byte(0x80166BB0u)&1u);
    }
    /* Continued capsule text follows its native live actor/cursor, including
     * a restored textbox. A closed box, another bank or scene cannot inherit
     * X's selectors just because a capsule remains elsewhere in the stage. */
    for(unsigned reject=0;reject<8;++reject) {
        reset();psx_mod_write_byte(PLAYER+2,1);p2[2]=0;
        psx_mod_write_word(0x8013BC28u,0x9F046100u);psx_mod_write_byte(0x8013BC34u,0x80);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,26);
        psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+5,2);
        if(reject==1)psx_mod_write_byte(0x8013BC34u,0);
        if(reject==2)psx_mod_write_word(0x8013BC28u,0x801F0000u);
        if(reject==3)psx_mod_write_byte(actor,0);
        if(reject==4)psx_mod_write_byte(actor+1,8);
        if(reject==5)psx_mod_write_byte(actor+5,4);
        if(reject==6)psx_mod_write_byte(PLAYER+2,0);
        if(reject==7)local_view=1;
        CHECK(mmx4_coop_lifecycle_x_capsule_text()==!reject);
    }
    reset();psx_mod_write_byte(PLAYER+2,1);p2[2]=0;
    mmx4_coop_enter_second();capsule_guard=1;
    CHECK(mmx4_coop_lifecycle_x_capsule_text());
    capsule_guard=0;mmx4_coop_leave_second();
    CHECK(!mmx4_coop_lifecycle_x_capsule_text() && !projected);

    reset();psx_mod_write_word(PLAYER+8,0x1000000u);p2[11]=2;
    psx_mod_write_word(actor+8,0x2000000u);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(script_active && script_owner==1 && p2[0xC4]);
    mmx4_coop_enter_second();mmx4_coop_call(&cpu,0x80036AE4u,0x14,0x40);
    mmx4_coop_leave_second();
    unsigned chained_serial=script_serial;
    psx_mod_write_byte(actor+5,3);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(script_active && script_owner==1 && script_serial==chained_serial && !p2[0xC4]);
    CHECK(p2[0xC0]==1);
    /* P2's bike waits for the same READY actor without allocating/restarting
     * it. Ordinary allocations and the campaign owner's bike remain native. */
    reset();uint32_t ready=0x80142F98u;
    psx_mod_write_word(MMX4_VEHICLE+0xA0,ready);
    psx_mod_write_byte(ready,1);psx_mod_write_byte(ready+1,0x1B);
    psx_mod_write_byte(ready+5,2);cpu.gpr[31]=0x8003B64Cu;
    CHECK(!shared_ride_ready(&cpu,0x8002AD7Cu));
    mmx4_coop_enter_second();
    CHECK(shared_ride_ready(&cpu,0x8002AD7Cu) && cpu.gpr[2]==0);
    CHECK(psx_mod_read_word(MMX4_VEHICLE+0xA0)==ready);
    CHECK(psx_mod_read_byte(ready+5)==2);
    cpu.gpr[31]=0x8001FE8Cu;CHECK(!shared_ride_ready(&cpu,0x8002AD7Cu));
    mmx4_coop_leave_second();CHECK(body_word(p2_vehicle,0xA0)==ready);
    reset();psx_mod_write_byte(PLAYER+2,0);psx_mod_write_byte(PLAYER+5,0x22);
    CHECK(mmx4_coop_lifecycle_nova_active());
    mmx4_coop_enter_second();CHECK(mmx4_coop_lifecycle_nova_active());mmx4_coop_leave_second();
    script_active=1;CHECK(!mmx4_coop_lifecycle_nova_active());script_active=0;
    psx_mod_write_byte(PLAYER+5,2);p2[2]=0;p2[5]=0x22;
    CHECK(mmx4_coop_lifecycle_nova_active());
    p2[2]=1;CHECK(!mmx4_coop_lifecycle_nova_active());
    initial_arrival=1;CHECK(mmx4_coop_lifecycle_initial_arrival());
    initial_arrival=0;CHECK(!mmx4_coop_lifecycle_initial_arrival());
    /* Refight entry is owned by the touching hero, even when the AI/view
     * owner differs. Both mode waits for the same pad; dead/withdrawn seats
     * cannot prevent a survivor from entering. Solid contact remains shared. */
    for(unsigned both=0;both<2;++both)for(unsigned seat=0;seat<2;++seat)
    for(unsigned absent=0;absent<3;++absent) {
        reset();both_door_players=(int)both;
        portal_hits[0]=portal_hits[1]=0;portal_hits[seat]=1;
        portal_admissions=0;portal_continuations[0]=portal_continuations[1]=0;
        psx_mod_write_byte(PLAY+12,12);psx_mod_write_byte(PLAY+13,0);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,27);psx_mod_write_byte(actor+4,1);
        psx_mod_write_word(actor+8,1024u<<16);
        if(absent==1) {
            if(seat)psx_mod_write_byte(PLAYER+0x5C,0);else p2[0x5C]=0;
        }else if(absent==2) {
            if(seat)psx_mod_write_byte(PLAYER,0);else departed=1;
        }
        unsigned solids_before=canonical_portal_solids;
        mmx4_coop_call(&cpu,0x800C7164u,actor,0);
        CHECK(canonical_portal_solids==solids_before+1 && !projected);
        if(both && !absent) {
            CHECK(!portal_admissions && !script_active && !warp_pending && !psx_mod_read_byte(actor+5));
            portal_hits[seat^1u]=1;mmx4_coop_call(&cpu,0x800C7164u,actor,0);
            CHECK(script_owner==0); /* P1 resolves simultaneous contact. */
        }else CHECK(script_owner==seat);
        CHECK(portal_admissions==1 && script_active && psx_mod_read_byte(actor+5)==1);
        unsigned owner=script_owner;
        portal_hits[0]=portal_hits[1]=0;
        if(owner)p2[0xC0]=0;else psx_mod_write_byte(PLAYER+0xC0,0);
        mmx4_coop_call(&cpu,0x800C7164u,actor,0);
        CHECK(portal_continuations[owner]==1 && !portal_continuations[owner^1u]);
        CHECK(psx_mod_read_byte(PLAY+0x43)==0 && !projected && !door_wait);
    }
    /* Nearby fortress presentations retain both heroes for either owner.
     * Actual transports and unsafe/nonparticipating partners keep carries. */
    for(unsigned section=0;section<2;++section)for(unsigned owner=0;owner<2;++owner)
    for(unsigned gate=0;gate<10;++gate) {
        reset();psx_mod_write_byte(PLAY+12,12);psx_mod_write_byte(PLAY+13,(uint8_t)section);
        psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);
        p2[3]=p2[4]=1;
        psx_mod_write_word(PLAYER+8,1000u<<16);put32(p2+8,1024u<<16);
        psx_mod_write_word(PLAYER+12,400u<<16);put32(p2+12,400u<<16);
        cpu.gpr[31]=0x80085CA8u;
        if(gate==1)put32(p2+8,1500u<<16);
        if(gate==2)put32(p2+12,650u<<16);
        if(gate==3)p2[3]=0;
        if(gate==4)p2[0x5C]=0;
        if(gate==5)p2[0x14]^=1;
        if(gate==6)psx_mod_write_byte(PLAY+12,11);
        if(gate==7)cpu.gpr[31]=0x800C1678u;
        if(gate==8)psx_mod_write_byte(PLAYER+0xC4,1);
        if(owner)mmx4_coop_enter_second();
        mmx4_coop_call(&cpu,0x80036AE4u,gate==9?0x15:0x14,0x40);
        if(owner)mmx4_coop_leave_second();
        CHECK(script_active && script_owner==owner && !projected);
        if(gate==0)CHECK(!warp_pending && !warp_phase);
        else if(gate!=4)CHECK(warp_pending);
    }
    reset();
    CHECK(!personal_floor_momentum(&cpu));
    /* Independent panes retain a partner below the same climbing shaft,
     * even when P1 has scrolled the partner completely out of view. */
    reset();split_views=1;latch_calls=0;
    {
        uint32_t fan=0x80165A30u;
        psx_mod_write_byte(PLAY+12,3);psx_mod_write_byte(fan,1);
        psx_mod_write_byte(fan+1,13);psx_mod_write_byte(fan+4,1);
        psx_mod_write_half(fan+10,6152);psx_mod_write_half(fan+14,2232);
        psx_mod_write_word(fan+0x68,0x800F0000u);
        psx_mod_write_byte(0x800F0003u,16);psx_mod_write_byte(0x800F0023u,16);
        put32(p2+8,6144u<<16);put32(p2+12,2500u<<16);
        put32(p2+0x68,0x800F0020u);cpu.gpr[6]=PLAYER;
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);CHECK(!latch_calls);
        put32(p2+12,2150u<<16);
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);CHECK(latch_calls==1);
        psx_mod_write_byte(fan+5,0);put32(p2+8,100u<<16);put32(p2+12,2500u<<16);
        mmx4_coop_call(&cpu,0x800C36E8u,fan,PLAY);CHECK(latch_calls==2);
    }
    /* Long-lived train scenery owns real scenes. It must follow progress
     * rather than whichever hero stayed near
     * the controller's allocation coordinate. P1's story remains canonical. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned mode=0;mode<2;++mode)
      for(unsigned kind=0;kind<2;++kind) {
        reset();split_views=mode;scene_calls=0;
        uint32_t actor=0x80169CB8u;
        uint32_t address=kind==0?0x800D8648u:0x800D9218u;
        psx_mod_write_byte(PLAY+12,8);
        psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(PLAYER+2,(uint8_t)campaign);p2[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
        psx_mod_write_half(actor+10,36);
        psx_mod_write_half(actor+14,650);
        psx_mod_write_half(PLAYER+10,128);psx_mod_write_half(PLAYER+14,650);
        put32(p2+8,1800u<<16);put32(p2+12,650u<<16);
        cpu.gpr[4]=actor;CHECK(scene_actor(&cpu,address));
        CHECK(scene_calls==1 && psx_mod_read_half(actor+0x1A)==1800);
        CHECK(observed_campaign==campaign && psx_mod_read_half(PLAYER+10)==128);
        CHECK(!projected && mmx4_coop_lifecycle_snapshot_ready());
      }
    for(unsigned owner=0;owner<2;++owner)for(unsigned dead=0;dead<2;++dead) {
        reset();const uint32_t actor=0x80169CB8u;
        psx_mod_write_byte(PLAY+12,8);psx_mod_write_byte(actor,1);
        psx_mod_write_byte(actor+1,3);psx_mod_write_half(PLAYER+10,owner?6464:7010);
        put32(p2+8,(owner?7010u:6464u)<<16);
        if(dead) {
            if(owner) {psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);}
            else {p2[4]=3;p2[0x5C]=0;}
        }
        mmx4_coop_call(&cpu,0x800D9C84u,actor,0);
        CHECK(scene_calls==1 && script_active && script_owner==owner && !projected);
        CHECK(!psx_mod_read_byte(actor) && mmx4_coop_lifecycle_snapshot_ready());
        CHECK(psx_mod_read_half(PLAYER+10)==(owner?6464:7010));
    }
    /* Only the inspected Eregion part2 admission commits a saved camera,
     * including WARNING's freeze; unrelated callers and sections do not. */
    for(unsigned owner=0;owner<2;++owner)for(unsigned excluded=0;excluded<4;++excluded) {
        reset();psx_mod_write_byte(PLAY+13,1);psx_mod_write_byte(PLAY+0x10,1);
        cpu.gpr[31]=excluded==1?0x8004954Cu:0x80049548u;
        if(excluded==2)psx_mod_write_byte(PLAY+13,0);
        if(excluded==3)psx_mod_write_byte(PLAY+12,1);
        unsigned count=scene_camera_commits;
        if(owner)mmx4_coop_enter_second();
        mmx4_coop_call(&cpu,0x80036AE4u,0x15,0);
        if(owner)mmx4_coop_leave_second();
        CHECK(scene_camera_commits==count+(excluded==0));
        if(!excluded)CHECK(scene_camera_owner==owner);
    }
    CHECK(!colonel_palette_refreshes(&cpu));
    CHECK(!survivor_palette_animation(&cpu));
    CHECK(!marine_chaser_dismount());
    puts("co-op lifecycle ownership checks passed");return 0;
}
