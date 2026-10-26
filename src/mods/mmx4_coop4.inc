/* Experimental N-seat native execution. Included here to share the audited ARC,
 * packet and immutable texture helpers with two-player co-op. The two engines
 * have different package gates and never execute together. */
#define FOUR_ID "mmx4.coop4"
#define FOUR_CAMERA 0x801419B0u
#define FOUR_ACTOR_DISPATCH 0x8F7FFE00u
#define FOUR_PLAYER_PASS 0x8F7FFE10u
#define FOUR_CAMERA_PASS 0x8F7FFE20u
typedef struct {
    PlayerContext player;
    uint8_t inventory[0x64], palette[0x1000], camera[0xFC];
    uint16_t previous;
} FourSeat;
typedef struct {
    uint32_t actor;
    uint8_t hit[4],sides[4],carried[4],vehicle_sides[4],vehicle_carried[4];
} FourContact;
static Mmx4CoopRoster four_roster;
static FourSeat four_seats[4];
static unsigned four_menu_character(unsigned seat) {
    four_target=seat;
    return seat<four_roster.count?mmx4_roster_character(four_roster.hero[seat]):0;
}
static FourContact four_contacts[256];
static unsigned four_contact_count,four_busy,four_current,four_collision;
static unsigned four_camera_call,four_render_call,four_present_call;
static unsigned four_controls_call;
static unsigned four_camera_owner;
static unsigned four_actor_call,four_pending,four_function,four_reaction,four_reaction_actor;
static unsigned four_draw_used[4];
static uint8_t four_effect_owner[32];
/* Native X stock displays live in the shared 60-byte particle pool. Each
 * complete X form can allocate one; their counters and HUDs remain personal. */
static uint8_t four_particle_owner[64];
static unsigned four_allocation_call,four_effect_call,four_world_draw_call;
static unsigned four_scan_call,four_supplemental,four_bounds_call;
static unsigned four_death_call;
#ifndef PSX_NO_DEBUG_TOOLS
static unsigned four_fixture_protection;
#endif
static unsigned four_normal_call;
static uint16_t four_menu_previous[4];
/* One authored door/world state machine, with native auto-walk protection
 * for every living participant. Dead seats never join or get revived. */
static struct { uint32_t actor; unsigned owner,participants; int8_t feet[4]; } four_door;
static unsigned four_door_call;
static uint32_t four_placement_table;
static uint8_t four_visited[512];
static uint8_t four_saved_inventory[0x64];
static const uint8_t four_personal_offsets[]={0x37,0x43,0x45,0x47,0x48,
    0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,
    0x56,0x57,0x58,0x5C,0x5D,0x60};
static void four_inventory_project(const uint8_t *inventory) {
    for(unsigned i=0;i<sizeof four_personal_offsets;++i) {
        unsigned off=four_personal_offsets[i];psx_mod_write_byte(PLAY+off,inventory[off]);
    }
}
static void four_form(unsigned seat) {
    unsigned form=four_roster.hero[seat];
    psx_mod_write_byte(PLAY+0x43,(uint8_t)mmx4_roster_character(form));
    psx_mod_write_byte(PLAY+0x37,form==MMX4_HERO_ULTIMATE_X || form==MMX4_HERO_BLACK_ZERO);
    psx_mod_write_byte(PLAY+0x47,(uint8_t)mmx4_roster_armor(form));
    /* Native capsule800C7094..712C: complete Ultimate = A7:0F,B8:02.
     * Fourth Armor's stock-charge arm has B8:01. Native progress is scoped
     * per seat; this roster never writes memory-card data. */
    psx_mod_write_byte(PLAY+0x48,form==MMX4_HERO_ULTIMATE_X?2:form==MMX4_HERO_FORCE_X?1:0);
    if(!mmx4_roster_character(form)) {
        psx_mod_write_byte(PLAYER+0xA7,(uint8_t)mmx4_roster_armor(form));
        psx_mod_write_byte(PLAYER+0xB8,psx_mod_read_byte(PLAY+0x48));
    }
    select_x_variant(form==MMX4_HERO_ULTIMATE_X);
}
static void four_enter(unsigned seat) {
    four_current=seat;four_target=seat;projected=1;
    capture_player(&first);capture(PLAY,four_saved_inventory,sizeof four_saved_inventory);
    for(unsigned i=0;i<9;++i)saved_resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);
    saved_dirty=psx_mod_read_byte(0x80166BB0u);
    project_player(&four_seats[seat].player);four_inventory_project(four_seats[seat].inventory);
    four_form(seat);counterpart=mmx4_roster_character(four_roster.hero[seat]);
    CharacterAssets *a=&assets[counterpart];
    uint32_t values[9]={a->sprites,a->assembly,a->colors,a->palette,
        a->menu[0],a->menu[1],a->menu[2],a->menu[3],a->menu[4]};
    for(unsigned i=0;i<9;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],values[i]);
    project(a->palette,four_seats[seat].palette,sizeof four_seats[seat].palette);
}
static void four_leave(void) {
    unsigned seat=four_current;
    capture_player(&four_seats[seat].player);
    capture(PLAY,four_seats[seat].inventory,sizeof four_seats[seat].inventory);
    capture(assets[counterpart].palette,four_seats[seat].palette,sizeof four_seats[seat].palette);
    project_player(&first);four_inventory_project(four_saved_inventory);
    for(unsigned i=0;i<9;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],saved_resources[i]);
    psx_mod_write_byte(0x80166BB0u,saved_dirty);
    select_x_variant(four_roster.hero[0]==MMX4_HERO_ULTIMATE_X);
    counterpart=mmx4_roster_character(four_roster.hero[0])^1u;
    projected=four_current=0;
}
static const uint8_t *four_body(unsigned seat) {
    return seat==0?first.body:four_seats[seat].player.body;
}
static int four_alive(unsigned seat) {
    if(seat>=four_roster.count)return 0;
    const uint8_t *b=four_body(seat);
    return b[0] && b[4]==1 && (b[0x5C]&127u);
}
static void four_first(void) {
    if(!projected)capture_player(&first);
}
static int four_free_scene_owner(void) {
    if(!mmx4_coop_free_cutscenes_enabled() || !enrolled || failed)return -1;
    int owner=-1;
    for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
        const uint8_t *body=four_body(seat);
        if(body[0xC4])return -1;
        if(body[0xC0] && owner<0)owner=(int)seat;
    }
    return owner;
}
static int four_free_scene_for(unsigned seat) {
    int owner=four_free_scene_owner();
    return owner>=0 && owner!=(int)seat && four_alive(seat) && !four_body(seat)[0xC0];
}
static int four_controls(CPUState *cpu,uint32_t address) {
    if(four_controls_call || !enrolled || failed || psx_mod_local_view_scope())return 0;
    four_first();unsigned seat=projected?four_current:0;
    uint8_t freeze=four_free_scene_for(seat)?psx_mod_read_byte(PLAY+0x10):0;
    if(!freeze)return 0;
    four_controls_call=1;psx_mod_write_byte(PLAY+0x10,0);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_byte(PLAY+0x10,freeze);four_controls_call=0;
    psx_mod_counter_add("mmx4.coop4.free-cutscene-control-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static uint32_t four_digest(void) {
    uint32_t hash=hash_word(2166136261u,four_roster.count);
    hash=hash_bytes(hash,four_roster.hero,sizeof four_roster.hero);
    hash=hash_word(hash,enrolled);hash=hash_word(hash,failed);hash=hash_word(hash,frames);
    hash=hash_word(hash,mmx4_coop_free_cutscenes_enabled());
    const uint32_t transients[]={four_busy,four_current,projected,four_camera_call,four_controls_call,four_collision,
        four_actor_call,four_pending,four_function,four_reaction,four_reaction_actor,four_scan_call,
        four_supplemental,four_bounds_call,four_death_call,four_allocation_call,four_effect_call,x_variant};
    for(unsigned i=0;i<sizeof transients/sizeof transients[0];++i)hash=hash_word(hash,transients[i]);
    hash=hash_bytes(hash,(const uint8_t*)four_menu_previous,sizeof four_menu_previous);
    hash=hash_word(hash,four_door.actor);hash=hash_word(hash,four_door.owner);
    hash=hash_word(hash,four_door.participants);hash=hash_word(hash,four_door_call);
    hash=hash_word(hash,four_camera_owner);
    hash=hash_bytes(hash,(const uint8_t*)four_door.feet,sizeof four_door.feet);
#ifndef PSX_NO_DEBUG_TOOLS
    hash=hash_word(hash,four_fixture_protection);
#endif
    if(projected) {
        hash=hash_player(hash,&first);hash=hash_bytes(hash,four_saved_inventory,sizeof four_saved_inventory);
        for(unsigned i=0;i<9;++i)hash=hash_word(hash,saved_resources[i]);
        hash=hash_word(hash,saved_dirty);
    }
    hash=hash_word(hash,four_placement_table);hash=hash_bytes(hash,four_visited,sizeof four_visited);
    hash=hash_bytes(hash,four_effect_owner,sizeof four_effect_owner);
    hash=hash_bytes(hash,four_particle_owner,sizeof four_particle_owner);
    for(unsigned seat=1;seat<four_roster.count;++seat) {
        hash=hash_player(hash,&four_seats[seat].player);
        hash=hash_bytes(hash,four_seats[seat].inventory,sizeof four_seats[seat].inventory);
        hash=hash_bytes(hash,four_seats[seat].palette,sizeof four_seats[seat].palette);
        hash=hash_bytes(hash,four_seats[seat].camera,sizeof four_seats[seat].camera);
        hash=hash_word(hash,four_seats[seat].previous);
    }
    hash=hash_word(hash,four_contact_count);
    for(unsigned i=0;i<four_contact_count;++i) {
        const FourContact *c=&four_contacts[i];
        hash=hash_word(hash,c->actor);hash=hash_bytes(hash,c->hit,sizeof c->hit);
        hash=hash_bytes(hash,c->sides,sizeof c->sides);hash=hash_bytes(hash,c->carried,sizeof c->carried);
        hash=hash_bytes(hash,c->vehicle_sides,sizeof c->vehicle_sides);
        hash=hash_bytes(hash,c->vehicle_carried,sizeof c->vehicle_carried);
    }
    return mmx4_coop_personal_digest(hash);
}
static void four_diagnostics(void) {
    if(!diagnostic || projected || psx_mod_local_view_scope())return;
    psx_mod_write_word(diagnostic,0x34503458u);psx_mod_write_word(diagnostic+4,four_roster.count);
    psx_mod_write_word(diagnostic+8,enrolled);psx_mod_write_word(diagnostic+12,failed);
    psx_mod_write_word(diagnostic+16,frames);psx_mod_write_word(diagnostic+20,four_digest());
    psx_mod_write_word(diagnostic+0x20,psx_mod_read_byte(PLAY+0x10));
    four_first();
    for(unsigned seat=0;seat<four_roster.count;++seat) {
        const PlayerContext *p=seat?&four_seats[seat].player:&first;
        project(diagnostic+0x100u+seat*0x100u,p->body,sizeof p->body);
        project(diagnostic+0x500u+seat*0x100u,four_seats[seat].camera,sizeof four_seats[seat].camera);
        project(diagnostic+0x900u+seat*0x100u,p->shots,0x100);
    }
    uint8_t menus[4][0x34];mmx4_coop_personal_export(menus);
    project(diagnostic+0xD00u,menus,sizeof menus);
}
static void four_reset(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(four_busy || projected)return;
    enrolled=frames=four_contact_count=0;
    four_placement_table=0;memset(four_visited,0,sizeof four_visited);
    memset(four_effect_owner,0,sizeof four_effect_owner);
    memset(four_particle_owner,0,sizeof four_particle_owner);
    memset(four_seats,0,sizeof four_seats);memset(four_contacts,0,sizeof four_contacts);
    mmx4_coop_personal_reset();
    memset(four_menu_previous,0,sizeof four_menu_previous);
    memset(&four_door,0,sizeof four_door);four_door_call=0;
    four_camera_owner=0;
    four_form(0);
}
static void four_canonical_form(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!projected)four_form(0);
}
static int four_enroll(CPUState *cpu) {
    uint8_t play[0x64],camera[0xFC];capture(PLAY,play,sizeof play);capture(FOUR_CAMERA,camera,sizeof camera);
    memcpy(four_seats[0].camera,camera,sizeof camera);
    capture_player(&first);
    uint32_t x=le32(first.body+8),y=le32(first.body+12),box=le32(first.body+0x68);
    unsigned incoming=play[0x1C]==1;
    unsigned geometry_wait=first.body[0xBE];
    for(unsigned seat=1;seat<four_roster.count;++seat) {
        unsigned form=four_roster.hero[seat],character=mmx4_roster_character(form);
        select_x_variant(form==MMX4_HERO_ULTIMATE_X);
        if(!load_character(character) || !load_stage_palette(character))return 0;
        FourSeat *p=&four_seats[seat];
        memcpy(p->inventory,play,sizeof play);memcpy(p->camera,camera,sizeof camera);
        capture(assets[character].palette,p->palette,sizeof p->palette);
        four_enter(seat);psx_mod_write_byte(PLAY+0x1E,0);
        guest(cpu,0x80035240u,0,0);
        uint32_t spawn=x+(24u*seat<<16),spawn_y=y;
        psx_mod_write_word(PLAYER+8,spawn);psx_mod_write_word(PLAYER+12,spawn_y);
        psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+5,2);
        guest(cpu,0x800350A4u,PLAYER,0);
        if(incoming) {
            guest(cpu,0x80035848u,PLAYER,0);
            psx_mod_write_byte(PLAYER+0xBE,(uint8_t)geometry_wait);
            if(geometry_wait)psx_mod_write_word(PLAYER+0x68,0);
            else guest(cpu,0x80035EA4u,PLAYER,0);
        }else {
            guest(cpu,0x80035EA4u,PLAYER,0);
            uint32_t own_box=psx_mod_read_word(PLAYER+0x68);
            if(box && own_box) {
                int a=(int8_t)psx_mod_read_byte(box+1)+psx_mod_read_byte(box+3);
                int b=(int8_t)psx_mod_read_byte(own_box+1)+psx_mod_read_byte(own_box+3);
                spawn_y+=(uint32_t)((a-b)*65536);psx_mod_write_word(PLAYER+12,spawn_y);
            }
        }
        psx_mod_write_word(PLAYER+0x18,spawn);psx_mod_write_word(PLAYER+0x1C,spawn_y);
        mmx4_coop_spawn_facing_right();
        if(!incoming)guest(cpu,0x8002C614u,PLAYER,0);
        four_leave();project(PLAY,play,sizeof play);project(FOUR_CAMERA,camera,sizeof camera);
    }
    four_form(0);enrolled=1;
    psx_mod_counter_add("mmx4.coop4.enrolled",four_roster.count);
    return 1;
}
static void four_player_pass(CPUState *cpu,uint32_t address) {
    (void)address;
    guest(cpu,0x80035EF0u,0,0);guest(cpu,0x80021C14u,0,0);guest(cpu,0x800311ECu,0,0);
    guest(cpu,0x80021340u,0,0);
    if(psx_mod_read_word(PLAYER+0x68) && !psx_mod_read_byte(PLAYER+0xBE) &&
       !psx_mod_read_byte(PLAYER+0xC4))guest(cpu,0x8002C614u,PLAYER,0);
    guest(cpu,0x80021D84u,0,0);guest(cpu,0x80021CC8u,0,0);
}
static void four_tick(CPUState *cpu,uint32_t address) {
    (void)address;
    if(four_busy || projected || failed || psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+1))return;
    four_busy=1;
    int arrival=psx_mod_read_byte(PLAY+0x1C)==1 && psx_mod_read_byte(PLAY+0x1E) &&
        psx_mod_read_byte(PLAY+0x1F)==1 && !psx_mod_read_byte(PLAYER+5) &&
        psx_mod_read_byte(PLAYER+0x67)==255;
    if(!enrolled && psx_mod_read_byte(PLAYER+4)==1 && (!psx_mod_read_byte(PLAY+0x1C) || arrival)) {
        if(!four_enroll(cpu)) {failed=1;fprintf(stderr,"mmx4.coop4: native enrollment failed\n");}
    }
    if(enrolled && !failed) {
        uint8_t pad[6],camera[0xFC];capture(PAD,pad,sizeof pad);capture(FOUR_CAMERA,camera,sizeof camera);
        four_first();
        for(unsigned seat=1;seat<four_roster.count;++seat) {
            FourSeat *p=&four_seats[seat];
            if(!p->player.body[0] || p->player.body[4]==3)continue;
            unsigned incoming=p->player.body[4]==1 && !p->player.body[5] &&
                p->player.body[0x67]==255 && !p->player.body[0xC0];
            uint8_t arrival_lock=psx_mod_read_byte(PLAY+0x1C);
            four_enter(seat);project(FOUR_CAMERA,p->camera,sizeof p->camera);
            uint16_t buttons=(uint16_t)~sio_get_pad_buttons_slot((int)seat);
            uint16_t input=(uint16_t)((buttons<<8)|(buttons>>8));
            psx_mod_write_half(PAD,input);psx_mod_write_half(PAD+2,p->previous);
            psx_mod_write_half(PAD+4,(uint16_t)(input&~p->previous));p->previous=input;
            /* Added heroes are synthetic gameplay work, not three extra
             * retail worlds competing for one PSX frame budget. ENHANCED
             * runs the native pass at this boundary without charging its
             * CPU work again; actual device time remains canonical. A bounded
             * wait still becomes charged, so device polls cannot hang. */
#if PSX_EXECUTION_ENHANCED
            int charged=0;uint64_t cycles=0;
            psx_mod_call_guest_uncharged_counted(cpu,FOUR_PLAYER_PASS,cpu->gpr[31],0,0,0,0,
                250000u,&charged,&cycles);
            psx_mod_counter_add("mmx4.coop4.extra-player-cycles",cycles);
            if(charged)psx_mod_counter_add("mmx4.coop4.extra-player-budget-fallback",1);
#else
            four_player_pass(cpu,FOUR_PLAYER_PASS);
#endif
            four_leave();project(PAD,pad,sizeof pad);project(FOUR_CAMERA,camera,sizeof camera);
            if(incoming)psx_mod_write_byte(PLAY+0x1C,arrival_lock);
            psx_mod_counter_add("mmx4.coop4.native-player-ticks",1);
        }
        four_first();
        static const char *const shot_counters[]={"mmx4.coop4.p1-active-shot-ticks","mmx4.coop4.p2-active-shot-ticks",
            "mmx4.coop4.p3-active-shot-ticks","mmx4.coop4.p4-active-shot-ticks"};
        for(unsigned seat=0;seat<four_roster.count;++seat) {
            const PlayerContext *p=seat?&four_seats[seat].player:&first;
            for(unsigned shot=0;shot<16;++shot)if(p->shots[shot*0x9Cu] && p->shots[shot*0x9Cu+3]) {
                psx_mod_counter_add(shot_counters[seat],1);break;
            }
        }
        ++frames;
    }
    four_busy=0;four_diagnostics();
}
static int four_camera_update(CPUState *cpu,uint32_t address) {
    if(four_camera_call || projected || !enrolled || failed || psx_mod_read_byte(PLAY)!=6)return 0;
    four_camera_call=1;four_first();
    int scene=four_free_scene_owner();unsigned owner=scene>=0?(unsigned)scene:0;
    if(owner!=four_camera_owner) {
        /* A departing scene owner must not leave a distant P1 in that
         * owner's camera for one frame: native27BE4 would clamp P1 through
         * terrain. New admissions keep their authored bounds and start from
         * the entrant's real origins; a return restores P1's complete view. */
        if(scene<0)project(FOUR_CAMERA,four_seats[0].camera,sizeof four_seats[0].camera);
        else for(unsigned layer=0;layer<3;++layer)
            project(FOUR_CAMERA+layer*0x54+8,four_seats[owner].camera+layer*0x54+8,20);
    }
    four_camera_owner=owner;
    if(owner)four_enter(owner);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    guest(cpu,0x80027D40u,0,0);guest(cpu,0x800281E8u,0,0);
    uint8_t camera[0xFC];capture(FOUR_CAMERA,camera,sizeof camera);
    if(owner)four_leave();four_first();
    memcpy(four_seats[owner].camera,camera,sizeof camera);
    for(unsigned seat=0;seat<four_roster.count;++seat)if(seat!=owner) {
        FourSeat *p=&four_seats[seat];
        unsigned free_scene=(unsigned)four_free_scene_for(seat);
        if(seat)four_enter(seat);
        /* Global stage bounds/enabled state follow the world; each seat keeps
         * the native current/previous/parallax origins from its last update. */
        uint8_t view[0xFC];memcpy(view,free_scene?p->camera:camera,sizeof view);
        if(!free_scene)for(unsigned layer=0;layer<3;++layer)
            memcpy(view+layer*0x54+8,p->camera+layer*0x54+8,20);
        project(FOUR_CAMERA,view,sizeof view);
        uint8_t freeze=free_scene?psx_mod_read_byte(PLAY+0x10):0;
        if(freeze)psx_mod_write_byte(PLAY+0x10,0);
        /* Private follow/parallax work belongs to the synthetic seat, just
         * like its extra controller pass. The canonical camera stays charged. */
#if PSX_EXECUTION_ENHANCED
        int charged=0;uint64_t cycles=0;
        psx_mod_call_guest_uncharged_counted(cpu,FOUR_CAMERA_PASS,cpu->gpr[31],0,0,0,0,
            100000u,&charged,&cycles);
        psx_mod_counter_add("mmx4.coop4.extra-camera-cycles",cycles);
        if(charged)psx_mod_counter_add("mmx4.coop4.extra-camera-budget-fallback",1);
#else
        guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        guest(cpu,0x80027D40u,0,0);guest(cpu,0x800281E8u,0,0);
#endif
        if(freeze)psx_mod_write_byte(PLAY+0x10,freeze);
        capture(FOUR_CAMERA,p->camera,sizeof p->camera);
        if(seat)four_leave();else four_first();
        project(FOUR_CAMERA,camera,sizeof camera);
    }
    four_camera_call=0;four_diagnostics();return mmx4_coop_finish(cpu,result);
}
static void four_camera_pass(CPUState *cpu,uint32_t address) {
    (void)address;
    guest(cpu,0x80027850u,0,0);
    guest(cpu,0x80027D40u,0,0);guest(cpu,0x800281E8u,0,0);
}
static int four_native_door(CPUState *cpu,uint32_t address) {
    if(four_door_call || projected || !enrolled || failed || psx_mod_local_view_scope() ||
       psx_mod_read_byte(PLAY)!=6)return 0;
    uint32_t actor=cpu->gpr[4];
    if(actor<0x80165A30u || actor>=0x80165A30u+32u*0x8Cu ||
       (actor-0x80165A30u)%0x8Cu || psx_mod_read_byte(actor+1)!=8)return 0;
    four_door_call=1;four_first();
    if(four_door.actor==actor && (!psx_mod_read_byte(actor) || !psx_mod_read_byte(actor+4)))
        memset(&four_door,0,sizeof four_door);
    unsigned owner=four_door.actor==actor?four_door.owner:0;
    if(!four_door.actor && psx_mod_read_byte(actor+4)==1 && !psx_mod_read_byte(actor+5)) {
        /* The retail query is read-only. Prefer the first living hero who
         * actually touches the gate, including when P1 is already dead. */
        for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
            if(seat)four_enter(seat);
            unsigned hit=guest(cpu,0x800C1E7Cu,actor,0);
            if(seat)four_leave();
            if(hit) {owner=seat;break;}
        }
    }
    if(owner)four_enter(owner);
    unsigned old_gate=psx_mod_read_byte(PLAYER+0xC4);
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    unsigned new_gate=psx_mod_read_byte(PLAYER+0xC4);
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
    uint32_t box=psx_mod_read_word(PLAYER+0x68);
    if(owner)four_leave();
    if(!four_door.actor && !old_gate && new_gate) {
        four_first();four_door.actor=actor;four_door.owner=owner;
        for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
            four_door.participants|=1u<<seat;
            uint32_t own_box=le32(four_body(seat)+0x68);
            if(box && own_box)four_door.feet[seat]=(int8_t)(
                (int8_t)psx_mod_read_byte(box+1)+psx_mod_read_byte(box+3)-
                (int8_t)psx_mod_read_byte(own_box+1)-psx_mod_read_byte(own_box+3));
        }
        psx_mod_counter_add("mmx4.coop4.group-door-admissions",1);
    }
    if(four_door.actor==actor) {
        unsigned phase=psx_mod_read_byte(actor+5);
        int release=!new_gate || psx_mod_read_byte(actor+4)!=1;
        for(unsigned seat=0;seat<four_roster.count;++seat) {
            if(seat==owner || !(four_door.participants&(1u<<seat)) || !four_alive(seat))continue;
            if(seat)four_enter(seat);
            /* A shared room boundary gathers the team at the entering hero's
             * feet. Keep each native body, HP, animation and inventory. The
             * retail owner alone advances the door animation and world. */
            uint32_t py=y+(uint32_t)(four_door.feet[seat]*65536);
            psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,py);
            psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,py);
            if(release) {
                psx_mod_write_byte(PLAYER+0xC4,0);guest(cpu,0x80035EA4u,PLAYER,0);
            }else {
                if(!psx_mod_read_byte(PLAYER+0xC4))psx_mod_write_byte(PLAYER+0xC4,1);
                if(phase>=2)psx_mod_write_word(PLAYER+0x68,0);
            }
            if(seat)four_leave();
        }
        if(release) {
            memset(&four_door,0,sizeof four_door);
            psx_mod_counter_add("mmx4.coop4.group-door-releases",1);
        }
    }
    four_door_call=0;return mmx4_coop_finish(cpu,result);
}
static int four_normal(CPUState *cpu,uint32_t address) {
    if(four_normal_call || projected || !enrolled || failed)return 0;
    four_first();unsigned eligible=0;int owner=-1;
    for(unsigned seat=0;seat<four_roster.count;++seat) {
        uint16_t input=mmx4_coop_input(seat);
        if(four_alive(seat)) {
            eligible|=1u<<seat;
            if(owner<0 && (input&~four_menu_previous[seat]&0x0800u))owner=(int)seat;
        }
        four_menu_previous[seat]=input;
    }
    if(psx_mod_read_byte(PLAY+0x1C) || psx_mod_read_byte(PLAY+0x10) ||
       psx_mod_read_byte(PLAY+0x0F) || psx_mod_read_byte(0x80141BDCu))owner=-1;
    uint8_t pad[6];capture(PAD,pad,sizeof pad);
    uint16_t edge=psx_mod_read_half(PAD+4)&(uint16_t)~0x0800u;
    if(owner>=0)edge|=0x0800u;
    psx_mod_write_half(PAD+4,edge);four_normal_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);four_normal_call=0;
    project(PAD,pad,sizeof pad);
    if(owner>=0 && psx_mod_read_byte(PLAY)==6 && psx_mod_read_byte(PLAY+1)==2) {
        mmx4_coop_personal_begin(four_roster.count,eligible);
        psx_mod_counter_add("mmx4.coop4.personal-menu-opens",1);
    }
    four_diagnostics();return mmx4_coop_finish(cpu,result);
}
static int four_record_index(uint32_t record,unsigned *index) {
    if(!four_placement_table || record<four_placement_table || (record-four_placement_table)%8u ||
       record-four_placement_table>=sizeof four_visited*8u*8u)return 0;
    *index=(record-four_placement_table)/8u;return 1;
}
static void four_placement_spawned(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(cpu->gpr[17] && four_record_index(cpu->gpr[19],&index))four_visited[index/8u]|=(uint8_t)(1u<<(index&7u));
}
static int four_placement_allocate(CPUState *cpu,uint32_t address) {
    (void)address;unsigned index;
    if(!four_scan_call || cpu->gpr[31]!=0x8002916Cu || !four_record_index(cpu->gpr[19],&index))return 0;
    uint32_t record=cpu->gpr[19];unsigned category=psx_mod_read_byte(record+3u);
    if((four_visited[index/8u]&(1u<<(index&7u))) ||
       (four_supplemental && ((psx_mod_read_byte(record)&0x70u) || category>=3u)))return mmx4_coop_finish(cpu,0);
    return 0;
}
static int four_scan(CPUState *cpu,uint32_t address) {
    if(four_scan_call || projected || !enrolled || failed || psx_mod_local_view_scope())return 0;
    unsigned stage=psx_mod_read_byte(PLAY+12),part=psx_mod_read_byte(PLAY+13);
    if(stage>=15 || part>1)return 0;
    uint32_t table=psx_mod_read_word(0x800F43C8u+stage*8u+part*4u);
    if((table&3u) || table<0x80010000u || table>=0x80200000u)return 0;
    if(table!=four_placement_table) {four_placement_table=table;memset(four_visited,0,sizeof four_visited);}
    four_first();
    for(unsigned i=0;i<sizeof four_visited*8u && table+i*8u+8u<=0x80200000u;++i) {
        uint32_t record=table+i*8u;if(psx_mod_read_byte(record+3u)==255)break;
        int x=(int16_t)psx_mod_read_half(record+4),y=(int16_t)psx_mod_read_half(record+6);unsigned visible=0;
        for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
            int cx=(int16_t)(le32(four_seats[seat].camera+8)>>16),cy=(int16_t)(le32(four_seats[seat].camera+12)>>16);
            if(x>=cx-64 && x<=cx+384 && y>=cy-64 && y<=cy+304)visible=1;
        }
        if(!visible)four_visited[i/8u]&=(uint8_t)~(1u<<(i&7u));
        else if(psx_mod_read_byte(record)&1u)four_visited[i/8u]|=(uint8_t)(1u<<(i&7u));
    }
    four_scan_call=1;uint8_t camera[0xFC];capture(FOUR_CAMERA,camera,sizeof camera);uint32_t result=0;
    for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
        project(FOUR_CAMERA,four_seats[seat].camera,sizeof camera);
        result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    }
    four_supplemental=1;
    for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
        const uint8_t *view=four_seats[seat].camera;
        int x=(int16_t)(le32(view+8)>>16),y=(int16_t)(le32(view+12)>>16);
        CPUState saved=*cpu;cpu->gpr[29]-=32u;
        uint32_t argument=cpu->gpr[29]+16u,previous=psx_mod_read_word(argument);
        cpu->gpr[4]=(uint32_t)(x-64);cpu->gpr[5]=(uint32_t)(x+384);
        cpu->gpr[6]=(uint32_t)(y-64);cpu->gpr[7]=(uint32_t)(y+304);
        psx_mod_write_word(argument,0);cpu->gpr[31]=0x80028FA8u;
        psx_dispatch_call(cpu,0x80028FECu,cpu->gpr[31]);psx_mod_write_word(argument,previous);
        if(cpu->muldiv_ts_done>saved.muldiv_ts_done)saved.muldiv_ts_done=cpu->muldiv_ts_done;
        if(cpu->gte_ts_done>saved.gte_ts_done)saved.gte_ts_done=cpu->gte_ts_done;
        *cpu=saved;
    }
    project(FOUR_CAMERA,camera,sizeof camera);four_supplemental=four_scan_call=0;
    return mmx4_coop_finish(cpu,result);
}
static int four_bounds(CPUState *cpu,uint32_t address) {
    if(four_bounds_call || !enrolled || failed || psx_mod_local_view_scope())return 0;
    uint32_t actor=cpu->gpr[4];
    if((actor&3u) || actor<0x80010000u || actor>0x801FFF00u || (int8_t)psx_mod_read_byte(actor+0x14)<0)return 0;
    unsigned layer=psx_mod_read_byte(actor+0x14);if(layer>=3)return 0;
    four_bounds_call=1;
    uint32_t native=address;uint32_t arg=cpu->gpr[5];
    if(address==0x8002B160u) {native=0x8002B1E8u;cpu->gpr[5]=cpu->gpr[6]=64;}
    if(address==0x8002B288u || address==0x8002B3C0u) {
        native=0x8002B318u;arg=address==0x8002B288u?32:96;cpu->gpr[5]=arg;cpu->gpr[6]=address==0x8002B288u?32:80;
    }
    unsigned draw=address!=0x8002B1E8u && address!=0x8002B160u,visible=0;
    uint8_t camera[0xFC];capture(FOUR_CAMERA,camera,sizeof camera);uint32_t result=draw?0:1;
    for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
        project(FOUR_CAMERA,four_seats[seat].camera,sizeof camera);
        uint32_t candidate=guest(cpu,native,actor,cpu->gpr[5]);
        if(draw) {if(psx_mod_read_byte(actor+3)) {visible=1;result=candidate;}}
        else if(!candidate)result=0;
    }
    if(draw)psx_mod_write_byte(actor+3,(uint8_t)visible);
    project(FOUR_CAMERA,camera,sizeof camera);four_bounds_call=0;return mmx4_coop_finish(cpu,result);
}
static FourContact *four_contact(uint32_t actor) {
    unsigned i=0;while(i<four_contact_count && four_contacts[i].actor<actor)++i;
    if(i<four_contact_count && four_contacts[i].actor==actor)return &four_contacts[i];
    if(four_contact_count==256)return NULL;
    memmove(four_contacts+i+1,four_contacts+i,(four_contact_count-i)*sizeof four_contacts[0]);
    memset(four_contacts+i,0,sizeof four_contacts[0]);four_contacts[i].actor=actor;++four_contact_count;
    return &four_contacts[i];
}
static int four_allocate(CPUState *cpu,uint32_t address) {
    if(four_allocation_call || psx_mod_local_view_scope())return 0;
    four_allocation_call=1;
    uint32_t actor=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    four_allocation_call=0;
    for(unsigned i=0;actor && i<four_contact_count;++i)if(four_contacts[i].actor==actor) {
        --four_contact_count;memmove(four_contacts+i,four_contacts+i+1,(four_contact_count-i)*sizeof four_contacts[0]);
        memset(four_contacts+four_contact_count,0,sizeof four_contacts[0]);break;
    }
    if(address==0x8002AD3Cu && actor>=0x8013E510u && actor<0x8013F310u && (actor-0x8013E510u)%0x70u==0)
        four_effect_owner[(actor-0x8013E510u)/0x70u]=(uint8_t)((projected?four_current:0)+1);
    if(address==0x8002AE50u && actor>=0x80173CA0u && actor<0x801754A0u && (actor-0x80173CA0u)%0x60u==0)
        four_particle_owner[(actor-0x80173CA0u)/0x60u]=(uint8_t)((projected?four_current:0)+1);
    return mmx4_coop_finish(cpu,actor);
}
static unsigned four_stored_charge_owner(uint32_t actor) {
    if(actor<0x80173CA0u || actor>=0x801754A0u || (actor-0x80173CA0u)%0x60u ||
       psx_mod_read_byte(actor+1)!=0x36)return 0;
    return four_particle_owner[(actor-0x80173CA0u)/0x60u];
}
static int four_stored_charge_tick(CPUState *cpu,uint32_t address) {
    if(four_effect_call || !enrolled || failed || psx_mod_local_view_scope())return 0;
    unsigned owner=four_stored_charge_owner(cpu->gpr[4]);
    if(owner<2 || owner>four_roster.count)return 0;
    unsigned previous=projected?four_current:0;
    if(previous==owner-1)return 0;
    four_effect_call=1;
    if(previous)four_leave();four_enter(owner-1);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);four_leave();
    if(previous)four_enter(previous);
    four_effect_call=0;
    psx_mod_counter_add("mmx4.coop4.stored-charge-hud-owner-ticks",1);
    return mmx4_coop_finish(cpu,result);
}
static unsigned four_personal_effect_owner(uint32_t actor) {
    if(actor<0x8013E510u || actor>=0x8013F310u || (actor-0x8013E510u)%0x70u)return 0;
    unsigned type=psx_mod_read_byte(actor+1);
    if(!(type==0 || type==1 || type==2 || type==7 || type==26 || type==27 || type==33))return 0;
    return four_effect_owner[(actor-0x8013E510u)/0x70u];
}
static int four_personal_effect(CPUState *cpu,uint32_t address) {
    if(four_effect_call || !enrolled || failed || psx_mod_local_view_scope())return 0;
    unsigned owner=four_personal_effect_owner(cpu->gpr[4]);
    if(owner<2 || owner>four_roster.count)return 0;
    unsigned previous=projected?four_current:0;
    if(previous==owner-1)return 0;
    four_effect_call=1;
    if(previous)four_leave();four_enter(owner-1);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);four_leave();
    if(previous)four_enter(previous);
    four_effect_call=0;return mmx4_coop_finish(cpu,result);
}
static int four_combat(CPUState *cpu,uint32_t address) {
    if(four_collision || !enrolled || failed || psx_mod_local_view_scope() || psx_mod_read_byte(PLAY)!=6)return 0;
    uint32_t actor=cpu->gpr[4],arg=cpu->gpr[5];
    if((actor&3u) || actor<0x80010000u || actor>0x801FFF00u)return 0;
    unsigned previous=projected?four_current:0;
    if(previous)four_leave();four_first();four_collision=1;
    FourContact *contact=address==0x8002DD04u?four_contact(actor):NULL;
    uint8_t token=psx_mod_read_byte(actor+0x65);uint32_t result=0;
    for(unsigned seat=0;seat<four_roster.count && !result;++seat) {
        if(!four_alive(seat) || four_body(seat)[0xBC])continue;
        if(contact)psx_mod_write_byte(actor+0x65,contact->hit[seat]);
        if(seat)four_enter(seat);
        result=guest(cpu,address,actor,arg);
        if(contact)contact->hit[seat]=psx_mod_read_byte(actor+0x65);
        if(result && result<0x7Fu && address==0x8002DD04u && four_actor_call) {
            four_reaction=mmx4_roster_character(four_roster.hero[seat])+1;four_reaction_actor=actor;
        }
        if(seat)four_leave();
        if(result)psx_mod_counter_add(seat==0?"mmx4.coop4.p1-contact":seat==1?"mmx4.coop4.p2-contact":seat==2?"mmx4.coop4.p3-contact":"mmx4.coop4.p4-contact",1);
    }
    psx_mod_write_byte(actor+0x65,contact?contact->hit[0]:token);
    if(previous)four_enter(previous);
    four_collision=0;return mmx4_coop_finish(cpu,result);
}
static int four_solid(CPUState *cpu,uint32_t address) {
    if(four_collision || projected || !enrolled || failed || psx_mod_local_view_scope())return 0;
    uint32_t actor=cpu->gpr[4],arg=cpu->gpr[5];
    if((actor&3u) || actor<0x80010000u || actor>0x801FFF00u)return 0;
    FourContact *c=four_contact(actor);if(!c)return 0;
    four_collision=1;four_first();
    uint8_t native_carried=psx_mod_read_byte(actor+0x76);
    uint32_t result=guest(cpu,address,actor,arg);
    const unsigned offsets[]={0x72,0x76,0x74,0x78};
    uint8_t canonical[4];for(unsigned i=0;i<4;++i)canonical[i]=psx_mod_read_byte(actor+offsets[i]);
    for(unsigned seat=1;seat<four_roster.count;++seat)if(four_alive(seat)) {
        uint8_t *fields[]={c->sides,c->carried,c->vehicle_sides,c->vehicle_carried};
        four_enter(seat);
        for(unsigned i=0;i<4;++i)psx_mod_write_byte(actor+offsets[i],i==1 && (int8_t)native_carried<0?native_carried:fields[i][seat]);
        guest(cpu,address,actor,arg);
        for(unsigned i=0;i<4;++i) {fields[i][seat]=psx_mod_read_byte(actor+offsets[i]);psx_mod_write_byte(actor+offsets[i],canonical[i]);}
        four_leave();
    }
    four_collision=0;return mmx4_coop_finish(cpu,result);
}
static void four_redirect_actor(CPUState *cpu,uint32_t address) {
    if(projected || four_actor_call || !enrolled || psx_mod_local_view_scope())return;
    uint32_t base=address==0x80021300u?0x8013BED0u:0x8013F328u;
    unsigned count=address==0x80021300u?48:32;
    uint32_t actor=cpu->gpr[4],function=cpu->gpr[2];
    if(actor<base || actor>=base+count*0x9Cu || (actor-base)%0x9Cu ||
       function<0x80010000u || function>=0x8012F800u || (function&3u))return;
    four_pending=actor;four_function=function;cpu->gpr[2]=FOUR_ACTOR_DISPATCH;
}
static void four_update_actor(CPUState *cpu,uint32_t address) {
    (void)address;
    uint32_t actor=four_pending,function=four_function;four_pending=four_function=0;
    if(!actor || !function || four_actor_call)return;
    four_actor_call=1;four_reaction=four_reaction_actor=0;four_first();
    unsigned target=0;uint64_t distance=UINT64_MAX;
    int ax=(int16_t)psx_mod_read_half(actor+10),ay=(int16_t)psx_mod_read_half(actor+14);
    for(unsigned seat=0;seat<four_roster.count;++seat)if(four_alive(seat)) {
        const uint8_t *b=four_body(seat);
        int64_t dx=(int16_t)(le32(b+8)>>16)-ax,dy=(int16_t)(le32(b+12)>>16)-ay;
        uint64_t d=(uint64_t)(dx*dx+dy*dy);
        if(d<distance) {distance=d;target=seat;}
    }
    uint8_t campaign=psx_mod_read_byte(PLAY+0x43);
    if(target) {four_enter(target);psx_mod_write_byte(PLAY+0x43,campaign);}
    uint32_t result=guest(cpu,function,actor,cpu->gpr[5]);
    if(target) {psx_mod_write_byte(PLAY+0x43,(uint8_t)mmx4_roster_character(four_roster.hero[target]));four_leave();}
    four_actor_call=four_reaction=four_reaction_actor=0;mmx4_coop_finish(cpu,result);
}
static void four_attack_character(CPUState *cpu,uint32_t address) {
    (void)address;
    if(four_actor_call && !four_collision && four_reaction && four_reaction_actor==cpu->gpr[16])cpu->gpr[2]=four_reaction-1;
}
static int four_death(CPUState *cpu,uint32_t address) {
    if(four_death_call || !enrolled || failed || cpu->gpr[4]!=PLAYER)return 0;
    unsigned current=projected?four_current:0,partner=0;
    four_first();
    for(unsigned seat=0;seat<four_roster.count;++seat)
        if(seat!=current && four_alive(seat))partner=1;
    if(!partner)return 0;
    uint8_t gates[9],lock=psx_mod_read_byte(PLAY+0x1C);capture(PLAY+0x12,gates,sizeof gates);
    four_death_call=1;uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);four_death_call=0;
    if(address==0x80035A6Cu)project(PLAY+0x12,gates,sizeof gates);
    if(address==0x80035A6Cu || psx_mod_read_byte(PLAYER+4)==2)psx_mod_write_byte(PLAY+0x1C,lock);
    return mmx4_coop_finish(cpu,result);
}
#ifndef PSX_NO_DEBUG_TOOLS
static void four_fixture(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!diagnostic || projected || !enrolled || psx_mod_netplay_is_active())return;
    if(four_fixture_protection) {
        four_first();
        for(unsigned i=0;i<four_roster.count;++i)if((four_fixture_protection&(1u<<i)) && four_alive(i)) {
            if(i)four_enter(i);
            psx_mod_write_byte(PLAYER+0x61,127);
            if(i)four_leave();
        }
    }
    uint32_t sequence=psx_mod_read_word(diagnostic+0x30u);
    if(!sequence || sequence==psx_mod_read_word(diagnostic+0x44u))return;
    unsigned command=psx_mod_read_word(diagnostic+0x34u),seat=psx_mod_read_word(diagnostic+0x38u);
    uint32_t x=psx_mod_read_word(diagnostic+0x3Cu),y=psx_mod_read_word(diagnostic+0x40u);
    unsigned accepted=0;
    if(seat<four_roster.count) {
        four_first();
        uint8_t *body=seat?four_seats[seat].player.body:first.body;
        if(command==1) {
            put32(body+8,x<<16);put32(body+12,y<<16);put32(body+0x18,x<<16);put32(body+0x1C,y<<16);
            accepted=1;
        }else if(command==2 && x<=127) {body[0x5C]=(uint8_t)x;accepted=1;}
        else if(command==3 && x<16) {four_fixture_protection=x;accepted=1;}
        else if(command==4 && x<=32) {
            /* Offline menu fixture: acquire two E-tanks and seed energy.
             * Cursor selection, healing and consumption remain native. */
            psx_mod_write_half(PLAY+0x5A,psx_mod_read_half(PLAY+0x5A)|0x3000u);
            if(seat)four_enter(seat);
            psx_mod_write_byte(PLAY+0x5C,(uint8_t)(0x80u|x));
            psx_mod_write_byte(PLAY+0x5D,(uint8_t)(0x80u|x));
            if(seat)four_leave();accepted=1;
        }
        if(!seat)project_player(&first);
    }
    psx_mod_write_word(diagnostic+0x48u,accepted);psx_mod_write_word(diagnostic+0x44u,sequence);
}
#endif
static int four_primary_sprite(CPUState *cpu,uint32_t address) {
    (void)address;
    if(rendering || projected || !enrolled || failed || !world_draw_visible())return 0;
    uint32_t actor=cpu->gpr[4];
    unsigned stock_owner=four_stored_charge_owner(actor);
    if(stock_owner && stock_owner<=four_roster.count) {
        unsigned seat=stock_owner-1;
        unsigned private_view=psx_mod_local_view_scope()!=0;
        unsigned hud_seat=private_view?local_view_seat:0;
        /* The canonical fallback has only P1's native HUD. Extra-seat
         * displays belong to their private views, including when a private
         * draw is refused. There is no shared/unified four-player HUD. */
        if(mmx4_roster_character(four_roster.hero[seat]) ||
           seat!=hud_seat) {
            psx_mod_counter_add("mmx4.coop4.stored-charge-hud-other-view-hidden",1);
            if(!private_view)psx_mod_counter_add("mmx4.coop4.stored-charge-hud-canonical-hidden",1);
            return mmx4_coop_finish(cpu,0);
        }
        rendering=1;
        if(seat)four_enter(seat);
        counterpart=seat?0:1;
        uint32_t original=psx_mod_read_word(SCRATCH+0x100);
        uint32_t base=arena+((psx_mod_read_word(SCRATCH)&1u)*4u+seat)*FRAME_ARENA;
        uint32_t start=base+four_draw_used[seat];
        psx_mod_write_word(SCRATCH+0x100,start);render_actor(cpu,actor,base,0);
        uint32_t end=psx_mod_read_word(SCRATCH+0x100);
        if(!failed)for(uint32_t at=start;at<end;at+=40) {
            uint32_t expanded=base+40000+(at-base)*2;
            psx_mod_tag_world_primitive(expanded,0);psx_mod_tag_world_primitive(expanded+40,0);
            psx_mod_anchor_hud_primitive(expanded,-1);psx_mod_anchor_hud_primitive(expanded+40,-1);
        }
        four_draw_used[seat]=end-base;
        psx_mod_write_word(SCRATCH+0x100,original);
        if(seat)four_leave();rendering=0;
        psx_mod_counter_add("mmx4.coop4.stored-charge-hud-owner-draws",1);
        return mmx4_coop_finish(cpu,0);
    }
    /* The shared world retains objects for every camera. Reject a foreign
     * view's offscreen sprites before native assembly/packet generation;
     * otherwise distant coordinates can wrap through the GPU's 11-bit XY.
     * Keep canonical simulation draws and screen-relative menus unchanged. */
    if(psx_mod_local_view_scope() && !mmx4_coop_lifecycle_personal_menus() &&
       mmx4_coop_split_sprite_outside(actor)) {
        psx_mod_counter_add("mmx4.coop4.private-sprites-culled",1);
        return mmx4_coop_finish(cpu,0);
    }
    unsigned owner=four_personal_effect_owner(actor);
    if(!(owner || actor==PLAYER || actor==0x80175D58u ||
       (actor>=SHOTS && actor<SHOTS+sizeof first.shots && (actor-SHOTS)%0x9Cu==0) ||
       (actor>=TRAILS && actor<TRAILS+sizeof first.trails && (actor-TRAILS)%0x60u==0) ||
       (actor>=CHARGE_GLOWS && actor<CHARGE_GLOWS+sizeof first.charge_glows && (actor-CHARGE_GLOWS)%0x50u==0)))return 0;
    rendering=1;
    unsigned seat=owner?owner-1:0;
    if(seat>=four_roster.count) {rendering=0;return 0;}
    if(seat)four_enter(seat);
    unsigned character=mmx4_roster_character(four_roster.hero[seat]);
    counterpart=seat?character:character^1u;
    uint32_t original=psx_mod_read_word(SCRATCH+0x100);
    uint32_t base=arena+((psx_mod_read_word(SCRATCH)&1u)*4u+seat)*FRAME_ARENA;
    psx_mod_write_word(SCRATCH+0x100,base+four_draw_used[seat]);render_actor(cpu,actor,base,character);
    four_draw_used[seat]=psx_mod_read_word(SCRATCH+0x100)-base;
    psx_mod_write_word(SCRATCH+0x100,original);
    if(seat)four_leave();rendering=0;
    return mmx4_coop_finish(cpu,0);
}
static int four_render(CPUState *cpu,uint32_t address) {
    /* Native Options also clears these object buckets. Only real world
     * passes may append the extra heroes, as in the two-player renderer. */
    if(four_render_call || projected || failed || !enrolled || !world_draw_visible())return 0;
    psx_mod_counter_add("mmx4.coop4.world-sprite-passes",1);
    four_render_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    rendering=1;
    for(unsigned seat=1;seat<four_roster.count && !failed;++seat) {
        four_enter(seat);
        uint32_t original=psx_mod_read_word(SCRATCH+0x100);
        uint32_t base=arena+((psx_mod_read_word(SCRATCH)&1u)*4u+seat)*FRAME_ARENA;
        psx_mod_write_word(SCRATCH+0x100,base+four_draw_used[seat]);
        if(psx_mod_read_byte(PLAYER+3))render_actor(cpu,PLAYER,base,counterpart);
        if(psx_mod_read_byte(0x80175D58u+3))render_actor(cpu,0x80175D58u,base,counterpart);
        for(unsigned i=0;i<2 && !failed;++i)if(psx_mod_read_byte(CHARGE_GLOWS+i*0x50u+3))render_actor(cpu,CHARGE_GLOWS+i*0x50u,base,counterpart);
        for(unsigned i=3;i && !failed;--i)if(psx_mod_read_byte(TRAILS+(i-1)*0x60u+3))render_actor(cpu,TRAILS+(i-1)*0x60u,base,counterpart);
        for(unsigned i=0;i<16 && !failed;++i)if(psx_mod_read_byte(SHOTS+i*0x9Cu+3))render_actor(cpu,SHOTS+i*0x9Cu,base,counterpart);
        four_draw_used[seat]=psx_mod_read_word(SCRATCH+0x100)-base;
        psx_mod_write_word(SCRATCH+0x100,original);four_leave();
    }
    rendering=four_render_call=0;return mmx4_coop_finish(cpu,result);
}
static int four_world_draw(CPUState *cpu,uint32_t address) {
    if(mmx4_coop_personal_world(cpu,address))return 1;
    if(four_world_draw_call || projected || !enrolled || failed)return 0;
    four_world_draw_call=1;memset(four_draw_used,0,sizeof four_draw_used);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);four_world_draw_call=0;
    return mmx4_coop_finish(cpu,result);
}
static int four_hud(CPUState *cpu,uint32_t address) {
    if(!hud_call && !projected && enrolled && !failed && !psx_mod_local_view_scope() && world_draw_visible()) {
        hud_call=1;
        uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        /* The retail HUD commits P1's delayed damage trail here. Other
         * seats draw only inside restored view transactions, so their +5D
         * trail and +5F delay otherwise stay at their spawn values forever.
         * Advance each at this same canonical boundary, independently of
         * which views are drawn, refused, hidden or owned by a LAN peer. */
        if(psx_mod_read_byte(PLAY+0x1F)) {
            uint32_t frame=psx_mod_read_word(0x80141BD8u);
            for(unsigned seat=1;seat<four_roster.count;++seat)
                mmx4_coop_hud_tick(four_seats[seat].player.body,frame);
            psx_mod_counter_add("mmx4.coop4.hud-state-ticks",1);
        }
        hud_call=0;
        return mmx4_coop_finish(cpu,result);
    }
    if(hud_call || projected || !enrolled || failed || !psx_mod_local_view_scope() || !local_view_seat)return 0;
    /* The existing HUD conversion already retains native icons and flat bars.
     * Its context calls dispatch to this engine's currently selected seat. */
    four_target=local_view_seat;second=four_seats[four_target].player;
    counterpart=mmx4_roster_character(four_roster.hero[four_target]);
    return coop_hud(cpu,address);
}
static int four_draw_view(CPUState *cpu,void *user,uint32_t alpha) {
    (void)alpha;
    if(mmx4_coop_lifecycle_personal_menus())
        return draw_local_view(cpu,user,0);
    uint32_t buffer=*(uint32_t*)user;
    project(FOUR_CAMERA,four_seats[local_view_seat].camera,sizeof four_seats[0].camera);
    gpu_set_gp0_source(0);
    gpu_write_gp0(0x02000000u|psx_mod_read_byte(buffer+0x2Du)|
        (uint32_t)psx_mod_read_byte(buffer+0x2Eu)<<8|(uint32_t)psx_mod_read_byte(buffer+0x2Fu)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x14u)|(uint32_t)psx_mod_read_half(buffer+0x16u)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x18u)|(uint32_t)psx_mod_read_half(buffer+0x1Au)<<16);
    guest(cpu,0x800EA714u,buffer+0x70u,12);guest(cpu,0x80023D68u,0,0);
    if(failed || projected || rendering || hud_call)return 0;
    guest(cpu,0x800EC228u,buffer+0x9Cu,0);draw_local_fade(buffer);return !failed;
}
typedef struct {
    FourSeat seats[4];
    LocalViewContext local;
    CharacterAssets x,spare;
    unsigned variant,counterpart;
} FourViewContext;
static void four_view_save(FourViewContext *context) {
    memcpy(context->seats,four_seats,sizeof four_seats);local_context_save(&context->local);
    context->x=assets[0];context->spare=spare_x;context->variant=x_variant;
    context->counterpart=counterpart;
}
static void four_view_restore(const FourViewContext *context) {
    memcpy(four_seats,context->seats,sizeof four_seats);local_context_restore(&context->local);
    assets[0]=context->x;spare_x=context->spare;x_variant=context->variant;counterpart=context->counterpart;
    four_current=four_target=0;four_render_call=four_world_draw_call=0;
}
static unsigned four_view_survivor(unsigned seat) {
    if(four_alive(seat))return seat;
    for(unsigned i=0;i<four_roster.count;++i)if(four_alive(i))return i;
    return seat;
}
static int four_draw_grid(CPUState *cpu,void *user,uint32_t cell) {
    FourViewContext context;four_view_save(&context);
    local_view_seat=cell;
    if(!mmx4_coop_lifecycle_personal_menus())local_view_seat=four_view_survivor(local_view_seat);
    int ok=four_draw_view(cpu,user,0);
    four_view_restore(&context);
    return ok;
}
static int four_present(CPUState *cpu,uint32_t address) {
    int menu=mmx4_coop_lifecycle_personal_menus();
    int paused_world=mmx4_coop_lifecycle_paused_world();
    if(four_present_call || projected || four_busy || failed || !enrolled ||
       psx_mod_read_byte(PLAY)!=6 || (psx_mod_read_byte(PLAY+1) && !menu && !paused_world) ||
       psx_mod_read_byte(PLAY+0x0Fu) || cpu->gpr[31]!=0x80012080u)return 0;
    uint32_t buffer=psx_mod_read_word(0x80142F80u);if(cpu->gpr[4]!=buffer+0x9Cu)return 0;
    four_present_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);guest(cpu,0x800EA20Cu,0,0);
    int port=psx_netplay_local_port();local_view_seat=port>=0 && (unsigned)port<four_roster.count?(unsigned)port:0;
    four_first();
    if(!menu)local_view_seat=four_view_survivor(local_view_seat);
    uint32_t before=four_digest();
    FourViewContext context;four_view_save(&context);
    PSXModRenderPass rect={sizeof rect,0,psx_mod_read_half(buffer+0x14u),psx_mod_read_half(buffer+0x16u),
        psx_mod_read_half(buffer+0x18u),psx_mod_read_half(buffer+0x1Au)};
    int ok;
    if(local_split) {
        PSXModStereoFrame frame={sizeof frame,1,rect.x,rect.y,rect.w,rect.h};
        ok=four_roster.count==3?psx_mod_render_local_triple(cpu,&frame,four_draw_grid,&buffer):
            psx_mod_render_local_grid(cpu,&frame,four_draw_grid,&buffer);
        psx_mod_counter_add(ok?"mmx4.coop4.local-grid-committed":"mmx4.coop4.local-grid-refused",1);
    }else ok=psx_mod_render_local_view(cpu,&rect,four_draw_view,&buffer);
    four_view_restore(&context);
    if(four_digest()!=before)psx_mod_counter_add("mmx4.coop4.local-view-state-leak",1);
    psx_mod_counter_add(ok?"mmx4.coop4.local-view-committed":"mmx4.coop4.local-view-refused",1);
    four_present_call=0;return mmx4_coop_finish(cpu,result);
}
static void four_boundary(CPUState *cpu,uint32_t address) {
#ifndef PSX_NO_DEBUG_TOOLS
    four_fixture(cpu,address);
#else
    (void)cpu;(void)address;
#endif
    /* Consume Start while the personal menu owns input. A held resume key
     * must not become a fresh gameplay pause edge on the first world frame. */
    if(psx_mod_read_byte(PLAY)==6 && psx_mod_read_byte(PLAY+1)==2)
        for(unsigned seat=0;seat<four_roster.count;++seat)four_menu_previous[seat]=mmx4_coop_input(seat);
    /* READY/arrival draws do not always pass through the ordinary world
     * renderer. Packet arenas still belong to this frame's buffer. */
    memset(four_draw_used,0,sizeof four_draw_used);four_diagnostics();
}
static void four_activate(void) {
    /* Reuse bounded native asset/audio initialization, without activating any
     * two-player gameplay hooks. Repack ARC reservations to leave room for a
     * third retained X body inside the existing 1 MiB mod memory contract. */
    activate_common(1);
    if(failed)return;
    char options[4][24];const char *values[4];
    for(unsigned i=0;i<4;++i) {char name[3]={'p',(char)('1'+i),0};
        if(!psx_mod_current_option_value(name,options[i],sizeof options[i]))options[i][0]=0;values[i]=options[i];}
    if(!mmx4_roster_parse(&four_roster,values)) {failed=1;return;}
    memset(four_seats,0,sizeof four_seats);memset(four_contacts,0,sizeof four_contacts);
    four_contact_count=four_busy=four_current=four_collision=four_camera_call=four_render_call=four_present_call=0;
    four_controls_call=0;
    four_camera_owner=0;
    four_actor_call=four_pending=four_function=four_reaction=four_reaction_actor=0;
    four_scan_call=four_supplemental=four_bounds_call=four_placement_table=0;
    four_death_call=0;
    four_normal_call=0;memset(four_menu_previous,0,sizeof four_menu_previous);
#ifndef PSX_NO_DEBUG_TOOLS
    four_fixture_protection=0;
#endif
    memset(&four_door,0,sizeof four_door);four_door_call=0;
    mmx4_coop_personal_reset();
    four_allocation_call=four_effect_call=four_world_draw_call=0;
    memset(four_effect_owner,0,sizeof four_effect_owner);
    memset(four_particle_owner,0,sizeof four_particle_owner);
    memset(four_visited,0,sizeof four_visited);
    /* Allocation is performed by activate's four-mode-aware layout below. */
    split_views=1;local_split=!psx_mod_netplay_is_active();
    psx_mod_set_local_pair_presentation(0);
    if(four_roster.count==3)psx_mod_set_local_triple_presentation(local_split);
    else psx_mod_set_local_grid_presentation(local_split);
    psx_mod_set_fixed_display_aspect(4,3);
    psx_mod_set_rewind_blocked(1);psx_mod_set_savestate_blocked(1);
    four_form(0);psx_mod_counter_add("mmx4.coop4.activated",1);
    psx_mod_counter_add("mmx4.coop4.diagnostic-address",diagnostic);four_diagnostics();
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop4) {
    static const PSXModNetplayProfile profile={FOUR_ID,MMX4_COOP_FOUR_NETPLAY_COMPATIBILITY,
        0,0,1,1u,"mmx4.widescreen",four_digest,1,FOUR_ID,"coop","cameras","individual",1,3,4,"p4"};
    psx_mod_register_netplay_profile(&profile);
    psx_mod_register_activation_plugin(FOUR_ID,four_activate);
    mmx4_coop_personal_register(FOUR_ID);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x8001FF8Cu,four_normal);
    psx_mod_register_function_entry_plugin(FOUR_ID,0x80035240u,four_reset);
    psx_mod_register_function_entry_plugin(FOUR_ID,0x80012EB8u,four_canonical_form);
    psx_mod_register_function_entry_plugin(FOUR_ID,0x800361F8u,four_canonical_form);
    psx_mod_register_function_entry_plugin(FOUR_ID,0x80021340u,four_tick);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800311ECu,four_controls);
    psx_mod_register_function_entry_plugin(FOUR_ID,0x8001FF50u,four_boundary);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80027850u,four_camera_update);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800C1994u,four_native_door);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800EA4D0u,retained_upload);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800241E8u,four_render);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80023D68u,four_world_draw);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80024334u,four_primary_sprite);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80024E70u,four_hud);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800EA80Cu,four_present);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x8002D9BCu,four_combat);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x8002DD04u,four_combat);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x8002E184u,four_solid);
    const uint32_t allocations[]={0x8002AB74u,0x8002ACA4u,0x8002AC0Cu,0x8002ADBCu,0x8002AD3Cu,0x8002AE50u};
    for(unsigned i=0;i<sizeof allocations/sizeof allocations[0];++i)psx_mod_register_function_filter_plugin(FOUR_ID,allocations[i],four_allocate);
    const uint32_t effects[]={0x800AEAC0u,0x800AED18u,0x800AF22Cu,0x800AEED8u,0x800B28CCu,0x800B2A3Cu,0x800B2AD0u};
    for(unsigned i=0;i<sizeof effects/sizeof effects[0];++i)psx_mod_register_function_filter_plugin(FOUR_ID,effects[i],four_personal_effect);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800B2C8Cu,four_stored_charge_tick);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80033BC8u,four_death);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80035A6Cu,four_death);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x80028E24u,four_scan);
    psx_mod_register_function_filter_plugin(FOUR_ID,0x800293E8u,four_placement_allocate);
    psx_mod_register_instruction_plugin(FOUR_ID,0x80029284u,0xA2620000u,four_placement_spawned);
    const uint32_t bounds[]={0x8002B1E8u,0x8002B160u,0x8002B318u,0x8002B288u,0x8002B3C0u};
    for(unsigned i=0;i<sizeof bounds/sizeof bounds[0];++i)psx_mod_register_function_filter_plugin(FOUR_ID,bounds[i],four_bounds);
    psx_mod_register_instruction_plugin(FOUR_ID,0x80021300u,0x0040F809u,four_redirect_actor);
    psx_mod_register_instruction_plugin(FOUR_ID,0x80021518u,0x0040F809u,four_redirect_actor);
    psx_mod_register_guest_function_plugin(FOUR_ID,FOUR_ACTOR_DISPATCH,four_update_actor);
    psx_mod_register_guest_function_plugin(FOUR_ID,FOUR_PLAYER_PASS,four_player_pass);
    psx_mod_register_guest_function_plugin(FOUR_ID,FOUR_CAMERA_PASS,four_camera_pass);
    const uint32_t reactions[]={0x80050998u,0x80071DE4u,0x80072514u,0x800753FCu,0x80082644u,0x800850DCu,0x80091344u};
    for(unsigned i=0;i<sizeof reactions/sizeof reactions[0];++i)
        psx_mod_register_instruction_plugin(FOUR_ID,reactions[i],0,four_attack_character);
}
