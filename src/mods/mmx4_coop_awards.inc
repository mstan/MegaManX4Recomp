/* The second award is a native frontend presentation, never a second reward.
 * 1FAFC advances the retail controller, particle pool and textbox. Its loader
 * runs only for the campaign; ST0F_UX/UZ share their graphic/map/assembly
 * members. The private controller starts after that load, with its own palette,
 * PL text bank, cameras and UI actors. No PLAY+43 projection is needed. */
typedef struct {
    /* 801396C8 is the native glyph assembly referenced by text actor22.
     * Keep it with the controller pointers at801396B8, before textbox state
     * at801397BC; sharing those glyph records mixes both typewriters. */
    uint8_t play[0x40],motion[0x104],text[0xEC],box[0x18],camera[0xFC];
    uint8_t particles[0x1800],lighting[0x40],red[8],green[8],palette[0x1000];
    uint8_t dirty;
} AwardContext;
static AwardContext award_second;
static uint8_t award_colors[0x1000];
static uint32_t award_play;
static unsigned award_initialized,award_call,award_ready,award_assets_ready;

int mmx4_coop_award_projected(void) {return (int)award_call;}
static int award_active(void) {
    return split_views && !failed && award_assets_ready &&
        psx_mod_read_byte(PLAY)==7 && psx_mod_read_byte(PLAY+0x43)<2;
}
static void award_capture(AwardContext *c) {
    capture(PLAY,c->play,sizeof c->play);
    capture(0x801396B8u,c->motion,sizeof c->motion);
    capture(0x801397BCu,c->text,sizeof c->text);
    capture(0x8013BC28u,c->box,sizeof c->box);
    capture(0x801419B0u,c->camera,sizeof c->camera);
    capture(0x80173CA0u,c->particles,sizeof c->particles);
    capture(0x8013E188u,c->lighting,sizeof c->lighting);
    capture(0x80175E9Cu,c->red,sizeof c->red);
    capture(0x80141BE4u,c->green,sizeof c->green);
    capture(psx_mod_read_word(SCRATCH+0x28),c->palette,sizeof c->palette);
    c->dirty=psx_mod_read_byte(0x80166BB0u);
}
static void award_project(const AwardContext *c) {
    project(PLAY,c->play,sizeof c->play);
    project(0x801396B8u,c->motion,sizeof c->motion);
    project(0x801397BCu,c->text,sizeof c->text);
    project(0x8013BC28u,c->box,sizeof c->box);
    project(0x801419B0u,c->camera,sizeof c->camera);
    project(0x80173CA0u,c->particles,sizeof c->particles);
    project(0x8013E188u,c->lighting,sizeof c->lighting);
    project(0x80175E9Cu,c->red,sizeof c->red);
    project(0x80141BE4u,c->green,sizeof c->green);
    project(psx_mod_read_word(SCRATCH+0x28),c->palette,sizeof c->palette);
    psx_mod_write_byte(0x80166BB0u,c->dirty);
}
static int award_activate(void) {
    award_initialized=award_call=award_ready=award_assets_ready=0;
    memset(&award_second,0,sizeof award_second);
    award_play=psx_mod_alloc_guest_memory(0x64,16);
    if(!award_play)return 0;
    /* Both character rows at800EE4DA/800EE4FA select file53. COL0F_U1 is
     * another native scene, not Zero's reward palette. */
    uint8_t file[6144];uint32_t size=0;Mmx4Asset colors;
    if(!psx_mod_read_disc_file("ARC/COL0F_U0.ARC",file,sizeof file,&size) ||
       !mmx4_coop_arc_asset(file,size,0,&colors) || colors.type!=5 ||
       colors.size!=sizeof award_colors)return 0;
    memcpy(award_colors,colors.data,colors.size);
    award_assets_ready=1;return 1;
}
/* a0 points at a private frontend record. Helpers that use absolute PLAY
 * addresses see the same phase flags during the native UI update. The
 * campaign byte and all persistent progression bytes stay canonical. */
static void award_loaded(CPUState *cpu,uint32_t address);
static int award_update_ui(CPUState *cpu,uint32_t address) {
    if(!award_call) {award_loaded(cpu,address);return 0;}
    uint8_t play[0x40];capture(award_play,play,sizeof play);project(PLAY,play,sizeof play);
    guest(cpu,0x80021858u,0,0);
    guest(cpu,0x80021D20u,0,0);
    capture(PLAY,play,sizeof play);project(award_play,play,sizeof play);
    return mmx4_coop_finish(cpu,0);
}
static int award_finish(CPUState *cpu,uint32_t address) {
    (void)address;
    if(award_call) {award_ready=1;return mmx4_coop_finish(cpu,0);}
    /* A longer partner description must finish before the one shared fade
     * and Save/Continue handoff. Both descriptions use P1's native input. */
    if(award_active() && award_initialized && !award_ready)
        return mmx4_coop_finish(cpu,0);
    return 0;
}
static void award_step(CPUState *cpu) {
    if(award_ready)return;
    AwardContext canonical;award_capture(&canonical);
    uint8_t scratch[0x400];capture(SCRATCH,scratch,sizeof scratch);
    unsigned character=psx_mod_read_byte(PLAY+0x43)^1u;
    uint8_t play[0x64];capture(PLAY,play,sizeof play);
    memcpy(play,award_second.play,sizeof award_second.play);play[0x43]=(uint8_t)character;
    project(award_play,play,sizeof play);
    award_project(&award_second);
    psx_mod_write_word(SCRATCH+0x48,assets[character].menu[4]);
    uint32_t data[32],control[32];
    memcpy(data,cpu->gte_data,sizeof data);memcpy(control,cpu->gte_ctrl,sizeof control);
    award_call=1;
    /* This finite UI pass cannot take an interrupt while RAM is projected.
     * Its shared fade/music requests and renderer are intercepted below. */
    psx_mod_call_guest_uncharged(cpu,0x8001FAFCu,cpu->gpr[31],award_play,0,0,0,0,NULL);
    award_call=0;
    memcpy(cpu->gte_data,data,sizeof data);memcpy(cpu->gte_ctrl,control,sizeof control);
    award_capture(&award_second);
    capture(award_play,award_second.play,sizeof award_second.play);
    award_project(&canonical);
    project(SCRATCH,scratch,sizeof scratch);
}
static int award_controller(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(award_call || psx_mod_local_view_scope())return 0;
    if(!award_active()) {award_initialized=award_ready=0;return 0;}
    unsigned initialize=!psx_mod_read_byte(PLAY+1) && !psx_mod_read_byte(PLAY+2);
    if(initialize)award_initialized=award_ready=0;
    else if(award_initialized)award_step(cpu);
    /* The first retail call may suspend for CD loading; leave its loader on
     * the ordinary dispatcher, rather than nesting it in a host callback. */
    return 0;
}
static int award_shared_request(CPUState *cpu,uint32_t address) {
    (void)address;
    return award_call?mmx4_coop_finish(cpu,0):0;
}
static void award_loaded(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(award_call || !award_active() || award_initialized ||
       psx_mod_read_byte(PLAY+1) || psx_mod_read_byte(PLAY+2)!=1)return;
    award_capture(&award_second);
    unsigned character=psx_mod_read_byte(PLAY+0x43)^1u;
    memcpy(award_second.palette,award_colors,sizeof award_second.palette);
    unsigned boss=psx_mod_read_byte(PLAY+0x26);
    if(!character && boss>=1 && boss<=8)
        capture(psx_mod_read_word(SCRATCH+0x30)+(boss-1)*0xC0u,
            award_second.palette+0xAE0,0xC0);
    award_initialized=1;
}
int mmx4_coop_award_story_instruction(CPUState *cpu,uint32_t address) {
    if(!award_call)return 0;
    if(address==0x80022554u)return 1; /* Private PL bank already installed. */
    unsigned reg=2;
    if(address==0x800224D4u || address==0x80022928u)reg=4;
    if(address==0x80022880u || address==0x800228B4u)reg=3;
    cpu->gpr[reg]=psx_mod_read_byte(PLAY+0x43)^1u;return 1;
}
static int award_draw(CPUState *cpu,unsigned seat) {
    if(!award_active() || !award_initialized)return 0;
    if(seat)award_project(&award_second);
    /* 2A484 produces the current flash/shade palette without advancing the
     * award. Upload through the renderer's journalled path, as personal menus
     * do, so authoritative CPU and backend VRAM both return unchanged. */
    psx_mod_write_byte(0x80166BB0u,(uint8_t)(psx_mod_read_byte(0x80166BB0u)|4u));
    guest(cpu,0x8002A484u,0,0);
    uint16_t palette[256*8];
    for(unsigned i=0;i<256*8;++i)palette[i]=psx_mod_read_half(0x80141F70u+i*2u);
    gr_vram_transfer_in(0,480,256,8,palette);
    guest(cpu,0x80023D68u,0,0);return 1;
}
static int award_snapshot(Mmx4SnapshotIO *s) {
    mmx4_snapshot_fixed(s,award_play);
    unsigned initialized=mmx4_snapshot_integer(s,award_initialized,1,1);
    unsigned ready=mmx4_snapshot_integer(s,award_ready,1,1);
    if(ready && !initialized)s->ok=0;
    if(s->mode==MMX4_SNAPSHOT_VALIDATE && initialized && s->ok &&
       s->pos<=s->len && sizeof award_second<=s->len-s->pos) {
        const uint8_t *play=s->in+s->pos;
        if(play[0]!=7 || play[1]>1 ||
           (!play[1] && play[2]!=1) || (play[1] && play[2]>6) ||
           play[0x26]<1 || play[0x26]>8)s->ok=0;
    }
    if(s->mode==MMX4_SNAPSHOT_READ) {
        award_initialized=initialized;award_ready=ready;award_call=0;
    }
    mmx4_snapshot_bytes(s,&award_second,sizeof award_second);
    return s->ok;
}
