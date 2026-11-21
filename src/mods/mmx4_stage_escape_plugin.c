#include "mod_plugins.h"
#include "cpu_state.h"

#define PLUGIN "mmx4.stage-escape"
#define PLAY 0x801721C0u
#define MENU 0x801754A0u

static void allow_native_escape(CPUState *cpu, uint32_t address) {
    (void)address;
    if (cpu->gpr[16] != MENU || psx_mod_read_byte(PLAY) != 6 ||
        psx_mod_read_byte(PLAY + 1) != 2) return;
    /* Retail pause initialization has finished deriving this menu-only bit
     * from the stage-clear flags. Keep those campaign flags untouched. The
     * original Escape action owns the shared fade, save and board transition;
     * co-op already routes either player's Escape through that same owner. */
    psx_mod_write_byte(MENU + 0x23, 1);
}

PSX_MOD_CONSTRUCTOR(mmx4_register_stage_escape_plugin) {
    (void)psx_mod_register_instruction_plugin(
        PLUGIN, 0x8002FF7Cu, 0xA2020024u, allow_native_escape);
}
