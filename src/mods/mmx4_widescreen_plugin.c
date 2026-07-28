#include "mod_plugins.h"

/*
 * X4's game-specific widescreen hooks remain part of generated/runtime code,
 * but their player-facing activation belongs to the Mods catalog rather than
 * generic recomp-ui Settings.
 */
static void mmx4_widescreen_activate(void) {
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
}

PSX_MOD_CONSTRUCTOR(mmx4_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.widescreen", mmx4_widescreen_activate);
}
