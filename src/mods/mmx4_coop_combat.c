/* Original SLUS-00561 combat and moving-solid boundaries. Shared enemies,
 * projectiles and scripts retain their original once-per-world-frame update.
 * See docs/COOP_COMBAT_VEHICLE_EVIDENCE.md for original instruction evidence. */
#include "mmx4_coop_internal.h"
#include "mmx4_story_damage.h"

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
#define SECONDARY_POOL 0x8013F328u
#define SECONDARY_CAPACITY 32u
#define SECONDARY_STRIDE 0x9Cu
#define PARTICLE_ALLOCATE 0x8002AE50u
#define PARTICLE_INITIALIZE 0x800CAE38u
#define TRAIN_PILLAR_UPDATE 0x800CB8F8u
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
/* Ride Chaser types56/57 retain their allocating rider. Web Spider type22
 * replaces this with its actual victim when native contact captures them. */
static uint8_t secondary_owner[SECONDARY_CAPACITY];
/* The type-11 death effect defers its character assembly and position reads
 * until the shared pool pass. Keep the allocation-time origin through rejoin. */
static struct { uint32_t x,y; uint8_t owner,screen; } particles[PARTICLE_CAPACITY];
static unsigned particle_allocation_call,particle_init_call;
static unsigned armor_call, armor_owner, armor_skip_update;
static unsigned survivor_pool_call;
static unsigned actor_call,actor_suspended,actor_owner;
static uint32_t actor_function,actor_pending,actor_pool;
static uint8_t actor_campaign,pool_first_freeze;
/* Valid only within one shared actor update, after an accepted player hit.
 * Boss reactions must use that attacker, independently of the AI target and
 * P1's campaign. No state survives the actor boundary or enters snapshots. */
static uint32_t reaction_actor;
static unsigned reaction_character;
static HitContact hits[SOLID_CAPACITY];
static unsigned hit_count;

int mmx4_coop_combat_snapshot_ready(void) {
    return !(combat_call || solid_call || allocation_call || aim_call || effect_allocation_call ||
        personal_effect_call || particle_allocation_call || particle_init_call || armor_call ||
        survivor_pool_call || actor_call || actor_suspended || actor_owner || actor_function ||
        actor_pending || actor_pool || actor_campaign || pool_first_freeze ||
        reaction_actor || reaction_character);
}
int mmx4_coop_combat_snapshot(Mmx4SnapshotIO *s) {
    unsigned count=mmx4_snapshot_integer(s,solid_count,4,SOLID_CAPACITY);
    if(s->mode==MMX4_SNAPSHOT_READ)solid_count=count;
    uint32_t previous=0;
    for(unsigned i=0;i<count && s->ok;++i) {
        uint32_t actor=mmx4_snapshot_integer(s,solids[i].actor,4,UINT32_MAX);
        if(!actor || !mmx4_snapshot_guest(actor) || actor<=previous)s->ok=0;
        previous=actor;
        if(s->mode==MMX4_SNAPSHOT_READ)solids[i].actor=actor;
        MMX4_SNAPSHOT_U8(s,solids[i].sides);MMX4_SNAPSHOT_U8(s,solids[i].carried);
        MMX4_SNAPSHOT_U8(s,solids[i].vehicle_sides);MMX4_SNAPSHOT_U8(s,solids[i].vehicle_carried);
    }
    count=mmx4_snapshot_integer(s,hit_count,4,SOLID_CAPACITY);
    if(s->mode==MMX4_SNAPSHOT_READ)hit_count=count;
    previous=0;
    for(unsigned i=0;i<count && s->ok;++i) {
        uint32_t actor=mmx4_snapshot_integer(s,hits[i].actor,4,UINT32_MAX);
        if(!actor || !mmx4_snapshot_guest(actor) || actor<=previous)s->ok=0;
        previous=actor;
        if(s->mode==MMX4_SNAPSHOT_READ)hits[i].actor=actor;
        MMX4_SNAPSHOT_U8(s,hits[i].token[0]);MMX4_SNAPSHOT_U8(s,hits[i].token[1]);
    }
    for(unsigned i=0;i<EFFECT_CAPACITY;++i)MMX4_SNAPSHOT_BOOL(s,second_effect_owner[i]);
    for(unsigned i=0;i<SECONDARY_CAPACITY;++i)MMX4_SNAPSHOT_FIELD(s,secondary_owner[i],1,2);
    for(unsigned i=0;i<PARTICLE_CAPACITY;++i) {
        MMX4_SNAPSHOT_U32(s,particles[i].x);MMX4_SNAPSHOT_U32(s,particles[i].y);
        MMX4_SNAPSHOT_BOOL(s,particles[i].owner);MMX4_SNAPSHOT_U8(s,particles[i].screen);
    }
    MMX4_SNAPSHOT_BOOL(s,armor_owner);MMX4_SNAPSHOT_BOOL(s,armor_skip_update);
    if(s->mode==MMX4_SNAPSHOT_READ) {
        memset(solids+solid_count,0,(SOLID_CAPACITY-solid_count)*sizeof solids[0]);
        memset(hits+hit_count,0,(SOLID_CAPACITY-hit_count)*sizeof hits[0]);
    }
    return s->ok;
}

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
    /* Attached effects also update in an owner projection. Their native
     * visibility queries must see both Split views: water's B288 otherwise
     * hides P2's ripples whenever they leave the canonical P1 camera. */
    return personal_effect_call ||
        (actor_call && !actor_suspended && (!mmx4_coop_projected() || actor_owner==1));
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
    unsigned attacker=0;
    if(address==ENEMY_HIT)reaction_actor=reaction_character=0;
    HitContact *history=address==ENEMY_HIT?hit_record(actor,1):NULL;
    uint8_t first_token=history?history->token[0]:0;
    if (history) psx_mod_write_byte(actor+0x65,first_token);
    /* Native enemy pools skip collisions while PLAYER+BC is nonzero.
     * The shared pool runs for P1, so apply the same gate to each projected
     * seat. Otherwise a persistent Zero saber reasserts +BD every world pass
     * and the next player tick restarts the five-tick hit pause forever. */
    if (mmx4_coop_alive(0) && !psx_mod_read_byte(MMX4_PLAYER+0xBC)) {
        result=mmx4_coop_call(cpu,address,actor,arg);
        if(result)attacker=1;
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
        if(result)attacker=2;
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
    if(address==ENEMY_HIT && actor_call && attacker && result && result<0x7Fu) {
        reaction_actor=actor;
        reaction_character=attacker==1?psx_mod_read_byte(MMX4_PLAYER+2)+1u:
            mmx4_coop_second_body()[2]+1u;
    }
    combat_call=0;
    /* The original actor caller applies the selected result and advances its
     * state once. In particular, one consumable attack cannot hit both seats. */
    return mmx4_coop_finish(cpu,result);
}

static int private_boss_hit(CPUState *cpu,uint32_t address) {
    (void)address;
    /* DD04 must keep testing overlap during private immunity: when a blade
     * leaves the boss, its no-contact path clears that seat's previous +65
     * token. Blocking DD04 itself retains the old token and can reject the
     * next same-key swing after immunity expires. Zero's native 800981AC
     * derives these keys from subtype/pose, so new swings can reuse them.
     * Gate only DE30's accepted
     * overlap, before it stores a token, consumes a shot or applies damage.
     * Its native duplicate result 7E preserves a still-held contact. */
    if(combat_call && mmx4_coop_ready() && !psx_mod_local_view_scope()) {
        if(!mmx4_coop_balance_contact_allowed(cpu->gpr[4],cpu->gpr[5],mmx4_coop_projected()?1u:0u))
            return mmx4_coop_finish(cpu,0x7Eu);
        return mmx4_coop_balance_project_contact(cpu,address);
    }
    return 0;
}

static void attack_reaction_character(CPUState *cpu,uint32_t address) {
    (void)address;
    /* These NOPs follow the retail load of PLAY+43 and precede its branch.
     * Change only the loaded character, retaining native attack result,
     * damage, reaction tables and every shared story/AI field. */
    if(actor_call && !actor_suspended && !combat_call && mmx4_coop_ready() &&
       !psx_mod_local_view_scope() && reaction_actor==cpu->gpr[16] &&
       reaction_character && reaction_character<=2)
        cpu->gpr[2]=reaction_character-1u;
}

static void owl_release_partner(CPUState *cpu,uint32_t address) {
    /* Retail clears PLAYER+BA when Owl releases or aborts his grab. Both
     * bodies can be caught by that one attack, but the selected AI target
     * can change before release. Apply the same native release to the other
     * captured body; preserve negative pending damage/throw commands. */
    if(!actor_call || actor_suspended || actor_pool!=0x80021234u ||
       !mmx4_coop_ready() || psx_mod_local_view_scope())return;
    uint32_t actor=cpu->gpr[address==0x8007687Cu?17:16];
    if(actor<0x8013BED0u || actor>=0x8013BED0u+48u*0x9Cu ||
       (actor-0x8013BED0u)%0x9Cu || !psx_mod_read_byte(actor) ||
       psx_mod_read_byte(actor+1)!=60)return;
    uint8_t *partner=mmx4_coop_projected()?mmx4_coop_first_body():mmx4_coop_second_body();
    if(partner[0xBA]==1) {
        partner[0xBA]=0;
        psx_mod_counter_add("mmx4.coop.owl-partner-releases",1);
    }
}

static int spider_web(uint32_t actor) {
    return actor>=SECONDARY_POOL && actor<SECONDARY_POOL+SECONDARY_CAPACITY*SECONDARY_STRIDE &&
        !((actor-SECONDARY_POOL)%SECONDARY_STRIDE) && psx_mod_read_byte(actor+1)==22;
}
/* Admit the counterpart to these campaign-specific bosses. The native
 * duplicate-hit gate has already run. DE84 follows the admission LBU's delay
 * slot and precedes its reaction-byte store. Cooldown tables stay untouched. */
static void counterpart_hit(CPUState *cpu,uint32_t address) {
    (void)address;
    if(combat_call && mmx4_coop_ready())mmx4_story_counterpart_hit(cpu);
}

static int accepted_hit(CPUState *cpu,uint32_t address) {
    /* 8009DB9C reads/writes PLAYER immediately after CONTACT. It must see
     * the same seat that the collision tested, throughout this actor pass. */
    if(address==CONTACT && actor_call && !actor_suspended && spider_web(cpu->gpr[4]))
        return mmx4_coop_alive(actor_owner) && !psx_mod_read_byte(MMX4_PLAYER+0xBC)?
            0:mmx4_coop_finish(cpu,0);
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
    if (allocation_call ||
        psx_mod_read_byte(MMX4_PLAY)!=6) return 0;
    allocation_call=1;
    uint32_t actor=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    allocation_call=0;
    if (actor) {forget_solid(actor);forget_hit(actor);mmx4_coop_balance_forget(actor);}
    if(address==0x8002AC0Cu && actor)
        mmx4_coop_web_forget(actor,(unsigned)(mmx4_coop_projected()!=0));
    if(actor>=SECONDARY_POOL && actor<SECONDARY_POOL+SECONDARY_CAPACITY*SECONDARY_STRIDE &&
       !((actor-SECONDARY_POOL)%SECONDARY_STRIDE))
        secondary_owner[(actor-SECONDARY_POOL)/SECONDARY_STRIDE]=(uint8_t)(mmx4_coop_projected()+1);
    return mmx4_coop_finish(cpu,actor);
}

unsigned mmx4_coop_secondary_allocation_owner(uint32_t actor) {
    if(actor<SECONDARY_POOL || actor>=SECONDARY_POOL+SECONDARY_CAPACITY*SECONDARY_STRIDE ||
       (actor-SECONDARY_POOL)%SECONDARY_STRIDE)return 0;
    return secondary_owner[(actor-SECONDARY_POOL)/SECONDARY_STRIDE];
}
unsigned mmx4_coop_vehicle_effect_owner(uint32_t actor) {
    if(actor>=EFFECT_POOL && actor<EFFECT_POOL+EFFECT_CAPACITY*EFFECT_STRIDE &&
       !((actor-EFFECT_POOL)%EFFECT_STRIDE)) {
        unsigned type=psx_mod_read_byte(actor+1);
        if(type==0x0C || type==0x0D || type==0x0F)
            return second_effect_owner[(actor-EFFECT_POOL)/EFFECT_STRIDE]?2u:
                (psx_mod_read_word(actor+0x50)==MMX4_VEHICLE?1u:0u);
        return 0;
    }
    if(actor<SECONDARY_POOL || actor>=SECONDARY_POOL+SECONDARY_CAPACITY*SECONDARY_STRIDE ||
       (actor-SECONDARY_POOL)%SECONDARY_STRIDE)return 0;
    unsigned type=psx_mod_read_byte(actor+1);
    return type==56 || type==57?secondary_owner[(actor-SECONDARY_POOL)/SECONDARY_STRIDE]:0;
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
    /* Types0/1 defer their assembly and PLAYER/Double origin reads until
     * the shared pool pass, then follow that owner while sliding/dashing.
     * Type3 dust already has a fixed origin and retains its shared update.
     * Type27 uploads Ground Hunter's compressed graphics from SCRATCH+14.
     * This deferred pass must use the allocating X, even with Zero/P1.
     * Type33's boot jets bind SCRATCH+1C and follow the owner's animation
     * frame/position. Reading the other hero's pose deletes them immediately.
     * Type 7 follows PLAYER/Double. Type 2 is X's delayed charge release:
     * it uses the owner's assembly/pose, then allocates a native projectile
     * at 800AF0A4. Keep that allocation in the owner's private shot pool.
     * Type26 follows a shot through +50 and copies its graphics at B2924.
     * That address names the allocating seat's projected shot arena. With
     * P1 restored it can instead name Zero's simultaneous Ryuuenjin; the
     * X-only effect frame then decodes unrelated bytes over PLAY state. */
    personal_effect_call=1;mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    mmx4_coop_leave_second();personal_effect_call=0;
    return mmx4_coop_finish(cpu,result);
}

static int aiming_laser_beam(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];
    if(personal_effect_call || !world_ready() || mmx4_coop_second_body()[2] ||
       actor<0x801435B0u || actor>=0x801441B0u || (actor-0x801435B0u)%0x60u ||
       psx_mod_read_byte(actor+1)!=9)return 0;
    /* Aiming Laser's reticles belong to the private shot pool, but 968A4
     * allocates its beam in a separate shared 60-byte pool. D67DC checks
     * PLAYER+93 (weapon6), PLAYER's pose, and uses PLAYER as the beam origin.
     * With Zero/P1 restored it deletes X/P2's beam before its first draw.
     * Only X creates this type: bind the deferred update to X/P2 once.
     * Native target checks, beam geometry, flicker and expiry remain intact. */
    personal_effect_call=1;mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    mmx4_coop_leave_second();personal_effect_call=0;
    psx_mod_counter_add("mmx4.coop.p2-aiming-laser-beam-ticks",1);
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
int mmx4_coop_stored_charge_owner(uint32_t actor) {
    int slot=particle_slot(actor);
    return slot>=0 && psx_mod_read_byte(actor+1)==0x36 ? particles[slot].owner : -1;
}
static int stored_charge_hud(CPUState *cpu,uint32_t address) {
    /* Native35558 creates type54 only for X. B2C8C later reads PLAYER+A6
     * and SCRATCH+1C from the shared particle pass: those must still belong
     * to the allocating X when Zero owns the canonical player slot. */
    if(personal_effect_call || !world_ready() || psx_mod_local_view_scope() ||
       mmx4_coop_stored_charge_owner(cpu->gpr[4])!=1)return 0;
    personal_effect_call=1;mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    mmx4_coop_leave_second();personal_effect_call=0;
    psx_mod_counter_add("mmx4.coop.p2-stored-charge-hud-ticks",1);
    return mmx4_coop_finish(cpu,result);
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
static int initialize_character_particle(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];int slot=particle_slot(actor);
    unsigned kind=psx_mod_read_byte(actor+1);
    if(particle_init_call || !mmx4_coop_ready() || slot<0 || !particles[slot].owner ||
       (kind!=0x11 && kind!=47 && kind!=48) || psx_mod_read_byte(actor+4))return 0;
    particle_init_call=1;
    int entered=!mmx4_coop_projected();
    if(entered)mmx4_coop_enter_second();
    uint32_t x=psx_mod_read_word(MMX4_PLAYER+8),y=psx_mod_read_word(MMX4_PLAYER+12);
    uint8_t screen=psx_mod_read_byte(MMX4_PLAYER+0x14);
    psx_mod_write_word(MMX4_PLAYER+8,particles[slot].x);
    psx_mod_write_word(MMX4_PLAYER+12,particles[slot].y);
    psx_mod_write_byte(MMX4_PLAYER+0x14,particles[slot].screen);
    /* Death11 and elemental fragments47/48 defer character resources and
     * the source body reads until the shared pool pass. Preserve their
     * allocating origin even if that body moved, died or warped meanwhile. */
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_word(MMX4_PLAYER+8,x);psx_mod_write_word(MMX4_PLAYER+12,y);
    psx_mod_write_byte(MMX4_PLAYER+0x14,screen);
    if(entered)mmx4_coop_leave_second();
    particles[slot].owner=0;
    psx_mod_counter_add("mmx4.coop.p2-character-particle-inits",1);
    particle_init_call=0;
    return mmx4_coop_finish(cpu,result);
}

static int initialize_train_pillar(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];int slot=particle_slot(actor);
    if(particle_init_call || !world_ready() || mmx4_coop_projected() ||
       psx_mod_local_view_scope() || !mmx4_coop_split_views() || slot<0 ||
       psx_mod_read_byte(MMX4_PLAY+12)!=8 || psx_mod_read_byte(MMX4_PLAY+13)!=1 ||
       psx_mod_read_byte(actor+1)!=20 || psx_mod_read_byte(actor+4))return 0;
    uint32_t train=psx_mod_read_word(actor+0x50);
    if(train<0x80169CB8u || train>=0x80169D78u || (train-0x80169CB8u)%0x30u ||
       !psx_mod_read_byte(train) || psx_mod_read_byte(train+1)!=2)return 0;
    unsigned owner=particles[slot].owner;
    uint8_t canonical[0xFC];
    if(!mmx4_coop_split_scene_camera_begin(owner,canonical))return 0;
    particle_init_call=1;
    uint8_t layer=psx_mod_read_byte(MMX4_PLAYER+0x14);
    if(owner)psx_mod_write_byte(MMX4_PLAYER+0x14,mmx4_coop_second_body()[0x14]);
    /* D9A58 queues type20 inside the train owner's projection. The shared
     * particle pass later calls CB940, placing it at CAMERA+(352,128).
     * Use that owner's view for this one native initialization, retaining
     * stage resources at SCRATCH+20 rather than projecting hero graphics.
     * Live movement/culling and the shared train clock still run once. */
    uint32_t result=mmx4_coop_call(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_byte(MMX4_PLAYER+0x14,layer);
    mmx4_coop_split_scene_camera_end(owner,canonical,0);
    particles[slot].owner=0;
    psx_mod_counter_add(owner?"mmx4.coop.p2-train-pillar-inits":
                              "mmx4.coop.p1-train-pillar-inits",1);
    particle_init_call=0;
    return mmx4_coop_finish(cpu,result);
}

int mmx4_coop_armor_mounted(void) {
    return mmx4_coop_ready() && psx_mod_read_byte(MMX4_PLAYER+0xC5) &&
        psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+1)==1 &&
        (psx_mod_read_byte(MMX4_VEHICLE+0x97)&0x40u);
}

void mmx4_coop_armor_dismount(CPUState *cpu,unsigned seat) {
    if(seat>1 || !mmx4_coop_ready() || psx_mod_local_view_scope())return;
    unsigned original=(unsigned)(mmx4_coop_projected()!=0);
    if(seat!=original) {if(seat)mmx4_coop_enter_second();else mmx4_coop_leave_second();}
    if(mmx4_coop_armor_mounted()) {
        /* Retail's B/EEF8 is also the automatic scene ejection path selected
         * by403DC. It clears C5/occupied40, restores the character's foot box,
         * and retains the actual armor's position, remaining HP and animation.
         * Run only its first native tick; never replay the whole world update. */
        unsigned guard=armor_call;armor_call=1;
        mmx4_coop_call(cpu,0x8003DC44u,MMX4_VEHICLE,0xBu);
        mmx4_coop_call(cpu,0x8003EEF8u,MMX4_VEHICLE,0);
        armor_call=guard;
        if(seat && mmx4_coop_first_vehicle()[0]) {
            /* Two authored vehicles can coexist. A forced departure cannot
             * overwrite P1's vehicle or carry unoccupied armor into a new
             * room; retire this abandoned private actor in that case. */
            psx_mod_write_byte(MMX4_VEHICLE,0);psx_mod_write_byte(MMX4_VEHICLE+3,0);
            armor_owner=armor_skip_update=0;
        }
        psx_mod_counter_add("mmx4.coop.armor-forced-dismounts",1);
    }
    if(seat!=original) {if(original)mmx4_coop_enter_second();else mmx4_coop_leave_second();}
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
    if(armor_owner && !psx_mod_read_byte(MMX4_VEHICLE)) {
        armor_owner=armor_skip_update=0;
    }
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
    /* Mode0 is the original unoccupied mount routine (table800F9150/9188).
     * P1 has first choice through the world update above. */
    if (!armor_owner && mmx4_coop_alive(1) && body[4]==1 && !body[0xC5] &&
        !second[0] && !psx_mod_read_byte(MMX4_PLAYER+0xC5) &&
        psx_mod_read_byte(MMX4_VEHICLE) && psx_mod_read_byte(MMX4_VEHICLE+1)==1 &&
        psx_mod_read_byte(MMX4_VEHICLE+4)==1 && psx_mod_read_byte(MMX4_VEHICLE+5)==0 &&
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
    uint8_t native_carried=psx_mod_read_byte(actor+0x76);
    uint32_t result=mmx4_coop_call(cpu,address,actor,arg);
    const uint8_t *second=mmx4_coop_second_body();
    /* Incoming beams remain excluded from combat, but need world-solid
     * floors to land. Terrain alone cannot see a portal or moving platform.
     * Outgoing, withdrawn, dead and uninitialized bodies remain excluded. */
    unsigned incoming=mmx4_coop_lifecycle_arriving(1) && second[0] && second[3] &&
        (second[0x5C]&127u) && body_word(second,0x68);
    if ((mmx4_coop_alive(1) || incoming) && second[4]==1 &&
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
            /* BED90 writes -1 before every update of the intro's dropping
             * slab. E184 skips its carry pass for a nonpositive signed
             * value; restoring P2's previous contact instead attaches them
             * to that slab while P1 falls normally. Preserve an authored
             * negative override from before P1's collision pass, which
             * C36C may already have replaced with its new contact result. */
            psx_mod_write_byte(actor+0x76,(int8_t)native_carried<0?
                native_carried:record->carried);
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
    if(mmx4_coop_projected() && mmx4_coop_lifecycle_defer_portal_solid(cpu->gpr[4]))
        return mmx4_coop_finish(cpu,0);
    if(mmx4_coop_web_busy())return 0;
    if(mmx4_coop_web_contact(cpu,address))return 1;
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
    actor_call=1;reaction_actor=reaction_character=0;
    psx_mod_write_byte(MMX4_PLAYER+0xBC,pool_first_freeze);
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    int owner=mmx4_coop_lifecycle_script_owner();
    unsigned target=owner>=0?(unsigned)owner:mmx4_coop_view_nearest(actors,
        (int16_t)psx_mod_read_half(actor+10),(int16_t)psx_mod_read_half(actor+14),0);
    /* Slash Beast waits at X9312 but native8006DC24 triggers on player
     * X>=9472. Nearest-to-boss selects the partner left behind and prevents
     * the leading seat from committing the introduction. Once C0 starts,
     * the ordinary script-owner latch takes over. Fight AI remains nearest. */
    if(owner<0 && actor_pool==0x80021234u &&
       psx_mod_read_byte(MMX4_PLAY+0x0C)==8 && psx_mod_read_byte(MMX4_PLAY+0x0D)==1 &&
       psx_mod_read_byte(actor+1)==54 && psx_mod_read_byte(actor+4)==1 &&
       psx_mod_read_byte(actor+5)==9 && !psx_mod_read_byte(actor+6))
        target=!actors[0].active || (actors[1].active && actors[1].world_x>actors[0].world_x);
    /* Eregion starts its WARNING/action15 in native phase1/9/0
     * (494E0). Its background position is behind the entering hero, so
     * nearest-to-boss can choose the trailing hero instead. Fight phases
     * retain nearest targeting; the admitted scene latches its owner. */
    unsigned intro_admission=actor_pool==0x80021234u &&
        !psx_mod_read_byte(MMX4_PLAY+12) && psx_mod_read_byte(MMX4_PLAY+13)==1 &&
        psx_mod_read_byte(actor+1)==8 && psx_mod_read_byte(actor+4)==1 &&
        psx_mod_read_byte(actor+5)==9 && !psx_mod_read_byte(actor+6);
    if(owner<0 && intro_admission)
        target=!actors[0].active || (actors[1].active && actors[1].world_x>actors[0].world_x);
    /* Outdoor Sky Lagoon's opening swarm waits in native49D60 until the
     * hero is193px ahead. Nearest targeting selects the waiting partner
     * behind these actors forever. Let either living hero cross that native
     * threshold; once activated, their ordinary nearest-target AI resumes. */
    unsigned intro_swarm=actor_pool==0x80021234u &&
        !psx_mod_read_byte(MMX4_PLAY+12) && !psx_mod_read_byte(MMX4_PLAY+13) &&
        psx_mod_read_byte(actor+1)==10 && psx_mod_read_byte(actor+2)>=3 &&
        psx_mod_read_byte(actor+2)<=8 && psx_mod_read_byte(actor+4)==1 &&
        psx_mod_read_byte(actor+5)==2;
    if(owner<0 && intro_swarm)
        target=!actors[0].active || (actors[1].active && actors[1].world_x>actors[0].world_x);
    unsigned vehicle_owner=actor_pool==0x8002144Cu?mmx4_coop_vehicle_effect_owner(actor):0;
    if(vehicle_owner)target=vehicle_owner-1;
    int web=actor_pool==0x8002144Cu && spider_web(actor);
    int captured=web && psx_mod_read_word(actor+0x8C)==3;
    if(captured) {
        unsigned victim=secondary_owner[(actor-SECONDARY_POOL)/SECONDARY_STRIDE];
        if(victim)target=victim-1;
        /* Native 8009DA7C pins PLAYER to the web and periodically damages
         * them without checking death. Release through native 8009DCF4
         * before selecting a new living AI target could transfer the web. */
        if(!mmx4_coop_alive(target))psx_mod_write_byte(actor+4,2);
    }
    actor_owner=target;
    actor_campaign=psx_mod_read_byte(MMX4_PLAY+0x43);
    /* Mushroom788B8 initializes its four rainbow CLUTs through7B7B4,
     * copying stage colors+6A0 into the stage working palette. A survivor
     * projection must not replace those resources with P2's hero palettes.
     * Keep this scoped to native boss initialization; live AI and personal
     * effects retain their existing resource bindings. */
    unsigned mushroom_palette=actor_pool==0x80021234u &&
        psx_mod_read_byte(actor+1)==61 && !psx_mod_read_byte(actor+4);
    unsigned dragoon_intro=actor_pool==0x80021234u &&
        psx_mod_read_byte(actor+1)==65 && psx_mod_read_byte(actor+4)==1 &&
        psx_mod_read_byte(actor+5)==2;
    unsigned sigma_victory=actor_pool==0x80021234u &&
        psx_mod_read_byte(MMX4_PLAY+12)==12 && psx_mod_read_byte(MMX4_PLAY+13)==1 &&
        psx_mod_read_byte(actor+1)==74 && psx_mod_read_byte(actor+4)==2 &&
        psx_mod_read_byte(actor+5)==6;
    uint32_t stage_colors=mushroom_palette?psx_mod_read_word(0x1F800024u):0;
    uint32_t stage_palette=mushroom_palette?psx_mod_read_word(0x1F800028u):0;
    uint8_t scene_camera[0xFC];
    unsigned dragoon_admission=psx_mod_read_byte(MMX4_PLAY+12)==4 &&
        psx_mod_read_byte(MMX4_PLAY+13)==1 && psx_mod_read_byte(actor+1)==65 &&
        psx_mod_read_byte(actor+4)==1 && psx_mod_read_byte(actor+5)==2 &&
        !psx_mod_read_byte(actor+6);
    int own_scene_camera=target && actor_pool==0x80021234u &&
        (intro_admission || dragoon_admission || (psx_mod_read_byte(MMX4_PLAY+12)==3 &&
        !psx_mod_read_byte(MMX4_PLAY+13) && psx_mod_read_byte(actor+1)==33 &&
        !psx_mod_read_byte(actor+4))) &&
        mmx4_coop_split_scene_camera_begin(target,scene_camera);
    uint32_t result=0;
    if(actors[target].active || vehicle_owner || captured) {
        if(target) {
            mmx4_coop_enter_second();
            psx_mod_write_byte(MMX4_PLAY+0x43,actor_campaign);
            if(mushroom_palette) {
                psx_mod_write_word(0x1F800024u,stage_colors);
                psx_mod_write_word(0x1F800028u,stage_palette);
            }
        }
        if(psx_mod_read_byte(MMX4_PLAYER+0xBC) && !(captured && psx_mod_read_byte(actor+4)==2)) {
            if(psx_mod_read_byte(actor+3))result=mmx4_coop_call(cpu,0x8002B3C0u,actor,0);
        }else {
            mmx4_coop_balance_begin(actor);
            result=mmx4_coop_call(cpu,function,actor,cpu->gpr[5]);
            mmx4_coop_balance_end();
            /* 7E3D0 releases the script before 7E400..7E430 install the
             * refight camera targets. Publish those completed native writes
             * while the actor still owns its projection, before its camera
             * can be replaced by the next saved-view handoff. */
            if(dragoon_intro && psx_mod_read_byte(actor+4)==1 &&
               psx_mod_read_byte(actor+5)==3)
                mmx4_coop_split_dragoon_camera_release(actor);
            if(sigma_victory)mmx4_coop_split_sigma_victory_release(actor);
            if(web && !captured && psx_mod_read_word(actor+0x8C)==3)
                secondary_owner[(actor-SECONDARY_POOL)/SECONDARY_STRIDE]=(uint8_t)(target+1);
            psx_mod_counter_add(target?"mmx4.coop.p2-native-ai":"mmx4.coop.p1-native-ai",1);
        }
        if(target) {
            uint8_t palette_refresh=mushroom_palette?
                (uint8_t)(psx_mod_read_byte(0x80166BB0u)&1u):0;
            psx_mod_write_byte(MMX4_PLAY+0x43,psx_mod_read_byte(MMX4_PLAYER+2));
            mmx4_coop_leave_second();
            if(palette_refresh)psx_mod_write_byte(0x80166BB0u,
                (uint8_t)(psx_mod_read_byte(0x80166BB0u)|palette_refresh));
        }
    }
    /* Mushroom's allocation and Dragoon's7DF50 directly admit a shared
     * scene. Start that scene from its entering hero's actual view;
     * otherwise the native pan begins at a distant P1 camera and hides P2.
     * Normal fight targeting and other enemy camera state stay unchanged. */
    if(own_scene_camera)mmx4_coop_split_scene_camera_end(target,scene_camera,
        mmx4_coop_lifecycle_script_owner()==(int)target);
    pool_first_freeze=psx_mod_read_byte(MMX4_PLAYER+0xBC);
    psx_mod_write_byte(MMX4_PLAYER+0xBC,0);
    actor_campaign=0;actor_call=actor_owner=0;
    reaction_actor=reaction_character=0;
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
    mmx4_coop_web_reset();
    memset(solids,0,sizeof solids);
    memset(second_effect_owner,0,sizeof second_effect_owner);
    memset(secondary_owner,0,sizeof secondary_owner);
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
    reaction_actor=reaction_character=0;
}

static uint32_t digest_byte(uint32_t seed, uint8_t value) {
    return (seed^value)*16777619u;
}

static uint32_t digest_word(uint32_t seed, uint32_t value) {
    for (unsigned i=0;i<4;++i) seed=digest_byte(seed,(uint8_t)(value>>(8*i)));
    return seed;
}

uint32_t mmx4_coop_combat_digest(uint32_t seed) {
    seed=mmx4_coop_web_digest(seed);
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
    for(unsigned i=0;i<SECONDARY_CAPACITY;++i)
        seed=digest_byte(seed,secondary_owner[i]);
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
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002DE30u,private_boss_hit);
    psx_mod_register_function_filter_plugin(COOP_ID,SOLID_CONTACT,solid_player_contact);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002AB74u,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002ACA4u,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002AC0Cu,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002ADBCu,allocate_actor);
    psx_mod_register_function_filter_plugin(COOP_ID,EFFECT_ALLOCATE,allocate_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800AEAC0u,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800AED18u,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800AF22Cu,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800AEED8u,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B28CCu,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B2A3Cu,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B2AD0u,personal_effect);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800D67DCu,aiming_laser_beam);
    psx_mod_register_function_filter_plugin(COOP_ID,PARTICLE_ALLOCATE,allocate_particle);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800B2C8Cu,stored_charge_hud);
    psx_mod_register_function_filter_plugin(COOP_ID,PARTICLE_INITIALIZE,initialize_character_particle);
    psx_mod_register_function_filter_plugin(COOP_ID,TRAIN_PILLAR_UPDATE,initialize_train_pillar);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8009485Cu,initialize_character_particle);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80098474u,initialize_character_particle);
    psx_mod_register_function_filter_plugin(COOP_ID,ARMOR_UPDATE,armor_world_update);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80021234u,survivor_actor_pool);
    psx_mod_register_function_filter_plugin(COOP_ID,0x8002144Cu,survivor_actor_pool);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80040CCCu,aim_at_participant);
    psx_mod_register_function_filter_plugin(COOP_ID,0x800419B8u,aim_at_participant);
    psx_mod_register_function_filter_plugin(COOP_ID,0x80042824u,aim_at_participant);
    psx_mod_register_instruction_plugin(COOP_ID,0x8002DE84u,0xA0820063u,counterpart_hit);
    psx_mod_register_instruction_plugin(COOP_ID,0x80050998u,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x80071DE4u,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x80072514u,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x800753FCu,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x80082644u,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x800850DCu,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x80091344u,0,attack_reaction_character);
    psx_mod_register_instruction_plugin(COOP_ID,0x80075694u,0xA0201982u,owl_release_partner);
    psx_mod_register_instruction_plugin(COOP_ID,0x800763D0u,0xA0201982u,owl_release_partner);
    psx_mod_register_instruction_plugin(COOP_ID,0x800764D4u,0xA0201982u,owl_release_partner);
    psx_mod_register_instruction_plugin(COOP_ID,0x800767E4u,0xA0201982u,owl_release_partner);
    psx_mod_register_instruction_plugin(COOP_ID,0x8007687Cu,0xA0201982u,owl_release_partner);
    psx_mod_register_instruction_plugin(COOP_ID,0x800778C8u,0xA0201982u,owl_release_partner);
    psx_mod_register_instruction_plugin(COOP_ID,0x80021300u,0x0040F809u,redirect_actor);
    psx_mod_register_instruction_plugin(COOP_ID,0x80021518u,0x0040F809u,redirect_actor);
    psx_mod_register_guest_function_plugin(COOP_ID,ACTOR_DISPATCH,update_actor);
}
