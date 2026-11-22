#include "mod_plugins.h"
#include "mod_netplay.h"
#include "cpu_state.h"
#include <string.h>

#define PLUGIN "mmx4.boss-rush-skip"
#define COOP_ID "mmx4.coop"
#define PLAY 0x801721C0u

static void allow_native_sigma_portal(CPUState *cpu, uint32_t address) {
    (void)address;
    uint32_t actor = cpu->gpr[16];
    if (psx_mod_local_view_scope() || psx_mod_read_byte(PLAY) != 6 ||
        psx_mod_read_byte(PLAY + 1) || psx_mod_read_byte(PLAY + 12) != 12 ||
        psx_mod_read_byte(PLAY + 13) || psx_mod_read_byte(PLAY + 0x59) != 255 ||
        psx_mod_read_byte(PLAY + 0x5F) < 10 ||
        actor < 0x80165A30u || actor >= 0x80166BB0u ||
        (actor - 0x80165A30u) % 0x8Cu || !psx_mod_read_byte(actor) ||
        psx_mod_read_byte(actor + 1) != 27 || psx_mod_read_byte(actor + 2) ||
        psx_mod_read_byte(actor + 4) != 1 || psx_mod_read_byte(actor + 5) != 4 ||
        psx_mod_read_byte(actor + 6) || cpu->gpr[3] < 8 || cpu->gpr[3] > 16)
        return;
    /* Retail's board admits stage12 only at story10 after the prior fortress
     * stages. At C78BC the final portal waits for eight refight flags. Change
     * only this completed eligibility count: native allocation, collision,
     * hero warp, Sigma introduction and rewards retain their original owners.
     * Never write those refight flags or mark the final stage completed. */
    cpu->gpr[3] = 16;
}

static void allow_online_sigma_portal(CPUState *cpu, uint32_t address) {
    char enabled[8];
    if (psx_mod_netplay_is_active() &&
        psx_mod_current_option_value("boss_rush_skip", enabled, sizeof enabled) &&
        strcmp(enabled, "true") == 0)
        allow_native_sigma_portal(cpu, address);
}

PSX_MOD_CONSTRUCTOR(mmx4_register_boss_rush_skip_plugin) {
    (void)psx_mod_register_instruction_plugin(
        PLUGIN, 0x800C7900u, 0x306300FFu, allow_native_sigma_portal);
    (void)psx_mod_register_instruction_plugin(
        COOP_ID, 0x800C7900u, 0x306300FFu, allow_online_sigma_portal);
}
