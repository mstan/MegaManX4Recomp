#include "mmx4_coop_roster.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
int main(void) {
    Mmx4CoopRoster roster,previous;
    mmx4_roster_default(&roster);assert(roster.count==4 && roster.hero[2]==MMX4_HERO_ULTIMATE_X);
    const char *choices[]={"x","force_x","ultimate_x","zero","black_zero","none"};
    unsigned valid=0;
    for(unsigned a=0;a<6;++a)for(unsigned b=0;b<6;++b)for(unsigned c=0;c<6;++c)for(unsigned d=0;d<6;++d) {
        const char *values[]={choices[a],choices[b],choices[c],choices[d]};
        previous=roster;
        int expected=a<5 && b<5 && c<5 && a!=b && a!=c && a!=d && b!=c && b!=d && c!=d;
        assert(mmx4_roster_parse(&roster,values)==expected);
        if(expected) {++valid;assert(roster.count==(d==5?3:4));}
        else assert(!memcmp(&roster,&previous,sizeof roster));
    }
    assert(valid==180); /* 5P4 complete rosters + 5P3 three-player rosters. */
    const char *invalid[]={"x","zero","unknown","none"};
    assert(!mmx4_roster_parse(&roster,invalid));
    assert(mmx4_roster_character(MMX4_HERO_BLACK_ZERO)==1);
    assert(mmx4_roster_armor(MMX4_HERO_FORCE_X)==15);
    return 0;
}
