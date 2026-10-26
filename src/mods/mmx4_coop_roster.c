#include "mmx4_coop_roster.h"
#include <string.h>
static const char *const names[]={"x","force_x","ultimate_x","zero","black_zero","none"};
const char *mmx4_roster_value(unsigned variant) {return variant<=MMX4_HERO_NONE?names[variant]:"";}
unsigned mmx4_roster_character(unsigned variant) {return variant==MMX4_HERO_ZERO || variant==MMX4_HERO_BLACK_ZERO;}
unsigned mmx4_roster_armor(unsigned variant) {return variant==MMX4_HERO_FORCE_X || variant==MMX4_HERO_ULTIMATE_X?15u:0u;}
void mmx4_roster_default(Mmx4CoopRoster *out) {
    *out=(Mmx4CoopRoster){{MMX4_HERO_X,MMX4_HERO_ZERO,MMX4_HERO_ULTIMATE_X,MMX4_HERO_BLACK_ZERO},4};
}
int mmx4_roster_parse(Mmx4CoopRoster *out,const char *const values[4]) {
    Mmx4CoopRoster candidate={{0},4};unsigned seen=0;
    if(!out || !values)return 0;
    for(unsigned seat=0;seat<4;++seat) {
        unsigned variant=0;
        if(!values[seat])return 0;
        while(variant<=MMX4_HERO_NONE && strcmp(values[seat],names[variant]))++variant;
        if(variant>MMX4_HERO_NONE || (variant==MMX4_HERO_NONE && seat!=3) || (seen&(1u<<variant)))return 0;
        seen|=1u<<variant;candidate.hero[seat]=(uint8_t)variant;
    }
    if(candidate.hero[3]==MMX4_HERO_NONE)candidate.count=3;
    *out=candidate;return 1;
}
