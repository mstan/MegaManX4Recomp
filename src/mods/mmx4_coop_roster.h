#ifndef MMX4_COOP_ROSTER_H
#define MMX4_COOP_ROSTER_H
#include <stdint.h>
enum Mmx4HeroVariant {
    MMX4_HERO_X, MMX4_HERO_FORCE_X, MMX4_HERO_ULTIMATE_X,
    MMX4_HERO_ZERO, MMX4_HERO_BLACK_ZERO, MMX4_HERO_NONE
};
typedef struct { uint8_t hero[4], count; } Mmx4CoopRoster;
/* Exact variant uniqueness; X and Ultimate X are distinct legal selections. */
int mmx4_roster_parse(Mmx4CoopRoster *out,const char *const values[4]);
unsigned mmx4_roster_character(unsigned variant);
unsigned mmx4_roster_armor(unsigned variant);
const char *mmx4_roster_value(unsigned variant);
void mmx4_roster_default(Mmx4CoopRoster *out);
#endif
