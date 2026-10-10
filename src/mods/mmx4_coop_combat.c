/* Original SLUS-00561 combat and moving-solid boundaries. Shared enemies,
 * projectiles and scripts retain their original once-per-world-frame update.
 * See docs/COOP_COMBAT_VEHICLE_EVIDENCE.md for original instruction evidence. */
#include "mmx4_coop_internal.h"

#include <stddef.h>
#include <string.h>

#define COOP_ID "mmx4.coop"
#define DOUBLE_BODY 0x80175D58u
#define CONTACT 0x8002D9BCu
#define ENEMY_HIT 0x8002DD04u
#define SOLID_CONTACT 0x8002E184u
#define SOLID_CAPACITY 256u
#define EFFECT_ALLOCATE 0x8002AD3Cu
#define EFFECT_POOL 0x8013E510u
#define EFFECT_STRIDE 0x70u
#define EFFECT_CAPACITY 32u
#define PARTICLE_ALLOCATE 0x8002AE50u
#define PARTICLE_INITIALIZE 0x800CAE38u
#define PARTICLE_POOL 0x80173CA0u
#define PARTICLE_STRIDE 0x60u
#define PARTICLE_CAPACITY 64u
#define ARMOR_UPDATE 0x8003D3F8u
#define ARMOR_MOUNT 0x8003E0D0u
#define ACTOR_DISPATCH 0x8F7FFF00u
/* Core accessors: first is the saved world context only while projected. */
uint8_t *mmx4_coop_second_vehicle(void);
uint8_t *mmx4_coop_first_vehicle(void);

typedef struct {
    uint32_t actor;
    uint8_t sides, carried, vehicle_sides, vehicle_carried;
} SolidContact;
typedef struct {
    uint32_t actor;
    uint8_t token[2];
} HitContact;

/* Keep records in ascending guest-address order, including in the digest.
 * Stage resets and original slot allocators invalidate previous contact. */
static SolidContact solids[SOLID_CAPACITY];
static unsigned solid_count;
static unsigned combat_call, solid_call, allocation_call, aim_call;
static unsigned effect_allocation_call;
static unsigned personal_effect_call;
static uint8_t second_effect_owner[EFFECT_CAPACITY];
/* The type-11 death effect defers its character assembly and position reads
 * until the shared pool pass. Keep the allocation-time origin through rejoin. */
static struct { uint32_t x,y; uint8_t owner,screen; } particles[PARTICLE_CAPACITY];
static unsigned particle_allocation_call,particle_init_call;
static unsigned armor_call, armor_owner, armor_skip_update;
static unsigned survivor_pool_call;
static unsigned actor_call,actor_suspended,actor_owner;
static uint32_t actor_function,actor_pending,actor_pool;
static uint8_t actor_campaign,pool_first_freeze;
static HitContact hits[SOLID_CAPACITY];
static unsigned hit_count;

static HitContact *hit_record(uint32_t actor,int create) {
    unsigned i=0;
    while (i<hit_count && hits[i].actor<actor) ++i;
    if (i<hit_count && hits[i].actor==actor) return &hits[i];
    if (!create) return NULL;
    if (hit_count==SOLID_CAPACITY) {
        psx_mod_counter_add("mmx4.coop.hit-context-overflow",1);
        return NULL;
    }
    memmove(hits+i+1,hits+i,(hit_count-i)*sizeof hits[0]);
    ++hit_count;
    memset(hits+i,0,sizeof hits[0]);
    hits[i].actor=actor;
    hits[i].token[0]=psx_mod_read_byte(actor+0x65);
    return &hits[i];
}
static void forget_hit(uint32_t actor) {
    HitContact *record=hit_record(actor,0);
    if (!record) return;
    size_t i=(size_t)(record-hits);
    --hit_count;
    memmove(hits+i,hits+i+1,(hit_count-i)*sizeof hits[0]);
    memset(hits+hit_count,0,sizeof hits[0]);
}

static void capture_vehicle(uint8_t *body) {
    for (unsigned i=0;i<0xB0;++i) body[i]=psx_mod_read_byte(MMX4_VEHICLE+i);
}
static void project_vehicle(const uint8_t *body) {
    for (unsigned i=0;i<0xB0;++i) psx_mod_write_byte(MMX4_VEHICLE+i,body[i]);
}

static uint32_t body_word(const uint8_t *body, unsigned offset) {
    return (uint32_t)body[offset] | (uint32_t)body[offset+1] << 8 |
        (uint32_t)body[offset+2] << 16 | (uint32_t)body[offset+3] << 24;
}

static int world_ready(void) {
    return mmx4_coop_ready() && !mmx4_coop_projected() &&
        psx_mod_read_byte(MMX4_PLAY)==6;
}

int mmx4_coop_combat_actor_context(void) {
    return actor_call && !actor_suspended && (!mmx4_coop_projected() || actor_owner==1);
}

int mmx4_coop_combat_canonical_call(CPUState *cpu,uint32_t address,
    PSXModFunctionFilterCallback callback) {
    if(!actor_call || !actor_owner || actor_suspended || !mmx4_coop_projected())
        return callback(cpu,address);
    actor_suspended=1;
    psx_mod_write_byte(MMX4_PLAY+0x43,psx_mod_read_byte(MMX4_PLAYER+2));
    mmx4_coop_leave_second();
    uint32_t result;
    if(callback(cpu,address))result=cpu->gpr[2];
    else result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    mmx4_coop_enter_second();
    psx_mod_write_byte(MMX4_PLAY+0x43,actor_campaign);
    actor_suspended=0;
    return mmx4_coop_finish(cpu,result);
}

static int accepted_hit_canonical(CPUState *cpu, uint32_t address) {
    if (combat_call || !world_ready()) return 0;
    combat_call=1;
    uint32_t actor=cpu->gpr[4], arg=cpu->gpr[5];
    uint32_t result=0;
    HitContact *history=address==ENEMY_HIT?hit_record(actor,1):NULL;
    uint8_t first_token=history?history->token[0]:0;
    if (history) psx_mod_write_byte(actor+0x65,first_token);
    /* Native enemy pools skip collisions while PLAYER+BC is nonzero.
     * The shared pool runs for P1, so apply the same gate to each projected
     * seat. Otherwise a persistent Zero saber reasserts +BD every world pass
     * and the next player tick restarts the five-tick hit pause forever. */
    if (mmx4_coop_alive(0) && !psx_mod_read_byte(MMX4_PLAYER+0xBC)) {
        result=mmx4_coop_call(cpu,address,actor,arg);
        if (history) {
            first_token=psx_mod_read_byte(actor+0x65);
            history=hit_record(actor,0);
            if (history) history->token[0]=first_token;
        }
    }
    if (!result && mmx4_coop_alive(1) && !mmx4_coop_second_body()[0xBC]) {
        if (history) psx_mod_write_byte(actor+0x65,history->token[1]);
        mmx4_coop_enter_second();
        result=mmx4_coop_call(cpu,address,actor,arg);
        mmx4_coop_leave_second();
        if (history) {
            uint8_t second_token=psx_mod_read_byte(actor+0x65);
            history=hit_record(actor,0);
            if (history) history->token[1]=second_token;
            psx_mod_write_byte(actor+0x65,first_token);
        }
        if (result) psx_mod_counter_add(address==CONTACT?
            "mmx4.coop.p2-contact":"mmx4.coop.p2-enemy-hit",1);
    }
    combat_call=0;
    /* The original actor caller applies the selected result and advances its
     * state once. In particular, one consumable attack cannot hit both seats. */
    return mmx4_coop_finish(cpu,result);
}

static int accepted_hit(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,accepted_hit_canonical);
}

static SolidContact *solid_record(uint32_t actor, int create) {
    unsigned i=0;
    while (i<solid_count && solids[i].actor<actor) ++i;
    if (i<solid_count && solids[i].actor==actor) return &solids[i];
    if (!create) return NULL;
    if (solid_count==SOLID_CAPACITY) {
        psx_mod_counter_add("mmx4.coop.solid-context-overflow",1);
        return NULL;
    }
    memmove(solids+i+1,solids+i,(solid_count-i)*sizeof solids[0]);
    ++solid_count;
    memset(solids+i,0,sizeof solids[0]);
    solids[i].actor=actor;
    return &solids[i];
}

static void forget_solid(uint32_t actor) {
    SolidContact *record=solid_record(actor,0);
    if (!record) return;
    size_t i=(size_t)(record-solids);
    --solid_count;
    memmove(solids+i,solids+i+1,(solid_count-i)*sizeof solids[0]);
    memset(solids+solid_count,0,sizeof solids[0]);
}

uint8_t mmx4_coop_solid_contact_bits(uint32_t actor, unsigned seat) {
    if (!seat) return psx_mod_read_byte(actor+0x72);
    if (seat!=1) return 0;
    SolidContact *record=solid_record(actor,0);
    return record?record->sides:0;
}

static int allocate_actor(CPUState *cpu, uint32_t address) {
    /* A private player pass can allocate shared effects/actors too. Unlike
     * collision hooks this wrapper never starts another projection, so it is
     * safe while projected and must invalidate reused slots in either seat. */
    if (allocation_call || !mmx4_coop_ready() ||
        psx_mod_read_byte(MMX4_PLAY)!=6) return 0;
    allocation_call=1;
    uint32_t actor=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    allocation_call=0;
    if (actor) {forget_solid(actor);forget_hit(actor);}
    return mmx4_coop_finish(cpu,actor);
}

static int allocate_effect(CPUState *cpu, uint32_t address) {
    if (effect_allocation_call) return 0;
    effect_allocation_call=1;
    uint32_t actor=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    effect_allocation_call=0;
    if (actor) forget_hit(actor);
    if (actor>=EFFECT_POOL && actor<EFFECT_POOL+EFFECT_CAPACITY*EFFECT_STRIDE &&
        !((actor-EFFECT_POOL)%EFFECT_STRIDE))
        second_effect_owner[(actor-EFFECT_POOL)/EFFECT_STRIDE]=
            (uint8_t)(mmx4_coop_projected()!=0);
    /* This must also run during enrollment before ready(), and clear an old
     * owner when a shared world/P1 allocator reuses the original slot. */
    return mmx4_coop_finish(cpu,actor);
}

static int personal_effect(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(personal_effect_call || !world_ready() || actor<EFFECT_POOL ||
       actor>=EFFECT_POOL+EFFECT_CAPACITY*EFFECT_STRIDE ||
       (actor-EFFECT_POOL)%EFFECT_STRIDE ||
       !second_effect_owner[(actor-EFFECT_POOL)/EFFECT_STRIDE])return 0;
    /* Type 7 follows PLAYER/Double. Type 2 is X's delayed charge release:
     * it uses the owner's assembly/pose, then allocates a native projectile
     * at 800AF0A4. Keep that allocation in the owner's private shot pool. */
    personal_effect_call=1;mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    mmx4_coop_leave_second();personal_effect_call=0;
    return mmx4_coop_finish(cpu,result);
}

void mmx4_coop_combat_clear_current_effects(void) {
    unsigned owner=(unsigned)(mmx4_coop_projected()!=0);
    for(unsigned i=0;i<EFFECT_CAPACITY;++i) {
        uint32_t actor=EFFECT_POOL+i*EFFECT_STRIDE;
        if(second_effect_owner[i]==owner && psx_mod_read_byte(actor) &&
           psx_mod_read_byte(actor+1)==2) {
            /* An outgoing player cannot leave a delayed charge emitter
             * behind to allocate another shot after its attacks were reset. */
            psx_mod_write_byte(actor,0);psx_mod_write_byte(actor+3,0);
            second_effect_owner[i]=0;
        }
    }
}

static int particle_slot(uint32_t actor) {
    if(actor<PARTICLE_POOL || actor>=PARTICLE_POOL+PARTICLE_CAPACITY*PARTICLE_STRIDE ||
       (actor-PARTICLE_POOL)%PARTICLE_STRIDE)return -1;
    return (int)((actor-PARTICLE_POOL)/PARTICLE_STRIDE);
}
static int allocate_particle(CPUState *cpu,uint32_t address) {
    if(particle_allocation_call)return 0;
    particle_allocation_call=1;
    uint32_t actor=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    particle_allocation_call=0;
    int slot=particle_slot(actor);
    if(slot>=0) {
        memset(&particles[slot],0,sizeof particles[slot]);
        if(mmx4_coop_projected()) {
            particles[slot].owner=1;
            particles[slot].x=psx_mod_read_word(MMX4_PLAYER+8);
            particles[slot].y=psx_mod_read_word(MMX4_PLAYER+12);
            particles[slot].screen=psx_mod_read_byte(MMX4_PLAYER+0x14);
        }
    }
    return mmx4_coop_finish(cpu,actor);
}
static int initialize_death_particle(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];int slot=particle_slot(actor);
    if(particle_init_call || !mmx4_coop_ready() || slot<0 || !particles[slot].owner ||
       psx_mod_read_byte(actor+1)!=0x11 || psx_mod_read_byte(actor+4))return 0;
    particle_init_call=1;
    int entered=!mmx4_coop_projected();
    if(entered)mmx4_coop_enter_second();
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8),y=psx_mod_read_word(MMX4_PLAYER+12);
    uint8_t screen=psx_mod_read_byte(MMX4_PLAYER+0x14);
    psx_mod_write_word(MMX4_PLAYER+8,particles[slot].x);
    psx_mod_write_word(MMX4_PLAYER+12,particles[slot].y);
    psx_mod_write_byte(MMX4_PLAYER+0x14,particles[slot].screen);
    /* The normal projection also supplies P2's private assembly/palette.
     * Do not require alive(): this callback belongs to an already dead seat. */
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_word(MMX4_PLAYER+8,x);psx_mod_write_word(MMX4_PLAYER+12,y);
    psx_mod_write_byte(MMX4_PLAYER+0x14,screen);
    if(entered)mmx4_coop_leave_second();
    particles[slot].owner=0;
    particle_init_call=0;
    return mmx4_coop_finish(cpu,result);
}

void mmx4_coop_combat_project_end(uint32_t vehicle_mirror) {
    if (!vehicle_mirror || !mmx4_coop_projected() || psx_mod_local_view_scope()) return;
    for (unsigned i=0;i<EFFECT_CAPACITY;++i) {
        if (!second_effect_owner[i]) continue;
        uint32_t actor=EFFECT_POOL+i*EFFECT_STRIDE;
        if (!psx_mod_read_byte(actor)) {
            second_effect_owner[i]=0;
            continue;
        }
        /* Root has refreshed the stable mirror from the owning vehicle before
         * this call. Proven vehicle creators store their following source at
         * +50. Other effect pointers, including hitboxes, stay untouched. */
        if (psx_mod_read_word(actor+0x50)==MMX4_VEHICLE)
            psx_mod_write_word(actor+0x50,vehicle_mirror);
    }
    /* Native dismount clears rider +C5 and Armor +97 bit40 (8003EF90-A8).
     * Return this same unoccupied actor to the world, preserving its HP and
     * position. Never overwrite another world vehicle or create a clone. */
    uint8_t *world=mmx4_coop_first_vehicle();
    if (armor_owner && !world[0] && psx_mod_read_byte(MMX4_VEHICLE) &&
        psx_mod_read_byte(MMX4_VEHICLE+1)==1 &&
        !psx_mod_read_byte(MMX4_PLAYER+0xC5) &&
        !(psx_mod_read_byte(MMX4_VEHICLE+0x97)&0x40)) {
        capture_vehicle(world);
        psx_mod_write_byte(MMX4_VEHICLE,0);
        psx_mod_write_byte(MMX4_VEHICLE+3,0);
        uint8_t *second=mmx4_coop_second_vehicle();
        second[0]=second[3]=0;
        for (unsigned i=0;i<EFFECT_CAPACITY;++i) {
            uint32_t actor=EFFECT_POOL+i*EFFECT_STRIDE;
            if (second_effect_owner[i] &&
                psx_mod_read_word(actor+0x50)==vehicle_mirror) {
                psx_mod_write_word(actor+0x50,MMX4_VEHICLE);
                second_effect_owner[i]=0;
            }
        }
        for (unsigned i=0;i<solid_count;++i) {
            psx_mod_write_byte(solids[i].actor+0x74,solids[i].vehicle_sides);
            psx_mod_write_byte(solids[i].actor+0x78,solids[i].vehicle_carried);
            solids[i].vehicle_sides=solids[i].vehicle_carried=0;
        }
        armor_owner=armor_skip_update=0;
    }
}

static int armor_world_update(CPUState *cpu,uint32_t address) {
    if (armor_call || !mmx4_coop_ready() || cpu->gpr[4]!=MMX4_VEHICLE) return 0;
    if (mmx4_coop_projected()) {
        /* The world advanced this exact actor before ownership transferred.
         * Its first private tick must not advance it a second time. */
        if (!armor_skip_update || psx_mod_read_byte(MMX4_VEHICLE+1)!=1) return 0;
        armor_skip_update=0;
        return mmx4_coop_finish(cpu,0);
    }
    if (!world_ready()) return 0;
    armor_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    uint8_t *second=mmx4_coop_second_vehicle();
    uint8_t *body=mmx4_coop_second_body();
    /* Mode9 is the original unoccupied mount routine (dispatch800F912C).
     * P1 has first choice through the world update above. */
    if (!armor_owner && mmx4_coop_alive(1) && body[4]==1 && !body[0xC5] &&
        !second[0] && !psx_mod_read_byte(MMX4_PLAYER+0xC5) &&
        psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+1)==1 &&
        psx_mod_read_byte(MMX4_VEHICLE+4)==1 && psx_mod_read_byte(MMX4_VEHICLE+5)==9 &&
        !(psx_mod_read_byte(MMX4_VEHICLE+0x97)&0x40)) {
        uint8_t candidate[0xB0],previous[0xB0];
        capture_vehicle(candidate);memcpy(previous,second,sizeof previous);
        mmx4_coop_enter_second();project_vehicle(candidate);
        /* Initialization (+6) already ran for P1. This retries only native
         * mount eligibility/overlap, never Armor AI, movement or terrain. */
        mmx4_coop_call(cpu,ARMOR_MOUNT,MMX4_VEHICLE,0);
        int mounted=psx_mod_read_byte(MMX4_PLAYER+0xC5)==1 &&
            psx_mod_read_byte(MMX4_VEHICLE+5)==0xA &&
            (psx_mod_read_byte(MMX4_VEHICLE+0x97)&0x40);
        if (!mounted) project_vehicle(previous);
        else {
            armor_owner=armor_skip_update=1;
            for (unsigned i=0;i<EFFECT_CAPACITY;++i)
                if (psx_mod_read_byte(EFFECT_POOL+i*EFFECT_STRIDE) &&
                    psx_mod_read_word(EFFECT_POOL+i*EFFECT_STRIDE+0x50)==MMX4_VEHICLE)
                    second_effect_owner[i]=1;
            for (unsigned i=0;i<solid_count;++i) {
                solids[i].vehicle_sides=psx_mod_read_byte(solids[i].actor+0x74);
                solids[i].vehicle_carried=psx_mod_read_byte(solids[i].actor+0x78);
            }
        }
        mmx4_coop_leave_second();
        if (mounted) {
            psx_mod_write_byte(MMX4_VEHICLE,0);
            psx_mod_write_byte(MMX4_VEHICLE+3,0);
            psx_mod_counter_add("mmx4.coop.p2-armor-mounted",1);
        }
    }
    armor_call=0;
    return mmx4_coop_finish(cpu,result);
}

static int solid_player_contact_canonical(CPUState *cpu, uint32_t address) {
    if (solid_call || !world_ready()) return 0;
    uint32_t actor=cpu->gpr[4], arg=cpu->gpr[5];
    /* Only original actor RAM is eligible; these fields are not present in
     * small effect actors. The caller supplies a native full solid actor. */
    if ((actor&3u) || actor<0x80010000u || actor>0x801FFF80u) return 0;
    solid_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,actor,arg);
    if (mmx4_coop_alive(1) && mmx4_coop_second_body()[4]==1 &&
        psx_mod_read_word(actor+0x68)) {
        SolidContact *record=solid_record(actor,1);
        if (record) {
            uint8_t sides=psx_mod_read_byte(actor+0x72);
            uint8_t carried=psx_mod_read_byte(actor+0x76);
            uint8_t vehicle_sides=psx_mod_read_byte(actor+0x74);
            uint8_t vehicle_carried=psx_mod_read_byte(actor+0x78);
            mmx4_coop_enter_second();
            /* Double already received its native world collision. A second
             * vehicle, when present, belongs to the projected second seat. */
            uint8_t double_active=psx_mod_read_byte(DOUBLE_BODY);
            psx_mod_write_byte(DOUBLE_BODY,0);
            psx_mod_write_byte(actor+0x72,record->sides);
            psx_mod_write_byte(actor+0x76,record->carried);
            psx_mod_write_byte(actor+0x74,record->vehicle_sides);
            psx_mod_write_byte(actor+0x78,record->vehicle_carried);
            mmx4_coop_call(cpu,address,actor,arg);
            record->sides=psx_mod_read_byte(actor+0x72);
            record->carried=psx_mod_read_byte(actor+0x76);
            record->vehicle_sides=psx_mod_read_byte(actor+0x74);
            record->vehicle_carried=psx_mod_read_byte(actor+0x78);
            psx_mod_write_byte(DOUBLE_BODY,double_active);
            mmx4_coop_leave_second();
            psx_mod_write_byte(actor+0x72,sides);
            psx_mod_write_byte(actor+0x76,carried);
            psx_mod_write_byte(actor+0x74,vehicle_sides);
            psx_mod_write_byte(actor+0x78,vehicle_carried);
            psx_mod_counter_add("mmx4.coop.p2-solid-contact",1);
        }
    } else forget_solid(actor);
    solid_call=0;
    return mmx4_coop_finish(cpu,result);
}

static int solid_player_contact(CPUState *cpu,uint32_t address) {
    return mmx4_coop_combat_canonical_call(cpu,address,solid_player_contact_canonical);
}

static uint64_t distance_squared(int16_t ax, int16_t ay,
                                 int16_t bx, int16_t by) {
    int64_t dx=(int64_t)ax-bx, dy=(int64_t)ay-by;
    return (uint64_t)(dx*dx+dy*dy);
}

static int aim_at_participant(CPUState *cpu, uint32_t address) {
    if (aim_call || actor_call || !world_ready() || !mmx4_coop_alive(1)) return 0;
    uint32_t actor=cpu->gpr[4];
    if ((actor&3u) || actor<0x80010000u || actor>0x801FFF80u) return 0;
    uint8_t *second=mmx4_coop_second_body();
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8);
    uint32_t y=psx_mod_read_word(MMX4_PLAYER+12);
    uint32_t second_x=body_word(second,8), second_y=body_word(second,12);
    int16_t ax=(int16_t)psx_mod_read_half(actor+0xA);
    int16_t ay=(int16_t)psx_mod_read_half(actor+0xE);
    uint64_t first_distance=distance_squared(ax,ay,(int16_t)(x>>16),
                                            (int16_t)(y>>16));
    uint64_t second_distance=distance_squared(ax,ay,(int16_t)(second_x>>16),
                                             (int16_t)(second_y>>16));
    /* Ties select P1; a fallen P1 cannot attract these aim routines. */
    if (mmx4_coop_alive(0) && first_distance<=second_distance) return 0;
    aim_call=1;
    psx_mod_write_word(MMX4_PLAYER+8,second_x);
    psx_mod_write_word(MMX4_PLAYER+12,second_y);
    /* Only three inspected routines are eligible. They write the enemy's
     * movement/state, not the player, and have no contact/combat calls. This
     * is one native update with a scoped aim target, never a second AI pass. */
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_word(MMX4_PLAYER+8,x);
    psx_mod_write_word(MMX4_PLAYER+12,y);
    aim_call=0;
    psx_mod_counter_add("mmx4.coop.p2-aim-target",1);
    return mmx4_coop_finish(cpu,result);
}

static void redirect_actor(CPUState *cpu,uint32_t address) {
    if(!survivor_pool_call || actor_call ||
       mmx4_coop_projected() || psx_mod_local_view_scope())return;
    uint32_t first=address==0x80021300u?0x8013BED0u:0x8013F328u;
    unsigned capacity=address==0x80021300u?48u:32u;
    uint32_t pool=address==0x80021300u?0x80021234u:0x8002144Cu;
    uint32_t actor=cpu->gpr[4],function=cpu->gpr[2];
    if(actor_pool!=pool || actor<first || actor>=first+capacity*0x9Cu ||
       (actor-first)%0x9Cu || (function&3u) || function<0x80010000u ||
       function>=0x8012F800u || actor_pending)return;
    actor_pending=actor;actor_function=function;
    cpu->gpr[2]=ACTOR_DISPATCH;
}

static void update_actor(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=actor_pending,function=actor_function;
    actor_pending=actor_function=0;
    if(!actor || !function || actor_call || !survivor_pool_call)return;
    actor_call=1;
    psx_mod_write_byte(MMX4_PLAYER+0xBC,pool_first_freeze);
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    int owner=mmx4_coop_lifecycle_script_owner();
    unsigned target=owner>=0?(unsigned)owner:mmx4_coop_view_nearest(actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    actor_owner=target;
    actor_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
    uint32_t result=0;
    if(actors[target].active) {
        if(target) {
            mmx4_coop_enter_second();
            psx_mod_write_byte(MMX4_PLAY+0x43,actor_campaign);
        }
        if(psx_mod_read_byte(MMX4_PLAYER+0xBC)) {
            if(psx_mod_read_byte(actor+3))result=mmx4_coop_call(cpu,0x8002B3C0u,actor,0);
        }else {
            result=mmx4_coop_call(cpu,function,actor,cpu->gpr[5]);
            psx_mod_counter_add(target?"mmx4.coop.p2-native-ai":"mmx4.coop.p1-native-ai",1);
        }
        if(target) {
            psx_mod_write_byte(MMX4_PLAY+0x43,psx_mod_read_byte(MMX4_PLAYER+2));
            mmx4_coop_leave_second();
        }
    }
    pool_first_freeze=psx_mod_read_byte(MMX4_PLAYER+0xBC);
    psx_mod_write_byte(MMX4_PLAYER+0xBC,0);
    actor_campaign=0;actor_call=actor_owner=0;
    mmx4_coop_finish(cpu,result);
}

static int survivor_actor_pool(CPUState *cpu,uint32_t address) {
    if (survivor_pool_call || !world_ready() || psx_mod_local_view_scope())return 0;
    /* Native activation/attack decisions read the singleton player even in
     * Unified. Run each shared actor once with the living nearest player,
     * or the current script owner during a door/boss handoff. */
    survivor_pool_call=1;actor_pool=address;
    pool_first_freeze=psx_mod_read_byte(MMX4_PLAYER+0xBC);
    psx_mod_write_byte(MMX4_PLAYER+0xBC,0);
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_byte(MMX4_PLAYER+0xBC,pool_first_freeze);
    survivor_pool_call=0;actor_pool=0;pool_first_freeze=0;
    return mmx4_coop_finish(cpu,result);
}

void mmx4_coop_combat_reset(void) {
    /* P2 enrollment invokes the native initializer within a projection. */
    if (mmx4_coop_projected()) return;
    memset(solids,0,sizeof solids);
    memset(second_effect_owner,0,sizeof second_effect_owner);
    memset(particles,0,sizeof particles);
    particle_allocation_call=particle_init_call=0;
    memset(hits,0,sizeof hits);
    hit_count=0;
    solid_count=0;
    combat_call=solid_call=allocation_call=aim_call=0;
    effect_allocation_call=personal_effect_call=0;
    armor_call=armor_owner=armor_skip_update=0;
    survivor_pool_call=0;
    actor_call=actor_suspended=actor_owner=0;
    actor_function=actor_pending=actor_pool=0;
    actor_campaign=pool_first_freeze=0;
}

static uint32_t digest_byte(uint32_t seed, uint8_t value) {
    return (seed^value)*16777619u;
}

static uint32_t digest_word(uint32_t seed, uint32_t value) {
    for (unsigned i=0;i<4;++i) seed=digest_byte(seed,(uint8_t)(value>>(8*i)));
    return seed;
}

uint32_t mmx4_coop_combat_digest(uint32_t seed) {
    seed=digest_word(seed,solid_count);
    for (unsigned i=0;i<solid_count;++i) {
        seed=digest_word(seed,solids[i].actor);
        seed=digest_byte(seed,solids[i].sides);
        seed=digest_byte(seed,solids[i].carried);
        seed=digest_byte(seed,solids[i].vehicle_sides);
        seed=digest_byte(seed,solids[i].vehicle_carried);
    }
    for (unsigned i=0;i<EFFECT_CAPACITY;++i)
        seed=digest_byte(seed,second_effect_owner[i]);
    for(unsigned i=0;i<PARTICLE_CAPACITY;++i) {
        seed=digest_byte(seed,particles[i].owner);
        if(particles[i].owner) {
            seed=digest_word(seed,particles[i].x);seed=digest_word(seed,particles[i].y);
            seed=digest_byte(seed,particles[i].screen);
        }
    }
    seed=digest_byte(seed,(uint8_t)armor_owner);
    seed=digest_byte(seed,(uint8_t)armor_skip_update);
    seed=digest_word(seed,hit_count);
    for (unsigned i=0;i<hit_count;++i) {
        seed=digest_word(seed,hits[i].actor);
        seed=digest_byte(seed,hits[i].token[0]);
        seed=digest_byte(seed,hits[i].token[1]);
    }
    seed=digest_word(seed,actor_function);
    seed=digest_word(seed,actor_pending);
    seed=digest_word(seed,actor_pool);
    seed=digest_byte(seed,(uint8_t)actor_call);
    seed=digest_byte(seed,(uint8_t)actor_suspended);
    seed=digest_byte(seed,(uint8_t)actor_owner);
    seed=digest_byte(seed,actor_campaign);
    seed=digest_byte(seed,pool_first_freeze);
    return seed;
}

PSX_MOD_CONSTRUCTOR(mmx4_register_coop_combat) {
    psx_mod_register_function_filter_plugin(COOP_ID,CONTACT,accepted_hit);
    psx_mod_register_function_filter_plugin(COOP_ID,ENEMY_HIT,accepted_hit);
    psx_mod_register_function_filter_plugin(COOP_ID,SOLID_CONTACT,solid_player_contact);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002AB74u,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002ACA4u,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002ADBCu,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,EFFECT_ALLOCATE,allocate_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800AF22Cu,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800AEED8u,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,PARTICLE_ALLOCATE,allocate_particle);
    psx_mod_register_function_filter_plugin(COOP_ID,PARTICLE_INITIALIZE,initialize_death_particle);
    psx_mod_register_function_filter_plugin(COOP_ID,ARMOR_UPDATE,armor_world_update);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80021234u,survivor_actor_pool);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002144Cu,survivor_actor_pool);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80040CCCu,aim_at_participant);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800419B8u,aim_at_participant);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80042824u,aim_at_participant);
    psx_mod_register_instruction_plugin(COOP_ID,0x80021300u,0x0040F809u,redirect_actor);
    psx_mod_register_instruction_plugin(COOP_ID,0x80021518u,0x0040F809u,redirect_actor);
    psx_mod_register_guest_function_plugin(COOP_ID,ACTOR_DISPATCH,update_actor);
}
