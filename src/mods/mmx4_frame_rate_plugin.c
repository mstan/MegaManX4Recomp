#include "mod_plugins.h"

/*
 * Keep MMX4's guest cadence, game logic, timers, and audio stock. These
 * callbacks only select how frequently the OpenGL presenter blends between
 * completed guest frames.
 */
static void mmx4_frame_rate_set(unsigned frames_per_second) {
    (void)psx_mod_set_frame_interpolation_blend(
        PSX_MOD_FRAME_INTERPOLATION_MOTION_ADAPTIVE);
    (void)psx_mod_set_frame_interpolation(frames_per_second);
}

static void mmx4_frame_rate_60_activate(void) {
    mmx4_frame_rate_set(60u);
}

static void mmx4_frame_rate_120_activate(void) {
    mmx4_frame_rate_set(120u);
}

static void mmx4_frame_rate_144_activate(void) {
    mmx4_frame_rate_set(144u);
}

static void mmx4_frame_rate_165_activate(void) {
    mmx4_frame_rate_set(165u);
}

static void mmx4_frame_rate_display_activate(void) {
    mmx4_frame_rate_set(0u);
}

PSX_MOD_CONSTRUCTOR(mmx4_register_frame_rate_plugins) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.framerate.60", mmx4_frame_rate_60_activate);
    (void)psx_mod_register_activation_plugin(
        "mmx4.framerate.120", mmx4_frame_rate_120_activate);
    (void)psx_mod_register_activation_plugin(
        "mmx4.framerate.144", mmx4_frame_rate_144_activate);
    (void)psx_mod_register_activation_plugin(
        "mmx4.framerate.165", mmx4_frame_rate_165_activate);
    (void)psx_mod_register_activation_plugin(
        "mmx4.framerate.display", mmx4_frame_rate_display_activate);
}
