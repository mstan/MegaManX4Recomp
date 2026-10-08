#ifndef MMX4_COOP_INTERNAL_H
#define MMX4_COOP_INTERNAL_H
#include <stdint.h>
#include "cpu_state.h"
#include "mod_plugins.h"
#define MMX4_PLAYER 0x801418C8u
#define MMX4_PLAY 0x801721C0u
#define MMX4_VEHICLE 0x80173A30u
/* Private runtime interface. Projection is strictly scoped and non-nesting;
 * callers check projected() before intercepting a native routine. */
int mmx4_coop_ready(void);
int mmx4_coop_projected(void);
int mmx4_coop_alive(unsigned seat);
uint8_t *mmx4_coop_second_body(void);
uint8_t *mmx4_coop_second_vehicle(void);
uint8_t *mmx4_coop_first_vehicle(void);
void mmx4_coop_enter_second(void);
void mmx4_coop_leave_second(void);
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1);
int mmx4_coop_finish(CPUState *cpu,uint32_t result);
uint16_t mmx4_coop_input(unsigned seat);
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat);
void mmx4_coop_combat_project_end(uint32_t vehicle_mirror);
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);
/* Implemented by lifecycle module, called around every private projection. */
void mmx4_coop_lifecycle_project(void);
void mmx4_coop_lifecycle_restore(void);
void mmx4_coop_lifecycle_reset(void);
void mmx4_coop_lifecycle_enrolled(CPUState *cpu);
void mmx4_coop_lifecycle_tick(CPUState *cpu);
int mmx4_coop_lifecycle_can_tick(void);
uint32_t mmx4_coop_lifecycle_digest(uint32_t seed);
uint32_t mmx4_coop_combat_digest(uint32_t seed);
#endif
