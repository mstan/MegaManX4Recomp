#include "mmx4_coop_debug_protocol.h"

int mmx4_debug_is_packet(uint16_t buttons) {return (buttons&6u)==0;}
uint16_t mmx4_debug_checksum(const uint16_t words[6]) {
    uint32_t hash=2166136261u;
    for(unsigned i=0;i<6;++i) {
        hash=(hash^(words[i]&255u))*16777619u;
        hash=(hash^(words[i]>>8))*16777619u;
    }
    return (uint16_t)((hash^(hash>>11)^(hash>>22))&2047u);
}
int mmx4_debug_decode(Mmx4DebugDecoder *state,uint16_t buttons,Mmx4DebugRequest *out) {
    if(!mmx4_debug_is_packet(buttons)) {
        state->next=state->have_previous=0;return 0;
    }
    uint16_t held=(uint16_t)~buttons;
    unsigned word=(held&1u)|((held>>2)&0x3FFEu);
    if(state->have_previous && state->previous==word)return 0;
    state->previous=(uint16_t)word;state->have_previous=1;
    unsigned index=word>>11,payload=word&2047u;
    if(index==0) {
        state->next=0;
        if((payload&0x7E0u)!=0x5A0u || !(payload&31u))return 0;
        state->words[0]=(uint16_t)payload;state->next=1;return 0;
    }
    if(index!=state->next || index>6) {state->next=0;return 0;}
    state->words[index]=(uint16_t)payload;++state->next;
    if(index!=6)return 0;
    state->next=0;
    unsigned sequence=state->words[0]&31u;
    if(payload!=mmx4_debug_checksum(state->words) || sequence==state->last_sequence)return 0;
    state->last_sequence=sequence;
    out->sequence=sequence;out->command=state->words[1]&7u;
    out->first=state->words[1]>>3;out->second=state->words[2]&255u;
    out->third=state->words[2]>>8;
    out->x=(int16_t)(state->words[3]|((state->words[4]&31u)<<11));
    out->y=(int16_t)((state->words[4]>>5)|(state->words[5]<<6));
    return 1;
}
