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
#define MIRROR 0x80110000u
#define ARMOR_UPDATE 0x8003D3F8u
#define ARMOR_MOUNT 0x8003E0D0u
#define AIM 0x80042824u
#define ACTOR 0x80165A30u
#define DOUBLE_BODY 0x80175D58u
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);

typedef struct { uint32_t address; PSXModFunctionFilterCallback callback; } Filter;
static Filter filters[32];
static unsigned filter_count;
static PSXModFunctionEntryCallback reset_callback;
static uint8_t ram[0x200000];
static uint8_t second[0xE4], first_saved[0xE4];
static uint8_t second_vehicle[0xB0], first_vehicle[0xB0];
static uint8_t second_double[0xE4], first_double[0xE4];
static unsigned projected, ready, alive[2];
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
int psx_mod_register_function_entry_plugin(const char *id,uint32_t address,
                                          PSXModFunctionEntryCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && address==0x80035240u);
    reset_callback=callback;
    return 1;
}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t address,
                                           PSXModFunctionFilterCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && filter_count<32);
    filters[filter_count++]=(Filter){address,callback};
    return 1;
}
static PSXModFunctionFilterCallback filter(uint32_t address) {
    for (unsigned i=0;i<filter_count;++i)
        if (filters[i].address==address) return filters[i].callback;
    CHECK(0);
    return NULL;
}
int mmx4_coop_ready(void) { return (int)ready; }
int mmx4_coop_projected(void) { return (int)projected; }
int mmx4_coop_alive(unsigned seat) { CHECK(seat<2);return (int)alive[seat]; }
uint8_t *mmx4_coop_second_body(void) { return second; }
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
    projected=0;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result) { cpu->gpr[2]=result;return 1; }
uint16_t mmx4_coop_input(unsigned seat) { (void)seat;return 0; }

uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    (void)a1;
    /* Re-entered native dispatch must decline the active host filter. */
    if (address!=ARMOR_MOUNT) CHECK(!filter(address)(cpu,address));
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
        address==EFFECT_ALLOC) {
        ++allocator_calls;
        if (allocated_actor && address!=EFFECT_ALLOC)
            psx_mod_write_byte(allocated_actor+0x65,0);
        return allocated_actor;
    }
    if (address==AIM || address==0x80040CCCu || address==0x800419B8u) {
        ++ai_updates;
        observed_target_x=psx_mod_read_word(MMX4_PLAYER+8);
        observed_target_y=psx_mod_read_word(MMX4_PLAYER+12);
        return 0x87654321u;
    }
    if (address==0x80021234u || address==0x8002144Cu) {
        ++ai_updates;
        observed_target_x=psx_mod_read_word(MMX4_PLAYER+8);
        observed_target_y=psx_mod_read_word(MMX4_PLAYER+12);
        observed_freeze=psx_mod_read_byte(MMX4_PLAYER+0xBC);
        return 0x1234u;
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
    CHECK(reset_callback);
    memset(ram,0,sizeof ram);
    memset(second,0,sizeof second);
    memset(second_vehicle,0,sizeof second_vehicle);
    memset(second_double,0,sizeof second_double);
    projected=0;ready=1;alive[0]=alive[1]=1;
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
    second[4]=1;second[0x5C]=32;
    second[8+2]=30;second[12+2]=20;
    second_vehicle[0]=1;second_vehicle[8+2]=30;second_double[0]=1;
    psx_mod_write_word(ACTOR+0x68,0x800F8000u);
    CPUState cpu={0};reset_callback(&cpu,0x80035240u);
    clear_calls();
}
static uint32_t invoke(uint32_t address) {
    CPUState cpu={0};cpu.gpr[4]=ACTOR;cpu.gpr[5]=0xAABBCCDDu;
    CHECK(filter(address)(&cpu,address));
    return cpu.gpr[2];
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
    CPUState cpu={0};reset_callback(&cpu,0x80035240u);
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
    CPUState cpu={0};reset_callback(&cpu,0x80035240u);
    CHECK(empty!=mmx4_coop_combat_digest(2166136261u)); /* Projection reset is inert. */
    mmx4_coop_leave_second();reset_callback(&cpu,0x80035240u);
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
static void survivor_pools(void) {
    const uint32_t pools[]={0x80021234u,0x8002144Cu};
    for (unsigned i=0;i<2;++i) {
        setup();alive[0]=0;
        psx_mod_write_byte(MMX4_PLAYER+0xBC,5);
        CHECK(invoke(pools[i])==0x1234u && ai_updates==1);
        CHECK(observed_target_x==30u<<16 && observed_target_y==20u<<16);
        CHECK(!observed_freeze);
        CHECK(psx_mod_read_word(MMX4_PLAYER+8)==10u<<16);
        CHECK(psx_mod_read_word(MMX4_PLAYER+12)==20u<<16);
        CHECK(psx_mod_read_byte(MMX4_PLAYER+0xBC)==5);
        alive[0]=1;CPUState cpu={0};
        CHECK(!filter(pools[i])(&cpu,pools[i]) && ai_updates==1);
    }
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
int main(void) {
    CHECK(filter_count==13);
    accepted_events();independent_solids();aim_selection();effect_ownership();armor_handoff();survivor_pools();independent_hit_tokens();
    puts("X4 co-op combat boundary checks passed");
    return 0;
}
