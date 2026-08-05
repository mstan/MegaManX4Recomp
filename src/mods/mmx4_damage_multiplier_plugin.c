#include "mod_plugins.h"
#include "cpu_state.h"

#include <stdlib.h>

#define PKG "mmx4.cheat.damage-multiplier"
#define FEATURE "damage-multiplier"
#define DAMAGE_FUNCTION 0x8002DE30u
#define DAMAGE_LOAD_ADDRESS 0x8002DEF8u
#define DAMAGE_SHIFT_ADDRESS 0x8002DF00u
#define STOCK_DAMAGE_LOAD 0x9083005Cu
#define STOCK_DAMAGE_SHIFT 0x00000000u

static uint32_t s_damage_multiplier = 1u;

static void mmx4_damage_multiplier_on_hit(
    struct CPUState* cpu, uint32_t address) {
    const uint32_t target = cpu->gpr[4];
    const uint32_t hit = cpu->gpr[5];
    const int32_t weapon =
        (int32_t)(int8_t)psx_mod_read_byte(hit + 1u);
    const uint32_t table = psx_mod_read_word(target + 88u);
    const uint32_t damage_address =
        table + (uint32_t)(weapon * 2) + 1u;
    const uint32_t stock_damage = psx_mod_read_byte(damage_address);

    (void)address;
    /*
     * The recompiler-owned instruction at 0x8002DEFC moves t8 into v0.
     * t8 is otherwise unused by this routine. Supplying the pre-scaled value
     * here supports every integer multiplier without mutating damage tables.
     */
    cpu->gpr[24] = stock_damage * s_damage_multiplier;
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
    (void)psx_mod_register_function_entry_plugin(
        "mmx4.damage-multiplier", DAMAGE_FUNCTION,
        mmx4_damage_multiplier_on_hit);
}
