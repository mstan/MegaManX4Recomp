/* Ownership-boundary tests with a small deterministic native-call model.
 * These prove projection/lifecycle bookkeeping, not original game behavior;
 * the original executable stage/menu/vehicle matrix remains required. */
#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static uint8_t ram[0x200000],p2[0xE4],p1_backup[0xE4];
static uint8_t p2_vehicle[0xB0],p1_vehicle_backup[0xB0];
static unsigned projected,ready=1,world_calls,observed_campaign,collect_calls[2];
static unsigned pause_commit,pause_exit;
static unsigned stage_script_calls,scene_calls,observed_controls;
static uint8_t p2_solid_bits;
static uint16_t inputs[2];
typedef struct { uint32_t address;PSXModFunctionFilterCallback filter; } Filter;
static Filter filters[32];static unsigned filter_count;
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
    (void)id;assert(filter_count<32);filters[filter_count++]=(Filter){a,f};return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,PSXModFunctionEntryCallback f) {
    (void)id;(void)a;(void)f;return 1;
}
void psx_mod_counter_add(const char *name,uint32_t amount) {(void)name;(void)amount;}
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat) {(void)cpu;(void)seat;}
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat) {
    return seat?p2_solid_bits:psx_mod_read_byte(actor+0x72);
}
int mmx4_coop_ready(void) {return ready;}
int mmx4_coop_projected(void) {return (int)projected;}
uint8_t *mmx4_coop_second_body(void) {return p2;}
uint8_t *mmx4_coop_second_vehicle(void) {return p2_vehicle;}
void mmx4_coop_clear_current_attacks(void) {}
uint16_t mmx4_coop_input(unsigned seat) {return inputs[seat];}
int mmx4_coop_finish(CPUState *cpu,uint32_t v) {cpu->gpr[2]=v;return 1;}
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
    switch(address) {
    case 0x8001FF8C:
        if(psx_mod_read_half(0x80166C0Cu)&0x800u)psx_mod_write_byte(MMX4_PLAY+1,2);
        else if(psx_mod_read_byte(MMX4_PLAYER+4)==3)psx_mod_write_byte(MMX4_PLAY+1,1);
        else ++world_calls;
        break;
    case 0x80021158:
        ++world_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);break;
    case 0x8002FCAC:
        observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(pause_commit) {
            psx_mod_write_byte(MMX4_PLAYER+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAYER+0x5C)+1));
            psx_mod_write_byte(MMX4_PLAY+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAY+0x5C)-1));
        }
        if(pause_exit) {
            mmx4_coop_call(cpu,0x80021158u,0,0);
            psx_mod_write_byte(MMX4_PLAY+1,0);
        }
        break;
    case 0x800C00BC:
        ++collect_calls[projected?1:0];
        if(psx_mod_read_word(MMX4_PLAYER+8)==psx_mod_read_word(a0+8)) {
            psx_mod_write_byte(a0+4,2);psx_mod_write_byte(a0+0x80,2);
            psx_mod_write_byte(MMX4_PLAY+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAY+0x5C)+1));
        }
        break;
    case 0x800BF730:
        if(psx_mod_read_byte(a0+4)==2) {
            psx_mod_write_byte(MMX4_PLAYER+0x5C,(uint8_t)(psx_mod_read_byte(MMX4_PLAYER+0x5C)+1));
            uint8_t n=(uint8_t)(psx_mod_read_byte(a0+0x80)-1);
            psx_mod_write_byte(a0+0x80,n);if(!n)psx_mod_write_byte(a0+4,3);
        }
        break;
    case 0x80035A6C:
        for(unsigned i=0x12;i<=0x1A;++i)psx_mod_write_byte(MMX4_PLAY+i,1);
        psx_mod_write_byte(MMX4_PLAY+0x1C,1);psx_mod_write_byte(MMX4_PLAYER+4,3);
        break;
    case 0x8001FA24:
        psx_mod_write_byte(MMX4_PLAY+0x59,5);psx_mod_write_byte(MMX4_PLAYER+0xB9,5);break;
    case 0x800C6EDC:
        psx_mod_write_byte(MMX4_PLAY+0x47,2);psx_mod_write_byte(MMX4_PLAYER+0xA7,2);break;
    case 0x8002166C:
        ++stage_script_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        mmx4_coop_call(cpu,0x80036AE4u,0x15,0x40);
        psx_mod_write_byte(MMX4_PLAY+0x1D,9);break;
    case 0x800C2BE0:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(psx_mod_read_byte(a0+0x72)&8u) {
            psx_mod_write_word(MMX4_PLAYER+8,psx_mod_read_word(a0+8));
            psx_mod_write_word(MMX4_PLAYER+12,psx_mod_read_word(a0+12));
            mmx4_coop_call(cpu,0x80036AE4u,0x14,0x40);
            psx_mod_write_byte(a0+5,1);
        }
        break;
    case 0x800BD654:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(psx_mod_read_word(MMX4_PLAYER+8)>=0x1000000u) {
            mmx4_coop_call(cpu,0x80036AE4u,0x15,0x40);
            psx_mod_write_byte(MMX4_PLAY+0x1D,8);
        }
        break;
    case 0x80036AE4:
        psx_mod_write_byte(MMX4_PLAYER+0xC0,1);psx_mod_write_byte(MMX4_PLAY+0x1C,1);break;
    case 0x80036B18:
        psx_mod_write_byte(MMX4_PLAYER+0xC0,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);break;
    case 0x800311EC:
        observed_controls=psx_mod_read_half(MMX4_PLAYER+0x7C);break;
    case 0x800C1E7C:
        result=psx_mod_read_word(MMX4_PLAYER+8)==psx_mod_read_word(a0+8);break;
    case 0x800C1994:
        ++scene_calls;observed_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
        if(!psx_mod_read_byte(a0+5) &&
           psx_mod_read_word(MMX4_PLAYER+8)==psx_mod_read_word(a0+8)) {
            psx_mod_write_byte(MMX4_PLAYER+0xC4,1);psx_mod_write_byte(a0+5,1);
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

#include "../src/mods/mmx4_coop_lifecycle.c"
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(void) {
    CPUState cpu={0};memset(ram,0,sizeof ram);memset(p2,0,sizeof p2);
    memset(p2_vehicle,0,sizeof p2_vehicle);
    fresh_game(&cpu,0);projected=0;inputs[0]=inputs[1]=0;
    world_calls=observed_campaign=pause_commit=pause_exit=0;
    stage_script_calls=scene_calls=observed_controls=p2_solid_bits=0;
    collect_calls[0]=collect_calls[1]=0;ready=1;
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+3,1);
    psx_mod_write_byte(MMX4_PLAYER+4,1);psx_mod_write_byte(MMX4_PLAYER+0x5C,16);
    p2[0]=p2[3]=p2[4]=1;p2[2]=1;p2[0x5C]=12;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+0x46,32);
    psx_mod_write_byte(MMX4_PLAY+0x44,2);
    mmx4_coop_enter_second();mmx4_coop_leave_second();
}
int main(void) {
    CPUState cpu={0};reset();
    /* A private upgrade must not overwrite P1 or the acquired shared pool. */
    psx_mod_write_byte(MMX4_PLAY+0x5C,0x89);psx_mod_write_half(MMX4_PLAY+0x5A,0x3000);
    mmx4_coop_enter_second();
    psx_mod_write_byte(MMX4_PLAY+0x46,34);psx_mod_write_byte(MMX4_PLAY+0x5A,4);
    psx_mod_write_byte(MMX4_PLAY+0x5C,0x88);mmx4_coop_lifecycle_project();
    mmx4_coop_leave_second();
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x46)==32);
    CHECK(psx_mod_read_half(MMX4_PLAY+0x5A)==0x3000);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x5C)==0x88);
    mmx4_coop_enter_second();CHECK(psx_mod_read_byte(MMX4_PLAY+0x46)==34);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x5A)==4);mmx4_coop_leave_second();
    uint32_t digest=mmx4_coop_lifecycle_digest(2166136261u);
    mmx4_coop_enter_second();mmx4_coop_leave_second();
    CHECK(digest==mmx4_coop_lifecycle_digest(2166136261u));
    mmx4_coop_enter_second();digest=mmx4_coop_lifecycle_digest(2166136261u);
    saved_inventory.max_hp=34;
    CHECK(digest!=mmx4_coop_lifecycle_digest(2166136261u));
    saved_inventory.max_hp=32;mmx4_coop_leave_second();
    /* Changing campaign rebuilds the counterpart's private campaign state. */
    psx_mod_write_byte(MMX4_PLAY+0x43,1);mmx4_coop_enter_second();
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==0);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x46)==32);mmx4_coop_leave_second();

    reset();inputs[0]=inputs[1]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(menu_active && menu_owner==0 && psx_mod_read_byte(PLAY+1)==2);
    reset();inputs[1]=START;mmx4_coop_call(&cpu,0x8001FF8Cu,PLAY,0);
    CHECK(menu_active && menu_owner==1 && psx_mod_read_byte(PLAY+1)==2);
    inputs[1]=0;pause_commit=1;psx_mod_write_byte(PLAY+0x5C,0x88);
    mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(p2[0x5C]==13 && psx_mod_read_byte(PLAYER+0x5C)==16);
    CHECK(psx_mod_read_byte(PLAY+0x5C)==0x87 && observed_campaign==1);
    pause_commit=0;pause_exit=1;mmx4_coop_call(&cpu,0x8002FCACu,MENU,0);
    CHECK(!menu_active && !projected && world_calls==1 && observed_campaign==0);

    /* A P2 collection heals P2 on later native actor ticks. */
    reset();uint32_t actor=0x8013D000u;
    psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+8,100);
    p2[8]=100;mmx4_coop_call(&cpu,0x800C00BCu,actor,0);
    CHECK(collect_calls[0]==1 && collect_calls[1]==1);
    mmx4_coop_call(&cpu,0x800BF730u,actor,0);
    CHECK(p2[0x5C]==13 && psx_mod_read_byte(PLAYER+0x5C)==16);
    mmx4_coop_call(&cpu,0x800BF730u,actor,0);CHECK(p2[0x5C]==14);
    CHECK(!pickup_record(actor,0));
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
    reset();p2[4]=2;p2[0x5C]=0;mmx4_coop_enter_second();
    mmx4_coop_call(&cpu,0x80035A6Cu,PLAYER,0);mmx4_coop_leave_second();
    CHECK(!psx_mod_read_byte(PLAY+0x12) && !psx_mod_read_byte(PLAY+0x1C));

    reset();mmx4_coop_call(&cpu,0x8001FA24u,PLAY,0);
    CHECK(psx_mod_read_byte(PLAYER+0xB9)==5 && p2[0xB9]==5);
    psx_mod_write_byte(PLAYER+2,1);psx_mod_write_byte(PLAY+0x43,1);p2[2]=0;
    mmx4_coop_call(&cpu,0x800C6EDCu,actor,0);
    CHECK(p2[0xA7]==2 && !psx_mod_read_byte(PLAYER+0xA7));
    CHECK(!psx_mod_read_byte(PLAY+0x47));

    reset();mmx4_coop_lifecycle_reset();p2[4]=3;p2[0x5C]=0;
    psx_mod_write_byte(PLAY+0x0D,1);mmx4_coop_lifecycle_reset();
    memset(p2,0,sizeof p2);p2[0]=p2[3]=p2[4]=1;p2[0x5C]=32;
    mmx4_coop_lifecycle_enrolled(&cpu);CHECK(p2[4]==3 && !p2[0x5C] && !p2[3]);
    full_stage(&cpu,0);mmx4_coop_lifecycle_reset();
    p2[4]=1;p2[3]=1;p2[0x5C]=32;mmx4_coop_lifecycle_enrolled(&cpu);
    CHECK(p2[4]==1 && p2[0x5C]==32);
    /* Native section mode 10 transitions through full loader mode 4 too;
     * retain a dead P1 when the same stage's section changed. */
    reset();mmx4_coop_lifecycle_reset();
    psx_mod_write_byte(PLAYER+4,3);psx_mod_write_byte(PLAYER+0x5C,0);
    psx_mod_write_byte(PLAY+0x0D,1);full_stage(&cpu,0);
    stage_initialization(&cpu,0);
    /* Original 8002A7D0 clears P1 before native 80035240; the callback
     * must retain the preceding death, not infer status from that storage. */
    psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_lifecycle_reset();
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
    mmx4_coop_lifecycle_enrolled(&cpu);
    CHECK(psx_mod_read_byte(PLAYER+4)==3 && !psx_mod_read_byte(PLAYER+0x5C));
    CHECK(p2[0x5C]==12 && p2[4]==1);

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
        mmx4_coop_lifecycle_enrolled(&cpu);
        CHECK(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+0x5C)==32);
        CHECK(p2[4]==1 && p2[0x5C]==12);
    }

    /* A native extra life does not make a section transfer revive a corpse. */
    reset();mmx4_coop_lifecycle_reset();p2[4]=3;p2[0x5C]=0;
    psx_mod_write_byte(PLAY+0x44,3);psx_mod_write_byte(PLAY+0x0D,1);
    full_stage(&cpu,0);stage_initialization(&cpu,0);
    psx_mod_write_byte(PLAYER+4,0);psx_mod_write_byte(PLAYER+0x5C,0);
    mmx4_coop_lifecycle_reset();CHECK(carry_resources && carry_dead);
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0x5C,32);
    p2[4]=1;p2[3]=1;p2[0x5C]=32;mmx4_coop_lifecycle_enrolled(&cpu);
    CHECK(p2[4]==3 && !p2[0x5C]);CHECK(psx_mod_read_byte(PLAYER+4)==1);

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
    CHECK(!projected);

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

    /* P2 may trigger a native portal first. Its scene sees the original
     * campaign, while only P2 receives native body/action writes. */
    reset();psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+8,0x02000000u);psx_mod_write_word(actor+12,0x03000000u);
    p2_solid_bits=8;mmx4_coop_call(&cpu,0x800C2BE0u,actor,0);
    CHECK(scene_calls==1 && observed_campaign==0 && script_owner==1 && script_active);
    CHECK(!psx_mod_read_byte(actor+0x72) && p2[0xC0]==1);
    CHECK(!psx_mod_read_byte(PLAYER+0xC0) && p2[0x5C]==12);
    mmx4_coop_lifecycle_tick(&cpu);
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

    /* Boss doors use C4 directly, not the general C0 script command pair.
     * Independent P2 entry must serialize the native auto-walk once. */
    reset();psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+8,0x02000000u);p2[11]=2;
    mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(scene_calls==1 && script_active && script_owner==1 && p2[0xC4]==1);
    CHECK(mmx4_coop_lifecycle_script_owner()==1);
    CHECK(!psx_mod_read_byte(PLAYER+0xC4) && observed_campaign==0);
    mmx4_coop_lifecycle_tick(&cpu);
    CHECK(!psx_mod_read_byte(PLAYER) && psx_mod_read_word(PLAYER+8)==0);
    psx_mod_write_byte(actor+5,3);mmx4_coop_call(&cpu,0x800C1994u,actor,0);
    CHECK(scene_calls==2 && !script_active && !p2[0xC4]);
    CHECK(mmx4_coop_lifecycle_script_owner()==-1);
    p2[0x89]=8;for(unsigned i=0;i<4;++i)mmx4_coop_lifecycle_tick(&cpu);
    CHECK(psx_mod_read_word(PLAYER+8)==0x02010000u);
    CHECK(!scene_record(actor,0));

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
    puts("co-op lifecycle ownership checks passed");return 0;
}
