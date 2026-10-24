/* SLUS-00561 native lifecycle and seat ownership. See
 * docs/COOP_LIFECYCLE_MENU_EVIDENCE.md for original executable evidence.
 * P1 owns the one native campaign/save record, including all upgrades.
 * P2's transient health/ammo/weapon, optional E-tank contents and menu/pickup
 * ownership live here. Tank acquisition still belongs to the campaign. */
#include "mmx4_coop_internal.h"
#include "mmx4_cheats.h"
#include "mod_netplay.h"
#include "gpu.h"
#include "ws_view_anchor.h"
#include <string.h>
#include <stdio.h>

#define ID "mmx4.coop"
#define PLAYER MMX4_PLAYER
#define PLAY MMX4_PLAY
#define MENU 0x801754A0u
#define PAD 0x80166C08u
#define START 0x0800u
#define SELECT 0x0100u
#define PICKUP_CAPACITY 128u
static int door_pool_actor(uint32_t actor) {
    return actor>=0x80165A30u && actor<0x80166BB0u && (actor-0x80165A30u)%0x8Cu==0;
}
static unsigned frost_path_contact_call;

typedef struct {
    uint8_t character, hp, ammo[16], weapon, etanks[2];
} Inventory;
typedef struct { uint32_t actor; uint8_t owner; } PickupOwner;
typedef struct { uint32_t actor,serial; uint8_t owner,recovered; } SceneOwner;
typedef struct {
    uint32_t actor,serial;
    uint16_t old_min,target_min,right;
    uint8_t phase,owner,stage,section;
    uint16_t bounds[4];
} BothDoor;
/* 1 opening, 2 physical traversal, 3 camera commitment, 4 next boss
 * admission, 5 its conversation. C4 remains the retail boss admission gate
 * until everyone clears; only the player's control pass temporarily masks it. */
static BothDoor both_door;
/* Exposes the combat module's independent previous-frame solid history. */
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);
static Inventory second_inventory, saved_inventory;
static Inventory view_second_inventory,view_saved_inventory;
static unsigned view_inventory_initialized,view_inventory_projected;
static PickupOwner pickups[PICKUP_CAPACITY];
static SceneOwner scenes[32];
static uint16_t menu_previous[2], select_previous;
static uint16_t story_pad[3];
static unsigned inventory_initialized, inventory_projected;
static unsigned separate_etanks;
/* Native MENU and its saved pre-pause weapon are personal. The world, fade
 * controller, campaign Quit and W-tank remain shared and run at most once. */
typedef struct { uint8_t native[0x34], original_weapon; } PersonalMenu;
static PersonalMenu personal_menu[2];
static unsigned simultaneous_menus, personal_active, personal_initialized, personal_eligible;
/* Quit is a shared campaign frontend, controlled by its requesting seat. */
static unsigned personal_modal_owner;
static unsigned personal_pass, personal_seat, personal_rendering;
static unsigned stage_known, last_stage, last_section, last_lives;
static unsigned carry_resources, carry_dead, carry_first_dead, carry_departure, carry_refill;
static unsigned departed, select_ticks, select_release, rejoin_pending;
static unsigned paired_victory;
static unsigned full_stage_load;
static unsigned reward_committed;
static unsigned first_before_clear_valid, first_before_clear_dead;
static uint8_t departure_active, departure_visible;
static uint8_t departure_vehicle_active, departure_vehicle_visible;
static unsigned menu_active, menu_owner, menu_projecting;
static unsigned normal_guard, menu_guard, world_guard, pickup_guard;
static unsigned collect_guard, save_guard, reward_guard, death_guard, death_check_guard, capsule_guard;
static unsigned scene_guard, script_pool_guard, command_guard, control_guard,text_guard,text_init_guard;
static unsigned door_wait;
static unsigned door_terrain_guard;
static uint32_t projected_portal_solid;
static unsigned flip_guard;
static unsigned script_active, script_owner, script_serial;
static unsigned warp_pending,warp_phase,warp_owner,warp_guard,warp_unlocked_ticks;
static unsigned initial_arrival;
static unsigned direct_initial_spawn;
static uint8_t warp_active,warp_visible,warp_vehicle_active,warp_vehicle_visible;
static uint32_t warp_origin_x,warp_origin_y;
/* Native stage kind5 owns floor momentum outside the player body. Retain
 * P2's own controller state without allocating another world actor. */
static uint8_t surface_valid[32],surface_second[32][0x30];
static unsigned surface_call;
/* Logging history is host-only and never influences simulation decisions. */
static struct { unsigned valid,stage,section,hp[2],second_active,second_hidden; } trace_previous;
static void start_arrival(CPUState *cpu,unsigned seat);
static uint32_t body_word(const uint8_t *body,unsigned at);

int mmx4_coop_lifecycle_snapshot_ready(void) {
    return !(inventory_projected || menu_projecting || personal_pass || personal_rendering || normal_guard || menu_guard || world_guard ||
        pickup_guard || collect_guard || save_guard || reward_guard || death_guard || death_check_guard ||
        capsule_guard || scene_guard || script_pool_guard || command_guard || control_guard || text_guard ||
        text_init_guard || flip_guard || warp_guard || door_wait || door_terrain_guard || surface_call || frost_path_contact_call);
}
static void snapshot_inventory(Mmx4SnapshotIO *s,Inventory *inventory) {
    MMX4_SNAPSHOT_BOOL(s,inventory->character);MMX4_SNAPSHOT_U8(s,inventory->hp);
    mmx4_snapshot_bytes(s,inventory->ammo,sizeof inventory->ammo);
    MMX4_SNAPSHOT_U8(s,inventory->weapon);
    for(unsigned i=0;i<2;++i)MMX4_SNAPSHOT_U8(s,inventory->etanks[i]);
}
int mmx4_coop_lifecycle_snapshot(Mmx4SnapshotIO *s) {
    /* Campaign/upgrades are owned by P1's native save record in guest RAM. */
    mmx4_snapshot_fixed(s,separate_etanks);
    mmx4_snapshot_fixed(s,simultaneous_menus);
    MMX4_SNAPSHOT_BOOL(s,personal_active);MMX4_SNAPSHOT_BOOL(s,personal_initialized);
    MMX4_SNAPSHOT_FIELD(s,personal_eligible,1,3);
    MMX4_SNAPSHOT_BOOL(s,personal_modal_owner);
    for(unsigned seat=0;seat<2;++seat) {
        mmx4_snapshot_bytes(s,personal_menu[seat].native,sizeof personal_menu[seat].native);
        MMX4_SNAPSHOT_U8(s,personal_menu[seat].original_weapon);
    }
    snapshot_inventory(s,&second_inventory);snapshot_inventory(s,&saved_inventory);
    for(unsigned i=0;i<PICKUP_CAPACITY;++i) {
        uint32_t actor=mmx4_snapshot_integer(s,pickups[i].actor,4,UINT32_MAX);
        if(!mmx4_snapshot_guest(actor))s->ok=0;
        if(s->mode==MMX4_SNAPSHOT_READ)pickups[i].actor=actor;
        MMX4_SNAPSHOT_BOOL(s,pickups[i].owner);
    }
    for(unsigned i=0;i<32;++i) {
        uint32_t actor=mmx4_snapshot_integer(s,scenes[i].actor,4,UINT32_MAX);
        if(!mmx4_snapshot_guest(actor))s->ok=0;
        if(s->mode==MMX4_SNAPSHOT_READ)scenes[i].actor=actor;
        MMX4_SNAPSHOT_U32(s,scenes[i].serial);
        MMX4_SNAPSHOT_BOOL(s,scenes[i].owner);MMX4_SNAPSHOT_BOOL(s,scenes[i].recovered);
    }
    for(unsigned i=0;i<2;++i)MMX4_SNAPSHOT_U16(s,menu_previous[i]);
    MMX4_SNAPSHOT_U16(s,select_previous);
    for(unsigned i=0;i<3;++i)MMX4_SNAPSHOT_U16(s,story_pad[i]);
    MMX4_SNAPSHOT_BOOL(s,inventory_initialized);MMX4_SNAPSHOT_BOOL(s,stage_known);
    MMX4_SNAPSHOT_FIELD(s,last_stage,1,255);MMX4_SNAPSHOT_FIELD(s,last_section,1,255);
    MMX4_SNAPSHOT_FIELD(s,last_lives,1,255);
    MMX4_SNAPSHOT_BOOL(s,carry_resources);MMX4_SNAPSHOT_BOOL(s,carry_dead);
    MMX4_SNAPSHOT_BOOL(s,carry_first_dead);MMX4_SNAPSHOT_BOOL(s,carry_departure);
    MMX4_SNAPSHOT_BOOL(s,carry_refill);MMX4_SNAPSHOT_BOOL(s,departed);
    MMX4_SNAPSHOT_BOOL(s,paired_victory);
    MMX4_SNAPSHOT_FIELD(s,select_ticks,4,90);MMX4_SNAPSHOT_BOOL(s,select_release);
    MMX4_SNAPSHOT_BOOL(s,rejoin_pending);MMX4_SNAPSHOT_BOOL(s,full_stage_load);
    MMX4_SNAPSHOT_BOOL(s,reward_committed);MMX4_SNAPSHOT_BOOL(s,first_before_clear_valid);
    MMX4_SNAPSHOT_BOOL(s,first_before_clear_dead);
    MMX4_SNAPSHOT_U8(s,departure_active);MMX4_SNAPSHOT_U8(s,departure_visible);
    MMX4_SNAPSHOT_U8(s,departure_vehicle_active);MMX4_SNAPSHOT_U8(s,departure_vehicle_visible);
    MMX4_SNAPSHOT_BOOL(s,menu_active);MMX4_SNAPSHOT_BOOL(s,menu_owner);
    MMX4_SNAPSHOT_BOOL(s,script_active);MMX4_SNAPSHOT_BOOL(s,script_owner);
    MMX4_SNAPSHOT_U32(s,script_serial);MMX4_SNAPSHOT_BOOL(s,warp_pending);
    MMX4_SNAPSHOT_FIELD(s,warp_phase,1,3);MMX4_SNAPSHOT_BOOL(s,warp_owner);
    MMX4_SNAPSHOT_U32(s,warp_unlocked_ticks);
    MMX4_SNAPSHOT_U8(s,warp_active);MMX4_SNAPSHOT_U8(s,warp_visible);
    MMX4_SNAPSHOT_U8(s,warp_vehicle_active);MMX4_SNAPSHOT_U8(s,warp_vehicle_visible);
    MMX4_SNAPSHOT_U32(s,warp_origin_x);MMX4_SNAPSHOT_U32(s,warp_origin_y);
    MMX4_SNAPSHOT_BOOL(s,initial_arrival);MMX4_SNAPSHOT_BOOL(s,direct_initial_spawn);
    uint32_t door_actor=mmx4_snapshot_integer(s,both_door.actor,4,UINT32_MAX);
    if(!mmx4_snapshot_guest(door_actor))s->ok=0;
    if(s->mode==MMX4_SNAPSHOT_READ)both_door.actor=door_actor;
    MMX4_SNAPSHOT_U32(s,both_door.serial);
    MMX4_SNAPSHOT_U16(s,both_door.old_min);MMX4_SNAPSHOT_U16(s,both_door.target_min);
    MMX4_SNAPSHOT_U16(s,both_door.right);
    uint32_t door_phase=mmx4_snapshot_integer(s,both_door.phase,1,5);
    if(door_phase && !door_pool_actor(door_actor))s->ok=0;
    if(s->mode==MMX4_SNAPSHOT_READ)both_door.phase=(uint8_t)door_phase;
    MMX4_SNAPSHOT_BOOL(s,both_door.owner);
    MMX4_SNAPSHOT_U8(s,both_door.stage);MMX4_SNAPSHOT_U8(s,both_door.section);
    for(unsigned i=0;i<4;++i)MMX4_SNAPSHOT_U16(s,both_door.bounds[i]);
    for(unsigned i=0;i<32;++i) {
        MMX4_SNAPSHOT_BOOL(s,surface_valid[i]);
        mmx4_snapshot_bytes(s,surface_second[i],sizeof surface_second[i]);
    }
    if(s->mode==MMX4_SNAPSHOT_READ) {
        memset(&view_second_inventory,0,sizeof view_second_inventory);
        memset(&view_saved_inventory,0,sizeof view_saved_inventory);
        view_inventory_initialized=view_inventory_projected=0;
        trace_previous.valid=0;
    }
    return s->ok;
}

static void read_bytes(uint32_t address,uint8_t *out,unsigned count) {
    for(unsigned i=0;i<count;++i)out[i]=psx_mod_read_byte(address+i);
}
static void write_bytes(uint32_t address,const uint8_t *in,unsigned count) {
    for(unsigned i=0;i<count;++i)psx_mod_write_byte(address+i,in[i]);
}
static void inventory_read(Inventory *out) {
    out->character=psx_mod_read_byte(PLAY+0x43);
    out->hp=psx_mod_read_byte(PLAY+0x45);
    read_bytes(PLAY+0x49,out->ammo,16);
    out->weapon=psx_mod_read_byte(PLAY+0x60);
    read_bytes(PLAY+0x5C,out->etanks,2);
}
static void inventory_write(const Inventory *in) {
    psx_mod_write_byte(PLAY+0x43,in->character);
    psx_mod_write_byte(PLAY+0x45,in->hp);
    write_bytes(PLAY+0x49,in->ammo,16);
    psx_mod_write_byte(PLAY+0x60,in->weapon);
    if(separate_etanks) {
        /* Both players own tanks acquired by the one campaign. Contents
         * belong to the projected seat; W-tanks remain shared. */
        unsigned flags=psx_mod_read_half(PLAY+0x5A);
        for(unsigned i=0;i<2;++i)psx_mod_write_byte(PLAY+0x5C+i,
            (uint8_t)(in->etanks[i]|((flags&(0x1000u<<i))?0x80u:0u)));
    }
}
void mmx4_coop_lifecycle_project(void) {
    if(inventory_projected)return;
    inventory_read(&saved_inventory);
    if(!inventory_initialized || second_inventory.character!=(saved_inventory.character^1u)) {
        memset(&second_inventory,0,sizeof second_inventory);
        second_inventory.character=(uint8_t)(saved_inventory.character^1u);
        second_inventory.hp=psx_mod_read_byte(PLAY+0x46);
        memset(second_inventory.ammo,48,sizeof second_inventory.ammo);
        memcpy(second_inventory.etanks,saved_inventory.etanks,2);
        inventory_initialized=1;
    }
    inventory_write(&second_inventory);inventory_projected=1;
}
void mmx4_coop_lifecycle_restore(void) {
    if(!inventory_projected)return;
    inventory_read(&second_inventory);
    inventory_write(&saved_inventory);inventory_projected=0;
}
void mmx4_coop_lifecycle_view_save(void) {
    view_second_inventory=second_inventory;view_saved_inventory=saved_inventory;
    view_inventory_initialized=inventory_initialized;view_inventory_projected=inventory_projected;
}
void mmx4_coop_lifecycle_view_restore(void) {
    second_inventory=view_second_inventory;saved_inventory=view_saved_inventory;
    inventory_initialized=view_inventory_initialized;inventory_projected=view_inventory_projected;
    /* A render watchdog can unwind before draw_menu clears its guard. View
     * restoration is outside that draw, including the failure path. */
    personal_rendering=0;
}

static PickupOwner *pickup_record(uint32_t actor,int create) {
    PickupOwner *empty=NULL;
    for(unsigned i=0;i<PICKUP_CAPACITY;++i) {
        if(pickups[i].actor==actor)return &pickups[i];
        if(!pickups[i].actor && !empty)empty=&pickups[i];
    }
    if(create && empty) {empty->actor=actor;empty->owner=0;return empty;}
    return NULL;
}
static void pickup_forget(uint32_t actor) {
    PickupOwner *record=pickup_record(actor,0);
    if(record)memset(record,0,sizeof *record);
}
static void fresh_game(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    mmx4_coop_bosses_new_stage();
    memset(&second_inventory,0,sizeof second_inventory);
    mmx4_coop_balance_reset();
    memset(&saved_inventory,0,sizeof saved_inventory);
    memset(pickups,0,sizeof pickups);
    memset(scenes,0,sizeof scenes);
    memset(surface_valid,0,sizeof surface_valid);memset(surface_second,0,sizeof surface_second);
    memset(&both_door,0,sizeof both_door);
    memset(menu_previous,0,sizeof menu_previous);
    memset(story_pad,0,sizeof story_pad);
    inventory_initialized=inventory_projected=stage_known=0;
    carry_resources=carry_dead=carry_first_dead=carry_departure=carry_refill=0;
    departed=select_ticks=select_release=select_previous=rejoin_pending=0;
    full_stage_load=0;
    reward_committed=paired_victory=0;
    first_before_clear_valid=first_before_clear_dead=0;
    script_active=script_owner=script_serial=0;
    warp_pending=warp_phase=warp_owner=warp_guard=warp_unlocked_ticks=0;
    initial_arrival=direct_initial_spawn=0;
    warp_active=warp_visible=warp_vehicle_active=warp_vehicle_visible=0;
    warp_origin_x=warp_origin_y=0;
    flip_guard=0;
    departure_vehicle_active=departure_vehicle_visible=0;
    menu_active=menu_owner=menu_projecting=0;
    personal_active=personal_initialized=personal_eligible=personal_modal_owner=0;
    personal_pass=personal_seat=personal_rendering=0;
    memset(personal_menu,0,sizeof personal_menu);
    trace_previous.valid=0;
}
void mmx4_coop_lifecycle_activate(void) {
    char value[16];
    simultaneous_menus=!psx_mod_current_option_value("simultaneous_menus",value,sizeof value) ||
        strcmp(value,"false");
    separate_etanks=psx_mod_current_option_value("separate_etanks",value,sizeof value) &&
        !strcmp(value,"true");
    separate_etanks|=simultaneous_menus;
    /* Reactivation starts a new native runtime; no campaign bytes are touched. */
    fresh_game(NULL,0);
    last_stage=last_section=last_lives=0;
    departure_active=departure_visible=0;
    normal_guard=menu_guard=world_guard=pickup_guard=frost_path_contact_call=0;
    collect_guard=save_guard=reward_guard=death_guard=death_check_guard=capsule_guard=0;
    scene_guard=script_pool_guard=command_guard=control_guard=text_guard=text_init_guard=0;
    door_wait=0;
    door_terrain_guard=0;
    projected_portal_solid=0;
    memset(&view_second_inventory,0,sizeof view_second_inventory);
    memset(&view_saved_inventory,0,sizeof view_saved_inventory);
    view_inventory_initialized=view_inventory_projected=0;
}
static void full_stage(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    mmx4_coop_bosses_new_stage();
    if(!mmx4_coop_projected()) {full_stage_load=1;paired_victory=0;}
}
static void stage_selection(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(mmx4_coop_projected() || psx_mod_local_view_scope())return;
    /* Native8002E420 clears PLAYER for the frontend at8002E478, outside
     * stage_initialization. Selecting the same stage afterward is a fresh
     * entry, not a section transfer with a frontend-zeroed P1 corpse. */
    mmx4_coop_bosses_new_stage();
    stage_known=0;first_before_clear_valid=0;
    carry_resources=carry_dead=carry_first_dead=carry_departure=carry_refill=0;
    direct_initial_spawn=paired_victory=0;
    memset(&both_door,0,sizeof both_door);
}
static unsigned seat_was_dead(unsigned seat) {
    const uint8_t *body=mmx4_coop_second_body();
    unsigned hp=seat?body[0x5C]:psx_mod_read_byte(PLAYER+0x5C);
    unsigned state=seat?body[4]:psx_mod_read_byte(PLAYER+4);
    /* Native outgoing beams finish in state 3 without consuming health.
     * Hidden passengers and voluntarily withdrawn P2 are not corpses. */
    unsigned outgoing=mmx4_coop_lifecycle_hidden(seat) || (seat && departed);
    /* Native death is state2, followed by HP0/state3. A healthy script
     * owner also finishes an outgoing beam in state3 before section loading;
     * state4 is another active native dispatcher, not a corpse. */
    return !(hp&0x7Fu) || (state==2 && !outgoing);
}

void mmx4_coop_lifecycle_follow_progress(uint8_t *body,unsigned character) {
    body[0xB9]=psx_mod_read_byte(PLAY+0x59);
    if(character==0) {
        body[0xA7]=psx_mod_read_byte(PLAY+0x47);
        /* Native capsule800C712C/player800314FC use B8 for this upgrade.
         * A6 is a transient weapon override and must remain seat-local. */
        body[0xB8]=psx_mod_read_byte(PLAY+0x48);
    }
}
static void stage_initialization(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(mmx4_coop_projected())return;
    mmx4_coop_bosses_stage_initialization();
    mmx4_coop_balance_reset();
    direct_initial_spawn=0;
    memset(&both_door,0,sizeof both_door);
    /* Native 8001FC20 calls 8002A7D0/8002A728 to clear PLAYER before
     * 80035240 initializes it. Capture the prior survivor status here;
     * zeroed initialization storage is not evidence of a player death. */
    first_before_clear_dead=seat_was_dead(0);
    first_before_clear_valid=1;
    const uint8_t *second=mmx4_coop_second_body();
    fprintf(stderr,"mmx4.coop: stage-init stage=%u section=%u p1-hp=%u p1-state=%u "
        "p2-hp=%u p2-state=%u warp=%u owner=%u withdrawn=%u\n",
        psx_mod_read_byte(PLAY+0x0C),psx_mod_read_byte(PLAY+0x0D),
        psx_mod_read_byte(PLAYER+0x5C)&127u,psx_mod_read_byte(PLAYER+4),
        second[0x5C]&127u,second[4],warp_phase,warp_owner,departed);
}

void mmx4_coop_lifecycle_reset(void) {
    paired_victory=0;
    uint8_t *body=mmx4_coop_second_body();
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    unsigned section=psx_mod_read_byte(PLAY+0x0D);
    unsigned lives=psx_mod_read_byte(PLAY+0x44);
    unsigned first_was_dead=first_before_clear_valid?first_before_clear_dead:seat_was_dead(0);
    unsigned second_was_dead=seat_was_dead(1);
    unsigned team_was_dead=first_was_dead &&
        (second_was_dead || departed || (!body[0] && !mmx4_coop_lifecycle_hidden(1)));
    unsigned life_preserved=lives>=last_lives && !(last_lives==0 && lives==255);
    /* Retail packs the story boss (Final Weapon2) and Sigma stage3 into
     * slot11's two sections. This is a new level after Double/Iris, not an
     * ordinary mid-stage transfer with resources carried forward. */
    unsigned fortress_stage=stage_known && stage==11 && last_stage==11 &&
        last_section==0 && section==1;
    /* Invoked before the core clears its second body. A section transfer
     * preserves the partner's state. Changed stage or lost shared life is a
     * team respawn. Section identity comes from native PLAY, never host time. */
    carry_resources=stage_known && stage==last_stage && !fortress_stage && life_preserved &&
        !team_was_dead && (!full_stage_load || section!=last_section);
    if(fortress_stage && (first_was_dead || second_was_dead))
        psx_mod_counter_add("mmx4.coop.fortress-stage-rejoins",1);
    unsigned forward_area=carry_resources && section>last_section;
    carry_dead=carry_resources && second_was_dead && !forward_area;
    carry_first_dead=carry_resources && first_was_dead && !forward_area;
    if(forward_area && (first_was_dead || second_was_dead))
        psx_mod_counter_add("mmx4.coop.forward-area-rejoins",1);
    /* Original 80035240 refills HP for spawn policies0 and-2. A living
     * counterpart follows that policy when advancing to the next section;
     * same-section checkpoints and carried deaths retain their resources. */
    unsigned spawn_policy=psx_mod_read_byte(PLAY+0x1E);
    carry_refill=carry_resources && section!=last_section && !carry_dead && !departed &&
        (spawn_policy==0 || spawn_policy==254 || (forward_area && second_was_dead));
    first_before_clear_valid=0;
    carry_departure=carry_resources && departed;
    if(carry_dead)psx_mod_counter_add("mmx4.coop.carried-p2-deaths",1);
    if(carry_first_dead)psx_mod_counter_add("mmx4.coop.carried-p1-deaths",1);
    if(carry_resources && inventory_initialized) {
        second_inventory.hp=(uint8_t)(body[0x5C]&0x7Fu);
        memcpy(second_inventory.ammo,body+0xA8,16);
        second_inventory.weapon=body[0x93];
    }
    if(!carry_resources)departed=select_ticks=select_release=rejoin_pending=0;
    full_stage_load=0;
    last_stage=stage;last_section=section;last_lives=lives;stage_known=1;
    reward_committed=0;
    memset(pickups,0,sizeof pickups);
    memset(scenes,0,sizeof scenes);script_active=0;
    memset(surface_valid,0,sizeof surface_valid);memset(surface_second,0,sizeof surface_second);
    memset(&both_door,0,sizeof both_door);
    warp_pending=warp_phase=warp_unlocked_ticks=0;
    initial_arrival=direct_initial_spawn=0;
    /* A native Quit/Continue retires the old menu continuation. Its callback
     * guard must not survive into the newly initialized stage: a retained
     * guard bypasses P2's menu projection and leaves menu_active latched when
     * the unwrapped native menu resumes gameplay. */
    menu_active=0;menu_owner=0;menu_guard=menu_projecting=0;
    personal_active=personal_initialized=personal_eligible=personal_modal_owner=0;
    personal_pass=personal_seat=personal_rendering=0;
    memset(personal_menu,0,sizeof personal_menu);
}

static int native_initial_arrival(void) {
    /* 8001FD1C holds PLAY+1C through the first beam. 80035694 sets +1F
     * and enters state1/action0 via 80035848; 80031520 releases the lock
     * only when that arrival animation finishes. No story lock is waived. */
    return !mmx4_coop_projected() && !psx_mod_local_view_scope() &&
        psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+1) &&
        psx_mod_read_byte(PLAY+0x1C)==1 &&
        psx_mod_read_byte(PLAY+0x1E) && psx_mod_read_byte(PLAY+0x1F)==1 &&
        psx_mod_read_byte(PLAYER) && psx_mod_read_byte(PLAYER+3) &&
        psx_mod_read_byte(PLAYER+4)==1 && !psx_mod_read_byte(PLAYER+5) &&
        psx_mod_read_byte(PLAYER+0x67)==255 &&
        (psx_mod_read_byte(PLAYER+0x5C)&127u) &&
        !psx_mod_read_byte(PLAYER+0xC0) && !psx_mod_read_byte(PLAYER+0xC3) &&
        !psx_mod_read_byte(PLAYER+0xC4) && !psx_mod_read_byte(PLAYER+0xC5) &&
        !psx_mod_read_byte(PLAYER+0xD9) && !psx_mod_read_byte(PLAY+0x0F) &&
        !psx_mod_read_byte(PLAY+0x10) && !script_active && !warp_pending &&
        !warp_phase && !departed && !carry_dead && !carry_first_dead && !carry_departure;
}
static void direct_spawn(CPUState *cpu,uint32_t address) {
    (void)address;
    if(mmx4_coop_projected() || psx_mod_local_view_scope())return;
    /* 80035694 selects 800358A4 for negative checkpoint returns in stages
     * 3/6. Remember only that native initialization boundary; a normal body,
     * manual rejoin, or door auto-walk is not a request to rewarp P1. Stage12
     * also uses this entry for authored story-specific heights and is kept
     * unchanged. The original initializer still supplies landing Y and box. */
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    direct_initial_spawn=cpu->gpr[31]==0x80035834u && cpu->gpr[4]==PLAYER &&
        ((stage==3 && psx_mod_read_byte(PLAY+0x1D)) || stage==6) &&
        psx_mod_read_byte(PLAY+0x1E)==255 &&
        psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+1) &&
        psx_mod_read_byte(PLAY+0x1C)==1 && psx_mod_read_byte(PLAY+0x1F)==1;
}
static int direct_pair_candidate(void) {
    const uint8_t *body=mmx4_coop_second_body();
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    return direct_initial_spawn && !mmx4_coop_projected() && !psx_mod_local_view_scope() &&
        ((stage==3 && psx_mod_read_byte(PLAY+0x1D)) || stage==6) &&
        psx_mod_read_byte(PLAY+0x1E)==255 &&
        psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+1) &&
        !psx_mod_read_byte(PLAY+0x1C) && psx_mod_read_byte(PLAY+0x1F)==1 &&
        psx_mod_read_byte(PLAYER) && psx_mod_read_byte(PLAYER+3) &&
        psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+5)==2 &&
        (psx_mod_read_byte(PLAYER+0x5C)&127u) && psx_mod_read_word(PLAYER+0x68) &&
        !psx_mod_read_byte(PLAYER+0x67) && !psx_mod_read_byte(PLAYER+0xBC) &&
        !psx_mod_read_byte(PLAYER+0xC0) && !psx_mod_read_byte(PLAYER+0xC3) &&
        !psx_mod_read_byte(PLAYER+0xC4) && !psx_mod_read_byte(PLAYER+0xC5) &&
        !psx_mod_read_byte(PLAYER+0xD9) && body[0] && body[3] && body[4]==1 && body[5]==2 &&
        (body[0x5C]&127u) && body_word(body,0x68) && !body[0x67] && !body[0xBC] &&
        !body[0xC0] && !body[0xC3] && !body[0xC4] && !body[0xC5] && !body[0xD9] &&
        !psx_mod_read_byte(PLAY+0x0F) && !psx_mod_read_byte(PLAY+0x10) &&
        !script_active && !warp_pending && !warp_phase && !departed &&
        !carry_dead && !carry_first_dead && !carry_departure;
}
static int native_chaser_arrival(void) {
    /* Marine Base's native bike entrance owns the lock until READY finishes.
     * Enroll after P1 starts accelerating, so P2 runs the same entrance just
     * behind it instead of appearing after control is released. The existing
     * shared_ride_ready filter retains the one canonical READY actor. */
    return !mmx4_coop_projected() && !psx_mod_local_view_scope() &&
        psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+1) &&
        psx_mod_read_byte(PLAY+0x0C)==5 && !psx_mod_read_byte(PLAY+0x1D) &&
        psx_mod_read_byte(PLAY+0x1C)==1 &&
        psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+5)==18 &&
        psx_mod_read_byte(PLAYER+0xC5)==255 && (psx_mod_read_byte(PLAYER+0x5C)&127u) &&
        psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+4)==1 &&
        !psx_mod_read_byte(MMX4_VEHICLE+5) && psx_mod_read_byte(MMX4_VEHICLE+6)==1 &&
        psx_mod_read_word(MMX4_VEHICLE+0x20)>=4u*65536u &&
        psx_mod_read_word(MMX4_VEHICLE+0xA0) &&
        !script_active && !warp_pending && !warp_phase && !departed &&
        !carry_dead && !carry_first_dead && !carry_departure;
}
int mmx4_coop_lifecycle_can_enroll(void) {
    /* Mushroom's negative portal return installs an idle body before the
     * first native world-solid pass lands it on the entrance platform.
     * Wait for the controller's completed ground contact instead of copying
     * that intermediate terrain Y to the partner. Initial descending beams
     * retain their existing concurrent enrollment path. */
    if(psx_mod_read_byte(PLAY+0x0C)==3 && psx_mod_read_byte(PLAY+0x1D)==1 &&
       psx_mod_read_byte(PLAY+0x1E)==255 && !psx_mod_read_byte(PLAY+0x1C) &&
       !(psx_mod_read_byte(PLAYER+0x89)&8u))return 0;
    return psx_mod_read_byte(PLAYER+4)==1 &&
        (!psx_mod_read_byte(PLAY+0x1C) || native_initial_arrival() || native_chaser_arrival());
}

void mmx4_coop_lifecycle_enrolled(CPUState *cpu) {
    unsigned together=native_initial_arrival();
    uint8_t geometry_wait=together?psx_mod_read_byte(PLAYER+0xBE):0;
    uint8_t *body=mmx4_coop_second_body();
    if(carry_resources) {
        if(carry_refill)second_inventory.hp=body[0x5C]&127u;
        body[0x5C]=body[0x5D]=body[0x5E]=second_inventory.hp;
        memcpy(body+0xA8,second_inventory.ammo,16);
        body[0x93]=second_inventory.weapon;
        if(carry_dead) {
            uint8_t *vehicle=mmx4_coop_second_vehicle();
            body[4]=3;body[5]=body[6]=body[3]=0;
            vehicle[0]=vehicle[3]=0;
        }
        if(carry_departure) {
            uint8_t *vehicle=mmx4_coop_second_vehicle();
            departure_vehicle_active=vehicle[0];departure_vehicle_visible=vehicle[3];
            vehicle[0]=vehicle[3]=body[0]=body[3]=0;departed=1;
        }
        if(!carry_dead && !carry_departure && body[2]==0) {
            mmx4_coop_enter_second();
            mmx4_coop_call(cpu,0x80037104u,PLAYER,0);
            mmx4_coop_call(cpu,0x800371E4u,PLAYER,0);
            mmx4_coop_leave_second();
        }
    }
    body[0xB9]|=psx_mod_read_byte(PLAY+0x59);
    if(carry_first_dead) {
        psx_mod_write_byte(PLAYER+0x5C,0);psx_mod_write_byte(PLAYER+0x5D,0);
        psx_mod_write_byte(PLAYER+0x5E,0);psx_mod_write_byte(PLAYER+4,3);
        psx_mod_write_byte(PLAYER+3,0);psx_mod_write_byte(PLAYER+5,0);
        psx_mod_write_byte(PLAYER+6,0);
        psx_mod_write_byte(MMX4_VEHICLE,0);psx_mod_write_byte(MMX4_VEHICLE+3,0);
    }
    unsigned direct_together=0;
    if(direct_pair_candidate()) {
        /* Original358A4 supplies each character's authored floor height:
         * Zero is one pixel above X. Enrollment's copied P1 Y can leave X
         * just above that floor, so run the same native direct initializer
         * for P2 before applying native camera bounds and fresh terrain at
         * its preferred +24 position. Do not reconstruct a landing offset.
         * +70 is this fresh terrain result; +89 can still be a stale previous
         * controller contact. A missing floor/ceiling contact cannot qualify
         * an additional P1 beam at an unsafe partner placement. */
        unsigned portal_floor=psx_mod_read_byte(PLAY+0x0C)==3 && psx_mod_read_byte(PLAY+0x1D)==1 &&
            !(psx_mod_read_byte(PLAYER+0x70)&8u) && (psx_mod_read_byte(PLAYER+0x89)&8u);
        if(!portal_floor) {
            mmx4_coop_enter_second();
            mmx4_coop_call(cpu,0x800358A4u,PLAYER,0);
            mmx4_coop_leave_second();
        }
        mmx4_coop_lifecycle_camera_bounds(cpu,0);
        mmx4_coop_enter_second();
        mmx4_coop_call(cpu,0x8002C614u,PLAYER,0);
        mmx4_coop_leave_second();
        direct_together=direct_pair_candidate() && (body[0x70]&8u) && !(body[0x70]&4u);
    }
    /* Consume even rejected/dead/withdrawn spawns. No later manual rejoin or
     * ordinary owner movement may inherit a request to restart P1. */
    direct_initial_spawn=0;
    carry_resources=carry_dead=carry_first_dead=carry_departure=carry_refill=0;
    if(!departed && body[4]==1 && !body[0xC5]) {
        warp_owner=0;warp_active=body[0];warp_visible=body[3];
        warp_vehicle_active=warp_vehicle_visible=0;
        warp_origin_x=body_word(body,8);warp_origin_y=body_word(body,12);
        if(!together && !direct_together && psx_mod_read_byte(PLAY+0x0C)==12 &&
           !psx_mod_read_byte(PLAY+0x0D) && psx_mod_read_byte(PLAY+0x1D)>=11 &&
           psx_mod_read_byte(PLAY+0x1D)<=18) {
            /* The refight hub's authored return pads can abut a wall. The
             * enrollment preference of +24 pixels puts X inside the wall
             * beside Slash's pad; an incoming beam has no usable terrain
             * box until it is already too late to avoid the crush. Reuse
             * the grounded partner-return gate: align each hero's feet,
             * probe the native standing box and only then start the beam.
             * P1's checkpoint pose and the actual portal state stay native. */
            body[0]=body[3]=0;warp_phase=1;warp_unlocked_ticks=0;
            psx_mod_counter_add("mmx4.coop.refight-safe-arrivals",1);
            return;
        }
        if(direct_together) {
            start_arrival(cpu,0);
            psx_mod_write_byte(PLAY+0x1C,1);
        }
        start_arrival(cpu,1);
        if(together) {
            /* 80028BF0 copies the authored spawn's +22 into PLAYER+BE.
             * 80031410 descends eight pixels per tick with no collision box
             * until that countdown expires, then calls 80035EA4. Enrollment
             * can occur partway through that descent, so keep the remaining
             * countdown with the initialized position rather than treating
             * an initial beam as a grounded room/rejoin arrival. */
            mmx4_coop_enter_second();
            psx_mod_write_byte(PLAYER+0xBE,geometry_wait);
            if(geometry_wait)psx_mod_write_word(PLAYER+0x68,0);
            else mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
            mmx4_coop_leave_second();
            /* Each body's original controller advances its own incoming
             * animation. P1's +67 is not a scene that suspends this P2 beam. */
            warp_phase=0;initial_arrival=1;
            psx_mod_counter_add("mmx4.coop.concurrent-stage-arrivals",1);
        }else if(direct_together) {
            /* Native358A4 already placed both bodies at their own safe floor.
             * Its old authored +BE belongs to the skipped descent and must
             * stay zero. Keep both original arrival controllers independent. */
            warp_phase=0;initial_arrival=1;
            psx_mod_counter_add("mmx4.coop.concurrent-return-arrivals",1);
        }
    }
}

int mmx4_coop_lifecycle_can_tick(void) {
    const uint8_t *body=mmx4_coop_second_body();
    return !departed && !menu_active && psx_mod_read_byte(PLAY+1)!=2 &&
        !mmx4_coop_lifecycle_hidden(1) && body[0] && body[4]!=3;
}
int mmx4_coop_lifecycle_script_owner(void) {
    return script_active?(int)script_owner:-1;
}
int mmx4_coop_lifecycle_hidden(unsigned seat) {
    return (warp_pending || warp_phase) && seat==(warp_owner^1u);
}
int mmx4_coop_lifecycle_arriving(unsigned seat) {
    return warp_phase==2 && seat==(warp_owner^1u);
}
int mmx4_coop_lifecycle_initial_arrival(void) {return initial_arrival!=0;}
int mmx4_coop_lifecycle_nova_active(void) {
    if(!mmx4_coop_ready() || script_active)return 0;
    /* Original 800375B4 enters X action34 and asserts PLAY+1C. This attack
     * lock is not a shared room transition or a command to join cameras. */
    if(psx_mod_read_byte(PLAYER) && psx_mod_read_byte(PLAYER+2)==0 &&
       psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+5)==0x22)return 1;
    const uint8_t *body=mmx4_coop_projected()?mmx4_coop_first_body():mmx4_coop_second_body();
    return body[0] && !body[2] && body[4]==1 && body[5]==0x22;
}
void mmx4_coop_lifecycle_camera_bounds(CPUState *cpu,unsigned owner) {
    unsigned seat=owner^1u;
    if(!mmx4_coop_ready() || mmx4_coop_projected() ||
       !psx_mod_read_byte(0x801419F4u) || !mmx4_coop_alive(seat))return;
    if(seat)mmx4_coop_enter_second();
    if(!psx_mod_read_byte(PLAYER+0xBC))
        mmx4_coop_call(cpu,0x80027BE4u,0x801419B0u,0);
    if(seat)mmx4_coop_leave_second();
}
static void request_script_departure(unsigned owner) {
    if(script_active && !paired_victory && !warp_pending && !warp_phase && mmx4_coop_alive(owner^1u)) {
        warp_owner=owner;warp_pending=1;
    }
}
void mmx4_coop_lifecycle_room_entry(unsigned owner) {
    if(owner>1 || !mmx4_coop_ready() || mmx4_coop_projected() ||
       script_active || warp_pending || warp_phase || departed ||
       !mmx4_coop_alive(owner) || !mmx4_coop_alive(owner^1u))return;
    memset(&both_door,0,sizeof both_door);
    /* A committed shared-room boundary may precede the boss's dialogue.
     * Carry the excluded partner through the same native outgoing/incoming
     * sequence; arrival waits for the leading player to reach the floor. */
    warp_owner=owner;warp_pending=1;
    psx_mod_counter_add("mmx4.coop.room-departures",1);
}
static uint32_t body_word(const uint8_t *body,unsigned at) {
    return (uint32_t)body[at]|(uint32_t)body[at+1]<<8|
        (uint32_t)body[at+2]<<16|(uint32_t)body[at+3]<<24;
}
static int both_door_scope(void) {
    return both_door.phase && both_door.stage==psx_mod_read_byte(PLAY+0x0C) &&
        both_door.section==psx_mod_read_byte(PLAY+0x0D);
}
int mmx4_coop_lifecycle_paired_door(void) {
    return both_door_scope() && both_door.phase<=3 && script_active &&
        both_door.serial==script_serial;
}
static void both_door_gate(unsigned seat,uint8_t value) {
    if(seat==(unsigned)(mmx4_coop_projected()!=0))psx_mod_write_byte(PLAYER+0xC4,value);
    else {
        uint8_t *body=seat?mmx4_coop_second_body():mmx4_coop_first_body();
        body[0xC4]=value;
    }
}
static int both_door_cleared(void) {
    for(unsigned seat=0;seat<2;++seat) {
        if(!mmx4_coop_alive(seat) || (seat && departed))continue;
        unsigned current=seat==(unsigned)(mmx4_coop_projected()!=0);
        const uint8_t *body=seat?mmx4_coop_second_body():mmx4_coop_first_body();
        int x=(int16_t)(current?psx_mod_read_half(PLAYER+10):(body_word(body,8)>>16));
        uint32_t box=current?psx_mod_read_word(PLAYER+0x68):body_word(body,0x68);
        if(!box || x+(int8_t)psx_mod_read_byte(box)-psx_mod_read_byte(box+2)<=
            (int16_t)both_door.right || x<(int16_t)both_door.target_min+8)return 0;
    }
    return 1;
}
static void both_door_abort(CPUState *cpu) {
    if(both_door.phase<=3 && both_door.phase &&
       script_active && script_serial==both_door.serial) {
        unsigned owner=both_door.owner;
        both_door_gate(0,0);both_door_gate(1,0);
        if(mmx4_coop_alive(owner)) {
            unsigned current=(unsigned)(mmx4_coop_projected()!=0);
            uint8_t campaign=psx_mod_read_byte(PLAY+0x43);
            if(owner!=current) {if(owner)mmx4_coop_enter_second();else mmx4_coop_leave_second();}
            mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
            if(owner!=current) {if(current)mmx4_coop_enter_second();else mmx4_coop_leave_second();}
            psx_mod_write_byte(PLAY+0x43,campaign);
        }
        script_active=0;
        for(unsigned i=0;i<sizeof scenes/sizeof scenes[0];++i)
            if(scenes[i].actor==both_door.actor)memset(&scenes[i],0,sizeof scenes[i]);
    }
    memset(&both_door,0,sizeof both_door);
}
static void both_door_tick(CPUState *cpu) {
    if(!both_door.phase)return;
    if(!both_door_scope() || psx_mod_read_byte(PLAY)!=6 ||
       (!mmx4_coop_alive(0) && !mmx4_coop_alive(1)) ||
       (both_door.phase<=3 && (!door_pool_actor(both_door.actor) ||
        !psx_mod_read_byte(both_door.actor) || psx_mod_read_byte(both_door.actor+1)!=8 ||
        psx_mod_read_byte(both_door.actor+4)!=1 || !psx_mod_read_byte(both_door.actor+5))))
        both_door_abort(cpu);
    if(both_door.phase>=2 && both_door.phase<=3) {
        /* A hero may retreat through a camera return strip while this door
         * remains open. On touching its original admission box again, restore
         * that hero's authored enclosure; do not clamp the distant partner. */
        unsigned seats=0;
        for(unsigned seat=0;seat<2;++seat)if(mmx4_coop_alive(seat)) {
            if(seat)mmx4_coop_enter_second();
            if(mmx4_coop_call(cpu,0x800C1E7Cu,both_door.actor,0))seats|=1u<<seat;
            if(seat)mmx4_coop_leave_second();
        }
        if(seats)mmx4_coop_split_door_camera_bounds(seats,both_door.bounds);
    }
}
static void start_outgoing(CPUState *cpu,unsigned seat) {
    mmx4_coop_armor_dismount(cpu,seat);
    if(seat)mmx4_coop_enter_second();
    mmx4_coop_clear_current_attacks();
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+0xBC,0);
    psx_mod_write_byte(PLAYER+0xBD,0);
    mmx4_coop_call(cpu,0x800350A4u,PLAYER,3);
    mmx4_coop_call(cpu,0x80035048u,PLAYER,0);
    if(seat)mmx4_coop_leave_second();
    warp_phase=3;
}
void mmx4_coop_spawn_facing_right(void) {
    /* Native facing is +15: 0 is left, 40 is right. +14 selects the camera
     * layer and must remain intact. Ride Chasers use the same facing byte. */
    psx_mod_write_byte(PLAYER+0x15,0x40);
    psx_mod_write_byte(MMX4_VEHICLE+0x15,0x40);
}
static void start_arrival(CPUState *cpu,unsigned seat) {
    if(seat)mmx4_coop_enter_second();
    psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+6,0);
    psx_mod_write_byte(PLAYER+0xBE,0);psx_mod_write_byte(PLAYER+0x89,0);
    psx_mod_write_byte(PLAYER+0x70,0);psx_mod_write_byte(PLAYER+0x71,0);
    psx_mod_write_byte(PLAYER+0xBC,0);psx_mod_write_byte(PLAYER+0xBD,0);
    /* The owner/native checkpoint has already supplied a valid world
     * position. Moving the body 96 pixels upward can put it through the
     * ceiling or above the room. Run the native incoming beam at this safe
     * landing position; its pose/beam animation still owns the arrival. */
    mmx4_coop_call(cpu,0x80035848u,PLAYER,0);
    if(seat)mmx4_coop_spawn_facing_right();
    if(seat)mmx4_coop_leave_second();
    warp_phase=2;
}
static void warp_begin(CPUState *cpu) {
    if(!warp_pending || mmx4_coop_projected())return;
    initial_arrival=direct_initial_spawn=0;
    unsigned seat=warp_owner^1u;
    mmx4_coop_armor_dismount(cpu,seat);
    if(seat)mmx4_coop_enter_second();
    warp_active=psx_mod_read_byte(PLAYER);warp_visible=psx_mod_read_byte(PLAYER+3);
    /* Unoccupied Ride Armor belongs to its world location, not a passenger.
     * Chasers retain their existing stage-specific transport behavior. */
    unsigned world_armor=psx_mod_read_byte(MMX4_VEHICLE) &&
        psx_mod_read_byte(MMX4_VEHICLE+1)==1 && !psx_mod_read_byte(PLAYER+0xC5);
    warp_vehicle_active=world_armor?0:psx_mod_read_byte(MMX4_VEHICLE);
    warp_vehicle_visible=world_armor?0:psx_mod_read_byte(MMX4_VEHICLE+3);
    warp_origin_x=psx_mod_read_word(PLAYER+8);warp_origin_y=psx_mod_read_word(PLAYER+12);
    mmx4_coop_clear_current_attacks();
    if(!world_armor) {
        psx_mod_write_byte(MMX4_VEHICLE,0);psx_mod_write_byte(MMX4_VEHICLE+3,0);
    }
    unsigned mounted=psx_mod_read_byte(PLAYER+0xC5);
    if(mounted) {psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);}
    if(seat)mmx4_coop_leave_second();
    warp_pending=0;warp_phase=1;warp_unlocked_ticks=0;
    if(!mounted)start_outgoing(cpu,seat);
    psx_mod_counter_add("mmx4.coop.script-departures",1);
}
static void position_return(CPUState *cpu,unsigned seat,uint32_t x,uint32_t y,
                            uint8_t active,uint8_t visible,uint8_t va,uint8_t vv,uint32_t surface_box) {
    if(seat)mmx4_coop_enter_second();
    uint32_t vehicle_dx=psx_mod_read_word(MMX4_VEHICLE+8)-psx_mod_read_word(PLAYER+8);
    uint32_t vehicle_dy=psx_mod_read_word(MMX4_VEHICLE+12)-psx_mod_read_word(PLAYER+12);
    psx_mod_write_byte(PLAYER,active);psx_mod_write_byte(PLAYER+3,visible);
    psx_mod_write_byte(PLAYER+4,1);
    if(surface_box && !psx_mod_read_byte(PLAYER+0xC5)) {
        mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
        uint32_t box=psx_mod_read_word(PLAYER+0x68);
        if(box) {
            int owner_bottom=(int8_t)psx_mod_read_byte(surface_box+1)+psx_mod_read_byte(surface_box+3);
            int partner_bottom=(int8_t)psx_mod_read_byte(box+1)+psx_mod_read_byte(box+3);
            y+=(uint32_t)((owner_bottom-partner_bottom)*65536);
        }
    }
    psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
    for(unsigned at=0x20;at<0x30;at+=4)psx_mod_write_word(PLAYER+at,0);
    for(unsigned at=0x7C;at<=0x80;at+=2)psx_mod_write_half(PLAYER+at,0);
    psx_mod_write_byte(PLAYER+0x67,0);psx_mod_write_byte(PLAYER+0xC0,0);
    psx_mod_write_byte(PLAYER+0xC4,0);psx_mod_write_byte(PLAYER+0x61,60);
    mmx4_coop_clear_current_attacks();
    /* A world armor left behind by an on-foot passenger must not be erased
     * or relocated when that hero returns. */
    unsigned world_armor=psx_mod_read_byte(MMX4_VEHICLE) &&
        psx_mod_read_byte(MMX4_VEHICLE+1)==1 && !psx_mod_read_byte(PLAYER+0xC5);
    if(va || vv || !world_armor) {
        psx_mod_write_byte(MMX4_VEHICLE,va);psx_mod_write_byte(MMX4_VEHICLE+3,vv);
    }
    if(va && psx_mod_read_byte(PLAYER+0xC5)) {
        psx_mod_write_word(MMX4_VEHICLE+8,x+vehicle_dx);
        psx_mod_write_word(MMX4_VEHICLE+12,y+vehicle_dy);
        psx_mod_write_word(MMX4_VEHICLE+0x18,x+vehicle_dx);
        psx_mod_write_word(MMX4_VEHICLE+0x1C,y+vehicle_dy);
    }
    warp_guard=1;mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
    mmx4_coop_call(cpu,0x8002C614u,PLAYER,0);warp_guard=0;
    if(seat)mmx4_coop_spawn_facing_right();
    if(seat)mmx4_coop_leave_second();
}
static int safe_return_position(CPUState *cpu,unsigned seat) {
    if(seat)mmx4_coop_enter_second();
    if(psx_mod_read_byte(PLAYER+0xC5)) {
        if(seat)mmx4_coop_leave_second();
        return 1;
    }
    /* Matching the leader's feet does not guarantee that the other hero's
     * wider standing box fits. Probe native terrain before exposing a body
     * to the incoming animation; prefer the same height and a small offset. */
    uint8_t body[0xE4];read_bytes(PLAYER,body,sizeof body);
    static const int offsets[]={0,8,-8,16,-16,24,-24,32,-32};
    int safe=0;
    unsigned guard=warp_guard;warp_guard=1;
    for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];++i) {
        write_bytes(PLAYER,body,sizeof body);
        uint32_t x=body_word(body,8)+(uint32_t)(offsets[i]*65536);
        int center=(int16_t)(x>>16);
        if(i && (center<(int16_t)psx_mod_read_half(0x801419D6u)+16 ||
           center>(int16_t)psx_mod_read_half(0x801419D4u)+304))continue;
        psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+0x18,x);
        psx_mod_write_byte(PLAYER+0x70,0);psx_mod_write_byte(PLAYER+0x71,0);
        psx_mod_write_byte(PLAYER+0x79,0);psx_mod_write_byte(PLAYER+0x89,0);
        mmx4_coop_call(cpu,0x8002C614u,PLAYER,0);
        unsigned contact=psx_mod_read_byte(PLAYER+0x70)|psx_mod_read_byte(PLAYER+0x71);
#ifndef PSX_NO_DEBUG_TOOLS
        psx_mod_counter_add("mmx4.coop.warp-return-terrain-probes",1);
        if((contact&12u)==12u)psx_mod_counter_add("mmx4.coop.warp-return-vertical-crush",1);
        if((contact&3u)==3u)psx_mod_counter_add("mmx4.coop.warp-return-horizontal-crush",1);
        if(psx_mod_read_byte(PLAYER+0x79))psx_mod_counter_add("mmx4.coop.warp-return-hazard",1);
        if(psx_mod_read_byte(PLAYER+0x5C)!=body[0x5C])psx_mod_counter_add("mmx4.coop.warp-return-health-change",1);
#endif
        if((contact&12u)!=12u && (contact&3u)!=3u &&
           !psx_mod_read_byte(PLAYER+0x79) && psx_mod_read_byte(PLAYER+0x5C)==body[0x5C] &&
           (!i || (contact&8u))) {safe=1;break;}
    }
    warp_guard=guard;
    if(!safe)write_bytes(PLAYER,body,sizeof body);
    if(seat)mmx4_coop_leave_second();
    return safe;
}
static void recover_warp_passenger(CPUState *cpu,unsigned seat) {
    if(seat_was_dead(seat)) {warp_phase=0;return;}
    position_return(cpu,seat,warp_origin_x,warp_origin_y,
        warp_active,warp_visible,warp_vehicle_active,warp_vehicle_visible,0);
    if(!safe_return_position(cpu,seat)) {
        /* Terrain may have changed since departure. Keep the living body
         * sheltered and retry its own origin rather than exposing it to a
         * crush or choosing the dead partner's last position. */
        if(seat)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
        if(seat)mmx4_coop_leave_second();
        warp_phase=1;return;
    }
    if(seat)mmx4_coop_enter_second();
    /* Position/visibility alone leave the outgoing beam's action0/1 active.
     * Initialize the native controllable pose before releasing the warp. */
    psx_mod_write_byte(PLAYER+0xBC,0);psx_mod_write_byte(PLAYER+0xBD,0);
    mmx4_coop_call(cpu,0x800343A4u,PLAYER,0);
    if(seat)mmx4_coop_leave_second();
    warp_phase=0;warp_unlocked_ticks=0;
    psx_mod_counter_add("mmx4.coop.warp-death-recoveries",1);
    fprintf(stderr,"mmx4.coop: warp-death-recovery seat=%u stage=%u section=%u x=%d y=%d\n",
        seat+1,psx_mod_read_byte(PLAY+12),psx_mod_read_byte(PLAY+13),
        (int16_t)(seat?(body_word(mmx4_coop_second_body(),8)>>16):psx_mod_read_half(PLAYER+10)),
        (int16_t)(seat?(body_word(mmx4_coop_second_body(),12)>>16):psx_mod_read_half(PLAYER+14)));
}
static int armor_return_anchor(unsigned owner,uint32_t *x,uint32_t *y,uint32_t *box) {
    uint8_t canonical[0xB0];
    const uint8_t *vehicle;
    if(owner)vehicle=mmx4_coop_second_vehicle();
    else {read_bytes(MMX4_VEHICLE,canonical,sizeof canonical);vehicle=canonical;}
    /* A mounted body has action18, personal phase1 and no foot contact box.
     * Its occupied, grounded native Armor supplies the actual floor instead.
     * Chasers, airborne/ejecting Armor and abandoned vehicles are ineligible. */
    unsigned mounted=owner?mmx4_coop_second_body()[0xC5]:psx_mod_read_byte(PLAYER+0xC5);
    uint32_t surface=body_word(vehicle,0x68);
    if(mounted!=255 || !vehicle[0] || vehicle[1]!=1 || vehicle[4]!=1 ||
       vehicle[5]>=11 || !(vehicle[0x97]&0x40u) || !(vehicle[0xA8]&8u) ||
       surface<0x80010000u || surface>0x801FFFFCu)return 0;
    *x=body_word(vehicle,8);*y=body_word(vehicle,12);*box=surface;
    return 1;
}
static void warp_tick(CPUState *cpu) {
    warp_begin(cpu);
    if(!warp_phase || mmx4_coop_projected())return;
    /* A room fade/section transfer or victory departure is not gameplay
     * resuming. Keep passengers outside until the native transition ends. */
    if(psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+1)!=0)return;
    unsigned seat=warp_owner^1u;
    const uint8_t *second=mmx4_coop_second_body();
    uint32_t x=warp_owner?body_word(second,8):psx_mod_read_word(PLAYER+8);
    uint32_t y=warp_owner?body_word(second,12):psx_mod_read_word(PLAYER+12);
    unsigned state=warp_owner?second[4]:psx_mod_read_byte(PLAYER+4);
    unsigned action=warp_owner?second[5]:psx_mod_read_byte(PLAYER+5);
    unsigned grounded=warp_owner?second[0x89]:psx_mod_read_byte(PLAYER+0x89);
    unsigned personal=warp_owner?second[0x67]:psx_mod_read_byte(PLAYER+0x67);
    unsigned locked=script_active || psx_mod_read_byte(PLAY+0x10) ||
        (psx_mod_read_byte(PLAY+0x1C) && !mmx4_coop_lifecycle_nova_active()) ||
        (warp_owner?(second[0xC0]|second[0xC3]|second[0xC4]):
            (psx_mod_read_byte(PLAYER+0xC0)|psx_mod_read_byte(PLAYER+0xC3)|
             psx_mod_read_byte(PLAYER+0xC4)));
    if(warp_phase==3) {
        /* Victory marks the owner state3 too. Finish the passenger's
         * outgoing beam so the canonical body can release stage termination,
         * without treating the winner's departure as a death to rescue. */
        if(!departed && !mmx4_coop_alive(warp_owner) && !psx_mod_read_byte(PLAY+0x0F)) {
            recover_warp_passenger(cpu,seat);return;
        }
        if(seat)mmx4_coop_enter_second();
        mmx4_coop_call(cpu,0x80031540u,PLAYER,0);
        if(psx_mod_read_byte(PLAYER+4)==3) {
            psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
            warp_phase=departed?0u:1u;
        }
        if(seat)mmx4_coop_leave_second();
        return;
    }
    if(psx_mod_read_byte(PLAY+0x0F)) {
        /* A new real transport can overtake an earlier incoming beam.
         * Hiding that pose leaves canonical P1 state1 forever, whereas the
         * native checkpoint loader waits for state3. Complete an outgoing
         * beam for the existing passenger without resampling its saved HP,
         * visibility or return origin. Already-departed/dead bodies stay so. */
        unsigned passenger_state=seat?second[4]:psx_mod_read_byte(PLAYER+4);
        unsigned passenger_hp=(seat?second[0x5C]:psx_mod_read_byte(PLAYER+0x5C))&127u;
        if(script_active && script_owner==warp_owner && passenger_state==1 && passenger_hp)
            start_outgoing(cpu,seat);
        return;
    }
    if(warp_phase==2 && !mmx4_coop_alive(warp_owner)) {
        /* The incoming player is alive even if their partner dies before
         * the return pose finishes; release control before team-wipe checks. */
        recover_warp_passenger(cpu,seat);return;
    }
    if(warp_phase==1) {
        if(!mmx4_coop_alive(warp_owner)) {
            recover_warp_passenger(cpu,seat);return;
        }
        /* Native walking/dashing has a different action from idle. A living
         * grounded leader already supplies a safe landing point; requiring
         * idle restarted the wait whenever that player kept moving. */
        uint32_t surface_box=warp_owner?body_word(second,0x68):psx_mod_read_word(PLAYER+0x68);
        int armor=action==18 && armor_return_anchor(warp_owner,&x,&y,&surface_box);
        if(locked || state!=1 || (!armor &&
           (personal || action<2 || action>=0x10 || !(grounded&8u)))) {warp_unlocked_ticks=0;return;}
        if(++warp_unlocked_ticks<3)return;
        position_return(cpu,seat,x,y,warp_active,warp_visible,
            warp_vehicle_active,warp_vehicle_visible,surface_box);
        if(!safe_return_position(cpu,seat)) {
            if(seat)mmx4_coop_enter_second();
            psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
            if(seat)mmx4_coop_leave_second();
            warp_unlocked_ticks=0;return;
        }
        /* Once the new grounded arrival is committed, a leader's death
         * must not send the survivor back outside the previous room. */
        warp_origin_x=seat?body_word(second,8):psx_mod_read_word(PLAYER+8);
        warp_origin_y=seat?body_word(second,12):psx_mod_read_word(PLAYER+12);
        if(seat)mmx4_coop_enter_second();
        if(psx_mod_read_byte(PLAYER+0xC5))warp_phase=0;
        else {
            if(seat)mmx4_coop_leave_second();
            start_arrival(cpu,seat);
            if(seat)mmx4_coop_enter_second();
            /* This return was aligned to the leader's validated floor.
             * Retain that contact for 31410's first incoming tick: clearing
             * it makes X descend another eight pixels through a thin floor
             * before the hidden passenger can participate in solid contact.
             * Initial airborne stage arrivals keep their native descent. */
            psx_mod_write_byte(PLAYER+0x70,8);
            psx_mod_write_byte(PLAYER+0x89,8);
        }
        if(seat)mmx4_coop_leave_second();
    }else if(locked) {
        /* A boss conversation can acquire its lock after the door releases.
         * Keep the passenger outside through the complete chained event.
         * A leader's +67 jump/fall phase and Nova's PLAY+1C are personal
         * actions: neither invalidates the already committed landing point. */
        if(seat)mmx4_coop_enter_second();
        psx_mod_write_byte(PLAYER,0);psx_mod_write_byte(PLAYER+3,0);
        psx_mod_write_byte(MMX4_VEHICLE,0);psx_mod_write_byte(MMX4_VEHICLE+3,0);
        if(seat)mmx4_coop_leave_second();
        warp_phase=1;warp_unlocked_ticks=0;
    }else {
        if(seat)mmx4_coop_enter_second();
        uint8_t lock=psx_mod_read_byte(PLAY+0x1C);
        uint8_t cameras[3]={psx_mod_read_byte(0x801419F4u),
            psx_mod_read_byte(0x80141A48u),psx_mod_read_byte(0x80141A9Cu)};
        /* Native 800312F8 refreshes this summary before action dispatch.
         * The isolated arrival bypasses that control pass, so consume the
         * preceding native terrain result before checking for touchdown. */
        psx_mod_write_byte(PLAYER+0x89,(uint8_t)(psx_mod_read_byte(PLAYER+0x70)|
            psx_mod_read_byte(PLAYER+0x71)));
        mmx4_coop_call(cpu,0x80031410u,PLAYER,0);
        warp_guard=1;mmx4_coop_call(cpu,0x8002C614u,PLAYER,0);warp_guard=0;
        psx_mod_write_byte(PLAY+0x1C,lock);
        psx_mod_write_byte(0x801419F4u,cameras[0]);
        psx_mod_write_byte(0x80141A48u,cameras[1]);
        psx_mod_write_byte(0x80141A9Cu,cameras[2]);
        if(psx_mod_read_byte(PLAYER+4)==1 && psx_mod_read_byte(PLAYER+5)==2) {
            warp_phase=0;
            psx_mod_counter_add("mmx4.coop.script-rejoins",1);
        }
        if(seat)mmx4_coop_leave_second();
    }
}
static void rescue_dead_scene_owner(CPUState *cpu) {
    if(!script_active || warp_pending || warp_phase || psx_mod_read_byte(PLAY+0x0F) ||
       !seat_was_dead(script_owner) || !mmx4_coop_alive(script_owner^1u))return;
    unsigned dead=script_owner,survivor=dead^1u;
    if(dead)mmx4_coop_enter_second();
    uint8_t command=psx_mod_read_byte(PLAYER+0xC0);
    uint8_t action=psx_mod_read_byte(PLAYER+0xC1),argument=psx_mod_read_byte(PLAYER+0xC2);
    uint8_t door=psx_mod_read_byte(PLAYER+0xC4),hold=psx_mod_read_byte(PLAYER+0xC3);
    unsigned paired=door && both_door_scope() && both_door.phase<=3 &&
        both_door.serial==script_serial;
    uint32_t scene_x=psx_mod_read_word(PLAYER+8),scene_y=psx_mod_read_word(PLAYER+12);
    psx_mod_write_byte(PLAYER+0xC0,0);psx_mod_write_byte(PLAYER+0xC3,0);
    psx_mod_write_byte(PLAYER+0xC4,0);psx_mod_write_byte(PLAYER+0x67,0);
    /* Door action19 otherwise keeps a fatal body in state1 forever. Let
     * the original death dispatcher emit its one native sequence. */
    if(psx_mod_read_byte(PLAYER+4)==1) {
        mmx4_coop_clear_current_attacks();
        psx_mod_write_byte(PLAYER+4,2);psx_mod_write_byte(PLAYER+5,0);
        psx_mod_write_byte(PLAYER+6,0);
    }
    if(dead)mmx4_coop_leave_second();
    script_owner=survivor;
    if(door && !paired) {
        /* The camera is already moving through the doorway. Continuing
         * from the passenger's old position lets it overtake/crush them. */
        const uint8_t *body=mmx4_coop_second_body();
        const uint8_t *vehicle=survivor?mmx4_coop_second_vehicle():mmx4_coop_first_vehicle();
        unsigned active=survivor?body[0]:psx_mod_read_byte(PLAYER);
        unsigned visible=survivor?body[3]:psx_mod_read_byte(PLAYER+3);
        position_return(cpu,survivor,scene_x,scene_y,(uint8_t)active,(uint8_t)visible,
            vehicle[0],vehicle[3],0);
    }
    if(survivor)mmx4_coop_enter_second();
    if(command)mmx4_coop_call(cpu,0x80036AE4u,action,argument);
    if(door) {
        /* Native 800338CC consumes C4=1 to initialize action19, then
         * marks it FF. The survivor must initialize its own pose. */
        psx_mod_write_byte(PLAYER+0xC4,(uint8_t)(paired && both_door.phase>=2?255u:1u));
        psx_mod_write_byte(PLAYER+0xC3,hold);
    }
    if(survivor)mmx4_coop_leave_second();
    for(unsigned i=0;i<sizeof scenes/sizeof scenes[0];++i)
        if(scenes[i].actor && scenes[i].owner==dead) {
            scenes[i].owner=(uint8_t)survivor;scenes[i].serial=script_serial;
            scenes[i].recovered=(uint8_t)(door!=0 && !paired);
        }
    if(paired)both_door.owner=(uint8_t)survivor;
    psx_mod_counter_add("mmx4.coop.dead-scene-owner-handoffs",1);
}
void mmx4_coop_lifecycle_tick(CPUState *cpu) {
    if(!mmx4_coop_ready() || mmx4_coop_projected())return;
    const uint8_t *second=mmx4_coop_second_body();
    unsigned stage=psx_mod_read_byte(PLAY+0x0C),section=psx_mod_read_byte(PLAY+0x0D);
    if(!mmx4_coop_alive(1))memset(surface_valid,0,sizeof surface_valid);
    unsigned hp[2]={psx_mod_read_byte(PLAYER+0x5C)&127u,second[0x5C]&127u};
    unsigned hidden=mmx4_coop_lifecycle_hidden(1);
    if(!psx_mod_local_view_scope() && trace_previous.valid &&
       (second[0]!=trace_previous.second_active || hidden!=trace_previous.second_hidden)) {
        fprintf(stderr,"mmx4.coop: partner-presence stage=%u section=%u checkpoint=%u "
            "mode=%u/%u active=%u->%u hidden=%u->%u visible=%u state=%u/%u hp=%u "
            "x=%d y=%d warp=%u pending=%u owner=%u withdrawn=%u script=%u/%u\n",
            stage,section,psx_mod_read_byte(PLAY+0x1D),psx_mod_read_byte(PLAY),psx_mod_read_byte(PLAY+1),
            trace_previous.second_active,second[0],trace_previous.second_hidden,hidden,
            second[3],second[4],second[5],hp[1],
            (int16_t)(body_word(second,8)>>16),(int16_t)(body_word(second,12)>>16),
            warp_phase,warp_pending,warp_owner,departed,script_active,script_owner);
    }
    if(trace_previous.valid)for(unsigned seat=0;seat<2;++seat) {
        if(hp[seat]!=trace_previous.hp[seat])
            fprintf(stderr,"mmx4.coop: health seat=%u stage=%u section=%u hp=%u->%u "
                "state=%u x=%d y=%d warp=%u owner=%u\n",seat+1,stage,section,
                trace_previous.hp[seat],hp[seat],seat?second[4]:psx_mod_read_byte(PLAYER+4),
                (int16_t)(seat?(body_word(second,8)>>16):psx_mod_read_half(PLAYER+10)),
                (int16_t)(seat?(body_word(second,12)>>16):psx_mod_read_half(PLAYER+14)),
                warp_phase,warp_owner);
    }
    trace_previous.valid=1;trace_previous.stage=stage;trace_previous.section=section;
    memcpy(trace_previous.hp,hp,sizeof hp);
    trace_previous.second_active=second[0];trace_previous.second_hidden=hidden;
    both_door_tick(cpu);
    warp_tick(cpu);
    rescue_dead_scene_owner(cpu);
    if(warp_pending || warp_phase) {
        if(departed) {
            uint16_t input=mmx4_coop_input(1);
            if(!(input&SELECT))select_release=0;
            else if(!select_release && !(select_previous&SELECT))rejoin_pending=1;
            select_previous=input;
        }
        return;
    }
    uint8_t *body=mmx4_coop_second_body();
    body[0xB9]|=psx_mod_read_byte(PLAY+0x59);
    stage=psx_mod_read_byte(PLAY+0x0C);
    if(stage_known && stage!=last_stage) {
        last_stage=stage;last_section=psx_mod_read_byte(PLAY+0x0D);
    }
    last_lives=psx_mod_read_byte(PLAY+0x44);
    uint16_t input=mmx4_coop_input(1);
    /* Native 8001FDD8..8001FDF8 selects Ride Chasers for Marine Base's
     * checkpoint zero. Keep the gate through transient mounting changes. */
    unsigned bike_sequence=stage==5 && (!psx_mod_read_byte(PLAY+0x1D) ||
        psx_mod_read_byte(PLAYER+0xC5)==0xFF || body[0xC5]==0xFF);
    if(departed) {
        if(!(input&SELECT))select_release=0;
        else if(!select_release && !(select_previous&SELECT))rejoin_pending=1;
        /* Wait for an actual living grounded native body. Joining at exactly
         * the same floor coordinate avoids guessing about pits to either
         * side; independent movement separates the bodies afterward. */
        if(rejoin_pending && mmx4_coop_alive(0) &&
           psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+1) &&
           !psx_mod_read_byte(PLAY+0x0F) && !psx_mod_read_byte(PLAY+0x10) &&
           !psx_mod_read_byte(PLAY+0x1C) && psx_mod_read_byte(PLAYER+5)>=2 &&
           psx_mod_read_byte(PLAYER+5)<0x10 &&
           !psx_mod_read_byte(PLAYER+0xC0) && !psx_mod_read_byte(PLAYER+0xC3) &&
           !psx_mod_read_byte(PLAYER+0xC4) && !psx_mod_read_byte(PLAYER+0x67) &&
           (psx_mod_read_byte(PLAYER+0x89)&8u) &&
           !psx_mod_read_byte(PLAYER+0x79) && !psx_mod_read_byte(PLAYER+0xC5)) {
            uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
            position_return(cpu,1,x,y,departure_active,departure_visible,
                departure_vehicle_active,departure_vehicle_visible,psx_mod_read_word(PLAYER+0x68));
            mmx4_coop_enter_second();
            if(!body[0xC5] || !departure_vehicle_active) {
                warp_owner=0;warp_phase=2;
                warp_active=departure_active;warp_visible=departure_visible;
                warp_vehicle_active=departure_vehicle_active;
                warp_vehicle_visible=departure_vehicle_visible;
                warp_origin_x=psx_mod_read_word(PLAYER+8);
                warp_origin_y=psx_mod_read_word(PLAYER+12);warp_unlocked_ticks=0;
                mmx4_coop_leave_second();start_arrival(cpu,1);mmx4_coop_enter_second();
            }
            mmx4_coop_leave_second();
            departed=0;select_ticks=0;select_release=1;rejoin_pending=0;
            psx_mod_counter_add("mmx4.coop.rejoins",1);
        }
    }else if((input&SELECT) && !select_release && mmx4_coop_alive(0) &&
             mmx4_coop_alive(1) && !bike_sequence && !body[0xC5] &&
             !body[0xC0] && !body[0xC3] && !body[0xC4] && !body[0x67] &&
             !body[0xBC] && !initial_arrival && !script_active && !psx_mod_read_byte(PLAY+1) &&
             !psx_mod_read_byte(PLAY+0x0F) && !psx_mod_read_byte(PLAY+0x10) &&
             !psx_mod_read_byte(PLAY+0x1C)) {
        if(++select_ticks>=90) {
            departure_active=body[0];departure_visible=body[3];
            uint8_t *vehicle=mmx4_coop_second_vehicle();
            departure_vehicle_active=vehicle[0];departure_vehicle_visible=vehicle[3];
            vehicle[0]=vehicle[3]=0;
            mmx4_coop_enter_second();mmx4_coop_clear_current_attacks();mmx4_coop_leave_second();
            departed=1;select_release=1;select_ticks=0;
            warp_owner=0;start_outgoing(cpu,1);
            psx_mod_counter_add("mmx4.coop.departures",1);
        }
    }else {select_ticks=0;if(!(input&SELECT))select_release=0;}
    select_previous=input;
}

/* Wrapper guards permit exactly one native call and preserve CPU registers
 * through the core's call service. Only the outer filter completes the entry. */
static void personal_menu_read(PersonalMenu *menu) {
    read_bytes(MENU,menu->native,sizeof menu->native);
    menu->original_weapon=psx_mod_read_byte(0x8013B818u);
}
static void personal_menu_write(const PersonalMenu *menu) {
    write_bytes(MENU,menu->native,sizeof menu->native);
    psx_mod_write_byte(0x8013B818u,menu->original_weapon);
}
static int personal_busy(unsigned seat) {
    const uint8_t *menu=personal_menu[seat].native;
    /* +28 also stores the old substate in Quit: it is not a busy flag. */
    return menu[4]==1 && (menu[5]==3 || menu[5]==4);
}
static int personal_tank_request(unsigned seat,uint16_t edge) {
    const uint8_t *menu=personal_menu[seat].native;
    return (personal_eligible&(1u<<seat)) && menu[4]==1 && menu[5]==1 &&
        menu[0x14]>=10 && menu[0x14]<=12 && (edge&~START)==0x40;
}
/* The secondary menu must not start a second fade/music transition. */
static int personal_shared_effect(CPUState *cpu,uint32_t address) {
    (void)address;
    return personal_pass && personal_seat==1?mmx4_coop_finish(cpu,0):0;
}
static int personal_menu_draw(CPUState *cpu,uint32_t address) {
    (void)address;
    int modal=!personal_seat && psx_mod_read_byte(MENU+4)==1 &&
        psx_mod_read_byte(MENU+5)>=5;
    return personal_pass && !personal_rendering && !modal?mmx4_coop_finish(cpu,0):0;
}
static int personal_dead_commit(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!personal_pass || (personal_eligible&(1u<<personal_seat)))return 0;
    /* Escape's native branch skips equipment installation itself. */
    if(psx_mod_read_word(MENU+0x30))return 0;
    /* Retail weapon installation rewrites animation state. Never apply it
     * to a corpse or a withdrawn partner just to finish the shared fade. */
    if(!personal_seat) {
        mmx4_coop_call(cpu,0x80021158u,0,0);
        mmx4_coop_call(cpu,0x800129A4u,8,0);
    }
    psx_mod_write_byte(MENU+5,3);
    return mmx4_coop_finish(cpu,0);
}
static uint32_t personal_step(CPUState *cpu,uint32_t address,unsigned seat,
                              uint16_t input,uint16_t old) {
    personal_menu_write(&personal_menu[seat]);
    uint16_t edge=(uint16_t)(input&~old);
    /* P2's Select is routed to the shared campaign Quit frontend below.
     * Its personal menu must not start a second Quit or an Escape. */
    if(seat) {
        input&=(uint16_t)~SELECT;old&=(uint16_t)~SELECT;edge&=(uint16_t)~SELECT;
        psx_mod_write_byte(MENU+0x23,personal_menu[0].native[0x23]);
        psx_mod_write_byte(MENU+0x24,0);
        if(personal_menu[seat].native[0x14]>=13)edge&=(uint16_t)~0x40u;
    }
    psx_mod_write_half(PAD,input);psx_mod_write_half(PAD+2,old);psx_mod_write_half(PAD+4,edge);
    /* Native exit substate2 restores gameplay palettes and commits equipment.
     * Reinstalling a portrait afterward corrupts the first world frames. */
    if(personal_menu[seat].native[4]!=2 || personal_menu[seat].native[5]<2)
        mmx4_coop_menu_assets(cpu,seat);
    if(seat) {mmx4_coop_enter_second();menu_projecting=1;}
    personal_pass=menu_guard=1;personal_seat=seat;
    uint32_t result=mmx4_coop_call(cpu,address,MENU,0);
    personal_pass=menu_guard=0;
    if(seat) {
        uint8_t refresh=psx_mod_read_byte(0x80166BB0u)&1u;
        menu_projecting=0;mmx4_coop_leave_second();
        psx_mod_write_byte(0x80166BB0u,(uint8_t)(psx_mod_read_byte(0x80166BB0u)|refresh));
    }
    personal_menu_read(&personal_menu[seat]);
    return result;
}
static uint32_t personal_tick(CPUState *cpu,uint32_t address) {
    uint8_t pad[6];read_bytes(PAD,pad,sizeof pad);
    uint16_t input[2]={mmx4_coop_input(0),mmx4_coop_input(1)};
    uint16_t old[2]={menu_previous[0],menu_previous[1]};
    uint16_t edge[2]={(uint16_t)(input[0]&~old[0]),(uint16_t)(input[1]&~old[1])};
    memcpy(menu_previous,input,sizeof input);
    uint32_t result=0;
    if(!personal_initialized) {
        result=personal_step(cpu,address,0,0,0);
        /* Native initialization has populated the first menu. Initialize the
         * other character once at the same fade boundary, in its own context. */
        if(personal_menu[0].native[4]==0 && personal_menu[0].native[5]==2) {
            memset(&personal_menu[1],0,sizeof personal_menu[1]);
            personal_menu[1].native[5]=1;
            uint8_t fading=psx_mod_read_byte(0x80141BDCu);
            psx_mod_write_byte(0x80141BDCu,0);
            personal_step(cpu,0x8002FD70u,1,0,0);
            psx_mod_write_byte(0x80141BDCu,fading);
            personal_initialized=1;
        }
    }else {
        int busy=personal_busy(0) || personal_busy(1);
        int modal=personal_menu[0].native[4]==1 && personal_menu[0].native[5]>=5;
        int tank[2]={personal_tank_request(0,edge[0]),personal_tank_request(1,edge[1])};
        int interactive=personal_menu[0].native[4]==1 && personal_menu[1].native[4]==1;
        int second_quit=interactive && !modal && (personal_eligible&2u) &&
            (edge[1]&SELECT) && !((personal_eligible&1u) && (edge[0]&SELECT));
        int second_escape=interactive && !modal && !busy && !tank[0] && !tank[1] &&
            !second_quit && (personal_eligible&2u) && edge[1]==0x40 &&
            personal_menu[1].native[5]==2 && personal_menu[1].native[0x14]==13 &&
            personal_menu[0].native[0x23] && !((personal_eligible&1u) && (edge[0]&SELECT));
        int campaign_request=second_quit || second_escape;
        if(!modal)personal_modal_owner=campaign_request?1u:0u;
        if(second_escape) {
            /* Escape leaves the campaign menu through its original shared
             * fade, once. Keep that transition out of P2's personal pass. */
            personal_menu[0].native[5]=2;personal_menu[0].native[0x14]=13;
        }
        unsigned resume=0;
        for(unsigned seat=0;seat<2;++seat)
            resume|=(personal_eligible&(1u<<seat)) && (edge[seat]&START);
        /* A tank confirmation wins over a simultaneous resume request.
         * Blocked exit/tank edges are consumed, never queued after healing. */
        if(interactive && !busy && !modal && !campaign_request && !tank[0] && !tank[1] && resume) {
            for(unsigned seat=0;seat<2;++seat) {
                personal_menu[seat].native[4]=2;personal_menu[seat].native[5]=0;
            }
        }
        /* Tick an existing P2 heal before P1's shared Options/Quit frontend.
         * Healing must continue under that frontend, whose assets draw last. */
        unsigned first=personal_busy(1) || (tank[1] && !tank[0])?1:0;
        for(unsigned turn=0;turn<2;++turn) {
            unsigned seat=first^turn;
            if(modal && seat && !personal_busy(seat))continue;
            unsigned source=!seat && (modal || campaign_request)?personal_modal_owner:seat;
            int allowed=interactive && (!modal || !seat) &&
                (personal_eligible&(1u<<source));
            /* Native Options uses Start to return to the character menu.
             * Let P1 close that modal without interpreting it as team resume. */
            uint16_t mask=modal && !seat?0:START;
            if(busy) {
                const uint8_t *menu=personal_menu[seat].native;
                /* Equipment and cursor input stay live. Serialize new tank
                 * uses (W energy is shared), and cover native Escape/Quit
                 * confirmations that can leave the menu without Start.
                 * Options, including its Start-to-return, remains usable. */
                if(tank[seat] || (!seat && (menu[5]==6 ||
                   (menu[5]==2 && menu[0x14]==13))))mask|=0x40;
            }
            uint16_t buttons=allowed?(uint16_t)(input[source]&~mask):0;
            uint16_t previous=allowed?(uint16_t)(old[source]&~mask):0;
            if(!seat && second_quit) {buttons=SELECT;previous=0;}
            if(!seat && second_escape) {buttons=0x40;previous=0;}
            result=personal_step(cpu,address,seat,buttons,previous);
            busy|=personal_busy(seat);
            modal|=personal_menu[0].native[4]==1 && personal_menu[0].native[5]>=5;
            if(psx_mod_read_byte(PLAY)!=6)break;
        }
    }
    personal_menu_write(&personal_menu[0]);write_bytes(PAD,pad,sizeof pad);
    if(personal_menu[0].native[4]!=1 || personal_menu[0].native[5]<5)
        personal_modal_owner=0;
    if(psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+1)!=2)
        menu_active=personal_active=0;
    return result;
}
int mmx4_coop_lifecycle_resuming_world(void) {
    /* Retail substate3 draws the restored world while its shared fade opens.
     * Keep gameplay paused, but let private cameras present that same world. */
    return personal_active && personal_initialized && menu_active &&
        personal_menu[0].native[4]==2 && personal_menu[0].native[5]>=3;
}
unsigned mmx4_coop_lifecycle_menu_fade(uint32_t buffer) {
    if(!personal_active || !personal_initialized || !menu_active)return 0;
    unsigned fading=psx_mod_read_byte(0x80141BDCu);
    /* Stay black while the native exit restores the stage palette/equipment,
     * including the boundary after its fade task has retired. */
    if(!fading && personal_menu[0].native[4]==2 &&
       personal_menu[0].native[5]<=2)return 255;
    if(!fading)return 0;
    /* 80012A3C has already built this frame's subtractive TILE in the shared
     * ordering table. Reuse its displayed shade, without ticking the task. */
    unsigned index=buffer==0x80166CB0u;
    return psx_mod_read_byte(0x80169D7Cu+index*16u);
}
int mmx4_coop_lifecycle_personal_menus(void) {
    /* Native campaign Options/Quit own a separate shared frontend, including
     * their confirmation pages. Present that frontend once until it returns. */
    int modal=personal_menu[0].native[4]==1 && personal_menu[0].native[5]>=5;
    return personal_active && personal_initialized && menu_active && !modal &&
        !mmx4_coop_lifecycle_resuming_world();
}
int mmx4_coop_lifecycle_draw_menu(CPUState *cpu,unsigned seat) {
    if(seat>1 || !mmx4_coop_lifecycle_personal_menus() || mmx4_coop_projected())return 0;
    personal_menu_write(&personal_menu[seat]);
    mmx4_coop_menu_assets(cpu,seat);
    if(seat)mmx4_coop_enter_second();
    personal_rendering=1;
    mmx4_coop_call(cpu,0x80023D90u,0,0);
    personal_rendering=0;
    if(seat)mmx4_coop_leave_second();
    return 1;
}
static int stage_normal(CPUState *cpu,uint32_t address) {
    if(normal_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    /* The native dispatcher has returned to ordinary gameplay. A retired
     * menu must not suppress the partner's controller on subsequent frames.
     * The menu's nested world draw/update still owns its live projection. */
    if(!psx_mod_read_byte(PLAY+1) && !menu_projecting && !world_guard)
        menu_active=menu_guard=personal_active=personal_initialized=0;
    warp_begin(cpu);
    if(warp_phase && !mmx4_coop_alive(warp_owner))warp_tick(cpu);
    uint16_t input[2]={mmx4_coop_input(0),mmx4_coop_input(1)};
    uint16_t edge[2]={(uint16_t)(input[0]&~menu_previous[0]),
                      (uint16_t)(input[1]&~menu_previous[1])};
    story_pad[0]=input[1];story_pad[1]=menu_previous[1];story_pad[2]=edge[1];
    memcpy(menu_previous,input,sizeof input);
    int owner=-1;
    if(mmx4_coop_alive(0) && (edge[0]&START))owner=0;
    else if(mmx4_coop_alive(1) && (edge[1]&START))owner=1;
    /* Paired doors keep a scene latch while both heroes walk through freely.
     * Only the actual opening and boss conversation need the pause lock. */
    int free_door=mmx4_coop_lifecycle_paired_door() && both_door.phase>=2;
    if((script_active && !free_door) || psx_mod_read_byte(PLAY+0x1C) || psx_mod_read_byte(PLAY+0x10) ||
       psx_mod_read_byte(PLAY+0x0F) || psx_mod_read_byte(0x80141BDCu))owner=-1;
    uint8_t pad[6];read_bytes(PAD,pad,6);
    uint16_t native_edge=psx_mod_read_half(PAD+4);
    /* Clear an ineligible/dead seat's Start. A P2 request uses the game's
     * existing pause branch, keeping the original transitions and fades. */
    native_edge=(uint16_t)(native_edge&~START);
    if(owner>=0)native_edge|=START;
    psx_mod_write_half(PAD+4,native_edge);
    normal_guard=1;
    uint32_t result;
    /* State3 also finishes a temporary passenger beam. Keep the living
     * leader's world running until that passenger returns; only a real
     * stage-owner departure should reach native stage termination. */
    if(psx_mod_read_byte(PLAYER+4)==3 &&
       (!(psx_mod_read_byte(PLAYER+0x5C)&127u) || mmx4_coop_lifecycle_hidden(0) ||
        (paired_victory && mmx4_coop_second_body()[4]!=3)) &&
       mmx4_coop_alive(1)) {
        if(owner>=0) {
            psx_mod_write_byte(PLAY+1,2);result=0;
            mmx4_coop_call(cpu,0x80023D68u,0,0);
        }else {
            mmx4_coop_call(cpu,0x8002B73Cu,0,0);
            mmx4_coop_call(cpu,0x80021158u,0,0);
            result=mmx4_coop_call(cpu,0x80023D68u,0,0);
        }
    }else result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    normal_guard=0;write_bytes(PAD,pad,6);
    if(psx_mod_read_byte(PLAY+1)==2 && owner>=0) {
        menu_active=1;menu_owner=(unsigned)owner;
        if(simultaneous_menus) {
            personal_active=1;personal_initialized=personal_modal_owner=0;
            personal_eligible=(unsigned)mmx4_coop_alive(0)|((unsigned)mmx4_coop_alive(1)<<1);
            memset(personal_menu,0,sizeof personal_menu);
            personal_menu_read(&personal_menu[0]);menu_owner=0;
        }
        psx_mod_counter_add(owner?"mmx4.coop.p2-menu":"mmx4.coop.p1-menu",1);
    }
    return mmx4_coop_finish(cpu,result);
}

static int pause_menu(CPUState *cpu,uint32_t address) {
    if(menu_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    if(personal_active)return mmx4_coop_finish(cpu,personal_tick(cpu,address));
    if(!menu_active) {menu_active=1;menu_owner=0;}
    uint8_t pad[6];read_bytes(PAD,pad,6);
    uint16_t input=mmx4_coop_input(menu_owner);
    uint16_t edge=(uint16_t)(input&~menu_previous[menu_owner]);
    uint16_t old=menu_previous[menu_owner];
    menu_previous[0]=mmx4_coop_input(0);menu_previous[1]=mmx4_coop_input(1);
    /* Either owner may open native 80030128's confirmed Quit with Select.
     * Campaign progress and save ownership still remain with P1. */
    psx_mod_write_half(PAD,input);psx_mod_write_half(PAD+2,old);
    psx_mod_write_half(PAD+4,edge);
    mmx4_coop_menu_assets(cpu,menu_owner);
    if(menu_owner) {mmx4_coop_enter_second();menu_projecting=1;}
    menu_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    menu_guard=0;
    if(menu_owner) {
        /* 800170E0 requests the shared gameplay palette after a menu fade.
         * Normal P2 projections discard private palette dirtiness; this
         * menu transition must publish its request to the canonical pass. */
        uint8_t refresh=psx_mod_read_byte(0x80166BB0u)&1u;
        menu_projecting=0;mmx4_coop_leave_second();
        psx_mod_write_byte(0x80166BB0u,
            (uint8_t)(psx_mod_read_byte(0x80166BB0u)|refresh));
    }
    write_bytes(PAD,pad,6);
    if(psx_mod_read_byte(PLAY+1)!=2 || psx_mod_read_byte(PLAY)!=6)menu_active=0;
    return mmx4_coop_finish(cpu,result);
}
static int menu_world(CPUState *cpu,uint32_t address) {
    if(personal_pass && personal_seat==1)return mmx4_coop_finish(cpu,0);
    if(world_guard || !menu_projecting || !mmx4_coop_projected())return 0;
    /* Both fade boundaries draw the world (80023D68), and exit-pause also
     * updates it once (80021158). Shared graphics/caches and actor pools must
     * use P1's context; resume P2 only for the personal menu/weapon work. */
    uint8_t refresh=psx_mod_read_byte(0x80166BB0u)&1u;
    mmx4_coop_leave_second();world_guard=1;
    psx_mod_write_byte(0x80166BB0u,
        (uint8_t)(psx_mod_read_byte(0x80166BB0u)|refresh));
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    world_guard=0;mmx4_coop_enter_second();
    return mmx4_coop_finish(cpu,result);
}

static int pickup_init_canonical(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];pickup_forget(actor);
    /* Native placement suppression reads the single campaign's flags. */
    return 0;
}
static int pickup_init(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,pickup_init_canonical);
}
static int personal_health_refill(uint32_t actor) {
    unsigned kind=psx_mod_read_byte(actor+0x7C);
    /* C00BC maps0/1 to small/large health and5 to full health. The full
     * capsule otherwise asserts C03BC's shared pause, which also forces
     * the distant peer's renderer back to the canonical camera. Preserve
     * the existing private large-weapon refill (kind3). */
    return mmx4_coop_split_views() && (kind==0 || kind==1 || kind==3 || kind==5);
}
static uint32_t pickup_native_call(CPUState *cpu,uint32_t address,uint32_t actor) {
    uint8_t freeze[8];
    int personal=personal_health_refill(actor);
    if(personal)read_bytes(PLAY+0x10,freeze,sizeof freeze);
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    /* Only health refills in Split release their own native world pause.
     * Keep any preexisting scene locks and native per-frame HP counting. */
    if(personal)write_bytes(PLAY+0x10,freeze,sizeof freeze);
    return result;
}
static int pickup_collect_canonical(CPUState *cpu,uint32_t address) {
    if(collect_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];
    uint8_t state=psx_mod_read_byte(actor+4);
    PickupOwner *record=personal_health_refill(actor)?pickup_record(actor,1):NULL;
    if(personal_health_refill(actor) && !record) {
        psx_mod_counter_add("mmx4.coop.pickup-owner-full",1);
        return mmx4_coop_finish(cpu,0);
    }
    collect_guard=1;
    uint32_t result=0;
    if(mmx4_coop_alive(0))
        result=pickup_native_call(cpu,address,actor);
    if(record && psx_mod_read_byte(actor+4)!=state)record->owner=0;
    if(psx_mod_read_byte(actor+4)==state && mmx4_coop_alive(1)) {
        /* Reserve ownership before making the second native call: a delayed
         * health pickup remains active and must heal this same body later. */
        record=pickup_record(actor,1);
        if(record) {
            mmx4_coop_enter_second();
            result=pickup_native_call(cpu,address,actor);
            mmx4_coop_leave_second();
            if(psx_mod_read_byte(actor+4)!=state) {
                record->owner=1;psx_mod_counter_add("mmx4.coop.p2-pickups",1);
            }else pickup_forget(actor);
        }else psx_mod_counter_add("mmx4.coop.pickup-owner-full",1);
    }
    if(record && psx_mod_read_byte(actor+4)!=2)pickup_forget(actor);
    collect_guard=0;return mmx4_coop_finish(cpu,result);
}
static int pickup_collect(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,pickup_collect_canonical);
}
static int pickup_update_canonical(CPUState *cpu,uint32_t address) {
    if(pickup_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];PickupOwner *record=pickup_record(actor,0);
    if(!record)return 0;
    unsigned state=psx_mod_read_byte(actor+4);
    if(state!=2) {pickup_forget(actor);return 0;}
    unsigned owner=record->owner;
    if(!mmx4_coop_alive(owner)) {
        /* A scripted death must never be undone by a pending healing actor. */
        psx_mod_write_byte(actor+4,3);pickup_forget(actor);
        uint8_t freeze[8];int personal=personal_health_refill(actor);
        if(personal)read_bytes(PLAY+0x10,freeze,sizeof freeze);
        mmx4_coop_call(cpu,0x800C03BCu,0,0);
        if(personal)write_bytes(PLAY+0x10,freeze,sizeof freeze);
        return mmx4_coop_finish(cpu,0);
    }
    if(owner)mmx4_coop_enter_second();
    pickup_guard=1;
    uint32_t result=pickup_native_call(cpu,address,actor);
    pickup_guard=0;
    if(owner)mmx4_coop_leave_second();
    if(psx_mod_read_byte(actor+4)!=2)pickup_forget(actor);
    return mmx4_coop_finish(cpu,result);
}
static int pickup_update(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,pickup_update_canonical);
}
static int reward_canonical(CPUState *cpu,uint32_t address) {
    if(reward_guard || !mmx4_coop_ready())return 0;
    unsigned projected=mmx4_coop_projected();
    if(projected)mmx4_coop_leave_second();
    /* A later passenger departure must not commit this scene again. Keep
     * original reward inputs even for Intro/final-story transitions. */
    if(reward_committed) {
        if(projected)mmx4_coop_enter_second();
        return mmx4_coop_finish(cpu,0);
    }
    reward_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    reward_committed=1;
    /* Continue-current-data uses this native RAM record, independently of
     * the memory card. Publish the newly committed boss/story bits now. */
    mmx4_coop_call(cpu,0x8001C3E8u,0,0);
    reward_guard=0;
    mmx4_coop_second_body()[0xB9]=psx_mod_read_byte(PLAY+0x59);
    if(projected)mmx4_coop_enter_second();
    return mmx4_coop_finish(cpu,result);
}
static int reward(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,reward_canonical);
}
static int campaign_save(CPUState *cpu,uint32_t address) {
    if(save_guard || !mmx4_coop_ready() || !mmx4_coop_projected())return 0;
    /* Both native record writers must observe the campaign owner, never
     * the temporary character selection used for P2 attacks or menus. */
    mmx4_coop_leave_second();save_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    save_guard=0;mmx4_coop_enter_second();
    return mmx4_coop_finish(cpu,result);
}
static int living_death_partner(void) {
    unsigned other=mmx4_coop_projected()?0u:1u;
    const uint8_t *body=other?mmx4_coop_second_body():mmx4_coop_first_body();
    /* Outgoing/incoming passengers still have a life to preserve, even
     * while alive() excludes them from attacks and native AI targeting. */
    return !(other && departed) && (body[0x5C]&127u) && body[4]!=2;
}
static int death_detection(CPUState *cpu,uint32_t address) {
    if(death_check_guard || !mmx4_coop_ready() || cpu->gpr[4]!=PLAYER ||
       !living_death_partner())return 0;
    uint8_t scene_lock=psx_mod_read_byte(PLAY+0x1C);
    death_check_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    death_check_guard=0;
    /* Native80033C7C acquires a single-player death lock before the death
     * animation can preserve it. Keep any prior real scene lock, but let
     * the surviving partner continue gameplay after a new fatal hit. */
    if(psx_mod_read_byte(PLAYER+4)==2)psx_mod_write_byte(PLAY+0x1C,scene_lock);
    return mmx4_coop_finish(cpu,result);
}
static int death_animation(CPUState *cpu,uint32_t address) {
    if(death_guard || !mmx4_coop_ready())return 0;
    if(!living_death_partner())return 0;
    uint8_t gates[9];read_bytes(PLAY+0x12,gates,9);
    uint8_t scene_lock=psx_mod_read_byte(PLAY+0x1C);
    unsigned initial=psx_mod_read_byte(PLAYER+5)==0;
    if(initial)mmx4_coop_armor_dismount(cpu,(unsigned)(mmx4_coop_projected()!=0));
    death_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    death_guard=0;write_bytes(PLAY+0x12,gates,9);
    if(initial && psx_mod_read_byte(PLAYER+5)!=0)
        psx_mod_counter_add(mmx4_coop_projected()?"mmx4.coop.p2-death-sequences":"mmx4.coop.p1-death-sequences",1);
    /* C4 door ownership is tracked here, but it is not a native C0 lock.
     * Preserve the real scene gate instead of turning every door into C0. */
    psx_mod_write_byte(PLAY+0x1C,scene_lock);
    return mmx4_coop_finish(cpu,result);
}

static int x_capsule_canonical(CPUState *cpu,uint32_t address) {
    if(capsule_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];
    /* Every capsule component reads the canonical X body, including child
     * upgrade animation 800C6EDC. In Zero's campaign all those reads belong
     * to X/P2. A dead X cannot begin or continue capsule acquisition. */
    unsigned x_seat=psx_mod_read_byte(PLAYER+2)==0?0u:1u;
    if(!mmx4_coop_alive(x_seat) &&
       !(address==0x800C62DCu && psx_mod_read_byte(actor+4)==0))
        return mmx4_coop_finish(cpu,0);
    if(!x_seat) {mmx4_cheats_enforce_current();return 0;}
    mmx4_coop_enter_second();capsule_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    capsule_guard=0;mmx4_coop_leave_second();
    return mmx4_coop_finish(cpu,result);
}

static int x_capsule(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,x_capsule_canonical);
}
int mmx4_coop_lifecycle_x_capsule_text(void) {
    if(!mmx4_coop_ready() || psx_mod_local_view_scope())return 0;
    unsigned campaign=mmx4_coop_projected()?mmx4_coop_first_body()[2]:
        psx_mod_read_byte(PLAYER+2);
    if(campaign!=1)return 0; /* P1 X already has the native X resources. */
    if(capsule_guard)return 1;
    uint32_t text=mmx4_coop_counterpart_story_text();
    uint32_t cursor=psx_mod_read_word(0x8013BC28u);
    if(!text || !psx_mod_read_byte(0x8013BC34u) || cursor<text || cursor-text>=0xA000u)
        return 0;
    /* Native C6814 opens the box, C6894/C68E0 wait for it in phases2/3.
     * Derive continuation from the native actor and bound script cursor;
     * no new saved flag can outlive the capsule or leak into story scenes. */
    for(unsigned slot=0;slot<32;++slot) {
        uint32_t actor=0x80165A30u+slot*0x8Cu;
        unsigned phase=psx_mod_read_byte(actor+5);
        if(psx_mod_read_byte(actor) && psx_mod_read_byte(actor+1)==26 &&
           psx_mod_read_byte(actor+4)==1 && phase>=2 && phase<=3)return 1;
    }
    return 0;
}
static SceneOwner *scene_record(uint32_t actor,int create) {
    SceneOwner *empty=NULL;
    for(unsigned i=0;i<sizeof scenes/sizeof scenes[0];++i) {
        if(scenes[i].actor==actor)return &scenes[i];
        if(!scenes[i].actor && !empty)empty=&scenes[i];
    }
    if(create && empty) {empty->actor=actor;empty->owner=0;return empty;}
    return NULL;
}
static uint32_t call_story_as_second(CPUState *cpu,uint32_t address,uint32_t actor,uint32_t arg) {
    uint8_t pad[6];read_bytes(PAD,pad,sizeof pad);
    uint8_t campaign=psx_mod_read_byte(PLAY+0x43);
    uint32_t stage_palette=psx_mod_read_word(0x1F800028u);
    mmx4_coop_enter_second();
    uint8_t character=psx_mod_read_byte(PLAY+0x43);
    psx_mod_write_byte(PLAY+0x43,campaign);
    /* The stage pool owns shared colors, even when P2 supplies its triggers.
     * Kind3 B5DCC caches palette destinations during initialization; late
     * controllers must bind the native stage palette, not P2's character
     * palette. Character/controller and other story projections stay private. */
    unsigned shared_stage=address==0x8002166Cu ||
        address==0x800D8648u || address==0x800D9218u;
    if(shared_stage)psx_mod_write_word(0x1F800028u,stage_palette);
    for(unsigned i=0;i<3;++i)psx_mod_write_half(PAD+i*2,story_pad[i]);
    uint32_t result=mmx4_coop_call(cpu,address,actor,arg);
    /* Native stage flash BD5CC clears the shared RGB additions and requests
     * palette bit4. 2A484 otherwise skips the zero-addition rebuild, leaving
     * the bright working palette resident after a P2 survivor projection.
     * Kind2/3 palette animation B5C60 also requests upload bit1. Dropping it
     * leaves the waterfall on its last uploaded frame after P1 dies, despite
     * advancing the animation clock and shared source colors. Retain these
     * shared stage requests; ordinary character projections stay private. */
    uint8_t shared_palette_refresh=shared_stage?
        (uint8_t)(psx_mod_read_byte(0x80166BB0u)&5u):0;
    /* Projected inventory reads on leave must see the counterpart identity,
     * even though shared story selection remained canonical throughout. */
    psx_mod_write_byte(PLAY+0x43,character);
    mmx4_coop_leave_second();
    if(shared_palette_refresh)psx_mod_write_byte(0x80166BB0u,
        (uint8_t)(psx_mod_read_byte(0x80166BB0u)|shared_palette_refresh));
    write_bytes(PAD,pad,sizeof pad);return result;
}
static int cyber_assessment(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(scene_guard || !mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12)!=6 ||
       psx_mod_read_byte(PLAY+13) || actor<0x80173CA0u || actor>=0x801754A0u ||
       (actor-0x80173CA0u)%0x60u || psx_mod_read_byte(actor+1)!=34)return 0;
    uint32_t parent=psx_mod_read_word(actor+0x50);
    if(parent<0x80142F98u || parent>=0x80143598u || (parent-0x80142F98u)%0x30u ||
       !psx_mod_read_byte(parent) || psx_mod_read_byte(parent+1)!=38)return 0;
    SceneOwner *record=scene_record(parent,0);
    unsigned owner=record && record->owner==1 && record->serial==script_serial &&
        script_active && script_owner==1;
    if(!owner)owner=!mmx4_coop_alive(0) && mmx4_coop_alive(1);
    if(!owner || departed || mmx4_coop_lifecycle_hidden(1) ||
       !(mmx4_coop_second_body()[0x5C]&127u))return 0;
    /* The assessment graphic is also the course controller's completion
     * producer (CEF6C writes parent+16). If it follows hidden P1 after a P2
     * portal admission, native camera culling destroys it before that write.
     * Keep its single native update attached to the actual scene owner,
     * including the ordinary target motion, grading and completion signal. */
    scene_guard=1;
    uint32_t result=call_story_as_second(cpu,address,actor,cpu->gpr[5]);
    scene_guard=0;
    psx_mod_counter_add("mmx4.coop.p2-assessment-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
int mmx4_coop_lifecycle_assessment_bounds(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[4];
    /* During the owner projection above, preserve the completion producer
     * through the shared camera handoff. Bounds is a read-only query. Native
     * target motion, grading, timers and its final self-deletion still run;
     * a render may hide it, but must not destroy it before parent+0x16. */
    if(scene_guard && mmx4_coop_projected() && !psx_mod_local_view_scope() &&
       cpu->gpr[31]==0x800CEA8Cu && psx_mod_read_byte(PLAY+12)==6 &&
       !psx_mod_read_byte(PLAY+13) && actor>=0x80173CA0u && actor<0x801754A0u &&
       !((actor-0x80173CA0u)%0x60u) && psx_mod_read_byte(actor+1)==34 &&
       psx_mod_read_byte(actor+4)==1) {
        psx_mod_counter_add("mmx4.coop.assessment-completion-protected",1);
        return mmx4_coop_finish(cpu,0);
    }
    return 0;
}
static int gameplay_scene(void) {
    unsigned mode=psx_mod_read_byte(PLAY);
    /* Mode7 is the campaign's weapon/technique reward screen. P2's last
     * gameplay script and cached pad must not replace P1's frontend input
     * or project the surviving body over its presentation actors. */
    return mode==6 || mode==10 || mode==11;
}
static int floor_controller(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[4];
    /* A surviving P2 also owns the shared stage-script pass. Its floor
     * controller already ran with private momentum before terrain; leave
     * P1's canonical controller intact instead of applying it a second time. */
    if(!surface_call && mmx4_coop_ready() && mmx4_coop_projected() &&
       actor>=0x80142F98u && actor<0x80143598u &&
       !((actor-0x80142F98u)%0x30u) && psx_mod_read_byte(actor+1)==5)
        return mmx4_coop_finish(cpu,0);
    return 0;
}
void mmx4_coop_lifecycle_surface(CPUState *cpu) {
    if(surface_call || !mmx4_coop_ready() || !mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=6 ||
       mmx4_coop_lifecycle_hidden(1) || !mmx4_coop_alive(1))return;
    surface_call=1;
    for(unsigned slot=0;slot<32;++slot) {
        uint32_t actor=0x80142F98u+slot*0x30u;
        unsigned active=psx_mod_read_byte(actor);
        if(!active || psx_mod_read_byte(actor+1)!=5) {surface_valid[slot]=0;continue;}
        if(psx_mod_read_byte(PLAY+0x15) && !(active&8u))continue;
        uint8_t canonical[0x30];read_bytes(actor,canonical,sizeof canonical);
        if(!surface_valid[slot] || body_word(surface_second[slot],0x10)!=body_word(canonical,0x10)) {
            memcpy(surface_second[slot],canonical,sizeof canonical);
            surface_second[slot][4]=0; /* Native initialization clears momentum. */
            surface_valid[slot]=1;
        }
        write_bytes(actor,surface_second[slot],sizeof canonical);
        mmx4_coop_call(cpu,0x800B60BCu,actor,0);
        read_bytes(actor,surface_second[slot],sizeof canonical);
        write_bytes(actor,canonical,sizeof canonical);
        psx_mod_counter_add("mmx4.coop.p2-floor-ticks",1);
    }
    surface_call=0;
}
static int stage_scripts(CPUState *cpu,uint32_t address) {
    if(script_pool_guard || !mmx4_coop_ready() || !gameplay_scene() || mmx4_coop_projected() ||
       mmx4_coop_alive(0) || !mmx4_coop_alive(1))return 0;
    /* Original 8002166C updates exactly 32 native stage/script actors from
     * 80142F98, stride 0x30. It never runs a player, projectile, or enemy
     * world loop. The surviving body's progression triggers run once. */
    script_pool_guard=1;
    uint8_t canonical_camera[0xFC];
    int own_camera=mmx4_coop_split_scene_camera_begin(1,canonical_camera);
    uint32_t result=call_story_as_second(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    /* Stage actors cull against CAMERA before applying their room bounds.
     * On P1's death this pool precedes the ordinary camera-owner handoff;
     * using P1's old room deletes P2's ladder return strips (Web Spider and
     * Sigma1), leaving its upper death boundary permanently clamped. Run
     * the one native pool in the survivor's real view in every stage, then
     * retain its completed camera/bounds writes for that same owner. */
    if(own_camera)mmx4_coop_split_scene_camera_end(1,canonical_camera,1);
    script_pool_guard=0;return mmx4_coop_finish(cpu,result);
}
static int volcano_platform_start(CPUState *cpu,uint32_t address) {
    if(scene_guard || !mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12)!=4 ||
       psx_mod_read_byte(PLAY+13)!=1 || psx_mod_read_byte(PLAY+0x1D) ||
       !mmx4_coop_alive(1))return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80142F98u || actor>=0x80143598u || (actor-0x80142F98u)%0x30u ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=34 ||
       psx_mod_read_byte(actor+4))return 0;
    uint32_t x=psx_mod_read_word(PLAYER+8);
    uint32_t second_x=body_word(mmx4_coop_second_body(),8);
    if((int32_t)second_x<577*65536 || (int32_t)x>=577*65536)return 0;
    /* BCEA8 arms the one authored pillar sequence at signed X>576. Its
     * initialization reads only PLAYER.x; keep the nine original timers,
     * emitter allocations and terrain edits in the native shared update. */
    scene_guard=1;
    psx_mod_write_word(PLAYER+8,second_x);
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_word(PLAYER+8,x);
    scene_guard=0;
    return mmx4_coop_finish(cpu,result);
}
static void completed_survivor_exit(CPUState *cpu,uint32_t address) {
    (void)address;
    const uint8_t *body=mmx4_coop_second_body();
    /* Native stage_normal tests only PLAYER.state3. A shared forward fade
     * can suspend P1's death dispatcher before it reaches that state, while
     * the surviving P2 has already finished the real outgoing beam. Admit
     * that completed owner at this predicate only; keep both bodies and the
     * native stage/world calls intact. A temporary or voluntary passenger
     * departure is not completion, nor is a retry (negative PLAY+0F). */
    if(mmx4_coop_ready() && !mmx4_coop_projected() && !psx_mod_local_view_scope() &&
       psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+1) &&
       (int8_t)psx_mod_read_byte(PLAY+0x0F)>0 &&
       !(psx_mod_read_byte(PLAYER+0x5C)&127u) && !departed &&
       !mmx4_coop_lifecycle_hidden(1) && (body[0x5C]&127u) && body[4]==3 &&
       cpu->gpr[3]!=3) {
        cpu->gpr[3]=3;
        psx_mod_counter_add("mmx4.coop.survivor-completed-exits",1);
    }
}
static void storm_survivor_exit(CPUState *cpu,uint32_t address) {
    (void)address;
    /* Storm Owl's background controller checks PLAYER.x at 800DA440,
     * outside the small stage-script pool. Keep its one world update and
     * native X>=6352 warp predicate, but allow either living hero to reach
     * it. A distant living P1 must not suppress the leading P2's exit. */
    if(mmx4_coop_ready() && !mmx4_coop_projected() && !psx_mod_local_view_scope() &&
       psx_mod_read_byte(PLAY)==6 && psx_mod_read_byte(PLAY+12)==7 &&
       !psx_mod_read_byte(PLAY+13) && mmx4_coop_alive(1)) {
        int32_t second=(int16_t)(body_word(mmx4_coop_second_body(),8)>>16);
        if(!mmx4_coop_alive(0) || second>(int32_t)cpu->gpr[2])cpu->gpr[2]=(uint32_t)second;
    }
}
static void frost_ice_activation(CPUState *cpu,uint32_t address) {
    (void)address;
    /* B9F38 starts the opening ice cycle when PLAYER.x enters673..3167.
     * The shared timer and map edits still run once; either living hero
     * can satisfy just this initial predicate. Survivor projection already
     * supplies P2's native load when P1 is dead. */
    if(!mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13) || !mmx4_coop_alive(1))return;
    uint16_t first=(uint16_t)cpu->gpr[2];
    uint16_t second=(uint16_t)(body_word(mmx4_coop_second_body(),8)>>16);
    if((!mmx4_coop_alive(0) || (uint16_t)(first-673u)>=2495u) &&
       (uint16_t)(second-673u)<2495u)cpu->gpr[2]=second;
}
static void frost_path_activation(CPUState *cpu,uint32_t address) {
    (void)address;
    /* The later bridge has its own type21/subtype3 controller. BA340 arms
     * its authored particle/tile sequence at signed PLAYER.x>=3168; the
     * opening-cycle predicate above does not cover this path. Substitute
     * only the triggering coordinate, keeping one shared native sequence. */
    if(!mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13) || !mmx4_coop_alive(1))return;
    int32_t first=(int32_t)cpu->gpr[2];
    int16_t second=(int16_t)(body_word(mmx4_coop_second_body(),8)>>16);
    if((!mmx4_coop_alive(0) || first<3168) && second>=3168)
        cpu->gpr[2]=(uint32_t)(int32_t)second;
}
static int frost_path_contact(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    /* C41C8's invisible type17 trigger starts the native subtype4 collapse
     * sequence when a player touches the ice. Extending its initial spawn
     * predicate alone leaves the path permanent when only P2 crosses it.
     * This pure overlap query has no player or world side effects. */
    if(frost_path_contact_call || cpu->gpr[31]!=0x800C4204u || cpu->gpr[5]!=PLAYER ||
       !mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12)!=2 ||
       psx_mod_read_byte(PLAY+13) || !door_pool_actor(actor) ||
       !psx_mod_read_byte(actor) || psx_mod_read_byte(actor+1)!=17 ||
       psx_mod_read_byte(actor+4)!=1 || !mmx4_coop_alive(1))return 0;
    frost_path_contact_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,PLAYER);
    if(!result) {
        mmx4_coop_enter_second();
        result=mmx4_coop_call(cpu,address,actor,PLAYER);
        mmx4_coop_leave_second();
        if(result)psx_mod_counter_add("mmx4.coop.p2-ice-path-collapse",1);
    }
    frost_path_contact_call=0;return mmx4_coop_finish(cpu,result);
}
static void intro_platform_activation(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[16];
    /* BEED4 arms one authored falling-platform sequence when PLAYER.x
     * reaches the subtype's threshold. A living P1 behind the leader must
     * not leave the shared floor/ceiling edits waiting indefinitely. Only
     * extend that loaded coordinate; native fall, tile and sound run once. */
    if(!mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12) ||
       psx_mod_read_byte(PLAY+13)!=1 || !mmx4_coop_alive(1) ||
       !door_pool_actor(actor) || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=1 || psx_mod_read_byte(actor+2)>=15 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5))return;
    /* The authored table has15 entries. Subtype14 is the lower breakable
     * object at752/304, armed atX592 just like its subtype1 upper piece.
     * Excluding it lets P2 drop the upper piece while the lower one waits
     * indefinitely for P1. Native code retains the drop and collision. */
    int32_t second=(int16_t)(body_word(mmx4_coop_second_body(),8)>>16);
    if(!mmx4_coop_alive(0) || second>(int32_t)cpu->gpr[3]) {
        cpu->gpr[3]=(uint32_t)second;
        psx_mod_counter_add("mmx4.coop.p2-intro-platform-predicate",1);
    }
}
static void intro_survivor_crusher(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=cpu->gpr[16];
    /* C0558 compares PLAYER.y against each authored closing threshold.
     * A dead or trailing P1 above the shaft must not suppress the leading
     * partner. Substitute only the predicate's loaded Y; keep the native
     * movement, tile edits and two-player solid-contact pass canonical. */
    if(!mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12) ||
       psx_mod_read_byte(PLAY+13)!=1 || !mmx4_coop_alive(1) ||
       actor<0x80165A30u || actor>=0x80165A30u+32u*0x8Cu ||
       (actor-0x80165A30u)%0x8Cu || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=3 || psx_mod_read_byte(actor+4)!=1)return;
    int32_t second=(int16_t)(body_word(mmx4_coop_second_body(),12)>>16);
    if(!mmx4_coop_alive(0) || second>(int32_t)cpu->gpr[3])cpu->gpr[3]=(uint32_t)second;
}
static int fan_latch(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+12)!=3 ||
       actor<0x80165A30u || actor>=0x80165A30u+32u*0x8Cu ||
       (actor-0x80165A30u)%0x8Cu || psx_mod_read_byte(actor+1)!=13 ||
       psx_mod_read_byte(actor+4)!=1 || psx_mod_read_byte(actor+5))return 0;
    /* The native closing predicate reads PLAYER via a2, even after P1 has
     * died. Let the living partner cross that same threshold; retain the
     * original once-only animation and shared input freeze. */
    if(!mmx4_coop_alive(0) && mmx4_coop_alive(1) &&
       !mmx4_coop_lifecycle_hidden(1) && cpu->gpr[6]==PLAYER) {
        uint32_t result=call_story_as_second(cpu,address,actor,cpu->gpr[5]);
        psx_mod_counter_add("mmx4.coop.survivor-latch",1);
        return mmx4_coop_finish(cpu,result);
    }
    if(!mmx4_coop_alive(0) || !mmx4_coop_alive(1) ||
       mmx4_coop_lifecycle_hidden(0) || mmx4_coop_lifecycle_hidden(1))return 0;
    const uint8_t *body=mmx4_coop_second_body();
    Mmx4CoopViewActor players[2]={
        {(int16_t)psx_mod_read_half(PLAYER+10),(int16_t)psx_mod_read_half(PLAYER+14),1},
        {(int16_t)(body_word(body,8)>>16),(int16_t)(body_word(body,12)>>16),1}};
    int camera_x=(int16_t)psx_mod_read_half(0x801419BAu);
    int camera_y=(int16_t)psx_mod_read_half(0x801419BEu);
    WsViewAnchor view={0};
    if(psx_mod_netplay_is_active()) {
        /* Own-view render anchors can differ between peers. Shared decisions
         * use only the admitted aspect and canonical native camera limits. */
        int aspect=psx_mod_netplay_aspect();
        int extra=aspect==2?120:aspect==1?53:0;
        view=ws_view_anchor(extra,camera_x,(int16_t)psx_mod_read_half(0x801419CEu),
            (int16_t)psx_mod_read_half(0x801419CCu));
    }else gpu_ws_bg2d_get_view(0,&view);
    /* Wait only for the visible partner. Camera following runs after this
     * actor pass, so the advancing player may cross its top edge this tick.
     * A dead, withdrawn, warping or distant partner must not hold it open. */
    int x=players[1].world_x-camera_x,y=players[1].world_y-camera_y;
    if(mmx4_coop_split_views()) {
        /* Each living hero has a separate viewport. P1's view must not
         * decide whether the partner still below this hatch exists. Keep
         * the latch open for a partner in this climbing shaft; someone in
         * another horizontal room must not hold an unrelated latch. */
        int shaft_x=players[1].world_x-(int16_t)psx_mod_read_half(actor+10);
        if(shaft_x < -320 || shaft_x > 320)return 0;
    }else if(x < -view.left-16 || x >= 320+view.right+16 || y < -32 || y >= 272)return 0;
    uint32_t player_box=body_word(body,0x68),platform_box=psx_mod_read_word(actor+0x68);
    if(!platform_box)return 0;
    /* X's native movement transition briefly clears its collision box.
     * Missing geometry is not evidence that the living on-screen partner
     * has cleared the hatch. Wait for its next ordinary contact box. */
    if(!player_box) {
        psx_mod_counter_add("mmx4.coop.latch-waits",1);
        return mmx4_coop_finish(cpu,0);
    }
    /* Original800C36E8 closes only when PLAYER's collision bottom clears
     * the platform's top. Apply that same test to the second body before
     * the native routine freezes input and begins the closing animation. */
    int distance=(int16_t)((uint16_t)psx_mod_read_half(actor+14)-
        (uint16_t)players[1].world_y);
    int clearance=(int8_t)psx_mod_read_byte(player_box+1)+psx_mod_read_byte(player_box+3)+
        psx_mod_read_byte(platform_box+3)-(int8_t)psx_mod_read_byte(platform_box+1);
    if(distance>=0 && distance>=clearance)return 0;
    psx_mod_counter_add("mmx4.coop.latch-waits",1);
    return mmx4_coop_finish(cpu,0);
}
static int shared_ride_ready(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!mmx4_coop_projected() || cpu->gpr[31]!=0x8003B64Cu)return 0;
    /* Original Ride Chaser initialization allocates stage actor 0x1B (READY)
     * at 8003B644. Both bikes later wait on vehicle+A0 at 8003BD44. Reuse
     * P1's presentation without reinitializing its actor or emitting READY
     * twice; all other stage actor allocations retain native behavior. */
    const uint8_t *vehicle=mmx4_coop_first_vehicle();
    psx_mod_write_word(MMX4_VEHICLE+0xA0,body_word(vehicle,0xA0));
    psx_mod_counter_add("mmx4.coop.shared-ride-ready",1);
    return mmx4_coop_finish(cpu,0);
}
static int script_text(CPUState *cpu,uint32_t address) {
    if(text_guard || !mmx4_coop_ready() || !gameplay_scene() || mmx4_coop_projected() ||
       !script_active || script_owner!=1)return 0;
    uint8_t pad[6];read_bytes(PAD,pad,sizeof pad);
    for(unsigned index=0;index<3;++index)psx_mod_write_half(PAD+index*2u,story_pad[index]);
    text_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    text_guard=0;write_bytes(PAD,pad,sizeof pad);
    psx_mod_counter_add("mmx4.coop.p2-dialogue-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static int shared_text_init(CPUState *cpu,uint32_t address) {
    if(text_init_guard || !mmx4_coop_ready() || !mmx4_coop_projected())return 0;
    /* Native8002217C saves/replaces the shared textbox CLUT at palette+2A0,
     * requests its upload, and binds the campaign's script resources. These
     * must survive leaving the actor projection. Subsequent text input still
     * belongs to the seat that committed the scene. A story pass can keep
     * PLAY+43 canonical while PLAYER+2 names P2; preserve that distinction. */
    uint8_t story_campaign=psx_mod_read_byte(PLAY+0x43);
    psx_mod_write_byte(PLAY+0x43,psx_mod_read_byte(PLAYER+2));
    mmx4_coop_leave_second();text_init_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    text_init_guard=0;mmx4_coop_enter_second();
    psx_mod_write_byte(PLAY+0x43,story_campaign);
    psx_mod_counter_add("mmx4.coop.canonical-dialogue-init",1);
    return mmx4_coop_finish(cpu,result);
}
static int waiting_door_query(CPUState *cpu,uint32_t address) {
    /* A mounted hero has no on-foot collision box. Eligibility remains a
     * read-only query: temporarily use the native foot box, restoring both
     * fields before returning. Both-player waiting must not eject either
     * rider until the actual door admission is committed. C5=0 also guards
     * the nested original query; no second Armor update runs here. */
    if(!door_wait && mmx4_coop_armor_mounted() && !psx_mod_local_view_scope()) {
        uint8_t mounted=psx_mod_read_byte(PLAYER+0xC5);
        uint32_t box=psx_mod_read_word(PLAYER+0x68);
        psx_mod_write_byte(PLAYER+0xC5,0);
        mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
        uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        psx_mod_write_byte(PLAYER+0xC5,mounted);psx_mod_write_word(PLAYER+0x68,box);
        return mmx4_coop_finish(cpu,result);
    }
    /* Only suppress admission, never the native door's animation/render pass. */
    if(!door_wait)return 0;
    psx_mod_counter_add("mmx4.coop.boss-door-waits",1);
    return mmx4_coop_finish(cpu,0);
}
static int both_door_traversal(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!scene_guard || !both_door_scope() || both_door.actor!=cpu->gpr[4] ||
       both_door.phase<2 || both_door.phase>3)return 0;
    /* Retail C1D90 moves the singleton by A400 fixed units every tick. Both
     * mode instead waits for actual movement and the full collision boxes
     * to clear. Keeping C4 outside the control pass also keeps bosses waiting. */
    if(!both_door_cleared())return mmx4_coop_finish(cpu,0);
    psx_mod_write_half(0x801419D6u,both_door.target_min);
    both_door.phase=3;
    /* Free movement can put the opener farther inside the new room than
     * retail's auto-walk, especially when P2 leads. An exact minimum-X
     * wait then never finishes (Frost: camera6080, minimum6032). Require
     * arrival inside the authored interval, after both boxes clear. */
    int camera_x=(int16_t)psx_mod_read_half(0x801419BAu);
    int minimum=(int16_t)both_door.target_min;
    int maximum=(int16_t)psx_mod_read_half(0x801419D4u);
    if(camera_x<minimum || camera_x>maximum)
        return mmx4_coop_finish(cpu,0);
    both_door_gate(0,0);both_door_gate(1,0);
    mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
    psx_mod_write_byte(both_door.actor+5,4);
    mmx4_coop_call(cpu,0x80015D60u,both_door.actor,1);
    uint32_t old_a2=cpu->gpr[6];cpu->gpr[6]=both_door.actor;
    mmx4_coop_call(cpu,0x8001540Cu,0,0x11);cpu->gpr[6]=old_a2;
    both_door.phase=4;
    psx_mod_counter_add("mmx4.coop.both-door-crossings",1);
    return mmx4_coop_finish(cpu,0);
}
static int paired_boss_admission(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!mmx4_coop_ready() || !both_door_scope() ||
       both_door.serial!=script_serial || both_door.phase>3)return 0;
    /* Colonel's 80085F8C, Storm Owl's General 80088D30 and Frost's
     * midboss 80052218 start admission without waiting for C4/camera arrival.
     * A leading player can reach that trigger with the partner still in the
     * aperture. That command aborts the crossing, restores
     * the sealed map's collision and crushes the leader. Defer the entire
     * admission (including its actor allocation) until physical clearance
     * and camera arrival, then let the original introduction run once. */
    psx_mod_counter_add("mmx4.coop.boss-admission-waits",1);
    return mmx4_coop_finish(cpu,0);
}
static void paired_intro_camera(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!mmx4_coop_ready() || psx_mod_local_view_scope() || !both_door_scope() ||
       both_door.serial!=script_serial || both_door.phase>3 ||
       psx_mod_read_byte(PLAY+0x0C) || psx_mod_read_byte(PLAY+0x0D))return;
    /* D0554 admits the act1 exit as soon as the camera reaches4848. That
     * can precede physical clearance of a paired door. Its command then
     * aborts the crossing before the new minimum is committed, so native
     * auto-walk chases a moving camera forever. Retain the original wait
     * until crossing finishes; the authored scene then runs once. */
    cpu->gpr[3]=cpu->gpr[2]^1u;
}
static void paired_iris_camera(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!mmx4_coop_ready() || psx_mod_local_view_scope() || !both_door_scope() ||
       both_door.serial!=script_serial || both_door.phase>4 ||
       psx_mod_read_byte(PLAY+0x0C)!=11 || psx_mod_read_byte(PLAY+0x0D))return;
    /* Iris waits for camera X=2032 before issuing action15. Free movement
     * through a paired door can already put the camera farther into her
     * authored 2032..2096 room; the door also commits its minimum of 2048.
     * Equality alone can therefore wait forever. Admit
     * only after the physical crossing, with the camera inside that room.
     * Earlier phases must keep waiting even if the camera crosses 2032. */
    int x=(int32_t)cpu->gpr[3];
    unsigned minimum=psx_mod_read_half(0x801419D6u);
    if(both_door.phase<4)cpu->gpr[3]=cpu->gpr[2]^1u;
    else if(x>=2032 && x<=2096 && (minimum==2032 || minimum==2048) &&
            psx_mod_read_half(0x801419D4u)==2096)cpu->gpr[3]=cpu->gpr[2];
}
static int both_door_map(CPUState *cpu,uint32_t address) {
    if(door_terrain_guard || !mmx4_coop_ready() || !both_door_scope() ||
       both_door.phase<2 || both_door.phase>3 || cpu->gpr[4]!=PLAYER ||
       !door_pool_actor(both_door.actor) || psx_mod_read_byte(both_door.actor+1)!=8 ||
       psx_mod_read_byte(both_door.actor+4)!=1 || psx_mod_read_byte(both_door.actor+5)!=3)return 0;
    uint32_t box=psx_mod_read_word(both_door.actor+0x68);
    if(!box || !mmx4_snapshot_guest(box))return 0;
    int x=(int16_t)cpu->gpr[5],y=(int16_t)cpu->gpr[6];
    int center_x=(int16_t)psx_mod_read_half(both_door.actor+10)+(int8_t)psx_mod_read_byte(box);
    int center_y=(int16_t)psx_mod_read_half(both_door.actor+14)+(int8_t)psx_mod_read_byte(box+1);
    int width=psx_mod_read_byte(box+2),height=psx_mod_read_byte(box+3);
    if(x<center_x-width || x>=center_x+width || y<center_y-height || y>=center_y+height)return 0;
    /* Retail door auto-walk disables the body's whole terrain box to cross
     * the sealed map cells. Normal Both controls keep their real box and
     * floor collision: only the observed 38 wall samples inside the fully
     * opened aperture become passable. This includes interior ceiling tests
     * that would otherwise crush a body against the floor. The aperture's
     * bottom edge is excluded, preserving its native floor; other tiles and
     * lethal classes retain the original query and its side effects. */
    door_terrain_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    door_terrain_guard=0;
    if(result==0x38u) {
        result=0;psx_mod_counter_add("mmx4.coop.both-door-map-passages",1);
    }
    return mmx4_coop_finish(cpu,result);
}
static void dismount_at_door(CPUState *cpu,unsigned seat,uint32_t actor) {
    if(seat)mmx4_coop_enter_second();
    unsigned touch=mmx4_coop_armor_mounted() &&
        mmx4_coop_call(cpu,0x800C1E7Cu,actor,0);
    if(seat)mmx4_coop_leave_second();
    if(touch)mmx4_coop_armor_dismount(cpu,seat);
}
static int scene_actor_canonical(CPUState *cpu,uint32_t address) {
    if(scene_guard || !mmx4_coop_ready() || !gameplay_scene() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];unsigned owner=0;
    unsigned train=address==0x800D8648u || address==0x800D9218u || address==0x800D9C84u;
    unsigned refight_portal=address==0x800C7164u;
    unsigned sigma_entrance=address==0x800D2A38u;
    if(sigma_entrance && (psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13)))return 0;
    if(refight_portal && (psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13) ||
       !door_pool_actor(actor) || psx_mod_read_byte(actor+1)!=27))return 0;
    SceneOwner *record=scene_record(actor,0);
    if((psx_mod_read_byte(actor+4)==0 || (train && !script_active)) && record) {
        memset(record,0,sizeof *record);record=NULL;
    }
    if(record)owner=record->owner;
    else if((address==0x800D0374u || address==0x800D1248u || address==0x800D1284u ||
             sigma_entrance) && script_active)
        owner=script_owner;
    else if(!mmx4_coop_alive(0) && mmx4_coop_alive(1))owner=1;
    else if(train && script_active)owner=script_owner;
    else if(train && mmx4_coop_alive(1) &&
            (int16_t)psx_mod_read_half(PLAYER+10)<
            (int16_t)(body_word(mmx4_coop_second_body(),8)>>16))owner=1;
    else if(refight_portal && psx_mod_read_byte(actor+4)==1 &&
            !psx_mod_read_byte(actor+5)) {
        /* C7970 only reads the original body/portal collision boxes. Keep
         * the full C7164 state machine with the hero who actually enters;
         * its later phases still wait on that body's native warp state. */
        unsigned first_hit=mmx4_coop_alive(0)?
            mmx4_coop_call(cpu,0x800C7970u,actor,PLAYER):0;
        unsigned second_hit=0;
        if(mmx4_coop_alive(1)) {
            mmx4_coop_enter_second();
            second_hit=mmx4_coop_call(cpu,0x800C7970u,actor,PLAYER);
            mmx4_coop_leave_second();
        }
        if(!first_hit && second_hit)owner=1;
        if(mmx4_coop_both_door_players() && mmx4_coop_alive(0) &&
           mmx4_coop_alive(1) && !departed && (!first_hit || !second_hit))door_wait=1;
    }
    else if(address==0x800C2BE0u && mmx4_coop_alive(1) &&
            !(mmx4_coop_solid_contact_bits(actor,0)&8u) &&
             (mmx4_coop_solid_contact_bits(actor,1)&8u))owner=1;
    else if(address==0x800C1994u && mmx4_coop_alive(1) &&
            psx_mod_read_byte(actor+4)==1 && !psx_mod_read_byte(actor+5)) {
        /* Native 800C1E7C is a read-only bounding/state eligibility query.
         * P1 wins a simultaneous door entry; P2 can enter independently. */
        uint32_t first_hit=mmx4_coop_alive(0)?
            mmx4_coop_call(cpu,0x800C1E7Cu,actor,0):0;
        if(!first_hit || mmx4_coop_both_door_players()) {
            mmx4_coop_enter_second();
            uint32_t second_hit=mmx4_coop_call(cpu,0x800C1E7Cu,actor,0);
            mmx4_coop_leave_second();
            if(!first_hit && second_hit)owner=1;
            if(mmx4_coop_both_door_players() && !departed && mmx4_coop_alive(0) &&
               (first_hit || second_hit) && (!first_hit || !second_hit))door_wait=1;
        }
    }
    else if((address==0x800BD654u || address==0x800C165Cu || address==0x800C169Cu) &&
            mmx4_coop_alive(1) &&
            (int32_t)psx_mod_read_word(PLAYER+8)<
            (int32_t)((uint32_t)mmx4_coop_second_body()[8]|
                (uint32_t)mmx4_coop_second_body()[9]<<8|
                (uint32_t)mmx4_coop_second_body()[10]<<16|
                (uint32_t)mmx4_coop_second_body()[11]<<24))owner=1;
    if(!mmx4_coop_alive(owner))return mmx4_coop_finish(cpu,0);
    unsigned serial=script_serial;
    unsigned old_door=address==0x800C1994u?(owner?
        mmx4_coop_second_body()[0xC4]:psx_mod_read_byte(PLAYER+0xC4)):0;
    uint8_t canonical_camera[0xFC];
    int own_camera=owner && mmx4_coop_split_scene_camera_begin(owner,canonical_camera);
    unsigned pair_candidate=address==0x800C1994u && !old_door && !door_wait &&
        door_pool_actor(actor) && psx_mod_read_byte(actor+1)==8 &&
        mmx4_coop_both_door_players() && !departed &&
        mmx4_coop_alive(0) && mmx4_coop_alive(1);
    if(address==0x800C1994u && !old_door && !door_wait) {
        /* Doors admit heroes on foot. Eject at the source, after both native
         * eligibility tests, so Both mode still waits for the actual partner. */
        if(pair_candidate) {
            dismount_at_door(cpu,0,actor);dismount_at_door(cpu,1,actor);
        }else dismount_at_door(cpu,owner,actor);
    }
    uint16_t old_min=psx_mod_read_half(0x801419D6u);
    uint32_t door_box=pair_candidate?psx_mod_read_word(actor+0x68):0;
    int door_right=(int16_t)psx_mod_read_half(actor+10);
    if(door_box)door_right+=(int8_t)psx_mod_read_byte(door_box)+psx_mod_read_byte(door_box+2);
    scene_guard=1;uint32_t result;
    if(owner) {
        uint8_t contact[4];static const unsigned at[]={0x72,0x74,0x76,0x78};
        if(address==0x800C2BE0u || refight_portal) {
            for(unsigned i=0;i<4;++i)contact[i]=psx_mod_read_byte(actor+at[i]);
            psx_mod_write_byte(actor+0x72,mmx4_coop_solid_contact_bits(actor,1));
            projected_portal_solid=actor;
        }
        result=call_story_as_second(cpu,address,actor,cpu->gpr[5]);
        if(address==0x800C2BE0u || refight_portal) {
            projected_portal_solid=0;
            for(unsigned i=0;i<4;++i)psx_mod_write_byte(actor+at[i],contact[i]);
            /* Retail C2C20 and C71A4 perform one solid pass after each portal
             * state update. Inside P2's story projection that bypasses the
             * combat wrapper, leaving P1 without platform collision whenever
             * P2 stands on it. Restore the canonical histories first, then
             * let that one native pass resolve contact for both seats. */
            mmx4_coop_call(cpu,0x8002E184u,actor,0);
        }
    }else result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    scene_guard=door_wait=0;
    unsigned new_door=address==0x800C1994u?(owner?
        mmx4_coop_second_body()[0xC4]:psx_mod_read_byte(PLAYER+0xC4)):0;
    if(address==0x800C1994u && !old_door && new_door) {
        /* Boss doors use personal C4, not 80036AE4/C0 script commands. */
        script_active=1;script_owner=owner;++script_serial;
        if(pair_candidate) {
            both_door=(BothDoor){actor,script_serial,old_min,old_min,(uint16_t)door_right,
                1,(uint8_t)owner,psx_mod_read_byte(PLAY+0x0C),psx_mod_read_byte(PLAY+0x0D)};
            /* The partner also belongs to this doorway's admission gate.
             * Signed FF takes 338CC's blez return, preserving its normal
             * action and box; only the native owner consumes positive C4=1. */
            both_door_gate(owner^1u,255);
        }else request_script_departure(owner);
    }else if(address==0x800C1994u && old_door && !new_door &&
             script_active && script_owner==owner && record && record->serial==script_serial) {
        script_active=0;
    }
    if(address==0x800C1994u && both_door.actor==actor && both_door.phase==1 &&
       psx_mod_read_byte(actor+5)==3) {
        /* Native C1C88 supplies the new room bounds and ends the opening
         * animation. Retain its max X, but defer its min X until everyone is
         * through so 27BE4 cannot push a waiting player into the doorway. */
        both_door.target_min=psx_mod_read_half(0x801419D6u);
        psx_mod_write_half(0x801419D6u,both_door.old_min);
        for(unsigned i=0;i<4;++i)both_door.bounds[i]=psx_mod_read_half(0x801419D4u+i*2u);
        mmx4_coop_split_door_camera_bounds(3,both_door.bounds);
        if(owner)mmx4_coop_enter_second();
        mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
        if(owner)mmx4_coop_leave_second();
        both_door.phase=2;
    }
    if(serial!=script_serial) {
        record=scene_record(actor,1);
        if(record) {record->owner=(uint8_t)owner;record->serial=script_serial;}
    }
    if(record && (!psx_mod_read_byte(actor) ||
        (!script_active && (psx_mod_read_byte(actor+4)>=3 ||
            (address==0x800C1994u && !new_door &&
                (psx_mod_read_byte(actor+4)>=2 || psx_mod_read_byte(actor+5)>=4))))))
        memset(record,0,sizeof *record);
    if(own_camera)mmx4_coop_split_scene_camera_end(owner,canonical_camera,
        script_active && script_owner==owner && (new_door || serial!=script_serial));
    return mmx4_coop_finish(cpu,result);
}
static int refight_portal_query(CPUState *cpu,uint32_t address) {
    (void)address;
    if(scene_guard && door_wait && mmx4_coop_ready())
        return mmx4_coop_finish(cpu,0);
    return 0;
}
int mmx4_coop_lifecycle_defer_portal_solid(uint32_t actor) {
    return scene_guard && projected_portal_solid && projected_portal_solid==actor;
}
static int scene_actor(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,scene_actor_canonical);
}
static int train_midboss(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(psx_mod_local_view_scope() || psx_mod_read_byte(PLAY+12)!=8 ||
       psx_mod_read_byte(PLAY+13) || actor<0x80169CB8u || actor>=0x80169D78u ||
       (actor-0x80169CB8u)%0x30u || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=3 || psx_mod_read_byte(actor+2))return 0;
    /* D9CB0 admits the train midboss only after PLAYER.x reaches7009.
     * This category7 controller is outside the nearest-enemy AI pool. Run
     * its one native allocation/scene with the progressing living hero. */
    return scene_actor(cpu,address);
}
int mmx4_coop_lifecycle_train(CPUState *cpu,uint32_t address) {
    return scene_actor(cpu,address);
}
int mmx4_coop_lifecycle_lift_start(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(!mmx4_coop_ready() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY+12)!=3 || psx_mod_read_byte(PLAY+13)!=1 ||
       !door_pool_actor(actor) || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=14 || psx_mod_read_byte(actor+4)!=1 ||
       psx_mod_read_byte(actor+5) || psx_mod_read_byte(actor+6) ||
       mmx4_coop_alive(0) || !mmx4_coop_alive(1))return 0;
    /* Native C3A7C waits for PLAYER.action before starting the shared lift.
     * A completed P1 death leaves action zero forever. Run that admission
     * update with the living rider; retain one lift/panel/collision update
     * and restore the canonical corpse and campaign afterward. Subsequent
     * ascent and authored ceiling-panel destruction remain native. */
    return scene_actor(cpu,address);
}
static int cyber_flip_query(CPUState *cpu,uint32_t address) {
    if(flip_guard || !mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+0x0C)!=6 ||
       psx_mod_read_byte(PLAY+0x0D)!=1)return 0;
    uint32_t actor=cpu->gpr[4];flip_guard=1;
    uint32_t result=0;
    if(mmx4_coop_alive(0))result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(!psx_mod_read_byte(actor+5) && mmx4_coop_alive(1)) {
        mmx4_coop_enter_second();
        result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
        mmx4_coop_leave_second();
    }
    flip_guard=0;return mmx4_coop_finish(cpu,result);
}
static int cyber_flip_world(CPUState *cpu,uint32_t address) {
    if(flip_guard || !mmx4_coop_ready() || mmx4_coop_projected() || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+0x0C)!=6 ||
       psx_mod_read_byte(PLAY+0x0D)!=1)return 0;
    flip_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    mmx4_coop_flip_second();mmx4_coop_split_flip_cameras();
    if(warp_phase)warp_origin_y=0x08000000u-warp_origin_y;
    flip_guard=0;psx_mod_counter_add("mmx4.coop.shared-cyber-flips",1);
    return mmx4_coop_finish(cpu,result);
}
static int paired_victory_scene(CPUState *cpu) {
    unsigned stage=psx_mod_read_byte(PLAY+12);
    uint32_t actor=psx_mod_read_word(0x1F80004Cu);
    if(stage<1 || stage>8 || cpu->gpr[4]!=0x14u ||
       actor<0x8013BED0u || actor>=0x8013BED0u+48u*0x9Cu ||
       (actor-0x8013BED0u)%0x9Cu || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+4)!=2 || (psx_mod_read_byte(actor+0x5C)&127u) ||
       psx_mod_read_word(PLAY+0x20)!=actor ||
       departed || warp_pending || warp_phase || !mmx4_coop_alive(0) || !mmx4_coop_alive(1))return 0;
    unsigned kind=psx_mod_read_byte(actor+1);
    /* These midbosses continue the stage; their scenes retain retail flow. */
    return kind!=18 && kind!=33 && kind!=70;
}
static int partner_victory_pose(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!paired_victory || !mmx4_coop_ready() || !mmx4_coop_projected() ||
       psx_mod_local_view_scope() || cpu->gpr[4]!=PLAYER ||
       !(psx_mod_read_byte(PLAY+0x0F)&0x10u))return 0;
    /* Match native34F7C's character cleanup/pose, leaving its one shared
     * music request to P1. Each native33368 then animates its own character
     * and outgoing beam; no partner withdrawal/return is needed. */
    mmx4_coop_call(cpu,0x80036088u,PLAYER,0);
    mmx4_coop_call(cpu,0x80038490u,PLAYER,0);
    mmx4_coop_call(cpu,0x80036534u,PLAYER,0);
    mmx4_coop_call(cpu,0x80036E98u,PLAYER,0);
    psx_mod_write_byte(PLAY+0x1C,1);
    psx_mod_write_byte(PLAYER+5,0x19);psx_mod_write_byte(PLAYER+6,0);
    psx_mod_counter_add("mmx4.coop.paired-victory-poses",1);
    return mmx4_coop_finish(cpu,0);
}
static int fortress_shared_scene(CPUState *cpu) {
    /* Native 14 is a stationary/facing scene command, not a room warp.
     * Refight defeat scenes and Sigma's phase presentations can retain two
     * nearby heroes. A distant, absent or still-transferring partner must
     * keep the ordinary carry path; native 15 transport is never bypassed. */
    if(cpu->gpr[4]!=0x14u || cpu->gpr[31]<0x80048000u || cpu->gpr[31]>=0x80094000u ||
       psx_mod_read_byte(PLAY+12)!=12 || psx_mod_read_byte(PLAY+13)>1 ||
       psx_mod_local_view_scope() || departed || warp_pending || warp_phase ||
       !mmx4_coop_alive(0) || !mmx4_coop_alive(1))return 0;
    uint8_t current[0xE4];read_bytes(PLAYER,current,sizeof current);
    const uint8_t *first=mmx4_coop_projected()?mmx4_coop_first_body():current;
    const uint8_t *second=mmx4_coop_projected()?current:mmx4_coop_second_body();
    if(!first[3] || !second[3] || first[4]!=1 || second[4]!=1 ||
       first[0x14]!=second[0x14] || first[0xC4] || second[0xC4])return 0;
    int x=(int16_t)(body_word(first,8)>>16)-(int16_t)(body_word(second,8)>>16);
    int y=(int16_t)(body_word(first,12)>>16)-(int16_t)(body_word(second,12)>>16);
    return x>=-240 && x<=240 && y>=-160 && y<=160;
}
static int script_command(CPUState *cpu,uint32_t address) {
    if(command_guard || !mmx4_coop_ready() || !gameplay_scene())return 0;
    /* Marine Base's Chaser exit gives each rider its own native action17.
     * Carrying the other seat as a shared scene suspends its vehicle before
     * ejection, then restores the mounted rider at the leader's on-foot Y.
     * Its delayed ejection starts below the landing floor. Let both original
     * vehicles issue/release their own pose, retaining native jumps/physics. */
    if(psx_mod_read_byte(PLAY+12)==5 && psx_mod_read_byte(PLAY+13)==1 &&
       psx_mod_read_byte(MMX4_VEHICLE) && !psx_mod_read_byte(MMX4_VEHICLE+1) &&
       ((address==0x80036AE4u && cpu->gpr[31]==0x8003BA84u &&
         cpu->gpr[4]==0x17u && cpu->gpr[5]==0x40u) ||
        (address==0x80036B18u && cpu->gpr[31]==0x8003BAC4u)))return 0;
    /* Owl's upper entry area8010AC70 sets minX2240; the adjacent rear area
     * 8010AC7C releases it to0 for backtracking. A trailing hero can occupy
     * that release after the leader crosses the entry, then warp past the
     * entry during the boss introduction. Commit its authored enclosure
     * only when the native Owl admission actually starts (750FC). Keep the
     * current camera position/limit easing and ordinary area scripts intact. */
    if(address==0x80036AE4u && cpu->gpr[31]==0x80075104u &&
       cpu->gpr[4]==0x14u && !cpu->gpr[5] && !psx_mod_local_view_scope() &&
       psx_mod_read_byte(PLAY)==6 && psx_mod_read_byte(PLAY+12)==7 &&
       psx_mod_read_byte(PLAY+13)==1)
        psx_mod_write_half(0x801419D6u,2240);
    unsigned owner=mmx4_coop_projected()?1u:0u;
    fprintf(stderr,"mmx4.coop: scene-command stage=%u section=%u address=%08X caller=%08X "
        "command=%u seat=%u pair=%u serial=%u/%u\n",
        psx_mod_read_byte(PLAY+12),psx_mod_read_byte(PLAY+13),address,cpu->gpr[31],
        cpu->gpr[4],owner,both_door.phase,both_door.serial,script_serial);
    unsigned redirect=!owner && !mmx4_coop_alive(0) && mmx4_coop_alive(1);
    if(redirect) {mmx4_coop_enter_second();owner=1;}
    if(address==0x80036AE4u && cpu->gpr[31]==0x80049548u &&
       cpu->gpr[4]==0x15u && !cpu->gpr[5] && !psx_mod_local_view_scope() &&
       psx_mod_read_byte(PLAY)==6 && !psx_mod_read_byte(PLAY+12) &&
       psx_mod_read_byte(PLAY+13)==1)
        mmx4_coop_split_commit_scene_camera(owner);
    if(address==0x80036AE4u && paired_victory_scene(cpu)) {
        paired_victory=1;
        psx_mod_counter_add("mmx4.coop.paired-victory-scenes",1);
    }
    unsigned paired_boss=address==0x80036AE4u && both_door_scope() &&
        both_door.serial==script_serial && both_door.phase>=4 &&
        cpu->gpr[31]>=0x80048000u && cpu->gpr[31]<0x80094000u &&
        (cpu->gpr[4]==0x14u || cpu->gpr[4]==0x15u);
    unsigned fortress_pair=address==0x80036AE4u && fortress_shared_scene(cpu);
    /* Only the first retail boss admission after this physical crossing and
     * its still-active conversation inherit the pair. A portal, stage warp,
     * later victory, or another command must retain the existing departure. */
    if(address==0x80036AE4u && !paired_boss)both_door_abort(cpu);
    command_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    command_guard=0;
    if(address==0x80036AE4u) {
        script_active=1;script_owner=owner;++script_serial;
        if(paired_boss) {
            both_door.phase=5;both_door.owner=(uint8_t)owner;both_door.serial=script_serial;
        }
    }else {script_active=0;memset(&both_door,0,sizeof both_door);}
    if(!paired_boss && !fortress_pair)request_script_departure(owner);
    if(redirect)mmx4_coop_leave_second();
    return mmx4_coop_finish(cpu,result);
}
static int nonowner_controls(CPUState *cpu,uint32_t address) {
    if(!control_guard && mmx4_coop_ready() &&
       mmx4_coop_lifecycle_hidden(mmx4_coop_projected()?1u:0u))
        return mmx4_coop_finish(cpu,0);
    if(!control_guard && initial_arrival && mmx4_coop_ready() &&
       mmx4_coop_projected() && !psx_mod_local_view_scope()) {
        uint8_t lock=psx_mod_read_byte(PLAY+0x1C);
        uint8_t cameras[3]={psx_mod_read_byte(0x801419F4u),
            psx_mod_read_byte(0x80141A48u),psx_mod_read_byte(0x80141A9Cu)};
        control_guard=1;
        uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        control_guard=0;
        /* P2's 80031410 may finish first. Its private beam cannot release
         * P1's native arrival lock or enable the shared cameras early. */
        psx_mod_write_byte(PLAY+0x1C,lock);
        psx_mod_write_byte(0x801419F4u,cameras[0]);
        psx_mod_write_byte(0x80141A48u,cameras[1]);
        psx_mod_write_byte(0x80141A9Cu,cameras[2]);
        if(psx_mod_read_byte(PLAYER+4)!=1 || psx_mod_read_byte(PLAYER+5)!=0)
            initial_arrival=0;
        return mmx4_coop_finish(cpu,result);
    }
    if(control_guard || !mmx4_coop_ready() || !script_active)return 0;
    unsigned seat=mmx4_coop_projected()?1u:0u;
    if(both_door_scope() && both_door.serial==script_serial &&
       both_door.phase>=2 && both_door.phase<=3 && mmx4_coop_alive(seat)) {
        /* Let the real controller leave action19 and retain walk, dash and
         * attacks. C4 is restored for the enemy/door dispatchers; positions,
         * velocities, native collision results and controller output persist. */
        uint8_t door=psx_mod_read_byte(PLAYER+0xC4);
        psx_mod_write_byte(PLAYER+0xC4,0);
        control_guard=1;
        uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        control_guard=0;
        if(both_door.phase>=2 && both_door.phase<=3)
            psx_mod_write_byte(PLAYER+0xC4,door);
        return mmx4_coop_finish(cpu,result);
    }
    if(seat==script_owner || !mmx4_coop_alive(seat))return 0;
    /* The scripted owner retains native auto-walk/warp. Its living partner
     * receives neutral controls until the common transition completes. */
    psx_mod_write_half(PLAYER+0x7C,0);psx_mod_write_half(PLAYER+0x7E,0);
    psx_mod_write_half(PLAYER+0x80,0);
    control_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    control_guard=0;return mmx4_coop_finish(cpu,result);
}

static int hidden_terrain(CPUState *cpu,uint32_t address) {
    (void)address;
    unsigned seat=mmx4_coop_projected()?1u:0u;
    if(!warp_guard && mmx4_coop_ready() && cpu->gpr[4]==PLAYER) {
        if(mmx4_coop_lifecycle_hidden(seat))return mmx4_coop_finish(cpu,0);
        /* The native door owns its auto-walk across the solid doorway.
         * A recovered partner must not be crushed by a terrain retry while
         * that control lock is active. Resume terrain at C4 release. */
        if(script_active && script_owner==seat && psx_mod_read_byte(PLAYER+0xC4)) {
            for(unsigned i=0;i<sizeof scenes/sizeof scenes[0];++i)
                if(scenes[i].actor && scenes[i].owner==seat && scenes[i].recovered) {
                    psx_mod_write_byte(PLAYER+0x70,(uint8_t)(psx_mod_read_byte(PLAYER+0x70)&8u));
                    psx_mod_write_byte(PLAYER+0x71,(uint8_t)(psx_mod_read_byte(PLAYER+0x71)&8u));
                    return mmx4_coop_finish(cpu,0);
                }
        }
    }
    return 0;
}
static uint32_t hash_byte(uint32_t hash,uint8_t value) {
    return (hash^value)*16777619u;
}
static uint32_t hash_word(uint32_t hash,uint32_t value) {
    for(unsigned i=0;i<4;++i)hash=hash_byte(hash,(uint8_t)(value>>(i*8)));
    return hash;
}
uint32_t mmx4_coop_lifecycle_digest(uint32_t seed) {
    /* Hash fields explicitly, excluding ABI padding and host pointers.
     * A projected P1 inventory backup is authoritative until restoration. */
    const Inventory *in=&second_inventory;
    seed=hash_byte(seed,in->character);seed=hash_byte(seed,in->hp);
    for(unsigned i=0;i<16;++i)seed=hash_byte(seed,in->ammo[i]);
    seed=hash_byte(seed,in->weapon);
    seed=hash_word(seed,separate_etanks);
    seed=hash_word(seed,simultaneous_menus);
    seed=hash_word(seed,personal_active);seed=hash_word(seed,personal_initialized);
    seed=hash_word(seed,personal_eligible);
    seed=hash_word(seed,personal_modal_owner);
    for(unsigned seat=0;seat<2;++seat) {
        for(unsigned i=0;i<sizeof personal_menu[seat].native;++i)
            seed=hash_byte(seed,personal_menu[seat].native[i]);
        seed=hash_byte(seed,personal_menu[seat].original_weapon);
    }
    if(separate_etanks)for(unsigned i=0;i<2;++i)seed=hash_byte(seed,in->etanks[i]);
    seed=hash_word(seed,inventory_projected);
    if(inventory_projected) {
        in=&saved_inventory;
        seed=hash_byte(seed,in->character);seed=hash_byte(seed,in->hp);
        for(unsigned i=0;i<16;++i)seed=hash_byte(seed,in->ammo[i]);
        seed=hash_byte(seed,in->weapon);
        if(separate_etanks)for(unsigned i=0;i<2;++i)seed=hash_byte(seed,in->etanks[i]);
    }
    seed=hash_word(seed,inventory_initialized);seed=hash_word(seed,stage_known);
    seed=hash_word(seed,last_stage);seed=hash_word(seed,last_section);
    seed=hash_word(seed,last_lives);seed=hash_word(seed,departed);
    seed=hash_word(seed,paired_victory);
    seed=hash_word(seed,select_ticks);seed=hash_word(seed,select_release);
    seed=hash_word(seed,rejoin_pending);seed=hash_word(seed,full_stage_load);
    seed=hash_word(seed,reward_committed);
    seed=hash_word(seed,first_before_clear_valid);
    if(first_before_clear_valid)seed=hash_word(seed,first_before_clear_dead);
    seed=hash_word(seed,select_previous);seed=hash_byte(seed,departure_active);
    seed=hash_byte(seed,departure_visible);seed=hash_word(seed,menu_active);
    seed=hash_word(seed,menu_owner);
    seed=hash_word(seed,menu_previous[0]);seed=hash_word(seed,menu_previous[1]);
    for(unsigned i=0;i<3;++i)seed=hash_word(seed,story_pad[i]);
    seed=hash_word(seed,carry_resources);seed=hash_word(seed,carry_dead);
    seed=hash_word(seed,carry_refill);
    seed=hash_word(seed,carry_first_dead);
    seed=hash_word(seed,carry_departure);
    seed=hash_word(seed,script_active);seed=hash_word(seed,script_owner);
    seed=hash_word(seed,script_serial);
    seed=hash_word(seed,warp_pending);seed=hash_word(seed,warp_phase);
    seed=hash_word(seed,warp_owner);seed=hash_byte(seed,warp_active);
    seed=hash_byte(seed,warp_visible);seed=hash_byte(seed,warp_vehicle_active);
    seed=hash_byte(seed,warp_vehicle_visible);seed=hash_word(seed,warp_origin_x);
    seed=hash_word(seed,warp_origin_y);seed=hash_word(seed,warp_guard);
    seed=hash_word(seed,warp_unlocked_ticks);
    seed=hash_word(seed,initial_arrival);
    seed=hash_word(seed,direct_initial_spawn);
    seed=hash_word(seed,flip_guard);
    seed=hash_byte(seed,departure_vehicle_active);seed=hash_byte(seed,departure_vehicle_visible);
    for(unsigned i=0;i<PICKUP_CAPACITY;++i) {
        seed=hash_word(seed,pickups[i].actor);seed=hash_byte(seed,pickups[i].owner);
    }
    for(unsigned i=0;i<sizeof scenes/sizeof scenes[0];++i) {
        seed=hash_word(seed,scenes[i].actor);seed=hash_byte(seed,scenes[i].owner);seed=hash_byte(seed,scenes[i].recovered);
        seed=hash_word(seed,scenes[i].serial);
    }
    seed=hash_word(seed,both_door.actor);seed=hash_word(seed,both_door.serial);
    seed=hash_word(seed,both_door.old_min);seed=hash_word(seed,both_door.target_min);
    seed=hash_word(seed,both_door.right);seed=hash_byte(seed,both_door.phase);
    seed=hash_byte(seed,both_door.owner);seed=hash_byte(seed,both_door.stage);
    seed=hash_byte(seed,both_door.section);
    for(unsigned i=0;i<4;++i)seed=hash_word(seed,both_door.bounds[i]);
    for(unsigned i=0;i<32;++i) {
        seed=hash_byte(seed,surface_valid[i]);
        for(unsigned j=0;j<sizeof surface_second[i];++j)seed=hash_byte(seed,surface_second[i][j]);
    }
    return seed;
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_lifecycle) {
    psx_mod_register_function_filter_plugin("mmx4.coop",0x800D9C84u,train_midboss);
    psx_mod_register_function_entry_plugin(ID,0x8001FBB8u,fresh_game);
    psx_mod_register_function_entry_plugin(ID,0x8001FBE0u,full_stage);
    psx_mod_register_function_entry_plugin(ID,0x8002E420u,stage_selection);
    psx_mod_register_function_entry_plugin(ID,0x8001FC20u,stage_initialization);
    psx_mod_register_function_entry_plugin(ID,0x800358A4u,direct_spawn);
    psx_mod_register_function_filter_plugin(ID,0x8001FF8Cu,stage_normal);
    psx_mod_register_function_filter_plugin(ID,0x8002FCACu,pause_menu);
    psx_mod_register_function_filter_plugin(ID,0x80023D90u,personal_menu_draw);
    psx_mod_register_function_filter_plugin(ID,0x80031064u,personal_dead_commit);
    psx_mod_register_function_filter_plugin(ID,0x800129F0u,personal_shared_effect);
    psx_mod_register_function_filter_plugin(ID,0x800129A4u,personal_shared_effect);
    psx_mod_register_function_filter_plugin(ID,0x80021158u,menu_world);
    psx_mod_register_function_filter_plugin(ID,0x80023D68u,menu_world);
    psx_mod_register_function_filter_plugin(ID,0x800BF76Cu,pickup_init);
    psx_mod_register_function_filter_plugin(ID,0x800C00BCu,pickup_collect);
    psx_mod_register_function_filter_plugin(ID,0x800BF730u,pickup_update);
    psx_mod_register_function_filter_plugin(ID,0x8001FA24u,reward);
    psx_mod_register_function_filter_plugin(ID,0x8001C07Cu,campaign_save);
    psx_mod_register_function_filter_plugin(ID,0x8001C3E8u,campaign_save);
    psx_mod_register_function_filter_plugin(ID,0x80035A6Cu,death_animation);
    psx_mod_register_function_filter_plugin(ID,0x80033BC8u,death_detection);
    psx_mod_register_function_filter_plugin(ID,0x8002166Cu,stage_scripts);
    psx_mod_register_function_filter_plugin(ID,0x800BCE48u,volcano_platform_start);
    psx_mod_register_function_filter_plugin(ID,0x800B60BCu,floor_controller);
    psx_mod_register_instruction_plugin(ID,0x800B9F74u,0x2442FD5Fu,frost_ice_activation);
    psx_mod_register_instruction_plugin(ID,0x800BA39Cu,0x28420C60u,frost_path_activation);
    psx_mod_register_function_filter_plugin(ID,0x8002C160u,frost_path_contact);
    psx_mod_register_instruction_plugin(ID,0x800DA44Cu,0x284218D0u,storm_survivor_exit);
    psx_mod_register_instruction_plugin(ID,0x80020018u,0x14620007u,completed_survivor_exit);
    psx_mod_register_instruction_plugin(ID,0x800BEEF4u,0x00041100u,intro_platform_activation);
    psx_mod_register_instruction_plugin(ID,0x800C0574u,0x00041100u,intro_survivor_crusher);
    psx_mod_register_function_filter_plugin(ID,0x800C36E8u,fan_latch);
    psx_mod_register_function_filter_plugin(ID,0x8002AD7Cu,shared_ride_ready);
    psx_mod_register_function_filter_plugin(ID,0x80021D20u,script_text);
    psx_mod_register_function_filter_plugin(ID,0x8002217Cu,shared_text_init);
    psx_mod_register_function_filter_plugin(ID,0x800C2BE0u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800C7164u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800C7970u,refight_portal_query);
    psx_mod_register_function_filter_plugin(ID,0x800C1994u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800C1E7Cu,waiting_door_query);
    psx_mod_register_function_filter_plugin(ID,0x800C1D90u,both_door_traversal);
    psx_mod_register_function_filter_plugin(ID,0x80052218u,paired_boss_admission);
    psx_mod_register_function_filter_plugin(ID,0x80085F8Cu,paired_boss_admission);
    psx_mod_register_function_filter_plugin(ID,0x80088D30u,paired_boss_admission);
    psx_mod_register_instruction_plugin(ID,0x800D0580u,0x14620006u,paired_intro_camera);
    psx_mod_register_instruction_plugin(ID,0x80081250u,0x14620005u,paired_iris_camera);
    psx_mod_register_function_filter_plugin(ID,0x8002D7E4u,both_door_map);
    psx_mod_register_function_filter_plugin(ID,0x8002D724u,both_door_map);
    psx_mod_register_function_filter_plugin(ID,0x800BD654u,scene_actor);
    /* Destructible section exits (Jungle waterfall): bind the outgoing walk
     * and its completion query to one body. Otherwise C165C redirects the
     * command to a surviving P2 but C169C waits forever for dead P1's X.
     * Keep C1390's intact obstacle/contact update outside this projection. */
    psx_mod_register_function_filter_plugin(ID,0x800C165Cu,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800C169Cu,scene_actor);
    /* Final Weapon's pre-Sigma NPC waits at D2420 for PLAYER.x>=2321.
     * Keep its complete D2A38 controller (camera wait, text and release)
     * with the surviving entrant; redirecting only 36AE4 cannot admit P2
     * while the native predicate still sees P1's corpse at the arrival. */
    psx_mod_register_function_filter_plugin(ID,0x800D2A38u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800CE894u,cyber_assessment);
    /* Act1 Dragoon first issues action15, then waits for PLAYER+C0=FF.
     * A P2-led door can withdraw P1 before this actor runs. Keep its native
     * command and subsequent completion reads on the committed scene owner;
     * redirecting only the command leaves D05B0 waiting on absent P1 forever. */
    psx_mod_register_function_filter_plugin(ID,0x800D0374u,scene_actor);
    /* Intro rescue actors live in the small sprite pool, outside the enemy
     * and stage-script projections. Their initialization and camera wait
     * must use the same body that owns the boss victory. Native D12C0 sets
     * the scroll limit to PLAYER.x-48; D14F8 waits for that exact value.
     * Reading a dead/lagging P1 puts the limit behind the arena and stalls
     * the surviving P2's scene indefinitely. Keep story identity canonical. */
    psx_mod_register_function_filter_plugin(ID,0x800D1248u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800D1284u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800C4EC8u,cyber_flip_query);
    psx_mod_register_function_filter_plugin(ID,0x800C4F40u,cyber_flip_world);
    psx_mod_register_function_filter_plugin(ID,0x80036AE4u,script_command);
    psx_mod_register_function_filter_plugin(ID,0x80034F7Cu,partner_victory_pose);
    psx_mod_register_function_filter_plugin(ID,0x80036B18u,script_command);
    psx_mod_register_function_filter_plugin(ID,0x800311ECu,nonowner_controls);
    psx_mod_register_function_filter_plugin(ID,0x8002C614u,hidden_terrain);
    static const uint32_t capsules[]={0x800C62DCu,0x800C6B84u,0x800C6C2Cu,
        0x800C6CE4u,0x800C6EDCu};
    for(unsigned i=0;i<sizeof capsules/sizeof capsules[0];++i)
        psx_mod_register_function_filter_plugin(ID,capsules[i],x_capsule);
}
