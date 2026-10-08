#include "mod_plugins.h"

#include <string.h>

#define PKG "mmx4.enhancement.fast-loading"
#define FEATURE "fast-loading"

/*
 * One choice owns loading acceleration for a launch. Host turbo and
 * guest-visible CD timing are deliberately mutually exclusive.
 */
static void mmx4_fast_loading_activate(void) {
    char mode[32] = "host-4x";
    (void)psx_mod_option_value(
        PKG, FEATURE, "mode", mode, sizeof mode);

    if (strcmp(mode, "host-2x") == 0) {
        (void)psx_mod_set_load_acceleration(2u, 0u);
    } else if (strcmp(mode, "host-8x") == 0) {
        (void)psx_mod_set_load_acceleration(8u, 0u);
    } else if (strcmp(mode, "host-16x") == 0) {
        (void)psx_mod_set_load_acceleration(16u, 0u);
    } else if (strcmp(mode, "host-uncapped") == 0) {
        (void)psx_mod_set_load_acceleration(0u, 0u);
    } else if (strcmp(mode, "disc-2x") == 0) {
        (void)psx_mod_set_disc_speed(2u, 0u);
    } else if (strcmp(mode, "disc-4x") == 0) {
        (void)psx_mod_set_disc_speed(4u, 0u);
    } else if (strcmp(mode, "disc-instant") == 0) {
        (void)psx_mod_set_disc_speed(0u, 32u);
    } else {
        (void)psx_mod_set_load_acceleration(4u, 0u);
    }
}

PSX_MOD_CONSTRUCTOR(mmx4_register_fast_loading_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.fast-loading", mmx4_fast_loading_activate);
}
