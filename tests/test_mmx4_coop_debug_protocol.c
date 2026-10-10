#include "mmx4_coop_debug_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint16_t encode(unsigned index,unsigned payload) {
    unsigned word=(index<<11)|payload;
    return (uint16_t)~((word&1u)|((word&0x3FFEu)<<2)|6u);
}
int main(void) {
    Mmx4DebugDecoder a={0},b={0};Mmx4DebugRequest ra={0},rb={0};
    uint16_t words[7]={0x5A1,(1u<<3)|4u,0,(-1234)&2047u,
        (((uint16_t)-1234)>>11)|((32760u&63u)<<5),32760u>>6,0};
    words[6]=mmx4_debug_checksum(words);
    for(unsigned i=0;i<7;++i) {
        /* Different host polling frequencies consume the same admitted pad. */
        CHECK(mmx4_debug_decode(&a,encode(i,words[i]),&ra)==(i==6));
        CHECK(!mmx4_debug_decode(&a,encode(i,words[i]),&ra));
        CHECK(mmx4_debug_decode(&b,encode(i,words[i]),&rb)==(i==6));
    }
    CHECK(ra.command==4 && ra.first==1 && ra.x==-1234 && ra.y==32760);
    CHECK(rb.sequence==ra.sequence && rb.x==ra.x && rb.y==ra.y);
    for(unsigned i=0;i<7;++i)CHECK(!mmx4_debug_decode(&a,encode(i,words[i]),&ra));
    words[0]=0x5A2;words[6]=mmx4_debug_checksum(words)^1;
    for(unsigned i=0;i<7;++i)CHECK(!mmx4_debug_decode(&a,encode(i,words[i]),&ra));
    CHECK(!mmx4_debug_decode(&a,encode(0,words[0]),&ra));
    CHECK(!mmx4_debug_decode(&a,0xFFFF,&ra));
    for(unsigned i=1;i<7;++i)CHECK(!mmx4_debug_decode(&a,encode(i,words[i]),&ra));
    words[6]=mmx4_debug_checksum(words);
    for(unsigned i=0;i<7;++i)CHECK(mmx4_debug_decode(&a,encode(i,words[i]),&ra)==(i==6));
    CHECK(ra.sequence==2);
    CHECK(!mmx4_debug_is_packet(0xFFF7));
    puts("synchronized debug protocol: passed");return 0;
}
