#ifndef MMX4_STORY_DAMAGE_H
#define MMX4_STORY_DAMAGE_H
#include "cpu_state.h"
#include "mod_plugins.h"

/* The native admission LBU has already selected its reaction and rejected an
 * unavailable cross-character entry. Reuse authored reactions/damage units;
 * preserve actual attack identity, duplicate-hit tokens and cooldown tables. */
static inline void mmx4_story_counterpart_hit(CPUState *cpu) {
    if(psx_mod_local_view_scope() || psx_mod_read_byte(0x801721C0u)!=6 ||
       psx_mod_read_byte(0x801721CCu)!=11 || psx_mod_read_byte(0x801721CDu) ||
       cpu->gpr[2]!=0xFFu)return;
    uint32_t actor=cpu->gpr[4],type=cpu->gpr[6];
    if(actor<0x8013BED0u || actor>=0x8013BED0u+48u*0x9Cu ||
       (actor-0x8013BED0u)%0x9Cu || !psx_mod_read_byte(actor))return;
    if(!psx_mod_read_byte(0x801418C8u+2) && psx_mod_read_byte(actor+1)==66 &&
       psx_mod_read_byte(actor+2)<=1 && type<=23 &&
       (cpu->gpr[7]==0x80107DFCu || cpu->gpr[7]==0x80107E84u)) {
        /* Retail Iris has no X entries. Reuse her own Zero reactions and
         * damage units: armor5/0 retains its eight-hit exposure sequence;
         * the core takes buster1/partial2/charged3, neutral weapon2,
         * fire/Nova4. Native800F25D8 identifies buster0/9/18..21/23,
         * normal weapons1..8, charged weapons10..17 and Nova22.
         * Keep X's actual attack identity for native consumption/tokens.
         * In particular the all-FF cooldown table is never admitted. */
        unsigned entry=(type==0 || type==23)?32u:
            (type>=19 && type<=21)?26u:
            (type==5 || type==14 || type==22)?35u:24u;
        cpu->gpr[2]=psx_mod_read_byte(cpu->gpr[7]+entry*2u);
        cpu->gpr[7]+=(entry-type)*2u;
        psx_mod_counter_add("mmx4.coop.iris-x-hits",1);
        return;
    }
    if(psx_mod_read_byte(0x801418C8u+2)!=1 ||
       psx_mod_read_byte(actor+1)!=73 || cpu->gpr[7]!=0x80108204u ||
       !((type>=24 && type<=32) || type==34))return;
    /* The first standing slash (24) and Kuuenzan (34) have no retail Double
     * damage entry: he is X's campaign boss. Reuse his authored two-point
     * saber damage for these collisions only. Keep S0/A2 and the attack
     * intact so consumption, hit tokens and the native result retain the
     * actual attack identity. A3 is only the subsequent damage lookup base.
     * This also precedes the damage multiplier and co-op health scaling. */
    uint32_t entry=(type==24 || type==34)?26u:type;
    if(!psx_mod_read_byte(cpu->gpr[7]+entry*2u+1u))return;
    cpu->gpr[7]-=(type-entry)*2u;
    cpu->gpr[2]=0;
}

#endif
