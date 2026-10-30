/* Boundary tests use the original-backed routine contracts documented in
 * COOP_COMBAT_VEHICLE_EVIDENCE.md. They exercise host ownership/call ordering;
 * they are not a substitute for native executable gameplay qualification. */
#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1); } } while (0)
#define CONTACT 0x8002D9BCu
#define HIT 0x8002DD04u
#define SOLID 0x8002E184u
#define ALLOC 0x8002ADBCu
#define EFFECT_ALLOC 0x8002AD3Cu
#define EFFECT 0x8013E510u
#define PARTICLE_ALLOC 0x8002AE50u
#define PARTICLE_INIT 0x800CAE38u
#define TRAIN_PILLAR 0x800CB8F8u
#define STOCK_HUD 0x800B2C8Cu
#define PARTICLE 0x80173CA0u
#define WATER 0x800AF22Cu
#define SLIDE_EFFECT 0x800AEAC0u
#define DASH_EFFECT 0x800AED18u
#define HOVER_EFFECT 0x800B2AD0u
#define SHOT_EFFECT 0x800B28CCu
#define SHOTS 0x801406F8u
#define MIRROR 0x80110000u
#define ARMOR_UPDATE 0x8003D3F8u
#define ARMOR_MOUNT 0x8003E0D0u
#define AIM 0x80042824u
#define ACTOR 0x80165A30u
#define DOUBLE_BODY 0x80175D58u
#define NATIVE_AI 0x80040000u
#define ACTOR_DISPATCH 0x8F7FFF00u
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);

typedef struct { uint32_t address; PSXModFunctionFilterCallback callback; } Filter;
static unsigned aiming_calls;
static Filter filters[32];
static unsigned filter_count;
static unsigned local_view;
int psx_mod_local_view_scope(void) {return (int)local_view;}
int mmx4_coop_web_busy(void) {return 0;}
int mmx4_coop_web_contact(CPUState *cpu,uint32_t address) {(void)cpu;(void)address;return 0;}
void mmx4_coop_web_forget(uint32_t actor,unsigned owner) {(void)actor;(void)owner;}
void mmx4_coop_web_reset(void) {}
void mmx4_coop_balance_forget(uint32_t actor) {(void)actor;}
void mmx4_coop_balance_begin(uint32_t actor) {(void)actor;}
void mmx4_coop_balance_end(void) {}
static unsigned boss_immunity[2];
int mmx4_coop_balance_hit_allowed(uint32_t actor,unsigned seat) {(void)actor;return !boss_immunity[seat];}
int mmx4_coop_balance_contact_allowed(uint32_t actor,uint32_t shot,unsigned seat) {
    (void)shot;return mmx4_coop_balance_hit_allowed(actor,seat);
}
int mmx4_coop_balance_project_contact(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;return 0;
}
uint32_t mmx4_coop_web_digest(uint32_t seed) {return seed;}
static uint8_t ram[0x200000];
static uint8_t scratchpad[1024];
static uint8_t second[0xE4], first_saved[0xE4];
static uint8_t second_vehicle[0xB0], first_vehicle[0xB0];
static uint8_t second_double[0xE4], first_double[0xE4];
static uint8_t second_shots[16*0x9C],first_shots[16*0x9C];
static unsigned projected, ready, alive[2],split;
static unsigned finishes,finishing_character;
static uint32_t finishing_actor;
void mmx4_coop_lifecycle_boss_finisher(uint32_t actor,unsigned character) {
    ++finishes;finishing_actor=actor;finishing_character=character;
}
static int script_owner=-1;
static uint32_t deferred_portal;
static unsigned arriving;
int mmx4_coop_lifecycle_arriving(unsigned seat) {return seat==1 && arriving;}
int mmx4_coop_lifecycle_defer_portal_solid(uint32_t actor) {return actor==deferred_portal;}
static unsigned target_camera_mode=1;
static PSXModFunctionEntryCallback redirects[2],actor_dispatch,double_admission;
static PSXModFunctionEntryCallback reaction_hooks[7];
static const uint32_t reaction_sites[]={0x80050998u,0x80071DE4u,0x80072514u,
    0x800753FCu,0x80082644u,0x800850DCu,0x80091344u};
static unsigned reaction_model,reaction_expected,reaction_checks;
static PSXModFunctionEntryCallback owl_release_hooks[6];
static const uint32_t owl_release_sites[]={0x80075694u,0x800763D0u,0x800764D4u,
    0x800767E4u,0x8007687Cu,0x800778C8u};
static unsigned owl_release_model;
static unsigned pool_updates,actor_updates[2],bounds_updates,ai_contact,ai_solid;
static unsigned dragoon_release_model,dragoon_release_calls,dragoon_release_projected;
void mmx4_coop_split_dragoon_camera_release(uint32_t actor) {
    CHECK(actor==0x8013BED0u && psx_mod_read_byte(actor+5)==3);
    ++dragoon_release_calls;dragoon_release_projected=projected;
}
static unsigned sigma_release_model,sigma_release_calls,sigma_release_projected;
void mmx4_coop_split_sigma_victory_release(uint32_t actor) {
    CHECK(actor==0x8013BED0u && !psx_mod_read_byte(actor));
    CHECK(mmx4_coop_combat_actor_context());
    ++sigma_release_calls;sigma_release_projected=projected;
}
static unsigned ai_direct_write,ai_canonical;
static unsigned ai_expected_campaign;
static uint8_t saved_campaign;
static uint32_t ai_target_x[2];
static uint32_t hit_result[2], allocated_actor;
static unsigned native_calls[2], consumed, ai_updates, allocator_calls;
static uint8_t observed_sides[2], observed_carried[2];
static uint8_t observed_vehicle_sides[2], observed_vehicle_carried[2];
static uint8_t observed_double[2], observed_vehicle[2];
static uint32_t observed_target_x, observed_target_y;
static unsigned armor_updates, mount_checks, mount_accept[2];
static uint8_t observed_freeze;
static unsigned token_model, attack_present[2];
static uint8_t attack_token[2];
static unsigned double_model;
static uint8_t attack_type[2];
static uint32_t double_tick(void);
static unsigned particle_initializations,particle_character;
static unsigned stock_hud_updates;
static unsigned water_updates;
static unsigned charge_updates;
static unsigned weapon_uploads;
static unsigned attached_effect_updates[2][2];
static uint32_t second_assembly,saved_assembly;
static unsigned palette_model;
static uint32_t saved_colors,saved_palette;
static uint8_t saved_palette_dirty;
#define SHARED_COLORS 0x80112000u
#define SHARED_PALETTE 0x80114000u
#define PRIVATE_COLORS 0x80116000u
#define PRIVATE_PALETTE 0x80118000u

static uint8_t *address_ptr(uint32_t address) {
    uint32_t physical=address&0x1FFFFFFFu;
    if(physical>=0x1F800000u && physical<0x1F800000u+sizeof scratchpad)
        return scratchpad+(physical-0x1F800000u);
    CHECK(physical<sizeof ram);
    return ram+physical;
}
uint8_t psx_mod_read_byte(uint32_t address) { return *address_ptr(address); }
void psx_mod_write_byte(uint32_t address,uint8_t value) { *address_ptr(address)=value; }
uint16_t psx_mod_read_half(uint32_t address) {
    return (uint16_t)(psx_mod_read_byte(address) |
        (uint16_t)psx_mod_read_byte(address+1)<<8);
}
void psx_mod_write_half(uint32_t address,uint16_t value) {
    psx_mod_write_byte(address,(uint8_t)value);
    psx_mod_write_byte(address+1,(uint8_t)(value>>8));
}
uint32_t psx_mod_read_word(uint32_t address) {
    return (uint32_t)psx_mod_read_half(address) |
        (uint32_t)psx_mod_read_half(address+2)<<16;
}
void psx_mod_write_word(uint32_t address,uint32_t value) {
    for (unsigned i=0;i<4;++i) psx_mod_write_byte(address+i,(uint8_t)(value>>(8*i)));
}
void psx_mod_counter_add(const char *name,uint32_t delta) { (void)name;(void)delta; }
int psx_mod_register_function_filter_plugin(const char *id,uint32_t address,
                                           PSXModFunctionFilterCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && filter_count<32);
    filters[filter_count++]=(Filter){address,callback};
    return 1;
}
int psx_mod_register_instruction_plugin(const char *id,uint32_t address,uint32_t word,
    PSXModFunctionEntryCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop"));
    if(address==0x8002DE84u) {
        CHECK(word==0xA0820063u);double_admission=callback;return 1;
    }
    for(unsigned i=0;i<7;++i)if(address==reaction_sites[i]) {
        CHECK(!word);reaction_hooks[i]=callback;return 1;
    }
    for(unsigned i=0;i<6;++i)if(address==owl_release_sites[i]) {
        CHECK(word==0xA0201982u);owl_release_hooks[i]=callback;return 1;
    }
    CHECK(word==0x0040F809u);
    CHECK(address==0x80021300u || address==0x80021518u);
    redirects[address==0x80021518u]=callback;return 1;
}
int psx_mod_register_guest_function_plugin(const char *id,uint32_t address,
    PSXModFunctionEntryCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && address==ACTOR_DISPATCH);
    CHECK((address&0x1FFFFFFFu)>=0x0F000000u && (address&0x1FFFFFFFu)<0x10000000u);
    actor_dispatch=callback;return 1;
}
static PSXModFunctionFilterCallback filter(uint32_t address) {
    for (unsigned i=0;i<filter_count;++i)
        if (filters[i].address==address) return filters[i].callback;
    CHECK(0);
    return NULL;
}
int mmx4_coop_ready(void) { return (int)ready; }
int mmx4_coop_projected(void) { return (int)projected; }
int mmx4_coop_split_views(void) {return (int)split;}
int mmx4_coop_lifecycle_script_owner(void) {return script_owner;}
static unsigned scene_camera_model,scene_camera_begins,scene_camera_ends;
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]) {
    CHECK(owner<2);
    if(!scene_camera_model || !split)return 0;
    ++scene_camera_begins;memcpy(canonical,address_ptr(0x801419B0u),0xFC);
    psx_mod_write_half(0x801419BAu,owner?4500:1000);return 1;
}
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed) {
    CHECK(owner<2);++scene_camera_ends;
    if(!committed)memcpy(address_ptr(0x801419B0u),canonical,0xFC);
}
int mmx4_coop_alive(unsigned seat) { CHECK(seat<2);return (int)alive[seat]; }
uint8_t *mmx4_coop_second_body(void) { return second; }
uint8_t *mmx4_coop_first_body(void) {return first_saved;}
void mmx4_coop_split_actors(Mmx4CoopViewActor actors[2]) {
    for(unsigned seat=0;seat<2;++seat) {
        const uint8_t *body=seat?second:first_saved;
        int16_t world_x=seat==projected?(int16_t)psx_mod_read_half(MMX4_PLAYER+10):
            (int16_t)((uint16_t)body[10]|(uint16_t)body[11]<<8);
        int16_t world_y=seat==projected?(int16_t)psx_mod_read_half(MMX4_PLAYER+14):
            (int16_t)((uint16_t)body[14]|(uint16_t)body[15]<<8);
        actors[seat]=(Mmx4CoopViewActor){world_x,world_y,alive[seat]};
    }
}
uint8_t *mmx4_coop_second_vehicle(void) { return second_vehicle; }
uint8_t *mmx4_coop_first_vehicle(void) { CHECK(projected);return first_vehicle; }
void mmx4_coop_enter_second(void) {
    CHECK(!projected);
    memcpy(first_saved,address_ptr(MMX4_PLAYER),sizeof first_saved);
    memcpy(first_vehicle,address_ptr(MMX4_VEHICLE),sizeof first_vehicle);
    memcpy(first_double,address_ptr(DOUBLE_BODY),sizeof first_double);
    memcpy(first_shots,address_ptr(SHOTS),sizeof first_shots);
    memcpy(address_ptr(MMX4_PLAYER),second,sizeof second);
    memcpy(address_ptr(MMX4_VEHICLE),second_vehicle,sizeof second_vehicle);
    memcpy(address_ptr(DOUBLE_BODY),second_double,sizeof second_double);
    memcpy(address_ptr(SHOTS),second_shots,sizeof second_shots);
    saved_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
    psx_mod_write_byte(MMX4_PLAY+0x43,second[2]);
    saved_assembly=psx_mod_read_word(0x1F80001Cu);
    psx_mod_write_word(0x1F80001Cu,second_assembly);
    if(palette_model) {
        saved_colors=psx_mod_read_word(0x1F800024u);saved_palette=psx_mod_read_word(0x1F800028u);
        saved_palette_dirty=psx_mod_read_byte(0x80166BB0u);
        psx_mod_write_word(0x1F800024u,PRIVATE_COLORS);psx_mod_write_word(0x1F800028u,PRIVATE_PALETTE);
    }
    projected=1;
}
void mmx4_coop_leave_second(void) {
    CHECK(projected);
    memcpy(second,address_ptr(MMX4_PLAYER),sizeof second);
    memcpy(second_vehicle,address_ptr(MMX4_VEHICLE),sizeof second_vehicle);
    memcpy(second_double,address_ptr(DOUBLE_BODY),sizeof second_double);
    memcpy(second_shots,address_ptr(SHOTS),sizeof second_shots);
    memcpy(address_ptr(MMX4_PLAYER),first_saved,sizeof first_saved);
    memcpy(address_ptr(MMX4_VEHICLE),first_vehicle,sizeof first_vehicle);
    memcpy(address_ptr(DOUBLE_BODY),first_double,sizeof first_double);
    memcpy(address_ptr(SHOTS),first_shots,sizeof first_shots);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==second[2]);
    psx_mod_write_byte(MMX4_PLAY+0x43,saved_campaign);
    psx_mod_write_word(0x1F80001Cu,saved_assembly);
    if(palette_model) {
        psx_mod_write_word(0x1F800024u,saved_colors);psx_mod_write_word(0x1F800028u,saved_palette);
        psx_mod_write_byte(0x80166BB0u,saved_palette_dirty);
    }
    projected=0;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result) { cpu->gpr[2]=result;return 1; }
uint16_t mmx4_coop_input(unsigned seat) { (void)seat;return 0; }

uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    (void)a1;
    if(address==0x8003DC44u) {
        psx_mod_write_byte(a0+5,(uint8_t)a1);psx_mod_write_byte(a0+6,0);return 0;
    }
    if(address==0x8003EEF8u) {
        CHECK(psx_mod_read_byte(a0+5)==0xB && !psx_mod_read_byte(a0+6));
        psx_mod_write_byte(a0+6,1);psx_mod_write_byte(a0+0x97,
            (uint8_t)(psx_mod_read_byte(a0+0x97)&~0x40u));
        psx_mod_write_byte(MMX4_PLAYER+0xC5,0);
        psx_mod_write_word(MMX4_PLAYER+0x68,0x800F8BC4u);return 0;
    }
    /* Re-entered native dispatch must decline the active host filter. */
    if (address!=ARMOR_MOUNT && address!=NATIVE_AI && address!=0x8002B3C0u)
        CHECK(!filter(address)(cpu,address));
    if(address==0x800D67DCu) {
        CHECK(projected && !psx_mod_read_byte(MMX4_PLAYER+2));
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0x93)==6);
        CHECK(psx_mod_read_word(a0+0x5C)==0x8013BED0u);
        psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8));
        ++aiming_calls;return 0xCAFEu;
    }
    unsigned seat=projected;
    if (address==ARMOR_UPDATE || address==ARMOR_MOUNT) {
        if (address==ARMOR_UPDATE) ++armor_updates;
        else ++mount_checks;
        if (mount_accept[seat]) {
            psx_mod_write_byte(MMX4_VEHICLE+5,0xA);
            psx_mod_write_byte(MMX4_VEHICLE+0x97,0x40);
            psx_mod_write_byte(MMX4_PLAYER+0xC5,1);
        }
        return 0;
    }
    if (address==CONTACT || address==HIT) {
        ++native_calls[seat];
        /* DD04 calls DE30 only after finding a live overlapping shot. */
        if(address==HIT && (!(token_model || double_model) || attack_present[seat]) &&
           !psx_mod_read_byte(a0+0x61) && !psx_mod_read_byte(a0+0x7A)) {
            CPUState shot={.gpr={[4]=a0,[5]=SHOTS}};
            if(filter(0x8002DE30u)(&shot,0x8002DE30u)) {
                CHECK(shot.gpr[2]==0x7Eu);return 0;
            }
        }
        if(address==HIT && double_model) {
            /* DD04 overlap and DE30's signed-byte token gate precede the
             * admission load. A rejected repeated hit returns DD04 zero. */
            if(!attack_present[seat]) {
                psx_mod_write_byte(a0+0x65,0);return 0;
            }
            uint8_t token=attack_token[seat],type=attack_type[seat];
            if(token && token==psx_mod_read_byte(a0+0x65))return 0;
            psx_mod_write_byte(a0+0x65,token);
            uint32_t table=psx_mod_read_word(a0+0x58);
            /* DE7C loads admission; DE80 is its load-delay slot. Only the
             * DE84 hook before SB observes the committed admission byte. */
            CPUState admission={.gpr={[2]=psx_mod_read_byte(table+type*2u),
                [4]=a0,[6]=type,[7]=table}};
            double_admission(&admission,0x8002DE84u);
            psx_mod_write_byte(a0+0x63,(uint8_t)admission.gpr[2]);
            if(type>=26 && type<36)psx_mod_write_byte(MMX4_PLAYER+0xBD,1);
            if(admission.gpr[2]==0xFFu)return 0x7F;
            uint8_t hp=psx_mod_read_byte(a0+0x5C);
            uint8_t amount=psx_mod_read_byte(admission.gpr[7]+type*2u+1u);
            psx_mod_write_byte(a0+0x5C,(uint8_t)(hp>amount?hp-amount:0));
            ++consumed;
            return hp>amount?type+1u:UINT32_MAX;
        }
        if (address==HIT && token_model) {
            /* Native DD04 clears +65 for no accepted overlap/blocked target;
             * DE30 rejects equal nonzero signed-byte tokens before damage. */
            if (!attack_present[seat] || psx_mod_read_byte(a0+0x61) ||
                psx_mod_read_byte(a0+0x7A)) {
                psx_mod_write_byte(a0+0x65,0);
                return 0;
            }
            uint8_t token=attack_token[seat];
            if (token && token==psx_mod_read_byte(a0+0x65)) return 0;
            psx_mod_write_byte(a0+0x65,token);
            psx_mod_write_byte(a0+0x5C,(uint8_t)(psx_mod_read_byte(a0+0x5C)-1));
            ++consumed;
            return 1;
        }
        uint32_t result=hit_result[seat];
        if (result) {
            ++consumed;
            if (address==CONTACT) {
                uint8_t hp=psx_mod_read_byte(MMX4_PLAYER+0x5C);
                psx_mod_write_byte(MMX4_PLAYER+0x5C,(uint8_t)(hp-1));
            } else psx_mod_write_byte(a0+0x5C,
                (uint8_t)(psx_mod_read_byte(a0+0x5C)-1));
        }
        return result;
    }
    if (address==SOLID) {
        ++native_calls[seat];
        observed_sides[seat]=psx_mod_read_byte(a0+0x72);
        observed_carried[seat]=psx_mod_read_byte(a0+0x76);
        observed_vehicle_sides[seat]=psx_mod_read_byte(a0+0x74);
        observed_vehicle_carried[seat]=psx_mod_read_byte(a0+0x78);
        observed_double[seat]=psx_mod_read_byte(DOUBLE_BODY);
        observed_vehicle[seat]=psx_mod_read_byte(MMX4_VEHICLE);
        psx_mod_write_byte(a0+0x72,(uint8_t)(0x10+seat));
        psx_mod_write_byte(a0+0x76,(uint8_t)(0x20+seat));
        psx_mod_write_byte(a0+0x74,(uint8_t)(0x30+seat));
        psx_mod_write_byte(a0+0x78,(uint8_t)(0x40+seat));
        psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(MMX4_PLAYER+8)+(2u<<16));
        psx_mod_write_word(MMX4_VEHICLE+8,psx_mod_read_word(MMX4_VEHICLE+8)+(3u<<16));
        return 0x12345678u;
    }
    if (address==ALLOC || address==0x8002AB74u || address==0x8002ACA4u ||
        address==EFFECT_ALLOC || address==PARTICLE_ALLOC) {
        ++allocator_calls;
        if (allocated_actor && address!=EFFECT_ALLOC && address!=PARTICLE_ALLOC)
            psx_mod_write_byte(allocated_actor+0x65,0);
        return allocated_actor;
    }
    if(address==TRAIN_PILLAR) {
        ++particle_initializations;
        CHECK(psx_mod_read_word(0x1F800020u)==0x80108000u);
        psx_mod_write_half(a0+10,(uint16_t)(psx_mod_read_half(0x801419BAu)+352));
        psx_mod_write_half(a0+14,(uint16_t)(psx_mod_read_half(0x801419BEu)+128));
        psx_mod_write_byte(a0+0x14,psx_mod_read_byte(MMX4_PLAYER+0x14));
        psx_mod_write_byte(a0+4,1);return 0xABCDEF20u;
    }
    if(address==PARTICLE_INIT || address==0x8009485Cu || address==0x80098474u) {
        ++particle_initializations;particle_character=psx_mod_read_byte(MMX4_PLAYER+2);
        /* CAE38 reads these fields and P2's projected character assembly.
         * Its native movement/draw remains one call at the shared pool tick. */
        psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8));
        psx_mod_write_word(a0+12,psx_mod_read_word(MMX4_PLAYER+12));
        psx_mod_write_byte(a0+0x14,psx_mod_read_byte(MMX4_PLAYER+0x14));
        psx_mod_write_byte(a0+4,1);
        return 0xABCDEF01u;
    }
    if(address==STOCK_HUD) {
        ++stock_hud_updates;
        CHECK(!psx_mod_read_byte(MMX4_PLAYER+2));
        /* Native B2D04..B2D3C consumes A6 solely to select the display frame. */
        unsigned count=psx_mod_read_byte(MMX4_PLAYER+0xA6);
        psx_mod_write_byte(a0+3,(uint8_t)(count!=0));
        psx_mod_write_byte(a0+0x47,(uint8_t)(31-count));
        return 0xABCDEF02u;
    }
    if(address==SHOT_EFFECT) {
        /* B28E0/B2924 follow the saved parent address in the current shot
         * arena. Identical guest pointers refer to different seats' shots. */
        uint32_t parent=psx_mod_read_word(a0+0x50);
        if(!psx_mod_read_byte(parent)) {psx_mod_write_byte(a0,0);return 0;}
        psx_mod_write_word(a0+0x38,psx_mod_read_word(parent+0x38));
        psx_mod_write_word(a0+0x3C,psx_mod_read_word(parent+0x3C));
        psx_mod_write_word(a0+8,psx_mod_read_word(parent+8));
        return 0x10203041u;
    }
    if(address==0x800B2A3Cu) {
        ++weapon_uploads;
        CHECK(projected && !psx_mod_read_byte(MMX4_PLAYER+2));
        /* Original27 reads the current character's sprite header before
         * its one-shot upload and deallocation. Do not advance either body. */
        psx_mod_write_word(a0+0x38,psx_mod_read_word(MMX4_PLAYER+0x38));
        psx_mod_write_byte(a0,0);
        return 0x10203040u;
    }
    if(address==HOVER_EFFECT) {
        /* Original B2AD0 expires the jets if their owner's pose is no
         * longer a hover/dash pose. Initialization binds its assembly. */
        if(psx_mod_read_byte(MMX4_PLAYER+0x47)!=229) {
            psx_mod_write_byte(a0,0);psx_mod_write_byte(a0+3,0);
        }else {
            uint32_t assembly=psx_mod_read_word(0x1F80001Cu);
            psx_mod_write_word(a0+0x3C,assembly+psx_mod_read_word(assembly+0x30));
            psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8));
            psx_mod_write_word(a0+12,psx_mod_read_word(MMX4_PLAYER+12));
            psx_mod_write_byte(a0+3,1);
        }
        return 0x87654321u;
    }
    if(address==SLIDE_EFFECT || address==DASH_EFFECT) {
        unsigned dash=address==DASH_EFFECT;
        ++attached_effect_updates[seat][dash];
        uint32_t owner=psx_mod_read_byte(MMX4_PLAYER+0xDE)?DOUBLE_BODY:MMX4_PLAYER;
        unsigned character=psx_mod_read_byte(owner+2);
        unsigned state=psx_mod_read_byte(a0+4);
        if(!state) {
            /* Original AEB38/AED64 reads the current assembly header only
             * when the shared effect first initializes, after allocation. */
            uint32_t assembly=psx_mod_read_word(0x1F80001Cu);
            psx_mod_write_word(a0+0x3C,assembly+psx_mod_read_word(assembly+4));
            psx_mod_write_byte(a0+3,1);psx_mod_write_byte(a0+4,1);
        }else if(dash && psx_mod_read_byte(owner+0x17)!=0x12) {
            /* Native AEE04 rejects an idle body for normal ground Dash.
             * Reading P1 here would also erase P2's exhaust prematurely. */
            psx_mod_write_byte(a0,0);psx_mod_write_byte(a0+3,0);
            return 0xAABBCCDDu;
        }else if(!dash && psx_mod_read_byte(owner+5)!=11 && psx_mod_read_byte(owner+5)!=52) {
            psx_mod_write_byte(a0+4,2);return 0xAABBCCDDu;
        }
        /* Native AEC9C/AEE5C selects character offsets and facing, writing
         * only the integer coordinate halfwords and preserving fractions. */
        int dx=dash?(character?36:32):10,dy=dash?(character?14:12):(character?25:17);
        uint8_t facing=psx_mod_read_byte(owner+0x15);
        int x=(int16_t)psx_mod_read_half(owner+10)+(facing?-dx:dx);
        int y=(int16_t)psx_mod_read_half(owner+14)+dy;
        psx_mod_write_byte(a0+0x15,facing);
        psx_mod_write_word(a0+8,(psx_mod_read_word(a0+8)&0xFFFFu)|((uint32_t)(uint16_t)x<<16));
        psx_mod_write_word(a0+12,(psx_mod_read_word(a0+12)&0xFFFFu)|((uint32_t)(uint16_t)y<<16));
        return 0xAABBCCDDu;
    }
    if(address==0x800AEED8u) {
        ++charge_updates;
        CHECK(projected && !psx_mod_read_byte(MMX4_PLAYER+2));
        psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8));
        psx_mod_write_byte(MMX4_PLAYER+0x98,
            (uint8_t)(psx_mod_read_byte(MMX4_PLAYER+0x98)-1));
        return 0x12345678u;
    }
    if(address==WATER) {
        ++water_updates;
        /* Native type 7 selects the owner body (or its Double) on every
         * update, rather than retaining an explicit following pointer.
         * Its final B288 visibility query must also admit both Split views
         * while this owner is projected, including at a distant waterfall. */
        CHECK(mmx4_coop_combat_actor_context());
        uint32_t source=psx_mod_read_byte(a0+2)&2u?DOUBLE_BODY:MMX4_PLAYER;
        psx_mod_write_word(a0+8,psx_mod_read_word(source+8));
        psx_mod_write_word(a0+12,psx_mod_read_word(source+12));
        psx_mod_write_byte(a0+0x15,psx_mod_read_byte(source+0x15));
        return 0x76543210u;
    }
    if (address==AIM || address==0x80040CCCu || address==0x800419B8u) {
        ++ai_updates;
        observed_target_x=psx_mod_read_word(MMX4_PLAYER+8);
        observed_target_y=psx_mod_read_word(MMX4_PLAYER+12);
        return 0x87654321u;
    }
    if (address==0x80021234u || address==0x8002144Cu) {
        {
            ++pool_updates;
            CHECK(!projected && !psx_mod_read_byte(MMX4_PLAYER+0xBC));
            unsigned shots=address==0x8002144Cu;
            uint32_t first=shots?0x8013F328u:0x8013BED0u;
            for(unsigned index=0;index<2;++index) {
                uint32_t actor=first+index*0x9Cu;
                if(!psx_mod_read_byte(actor))continue;
                CPUState dispatch=*cpu;dispatch.gpr[4]=actor;dispatch.gpr[2]=NATIVE_AI;
                redirects[shots](&dispatch,shots?0x80021518u:0x80021300u);
                CHECK(dispatch.gpr[2]==ACTOR_DISPATCH);
                actor_dispatch(&dispatch,ACTOR_DISPATCH);
                CHECK(!projected && !psx_mod_read_byte(MMX4_PLAYER+0xBC));
            }
            return 0x1234u;
        }
        ++ai_updates;
        observed_target_x=psx_mod_read_word(MMX4_PLAYER+8);
        observed_target_y=psx_mod_read_word(MMX4_PLAYER+12);
        observed_freeze=psx_mod_read_byte(MMX4_PLAYER+0xBC);
        return 0x1234u;
    }
    if(address==0x8002B3C0u) {++bounds_updates;return 0;}
    if(address==NATIVE_AI) {
        CHECK(mmx4_coop_combat_actor_context());
        ++actor_updates[seat];
        ai_target_x[seat]=psx_mod_read_word(MMX4_PLAYER+8);
        if(dragoon_release_model) {
            psx_mod_write_byte(a0+5,3);return 0;
        }
        if(sigma_release_model) {
            /* Native8F884 retires the actor after releasing the scene and
             * writing the ending corridor targets. Publish before restore. */
            psx_mod_write_byte(a0,0);return 0;
        }
        if(palette_model) {
            if(psx_mod_read_byte(a0+1)==61 && !psx_mod_read_byte(a0+4)) {
                /* Original788B8 ->7B7B4:64 source colors, one shared
                 * destination block and one palette-upload request. */
                uint32_t source=psx_mod_read_word(0x1F800024u)+0x6A0u;
                uint32_t destination=psx_mod_read_word(0x1F800028u)+
                    (psx_mod_read_byte(MMX4_PLAY+12)==3?0x780u:0x580u);
                for(unsigned i=0;i<64;++i)
                    psx_mod_write_half(destination+i*2,psx_mod_read_half(source+i*2));
                psx_mod_write_byte(0x80166BB0u,psx_mod_read_byte(0x80166BB0u)|1u);
            }else if(seat) {
                CHECK(psx_mod_read_word(0x1F800024u)==PRIVATE_COLORS);
                CHECK(psx_mod_read_word(0x1F800028u)==PRIVATE_PALETTE);
            }
            return 0x12345678u;
        }
        if(owl_release_model) {
            unsigned i=owl_release_model-1;
            CPUState release={.gpr={[16]=a0,[17]=a0}};
            owl_release_hooks[i](&release,owl_release_sites[i]);
            psx_mod_write_byte(MMX4_PLAYER+0xBA,0); /* Retail SB follows hook. */
            return 0;
        }
        if(reaction_model) {
            uint32_t actor=cpu->gpr[4];
            CHECK(filter(HIT)(cpu,HIT));
            for(unsigned i=0;i<7;++i) {
                CPUState reaction={.gpr={[2]=psx_mod_read_byte(MMX4_PLAY+0x43),[16]=actor}};
                reaction_hooks[i](&reaction,reaction_sites[i]);
                CHECK(reaction.gpr[2]==reaction_expected);++reaction_checks;
                reaction.gpr[2]=9;reaction.gpr[16]=actor+0x9Cu;
                reaction_hooks[i](&reaction,reaction_sites[i]);CHECK(reaction.gpr[2]==9);
            }
            return 0;
        }
        if(double_model) {
            uint32_t result=double_tick();
            CHECK(projected==seat && mmx4_coop_combat_actor_context());
            return result;
        }
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==ai_expected_campaign);
        if(psx_mod_read_byte(a0+1)==22) {
            if(psx_mod_read_byte(a0+4)==2) {
                psx_mod_write_byte(MMX4_PLAYER+0xBA,0);
                psx_mod_write_byte(a0,0);return 0;
            }
            CHECK(!filter(CONTACT)(cpu,CONTACT)); /* Current seat owns continuation. */
            psx_mod_write_byte(MMX4_PLAYER+0xBA,1);
            psx_mod_write_word(a0+0x8C,3);
            return 0;
        }
        if(ai_direct_write)psx_mod_write_byte(MMX4_PLAYER+0x5C,
            psx_mod_read_byte(MMX4_PLAYER+0x5C)-1u);
        if(ai_contact)filter(CONTACT)(cpu,CONTACT);
        if(ai_solid)filter(SOLID)(cpu,SOLID);
        if(ai_canonical) {
            CHECK(mmx4_coop_combat_canonical_call(cpu,CONTACT,filter(CONTACT)));
        }
        CHECK(projected==seat && mmx4_coop_combat_actor_context());
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==ai_expected_campaign);
        return 0x12345678u;
    }
    CHECK(0);
    return 0;
}

static void clear_calls(void) {
    memset(native_calls,0,sizeof native_calls);
    consumed=ai_updates=allocator_calls=0;
    armor_updates=mount_checks=0;
}
static void setup(void) {
    memset(ram,0,sizeof ram);
    memset(scratchpad,0,sizeof scratchpad);
    second_assembly=saved_assembly=0;
    palette_model=0;
    memset(attached_effect_updates,0,sizeof attached_effect_updates);
    memset(second,0,sizeof second);
    memset(second_vehicle,0,sizeof second_vehicle);
    memset(second_double,0,sizeof second_double);
    memset(second_shots,0,sizeof second_shots);
    projected=split=0;ready=1;alive[0]=alive[1]=1;script_owner=-1;deferred_portal=arriving=0;
    pool_updates=bounds_updates=ai_contact=ai_solid=ai_direct_write=ai_canonical=0;
    ai_expected_campaign=0;
    memset(actor_updates,0,sizeof actor_updates);
    hit_result[0]=hit_result[1]=0;boss_immunity[0]=boss_immunity[1]=0;
    allocated_actor=ACTOR;
    mount_accept[0]=mount_accept[1]=0;
    token_model=double_model=local_view=reaction_model=0;attack_present[0]=attack_present[1]=0;
    owl_release_model=0;
    attack_type[0]=attack_type[1]=0;
    attack_token[0]=attack_token[1]=0;
    psx_mod_write_byte(MMX4_PLAY,6);
    psx_mod_write_byte(MMX4_PLAYER+4,1);
    psx_mod_write_byte(MMX4_PLAYER+0x5C,32);
    psx_mod_write_word(MMX4_PLAYER+8,10u<<16);
    psx_mod_write_word(MMX4_PLAYER+12,20u<<16);
    psx_mod_write_byte(DOUBLE_BODY,1);
    psx_mod_write_byte(MMX4_VEHICLE,1);
    psx_mod_write_word(MMX4_VEHICLE+8,10u<<16);
    second[2]=1;second[4]=1;second[0x5C]=32;
    second[8+2]=30;second[12+2]=20;
    second_vehicle[0]=1;second_vehicle[8+2]=30;second_double[0]=1;
    psx_mod_write_word(ACTOR+0x68,0x800F8000u);
    mmx4_coop_combat_reset();
    particle_initializations=particle_character=0;
    clear_calls();
}
static uint32_t invoke(uint32_t address) {
    CPUState cpu={0};cpu.gpr[4]=ACTOR;cpu.gpr[5]=0xAABBCCDDu;
    CHECK(filter(address)(&cpu,address));
    return cpu.gpr[2];
}
static void death_particle_ownership(void) {
    const uint32_t routines[]={PARTICLE_INIT,0x8009485Cu,0x80098474u};
    const uint8_t kinds[]={0x11,47,48};
    for(unsigned effect=0;effect<3;++effect)for(unsigned character=0;character<2;++character) {
        setup();second[2]=(uint8_t)character;
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)(1-character));
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)(1-character));
        second[0x14]=7;allocated_actor=PARTICLE;
        mmx4_coop_enter_second();CHECK(invoke(PARTICLE_ALLOC)==PARTICLE);mmx4_coop_leave_second();
        /* P2 is dead; it can even rejoin elsewhere before deferred init. */
        alive[1]=0;second[10]=99;second[14]=77;second[0x14]=8;
        psx_mod_write_byte(PARTICLE+1,kinds[effect]);
        uint8_t p1[sizeof second],p2[sizeof second];
        memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);memcpy(p2,second,sizeof p2);
        CPUState cpu={0};cpu.gpr[4]=PARTICLE;
        CHECK(filter(routines[effect])(&cpu,routines[effect]) && cpu.gpr[2]==0xABCDEF01u);
        CHECK(particle_initializations==1 && particle_character==character);
        CHECK(psx_mod_read_word(PARTICLE+8)==30u<<16 && psx_mod_read_word(PARTICLE+12)==20u<<16);
        CHECK(psx_mod_read_byte(PARTICLE+0x14)==7 && !projected);
        CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1) && !memcmp(p2,second,sizeof p2));
        CHECK(!filter(routines[effect])(&cpu,routines[effect]));
        /* A P1 allocation reusing the slot must clear its former P2 owner. */
        mmx4_coop_enter_second();invoke(PARTICLE_ALLOC);mmx4_coop_leave_second();
        invoke(PARTICLE_ALLOC);psx_mod_write_byte(PARTICLE+4,0);
        CHECK(!filter(PARTICLE_INIT)(&cpu,PARTICLE_INIT));
        /* Reset removes pending ownership, and unrelated effect types pass. */
        mmx4_coop_enter_second();invoke(PARTICLE_ALLOC);mmx4_coop_leave_second();
        psx_mod_write_byte(PARTICLE+1,0x12);CHECK(!filter(PARTICLE_INIT)(&cpu,PARTICLE_INIT));
        mmx4_coop_combat_reset();psx_mod_write_byte(PARTICLE+1,0x11);
        CHECK(!filter(PARTICLE_INIT)(&cpu,PARTICLE_INIT));
    }
}
static void train_pillar_ownership(void) {
    for(unsigned owner=0;owner<2;++owner) {
        setup();split=scene_camera_model=1;
        scene_camera_begins=scene_camera_ends=0;
        psx_mod_write_byte(MMX4_PLAY+12,8);psx_mod_write_byte(MMX4_PLAY+13,1);
        psx_mod_write_byte(MMX4_PLAYER+0x14,0);second[0x14]=1;
        psx_mod_write_word(0x1F800020u,0x80108000u);
        psx_mod_write_half(0x801419BAu,900);psx_mod_write_half(0x801419BEu,200);
        allocated_actor=PARTICLE;
        if(owner)mmx4_coop_enter_second();
        invoke(PARTICLE_ALLOC);
        if(owner)mmx4_coop_leave_second();
        psx_mod_write_byte(PARTICLE,0x41);psx_mod_write_byte(PARTICLE+1,20);
        psx_mod_write_word(PARTICLE+0x50,0x80169CB8u);
        psx_mod_write_byte(0x80169CB8u,1);psx_mod_write_byte(0x80169CB9u,2);
        uint8_t camera[0xFC],p1[0xE4],p2[0xE4];
        memcpy(camera,address_ptr(0x801419B0u),sizeof camera);
        memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);memcpy(p2,second,sizeof p2);
        CPUState cpu={0};cpu.gpr[4]=PARTICLE;
        CHECK(filter(TRAIN_PILLAR)(&cpu,TRAIN_PILLAR) && cpu.gpr[2]==0xABCDEF20u);
        CHECK(particle_initializations==1 && scene_camera_begins==1 && scene_camera_ends==1);
        CHECK(psx_mod_read_half(PARTICLE+10)==(owner?4500:1000)+352);
        CHECK(psx_mod_read_half(PARTICLE+14)==328 && psx_mod_read_byte(PARTICLE+0x14)==owner);
        CHECK(!memcmp(camera,address_ptr(0x801419B0u),sizeof camera));
        CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1) && !memcmp(p2,second,sizeof p2));
        CHECK(!projected && mmx4_coop_combat_snapshot_ready());
        CHECK(!filter(TRAIN_PILLAR)(&cpu,TRAIN_PILLAR)); /* Native live tick, once. */
        psx_mod_write_byte(PARTICLE+4,0);split=0;
        CHECK(!filter(TRAIN_PILLAR)(&cpu,TRAIN_PILLAR));
        split=1;psx_mod_write_byte(MMX4_PLAY+12,4);
        CHECK(!filter(TRAIN_PILLAR)(&cpu,TRAIN_PILLAR));
        psx_mod_write_byte(MMX4_PLAY+12,8);psx_mod_write_word(PARTICLE+0x50,0);
        CHECK(!filter(TRAIN_PILLAR)(&cpu,TRAIN_PILLAR));
        scene_camera_model=0;
    }
}
static void stored_charge_hud_ownership(void) {
    setup();allocated_actor=PARTICLE;
    psx_mod_write_byte(MMX4_PLAYER+2,1);second[2]=0;
    psx_mod_write_byte(MMX4_PLAYER+0xA6,7);second[0xA6]=4;
    mmx4_coop_enter_second();CHECK(invoke(PARTICLE_ALLOC)==PARTICLE);mmx4_coop_leave_second();
    psx_mod_write_byte(PARTICLE,0x21);psx_mod_write_byte(PARTICLE+1,0x36);
    CHECK(mmx4_coop_stored_charge_owner(PARTICLE)==1);
    uint8_t p1[sizeof second],p2[sizeof second];
    memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);memcpy(p2,second,sizeof p2);
    CPUState cpu={0};cpu.gpr[4]=PARTICLE;
    stock_hud_updates=0;
    CHECK(filter(STOCK_HUD)(&cpu,STOCK_HUD) && cpu.gpr[2]==0xABCDEF02u);
    CHECK(stock_hud_updates==1 && psx_mod_read_byte(PARTICLE+3)==1 && psx_mod_read_byte(PARTICLE+0x47)==27);
    CHECK(!projected && mmx4_coop_combat_snapshot_ready());
    CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1) && !memcmp(p2,second,sizeof p2));
    second[0xA6]=0;
    CHECK(filter(STOCK_HUD)(&cpu,STOCK_HUD) && !psx_mod_read_byte(PARTICLE+3));
    CHECK(psx_mod_read_byte(MMX4_PLAYER+0xA6)==7 && !second[0xA6]);
    /* Private draws never tick this controller, and unrelated/misaligned
     * actors cannot borrow an allocation owner's presentation. */
    local_view=1;CHECK(!filter(STOCK_HUD)(&cpu,STOCK_HUD));local_view=0;
    CHECK(mmx4_coop_stored_charge_owner(PARTICLE+1)==-1);
    CHECK(mmx4_coop_stored_charge_owner(PARTICLE-0x60)==-1);
    CHECK(mmx4_coop_stored_charge_owner(PARTICLE+64*0x60)==-1);
    psx_mod_write_byte(PARTICLE+1,0x35);CHECK(mmx4_coop_stored_charge_owner(PARTICLE)==-1);
    CHECK(!filter(STOCK_HUD)(&cpu,STOCK_HUD));
    /* Reallocation by P1 clears P2 ownership. P1 then uses the native
     * update unchanged; no additional simulation or charge consumption. */
    invoke(PARTICLE_ALLOC);psx_mod_write_byte(PARTICLE+1,0x36);
    CHECK(mmx4_coop_stored_charge_owner(PARTICLE)==0 && !filter(STOCK_HUD)(&cpu,STOCK_HUD));
}
static void accepted_events(void) {
    setup();boss_immunity[0]=1;hit_result[0]=1;hit_result[1]=2;
    CHECK(invoke(HIT)==2 && native_calls[0]==1 && native_calls[1]==1);
    setup();boss_immunity[0]=boss_immunity[1]=1;hit_result[0]=1;hit_result[1]=2;
    CHECK(!invoke(HIT) && native_calls[0]==1 && native_calls[1]==1);
    CHECK(invoke(CONTACT)==1); /* Offensive immunity never suppresses contact. */
    const uint32_t addresses[]={CONTACT,HIT};
    for (unsigned i=0;i<2;++i) {
        setup();hit_result[0]=1;hit_result[1]=2;
        CHECK(invoke(addresses[i])==1);
        CHECK(native_calls[0]==1 && !native_calls[1] && consumed==1);
        setup();hit_result[0]=UINT32_MAX;hit_result[1]=2;
        CHECK(invoke(addresses[i])==UINT32_MAX);
        CHECK(native_calls[0]==1 && !native_calls[1] && consumed==1);
        setup();hit_result[1]=2;
        CHECK(invoke(addresses[i])==2);
        CHECK(native_calls[0]==1 && native_calls[1]==1 && consumed==1);
        CHECK(!projected);
        if (!i) CHECK(psx_mod_read_byte(MMX4_PLAYER+0x5C)==32 && second[0x5C]==31);
        setup();alive[0]=0;hit_result[0]=1;hit_result[1]=2;
        CHECK(invoke(addresses[i])==2);
        CHECK(!native_calls[0] && native_calls[1]==1 && consumed==1);
        setup();alive[1]=0;
        CHECK(!invoke(addresses[i]));
        CHECK(native_calls[0]==1 && !native_calls[1] && !consumed);
    }
}
static void independent_solids(void) {
    setup();alive[1]=0;arriving=1;second[0]=second[3]=1;
    second[0x68]=0;second[0x69]=0x80;second[0x6A]=0x0F;second[0x6B]=0x80;
    invoke(SOLID);CHECK(native_calls[0]==1 && native_calls[1]==1);
    clear_calls();second[0x5C]=0;invoke(SOLID);CHECK(!native_calls[1]);
    clear_calls();second[0x5C]=32;arriving=0;invoke(SOLID);CHECK(!native_calls[1]);
    setup();deferred_portal=ACTOR;
    mmx4_coop_enter_second();
    CHECK(invoke(SOLID)==0 && !native_calls[0] && !native_calls[1]);
    CHECK(projected);
    mmx4_coop_leave_second();deferred_portal=0;
    invoke(SOLID);
    CHECK(native_calls[0]==1 && native_calls[1]==1);
    setup();
    uint32_t empty=mmx4_coop_combat_digest(2166136261u);
    psx_mod_write_byte(ACTOR+0x72,3);psx_mod_write_byte(ACTOR+0x76,4);
    psx_mod_write_byte(ACTOR+0x74,5);psx_mod_write_byte(ACTOR+0x78,6);
    CHECK(invoke(SOLID)==0x12345678u);
    CHECK(native_calls[0]==1 && native_calls[1]==1 && !ai_updates);
    CHECK(observed_sides[0]==3 && observed_carried[0]==4);
    CHECK(!observed_sides[1] && !observed_carried[1]);
    CHECK(!observed_vehicle_sides[1] && !observed_vehicle_carried[1]);
    CHECK(observed_double[0]==1 && !observed_double[1]);
    CHECK(observed_vehicle[0]==1 && observed_vehicle[1]==1);
    CHECK(psx_mod_read_byte(DOUBLE_BODY)==1 && second_double[0]==1);
    CHECK(psx_mod_read_byte(ACTOR+0x72)==0x10 && psx_mod_read_byte(ACTOR+0x76)==0x20);
    CHECK(psx_mod_read_byte(ACTOR+0x74)==0x30 && psx_mod_read_byte(ACTOR+0x78)==0x40);
    CHECK(mmx4_coop_solid_contact_bits(ACTOR,0)==0x10);
    CHECK(mmx4_coop_solid_contact_bits(ACTOR,1)==0x11);
    CHECK(!mmx4_coop_solid_contact_bits(ACTOR,2));
    CHECK(psx_mod_read_word(MMX4_PLAYER+8)==12u<<16 && second[10]==32);
    CHECK(psx_mod_read_word(MMX4_VEHICLE+8)==13u<<16 && second_vehicle[10]==33);
    uint32_t populated=mmx4_coop_combat_digest(2166136261u);
    CHECK(populated!=empty);
    clear_calls();invoke(SOLID);
    CHECK(observed_sides[1]==0x11 && observed_carried[1]==0x21);
    CHECK(observed_vehicle_sides[1]==0x31 && observed_vehicle_carried[1]==0x41);
    CHECK(populated==mmx4_coop_combat_digest(2166136261u));
    /* A falling solid explicitly disables carry before the P1 pass. Its
     * new P1 contact result must not erase that instruction for P2, even
     * when P2 had a positive carry history on the previous world tick. */
    clear_calls();psx_mod_write_byte(ACTOR+0x76,255);invoke(SOLID);
    CHECK(observed_carried[0]==255 && observed_carried[1]==255);
    CHECK(psx_mod_read_byte(ACTOR+0x76)==0x20);
    clear_calls();invoke(SOLID);
    CHECK(observed_carried[1]==0x21); /* No permanent carry disable. */
    /* Reuse from a private player allocation must clear prior solid history. */
    mmx4_coop_enter_second();CHECK(invoke(ALLOC)==ACTOR);mmx4_coop_leave_second();
    CHECK(allocator_calls==1 && empty==mmx4_coop_combat_digest(2166136261u));
    CHECK(!mmx4_coop_solid_contact_bits(ACTOR,1));
    invoke(SOLID);CHECK(!observed_sides[1] && !observed_vehicle_sides[1]);
    mmx4_coop_combat_reset();
    CHECK(empty==mmx4_coop_combat_digest(2166136261u));
    invoke(SOLID);CHECK(!observed_sides[1]);
}
static void aim_selection(void) {
    setup();psx_mod_write_word(ACTOR+8,29u<<16);psx_mod_write_word(ACTOR+12,20u<<16);
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8), y=psx_mod_read_word(MMX4_PLAYER+12);
    CHECK(invoke(AIM)==0x87654321u && ai_updates==1);
    CHECK(observed_target_x==30u<<16 && observed_target_y==20u<<16);
    CHECK(psx_mod_read_word(MMX4_PLAYER+8)==x && psx_mod_read_word(MMX4_PLAYER+12)==y);
    /* A tie declines the filter so original dispatch uses P1 once. */
    clear_calls();psx_mod_write_word(ACTOR+8,20u<<16);
    CPUState cpu={0};cpu.gpr[4]=ACTOR;
    CHECK(!filter(AIM)(&cpu,AIM) && !ai_updates);
    alive[0]=0;
    CHECK(invoke(AIM)==0x87654321u && ai_updates==1);
    CHECK(observed_target_x==30u<<16 && psx_mod_read_word(MMX4_PLAYER+8)==x);
    clear_calls();alive[1]=0;
    CHECK(!filter(AIM)(&cpu,AIM) && !ai_updates);
    setup();ready=0;
    CHECK(!filter(CONTACT)(&cpu,CONTACT));
    CHECK(!filter(SOLID)(&cpu,SOLID));
    CHECK(!filter(AIM)(&cpu,AIM));
    CHECK(!native_calls[0] && !native_calls[1] && !ai_updates);
}
/* Root exports this private interface and calls it before restoring P1. */
void mmx4_coop_combat_project_end(uint32_t vehicle_mirror);
static void effect_ownership(void) {
    setup();
    uint32_t empty=mmx4_coop_combat_digest(2166136261u);
    allocated_actor=EFFECT;
    /* Enrollment creates the Chaser's following effects before ready(). */
    ready=0;mmx4_coop_enter_second();CHECK(invoke(EFFECT_ALLOC)==EFFECT);
    psx_mod_write_byte(EFFECT,0x41);
    psx_mod_write_byte(EFFECT+1,0x0D);
    CHECK(mmx4_coop_vehicle_effect_owner(EFFECT)==2);
    psx_mod_write_word(EFFECT+0x50,MMX4_VEHICLE);
    psx_mod_write_byte(EFFECT+0x70,0x41);
    psx_mod_write_word(EFFECT+0x70+0x50,MMX4_VEHICLE);
    mmx4_coop_combat_project_end(MIRROR);
    CHECK(psx_mod_read_word(EFFECT+0x50)==MIRROR);
    CHECK(psx_mod_read_word(EFFECT+0x70+0x50)==MMX4_VEHICLE);
    CHECK(empty!=mmx4_coop_combat_digest(2166136261u));
    /* An owned effect's nonvehicle source must not be rewritten. */
    psx_mod_write_word(EFFECT+0x50,0x800F1234u);
    mmx4_coop_combat_project_end(MIRROR);
    CHECK(psx_mod_read_word(EFFECT+0x50)==0x800F1234u);
    mmx4_coop_leave_second();
    /* P1 reuse clears ownership even before enrollment. */
    CHECK(invoke(EFFECT_ALLOC)==EFFECT);
    CHECK(empty==mmx4_coop_combat_digest(2166136261u));
    mmx4_coop_enter_second();psx_mod_write_word(EFFECT+0x50,MMX4_VEHICLE);
    mmx4_coop_combat_project_end(MIRROR);
    CHECK(psx_mod_read_word(EFFECT+0x50)==MMX4_VEHICLE);
    CHECK(invoke(EFFECT_ALLOC)==EFFECT);
    mmx4_coop_combat_reset();
    CHECK(empty!=mmx4_coop_combat_digest(2166136261u)); /* Projection reset is inert. */
    mmx4_coop_leave_second();mmx4_coop_combat_reset();
    CHECK(empty==mmx4_coop_combat_digest(2166136261u));
}
static uint32_t invoke_armor(void) {
    CPUState cpu={0};cpu.gpr[4]=MMX4_VEHICLE;
    CHECK(filter(ARMOR_UPDATE)(&cpu,ARMOR_UPDATE));
    return cpu.gpr[2];
}
static void setup_armor(void) {
    setup();memset(second_vehicle,0,sizeof second_vehicle);
    psx_mod_write_byte(MMX4_VEHICLE+1,1);
    psx_mod_write_byte(MMX4_VEHICLE+4,1);
    psx_mod_write_byte(MMX4_VEHICLE+5,0);
    psx_mod_write_byte(MMX4_VEHICLE+6,1);
    psx_mod_write_byte(MMX4_VEHICLE+0x5C,17);
}
static void armor_handoff(void) {
    setup_armor();mount_accept[1]=1;
    invoke_armor();
    CHECK(armor_updates==1 && mount_checks==1);
    CHECK(!psx_mod_read_byte(MMX4_VEHICLE) && second_vehicle[0]==1);
    CHECK(second[0xC5]==1 && !psx_mod_read_byte(MMX4_PLAYER+0xC5));
    CHECK(second_vehicle[0x5C]==17 && second_vehicle[10]==10);
    mmx4_coop_enter_second();invoke_armor();
    CHECK(armor_updates==1); /* No second advance on the transfer frame. */
    CPUState cpu={0};cpu.gpr[4]=MMX4_VEHICLE;
    CHECK(!filter(ARMOR_UPDATE)(&cpu,ARMOR_UPDATE));
    mmx4_coop_call(&cpu,ARMOR_UPDATE,MMX4_VEHICLE,0);
    CHECK(armor_updates==2);
    /* Native dismount hands the same remaining-HP actor back to the world. */
    psx_mod_write_byte(MMX4_PLAYER+0xC5,0);
    psx_mod_write_byte(MMX4_VEHICLE+0x97,0);
    psx_mod_write_byte(MMX4_VEHICLE+5,0);
    psx_mod_write_word(MMX4_VEHICLE+8,44u<<16);
    mmx4_coop_combat_project_end(MIRROR);mmx4_coop_leave_second();
    CHECK(psx_mod_read_byte(MMX4_VEHICLE)==1 && !second_vehicle[0]);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE+0x5C)==17);
    CHECK(psx_mod_read_word(MMX4_VEHICLE+8)==44u<<16);
    setup_armor();mount_accept[0]=mount_accept[1]=1;
    invoke_armor();CHECK(armor_updates==1 && !mount_checks);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE)==1 && !second_vehicle[0]);
    setup_armor();invoke_armor();
    CHECK(armor_updates==1 && mount_checks==1 && !second_vehicle[0]);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE)==1);
    setup_armor();second_vehicle[0]=1;mount_accept[1]=1;
    invoke_armor();CHECK(armor_updates==1 && !mount_checks);
}
static void armor_forced_dismount(void) {
    CPUState cpu={0};
    setup_armor();mount_accept[1]=1;invoke_armor();
    uint8_t first[0xE4];memcpy(first,address_ptr(MMX4_PLAYER),sizeof first);
    mmx4_coop_armor_dismount(&cpu,1);
    /* The root projection adapter commits this callback on leave. */
    mmx4_coop_enter_second();mmx4_coop_combat_project_end(MIRROR);mmx4_coop_leave_second();
    CHECK(!projected && !second[0xC5] && !second_vehicle[0]);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+0x5C)==17);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE+5)==0xB);
    CHECK(!(psx_mod_read_byte(MMX4_VEHICLE+0x97)&0x40));
    CHECK(!memcmp(first,address_ptr(MMX4_PLAYER),sizeof first));
    /* Preserve another actual world vehicle on forced private abandonment. */
    setup_armor();mount_accept[1]=1;invoke_armor();
    psx_mod_write_byte(MMX4_VEHICLE,1);psx_mod_write_byte(MMX4_VEHICLE+0x5C,29);
    mmx4_coop_enter_second();mmx4_coop_armor_dismount(&cpu,1);
    CHECK(projected && !mmx4_coop_armor_mounted());
    mmx4_coop_leave_second();CHECK(!second_vehicle[0]);
    CHECK(psx_mod_read_byte(MMX4_VEHICLE+0x5C)==29);
    /* Chasers and private presentation transactions must remain unchanged. */
    setup_armor();psx_mod_write_byte(MMX4_PLAYER+0xC5,255);
    psx_mod_write_byte(MMX4_VEHICLE+1,0);psx_mod_write_byte(MMX4_VEHICLE+0x97,0x40);
    mmx4_coop_armor_dismount(&cpu,0);CHECK(psx_mod_read_byte(MMX4_PLAYER+0xC5)==255);
    psx_mod_write_byte(MMX4_VEHICLE+1,1);local_view=1;
    mmx4_coop_armor_dismount(&cpu,0);CHECK(psx_mod_read_byte(MMX4_PLAYER+0xC5)==255);
    local_view=0;mmx4_coop_armor_dismount(&cpu,0);
    CHECK(!psx_mod_read_byte(MMX4_PLAYER+0xC5) && psx_mod_read_byte(MMX4_VEHICLE));
}
static void independent_hit_tokens(void) {
    setup();token_model=1;attack_present[0]=attack_present[1]=1;
    attack_token[0]=attack_token[1]=1;
    psx_mod_write_byte(ACTOR+0x5C,10);
    CHECK(invoke(HIT)==1 && consumed==1 && !native_calls[1]);
    /* P1's persistent token is a duplicate. P2's identical token is its own
     * independent accepted attack, and each remains suppressed afterward. */
    CHECK(invoke(HIT)==1 && consumed==2 && native_calls[1]==1);
    CHECK(!invoke(HIT) && consumed==2);
    CHECK(psx_mod_read_byte(ACTOR+0x5C)==8 && psx_mod_read_byte(ACTOR+0x65)==1);
    /* P1 missing must not reset P2's persistent attack history each frame. */
    attack_present[0]=0;
    CHECK(!invoke(HIT) && !invoke(HIT) && consumed==2);
    attack_token[1]=2;
    CHECK(invoke(HIT)==1 && consumed==3);
    CHECK(!invoke(HIT) && consumed==3);
    /* Native invulnerability is still shared and is never altered by the
     * history projection. Native DD04 clears each seat's token while blocked. */
    psx_mod_write_byte(ACTOR+0x61,5);
    CHECK(!invoke(HIT) && consumed==3 && psx_mod_read_byte(ACTOR+0x61)==5);
    psx_mod_write_byte(ACTOR+0x61,0);
    CHECK(invoke(HIT)==1 && consumed==4);
    invoke(ALLOC);attack_present[0]=1;
    CHECK(invoke(HIT)==1 && consumed==5); /* Reused target has no old tokens. */
    setup();token_model=1;attack_present[1]=1;attack_token[1]=0;
    CHECK(invoke(HIT)==1 && invoke(HIT)==1 && consumed==2); /* Native zero-token semantics. */
    for(unsigned seat=0;seat<2;++seat) {
        setup();token_model=1;attack_present[seat]=1;attack_token[seat]=1;
        CHECK(invoke(HIT)==1 && consumed==1);
        boss_immunity[seat]=1;
        CHECK(!invoke(HIT) && consumed==1); /* Held contact stays one hit. */
        attack_present[seat]=0;CHECK(!invoke(HIT)); /* The sword leaves contact. */
        attack_present[seat]=1;CHECK(!invoke(HIT) && consumed==1);
        boss_immunity[seat]=0;
        CHECK(invoke(HIT)==1 && consumed==2); /* Fresh same-key swing can hit. */
        CHECK(!invoke(HIT) && consumed==2);
        boss_immunity[seat]=1;CHECK(!invoke(HIT));boss_immunity[seat]=0;
        CHECK(!invoke(HIT) && consumed==2); /* No fresh gap: still same swing. */
    }
}
static void setup_split_actors(unsigned shots) {
    setup();split=target_camera_mode;
    uint32_t first=shots?0x8013F328u:0x8013BED0u;
    for(unsigned index=0;index<2;++index) {
        uint32_t actor=first+index*0x9Cu;
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+3,1);
        psx_mod_write_word(actor+8,(index?29u:11u)<<16);
        psx_mod_write_word(actor+12,20u<<16);
        psx_mod_write_word(actor+0x68,0x800F8000u);
    }
}
static void mushroom_shared_palette(void) {
    for(unsigned campaign=0;campaign<2;++campaign)
    for(unsigned mode=0;mode<2;++mode)for(unsigned owner=0;owner<2;++owner)
    for(unsigned fortress=0;fortress<2;++fortress)for(unsigned condition=0;condition<3;++condition) {
        target_camera_mode=mode;setup_split_actors(0);palette_model=1;
        const uint32_t actor=0x8013BED0u;
        psx_mod_write_byte(actor+0x9C,0);script_owner=(int)owner;
        psx_mod_write_byte(actor+1,condition==2?60:61);
        psx_mod_write_byte(actor+4,condition==1?1:0);
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);second[2]=(uint8_t)(campaign^1u);
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAY+12,fortress?12:3);
        psx_mod_write_word(0x1F800024u,SHARED_COLORS);
        psx_mod_write_word(0x1F800028u,SHARED_PALETTE);
        psx_mod_write_byte(0x80166BB0u,8);
        unsigned offset=fortress?0x580u:0x780u;
        for(unsigned i=0;i<64;++i) {
            psx_mod_write_half(SHARED_COLORS+0x6A0u+i*2,(uint16_t)(0x2100u+i));
            psx_mod_write_half(SHARED_PALETTE+offset+i*2,0x55AAu);
            psx_mod_write_half(PRIVATE_PALETTE+offset+i*2,0x6BB6u);
        }
        invoke(0x80021234u);
        for(unsigned i=0;i<64;++i) {
            CHECK(psx_mod_read_half(SHARED_PALETTE+offset+i*2)==
                (condition==0?0x2100u+i:0x55AAu));
            CHECK(psx_mod_read_half(PRIVATE_PALETTE+offset+i*2)==0x6BB6u);
        }
        CHECK(psx_mod_read_byte(0x80166BB0u)==(condition==0?9:8));
        CHECK(actor_updates[owner]==1 && !actor_updates[owner^1u]);
        CHECK(psx_mod_read_word(0x1F800024u)==SHARED_COLORS);
        CHECK(psx_mod_read_word(0x1F800028u)==SHARED_PALETTE);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign && !projected);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0x5C)==32 && second[0x5C]==32);
        CHECK(mmx4_coop_combat_snapshot_ready());
    }
    palette_model=0;
}
static void split_native_targets(void) {
    const uint32_t pools[]={0x80021234u,0x8002144Cu};
    for(unsigned admission=0;admission<2;++admission)
    for(unsigned camera_mode=0;camera_mode<2;++camera_mode) {
        target_camera_mode=camera_mode;setup_split_actors(0);
        psx_mod_write_byte(MMX4_PLAY+12,admission?4:3);
        psx_mod_write_byte(MMX4_PLAY+13,admission);
        psx_mod_write_byte(0x8013BED0u,0);
        psx_mod_write_byte(0x8013BED0u+0x9Cu+1,admission?65:33);
        psx_mod_write_byte(0x8013BED0u+0x9Cu+4,admission?1:0);
        psx_mod_write_byte(0x8013BED0u+0x9Cu+5,admission?2:0);
        psx_mod_write_half(0x801419BAu,1000);
        scene_camera_model=1;scene_camera_begins=scene_camera_ends=0;
        invoke(0x80021234u);
        CHECK(actor_updates[1]==1 && !projected);
        CHECK(scene_camera_begins==camera_mode && scene_camera_ends==camera_mode);
        CHECK(psx_mod_read_half(0x801419BAu)==1000);
        CHECK(mmx4_coop_combat_snapshot_ready());scene_camera_model=0;
    }
    for(target_camera_mode=0;target_camera_mode<2;++target_camera_mode)
    for(unsigned shots=0;shots<2;++shots) {
        setup_split_actors(shots);ai_direct_write=1;
        CHECK(invoke(pools[shots])==0x1234u);
        CHECK(pool_updates==1 && actor_updates[0]==1 && actor_updates[1]==1);
        CHECK(ai_target_x[0]==10u<<16 && ai_target_x[1]==30u<<16);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0x5C)==31 && second[0x5C]==31);
        CHECK(!projected && !mmx4_coop_combat_actor_context());
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==0);
        setup_split_actors(shots);psx_mod_write_byte(MMX4_PLAYER+0xBC,5);
        invoke(pools[shots]);
        CHECK(!actor_updates[0] && actor_updates[1]==1 && bounds_updates==1);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0xBC)==5);
        setup_split_actors(shots);second[0xBC]=5;invoke(pools[shots]);
        CHECK(actor_updates[0]==1 && !actor_updates[1] && bounds_updates==1);
        CHECK(second[0xBC]==5 && !psx_mod_read_byte(MMX4_PLAYER+0xBC));
        setup_split_actors(shots);alive[0]=0;invoke(pools[shots]);
        CHECK(!actor_updates[0] && actor_updates[1]==2);
        setup_split_actors(shots);alive[1]=0;invoke(pools[shots]);
        CHECK(actor_updates[0]==2 && !actor_updates[1]);
        setup_split_actors(shots);alive[0]=alive[1]=0;invoke(pools[shots]);
        CHECK(!actor_updates[0] && !actor_updates[1]);
        setup_split_actors(shots);ai_contact=1;hit_result[0]=1;invoke(pools[shots]);
        CHECK(native_calls[0]==2 && !native_calls[1] && consumed==2);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0x5C)==30 && second[0x5C]==32);
        setup_split_actors(shots);ai_contact=1;hit_result[1]=1;invoke(pools[shots]);
        CHECK(native_calls[0]==2 && native_calls[1]==2 && consumed==2);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0x5C)==32 && second[0x5C]==30);
        setup_split_actors(shots);ai_solid=1;invoke(pools[shots]);
        CHECK(native_calls[0]==2 && native_calls[1]==2);
        CHECK(psx_mod_read_word(MMX4_PLAYER+8)==14u<<16);
        CHECK(second[10]==34 && psx_mod_read_byte(MMX4_PLAY+0x43)==0);
        setup_split_actors(shots);ai_canonical=1;invoke(pools[shots]);
        CHECK(native_calls[0]==2 && native_calls[1]==2 && !projected);
        setup_split_actors(shots);
        uint32_t first=shots?0x8013F328u:0x8013BED0u;
        psx_mod_write_word(first+8,20u<<16);psx_mod_write_byte(first+0x9Cu,0);
        invoke(pools[shots]);CHECK(actor_updates[0]==1 && !actor_updates[1]);
        setup_split_actors(shots);script_owner=1;invoke(pools[shots]);
        CHECK(!actor_updates[0] && actor_updates[1]==2 && !projected);
    }
    setup();CPUState cpu={0};cpu.gpr[2]=NATIVE_AI;cpu.gpr[4]=0x8013BED0u;
    redirects[0](&cpu,0x80021300u);CHECK(cpu.gpr[2]==NATIVE_AI);
}
static void dragoon_camera_release(void) {
    const uint32_t actor=0x8013BED0u;
    for(unsigned seat=0;seat<2;++seat)for(unsigned gate=0;gate<4;++gate) {
        setup_split_actors(0);script_owner=(int)seat;
        psx_mod_write_byte(actor+0x9C,0);
        psx_mod_write_byte(actor+1,gate==1?61:65);
        psx_mod_write_byte(actor+4,gate==2?0:1);
        psx_mod_write_byte(actor+5,gate==3?3:2);
        dragoon_release_model=1;dragoon_release_calls=0;
        invoke(0x80021234u);
        CHECK(dragoon_release_calls==(gate==0));
        if(!gate)CHECK(dragoon_release_projected==seat);
        CHECK(!projected && mmx4_coop_combat_snapshot_ready());
        dragoon_release_model=0;
    }
}
static void sigma_victory_camera_release(void) {
    const uint32_t actor=0x8013BED0u;
    for(unsigned seat=0;seat<2;++seat)for(unsigned gate=0;gate<6;++gate) {
        setup_split_actors(0);script_owner=(int)seat;
        psx_mod_write_byte(actor+0x9C,0);
        psx_mod_write_byte(MMX4_PLAY+12,gate==4?11:12);
        psx_mod_write_byte(MMX4_PLAY+13,gate==5?0:1);
        psx_mod_write_byte(actor+1,gate==1?68:74);
        psx_mod_write_byte(actor+4,gate==2?1:2);
        psx_mod_write_byte(actor+5,gate==3?5:6);
        sigma_release_model=1;sigma_release_calls=0;
        invoke(0x80021234u);
        CHECK(sigma_release_calls==(gate==0));
        if(!gate)CHECK(sigma_release_projected==seat);
        CHECK(actor_updates[seat]==1 && !actor_updates[seat^1u]);
        CHECK(!projected && mmx4_coop_combat_snapshot_ready());
        sigma_release_model=0;
    }
}
static void owl_grab_release(void) {
    const uint32_t actor=0x8013BED0u;
    const uint8_t commands[]={0,1,0xFE,0xFF};
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned seat=0;seat<2;++seat)
    for(unsigned site=0;site<6;++site)for(unsigned c=0;c<sizeof commands;++c) {
        setup_split_actors(0);script_owner=(int)seat;owl_release_model=site+1;
        psx_mod_write_byte(actor+0x9C,0);psx_mod_write_byte(actor+1,60);
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);second[2]=(uint8_t)(campaign^1);
        psx_mod_write_byte(MMX4_PLAYER+0xBA,seat?commands[c]:1);
        second[0xBA]=seat?1:commands[c];
        invoke(0x80021234u);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0xBA)==(seat && commands[c]!=1?commands[c]:0));
        CHECK(second[0xBA]==(!seat && commands[c]!=1?commands[c]:0));
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0x5C)==32 && second[0x5C]==32);
        CHECK(actor_updates[seat]==1 && !actor_updates[seat^1] && !projected);
        CHECK(mmx4_coop_combat_snapshot_ready());
    }
    /* The callback cannot clear a capture outside Owl's shared AI update. */
    setup();second[0xBA]=1;
    CPUState release={.gpr={[16]=actor,[17]=actor}};
    for(unsigned i=0;i<6;++i)owl_release_hooks[i](&release,owl_release_sites[i]);
    CHECK(second[0xBA]==1);
    setup_split_actors(0);owl_release_model=1;script_owner=0;
    psx_mod_write_byte(actor+0x9C,0);psx_mod_write_byte(actor+1,57);second[0xBA]=1;
    invoke(0x80021234u);CHECK(second[0xBA]==1);
}
static void slash_intro_leader(void) {
    for(unsigned leader=0;leader<2;++leader) {
        setup_split_actors(0);
        psx_mod_write_byte(MMX4_PLAY+0x0C,8);psx_mod_write_byte(MMX4_PLAY+0x0D,1);
        const uint32_t actor=0x8013BED0u;
        psx_mod_write_byte(actor+0x9C,0);
        psx_mod_write_byte(actor+1,54);psx_mod_write_byte(actor+4,1);
        psx_mod_write_byte(actor+5,9);psx_mod_write_byte(actor+6,0);
        psx_mod_write_word(MMX4_PLAYER+8,(leader?9360u:9500u)<<16);
        second[10]=(uint8_t)(leader?9500u:9360u);second[11]=(uint8_t)((leader?9500u:9360u)>>8);
        psx_mod_write_word(actor+8,9312u<<16);
        invoke(0x80021234u);
        CHECK(actor_updates[leader]==1 && !actor_updates[leader^1]);
        CHECK(ai_target_x[leader]==9500u<<16 && !projected);
        /* Outside the entrance gate, regular fight targeting remains nearest. */
        memset(actor_updates,0,sizeof actor_updates);psx_mod_write_byte(actor+6,1);
        invoke(0x80021234u);
        CHECK(actor_updates[leader^1]==1 && !actor_updates[leader]);
        memset(actor_updates,0,sizeof actor_updates);psx_mod_write_byte(actor+6,0);
        script_owner=(int)(leader^1);invoke(0x80021234u);
        CHECK(actor_updates[leader^1]==1 && !actor_updates[leader]);
    }
}
static void intro_entry_leader(void) {
    const uint32_t actor=0x8013BED0u;
    for(unsigned leader=0;leader<2;++leader)
    for(unsigned campaign=0;campaign<2;++campaign)
    for(unsigned condition=0;condition<8;++condition) {
        setup_split_actors(0);
        ai_expected_campaign=campaign;
        psx_mod_write_byte(actor+0x9C,0);
        psx_mod_write_byte(MMX4_PLAY+12,condition==3?1:0);
        psx_mod_write_byte(MMX4_PLAY+13,condition==4?0:1);
        psx_mod_write_byte(actor+1,condition==5?7:8);
        psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+5,9);
        psx_mod_write_byte(actor+6,condition==1?1:0);
        if(condition==2)psx_mod_write_byte(actor+5,2);
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        second[2]=(uint8_t)(campaign^1u);
        unsigned first=leader?4282:4536,other=leader?4536:4282;
        psx_mod_write_word(MMX4_PLAYER+8,first<<16);
        second[10]=(uint8_t)other;second[11]=(uint8_t)(other>>8);
        psx_mod_write_word(actor+8,3936u<<16);
        if(condition==6)script_owner=(int)(leader^1u);
        if(condition==7)alive[leader]=0;
        invoke(0x80021234u);
        unsigned target=condition?leader^1u:leader;
        CHECK(actor_updates[target]==1 && !actor_updates[target^1u]);
        CHECK(ai_target_x[target]==((condition?4282u:4536u)<<16));
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        CHECK(!projected && mmx4_coop_combat_snapshot_ready());
    }
}
static void intro_swarm_leader(void) {
    const uint32_t actor=0x8013BED0u;
    for(unsigned leader=0;leader<2;++leader)
    for(unsigned subtype=3;subtype<=8;++subtype)
    for(unsigned condition=0;condition<9;++condition) {
        setup_split_actors(0);
        psx_mod_write_byte(actor+0x9C,0);
        psx_mod_write_byte(MMX4_PLAY+12,condition==1?1:0);
        psx_mod_write_byte(MMX4_PLAY+13,condition==2?1:0);
        psx_mod_write_byte(actor+1,condition==3?7:10);
        psx_mod_write_byte(actor+2,(uint8_t)(condition==4?9:subtype));
        psx_mod_write_byte(actor+4,condition==5?2:1);
        psx_mod_write_byte(actor+5,condition==6?6:2);
        psx_mod_write_word(MMX4_PLAYER+8,(leader?200u:400u)<<16);
        second[10]=(uint8_t)(leader?400u:200u);
        second[11]=(uint8_t)((leader?400u:200u)>>8);
        psx_mod_write_word(actor+8,40u<<16);
        if(condition==7)script_owner=(int)(leader^1u);
        if(condition==8)alive[leader]=0;
        invoke(0x80021234u);
        unsigned expected=condition?leader^1u:leader;
        CHECK(actor_updates[expected]==1 && !actor_updates[expected^1u]);
        CHECK(ai_target_x[expected]==((condition?200u:400u)<<16));
        CHECK(!projected && mmx4_coop_combat_snapshot_ready());
    }
}
static void vehicle_secondary_ownership(void) {
    for(unsigned seat=0;seat<2;++seat)for(unsigned type=56;type<=57;++type) {
        setup();const uint32_t actor=0x8013F328u;
        allocated_actor=actor;ready=0; /* Original enrollment creates exhaust. */
        if(seat)mmx4_coop_enter_second();
        CHECK(invoke(0x8002ACA4u)==actor);
        CHECK(mmx4_coop_secondary_allocation_owner(actor)==seat+1);
        if(seat)mmx4_coop_leave_second();
        ready=1;psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,(uint8_t)type);
        psx_mod_write_word(actor+8,(seat?10u:30u)<<16); /* Other rider is nearest. */
        CHECK(mmx4_coop_vehicle_effect_owner(actor)==seat+1);
        invoke(0x8002144Cu);
        CHECK(actor_updates[seat]==1 && !actor_updates[seat^1]);
        CHECK(ai_target_x[seat]==(seat?30u:10u)<<16 && !projected);
        alive[seat]=0;invoke(0x8002144Cu);
        CHECK(actor_updates[seat]==2); /* Native cleanup still advances. */
        if(!seat)mmx4_coop_enter_second();
        invoke(0x8002ACA4u);
        if(!seat)mmx4_coop_leave_second();
        CHECK(mmx4_coop_vehicle_effect_owner(actor)==(seat^1)+1);
        CHECK(mmx4_coop_secondary_allocation_owner(actor)==(seat^1)+1);
        psx_mod_write_byte(actor+1,1);CHECK(!mmx4_coop_vehicle_effect_owner(actor));
    }
}
static void spider_capture_ownership(void) {
    for(unsigned victim=0;victim<2;++victim) {
        setup();const uint32_t actor=0x8013F328u;
        /* Allocation target may differ from the player actually caught. */
        allocated_actor=actor;
        if(!victim)mmx4_coop_enter_second();
        invoke(0x8002ACA4u);
        if(!victim)mmx4_coop_leave_second();
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,22);
        psx_mod_write_byte(actor+4,1);
        psx_mod_write_word(actor+8,(victim?30u:10u)<<16);
        invoke(0x8002144Cu);
        CHECK(actor_updates[victim]==1 && !actor_updates[victim^1]);
        /* Moving the other player nearer cannot transfer the capture. */
        psx_mod_write_word(actor+8,(victim?10u:30u)<<16);
        invoke(0x8002144Cu);
        CHECK(actor_updates[victim]==2 && !actor_updates[victim^1]);
        alive[victim]=0;
        if(victim)second[0xBC]=4;else psx_mod_write_byte(MMX4_PLAYER+0xBC,4);
        invoke(0x8002144Cu);
        CHECK(actor_updates[victim]==3 && !actor_updates[victim^1]);
        CHECK(!psx_mod_read_byte(actor) && !second[0xBA] && !psx_mod_read_byte(MMX4_PLAYER+0xBA));
        CHECK(!projected);
    }
}
static void attached_dash_slide_effects(void) {
    for(unsigned campaign=0;campaign<2;++campaign)
    for(unsigned double_owner=0;double_owner<2;++double_owner)
    for(unsigned right=0;right<2;++right)
    for(unsigned dash=0;dash<2;++dash) {
        setup();unsigned character=campaign^1u;
        uint32_t address=dash?DASH_EFFECT:SLIDE_EFFECT;
        uint8_t *owner=double_owner?second_double:second;
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAYER+5,2); /* P1 stays idle while P2 moves. */
        second[2]=second_double[2]=(uint8_t)character;
        second[0xDE]=(uint8_t)double_owner;
        owner[5]=(uint8_t)(dash?12:11);owner[0x17]=(uint8_t)(dash?0x12:0x0F);
        owner[0x15]=(uint8_t)(right?0x40:0);
        owner[10]=(uint8_t)(double_owner?120:80);owner[14]=(uint8_t)(double_owner?60:40);
        const uint32_t primary_assembly=0x80115000u;
        second_assembly=0x80116000u;
        psx_mod_write_word(0x1F80001Cu,primary_assembly);
        psx_mod_write_word(primary_assembly+4,0x80);
        psx_mod_write_word(second_assembly+4,0x120);
        allocated_actor=EFFECT;
        mmx4_coop_enter_second();invoke(EFFECT_ALLOC);mmx4_coop_leave_second();
        /* Both native creators leave origin/assembly deferred at state0. */
        psx_mod_write_byte(EFFECT,0x21);psx_mod_write_byte(EFFECT+1,(uint8_t)dash);
        psx_mod_write_word(EFFECT+8,0x00001234u);psx_mod_write_word(EFFECT+12,0x00005678u);
        uint8_t p1[sizeof second],p2[sizeof second],p2_double[sizeof second_double];
        memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);
        memcpy(p2,second,sizeof p2);memcpy(p2_double,second_double,sizeof p2_double);
        CPUState update={.gpr={[4]=EFFECT}};
        CHECK(filter(address)(&update,address) && update.gpr[2]==0xAABBCCDDu);
        CHECK(attached_effect_updates[1][dash]==1 && !attached_effect_updates[0][dash] && !projected);
        int dx=dash?(character?36:32):10,dy=dash?(character?14:12):(character?25:17);
        int x=(double_owner?120:80)+(right?-dx:dx),y=(double_owner?60:40)+dy;
        CHECK(psx_mod_read_word(EFFECT+8)==((uint32_t)x<<16|0x1234u));
        CHECK(psx_mod_read_word(EFFECT+12)==((uint32_t)y<<16|0x5678u));
        CHECK(psx_mod_read_byte(EFFECT+0x15)==(right?0x40:0));
        CHECK(psx_mod_read_word(EFFECT+0x3C)==second_assembly+0x120);
        CHECK(psx_mod_read_word(0x1F80001Cu)==primary_assembly);
        CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1));
        CHECK(!memcmp(p2,second,sizeof p2) && !memcmp(p2_double,second_double,sizeof p2_double));
        /* Follow the allocating owner, not the nearer or idle canonical body.
         * A native pool tick updates this effect exactly once. */
        owner[10]+=7;psx_mod_write_word(MMX4_PLAYER+8,300u<<16);
        CHECK(filter(address)(&update,address));
        CHECK(attached_effect_updates[1][dash]==2 && !attached_effect_updates[0][dash]);
        CHECK(psx_mod_read_word(EFFECT+8)==((uint32_t)(x+7)<<16|0x1234u));
        CHECK(psx_mod_read_byte(EFFECT) && psx_mod_read_byte(EFFECT+3) && !projected);
        /* The owner's native pose controls cleanup even when P1 starts Dash. */
        owner[5]=2;owner[0x17]=0;psx_mod_write_byte(MMX4_PLAYER+0x17,0x12);
        CHECK(filter(address)(&update,address));
        CHECK(attached_effect_updates[1][dash]==3 && !attached_effect_updates[0][dash]);
        CHECK(dash?!psx_mod_read_byte(EFFECT):psx_mod_read_byte(EFFECT+4)==2);
        /* Reallocation by P1 clears the old P2 sidecar; the filter declines,
         * and the native canonical pass binds P1 resources without projecting. */
        invoke(EFFECT_ALLOC);psx_mod_write_byte(EFFECT,0x21);psx_mod_write_byte(EFFECT+4,0);
        CHECK(!filter(address)(&update,address));
        CHECK(mmx4_coop_call(&update,address,EFFECT,0)==0xAABBCCDDu);
        CHECK(attached_effect_updates[0][dash]==1 && !projected);
        CHECK(psx_mod_read_word(EFFECT+0x3C)==primary_assembly+0x80);
        CHECK(psx_mod_read_word(0x1F80001Cu)==primary_assembly);
    }
    /* Effect3 carries its allocated world origin and resource pointers.
     * No owner projection is installed for its shared native animation. */
    for(unsigned i=0;i<filter_count;++i)CHECK(filters[i].address!=0x800AF6A0u);
}
#define DOUBLE_ENEMY 0x8013BF6Cu
#define DOUBLE_TABLE 0x80108204u
#define DOUBLE_COOLDOWN 0x801060F0u
static void double_setup(unsigned zero_seat) {
    setup();double_model=1;
    psx_mod_write_byte(MMX4_PLAY+0x0C,11);
    psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)!zero_seat);
    second[2]=(uint8_t)zero_seat;
    psx_mod_write_byte(DOUBLE_ENEMY,1);psx_mod_write_byte(DOUBLE_ENEMY+1,73);
    psx_mod_write_byte(DOUBLE_ENEMY+4,1);psx_mod_write_byte(DOUBLE_ENEMY+5,3);
    psx_mod_write_byte(DOUBLE_ENEMY+0x5C,48);
    psx_mod_write_word(DOUBLE_ENEMY+0x58,DOUBLE_TABLE);
    /* Original 80108204: normal charged X7=5; authored Zero25=3,
     * Zero26..31=2 and Zero32=1 all have FF admission. Cooldown is all FF. */
    for(unsigned type=0;type<44;++type) {
        psx_mod_write_byte(DOUBLE_TABLE+type*2u,0xFF);
        psx_mod_write_byte(DOUBLE_COOLDOWN+type*2u,0xFF);
    }
    psx_mod_write_byte(DOUBLE_TABLE+14,0);psx_mod_write_byte(DOUBLE_TABLE+15,5);
    for(unsigned type=25;type<=32;++type)
        psx_mod_write_byte(DOUBLE_TABLE+type*2u+1u,(uint8_t)(type==25?3:type==32?1:2));
    attack_present[zero_seat]=1;attack_type[zero_seat]=26;attack_token[zero_seat]=3;
}
static uint32_t double_tick(void) {
    unsigned target=projected;
    CPUState hit={.gpr={[4]=DOUBLE_ENEMY}};
    CHECK(filter(HIT)(&hit,HIT) && projected==target);
    /* Original 8CD80 calls DD04 first, then consumes its one result and
     * advances the +8B cooldown. Its alternate table rejects every class. */
    uint8_t timer=psx_mod_read_byte(DOUBLE_ENEMY+0x8B);
    if(timer) {
        psx_mod_write_byte(DOUBLE_ENEMY+0x8B,(uint8_t)(timer-1));
        if(timer==1)psx_mod_write_word(DOUBLE_ENEMY+0x58,DOUBLE_TABLE);
    }else if(hit.gpr[2]==UINT32_MAX) {
        psx_mod_write_byte(DOUBLE_ENEMY+4,2);
        psx_mod_write_byte(DOUBLE_ENEMY+5,0);psx_mod_write_byte(DOUBLE_ENEMY+6,0);
    }else if(hit.gpr[2] && hit.gpr[2]!=0x7F) {
        psx_mod_write_word(DOUBLE_ENEMY+0x58,DOUBLE_COOLDOWN);
        psx_mod_write_byte(DOUBLE_ENEMY+0x8B,64);
    }
    return hit.gpr[2];
}
static void double_counterpart_damage(void) {
    for(unsigned spin=0;spin<3;++spin)
    for(unsigned zero=0;zero<2;++zero)for(unsigned survivor=0;survivor<2;++survivor) {
        double_setup(zero);if(survivor)alive[zero^1u]=0;
        unsigned type=spin==2?24u:spin?34u:26u;
        attack_type[zero]=(uint8_t)type;
        uint8_t original[88];memcpy(original,address_ptr(DOUBLE_TABLE),sizeof original);
        CHECK(double_tick()==type+1u && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==46);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x63)==0 && consumed==1);
        CHECK((zero?second[0xBD]:psx_mod_read_byte(MMX4_PLAYER+0xBD))==(type>=26));
        CHECK(psx_mod_read_word(DOUBLE_ENEMY+0x58)==DOUBLE_COOLDOWN);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x8B)==64);
        /* A fresh slash during native invulnerability is still deflected. */
        ++attack_token[zero];CHECK(double_tick()==0x7F);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==46 && consumed==1);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x8B)==63);
        attack_present[zero]=0;
        for(unsigned i=0;i<63;++i)CHECK(!double_tick());
        CHECK(psx_mod_read_word(DOUBLE_ENEMY+0x58)==DOUBLE_TABLE);
        attack_present[zero]=1;++attack_token[zero];
        CHECK(double_tick()==type+1u && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==44 && consumed==2);
        /* Repeated persistent attack tokens are rejected before admission. */
        psx_mod_write_word(DOUBLE_ENEMY+0x58,DOUBLE_TABLE);
        CHECK(!double_tick() && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==44 && consumed==2);
        CHECK(!memcmp(original,address_ptr(DOUBLE_TABLE),sizeof original));
        CHECK(!projected && mmx4_coop_combat_snapshot_ready());
    }
    for(unsigned type=24;type<=36;++type) {
        double_setup(1);attack_type[1]=(uint8_t)type;
        uint32_t result=double_tick();unsigned amount=type==25?3:type>=26 && type<=31?2:type==32?1:(type==24 || type==34)?2:0;
        CHECK(result==(amount?type+1u:0x7Fu));
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==48-amount);
    }
    /* The shared AI target may be X or Zero. Canonical collision handling
     * suspends that projection and binds each attack pool to its real body. */
    for(unsigned zero=0;zero<2;++zero)for(unsigned nearest=0;nearest<2;++nearest) {
        double_setup(zero);split=1;
        psx_mod_write_word(MMX4_PLAYER+8,10u<<16);second[10]=80;
        psx_mod_write_word(DOUBLE_ENEMY+8,(nearest?70u:20u)<<16);
        psx_mod_write_word(DOUBLE_ENEMY+12,20u<<16);
        CHECK(invoke(0x80021234u)==0x1234u);
        CHECK(actor_updates[nearest]==1 && !actor_updates[nearest^1u]);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==46 && consumed==1);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+2)==!zero && second[2]==zero && !projected);
        CHECK(mmx4_coop_combat_snapshot_ready());
    }
    /* A lethal accepted slash returns the native negative result; the
     * original caller enters its defeat state rather than an extra hit/call. */
    for(unsigned spin=0;spin<2;++spin)
    for(unsigned zero=0;zero<2;++zero)for(unsigned survivor=0;survivor<2;++survivor) {
        double_setup(zero);if(survivor)alive[zero^1u]=0;
        attack_type[zero]=(uint8_t)(spin?34:26);
        psx_mod_write_byte(DOUBLE_ENEMY+0x5C,2);
        CHECK(double_tick()==UINT32_MAX && !psx_mod_read_byte(DOUBLE_ENEMY+0x5C));
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+4)==2 && !psx_mod_read_byte(DOUBLE_ENEMY+5));
        CHECK(consumed==1 && native_calls[zero]==1 && !projected);
    }
    /* Admission stays retail for X, unrelated bosses/tables, invalid slots,
     * solo/not-ready, other sections and private render transactions. */
    for(unsigned excluded=0;excluded<8;++excluded) {
        double_setup(1);
        if(excluded==0)second[2]=0;
        if(excluded==1)psx_mod_write_byte(DOUBLE_ENEMY+1,66);
        if(excluded==2) {
            psx_mod_write_word(DOUBLE_ENEMY+0x58,DOUBLE_COOLDOWN);
        }
        if(excluded==3)psx_mod_write_byte(MMX4_PLAY+0x0D,1);
        if(excluded==4)local_view=1;
        if(excluded==5)psx_mod_write_byte(DOUBLE_ENEMY,0);
        if(excluded==6)psx_mod_write_byte(MMX4_PLAY+0x0C,10);
        if(excluded==7)psx_mod_write_byte(DOUBLE_TABLE+26*2u+1u,0);
        CHECK(double_tick()==0x7F && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==48 && !consumed);
    }
    double_setup(1);ready=0;
    CHECK(!filter(HIT)(&(CPUState){.gpr={[4]=DOUBLE_ENEMY}},HIT));
    CPUState direct={.gpr={[2]=0xFF,[4]=DOUBLE_ENEMY,[6]=26,[7]=DOUBLE_TABLE}};
    double_admission(&direct,0x8002DE84u);CHECK(direct.gpr[2]==0xFF);
    /* Charged X's accepted value/damage and native cooldown are unchanged. */
    double_setup(1);attack_present[1]=0;attack_present[0]=1;attack_type[0]=7;attack_token[0]=1;
    CHECK(double_tick()==8 && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==43 && consumed==1);
    CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x8B)==64);
    const uint8_t rejected_x[]={0,1,8,17,21,23};
    for(unsigned i=0;i<sizeof rejected_x;++i) {
        double_setup(1);attack_present[1]=0;attack_present[0]=1;attack_type[0]=rejected_x[i];
        CHECK(double_tick()==0x7F && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==48 && !consumed);
    }
}

static void iris_setup(unsigned x_seat,unsigned core) {
    setup();double_model=1;
    psx_mod_write_byte(MMX4_PLAY+0x0C,11);
    psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)x_seat);
    second[2]=(uint8_t)(x_seat^1u);
    psx_mod_write_byte(DOUBLE_ENEMY,1);psx_mod_write_byte(DOUBLE_ENEMY+1,66);
    psx_mod_write_byte(DOUBLE_ENEMY+2,(uint8_t)core);
    psx_mod_write_byte(DOUBLE_ENEMY+4,1);psx_mod_write_byte(DOUBLE_ENEMY+0x5C,48);
    uint32_t table=core?0x80107E84u:0x80107DFCu;
    psx_mod_write_word(DOUBLE_ENEMY+0x58,table);
    for(unsigned type=0;type<44;++type) {
        psx_mod_write_byte(table+type*2u,0xFF);
        psx_mod_write_byte(DOUBLE_COOLDOWN+type*2u,0xFF);
    }
    static const uint8_t zero_damage[]={2,2,3,3,3,2,2,3,1,2,2,4};
    for(unsigned type=24;type<=35;++type) {
        psx_mod_write_byte(table+type*2u,(uint8_t)(core?0:5));
        psx_mod_write_byte(table+type*2u+1u,core?zero_damage[type-24]:0);
    }
    attack_present[x_seat]=1;attack_type[x_seat]=0;attack_token[x_seat]=3;
}
static uint32_t iris_hit(void) {
    unsigned armor=!psx_mod_read_byte(DOUBLE_ENEMY+2);
    /* Iris gives armor127HP for collision, then restores the48HP meter.
     * Only her exposed core owns lasting health loss. */
    if(armor)psx_mod_write_byte(DOUBLE_ENEMY+0x5C,127);
    CPUState hit={.gpr={[4]=DOUBLE_ENEMY}};
    CHECK(filter(HIT)(&hit,HIT) && !projected);
    if(armor)psx_mod_write_byte(DOUBLE_ENEMY+0x5C,48);
    return hit.gpr[2];
}
static void iris_counterpart_damage(void) {
    for(unsigned core=0;core<2;++core)
    for(unsigned x=0;x<2;++x)for(unsigned survivor=0;survivor<2;++survivor)
    for(unsigned type=0;type<=23;++type) {
        iris_setup(x,core);if(survivor)alive[x^1u]=0;
        attack_type[x]=(uint8_t)type;
        uint32_t table=psx_mod_read_word(DOUBLE_ENEMY+0x58);
        uint8_t original[88];memcpy(original,address_ptr(table),sizeof original);
        unsigned damage=(type==0 || type==23)?1:(type>=19 && type<=21)?3:
            (type==5 || type==14 || type==22)?4:2;
        CHECK(iris_hit()==type+1u && consumed==1);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x63)==(core?0:5));
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==48-(core?damage:0));
        CHECK(!memcmp(original,address_ptr(table),sizeof original));
        CHECK(psx_mod_read_word(DOUBLE_ENEMY+0x58)==table);
        /* The same persistent attack token still cannot hit twice. */
        CHECK(!iris_hit() && consumed==1);
        /* Real cooldown masking stays closed, even for a fresh attack. */
        psx_mod_write_word(DOUBLE_ENEMY+0x58,DOUBLE_COOLDOWN);++attack_token[x];
        CHECK(iris_hit()==0x7F && consumed==1);
        CHECK(mmx4_coop_combat_snapshot_ready());
    }
    for(unsigned excluded=0;excluded<7;++excluded) {
        iris_setup(1,1);
        if(excluded==0)psx_mod_write_byte(DOUBLE_ENEMY+1,67);
        if(excluded==1)psx_mod_write_byte(DOUBLE_ENEMY+2,2);
        if(excluded==2)psx_mod_write_byte(MMX4_PLAY+0x0D,1);
        if(excluded==3)psx_mod_write_byte(MMX4_PLAY+0x0C,10);
        if(excluded==4)local_view=1;
        if(excluded==5)psx_mod_write_byte(DOUBLE_ENEMY,0);
        if(excluded==6)attack_type[1]=36;
        CHECK(iris_hit()==0x7F && !consumed);
        CHECK(psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==48);
    }
    /* Counterpart admission does not change Iris's authored Zero response. */
    iris_setup(0,1);attack_present[0]=0;attack_present[1]=1;
    attack_type[1]=26;attack_token[1]=3;
    CHECK(iris_hit()==27 && psx_mod_read_byte(DOUBLE_ENEMY+0x5C)==45);
    for(unsigned x=0;x<2;++x)for(unsigned survivor=0;survivor<2;++survivor) {
        iris_setup(x,1);if(survivor)alive[x^1u]=0;
        psx_mod_write_byte(DOUBLE_ENEMY+0x5C,1);
        CHECK(iris_hit()==UINT32_MAX && !psx_mod_read_byte(DOUBLE_ENEMY+0x5C));
        CHECK(consumed==1 && !projected && mmx4_coop_combat_snapshot_ready());
    }
}

static void aiming_beam_owner(void) {
    setup();const uint32_t beam=0x801435B0u;
    second[2]=0;second[0x93]=6;
    psx_mod_write_byte(MMX4_PLAYER+2,1);psx_mod_write_byte(MMX4_PLAYER+0x93,0);
    psx_mod_write_byte(MMX4_PLAY+0x43,1);
    psx_mod_write_byte(beam,1);psx_mod_write_byte(beam+1,9);
    psx_mod_write_word(beam+0x5C,0x8013BED0u);
    CPUState cpu={.gpr={[4]=beam}};
    uint8_t p1[sizeof second],p2[sizeof second];
    memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);memcpy(p2,second,sizeof p2);
    unsigned before=aiming_calls;
    CHECK(filter(0x800D67DCu)(&cpu,0x800D67DCu) && cpu.gpr[2]==0xCAFEu);
    CHECK(aiming_calls==before+1 && psx_mod_read_word(beam+8)==30u<<16);
    CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1) && !memcmp(p2,second,sizeof p2));
    CHECK(!projected && psx_mod_read_byte(MMX4_PLAY+0x43)==1 && mmx4_coop_combat_snapshot_ready());
    mmx4_coop_enter_second();CHECK(!filter(0x800D67DCu)(&cpu,0x800D67DCu));mmx4_coop_leave_second();
    second[2]=1;CHECK(!filter(0x800D67DCu)(&cpu,0x800D67DCu));second[2]=0;
    psx_mod_write_byte(beam+1,8);CHECK(!filter(0x800D67DCu)(&cpu,0x800D67DCu));
    psx_mod_write_byte(beam+1,9);cpu.gpr[4]=beam+4;
    CHECK(!filter(0x800D67DCu)(&cpu,0x800D67DCu));
    cpu.gpr[4]=0x801441B0u;CHECK(!filter(0x800D67DCu)(&cpu,0x800D67DCu));
}
int main(void) {
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned seat=0;seat<2;++seat) {
        setup();finishes=0;psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);
        second[2]=(uint8_t)(campaign^1u);psx_mod_write_byte(ACTOR+0x5C,1);
        hit_result[seat]=UINT32_MAX;
        CHECK(invoke(HIT)==UINT32_MAX);
        CHECK(finishes==1 && finishing_actor==ACTOR && finishing_character==(campaign^seat));
        setup();finishes=0;psx_mod_write_byte(ACTOR+0x5C,2);hit_result[seat]=1;
        CHECK(invoke(HIT)==1 && !finishes);
    }
    stored_charge_hud_ownership();
    aiming_beam_owner();
    accepted_events();independent_solids();aim_selection();effect_ownership();armor_handoff();armor_forced_dismount();independent_hit_tokens();
    split_native_targets();
    mushroom_shared_palette();
    double_counterpart_damage();
    iris_counterpart_damage();
    slash_intro_leader();
    intro_entry_leader();
    intro_swarm_leader();
    dragoon_camera_release();
    sigma_victory_camera_release();
    owl_grab_release();
    vehicle_secondary_ownership();
    spider_capture_ownership();
    attached_dash_slide_effects();
    death_particle_ownership();
    train_pillar_ownership();
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned target=0;target<2;++target) {
        setup();water_updates=0;
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);second[2]=(uint8_t)(campaign^1u);
        psx_mod_write_word(MMX4_PLAYER+8,20u<<16);second[10]=80;
        psx_mod_write_word(DOUBLE_BODY+8,30u<<16);second_double[10]=90;
        second[0x15]=second_double[0x15]=0x40;
        allocated_actor=EFFECT;mmx4_coop_enter_second();invoke(EFFECT_ALLOC);mmx4_coop_leave_second();
        psx_mod_write_byte(EFFECT,0x41);psx_mod_write_byte(EFFECT+1,7);
        psx_mod_write_byte(EFFECT+2,(uint8_t)(target*2));
        CPUState water={0};water.gpr[4]=EFFECT;
        CHECK(filter(WATER)(&water,WATER) && water.gpr[2]==0x76543210u && water_updates==1 && !projected);
        CHECK(psx_mod_read_word(EFFECT+8)==(target?90u:80u)<<16);
        CHECK(psx_mod_read_byte(EFFECT+0x15)==0x40);
        CHECK(psx_mod_read_word(MMX4_PLAYER+8)==20u<<16);
        /* Reusing this shared effect slot for P1 clears P2 ownership. */
        invoke(EFFECT_ALLOC);CHECK(!filter(WATER)(&(CPUState){.gpr={[4]=EFFECT}},WATER));
    }
    /* A deferred charged-fire attachment must follow its own parent shot,
     * even when the same guest slot contains the other hero's live attack.
     * Once its own parent disappears it expires instead of following P1. */
    setup();allocated_actor=EFFECT;
    psx_mod_write_byte(SHOTS,1);
    psx_mod_write_word(SHOTS+0x38,0x80101000u);
    psx_mod_write_word(SHOTS+0x3C,0x80102000u);
    psx_mod_write_word(SHOTS+8,10u<<16);
    mmx4_coop_enter_second();invoke(EFFECT_ALLOC);
    psx_mod_write_byte(SHOTS,1);
    psx_mod_write_word(SHOTS+0x38,0x80103000u);
    psx_mod_write_word(SHOTS+0x3C,0x80104000u);
    psx_mod_write_word(SHOTS+8,80u<<16);
    mmx4_coop_leave_second();
    psx_mod_write_byte(EFFECT,1);psx_mod_write_byte(EFFECT+1,26);
    psx_mod_write_word(EFFECT+0x50,SHOTS);
    CPUState attachment={.gpr={[4]=EFFECT}};
    CHECK(filter(SHOT_EFFECT)(&attachment,SHOT_EFFECT));
    CHECK(attachment.gpr[2]==0x10203041u && !projected);
    CHECK(psx_mod_read_word(EFFECT+0x38)==0x80103000u);
    CHECK(psx_mod_read_word(EFFECT+0x3C)==0x80104000u);
    CHECK(psx_mod_read_word(EFFECT+8)==80u<<16);
    CHECK(psx_mod_read_word(SHOTS+0x38)==0x80101000u);
    second_shots[0]=0;
    CHECK(filter(SHOT_EFFECT)(&attachment,SHOT_EFFECT));
    CHECK(!psx_mod_read_byte(EFFECT) && psx_mod_read_byte(SHOTS));
    invoke(EFFECT_ALLOC);CHECK(!filter(SHOT_EFFECT)(&attachment,SHOT_EFFECT));

    /* Ground Hunter's deferred one-shot graphics upload must bind X/P2 even
     * if that owner has died before the shared effect-pool update. */
    for(unsigned dead=0;dead<2;++dead) {
        setup();second[2]=0;psx_mod_write_byte(MMX4_PLAYER+2,1);
        psx_mod_write_word(MMX4_PLAYER+0x38,0x800F1234u);
        second[0x38]=0x78;second[0x39]=0x56;second[0x3A]=0x0F;second[0x3B]=0x80;
        allocated_actor=EFFECT;
        mmx4_coop_enter_second();invoke(EFFECT_ALLOC);mmx4_coop_leave_second();
        psx_mod_write_byte(EFFECT,1);psx_mod_write_byte(EFFECT+1,27);
        alive[1]=!dead;weapon_uploads=0;
        uint8_t p1[sizeof second],p2[sizeof second];
        memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);memcpy(p2,second,sizeof p2);
        CPUState upload={.gpr={[4]=EFFECT}};
        CHECK(filter(0x800B2A3Cu)(&upload,0x800B2A3Cu));
        CHECK(upload.gpr[2]==0x10203040u && weapon_uploads==1 && !projected);
        CHECK(!psx_mod_read_byte(EFFECT) && psx_mod_read_word(EFFECT+0x38)==0x800F5678u);
        CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1) && !memcmp(p2,second,sizeof p2));
        invoke(EFFECT_ALLOC);CHECK(!filter(0x800B2A3Cu)(&upload,0x800B2A3Cu));
    }
    /* P2 hover jets survive while the unrelated P1 is idle, follow the
     * allocating X, and expire when that owner leaves the hover pose. */
    setup();second[2]=0;psx_mod_write_byte(MMX4_PLAYER+2,1);
    second[0x47]=229;psx_mod_write_byte(MMX4_PLAYER+0x47,0);
    second_assembly=0x80112000u;psx_mod_write_word(0x1F80001Cu,0x80110000u);
    allocated_actor=EFFECT;mmx4_coop_enter_second();invoke(EFFECT_ALLOC);mmx4_coop_leave_second();
    psx_mod_write_word(0x80110030u,0x60);psx_mod_write_word(0x80112030u,0x80);
    psx_mod_write_byte(EFFECT,0x21);psx_mod_write_byte(EFFECT+1,33);
    CPUState hover={.gpr={[4]=EFFECT}};
    CHECK(filter(HOVER_EFFECT)(&hover,HOVER_EFFECT) && hover.gpr[2]==0x87654321u);
    CHECK(psx_mod_read_byte(EFFECT) && psx_mod_read_byte(EFFECT+3));
    CHECK(psx_mod_read_word(EFFECT+8)==30u<<16 && psx_mod_read_word(EFFECT+12)==20u<<16);
    CHECK(psx_mod_read_word(EFFECT+0x3C)==0x80112080u);
    CHECK(!projected && !psx_mod_read_byte(MMX4_PLAYER+0x47));
    CHECK(psx_mod_read_word(0x1F80001Cu)==0x80110000u);
    second[10]=80;
    CHECK(filter(HOVER_EFFECT)(&hover,HOVER_EFFECT) && psx_mod_read_word(EFFECT+8)==80u<<16);
    second[0x47]=0;
    CHECK(filter(HOVER_EFFECT)(&hover,HOVER_EFFECT) && !psx_mod_read_byte(EFFECT));
    invoke(EFFECT_ALLOC);CHECK(!filter(HOVER_EFFECT)(&hover,HOVER_EFFECT));

    /* X/P2's shared delayed release must use X's body and private pool,
     * without consuming Zero/P1's shot accounting. Reuse clears ownership. */
    setup();second[2]=0;psx_mod_write_byte(MMX4_PLAYER+2,1);
    second[0x98]=1;psx_mod_write_byte(MMX4_PLAYER+0x98,2);charge_updates=0;
    allocated_actor=EFFECT;mmx4_coop_enter_second();invoke(EFFECT_ALLOC);mmx4_coop_leave_second();
    psx_mod_write_byte(EFFECT,0x21);psx_mod_write_byte(EFFECT+1,2);
    CPUState charge={0};charge.gpr[4]=EFFECT;
    CHECK(filter(0x800AEED8u)(&charge,0x800AEED8u) && charge.gpr[2]==0x12345678u);
    CHECK(charge_updates==1 && !second[0x98] && psx_mod_read_byte(MMX4_PLAYER+0x98)==2);
    CHECK(psx_mod_read_word(EFFECT+8)==30u<<16 && !projected);
    /* Cancelling P2's emitter preserves P1's emitter and both water actors. */
    psx_mod_write_byte(EFFECT+0x70,0x21);psx_mod_write_byte(EFFECT+0x71,2);
    psx_mod_write_byte(EFFECT+0xE0,0x41);psx_mod_write_byte(EFFECT+0xE1,7);
    mmx4_coop_enter_second();mmx4_coop_combat_clear_current_effects();mmx4_coop_leave_second();
    CHECK(!psx_mod_read_byte(EFFECT) && psx_mod_read_byte(EFFECT+0x70) && psx_mod_read_byte(EFFECT+0xE0));
    mmx4_coop_combat_clear_current_effects();CHECK(!psx_mod_read_byte(EFFECT+0x70));
    CHECK(psx_mod_read_byte(EFFECT+0xE0));
    invoke(EFFECT_ALLOC);CHECK(!filter(0x800AEED8u)(&charge,0x800AEED8u));

    /* A nonlethal saber hit pauses its owner. The other seat continues;
     * accepted-hit callbacks must not repeatedly arm that owner's +BD while
     * the original player routine is counting down +BC. */
    setup();hit_result[1]=1;second[0xBC]=5;
    CHECK(!invoke(HIT) && native_calls[0]==1 && !native_calls[1]);
    CHECK(second[0xBC]==5 && !second[0xBD] && !consumed);
    hit_result[0]=1;
    CHECK(invoke(HIT)==1 && consumed==1 && !native_calls[1]);
    hit_result[0]=0;second[0xBC]=0;
    CHECK(invoke(HIT)==1 && native_calls[1]==1 && consumed==2);
    psx_mod_write_byte(MMX4_PLAYER+0xBC,5);second[0xBC]=0;
    CHECK(invoke(HIT)==1 && native_calls[0]==3 && native_calls[1]==2);
    /* The native caller chooses reactions after the hit wrapper has restored
     * its original AI projection. Hit owner, AI owner and campaign may all
     * differ. Misses/rejected attacks cannot reuse a previous character. */
    for(unsigned campaign=0;campaign<2;++campaign)for(unsigned ai=0;ai<2;++ai)
    for(unsigned hitter=0;hitter<2;++hitter)for(unsigned outcome=0;outcome<3;++outcome) {
        setup();reaction_model=1;reaction_checks=0;
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)campaign);second[2]=(uint8_t)(campaign^1u);
        uint32_t enemy=0x8013BED0u;
        psx_mod_write_byte(enemy,1);psx_mod_write_byte(enemy+1,61);
        psx_mod_write_word(enemy+8,(ai?30u:10u)<<16);
        psx_mod_write_word(enemy+12,(ai?40u:20u)<<16);
        hit_result[hitter]=outcome==0?7u:outcome==1?0u:0x7Fu;
        reaction_expected=outcome==0?(campaign^hitter):campaign;
        invoke(0x80021234u);
        CHECK(reaction_checks==7 && actor_updates[ai]==1);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign && !projected);
        CHECK(mmx4_coop_combat_snapshot_ready());
        CPUState outside={.gpr={[2]=9,[16]=enemy}};
        reaction_hooks[0](&outside,reaction_sites[0]);CHECK(outside.gpr[2]==9);
    }
    puts("X4 co-op combat boundary checks passed");
    return 0;
}
