#include "mmx4_coop_texture_cache.h"
#include <limits.h>
#include <string.h>

static void retain_packets(uint8_t used[512],uint16_t first,uint32_t begin,
    unsigned bytes,uint32_t (*read_word)(uint32_t)) {
    for(unsigned offset=0;offset+40<=bytes;offset+=40) {
        uint32_t at=begin+offset;
        if((read_word(at)>>24)!=9 || (read_word(at+4)&0xFC000000u)!=0x34000000u)continue;
        unsigned id=(read_word(at+16)>>24)|((read_word(at+28)>>24)<<8);
        if(id>=first && id-first<4096)used[(id-first)/8]|=(uint8_t)(1u<<((id-first)%8));
    }
}
unsigned mmx4_coop_texture_cache_slot(unsigned count,uint16_t first,
    unsigned start,uint32_t world,uint32_t hud,uint32_t (*read_word)(uint32_t)) {
    return mmx4_coop_texture_cache_slot_buffers(count,first,start,world,hud,2,read_word);
}
unsigned mmx4_coop_texture_cache_slot_buffers(unsigned count,uint16_t first,
    unsigned start,uint32_t world,uint32_t hud,unsigned world_buffers,uint32_t (*read_word)(uint32_t)) {
    if(!world_buffers || world_buffers>8)return UINT_MAX;
    if(count<4096)return count;
    if(count!=4096 || !world || !hud || !read_word)return UINT_MAX;
    uint8_t used[512];memset(used,0,sizeof used);
    for(unsigned buffer=0;buffer<world_buffers;++buffer) {
        /* render_actor emits at most 1,000 pairs of 40-byte triangles after
         * its 40,000-byte native packet scratch region. HUD pairs occupy
         * [3000,8000). Pin even unused old tails: conservative retention is
         * safe. Refuse reuse when the complete live set pins every ID. */
        retain_packets(used,first,world+buffer*MMX4_COOP_FRAME_ARENA_BYTES+40000u,80000u,read_word);
    }
    for(unsigned buffer=0;buffer<2;++buffer) {
        retain_packets(used,first,hud+buffer*0x8000u+0x3000u,0x5000u,read_word);
    }
    for(unsigned i=0;i<4096;++i) {
        unsigned slot=(start+i)&4095u;
        if(!(used[slot/8]&(1u<<(slot%8))))return slot;
    }
    return UINT_MAX;
}
