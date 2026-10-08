#include "mmx4_coop_assets.h"
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0]|p[1]<<8); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16; }
int mmx4_coop_arc_asset(const uint8_t *data,size_t size,unsigned member,Mmx4Asset *out) {
    if (!data || !out || size<2048 || u32(data+4)!=size) return 0;
    unsigned count=u32(data); size_t cursor=2048;
    if (count>255 || member>=count) return 0;
    Mmx4Asset found={0};
    for(unsigned i=0;i<count;++i) {
        size_t bytes=u32(data+12+i*8);
        if (cursor>size || bytes>size-cursor) return 0;
        if(i==member) found=(Mmx4Asset){data+cursor,bytes,u32(data+8+i*8)};
        cursor+=(bytes+2047)&~(size_t)2047;
    }
    if(cursor!=size) return 0;
    *out=found; return 1;
}
int mmx4_coop_decode_sprite(const uint8_t *src,size_t size,uint8_t *dst,
                            size_t capacity,size_t *written) {
    size_t in=0,out=0;
    if(written)*written=0;
    if(!src || !dst || !written)return 0;
    while(size-in>=2) {
        unsigned flags=u16(src+in); in+=2;
        for(unsigned bit=0x8000;bit;bit>>=1) {
            if(size-in<2)return 0;
            unsigned code=u16(src+in);in+=2;
            if(!(flags&bit)) {
                if(capacity-out<2)return 0;
                dst[out++]=(uint8_t)code;dst[out++]=(uint8_t)(code>>8);continue;
            }
            unsigned count=code>>11,distance=code&0x7ff;
            if(!count) { if(size-in<2)return 0;count=u16(src+in);in+=2; }
            if(!count&&!distance) { *written=out;return 1; }
            if((size_t)count*2>capacity-out || (size_t)distance*2>out)return 0;
            while(count--) {
                uint8_t lo=distance?dst[out-distance*2]:0;
                uint8_t hi=distance?dst[out-distance*2+1]:0;
                dst[out++]=lo;dst[out++]=hi;
            }
        }
    }
    return 0;
}
