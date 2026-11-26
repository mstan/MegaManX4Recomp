/* Nightmare Zero palette for the original USA resource layout. The native
 * palette copier still owns selection, animation, lighting and uploads.
 * See the package's CREDITS.md for the public color-code reference. */
#include "cpu_state.h"
#include "mod_plugins.h"
#include "mod_runtime.h"
#include "mod_snapshot.h"
#include <stdlib.h>

#define PACKAGE "mmx4.palette.nightmare-zero"
#define PLUGIN "mmx4.nightmare-zero"
#define PLAY 0x801721C0u
#define PLAYER 0x801418C8u

typedef struct { unsigned offset; uint16_t color; } Color;
static const Color colors[] = {
    {0x004,0xBB66},{0x006,0xBB66},{0x008,0xE657},{0x00A,0xDDB3},
    {0x00C,0xD90F},{0x00E,0xC88B},{0x010,0xEF1C},{0x012,0xF30C},
    {0x014,0xDDCB},{0x016,0xC88B},{0x018,0xE69A},{0x01A,0xE1D8},
    {0x01C,0xD575},{0x01E,0xE238},
    {0x062,0xFBDE},{0x064,0xFBDE},{0x066,0xE2D4},{0x068,0xC18C},
    {0x06A,0xB108},{0x06C,0xA084},{0x06E,0xFBDE},{0x070,0xE2D4},
    {0x072,0xC18C},{0x074,0xB108},{0x076,0xA084},{0x078,0xFBDE},
    {0x07A,0xE2D4},{0x07C,0xC18C},{0x07E,0xA084},
    {0x082,0xFBDE},{0x084,0xFBDE},{0x086,0xE2D4},{0x088,0xC18C},
    {0x08A,0xB108},{0x08C,0xA084},{0x08E,0xFBDE},{0x090,0xE2D4},
    {0x092,0xC18C},{0x094,0xB108},{0x096,0xA084},{0x098,0xFBDE},
    {0x09A,0xE2D4},{0x09C,0xC18C},{0x09E,0xA084},
    {0x12E,0xD673},{0x130,0xC1CE},{0x132,0xB96B},{0x134,0xB129},{0x136,0xA084},
    {0x462,0xFF71},{0x464,0xFF72},{0x466,0xFF73},{0x468,0xFF74},
    {0x46A,0xFF95},{0x46C,0xFF96},{0x46E,0xFF97},{0x470,0xFF98},
    {0x472,0xFFB9},{0x474,0xFFBA},{0x476,0xFFBB},{0x478,0xFFDC},
    {0x47A,0xFFDD},{0x47C,0xFFDE},{0x47E,0xFFFF}
};
/* D0 compares the full stage/section halfword, not just the stage byte.
 * 50000F02 fills colors1..15 of each of the three native trail CLUTs.
 * The original addresses also guard the unrelocated COL resource. */
typedef struct { uint16_t stage; uint32_t first; } Trails;
static const Trails trails[] = {
    {0x0000,0x801D1D92u},{0x0100,0x801D0D96u},
    {0x0001,0x801D129Au},{0x0101,0x801D24E6u},
    {0x0002,0x801D68F6u},{0x0102,0x801D9D9Au},
    {0x0003,0x801CE3AAu},{0x0103,0x801CE8DAu},
    {0x0004,0x801CB11Au},{0x0104,0x801D3A22u},
    {0x0105,0x801D0C86u},{0x0006,0x801CF966u},
    {0x0106,0x801D943Eu},{0x0007,0x801CF8D6u},
    {0x0107,0x801CAB8Eu},{0x0008,0x801C73DEu},
    {0x0108,0x801D1B3Eu},{0x000A,0x801CB2E2u},
    {0x000B,0x801C7DF2u},{0x010B,0x801D1C1Au},
    {0x000C,0x801CBDD2u},{0x010C,0x801C7E4Eu}
};

static int range(uint32_t p,unsigned bytes) {
    return !(p&1u) && ((p>=0x80010000u && p<=0x80200000u-bytes) ||
                      (p>=0x9F000000u && p<=0x9F100000u-bytes));
}
static void apply(uint32_t actor) {
    /* The original player initializer copies this COL in mode5, before it
     * publishes gameplay mode6. The entry still owns an identified Zero
     * actor and all original resource/code guards below. */
    unsigned mode=psx_mod_read_byte(PLAY);
    if(!mod_runtime_snapshot_package_active(PACKAGE) || (mode!=5 && mode!=6) ||
       !range(actor,0xE4) || psx_mod_read_byte(actor+2)!=1 ||
       psx_mod_read_word(0x800361F8u)!=0x27BDFFE8u ||
       psx_mod_read_word(0x80036200u)!=0x80820002u ||
       psx_mod_read_word(0x800361B8u)!=0x8C420024u)return;
    uint32_t source=psx_mod_read_word(0x1F800024u);
    uint32_t palette=psx_mod_read_word(0x1F800028u);
    if(!range(source,0x480) || !range(palette,0x200) ||
       (source<palette+0x200u && palette<source+0x480u))return;
    unsigned stage=psx_mod_read_half(PLAY+0xC),match=0;
    for(unsigned i=0;i<sizeof trails/sizeof trails[0];++i) {
        if(stage!=trails[i].stage)continue;
        /* Co-op owns an equivalent private COL buffer at the same native
         * scratch pointer. Never write another stage's literal RAM range. */
        if(palette!=trails[i].first-0x1A2u && palette<0x9F000000u)return;
        match=1;break;
    }
    for(unsigned i=0;i<sizeof colors/sizeof colors[0];++i) {
        psx_mod_write_half(source+colors[i].offset,colors[i].color);
        /* Native Black Zero remains an independent code choice. Nightmare
         * has explicit color priority for both native body source rows. */
        if(colors[i].offset<0x20)
            psx_mod_write_half(source+13*32+colors[i].offset,colors[i].color);
    }
    if(match)for(unsigned row=13;row<16;++row)
        for(unsigned color=1;color<16;++color)
            psx_mod_write_half(palette+row*32+color*2,0xC412);
    psx_mod_write_byte(0x80166BB0u,psx_mod_read_byte(0x80166BB0u)|1u);
}
static void palette_entry(CPUState *cpu,uint32_t address) {
    (void)address;apply(cpu->gpr[4]);
}
static void restored(void) {
    /* Restore only a live gameplay palette, never an unfinished initializer. */
    if(psx_mod_read_byte(PLAY)==6)apply(PLAYER);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_nightmare_zero) {
    if(!psx_mod_register_function_entry_plugin(PLUGIN,0x800361F8u,palette_entry) ||
       !psx_mod_register_savestate_plugin(PLUGIN,restored))abort();
}
