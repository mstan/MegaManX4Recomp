/* SLUS-00561 native lifecycle and inventory ownership. See
 * docs/COOP_LIFECYCLE_MENU_EVIDENCE.md for original executable evidence.
 * Shared stage/story/tanks remain in guest RAM; only the second player's
 * character inventory and serialized menu/pickup ownership live here. */
#include "mmx4_coop_internal.h"
#include <string.h>

#define ID "mmx4.coop"
#define PLAYER MMX4_PLAYER
#define PLAY MMX4_PLAY
#define MENU 0x801754A0u
#define PAD 0x80166C08u
#define START 0x0800u
#define SELECT 0x0100u
#define PICKUP_CAPACITY 128u

typedef struct {
    uint8_t campaign, hp, max_hp, armor, upgrade, ammo[16], hearts, weapon;
} Inventory;
typedef struct { uint32_t actor; uint8_t owner; } PickupOwner;
typedef struct { uint32_t actor; uint8_t owner; } SceneOwner;
/* Exposes the combat module's independent previous-frame solid history. */
uint8_t mmx4_coop_solid_contact_bits(uint32_t actor,unsigned seat);
static Inventory second_inventory, saved_inventory;
static PickupOwner pickups[PICKUP_CAPACITY];
static SceneOwner scenes[32];
static uint16_t menu_previous[2], select_previous;
static unsigned inventory_initialized, inventory_projected;
static unsigned stage_known, last_stage, last_section, last_lives;
static unsigned carry_resources, carry_dead, carry_first_dead, carry_departure;
static unsigned departed, select_ticks, select_release, rejoin_pending;
static unsigned full_stage_load;
static unsigned first_before_clear_valid, first_before_clear_dead;
static uint8_t departure_active, departure_visible;
static unsigned menu_active, menu_owner, menu_projecting;
static unsigned normal_guard, menu_guard, world_guard, pickup_guard;
static unsigned collect_guard, init_guard, reward_guard, death_guard, capsule_guard;
static unsigned scene_guard, script_pool_guard, command_guard, control_guard;
static unsigned script_active, script_owner, script_serial, transport_pending;
static uint32_t transport_x[2],transport_y[2];

static void read_bytes(uint32_t address,uint8_t *out,unsigned count) {
    for(unsigned i=0;i<count;++i)out[i]=psx_mod_read_byte(address+i);
}
static void write_bytes(uint32_t address,const uint8_t *in,unsigned count) {
    for(unsigned i=0;i<count;++i)psx_mod_write_byte(address+i,in[i]);
}
static void inventory_read(Inventory *out) {
    out->campaign=psx_mod_read_byte(PLAY+0x43);
    out->hp=psx_mod_read_byte(PLAY+0x45);
    out->max_hp=psx_mod_read_byte(PLAY+0x46);
    out->armor=psx_mod_read_byte(PLAY+0x47);
    out->upgrade=psx_mod_read_byte(PLAY+0x48);
    read_bytes(PLAY+0x49,out->ammo,16);
    out->hearts=psx_mod_read_byte(PLAY+0x5A);
    out->weapon=psx_mod_read_byte(PLAY+0x60);
}
static void inventory_write(const Inventory *in) {
    psx_mod_write_byte(PLAY+0x43,in->campaign);
    psx_mod_write_byte(PLAY+0x45,in->hp);
    psx_mod_write_byte(PLAY+0x46,in->max_hp);
    psx_mod_write_byte(PLAY+0x47,in->armor);
    psx_mod_write_byte(PLAY+0x48,in->upgrade);
    write_bytes(PLAY+0x49,in->ammo,16);
    /* High acquisition byte contains the common E/W/EX tank ownership. */
    psx_mod_write_byte(PLAY+0x5A,in->hearts);
    psx_mod_write_byte(PLAY+0x60,in->weapon);
}
void mmx4_coop_lifecycle_project(void) {
    if(inventory_projected)return;
    inventory_read(&saved_inventory);
    if(!inventory_initialized || second_inventory.campaign!=(saved_inventory.campaign^1u)) {
        memset(&second_inventory,0,sizeof second_inventory);
        second_inventory.campaign=(uint8_t)(saved_inventory.campaign^1u);
        second_inventory.hp=second_inventory.max_hp=32;
        memset(second_inventory.ammo,48,sizeof second_inventory.ammo);
        inventory_initialized=1;
    }
    inventory_write(&second_inventory);inventory_projected=1;
}
void mmx4_coop_lifecycle_restore(void) {
    if(!inventory_projected)return;
    inventory_read(&second_inventory);
    inventory_write(&saved_inventory);inventory_projected=0;
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
    memset(&second_inventory,0,sizeof second_inventory);
    memset(&saved_inventory,0,sizeof saved_inventory);
    memset(pickups,0,sizeof pickups);
    memset(scenes,0,sizeof scenes);
    memset(menu_previous,0,sizeof menu_previous);
    inventory_initialized=inventory_projected=stage_known=0;
    carry_resources=carry_dead=carry_first_dead=carry_departure=0;
    departed=select_ticks=select_release=select_previous=rejoin_pending=0;
    full_stage_load=0;
    first_before_clear_valid=first_before_clear_dead=0;
    script_active=script_owner=script_serial=transport_pending=0;
    memset(transport_x,0,sizeof transport_x);memset(transport_y,0,sizeof transport_y);
    menu_active=menu_owner=menu_projecting=0;
}
static void full_stage(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!mmx4_coop_projected())full_stage_load=1;
}
static void stage_initialization(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(mmx4_coop_projected())return;
    /* Native 8001FC20 calls 8002A7D0/8002A728 to clear PLAYER before
     * 80035240 initializes it. Capture the prior survivor status here;
     * zeroed initialization storage is not evidence of a player death. */
    first_before_clear_dead=!(psx_mod_read_byte(PLAYER+0x5C)&0x7Fu) ||
        psx_mod_read_byte(PLAYER+4)>=2;
    first_before_clear_valid=1;
}

void mmx4_coop_lifecycle_reset(void) {
    uint8_t *body=mmx4_coop_second_body();
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    unsigned section=psx_mod_read_byte(PLAY+0x0D);
    unsigned lives=psx_mod_read_byte(PLAY+0x44);
    unsigned first_was_dead=first_before_clear_valid?first_before_clear_dead:
        (!(psx_mod_read_byte(PLAYER+0x5C)&0x7Fu) || psx_mod_read_byte(PLAYER+4)>=2);
    unsigned team_was_dead=first_was_dead &&
        (!body[0] || !(body[0x5C]&0x7Fu) || body[4]>=2);
    unsigned life_preserved=lives>=last_lives && !(last_lives==0 && lives==255);
    /* Invoked before the core clears its second body. A section transfer
     * preserves the partner's state. Changed stage or lost shared life is a
     * team respawn. Section identity comes from native PLAY, never host time. */
    carry_resources=stage_known && stage==last_stage && life_preserved &&
        !team_was_dead && (!full_stage_load || section!=last_section);
    carry_dead=carry_resources && (!(body[0x5C]&0x7Fu) || body[4]>=2);
    carry_first_dead=carry_resources && first_was_dead;
    first_before_clear_valid=0;
    carry_departure=carry_resources && departed;
    if(carry_resources && inventory_initialized) {
        second_inventory.hp=(uint8_t)(body[0x5C]&0x7Fu);
        memcpy(second_inventory.ammo,body+0xA8,16);
        second_inventory.weapon=body[0x93];
    }
    if(!carry_resources)departed=select_ticks=select_release=rejoin_pending=0;
    full_stage_load=0;
    last_stage=stage;last_section=section;last_lives=lives;stage_known=1;
    memset(pickups,0,sizeof pickups);
    memset(scenes,0,sizeof scenes);script_active=transport_pending=0;
    menu_active=0;menu_owner=0;
}

void mmx4_coop_lifecycle_enrolled(CPUState *cpu) {
    uint8_t *body=mmx4_coop_second_body();
    if(carry_resources) {
        body[0x5C]=body[0x5D]=body[0x5E]=second_inventory.hp;
        memcpy(body+0xA8,second_inventory.ammo,16);
        body[0x93]=second_inventory.weapon;
        if(carry_dead) {body[4]=3;body[5]=body[6]=body[3]=0;}
        if(carry_departure) {body[0]=body[3]=0;departed=1;}
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
    }
    carry_resources=carry_dead=carry_first_dead=carry_departure=0;
}

int mmx4_coop_lifecycle_can_tick(void) {
    const uint8_t *body=mmx4_coop_second_body();
    return !departed && !menu_active && psx_mod_read_byte(PLAY+1)!=2 &&
        body[0] && body[4]!=3;
}
static void apply_transport(CPUState *cpu) {
    if(mmx4_coop_projected())return;
    for(unsigned seat=0;seat<2;++seat)if(transport_pending&(1u<<seat)) {
        transport_pending&=~(1u<<seat);
        if(!mmx4_coop_alive(seat))continue;
        if(seat)mmx4_coop_enter_second();
        psx_mod_write_word(PLAYER+8,transport_x[seat]);
        psx_mod_write_word(PLAYER+12,transport_y[seat]);
        psx_mod_write_word(PLAYER+0x18,transport_x[seat]);
        psx_mod_write_word(PLAYER+0x1C,transport_y[seat]);
        for(unsigned at=0x20;at<0x30;at+=4)psx_mod_write_word(PLAYER+at,0);
        if(psx_mod_read_byte(PLAYER+0xC5)) {
            /* A living rider's own vehicle follows the same transition. */
            psx_mod_write_word(MMX4_VEHICLE+8,transport_x[seat]);
            psx_mod_write_word(MMX4_VEHICLE+12,transport_y[seat]);
            psx_mod_write_word(MMX4_VEHICLE+0x18,transport_x[seat]);
            psx_mod_write_word(MMX4_VEHICLE+0x1C,transport_y[seat]);
        }
        mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
        if(seat)mmx4_coop_leave_second();
    }
}
static void queue_partner_transport(unsigned owner) {
    unsigned partner=owner^1u;
    if(!mmx4_coop_alive(partner))return;
    transport_x[partner]=psx_mod_read_word(PLAYER+8);
    transport_y[partner]=psx_mod_read_word(PLAYER+12);
    transport_pending|=1u<<partner;
}
void mmx4_coop_lifecycle_tick(CPUState *cpu) {
    if(!mmx4_coop_ready() || mmx4_coop_projected())return;
    apply_transport(cpu);
    uint8_t *body=mmx4_coop_second_body();
    body[0xB9]|=psx_mod_read_byte(PLAY+0x59);
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    if(stage_known && stage!=last_stage) {
        last_stage=stage;last_section=psx_mod_read_byte(PLAY+0x0D);
    }
    last_lives=psx_mod_read_byte(PLAY+0x44);
    uint16_t input=mmx4_coop_input(1);
    if(departed) {
        if(!(input&SELECT))select_release=0;
        else if(!select_release && !(select_previous&SELECT))rejoin_pending=1;
        /* Wait for an actual living grounded native body. Joining at exactly
         * the same floor coordinate avoids guessing about pits to either
         * side; independent movement separates the bodies afterward. */
        if(rejoin_pending && mmx4_coop_alive(0) &&
           (psx_mod_read_byte(PLAYER+0x89)&8u) &&
           !psx_mod_read_byte(PLAYER+0x79) && !psx_mod_read_byte(PLAYER+0xC5)) {
            uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
            body[0]=departure_active;body[3]=departure_visible;
            mmx4_coop_enter_second();
            psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
            psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
            for(unsigned at=0x20;at<0x30;at+=4)psx_mod_write_word(PLAYER+at,0);
            psx_mod_write_byte(PLAYER+0x67,0);
            mmx4_coop_call(cpu,0x80035EA4u,PLAYER,0);
            mmx4_coop_call(cpu,0x8002C614u,PLAYER,0);
            mmx4_coop_leave_second();
            departed=0;select_ticks=0;select_release=1;rejoin_pending=0;
            psx_mod_counter_add("mmx4.coop.rejoins",1);
        }
    }else if((input&SELECT) && !select_release && mmx4_coop_alive(0) &&
             mmx4_coop_alive(1) && !body[0xC5]) {
        if(++select_ticks>=90) {
            departure_active=body[0];departure_visible=body[3];
            body[0]=body[3]=0;departed=1;select_release=1;select_ticks=0;
            psx_mod_counter_add("mmx4.coop.departures",1);
        }
    }else {select_ticks=0;if(!(input&SELECT))select_release=0;}
    select_previous=input;
}

/* Wrapper guards permit exactly one native call and preserve CPU registers
 * through the core's call service. Only the outer filter completes the entry. */
static int stage_normal(CPUState *cpu,uint32_t address) {
    if(normal_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    apply_transport(cpu);
    uint16_t input[2]={mmx4_coop_input(0),mmx4_coop_input(1)};
    uint16_t edge[2]={(uint16_t)(input[0]&~menu_previous[0]),
                      (uint16_t)(input[1]&~menu_previous[1])};
    memcpy(menu_previous,input,sizeof input);
    int owner=-1;
    if(mmx4_coop_alive(0) && (edge[0]&START))owner=0;
    else if(mmx4_coop_alive(1) && (edge[1]&START))owner=1;
    if(script_active || psx_mod_read_byte(PLAY+0x1C) || psx_mod_read_byte(PLAY+0x10) ||
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
    if(psx_mod_read_byte(PLAYER+4)==3 && mmx4_coop_alive(1)) {
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
        psx_mod_counter_add(owner?"mmx4.coop.p2-menu":"mmx4.coop.p1-menu",1);
    }
    return mmx4_coop_finish(cpu,result);
}

static int pause_menu(CPUState *cpu,uint32_t address) {
    if(menu_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    if(!menu_active) {menu_active=1;menu_owner=0;}
    uint8_t pad[6];read_bytes(PAD,pad,6);
    uint16_t input=mmx4_coop_input(menu_owner);
    uint16_t edge=(uint16_t)(input&~menu_previous[menu_owner]);
    uint16_t old=menu_previous[menu_owner];
    menu_previous[0]=mmx4_coop_input(0);menu_previous[1]=mmx4_coop_input(1);
    psx_mod_write_half(PAD,input);psx_mod_write_half(PAD+2,old);
    psx_mod_write_half(PAD+4,edge);
    mmx4_coop_menu_assets(cpu,menu_owner);
    if(menu_owner) {mmx4_coop_enter_second();menu_projecting=1;}
    menu_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    menu_guard=0;
    if(menu_owner) {menu_projecting=0;mmx4_coop_leave_second();}
    write_bytes(PAD,pad,6);
    if(psx_mod_read_byte(PLAY+1)!=2 || psx_mod_read_byte(PLAY)!=6)menu_active=0;
    return mmx4_coop_finish(cpu,result);
}
static int menu_world(CPUState *cpu,uint32_t address) {
    if(world_guard || !menu_projecting || !mmx4_coop_projected())return 0;
    /* Native exit-pause runs the world once. Restore P1/campaign before that
     * shared pass and resume P2's weapon/menu context after it returns. */
    mmx4_coop_leave_second();world_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    world_guard=0;mmx4_coop_enter_second();
    return mmx4_coop_finish(cpu,result);
}

static int pickup_init(CPUState *cpu,uint32_t address) {
    if(init_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];pickup_forget(actor);
    uint8_t hearts=psx_mod_read_byte(PLAY+0x5A);
    /* A physical Heart Tank is consumed once across the shared world, while
     * maximum HP belongs only to its collector. */
    psx_mod_write_byte(PLAY+0x5A,(uint8_t)(hearts|second_inventory.hearts));
    init_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    init_guard=0;psx_mod_write_byte(PLAY+0x5A,hearts);
    return mmx4_coop_finish(cpu,result);
}
static int pickup_collect(CPUState *cpu,uint32_t address) {
    if(collect_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];
    uint8_t state=psx_mod_read_byte(actor+4);
    collect_guard=1;
    uint32_t result=0;
    if(mmx4_coop_alive(0))
        result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    if(psx_mod_read_byte(actor+4)==state && mmx4_coop_alive(1)) {
        /* Reserve ownership before making the second native call: a delayed
         * health pickup remains active and must heal this same body later. */
        PickupOwner *record=pickup_record(actor,1);
        if(record) {
            mmx4_coop_enter_second();
            result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
            mmx4_coop_leave_second();
            if(psx_mod_read_byte(actor+4)!=state) {
                record->owner=1;psx_mod_counter_add("mmx4.coop.p2-pickups",1);
            }else pickup_forget(actor);
        }else psx_mod_counter_add("mmx4.coop.pickup-owner-full",1);
    }
    collect_guard=0;return mmx4_coop_finish(cpu,result);
}
static int pickup_update(CPUState *cpu,uint32_t address) {
    if(pickup_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];PickupOwner *record=pickup_record(actor,0);
    if(!record || !record->owner)return 0;
    unsigned state=psx_mod_read_byte(actor+4);
    if(state!=2) {pickup_forget(actor);return 0;}
    if(!mmx4_coop_alive(1)) {
        /* A scripted death must never be undone by a pending healing actor. */
        psx_mod_write_byte(actor+4,3);pickup_forget(actor);
        mmx4_coop_call(cpu,0x800C03BCu,0,0);
        return mmx4_coop_finish(cpu,0);
    }
    mmx4_coop_enter_second();pickup_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    pickup_guard=0;mmx4_coop_leave_second();
    if(psx_mod_read_byte(actor+4)!=2)pickup_forget(actor);
    return mmx4_coop_finish(cpu,result);
}
static int reward(CPUState *cpu,uint32_t address) {
    if(reward_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    reward_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    reward_guard=0;
    mmx4_coop_second_body()[0xB9]|=psx_mod_read_byte(PLAY+0x59);
    return mmx4_coop_finish(cpu,result);
}
static int death_animation(CPUState *cpu,uint32_t address) {
    if(death_guard || !mmx4_coop_ready())return 0;
    unsigned other=mmx4_coop_projected()?0u:1u;
    if(!mmx4_coop_alive(other))return 0;
    uint8_t gates[9];read_bytes(PLAY+0x12,gates,9);
    death_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    death_guard=0;write_bytes(PLAY+0x12,gates,9);
    psx_mod_write_byte(PLAY+0x1C,(uint8_t)(script_active!=0));
    return mmx4_coop_finish(cpu,result);
}

static int x_capsule(CPUState *cpu,uint32_t address) {
    if(capsule_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];
    /* Every capsule component reads the canonical X body, including child
     * upgrade animation 800C6EDC. In Zero's campaign all those reads belong
     * to X/P2. A dead X cannot begin or continue capsule acquisition. */
    unsigned x_seat=psx_mod_read_byte(PLAYER+2)==0?0u:1u;
    if(!mmx4_coop_alive(x_seat) &&
       !(address==0x800C62DCu && psx_mod_read_byte(actor+4)==0))
        return mmx4_coop_finish(cpu,0);
    if(!x_seat)return 0;
    mmx4_coop_enter_second();capsule_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    capsule_guard=0;mmx4_coop_leave_second();
    return mmx4_coop_finish(cpu,result);
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
    uint8_t campaign=psx_mod_read_byte(PLAY+0x43);
    mmx4_coop_enter_second();
    uint8_t character=psx_mod_read_byte(PLAY+0x43);
    psx_mod_write_byte(PLAY+0x43,campaign);
    uint32_t result=mmx4_coop_call(cpu,address,actor,arg);
    /* Projected inventory reads on leave must see the counterpart identity,
     * even though shared story selection remained canonical throughout. */
    psx_mod_write_byte(PLAY+0x43,character);
    mmx4_coop_leave_second();return result;
}
static int stage_scripts(CPUState *cpu,uint32_t address) {
    if(script_pool_guard || !mmx4_coop_ready() || mmx4_coop_projected() ||
       mmx4_coop_alive(0) || !mmx4_coop_alive(1))return 0;
    /* Original 8002166C updates exactly 32 native stage/script actors from
     * 80142F98, stride 0x30. It never runs a player, projectile, or enemy
     * world loop. The surviving body's progression triggers run once. */
    script_pool_guard=1;
    uint32_t result=call_story_as_second(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    script_pool_guard=0;return mmx4_coop_finish(cpu,result);
}
static int scene_actor(CPUState *cpu,uint32_t address) {
    if(scene_guard || !mmx4_coop_ready() || mmx4_coop_projected())return 0;
    uint32_t actor=cpu->gpr[4];unsigned owner=0;
    SceneOwner *record=scene_record(actor,0);
    if(psx_mod_read_byte(actor+4)==0 && record) {
        memset(record,0,sizeof *record);record=NULL;
    }
    if(record)owner=record->owner;
    else if(!mmx4_coop_alive(0) && mmx4_coop_alive(1))owner=1;
    else if(address==0x800C2BE0u && mmx4_coop_alive(1) &&
            !(mmx4_coop_solid_contact_bits(actor,0)&8u) &&
             (mmx4_coop_solid_contact_bits(actor,1)&8u))owner=1;
    else if(address==0x800BD654u && mmx4_coop_alive(1) &&
            (int32_t)psx_mod_read_word(PLAYER+8)<
            (int32_t)((uint32_t)mmx4_coop_second_body()[8]|
                (uint32_t)mmx4_coop_second_body()[9]<<8|
                (uint32_t)mmx4_coop_second_body()[10]<<16|
                (uint32_t)mmx4_coop_second_body()[11]<<24))owner=1;
    if(!mmx4_coop_alive(owner))return mmx4_coop_finish(cpu,0);
    unsigned serial=script_serial;
    scene_guard=1;uint32_t result;
    if(owner) {
        uint8_t contact[4];static const unsigned at[]={0x72,0x74,0x76,0x78};
        if(address==0x800C2BE0u) {
            for(unsigned i=0;i<4;++i)contact[i]=psx_mod_read_byte(actor+at[i]);
            psx_mod_write_byte(actor+0x72,mmx4_coop_solid_contact_bits(actor,1));
        }
        result=call_story_as_second(cpu,address,actor,cpu->gpr[5]);
        if(address==0x800C2BE0u)
            for(unsigned i=0;i<4;++i)psx_mod_write_byte(actor+at[i],contact[i]);
    }else result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    scene_guard=0;
    if(serial!=script_serial) {
        record=scene_record(actor,1);
        if(record)record->owner=(uint8_t)owner;
    }
    if(record && (!psx_mod_read_byte(actor) ||
        (!script_active && psx_mod_read_byte(actor+4)>=3)))
        memset(record,0,sizeof *record);
    return mmx4_coop_finish(cpu,result);
}
static int script_command(CPUState *cpu,uint32_t address) {
    if(command_guard || !mmx4_coop_ready())return 0;
    unsigned owner=mmx4_coop_projected()?1u:0u;
    unsigned redirect=!owner && !mmx4_coop_alive(0) && mmx4_coop_alive(1);
    if(redirect) {mmx4_coop_enter_second();owner=1;}
    command_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    command_guard=0;
    if(address==0x80036AE4u) {
        script_active=1;script_owner=owner;++script_serial;
    }else script_active=0;
    queue_partner_transport(owner);
    if(redirect)mmx4_coop_leave_second();
    return mmx4_coop_finish(cpu,result);
}
static int nonowner_controls(CPUState *cpu,uint32_t address) {
    if(control_guard || !mmx4_coop_ready() || !script_active)return 0;
    unsigned seat=mmx4_coop_projected()?1u:0u;
    if(seat==script_owner || !mmx4_coop_alive(seat))return 0;
    /* The scripted owner retains native auto-walk/warp. Its living partner
     * receives neutral controls until the common transition completes. */
    psx_mod_write_half(PLAYER+0x7C,0);psx_mod_write_half(PLAYER+0x7E,0);
    psx_mod_write_half(PLAYER+0x80,0);
    control_guard=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    control_guard=0;return mmx4_coop_finish(cpu,result);
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
    seed=hash_byte(seed,in->campaign);seed=hash_byte(seed,in->hp);
    seed=hash_byte(seed,in->max_hp);seed=hash_byte(seed,in->armor);
    seed=hash_byte(seed,in->upgrade);
    for(unsigned i=0;i<16;++i)seed=hash_byte(seed,in->ammo[i]);
    seed=hash_byte(seed,in->hearts);seed=hash_byte(seed,in->weapon);
    seed=hash_word(seed,inventory_projected);
    if(inventory_projected) {
        in=&saved_inventory;
        seed=hash_byte(seed,in->campaign);seed=hash_byte(seed,in->hp);
        seed=hash_byte(seed,in->max_hp);seed=hash_byte(seed,in->armor);
        seed=hash_byte(seed,in->upgrade);
        for(unsigned i=0;i<16;++i)seed=hash_byte(seed,in->ammo[i]);
        seed=hash_byte(seed,in->hearts);seed=hash_byte(seed,in->weapon);
    }
    seed=hash_word(seed,inventory_initialized);seed=hash_word(seed,stage_known);
    seed=hash_word(seed,last_stage);seed=hash_word(seed,last_section);
    seed=hash_word(seed,last_lives);seed=hash_word(seed,departed);
    seed=hash_word(seed,select_ticks);seed=hash_word(seed,select_release);
    seed=hash_word(seed,rejoin_pending);seed=hash_word(seed,full_stage_load);
    seed=hash_word(seed,first_before_clear_valid);
    if(first_before_clear_valid)seed=hash_word(seed,first_before_clear_dead);
    seed=hash_word(seed,select_previous);seed=hash_byte(seed,departure_active);
    seed=hash_byte(seed,departure_visible);seed=hash_word(seed,menu_active);
    seed=hash_word(seed,menu_owner);
    seed=hash_word(seed,menu_previous[0]);seed=hash_word(seed,menu_previous[1]);
    seed=hash_word(seed,carry_resources);seed=hash_word(seed,carry_dead);
    seed=hash_word(seed,carry_first_dead);
    seed=hash_word(seed,carry_departure);
    seed=hash_word(seed,script_active);seed=hash_word(seed,script_owner);
    seed=hash_word(seed,script_serial);seed=hash_word(seed,transport_pending);
    for(unsigned i=0;i<2;++i) {
        seed=hash_word(seed,transport_x[i]);seed=hash_word(seed,transport_y[i]);
    }
    for(unsigned i=0;i<PICKUP_CAPACITY;++i) {
        seed=hash_word(seed,pickups[i].actor);seed=hash_byte(seed,pickups[i].owner);
    }
    for(unsigned i=0;i<sizeof scenes/sizeof scenes[0];++i) {
        seed=hash_word(seed,scenes[i].actor);seed=hash_byte(seed,scenes[i].owner);
    }
    return seed;
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_lifecycle) {
    psx_mod_register_function_entry_plugin(ID,0x8001FBB8u,fresh_game);
    psx_mod_register_function_entry_plugin(ID,0x8001FBE0u,full_stage);
    psx_mod_register_function_entry_plugin(ID,0x8001FC20u,stage_initialization);
    psx_mod_register_function_filter_plugin(ID,0x8001FF8Cu,stage_normal);
    psx_mod_register_function_filter_plugin(ID,0x8002FCACu,pause_menu);
    psx_mod_register_function_filter_plugin(ID,0x80021158u,menu_world);
    psx_mod_register_function_filter_plugin(ID,0x800BF76Cu,pickup_init);
    psx_mod_register_function_filter_plugin(ID,0x800C00BCu,pickup_collect);
    psx_mod_register_function_filter_plugin(ID,0x800BF730u,pickup_update);
    psx_mod_register_function_filter_plugin(ID,0x8001FA24u,reward);
    psx_mod_register_function_filter_plugin(ID,0x80035A6Cu,death_animation);
    psx_mod_register_function_filter_plugin(ID,0x8002166Cu,stage_scripts);
    psx_mod_register_function_filter_plugin(ID,0x800C2BE0u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x800BD654u,scene_actor);
    psx_mod_register_function_filter_plugin(ID,0x80036AE4u,script_command);
    psx_mod_register_function_filter_plugin(ID,0x80036B18u,script_command);
    psx_mod_register_function_filter_plugin(ID,0x800311ECu,nonowner_controls);
    static const uint32_t capsules[]={0x800C62DCu,0x800C6B84u,0x800C6C2Cu,
        0x800C6CE4u,0x800C6EDCu};
    for(unsigned i=0;i<sizeof capsules/sizeof capsules[0];++i)
        psx_mod_register_function_filter_plugin(ID,capsules[i],x_capsule);
}
