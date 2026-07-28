#include "mod_plugins.h"

#include <stdlib.h>
#include <string.h>

#define PKG "mmx4.enhancement.frame-interpolation"
#define FEATURE "frame-interpolation"

/*
 * Presentation-only interpolation: completed frames are blended at the chosen
 * output rate while guest VBlank, game logic, timers, and audio stay at X4's
 * stock cadence.
 */
static void mmx4_frame_interpolation_activate(void) {
    char rate[16];
    unsigned long fps = 0ul; /* 0 = follow measured display refresh */

    if (psx_mod_option_value(PKG, FEATURE, "rate", rate, sizeof rate) &&
        strcmp(rate, "display") != 0) {
        char* end = rate;
        const unsigned long parsed = strtoul(rate, &end, 10);
        if (end != rate && *end == '\0') fps = parsed;
    }

    (void)psx_mod_set_frame_interpolation((uint32_t)fps);
}

PSX_MOD_CONSTRUCTOR(mmx4_register_frame_interpolation_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.frame-interpolation", mmx4_frame_interpolation_activate);
}
