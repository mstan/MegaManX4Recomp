#include "mod_plugins.h"
#include "cpu_state.h"

#include <stdlib.h>

#define PKG "mmx4.cheat.damage-multiplier"
#define FEATURE "damage-multiplier"
#define DAMAGE_SUBTRACT_ADDRESS 0x8002DF04u
#define STOCK_DAMAGE_SUBTRACT 0x00621823u
#define DAMAGE_LOAD_ADDRESS 0x8002DEF8u
#define DAMAGE_SHIFT_ADDRESS 0x8002DF00u
#define STOCK_DAMAGE_LOAD 0x9083005Cu
#define STOCK_DAMAGE_SHIFT 0x00000000u

static uint32_t s_damage_multiplier = 1u;

static void mmx4_damage_multiplier_on_hit(
    struct CPUState* cpu, uint32_t address) {
    (void)address;
    /* At original SUBU v1,v1,v0, the delayed byte load has committed.
     * The native damage load stays intact when this feature is disabled. */
    uint32_t scaled=cpu->gpr[2]*s_damage_multiplier;
    cpu->gpr[2]=scaled>127u?127u:scaled;

}

static void mmx4_damage_multiplier_enforce(void) {
    if (!psx_mod_game_started()) return;

    /*
     * Older saves can contain either the former one-hit patch at DEF8 or the
     * interim power-of-two shift at DF00. Restore both guest instructions.
     * The current multiplier lives in the trusted host hook, so the original
     * instruction bytes also let exact native-code validation resume.
     */
    if (psx_mod_read_word(DAMAGE_LOAD_ADDRESS) != STOCK_DAMAGE_LOAD) {
        psx_mod_write_code_word(DAMAGE_LOAD_ADDRESS, STOCK_DAMAGE_LOAD);
    }
    if (psx_mod_read_word(DAMAGE_SHIFT_ADDRESS) != STOCK_DAMAGE_SHIFT) {
        psx_mod_write_code_word(
            DAMAGE_SHIFT_ADDRESS, STOCK_DAMAGE_SHIFT);
    }
}

static void mmx4_damage_multiplier_activate(void) {
    char value[8];
    unsigned long multiplier = 1ul;

    if (psx_mod_option_value(
            PKG, FEATURE, "multiplier", value, sizeof value)) {
        char* end = value;
        const unsigned long parsed = strtoul(value, &end, 10);
        if (end != value && *end == '\0' &&
            parsed >= 1ul && parsed <= 255ul) {
            multiplier = parsed;
        }
    }

    s_damage_multiplier = (uint32_t)multiplier;
    mmx4_damage_multiplier_enforce();
}

PSX_MOD_CONSTRUCTOR(mmx4_register_damage_multiplier_plugin) {
    (void)psx_mod_register_activation_plugin(
        "mmx4.damage-multiplier", mmx4_damage_multiplier_activate);
    (void)psx_mod_register_vblank_plugin(
        "mmx4.damage-multiplier", mmx4_damage_multiplier_enforce);
    (void)psx_mod_register_instruction_plugin(
        "mmx4.damage-multiplier", DAMAGE_SUBTRACT_ADDRESS, STOCK_DAMAGE_SUBTRACT,
        mmx4_damage_multiplier_on_hit);
}
