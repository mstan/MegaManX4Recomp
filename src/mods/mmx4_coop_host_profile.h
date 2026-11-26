#ifndef MMX4_COOP_HOST_PROFILE_H
#define MMX4_COOP_HOST_PROFILE_H
#include "mod_netplay.h"

#if defined(__linux__)
#define MMX4_HOST_PROFILE "mmx4.linux64.enhanced.release"
#define MMX4_HOST_ABI "linux-gnu-gcc13.3.0"
#else
#define MMX4_HOST_PROFILE "mmx4.win64.enhanced.release"
#define MMX4_HOST_ABI "win64-mingw-gcc15.2.0"
#endif
#define MMX4_HOST_EXECUTION "bc888005a3b2cce87ab94f7dc2983226ba806580953c934ec34636ee451c74e6"
#define MMX4_HOST_POLICY MMX4_HOST_PROFILE "/v1;kernel=host-native-v1;abi=" MMX4_HOST_ABI ";execution=" MMX4_HOST_EXECUTION ";cpu=x64-portable-known;fp=2;bios=OPENBIOS:c4daeb88;overlays=interpreted-sealed;manifest=sha256-v1;artifact=exact;seats=2;rollback=off;renderer=retained-opengl;aspect=4:3;debug=off;route=normal"

int mmx4_coop_host_profiles(const PSXModNetplayHostStartup *, PSXHostProfileCatalog *);
#endif
