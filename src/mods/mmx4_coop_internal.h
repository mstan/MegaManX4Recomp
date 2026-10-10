#ifndef MMX4_COOP_INTERNAL_H
#define MMX4_COOP_INTERNAL_H
#include <stdint.h>
#include "cpu_state.h"

void mmx4_coop_audio_reset(void);
int mmx4_coop_audio_load(unsigned character,const uint8_t *file,uint32_t size);
#include "mod_plugins.h"
#include "mmx4_coop_views.h"
#define MMX4_PLAYER 0x801418C8u
#define MMX4_PLAY 0x801721C0u
#define MMX4_VEHICLE 0x80173A30u
/* Private runtime interface. Projection is strictly scoped and non-nesting;
 * callers check projected() before intercepting a native routine. */
int mmx4_coop_ready(void);
void mmx4_coop_development_mask_pad(void);
int mmx4_coop_projected(void);
int mmx4_coop_split_views(void);
int mmx4_coop_split_activate(void);
void mmx4_coop_split_reset(void);
uint32_t mmx4_coop_split_digest(uint32_t seed);
void mmx4_coop_split_camera(CPUState *cpu,unsigned owner);
void mmx4_coop_split_camera_prepare(unsigned owner);
int mmx4_coop_split_scene_camera_begin(unsigned owner,uint8_t canonical[0xFC]);
void mmx4_coop_split_scene_camera_end(unsigned owner,const uint8_t canonical[0xFC],int committed);
void mmx4_coop_split_actors(Mmx4CoopViewActor actors[2]);
int mmx4_coop_split_camera_copy(unsigned seat,uint8_t layers[0xFC]);
unsigned mmx4_coop_split_view_seat(unsigned seat);
void mmx4_coop_lifecycle_view_save(void);
void mmx4_coop_lifecycle_view_restore(void);
void mmx4_widescreen_view_save(void);
void mmx4_widescreen_view_restore(void);
int mmx4_coop_alive(unsigned seat);
uint8_t *mmx4_coop_second_body(void);
uint8_t *mmx4_coop_first_body(void);
uint8_t *mmx4_coop_second_vehicle(void);
uint8_t *mmx4_coop_first_vehicle(void);
void mmx4_coop_enter_second(void);
void mmx4_coop_leave_second(void);
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1);
int mmx4_coop_finish(CPUState *cpu,uint32_t result);
uint16_t mmx4_coop_input(unsigned seat);
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat);
void mmx4_coop_combat_reset(void);
void mmx4_coop_combat_project_end(uint32_t vehicle_mirror);
int mmx4_coop_combat_actor_context(void);
int mmx4_coop_combat_canonical_call(CPUState *cpu,uint32_t address,
    PSXModFunctionFilterCallback callback);
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);
/* Implemented by lifecycle module, called around every private projection. */
void mmx4_coop_lifecycle_project(void);
void mmx4_coop_lifecycle_follow_progress(uint8_t *body,unsigned character);
void mmx4_coop_lifecycle_restore(void);
void mmx4_coop_lifecycle_reset(void);
void mmx4_coop_lifecycle_enrolled(CPUState *cpu);
void mmx4_coop_lifecycle_tick(CPUState *cpu);
int mmx4_coop_lifecycle_can_tick(void);
/* -1 during ordinary play; otherwise the native shared script's seat. */
int mmx4_coop_lifecycle_script_owner(void);
int mmx4_coop_lifecycle_hidden(unsigned seat);
void mmx4_coop_lifecycle_camera_bounds(CPUState *cpu,unsigned owner);
void mmx4_coop_clear_current_attacks(void);
void mmx4_coop_combat_clear_current_effects(void);
/* Current projected player and vehicle, at enrollment or incoming handoff. */
void mmx4_coop_spawn_facing_right(void);
uint32_t mmx4_coop_lifecycle_digest(uint32_t seed);
uint32_t mmx4_coop_combat_digest(uint32_t seed);
#endif
