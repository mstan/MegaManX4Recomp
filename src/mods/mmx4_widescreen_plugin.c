#include "mod_plugins.h"

/*
 * MMX4 uses the shared Capcom tile-ring format, but unlike MMX5/MMX6 its
 * streamer does not compose layer scroll with a parent selected at +0x52.
 * The game.toml layout was validated against the native 21-column window;
 * refill all 29 visible columns before rendering so stage-entry ring contents
 * cannot leak stale tiles into the widescreen view.
 */
extern void gpu_ws_mmx6_set_freshfix(int on);
extern void gpu_ws_bg2d_set_parent_links(int on);

/*
 * X4's background-ring, culling, and HUD hooks remain part of generated/runtime
 * code, but their player-facing activation belongs to the Mods catalog rather
 * than generic recomp-ui Settings.
 */
static void mmx4_widescreen_activate(void) {
    gpu_ws_bg2d_set_parent_links(0);
    gpu_ws_mmx6_set_freshfix(1);
    (void)psx_mod_set_fixed_display_aspect(16u, 9u);
}

PSX_MOD_CONSTRUCTOR(mmx4_register_widescreen_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.widescreen", mmx4_widescreen_activate);
}
