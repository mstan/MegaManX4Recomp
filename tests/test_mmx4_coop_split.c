#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value) do {if(!(value)) {fprintf(stderr,"line %d: %s\n",__LINE__,#value);exit(1);}} while(0)
#define CAMERA 0x801419B0u
#define TABLE 0x800F5000u
#define ACTOR 0x80170000u

static uint8_t ram[0x200000],second[0xE4],backup[0xE4];
static unsigned split=1,projected,local_scope,ready=1,capacity=16,allocations,scans;
static int script_owner=-1;
typedef struct {uint32_t address;PSXModFunctionFilterCallback callback;} Filter;
static Filter filters[16];static unsigned filter_count;
static PSXModFunctionEntryCallback spawned;
uint8_t psx_mod_read_byte(uint32_t address) {return ram[address&0x1FFFFFu];}
void psx_mod_write_byte(uint32_t address,uint8_t value) {ram[address&0x1FFFFFu]=value;}
uint16_t psx_mod_read_half(uint32_t address) {
    return psx_mod_read_byte(address)|(uint16_t)psx_mod_read_byte(address+1)<<8;
}
void psx_mod_write_half(uint32_t address,uint16_t value) {
    psx_mod_write_byte(address,(uint8_t)value);psx_mod_write_byte(address+1,(uint8_t)(value>>8));
}
uint32_t psx_mod_read_word(uint32_t address) {
    return psx_mod_read_half(address)|(uint32_t)psx_mod_read_half(address+2)<<16;
}
void psx_mod_write_word(uint32_t address,uint32_t value) {
    psx_mod_write_half(address,(uint16_t)value);psx_mod_write_half(address+2,(uint16_t)(value>>16));
}
uint32_t psx_mod_memory_alloc(uint32_t size,uint32_t alignment) {
    CHECK(size<4096 && alignment==4);return 0x801F0000u;
}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t address,PSXModFunctionFilterCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && filter_count<16);
    filters[filter_count++]=(Filter){address,callback};return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t address,PSXModFunctionEntryCallback callback) {
    (void)id;(void)address;(void)callback;return 1;
}
int psx_mod_register_instruction_plugin(const char *id,uint32_t address,uint32_t word,PSXModFunctionEntryCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && address==0x80029284u && word==0xA2620000u);
    spawned=callback;return 1;
}
void psx_mod_counter_add(const char *name,uint32_t amount) {(void)name;(void)amount;}
int32_t psx_mod_widescreen_x_margin(void) {return 0;}
int psx_mod_local_view_scope(void) {return (int)local_scope;}
int mmx4_coop_ready(void) {return (int)ready;}
int mmx4_coop_projected(void) {return (int)projected;}
int mmx4_coop_split_views(void) {return (int)split;}
uint8_t *mmx4_coop_second_body(void) {return second;}
uint8_t *mmx4_coop_first_body(void) {return backup;}
static unsigned actor_context;
int mmx4_coop_combat_actor_context(void) {return (int)actor_context;}
int mmx4_coop_lifecycle_script_owner(void) {return script_owner;}
void mmx4_coop_lifecycle_camera_bounds(CPUState *cpu,unsigned owner) {(void)cpu;(void)owner;}
int mmx4_coop_alive(unsigned seat) {
    const uint8_t *body=seat?second:backup;
    if(seat==projected)return psx_mod_read_byte(MMX4_PLAYER) && psx_mod_read_byte(MMX4_PLAYER+4)<2;
    return body[0] && body[4]<2;
}
void mmx4_coop_enter_second(void) {
    CHECK(!projected);
    for(unsigned offset=0;offset<sizeof second;++offset) {
        backup[offset]=psx_mod_read_byte(MMX4_PLAYER+offset);
        psx_mod_write_byte(MMX4_PLAYER+offset,second[offset]);
    }
    projected=1;
}
void mmx4_coop_leave_second(void) {
    CHECK(projected);
    for(unsigned offset=0;offset<sizeof second;++offset) {
        second[offset]=psx_mod_read_byte(MMX4_PLAYER+offset);
        psx_mod_write_byte(MMX4_PLAYER+offset,backup[offset]);
    }
    projected=0;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result) {cpu->gpr[2]=result;return 1;}
static void native_scan(CPUState *cpu);
static uint32_t invoke(CPUState *cpu,uint32_t address) {
    for(unsigned index=0;index<filter_count;++index)
        if(filters[index].address==address && filters[index].callback(cpu,address))return cpu->gpr[2];
    uint32_t actor=cpu->gpr[4];
    if(address==0x800293E8u)return allocations<capacity?0x80160000u+allocations++*0x100u:0;
    if(address==0x80028E24u) {
        CPUState saved=*cpu;++scans;
        int x=(int16_t)psx_mod_read_half(CAMERA+10),y=(int16_t)psx_mod_read_half(CAMERA+14);
        cpu->gpr[4]=(uint32_t)(x-48);cpu->gpr[5]=(uint32_t)(x+368);
        cpu->gpr[6]=(uint32_t)(y-48);cpu->gpr[7]=(uint32_t)(y+288);
        native_scan(cpu);*cpu=saved;return 0;
    }
    if(address==0x80027850u) {
        int x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10)-160;
        int y=(int16_t)psx_mod_read_half(MMX4_PLAYER+14)-128;
        psx_mod_write_word(CAMERA+0x14,psx_mod_read_word(CAMERA+8));
        psx_mod_write_word(CAMERA+0x18,psx_mod_read_word(CAMERA+12));
        if(x<0)x=0;if(y<0)y=0;
        int minimum=(int16_t)psx_mod_read_half(CAMERA+0x26);
        if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)<minimum+8)
            psx_mod_write_half(MMX4_PLAYER+10,(uint16_t)(minimum+8));
        psx_mod_write_half(CAMERA+10,(uint16_t)x);psx_mod_write_half(CAMERA+14,(uint16_t)y);return 0;
    }
    if(address==0x80027D40u || address==0x800281E8u) {
        unsigned layer=address==0x80027D40u?1:2;
        psx_mod_write_half(CAMERA+layer*0x54u+10,psx_mod_read_half(CAMERA+10)/2);
        psx_mod_write_half(CAMERA+layer*0x54u+14,psx_mod_read_half(CAMERA+14)/4);return 0;
    }
    if(address==0x8002B1E8u || address==0x8002B318u) {
        unsigned layer=psx_mod_read_byte(actor+0x14u);
        int x=(int16_t)psx_mod_read_half(actor+10)-(int16_t)psx_mod_read_half(CAMERA+layer*0x54u+10);
        int y=(int16_t)psx_mod_read_half(actor+14)-(int16_t)psx_mod_read_half(CAMERA+layer*0x54u+14);
        unsigned inside=(uint16_t)(x+cpu->gpr[5])<(uint16_t)(320+2*cpu->gpr[5]) &&
            (uint16_t)(y+cpu->gpr[6])<(uint16_t)(240+2*cpu->gpr[6]);
        if(address==0x8002B318u)psx_mod_write_byte(actor+3,(uint8_t)inside);
        return address==0x8002B318u?inside:!inside;
    }
    CHECK(0);return 0;
}
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t arg0,uint32_t arg1) {
    CPUState saved=*cpu;cpu->gpr[4]=arg0;cpu->gpr[5]=arg1;
    uint32_t result=invoke(cpu,address);*cpu=saved;return result;
}
void psx_dispatch_call(CPUState *cpu,uint32_t address,uint32_t caller) {
    (void)caller;CHECK(address==0x80028FECu);native_scan(cpu);
}
static void native_scan(CPUState *cpu) {
    CPUState saved=*cpu;
    for(unsigned index=0;index<8;++index) {
        uint32_t record=TABLE+index*8u;
        if(psx_mod_read_byte(record+3)==255)break;
        if(psx_mod_read_byte(record)&0x81u)continue;
        int x=(int16_t)psx_mod_read_half(record+4),y=(int16_t)psx_mod_read_half(record+6);
        if(x<=(int32_t)saved.gpr[4] || x>=(int32_t)saved.gpr[5] ||
           y<=(int32_t)saved.gpr[6] || y>=(int32_t)saved.gpr[7])continue;
        *cpu=saved;cpu->gpr[19]=record;cpu->gpr[18]=record+3u;
        cpu->gpr[31]=0x8002916Cu;cpu->gpr[4]=psx_mod_read_byte(record+3u);
        cpu->gpr[17]=invoke(cpu,0x800293E8u);
        if(cpu->gpr[17]) {
            spawned(cpu,0x80029284u);psx_mod_write_byte(record,psx_mod_read_byte(record)|1u);
        }
    }
    *cpu=saved;
}
static void actor_position(uint32_t actor,int x,int y) {
    psx_mod_write_half(actor+10,(uint16_t)x);psx_mod_write_half(actor+14,(uint16_t)y);
}
static void second_position(int x,int y) {
    second[10]=(uint8_t)x;second[11]=(uint8_t)(x>>8);
    second[14]=(uint8_t)y;second[15]=(uint8_t)(y>>8);
}
static void placement(unsigned index,int x,int y,unsigned category) {
    uint32_t record=TABLE+index*8u;
    psx_mod_write_half(record+4,(uint16_t)x);psx_mod_write_half(record+6,(uint16_t)y);
    psx_mod_write_byte(record+3,(uint8_t)category);
}
int main(void) {
    CPUState cpu={0};cpu.gpr[29]=0x801FF000u;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAYER,1);
    psx_mod_write_byte(MMX4_PLAYER+4,1);second[0]=second[4]=1;
    psx_mod_write_byte(CAMERA+0x44,1);actor_position(MMX4_PLAYER,100,140);
    second_position(1200,2000);
    CHECK(mmx4_coop_split_activate());
    uint8_t canonical[0xFC],view[0xFC];
    for(unsigned offset=0;offset<sizeof canonical;++offset)canonical[offset]=psx_mod_read_byte(CAMERA+offset);
    mmx4_coop_split_camera(&cpu,0);
    for(unsigned offset=0;offset<sizeof canonical;++offset)CHECK(canonical[offset]==psx_mod_read_byte(CAMERA+offset));
    CHECK(!projected && mmx4_coop_split_camera_copy(1,view));
    CHECK((view[10]|(unsigned)view[11]<<8)==1040 && (view[14]|(unsigned)view[15]<<8)==1872);
    CHECK((view[0x54+10]|(unsigned)view[0x54+11]<<8)==520);
    actor_position(MMX4_PLAYER,600,300);actor_position(CAMERA,440,172);
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && (view[10]|(unsigned)view[11]<<8)==1040);
    actor_position(MMX4_PLAYER,100,140);actor_position(CAMERA,0,0);
    mmx4_coop_split_camera(&cpu,0);
    cpu.gpr[4]=ACTOR;cpu.gpr[5]=cpu.gpr[6]=64;
    actor_position(ACTOR,1200,2000);CHECK(!invoke(&cpu,0x8002B1E8u));
    CHECK(invoke(&cpu,0x8002B318u) && psx_mod_read_byte(ACTOR+3));
    actor_position(ACTOR,600,1000);CHECK(invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,100,2000);CHECK(invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,1200,140);CHECK(invoke(&cpu,0x8002B1E8u));
    mmx4_coop_enter_second();actor_context=1;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    CHECK(actors[0].world_x==100 && actors[0].world_y==140);
    CHECK(actors[1].world_x==1200 && actors[1].world_y==2000);
    actor_position(ACTOR,1200,2000);CHECK(!invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,100,140);CHECK(!invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,100,2000);CHECK(invoke(&cpu,0x8002B1E8u));
    actor_context=0;mmx4_coop_leave_second();
    actor_position(ACTOR,1200,2000);local_scope=1;
    CHECK(invoke(&cpu,0x8002B1E8u));local_scope=0;
    script_owner=0;CHECK(invoke(&cpu,0x8002B1E8u));
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[10]==0 && view[11]==0);script_owner=-1;
    psx_mod_write_word(0x800F43C8u,TABLE);
    placement(0,100,140,0);placement(1,1200,2000,0);
    placement(2,600,1000,0);placement(3,100,2000,0);placement(4,1300,2100,3);
    psx_mod_write_byte(TABLE+5u*8u+3u,255);
    capacity=1;mmx4_coop_split_reset();mmx4_coop_split_camera(&cpu,0);
    invoke(&cpu,0x80028E24u);
    CHECK(allocations==1 && (psx_mod_read_byte(TABLE)&1u) && !psx_mod_read_byte(TABLE+8));
    capacity=16;mmx4_coop_split_camera(&cpu,0);invoke(&cpu,0x80028E24u);
    CHECK(allocations==3 && (psx_mod_read_byte(TABLE+8)&1u) && (psx_mod_read_byte(TABLE+32)&1u));
    CHECK(!psx_mod_read_byte(TABLE+16) && !psx_mod_read_byte(TABLE+24));
    psx_mod_write_byte(TABLE,0);invoke(&cpu,0x80028E24u);CHECK(allocations==3);
    uint32_t digest=mmx4_coop_split_digest(2166136261u);
    actor_position(CAMERA,3000,3000);second_position(3200,3200);
    mmx4_coop_split_camera(&cpu,0);invoke(&cpu,0x80028E24u);
    CHECK(mmx4_coop_split_digest(2166136261u)!=digest);
    actor_position(CAMERA,0,0);second_position(1200,2000);
    mmx4_coop_split_camera(&cpu,0);invoke(&cpu,0x80028E24u);CHECK(allocations==4);
    second_position(-100,140);mmx4_coop_split_camera(&cpu,0);
    CHECK(second[10]==8 && second[11]==0 && psx_mod_read_half(MMX4_PLAYER+10)==100);
    CHECK(scans>0);
    /* Native room following eases +1E toward the authored target +26 and
     * 27BE4 clamps a player to that lower X limit. P1 entering a different
     * camera region must not import its target into stationary P2's view. */
    mmx4_coop_split_reset();actor_position(MMX4_PLAYER,100,140);
    actor_position(CAMERA,0,0);second_position(100,140);
    psx_mod_write_half(CAMERA+0x26,0);mmx4_coop_split_camera(&cpu,0);
    psx_mod_write_half(CAMERA+0x26,512);mmx4_coop_split_camera(&cpu,0);
    CHECK(second[10]==100 && second[11]==0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x26]==0 && view[0x27]==0);
    psx_mod_write_byte(MMX4_PLAYER+4,3);mmx4_coop_split_camera_prepare(1);
    CHECK(psx_mod_read_half(CAMERA+0x26)==0 && second[10]==100 && second[11]==0);
    psx_mod_write_byte(MMX4_PLAYER+4,1);psx_mod_write_half(CAMERA+0x26,512);
    script_owner=0;mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x26]==0 && view[0x27]==2);
    script_owner=-1;split=0;CHECK(mmx4_coop_split_activate());
    CHECK(!mmx4_coop_split_camera_copy(1,view));
    puts("mmx4_coop_split_test: PASS");return 0;
}
