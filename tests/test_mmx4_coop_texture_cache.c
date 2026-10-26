#include "mmx4_coop_texture_cache.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(v) do { if(!(v)) { fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1); } } while(0)
#define WORLD 0x1000u
#define HUD (WORLD+8*MMX4_COOP_FRAME_ARENA_BYTES)
static uint32_t ram[(HUD+2*0x8000u)/4];
static uint32_t saved[sizeof ram/sizeof ram[0]];
static unsigned reads;
static uint32_t read_word(uint32_t at) {
    CHECK(!(at&3) && at/4<sizeof ram/sizeof ram[0]);++reads;return ram[at/4];
}
static void triangle(uint32_t at,unsigned id) {
    ram[at/4]=0x09FFFFFFu;ram[at/4+1]=0x34808080u;
    ram[at/4+4]=(id&255u)<<24;ram[at/4+7]=(id>>8)<<24;
}
static void pair(uint32_t at,unsigned id) {triangle(at,id);triangle(at+40,id);}
static unsigned slot(unsigned count,unsigned first,unsigned start) {
    return mmx4_coop_texture_cache_slot(count,(uint16_t)first,start,WORLD,HUD,read_word);
}
int main(void) {
    CHECK(slot(4095,0x6000,0)==4095 && !reads);
    /* Both world buffers, both HUD buffers, and old inactive tails remain
     * valid even while the other buffer is being rebuilt. */
    pair(WORLD+40000,0x6000);pair(WORLD+MMX4_COOP_FRAME_ARENA_BYTES+40000,0x6FFF);
    pair(HUD+0x3000,0x6001);pair(HUD+0x8000+0x3000,0x6002);
    pair(WORLD+40000+79920,0x6004);
    CHECK(slot(4096,0x6000,4095)==3);
    CHECK(slot(4096,0x6000,4)==5);
    CHECK(slot(4096,0xA000,0)==0); /* Own-view IDs are a separate namespace. */
    memcpy(saved,ram,sizeof ram);
    unsigned restored=slot(4096,0x6000,4095);
    memset(ram,0,sizeof ram);CHECK(slot(4096,0x6000,4095)==4095);
    memcpy(ram,saved,sizeof ram);CHECK(slot(4096,0x6000,4095)==restored);
    memset(ram,0,sizeof ram);
    for(unsigned buffer=0;buffer<8;++buffer)pair(WORLD+buffer*MMX4_COOP_FRAME_ARENA_BYTES+40000u,0x6000u+buffer);
    CHECK(mmx4_coop_texture_cache_slot_buffers(4096,0x6000,0,WORLD,HUD,8,read_word)==8);
    CHECK(mmx4_coop_texture_cache_slot_buffers(4096,0x6000,0,WORLD,HUD,2,read_word)==2);
    CHECK(mmx4_coop_texture_cache_slot_buffers(4096,0x6000,0,WORLD,HUD,9,read_word)==UINT_MAX);

    /* Exercise sustained pressure beyond the old lifetime capacity in every
     * pool. A replacement must never change a current/previous-frame packet. */
    const unsigned bases[]={0x6000,0x8000,0xA000,0xB000};
    for(unsigned pool=0;pool<4;++pool) {
        memset(ram,0,sizeof ram);
        unsigned live[2][64];memset(live,0xFF,sizeof live);
        for(unsigned i=0;i<5000;++i) {
            unsigned buffer=(i/64)&1u,index=i%64;
            unsigned chosen=slot(4096,bases[pool],i);
            CHECK(chosen<4096);
            for(unsigned b=0;b<2;++b)for(unsigned j=0;j<64;++j)CHECK(chosen!=live[b][j]);
            uint32_t at=pool&1?HUD+buffer*0x8000u+0x3000u+index*80u:
                WORLD+buffer*MMX4_COOP_FRAME_ARENA_BYTES+40000u+index*80u;
            pair(at,bases[pool]+chosen);live[buffer][index]=chosen;
        }
    }
    /* Refuse an overfull/corrupt packet set instead of evicting a live ID. */
    memset(ram,0,sizeof ram);
    for(unsigned i=0;i<4000;++i)triangle(WORLD+(i/2000)*MMX4_COOP_FRAME_ARENA_BYTES+40000+(i%2000)*40,0x6000+i);
    for(unsigned i=0;i<96;++i)triangle(HUD+0x3000+i*40,0x6000+4000+i);
    CHECK(slot(4096,0x6000,0)==UINT_MAX);
    CHECK(slot(4097,0x6000,0)==UINT_MAX);
    puts("texture cache: sustained reuse preserves both packet buffers and restored IDs");
    return 0;
}
