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
#define PARTICLE 0x80173CA0u
#define WATER 0x800AF22Cu
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
static Filter filters[32];
static unsigned filter_count;
int psx_mod_local_view_scope(void) {return 0;}
static uint8_t ram[0x200000];
static uint8_t second[0xE4], first_saved[0xE4];
static uint8_t second_vehicle[0xB0], first_vehicle[0xB0];
static uint8_t second_double[0xE4], first_double[0xE4];
static unsigned projected, ready, alive[2],split;
static int script_owner=-1;
static unsigned target_camera_mode=1;
static PSXModFunctionEntryCallback redirects[2],actor_dispatch;
static unsigned pool_updates,actor_updates[2],bounds_updates,ai_contact,ai_solid;
static unsigned ai_direct_write,ai_canonical;
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
static unsigned particle_initializations,particle_character;
static unsigned water_updates;
static unsigned charge_updates;

static uint8_t *address_ptr(uint32_t address) {
    uint32_t physical=address&0x1FFFFFFFu;
    CHECK(physical<sizeof ram);
    return ram+physical;
}
uint8_t psx_mod_read_byte(uint32_t address) { return *address_ptr(address); }
void psx_mod_write_byte(uint32_t address,uint8_t value) { *address_ptr(address)=value; }
uint16_t psx_mod_read_half(uint32_t address) {
    return (uint16_t)(psx_mod_read_byte(address) |
        (uint16_t)psx_mod_read_byte(address+1)<<8);
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
    CHECK(!strcmp(id,"mmx4.coop") && word==0x0040F809u);
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
    memcpy(address_ptr(MMX4_PLAYER),second,sizeof second);
    memcpy(address_ptr(MMX4_VEHICLE),second_vehicle,sizeof second_vehicle);
    memcpy(address_ptr(DOUBLE_BODY),second_double,sizeof second_double);
    saved_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
    psx_mod_write_byte(MMX4_PLAY+0x43,second[2]);
    projected=1;
}
void mmx4_coop_leave_second(void) {
    CHECK(projected);
    memcpy(second,address_ptr(MMX4_PLAYER),sizeof second);
    memcpy(second_vehicle,address_ptr(MMX4_VEHICLE),sizeof second_vehicle);
    memcpy(second_double,address_ptr(DOUBLE_BODY),sizeof second_double);
    memcpy(address_ptr(MMX4_PLAYER),first_saved,sizeof first_saved);
    memcpy(address_ptr(MMX4_VEHICLE),first_vehicle,sizeof first_vehicle);
    memcpy(address_ptr(DOUBLE_BODY),first_double,sizeof first_double);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==second[2]);
    psx_mod_write_byte(MMX4_PLAY+0x43,saved_campaign);
    projected=0;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result) { cpu->gpr[2]=result;return 1; }
uint16_t mmx4_coop_input(unsigned seat) { (void)seat;return 0; }

uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    (void)a1;
    /* Re-entered native dispatch must decline the active host filter. */
    if (address!=ARMOR_MOUNT && address!=NATIVE_AI && address!=0x8002B3C0u)
        CHECK(!filter(address)(cpu,address));
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
    if(address==PARTICLE_INIT) {
        ++particle_initializations;particle_character=psx_mod_read_byte(MMX4_PLAYER+2);
        /* CAE38 reads these fields and P2's projected character assembly.
         * Its native movement/draw remains one call at the shared pool tick. */
        psx_mod_write_word(a0+8,psx_mod_read_word(MMX4_PLAYER+8));
        psx_mod_write_word(a0+12,psx_mod_read_word(MMX4_PLAYER+12));
        psx_mod_write_byte(a0+0x14,psx_mod_read_byte(MMX4_PLAYER+0x14));
        psx_mod_write_byte(a0+4,1);
        return 0xABCDEF01u;
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
         * update, rather than retaining an explicit following pointer. */
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
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==0);
        if(ai_direct_write)psx_mod_write_byte(MMX4_PLAYER+0x5C,
            psx_mod_read_byte(MMX4_PLAYER+0x5C)-1u);
        if(ai_contact)filter(CONTACT)(cpu,CONTACT);
        if(ai_solid)filter(SOLID)(cpu,SOLID);
        if(ai_canonical) {
            CHECK(mmx4_coop_combat_canonical_call(cpu,CONTACT,filter(CONTACT)));
        }
        CHECK(projected==seat && mmx4_coop_combat_actor_context());
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==0);
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
    memset(second,0,sizeof second);
    memset(second_vehicle,0,sizeof second_vehicle);
    memset(second_double,0,sizeof second_double);
    projected=split=0;ready=1;alive[0]=alive[1]=1;script_owner=-1;
    pool_updates=bounds_updates=ai_contact=ai_solid=ai_direct_write=ai_canonical=0;
    memset(actor_updates,0,sizeof actor_updates);
    hit_result[0]=hit_result[1]=0;
    allocated_actor=ACTOR;
    mount_accept[0]=mount_accept[1]=0;
    token_model=0;attack_present[0]=attack_present[1]=0;
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
    for(unsigned character=0;character<2;++character) {
        setup();second[2]=(uint8_t)character;
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)(1-character));
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)(1-character));
        second[0x14]=7;allocated_actor=PARTICLE;
        mmx4_coop_enter_second();CHECK(invoke(PARTICLE_ALLOC)==PARTICLE);mmx4_coop_leave_second();
        /* P2 is dead; it can even rejoin elsewhere before deferred init. */
        alive[1]=0;second[10]=99;second[14]=77;second[0x14]=8;
        psx_mod_write_byte(PARTICLE+1,0x11);
        uint8_t p1[sizeof second],p2[sizeof second];
        memcpy(p1,address_ptr(MMX4_PLAYER),sizeof p1);memcpy(p2,second,sizeof p2);
        CPUState cpu={0};cpu.gpr[4]=PARTICLE;
        CHECK(filter(PARTICLE_INIT)(&cpu,PARTICLE_INIT) && cpu.gpr[2]==0xABCDEF01u);
        CHECK(particle_initializations==1 && particle_character==character);
        CHECK(psx_mod_read_word(PARTICLE+8)==30u<<16 && psx_mod_read_word(PARTICLE+12)==20u<<16);
        CHECK(psx_mod_read_byte(PARTICLE+0x14)==7 && !projected);
        CHECK(!memcmp(p1,address_ptr(MMX4_PLAYER),sizeof p1) && !memcmp(p2,second,sizeof p2));
        CHECK(!filter(PARTICLE_INIT)(&cpu,PARTICLE_INIT));
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
static void accepted_events(void) {
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
    psx_mod_write_byte(MMX4_VEHICLE+5,9);
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
    psx_mod_write_byte(MMX4_VEHICLE+5,9);
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
static void split_native_targets(void) {
    const uint32_t pools[]={0x80021234u,0x8002144Cu};
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
int main(void) {
    CHECK(filter_count==17);
    accepted_events();independent_solids();aim_selection();effect_ownership();armor_handoff();independent_hit_tokens();
    split_native_targets();
    death_particle_ownership();
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
    puts("X4 co-op combat boundary checks passed");
    return 0;
}
