#include "mmx4_coop_hud.h"

void mmx4_coop_hud_tick(uint8_t body[0xE4],uint32_t native_frame) {
    unsigned health=body[0x5C]&127u;
    int trail=(int8_t)body[0x5D];
    if(trail==(int)health)return;
    if(trail<(int)health) {
        body[0x5D]=(uint8_t)health;
        return;
    }
    if(body[0x5C]&128u)body[0x5F]=16;
    if(body[0x5F])--body[0x5F];
    else if(!(native_frame&3u))--body[0x5D];
}
