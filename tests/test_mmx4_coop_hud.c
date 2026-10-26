#include "mmx4_coop_hud.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(v) do {if(!(v)){fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(1);}}while(0)

int main(void) {
    uint8_t bodies[4][0xE4],before[0xE4];
    memset(bodies,0xA5,sizeof bodies);
    for(unsigned seat=0;seat<4;++seat) {
        bodies[seat][0x5C]=32;bodies[seat][0x5D]=32;bodies[seat][0x5F]=0;
    }
    /* Reproduce the owner's independent health values. Missing canonical
     * updates leave all three extra trails stuck at 32 indefinitely. */
    bodies[0][0x5C]=29;bodies[1][0x5C]=14;
    bodies[2][0x5C]=24;bodies[3][0x5C]=15;
    for(unsigned frame=1;frame<=72;++frame)
        for(unsigned seat=0;seat<4;++seat) {
            memcpy(before,bodies[seat],sizeof before);
            mmx4_coop_hud_tick(bodies[seat],frame);
            for(unsigned off=0;off<sizeof before;++off)
                if(off!=0x5D && off!=0x5F)CHECK(before[off]==bodies[seat][off]);
            CHECK(bodies[seat][0x5D]>=bodies[seat][0x5C]);
            if(frame<4)CHECK(bodies[seat][0x5D]==32);
        }
    for(unsigned seat=0;seat<4;++seat)CHECK(bodies[seat][0x5D]==bodies[seat][0x5C]);
    /* Native marked damage starts a 16-tick delay, then loses one trail
     * unit every fourth native frame. Other seats remain independent. */
    bodies[1][0x5C]=128|10;
    mmx4_coop_hud_tick(bodies[1],73);
    CHECK(bodies[1][0x5D]==14 && bodies[1][0x5F]==15);
    bodies[1][0x5C]=10;
    for(unsigned frame=74;frame<=88;++frame)mmx4_coop_hud_tick(bodies[1],frame);
    CHECK(bodies[1][0x5D]==14 && !bodies[1][0x5F]);
    for(unsigned frame=89;frame<=104;++frame)mmx4_coop_hud_tick(bodies[1],frame);
    CHECK(bodies[1][0x5D]==10);
    CHECK(bodies[0][0x5D]==29 && bodies[2][0x5D]==24 && bodies[3][0x5D]==15);
    bodies[1][0x5C]=32;mmx4_coop_hud_tick(bodies[1],105);
    CHECK(bodies[1][0x5D]==32); /* Healing has no red deficit. */
    puts("mmx4_coop_hud_test: PASS");return 0;
}
