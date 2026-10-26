#ifndef MMX4_COOP_HUD_H
#define MMX4_COOP_HUD_H
#include <stdint.h>

/* Native 800254AC..8002555C damage-trail update, without emitting packets.
 * Call once alongside the canonical HUD, never from a private render pass.
 * Actual health (+5C) and every other body field remain untouched. */
void mmx4_coop_hud_tick(uint8_t body[0xE4],uint32_t native_frame);
#endif
