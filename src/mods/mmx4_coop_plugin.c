/* SLUS-00561 co-op development spike. All hook addresses are verified against
 * the original executable; generated C is never patched. The world runs once,
 * with a second native player/controller/projectile pass over private context.
 */
#include "mod_plugins.h"
#include "execution_profile.h"
#include "cpu_state.h"
#include "sio.h"
#include "mmx4_coop_assets.h"
#include "mmx4_coop_memory.h"
#include "mmx4_coop_texture_cache.h"
#include "mmx4_coop_internal.h"
#include "mmx4_diagnostics.h"
#include "mmx4_coop_debug_protocol.h"
#include "mmx4_coop_views.h"
#include "mmx4_cheats.h"
#include "mod_netplay.h"
#include "psx_netplay.h"
#include "mod_snapshot.h"
#include "gpu.h"
#include "gpu_render.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ID "mmx4.coop"
#define PLAYER 0x801418C8u
#define PLAY 0x801721C0u
#define SCRATCH 0x1F800000u
#define SHOTS 0x801406F8u
#define TRAILS 0x80141AB0u
#define CHARGE_GLOWS 0x8013E470u
#define PAD 0x80166C08u
#define FRAME_ARENA 0x40000u
typedef struct {
    uint8_t body[0xE4], shots[0x9C0], trails[0x120], charge_glows[0xA0];
    uint8_t double_body[0xE4],vehicle[0xB0];
} PlayerContext;
typedef struct {
    uint32_t sprites, assembly, colors, palette;
    uint32_t menu[5],menu_gfx;
    uint16_t *ui_pixels;
    uint8_t *file; uint32_t size; Mmx4Asset compressed; unsigned loaded,initialized;
} CharacterAssets;
typedef struct {
    uint8_t *file;uint32_t size;
    Mmx4Asset compressed,assembly,colors;
    unsigned loaded;
} XArchive;
typedef struct { uint32_t source; uint16_t id,tile,tiles; uint8_t variant,colors[32]; } SpriteBank;
typedef struct {uint16_t page,clut,uv,id,colors[16];unsigned character;} UiBank;
typedef struct {
    uint8_t *file,*colors;uint16_t *pixels;
    uint32_t size,assembly;Mmx4Asset poses;uint16_t origin[2];unsigned loaded;
} BoardAssets;
static BoardAssets board_assets[2];
static uint32_t board_heroes[2];
static uint32_t board_companions[2];
static unsigned board_call;
static PlayerContext second,first;
static CharacterAssets assets[2];
static XArchive ultimate_archive;
static unsigned x_variant,ultimate_armor_x,black_zero;
static SpriteBank banks[4096];
static UiBank ui_banks[4096];
static SpriteBank view_banks[4096];
static UiBank view_ui_banks[4096];
static unsigned view_bank_count,view_ui_bank_count,local_view_call,local_view_seat;
static unsigned ui_bank_count,hud_call;
static unsigned hud_side_by_side;
static unsigned boss_door_both;
static uint32_t hud_arena;
static uint32_t world_packet_used;
static unsigned bank_count,inside,enrolled,failed,rendering;
/* Host-only reporting history; excluded from snapshots and simulation hashes. */
static unsigned traced_failure;
static uint32_t arena,diagnostic,frames;
static uint32_t saved_resources[9];
static const unsigned resource_offsets[9]={0x14,0x1C,0x24,0x28,0x34,0x38,0x3C,0x44,0x48};
static uint8_t saved_dirty;
static uint8_t saved_cheat;
static uint16_t previous_input;
static unsigned counterpart;
static unsigned projected;
static unsigned camera_call;
static unsigned camera_update_call;
static unsigned split_views;
static unsigned local_split,local_aspect_set,local_shared_menu,local_board_view;
static uint32_t fixture_sequence;
static unsigned fixture_invulnerable;
#ifdef MMX4_COOP_NETPLAY_DEBUG
static Mmx4DebugDecoder development_decoder;
#endif
static unsigned menu_texture_owner=2;
static unsigned upload_guard;
/* Original executable constants, reconstructed before guest code is mapped. */
static uint16_t ui_layout[11][2];
static unsigned ui_layout_ready;
static void diagnostics(void);

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8));
}
static void capture(uint32_t a,void *data,size_t n) {
    mmx4_coop_capture(a,data,n);
}
static void project(uint32_t a,const void *data,size_t n) {
    mmx4_coop_project(a,data,n);
}
static void capture_player(PlayerContext *p) {
    capture(PLAYER,p->body,sizeof p->body);capture(SHOTS,p->shots,sizeof p->shots);
    capture(TRAILS,p->trails,sizeof p->trails);capture(0x80175D58,p->double_body,sizeof p->double_body);
    capture(CHARGE_GLOWS,p->charge_glows,sizeof p->charge_glows);
    capture(MMX4_VEHICLE,p->vehicle,sizeof p->vehicle);
}
static void project_player(const PlayerContext *p) {
    project(PLAYER,p->body,sizeof p->body);project(SHOTS,p->shots,sizeof p->shots);
    project(TRAILS,p->trails,sizeof p->trails);project(0x80175D58,p->double_body,sizeof p->double_body);
    project(CHARGE_GLOWS,p->charge_glows,sizeof p->charge_glows);
    project(MMX4_VEHICLE,p->vehicle,sizeof p->vehicle);
}
void mmx4_coop_flip_second(void) {
    if(projected || psx_mod_local_view_scope())return;
    /* Match800C4F40/800C5078: mirror the body and active native shots,
     * including their previous Y, once about the Cyber room's2048 axis. */
    put32(second.body+12,0x08000000u-le32(second.body+12));
    put32(second.body+28,0x08000000u-le32(second.body+28));
    for(unsigned i=0;i<16;++i) {
        uint8_t *shot=second.shots+i*0x9C;
        if(shot[0]) {
            put32(shot+12,0x08000000u-le32(shot+12));
            put32(shot+28,0x08000000u-le32(shot+28));
        }
    }
}
static uint32_t guest(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    uint32_t r[32],data[32],ctrl[32],pc=cpu->pc,hi=cpu->hi,lo=cpu->lo;
    memcpy(r,cpu->gpr,sizeof r);memcpy(data,cpu->gte_data,sizeof data);memcpy(ctrl,cpu->gte_ctrl,sizeof ctrl);
    cpu->pc=0;cpu->gpr[4]=a0;cpu->gpr[5]=a1;
    psx_dispatch_call(cpu,address,r[31]);uint32_t result=cpu->gpr[2];
    memcpy(cpu->gpr,r,sizeof r);memcpy(cpu->gte_data,data,sizeof data);memcpy(cpu->gte_ctrl,ctrl,sizeof ctrl);
    cpu->pc=pc;cpu->hi=hi;cpu->lo=lo;return result;
}
static int prepare_ui_layout(void) {
    if(ui_layout_ready)return 1;
    uint32_t size=0;
    if(!psx_mod_read_disc_file("SLUS_005.61",NULL,0,&size) || size<0x800 || size>0x200000u)return 0;
    uint8_t *file=malloc(size);if(!file)return 0;
    int valid=psx_mod_read_disc_file("SLUS_005.61",file,size,&size) && size>=0x800 &&
        !memcmp(file,"PS-X EXE",8);
    uint32_t base=valid?le32(file+0x18):0,bytes=valid?le32(file+0x1C):0;
    uint32_t delta=0x800F1614u-base;
    if(!valid || base>0x800F1614u || bytes>size-0x800u || delta>bytes ||
       sizeof ui_layout>bytes-delta)valid=0;
    if(valid)for(unsigned type=0;type<11;++type) {
        const uint8_t *entry=file+0x800u+delta+type*4u;
        ui_layout[type][0]=(uint16_t)(entry[0]|entry[1]<<8);
        ui_layout[type][1]=(uint16_t)(entry[2]|entry[3]<<8);
        if(ui_layout[type][0]>960 || ui_layout[type][1]>496)valid=0;
    }
    free(file);ui_layout_ready=(unsigned)valid;return valid;
}
static int retain_ui(CharacterAssets *a,const Mmx4Asset *v) {
    unsigned type=v->type&255,layout=(v->type>>8)&255;
    if(!ui_layout_ready || type>10 || layout>2 || v->size%2048)return 0;
    unsigned x=ui_layout[type][0],y=ui_layout[type][1],base_y=y&256;
    for(size_t at=0;at<v->size;at+=2048) {
        if(x+64>1024 || y+16>512)return 0;
        for(unsigned row=0;row<16;++row)for(unsigned column=0;column<64;++column) {
            const uint8_t *p=v->data+at+(row*64+column)*2;
            a->ui_pixels[(y+row)*1024+x+column]=(uint16_t)(p[0]|p[1]<<8);
        }
        if(y==base_y+(layout==1?160:240)) {x+=64;y=layout==2?176:base_y;}
        else y+=16;
    }
    return 1;
}
static int prepare_ultimate_archive(void) {
    XArchive *u=&ultimate_archive;
    if(u->loaded)return 1;
    if(!assets[0].loaded)return 0;
    if(!u->file) {
        if(!psx_mod_read_disc_file("ARC/PL02_U.ARC",NULL,0,&u->size) || !(u->file=malloc(u->size)))return 0;
        if(!psx_mod_read_disc_file("ARC/PL02_U.ARC",u->file,u->size,&u->size)) {
            free(u->file);u->file=NULL;u->size=0;return 0;
        }
    }
    if(!mmx4_coop_arc_asset(u->file,u->size,2,&u->compressed) || u->compressed.type!=2 ||
       !mmx4_coop_arc_asset(u->file,u->size,9,&u->assembly) || u->assembly.type!=3 ||
       !mmx4_coop_arc_asset(u->file,u->size,0,&u->colors) || u->colors.type!=9 ||
       u->compressed.size>0x34000 || u->assembly.size>0xC000 || u->colors.size>0x1000)return 0;
    /* Original PL02 changes only body/assembly/colors. Keep one fixed guest
     * allocation and the shared normal-X UI/VAB cache; verify that contract
     * directly from the user's archives rather than assuming equal audio. */
    static const unsigned shared[]={1,3,4,5,6,7,8,10,11,12,13,14,15};
    for(unsigned i=0;i<sizeof shared/sizeof shared[0];++i) {
        Mmx4Asset normal,ultimate;
        if(!mmx4_coop_arc_asset(assets[0].file,assets[0].size,shared[i],&normal) ||
           !mmx4_coop_arc_asset(u->file,u->size,shared[i],&ultimate) ||
           normal.type!=ultimate.type || normal.size!=ultimate.size ||
           memcmp(normal.data,ultimate.data,normal.size))return 0;
    }
    u->loaded=1;return 1;
}
static const Mmx4Asset *character_compressed(unsigned character) {
    return character==0 && x_variant?&ultimate_archive.compressed:&assets[character].compressed;
}
static void select_x_variant(unsigned ultimate) {
    ultimate=ultimate!=0;
    if(x_variant!=ultimate) {x_variant=ultimate;assets[0].initialized=0;}
}
static int prepare_character(unsigned character,int initialize_guest) {
    if(character>1 || !prepare_ui_layout())return 0;
    CharacterAssets *a=&assets[character];
    if(a->loaded && (!initialize_guest || a->initialized))return 1;
    const char *name=character?"ARC/PL01_U.ARC":"ARC/PL00_U.ARC";
    if(!a->file) {
        if(!psx_mod_read_disc_file(name,NULL,0,&a->size) || !(a->file=malloc(a->size)))return 0;
        if(!psx_mod_read_disc_file(name,a->file,a->size,&a->size)) {
            free(a->file);a->file=NULL;a->size=0;return 0;
        }
    }
    Mmx4Asset body,assembly,colors;
    if(!mmx4_coop_arc_asset(a->file,a->size,2,&body) || body.type!=2 ||
       !mmx4_coop_arc_asset(a->file,a->size,9,&assembly) || assembly.type!=3 ||
       !mmx4_coop_arc_asset(a->file,a->size,0,&colors) || colors.type!=9)return 0;
    a->compressed=body;
    if(body.size>0x34000 || assembly.size>0xC000 || colors.size>0x1000)return 0;
    if(initialize_guest) {
        if(character==0 && x_variant) {
            if(!prepare_ultimate_archive())return 0;
            body=ultimate_archive.compressed;assembly=ultimate_archive.assembly;colors=ultimate_archive.colors;
        }
        project(a->sprites,body.data,body.size);project(a->assembly,assembly.data,assembly.size);
        project(a->colors,colors.data,colors.size);
    }
    static const unsigned members[5]={6,7,8,14,15};
    static const uint32_t limits[5]={0x1000,0x1000,0x1000,0x1000,0xA000};
    for(unsigned i=0;i<5;++i) {
        Mmx4Asset v;if(!mmx4_coop_arc_asset(a->file,a->size,members[i],&v) || v.size>limits[i])return 0;
        if(initialize_guest)project(a->menu[i],v.data,v.size);
    }
    Mmx4Asset ui,gfx;
    if(!mmx4_coop_arc_asset(a->file,a->size,1,&ui) ||
       !mmx4_coop_arc_asset(a->file,a->size,5,&gfx) || gfx.size!=0x8000)return 0;
    if(!a->loaded) {
        if(!a->ui_pixels)a->ui_pixels=calloc(1024*512,sizeof(uint16_t));
        if(!a->ui_pixels || !retain_ui(a,&ui) || !retain_ui(a,&gfx) ||
           !mmx4_coop_audio_prepare(character,a->file,a->size,0))return 0;
        a->loaded=1;
    }
    if(initialize_guest) {
        project(a->menu_gfx,gfx.data,gfx.size);
        if(!mmx4_coop_audio_load(character,a->file,a->size))return 0;
        a->initialized=1;
    }
    return 1;
}
static int load_character(unsigned character) {
    /* Both immutable audio banks must exist before the first residency bind. */
    return character<2 && prepare_character(0,0) && prepare_character(1,0) &&
        prepare_ultimate_archive() && prepare_character(character,1);
}
static void enter_second(void) {
    projected=1;
    /* P2 follows the one campaign's permanent progress. Its native X body
     * consumes armor bytes; Zero keeps its own character-specific fields. */
    mmx4_coop_lifecycle_follow_progress(second.body,counterpart);
    capture_player(&first);project_player(&second);
    mmx4_coop_lifecycle_project();
    /* PLAY+37 is the one native campaign code flag. Only P1 owns its saved
     * value; P2's character code comes solely from agreed active options. */
    saved_cheat=psx_mod_read_byte(PLAY+0x37);
    psx_mod_write_byte(PLAY+0x37,(uint8_t)mmx4_cheats_character_enabled(counterpart));
    CharacterAssets *a=&assets[counterpart];
    uint32_t values[9]={a->sprites,a->assembly,a->colors,a->palette,a->menu[0],a->menu[1],a->menu[2],a->menu[3],a->menu[4]};
    for(unsigned i=0;i<9;++i) {saved_resources[i]=psx_mod_read_word(SCRATCH+resource_offsets[i]);psx_mod_write_word(SCRATCH+resource_offsets[i],values[i]);}
    saved_dirty=psx_mod_read_byte(0x80166BB0);
}
static void leave_second(void) {
    capture_player(&second);
    project(diagnostic+0x200,second.vehicle,sizeof second.vehicle);
    mmx4_coop_combat_project_end(diagnostic+0x200);
    project_player(&first);
    mmx4_coop_lifecycle_restore();
    psx_mod_write_byte(PLAY+0x37,saved_cheat);saved_cheat=0;
    for(unsigned i=0;i<9;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],saved_resources[i]);
    psx_mod_write_byte(0x80166BB0,saved_dirty);
    projected=0;
    diagnostics();
}
int mmx4_coop_ready(void) { return enrolled && !failed; }
int mmx4_coop_both_door_players(void) {return (int)boss_door_both;}
uint32_t mmx4_coop_counterpart_story_text(void) {
    return assets[counterpart].initialized?assets[counterpart].menu[4]:0;
}
int mmx4_coop_projected(void) { return (int)projected; }
int mmx4_coop_split_views(void) { return (int)split_views; }
int mmx4_coop_alive(unsigned seat) {
    if(mmx4_coop_lifecycle_hidden(seat))return 0;
    if(seat && !enrolled)return 0;
    if((!seat && !projected) || (seat && projected))
        return psx_mod_read_byte(PLAYER) && (psx_mod_read_byte(PLAYER+0x5C)&0x7F)>0 && psx_mod_read_byte(PLAYER+4)<2;
    const uint8_t *body=seat?second.body:first.body;
    return body[0] && (body[0x5C]&0x7F)>0 && body[4]<2;
}
uint8_t *mmx4_coop_second_body(void) {return second.body;}
uint8_t *mmx4_coop_first_body(void) {return first.body;}
uint8_t *mmx4_coop_second_vehicle(void) {return second.vehicle;}
uint8_t *mmx4_coop_first_vehicle(void) {return first.vehicle;}
void mmx4_coop_clear_current_attacks(void) {
    mmx4_coop_combat_clear_current_effects();
    for(unsigned i=0;i<sizeof second.shots;++i)psx_mod_write_byte(SHOTS+i,0);
    /* Native 80035240 seeds each trail's assembly, CLUT and predecessor.
     * Warps cancel the current images without discarding those resources. */
    for(unsigned i=0;i<3;++i) {
        psx_mod_write_byte(TRAILS+i*0x60+3,0);
        psx_mod_write_byte(TRAILS+i*0x60+4,0);
    }
    /* Keep native effect resource pointers initialized; cancel their visible
     * state together with the charge counters below. */
    for(unsigned i=0;i<2;++i) {
        psx_mod_write_byte(CHARGE_GLOWS+i*0x50+3,0);
        psx_mod_write_byte(CHARGE_GLOWS+i*0x50+4,0);
    }
    for(unsigned i=0;i<sizeof second.double_body;++i)psx_mod_write_byte(0x80175D58u+i,0);
    for(unsigned i=0x8C;i<=0x92;++i)psx_mod_write_byte(PLAYER+i,0);
    psx_mod_write_byte(PLAYER+0x98,0);psx_mod_write_byte(PLAYER+0x99,0);
    for(unsigned i=0x9B;i<=0x9F;++i)psx_mod_write_byte(PLAYER+i,0);
}
void mmx4_coop_enter_second(void) {enter_second();}
void mmx4_coop_leave_second(void) {leave_second();}
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    return guest(cpu,address,a0,a1);
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result) {
    cpu->gpr[2]=result;return 1;
}
uint16_t mmx4_coop_input(unsigned seat) {
    uint16_t raw=(uint16_t)~sio_get_pad_buttons_slot(seat);
    if(seat && psx_mod_read_byte(diagnostic+0x20))raw=(uint16_t)~psx_mod_read_half(diagnostic+0x24);
    uint16_t buttons=(uint16_t)~raw;
    mmx4_diagnostics_resolve_input(seat,&buttons);
#ifdef MMX4_COOP_NETPLAY_DEBUG
    if(psx_mod_netplay_is_active() && mmx4_debug_is_packet(buttons))buttons=0xFFFFu;
#endif
    raw=(uint16_t)~buttons;
    return (uint16_t)((raw<<8)|(raw>>8));
}
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat) {
    unsigned character=seat?counterpart:(counterpart^1u);
    if(character==menu_texture_owner || character>1 || !load_character(character))return;
    uint32_t rect=diagnostic+0x80;
    psx_mod_write_half(rect,832);psx_mod_write_half(rect+2,256);
    psx_mod_write_half(rect+4,64);psx_mod_write_half(rect+6,256);
    if(psx_mod_local_view_scope()) {
        /* These are immutable presentation assets. Install them through the
         * renderer's journalled upload, before it mutates CPU VRAM. GP0(A0)
         * streams into CPU VRAM before the backend sees its commit, so it is
         * unsuitable for these reversible own-menu texture switches. */
        uint16_t pixels[64*256];
        for(unsigned i=0;i<64*256;++i)
            pixels[i]=psx_mod_read_half(assets[character].menu_gfx+i*2);
        gr_vram_transfer_in(832,256,64,256,pixels);
        for(unsigned i=0;i<256*2;++i)
            pixels[i]=psx_mod_read_half(assets[character].menu[1]+i*2);
        gr_vram_transfer_in(0,484,256,2,pixels);
    } else {
        guest(cpu,0x800EA4D0,rect,assets[character].menu_gfx);
        /* Native170B0's pause palette also selects the portrait. Use its
         * retained source for either hero. */
        guest(cpu,0x800EA4D0,0x800F1AD0u,assets[character].menu[1]);
    }
    menu_texture_owner=character;
}
/* Native GPU DMA masks its source to physical RAM. Retained character data
 * lives in Expansion 1, so its uploads must use the CPU GP0 transfer path.
 * Drain earlier native work first; keep the original command/cache semantics.
 * This also covers palette uploads performed inside P2's native pause menu. */
static int retained_upload(CPUState *cpu,uint32_t address) {
    (void)address;
    if(upload_guard)return 0;
    uint32_t source=cpu->gpr[5],rect=cpu->gpr[4];
    unsigned owner=2;
    for(unsigned i=0;i<2;++i)
        if(source>=assets[i].sprites && source-assets[i].sprites<0x60000u)owner=i;
    if(owner==2)return 0;
    uint32_t x=psx_mod_read_half(rect),y=psx_mod_read_half(rect+2);
    uint32_t w=psx_mod_read_half(rect+4),h=psx_mod_read_half(rect+6);
    uint32_t words=(w*h+1u)/2u,offset=source-assets[owner].sprites;
    if(!w || w>1024 || !h || h>512 || x>=1024 || y>=512 ||
       (source&3u) || words>(0x60000u-offset)/4u) {
        failed=9;return mmx4_coop_finish(cpu,(uint32_t)-1);
    }
    upload_guard=1;guest(cpu,0x800EA20C,0,0);
    gpu_set_gp0_source(source);gpu_write_gp1(0x04000000u);
    gpu_write_gp0(0x01000000u);gpu_write_gp0(0xA0000000u);
    gpu_write_gp0(x|(y<<16));gpu_write_gp0(w|(h<<16));
    for(uint32_t i=0;i<words;++i) {
        gpu_set_gp0_source(source+i*4);gpu_write_gp0(psx_mod_read_word(source+i*4));
    }
    if(words>=16)gpu_write_gp1(0x04000002u);
    upload_guard=0;return mmx4_coop_finish(cpu,0);
}
static void diagnostics(void) {
    /* Presentation failures also stop P2's controller/HUD. Capture their
     * reason in the portable startup log even if input recording is off.
     * A sandboxed own-view failure is transient until its context restores. */
    if(!psx_mod_local_view_scope()) {
        if(!failed)traced_failure=0;
        else if(failed!=traced_failure) {
            static const char *const reasons[]={"none","initialization-or-enrollment",
                "reserved","reserved","actor-packet-bounds","actor-packet-format",
                "hud-packet-bounds","hud-texture-or-arena","actor-texture","character-upload-bounds"};
            fprintf(stderr,"mmx4.coop: disabled code=%u reason=%s frame=%u mode=%u/%u "
                "stage=%u section=%u checkpoint=%u enrolled=%u banks=%u ui-banks=%u "
                "p2=%u/%u/%u hp=%u x=%d y=%d\n",failed,
                failed<sizeof reasons/sizeof reasons[0]?reasons[failed]:"unknown",frames,
                psx_mod_read_byte(PLAY),psx_mod_read_byte(PLAY+1),
                psx_mod_read_byte(PLAY+0x0C),psx_mod_read_byte(PLAY+0x0D),
                psx_mod_read_byte(PLAY+0x1D),enrolled,bank_count,ui_bank_count,
                second.body[0],second.body[3],second.body[4],second.body[0x5C]&127u,
                (int16_t)(le32(second.body+8)>>16),(int16_t)(le32(second.body+12)>>16));
            fflush(stderr);traced_failure=failed;
        }
    }
    psx_mod_write_word(diagnostic,0x5834434Fu);psx_mod_write_word(diagnostic+4,frames);
    project(diagnostic+0x100,second.body,sizeof second.body);
    project(diagnostic+0x200,second.vehicle,sizeof second.vehicle);
#ifndef PSX_NO_DEBUG_TOOLS
    for(unsigned i=0;i<16;++i)project(diagnostic+0x480+i*8,second.shots+i*0x9C,8);
    project(diagnostic+0x500,second.trails,sizeof second.trails);
    if(!projected) {
        /* One completed Lightning Web is enough for native partner-contact
         * inspection; do not expose or mutate the complete private pool. */
        uint32_t slot=UINT32_MAX;
        for(unsigned i=0;i<16;++i) {
            const uint8_t *shot=second.shots+i*0x9C;
            if(shot[0] && shot[1]==1 && shot[0x75] && le32(shot+0x68)==0x80108800u) {
                slot=i;break;
            }
        }
        psx_mod_write_word(diagnostic+0x6F0,0x57454231u);
        psx_mod_write_word(diagnostic+0x6F4,slot);
        psx_mod_write_word(diagnostic+0x6F8,frames);
        psx_mod_write_word(diagnostic+0x6FC,0);
        if(slot!=UINT32_MAX)project(diagnostic+0x700,second.shots+slot*0x9C,0x9C);
        else for(unsigned i=0;i<0x9C;++i)psx_mod_write_byte(diagnostic+0x700+i,0);
    }
    if(projected)project(diagnostic+0x620,first.vehicle,sizeof first.vehicle);
    else {
        uint8_t vehicle[0xB0];capture(MMX4_VEHICLE,vehicle,sizeof vehicle);
        project(diagnostic+0x620,vehicle,sizeof vehicle);
    }
#endif
    psx_mod_write_word(diagnostic+8,enrolled);psx_mod_write_word(diagnostic+12,failed);
    psx_mod_write_word(diagnostic+16,bank_count);
    psx_mod_write_word(diagnostic+0x1C,1); /* completed-context mirror version */
    if(!projected) {
        uint8_t body[0xE4],play[0x64];
        capture(PLAYER,body,sizeof body);capture(PLAY,play,sizeof play);
        project(diagnostic+0x300,body,sizeof body);project(diagnostic+0x400,play,sizeof play);
    }
}
static void reset_player(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    /* This owned initializer hook also refreshes P1's selected native code.
     * The helper leaves P2's already-installed projection flag untouched. */
    mmx4_cheats_enforce_current();
    if(inside)return;
    mmx4_coop_lifecycle_reset();
    mmx4_coop_combat_reset();
    mmx4_coop_split_reset();
    menu_texture_owner=2;
    enrolled=0;memset(&second,0,sizeof second);previous_input=0;
    if(diagnostic)diagnostics();
}
static int load_stage_palette(unsigned character) {
    unsigned stage=psx_mod_read_byte(PLAY+0xC),section=psx_mod_read_byte(PLAY+0xD);
    char name[32];uint8_t file[6144];uint32_t size=0;Mmx4Asset palette;
    snprintf(name,sizeof name,"ARC/COL%02X_%u%c.ARC",stage,section,character?'Z':'X');
    if(!psx_mod_read_disc_file(name,file,sizeof file,&size) || size!=sizeof file ||
       !mmx4_coop_arc_asset(file,size,0,&palette) || palette.type!=5 || palette.size!=0x1000)
        return 0;
    /* COL's native palette includes the character's three trail CLUTs.
     * Copying the campaign palette gave the counterpart P1's dash colors. */
    project(assets[character].palette,palette.data,palette.size);
    return 1;
}
static int enroll(CPUState *cpu) {
    if(!psx_mod_texture_banks_supported())return 0;
    counterpart=psx_mod_read_byte(PLAYER+2)^1;
    /* A manually entered native X code belongs to P1's campaign only.
     * X/P2 uses its own selected option, never Zero/P1's black-code byte. */
    if(counterpart<=1)select_x_variant(counterpart==1?
        (psx_mod_read_byte(PLAY+0x37)!=0 || mmx4_cheats_character_enabled(0)):
        mmx4_cheats_character_enabled(0));
    if(counterpart>1 || !load_character(counterpart^1u) || !load_character(counterpart))return 0;
    uint8_t play[0x64];
    if(!load_stage_palette(counterpart))return 0;
    capture(PLAY,play,sizeof play);
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
    uint32_t first_box=psx_mod_read_word(PLAYER+0x68);
    unsigned on_chaser=psx_mod_read_byte(PLAYER+0xC5)==0xFF;
    unsigned incoming=psx_mod_read_byte(PLAY+0x1C)==1;
    /* Native player initialization disables the shared camera at 8003558C.
     * Keep P1's actual camera state during either concurrent or late arrival. */
    uint8_t camera_enabled[3]={psx_mod_read_byte(0x801419F4u),
        psx_mod_read_byte(0x80141A48u),psx_mod_read_byte(0x80141A9Cu)};
    uint32_t spawn_x=on_chaser && incoming?x-(32u<<16):x+(24u<<16);
    enter_second();psx_mod_write_byte(PLAY+0x43,(uint8_t)counterpart);
    psx_mod_write_byte(PLAY+0x1E,0);
    guest(cpu,0x80035240,0,0);
    psx_mod_write_word(PLAYER+8,spawn_x);psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+5,2);
    guest(cpu,0x800350A4,PLAYER,0);
    if(!incoming) {
        guest(cpu,0x80035EA4,PLAYER,0);
        uint32_t second_box=psx_mod_read_word(PLAYER+0x68);
        if(!on_chaser && first_box && second_box) {
            /* Share the landing surface, accounting for each character's
             * native box centre/height. X's copied Y places Zero's feet one
             * pixel below a portal top, so its first sweep misses the floor. */
            int first_bottom=(int8_t)psx_mod_read_byte(first_box+1)+psx_mod_read_byte(first_box+3);
            int second_bottom=(int8_t)psx_mod_read_byte(second_box+1)+psx_mod_read_byte(second_box+3);
            y+=(uint32_t)((first_bottom-second_bottom)*65536);
            psx_mod_write_word(PLAYER+12,y);
        }
    }
    psx_mod_write_word(PLAYER+0x18,spawn_x);psx_mod_write_word(PLAYER+0x1C,y);
    mmx4_coop_spawn_facing_right();
    if(on_chaser) {
        guest(cpu,0x80035A24,PLAYER,0);guest(cpu,0x80021C14,0,0);
        /* The bike initializer requests boarding (C5=1). Finish that native
         * handoff before its first acceleration tick requests rider pose47.
         * Otherwise 33974 overwrites the request's substate6=0 with1 during
         * P2's next controller tick, leaving the rider in the standing pose
         * for the whole entrance. Keep native character cleanup/mount state. */
        guest(cpu,0x80033974,PLAYER,0);
        if(incoming)psx_mod_counter_add("mmx4.coop.concurrent-chaser-arrivals",1);
    }
    /* An authored incoming position can be inside the stage's ceiling.
     * Its native delay/beam clears that before installing a normal box. */
    if(!incoming)guest(cpu,0x8002C614,PLAYER,0);
    mmx4_coop_spawn_facing_right();
    leave_second();project(PLAY,play,sizeof play);
    psx_mod_write_byte(0x801419F4u,camera_enabled[0]);
    psx_mod_write_byte(0x80141A48u,camera_enabled[1]);
    psx_mod_write_byte(0x80141A9Cu,camera_enabled[2]);enrolled=1;
    mmx4_coop_lifecycle_enrolled(cpu);
    psx_mod_counter_add("mmx4.coop.enrolled",1);return 1;
}
static void second_tick(CPUState *cpu,uint32_t address) {
    (void)address;
    if(failed) {diagnostics();return;}
    if(inside || projected || psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+1)==2)return;
    inside=1;
    if(!enrolled && mmx4_coop_lifecycle_can_enroll()) {
        if(!enroll(cpu))failed=1;
    }
    if(enrolled && !failed)mmx4_coop_lifecycle_tick(cpu);
    if(enrolled && !failed && mmx4_coop_lifecycle_can_tick()) {
        uint8_t input[6];capture(PAD,input,sizeof input);
        uint8_t camera_view[0xFC],camera_origins[3][20],camera_speed[3];
        /* The authored bike entrance consumes the canonical autoscroll
         * delta at3B898. While READY locks the cameras, the saved Split shadow
         * can have equal current/previous origins; using it removes six pixels
         * of travel each tick and makes P2 chase a moving entrance threshold.
         * Keep the shared entrance camera through P2's native mode0 only, then
         * restore ordinary independent camera movement. No poses are copied. */
        int chaser_entrance=psx_mod_read_byte(PLAY+12)==5 &&
            !psx_mod_read_byte(PLAY+13) && mmx4_coop_alive(0) &&
            psx_mod_read_byte(MMX4_VEHICLE) && !psx_mod_read_byte(MMX4_VEHICLE+1) &&
            second.vehicle[0] && !second.vehicle[1] && second.vehicle[4]==1 &&
            !second.vehicle[5];
        int own_camera=split_views && !chaser_entrance &&
            mmx4_coop_split_camera_copy(1,camera_view);
        if(own_camera)for(unsigned layer=0;layer<3;++layer) {
            uint32_t camera=0x801419B0u+layer*0x54u+8u;
            capture(camera,camera_origins[layer],20);
            project(camera,camera_view+layer*0x54u+8u,20);
        }
        int chaser_scroll=own_camera && second.vehicle[0] && !second.vehicle[1] &&
            second.body[0xC5]==0xFF;
        if(chaser_scroll)for(unsigned layer=0;layer<3;++layer) {
            uint32_t speed=0x801419B0u+layer*0x54u+0x47u;
            camera_speed[layer]=psx_mod_read_byte(speed);
            psx_mod_write_byte(speed,camera_view[layer*0x54u+0x47u]);
        }
        uint16_t raw=mmx4_coop_input(1);
        enter_second();
        psx_mod_write_half(PAD,raw);psx_mod_write_half(PAD+2,previous_input);
        psx_mod_write_half(PAD+4,(uint16_t)(raw&~previous_input));previous_input=raw;
        guest(cpu,0x80035EF0,0,0);
        /* Armor402C4 clamps against current camera limits and kills an
         * occupied Armor below that room. Origins alone are insufficient:
         * the upper Dragoon route can lift P1's floor above a safe P2 rider.
         * Run the one native vehicle update with its rider's actual bounds;
         * camera-area scripts and the on-foot controller keep their scopes. */
        int vehicle_camera=own_camera && second.vehicle[0] && second.vehicle[1]<=1 &&
            second.body[0xC5];
        int chaser_exit=vehicle_camera && !second.vehicle[1];
        uint8_t vehicle_limits[3][16];
        uint8_t vehicle_exit[3];
        if(vehicle_camera)for(unsigned layer=0;layer<3;++layer) {
            uint32_t limits=0x801419B0u+layer*0x54u+0x1Cu;
            capture(limits,vehicle_limits[layer],16);
            project(limits,camera_view+layer*0x54u+0x1Cu,16);
            /* Marine's native Chaser reads +51 to start dismounting. A
             * leading partner's exit must not eject this rider early. */
            if(chaser_exit) {
                uint32_t exit_flag=0x801419B0u+layer*0x54u+0x51u;
                vehicle_exit[layer]=psx_mod_read_byte(exit_flag);
                psx_mod_write_byte(exit_flag,camera_view[layer*0x54u+0x51u]);
            }
        }
        guest(cpu,0x80021C14,0,0);
        if(vehicle_camera)for(unsigned layer=0;layer<3;++layer) {
            project(0x801419B0u+layer*0x54u+0x1Cu,vehicle_limits[layer],16);
            if(chaser_exit)psx_mod_write_byte(0x801419B0u+layer*0x54u+0x51u,vehicle_exit[layer]);
        }
        guest(cpu,0x800311EC,0,0);guest(cpu,0x80021340,0,0);
        mmx4_coop_lifecycle_surface(cpu);
        guest(cpu,0x8002C614,PLAYER,0);
        guest(cpu,0x80021D84,0,0);guest(cpu,0x80021CC8,0,0);
        leave_second();project(PAD,input,sizeof input);++frames;
        if(chaser_scroll) {
            /* Native3C148/3C430/3C68C change camera+47 during a dash.
             * Leaving those writes canonical accelerates P1's next scroll.
             * Keep P2's acceleration for its own native camera update only. */
            uint8_t speed[3];
            for(unsigned layer=0;layer<3;++layer) {
                uint32_t address=0x801419B0u+layer*0x54u+0x47u;
                speed[layer]=psx_mod_read_byte(address);
                psx_mod_write_byte(address,camera_speed[layer]);
            }
            mmx4_coop_split_camera_scroll(1,speed);
        }
        if(own_camera)for(unsigned layer=0;layer<3;++layer)
            project(0x801419B0u+layer*0x54u+8u,camera_origins[layer],20);
        psx_mod_counter_add("mmx4.coop.player-ticks",1);
    }
    diagnostics();inside=0;
}

static int camera_world_ready(void) {
    return mmx4_coop_camera_world_ready(psx_mod_read_byte(PLAY),
        psx_mod_read_byte(PLAY+1),psx_mod_read_byte(PLAYER+4),mmx4_coop_lifecycle_script_owner());
}
static int camera_target(CPUState *cpu,uint32_t address) {
    if(!camera_world_ready())return 0;
    int owner=mmx4_coop_lifecycle_script_owner();
    if(split_views && owner<0 && mmx4_coop_alive(0))return 0;
    if(camera_call || projected || !mmx4_coop_ready() || !mmx4_coop_alive(1) ||
       (owner<0 && (psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C))) ||
       owner==0)return 0;
    unsigned axis=address==0x80027AACu?8:12;
    uint32_t original=psx_mod_read_word(PLAYER+axis);
    int32_t partner=(int32_t)le32(second.body+axis);
    /* Native doors finish when the shared camera reaches their target.
     * Averaging a frozen partner would prevent that equality indefinitely. */
    int32_t target=owner==1 || !mmx4_coop_alive(0)?partner:
        (int32_t)(((int64_t)(int32_t)original+partner)/2);
    camera_call=1;psx_mod_write_word(PLAYER+axis,(uint32_t)target);
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    psx_mod_write_word(PLAYER+axis,original);camera_call=0;
    return mmx4_coop_finish(cpu,result);
}
static void constrain_team(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!camera_world_ready() || split_views || projected || !mmx4_coop_ready() || mmx4_coop_lifecycle_script_owner()>=0 ||
       psx_mod_read_byte(PLAY+0x10) || psx_mod_read_byte(PLAY+0x1C))return;
    if(!split_views && mmx4_coop_alive(0) && mmx4_coop_alive(1) && !second.body[0xC5] &&
       !psx_mod_read_byte(PLAYER+0xC5)) {
        int32_t x=(int32_t)psx_mod_read_word(PLAYER+8),p=(int32_t)le32(second.body+8);
        int64_t distance=(int64_t)x-p,limit=240*65536;
        if(distance>limit || distance < -limit) {
            /* Undo only this frame's outward movement. Never pull a partner
             * through terrain to make the camera catch up. */
            int32_t old_x=(int32_t)psx_mod_read_word(PLAYER+0x18),old_p=(int32_t)le32(second.body+0x18);
            if((distance>0 && x>old_x) || (distance<0 && x<old_x))psx_mod_write_word(PLAYER+8,(uint32_t)old_x);
            if((distance>0 && p<old_p) || (distance<0 && p>old_p))put32(second.body+8,(uint32_t)old_p);
        }
    }
    /* Falling boss entries need the authored deeper floor. Scripted lifts
     * replace that limit with their higher destination before the camera
     * and riders get there; keep the current view's bottom during ascent. */
    int bottom=mmx4_coop_view_death_bottom(
        (int16_t)psx_mod_read_half(0x801419BEu),
        (int16_t)psx_mod_read_half(0x801419D8u));
    for(unsigned seat=0;seat<2;++seat) {
        int y=seat?(int16_t)(le32(second.body+12)>>16):(int16_t)psx_mod_read_half(PLAYER+14);
        if(mmx4_coop_alive(seat) && y-8>=bottom) {
            if(seat)second.body[0x5C]=0x80;else psx_mod_write_byte(PLAYER+0x5C,0x80);
        }
    }
}
static int camera_update(CPUState *cpu,uint32_t address) {
    if(camera_update_call || projected)return 0;
    /* With no survivor, falling back to the singleton camera follows P1's
     * old corpse after a P2-led climb. Keep the last view through the native
     * death sequence; the stage loader will initialize the respawn camera.
     * Healthy players hidden by a shared scene must retain that scene's pan. */
    if(mmx4_coop_ready() && psx_mod_read_byte(PLAY)==6 && psx_mod_read_byte(PLAY+1)<=1 &&
       !mmx4_coop_alive(0) && !mmx4_coop_alive(1) &&
       mmx4_coop_lifecycle_script_owner()<0 &&
       (psx_mod_read_byte(PLAYER+4)>=2 || second.body[4]>=2 ||
        !(psx_mod_read_byte(PLAYER+0x5C)&127u) || !(second.body[0x5C]&127u)))
        return mmx4_coop_finish(cpu,0);
    if(!camera_world_ready())return 0;
    constrain_team(cpu,address);
    int scene_owner=mmx4_coop_lifecycle_script_owner();
    if(!mmx4_coop_camera_scope_requested((int)split_views,mmx4_coop_ready(),
        mmx4_coop_alive(0),mmx4_coop_alive(1),scene_owner))return 0;
    unsigned owner=scene_owner>=0?(unsigned)scene_owner:!mmx4_coop_alive(0);
    if(split_views)mmx4_coop_split_camera_prepare(owner);
    /* 80027850 also checks player A4 and clamps the canonical body against
     * scrolling bounds at 80027BE4. Changing only its target XY can crush a
     * frozen P1 in a P2-led door. Run that one camera pass as its actual owner. */
    camera_update_call=1;
    if(owner) {
        enter_second();
    }
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(owner)leave_second();
    if(split_views)mmx4_coop_split_camera(cpu,owner);
    else mmx4_coop_lifecycle_camera_bounds(cpu,owner);
    camera_update_call=0;
    return mmx4_coop_finish(cpu,result);
}

void mmx4_coop_development_mask_pad(void) {
#ifdef MMX4_COOP_NETPLAY_DEBUG
    PsxNetPad pad;
    if(psx_mod_netplay_is_active() && psx_netplay_sim_pad(0,&pad) &&
       mmx4_debug_is_packet(pad.buttons))
        for(unsigned i=0;i<3;++i)psx_mod_write_half(PAD+i*2u,0);
#endif
}
#ifndef PSX_NO_DEBUG_TOOLS
static void development_teleport(unsigned seat,uint32_t x,uint32_t y) {
    if(!mmx4_coop_alive(seat))return;
    if(seat)enter_second();
    uint32_t dx=x-psx_mod_read_word(PLAYER+8),dy=y-psx_mod_read_word(PLAYER+12);
    if(psx_mod_read_byte(PLAYER+0xC5) && psx_mod_read_byte(MMX4_VEHICLE)) {
        for(unsigned at=8;at<=0x1C;at+=0x10) {
            psx_mod_write_word(MMX4_VEHICLE+at,psx_mod_read_word(MMX4_VEHICLE+at)+dx);
            psx_mod_write_word(MMX4_VEHICLE+at+4,psx_mod_read_word(MMX4_VEHICLE+at+4)+dy);
        }
    }
    psx_mod_write_word(PLAYER+8,x);psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_word(PLAYER+0x18,x);psx_mod_write_word(PLAYER+0x1C,y);
    for(unsigned at=0x20;at<0x2C;at+=4)psx_mod_write_word(PLAYER+at,0);
    /* +2C is acceleration, not residual velocity. Native32558/32608/326C0
     * install4200 for an ordinary fall; clearing it after an idle teleport
     * can leave a healthy character suspended at the destination indefinitely.
     * Preserve action-specific gravity and attached/scripted movement. */
    unsigned action=psx_mod_read_byte(PLAYER+5);
    if(!psx_mod_read_byte(PLAYER+0xC5) && psx_mod_read_byte(PLAYER+4)==1 &&
       action>=2 && action<0x10 && !psx_mod_read_word(PLAYER+0x2C))
        psx_mod_write_word(PLAYER+0x2C,0x4200);
    psx_mod_write_byte(PLAYER+0x89,0);psx_mod_write_byte(PLAYER+0x70,0);
    psx_mod_write_byte(PLAYER+0x71,0);
    if(seat)leave_second();
}
#endif
/* Private QA requests are committed at the native gameplay dispatch, never
 * by interleaving multiple TCP RAM writes with a running world update. */
static int development_fixture(CPUState *cpu,uint32_t address) {
    (void)address;
#ifndef PSX_NO_DEBUG_TOOLS
    if(projected || !diagnostic)return 0;
    Mmx4DebugRequest request={0};
    unsigned command,first_arg,second_arg,third_arg;
    uint32_t x,y,sequence;
    if(psx_mod_netplay_is_active()) {
#ifdef MMX4_COOP_NETPLAY_DEBUG
        PsxNetPad pad;
        if(!psx_netplay_sim_pad(0,&pad))return 0;
        int received=mmx4_debug_decode(&development_decoder,pad.buttons,&request);
        if(development_decoder.have_previous)
            psx_mod_write_word(diagnostic+0x48,development_decoder.previous);
        command=received?request.command:0;
        first_arg=request.first;second_arg=request.second;third_arg=request.third;
        x=(uint32_t)(int32_t)request.x<<16;y=(uint32_t)(int32_t)request.y<<16;
        sequence=request.sequence;
        if(received)psx_mod_write_word(diagnostic+0x4C,sequence);
#else
        return 0;
#endif
    }else {
        sequence=psx_mod_read_word(diagnostic+0x30);
        command=sequence!=fixture_sequence?psx_mod_read_byte(diagnostic+0x34):0;
        first_arg=psx_mod_read_byte(diagnostic+0x35);
        second_arg=psx_mod_read_byte(diagnostic+0x36);third_arg=psx_mod_read_byte(diagnostic+0x37);
        x=psx_mod_read_word(diagnostic+0x40);y=psx_mod_read_word(diagnostic+0x44);
        fixture_sequence=sequence;
    }
    if(enrolled && psx_mod_read_byte(PLAY)==6) {
        if((fixture_invulnerable&1u) && mmx4_coop_alive(0)) {
            psx_mod_write_byte(PLAYER+0x61,127);
            if(psx_mod_read_byte(PLAYER+0xC5) && psx_mod_read_byte(MMX4_VEHICLE))
                psx_mod_write_byte(MMX4_VEHICLE+0x61,127);
        }
        if((fixture_invulnerable&2u) && mmx4_coop_alive(1)) {
            second.body[0x61]=127;
            if(second.body[0xC5] && second.vehicle[0])second.vehicle[0x61]=127;
        }
    }
    if(!command)return 0;
    psx_mod_write_word(diagnostic+0x54,2); /* Rejected until applied below. */
    fprintf(stderr,"mmx4.coop: development command=%u args=%u,%u,%u mode=%u "
        "stage=%u section=%u ready=%u\n",command,first_arg,second_arg,third_arg,
        psx_mod_read_byte(PLAY),psx_mod_read_byte(PLAY+0xC),psx_mod_read_byte(PLAY+0xD),
        mmx4_coop_ready());
    if(command==1) {
        unsigned stage=first_arg,section=second_arg;
        unsigned campaign=third_arg;
        if(stage<=12 && section<=1 && campaign<=1 &&
           (!mmx4_coop_ready() || campaign==psx_mod_read_byte(PLAY+0x43))) {
            psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
            psx_mod_write_byte(PLAY+0xC,(uint8_t)stage);psx_mod_write_byte(PLAY+0xD,(uint8_t)section);
            /* Native stage selection also supplies the reward-stage field.
             * Without it a skipped Maverick fight follows Intro completion. */
            psx_mod_write_byte(PLAY+0x26,(uint8_t)(stage>=1 && stage<=8?stage:0));
            psx_mod_write_word(PLAY,4);psx_mod_write_byte(PLAY+0x1D,0);psx_mod_write_byte(PLAY+0x1E,0);
            psx_mod_write_word(diagnostic+0x38,sequence);
            psx_mod_write_word(diagnostic+0x54,1);
            return mmx4_coop_finish(cpu,0);
        }
    }else if(command==3 && mmx4_coop_ready()) {
        unsigned checkpoint=first_arg;
        unsigned stage=psx_mod_read_byte(PLAY+0xC),section=psx_mod_read_byte(PLAY+0xD);
        /* Only original spawn lists whose bounds have been verified. Frost part2
         * has four pointers at800F417C..800F4188; the last two lead to the
         * late corridor and boss checkpoint (800F3674/800F3698). */
        if((stage==0 && section<=1 && checkpoint<3) ||
           (stage==1 && section==0 && checkpoint<5) ||
           (stage==1 && section==1 && checkpoint<6) ||
           (stage==2 && section==0 && checkpoint<5) ||
           (stage==2 && section==1 && checkpoint<4) ||
           (stage==3 && section<=1 && checkpoint<4) ||
           (stage==4 && section==0 && checkpoint<2) ||
           (stage==4 && section==1 && checkpoint<4) ||
           (stage==8 && section==0 && checkpoint<3) ||
           (stage==8 && section==1 && checkpoint<4) ||
           (stage==6 && section==0 && checkpoint<6) ||
           (stage==7 && section==1 && checkpoint<4) ||
           (stage==10 && section==0 && checkpoint<3) ||
           (stage==11 && section==0 && checkpoint<2) ||
           /* Native80028C1C directory800F42B4: three pointers at
            * 800F4240..4248, records800F3D10/3D34/3D58. */
           (stage==11 && section==1 && checkpoint<3) ||
           ((stage==5 || stage==6) && section==1 && checkpoint<2) ||
           (stage==12 && section==0 && checkpoint<19) ||
           (stage==12 && section==1 && checkpoint<7)) {
            psx_mod_write_byte(PLAY+0x1D,(uint8_t)checkpoint);
            psx_mod_write_byte(PLAY+0x1E,0);psx_mod_write_word(PLAY,5);
            psx_mod_write_word(diagnostic+0x38,sequence);
            psx_mod_write_word(diagnostic+0x54,1);
            return mmx4_coop_finish(cpu,0);
        }
    }else if(command==2 && mmx4_coop_ready()) {
        unsigned seat=first_arg,hp=second_arg;
        if(seat==0) {
            psx_mod_write_byte(PLAYER+0x5C,(uint8_t)hp);
            if(psx_mod_read_byte(PLAYER+0xC5) && psx_mod_read_byte(MMX4_VEHICLE))
                psx_mod_write_byte(MMX4_VEHICLE+0x5C,(uint8_t)(hp&127u));
        }else if(seat==1) {
            second.body[0x5C]=(uint8_t)hp;
            if(second.body[0xC5] && second.vehicle[0])second.vehicle[0x5C]=(uint8_t)(hp&127u);
        }
        if(seat<=1)psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==4 && mmx4_coop_ready() && !psx_mod_read_byte(PLAY+0x1C)) {
        unsigned seat=first_arg;
        if(seat<=2 && (seat==2 || mmx4_coop_alive(seat))) {
            if(seat==2) {development_teleport(0,x,y);development_teleport(1,x,y);}
            else development_teleport(seat,x,y);
            psx_mod_write_word(diagnostic+0x54,1);
        }
    }else if(command==5) {
        fixture_invulnerable=first_arg&3u;
        psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==8 && mmx4_coop_ready() && first_arg>=2 && first_arg<=10 &&
             !psx_mod_netplay_is_active() && psx_mod_read_byte(PLAY)==6 &&
             !psx_mod_read_byte(PLAY+1) && !psx_mod_read_byte(PLAY+0x1C)) {
        /* Offline QA only: prepare an authored movie gateway, bypassing its
         * earlier campaign prerequisites. The original dispatcher plays the
         * movie and owns all completion, dialogue and next-scene writes. */
        if(first_arg==2 || first_arg==3 || first_arg==4 || first_arg==6 || first_arg==7) {
            /* Native2E420 derives its movie/dialogue route from the saved
             * campaign progress. Enter that real frontend initialization;
             * no nested frontend call or guessed movie substate is needed. */
            unsigned progress=first_arg==3?1u:first_arg==4?8u:first_arg==7?3u:0u;
            psx_mod_write_byte(PLAY+0x5F,(uint8_t)progress);
            psx_mod_write_byte(PLAY+0x59,(uint8_t)(first_arg==4?255u:first_arg==7?15u:0u));
            psx_mod_write_word(PLAY,3u);
        }else psx_mod_write_word(PLAY,first_arg==5 || first_arg==10?11u:10u);
        psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==7 && mmx4_coop_ready() && first_arg<2 && second_arg<=8 && third_arg<=2) {
        /* Weapon/progress setup follows the same admitted native boundary
         * in offline and development netplay; P1 still owns all progress. */
        if(third_arg==1)psx_mod_write_byte(PLAY+0x59,255);
        if(third_arg==2) {
            psx_mod_write_byte(PLAY+0x47,15);psx_mod_write_byte(PLAY+0x48,2);
        }
        psx_mod_write_byte(PLAYER+0xB9,psx_mod_read_byte(PLAY+0x59));
        if(first_arg)enter_second();
        psx_mod_write_byte(PLAY+0x60,(uint8_t)second_arg);
        psx_mod_write_byte(PLAYER+0x93,(uint8_t)second_arg);
        guest(cpu,0x80037104u,PLAYER,0);guest(cpu,0x800371E4u,PLAYER,0);
        if(first_arg)leave_second();
        psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==6 && first_arg==252 && second_arg<=32 && third_arg<=2 &&
             mmx4_coop_ready()) {
        /* Private QA: acquire the campaign's two E-tanks, then prepare a
         * seat's energy. Actual selection/healing/consumption is native.
         * The same setup arrives through agreed inputs on both peers. */
        psx_mod_write_half(PLAY+0x5A,(uint16_t)(psx_mod_read_half(PLAY+0x5A)|0x3000u));
        if(third_arg!=1)for(unsigned i=0;i<2;++i)
            psx_mod_write_byte(PLAY+0x5C+i,(uint8_t)(0x80u|second_arg));
        if(third_arg!=0) {
            enter_second();
            for(unsigned i=0;i<2;++i)psx_mod_write_byte(PLAY+0x5C+i,(uint8_t)(0x80u|second_arg));
            leave_second();
        }
        psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==6 && first_arg==253 && second_arg<=1 &&
             mmx4_coop_balance_development_mode(second_arg)) {
        /* Matched saved-fight OFF/ON controls, through the same agreed
         * developer input in local play and netplay. No actor edits. */
        psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==6 && first_arg==254 && second_arg<4 && mmx4_coop_ready() &&
             psx_mod_read_byte(PLAY+0xC)==2 && psx_mod_read_byte(PLAY+0xD)==1 &&
             !psx_mod_read_byte(PLAY+0x1C)) {
        /* Private QA: start the original ice-bird field controller without
         * skipping the intervening platforms, enemies and camera regions.
         * Its native update owns the fade, tile edits and lifetime. This is
         * a synchronized setup fixture, never a production gameplay rule. */
        uint32_t effect=guest(cpu,0x8002AD7Cu,0,0);
        if(effect) {
            psx_mod_write_word(effect,1u|(17u<<8)|(second_arg<<16));
            psx_mod_write_word(effect+4,0);
            psx_mod_write_word(diagnostic+0x54,1);
        }
    }else if(command==6 && first_arg==255 && second_arg<2 && mmx4_coop_ready()) {
        /* Private QA: request the existing native room handoff directly.
         * This shares its ordinary departure, scene gates and terrain-safe
         * return, and is encoded through agreed debug inputs in netplay. */
        mmx4_coop_lifecycle_room_entry(second_arg);
        if(mmx4_coop_lifecycle_hidden(second_arg^1u))
            psx_mod_write_word(diagnostic+0x54,1);
    }else if(command==6 && first_arg<48 && second_arg>0 && second_arg<128) {
        uint32_t actor=0x8013BED0u+first_arg*0x9Cu;
        if(psx_mod_read_byte(actor)) {
            psx_mod_write_byte(actor+0x5C,(uint8_t)second_arg);
            mmx4_coop_balance_forget(actor);
            psx_mod_write_word(diagnostic+0x54,1);
        }
    }
    psx_mod_write_word(diagnostic+0x38,sequence);
    if(command==8 && psx_mod_read_word(diagnostic+0x54)==1)
        return mmx4_coop_finish(cpu,0);
#else
    (void)cpu;
#endif
    return 0;
}

#ifndef PSX_NO_DEBUG_TOOLS
static unsigned development_movie_call;
#define MOVIE_COUNTERS(n) {"mmx4.qa.movie" #n ".native-starts", \
    "mmx4.qa.movie" #n ".native-returns", "mmx4.qa.movie" #n ".last-frame"}
static const char *const development_movie_counters[11][3]={
    MOVIE_COUNTERS(0),MOVIE_COUNTERS(1),MOVIE_COUNTERS(2),MOVIE_COUNTERS(3),
    MOVIE_COUNTERS(4),MOVIE_COUNTERS(5),MOVIE_COUNTERS(6),MOVIE_COUNTERS(7),
    MOVIE_COUNTERS(8),MOVIE_COUNTERS(9),MOVIE_COUNTERS(10)};
#undef MOVIE_COUNTERS
static int development_movie_observer(CPUState *cpu,uint32_t address) {
    if(development_movie_call || !diagnostic)return 0;
    unsigned movie=cpu->gpr[4]&255u;
    if(movie>10)return 0;
    /* Counter names are borrowed by the runtime and need static lifetime. */
    psx_mod_counter_add(development_movie_counters[movie][0],1);
    development_movie_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    development_movie_call=0;
    psx_mod_counter_add(development_movie_counters[movie][1],1);
    psx_mod_counter_add(development_movie_counters[movie][2],psx_mod_read_word(0x801441C0u));
    return mmx4_coop_finish(cpu,result);
}
#endif

static uint32_t character_palette(unsigned character) {
    return character==counterpart?assets[character].palette:psx_mod_read_word(SCRATCH+0x28);
}
static void character_palette_colors(unsigned character,unsigned offset,uint16_t colors[16]) {
    uint8_t source[32];capture(character_palette(character)+offset,source,sizeof source);
    /* Both character banks are native source COLs.8002A484 lights them into
     * the shared working palette at80141F70; private immutable textures must
     * consume that same transform instead of baking the unlit source colors.
     * Keep native per-CLUT enable bits, transparency and STP. No palette or
     * gameplay writes are made, and changed colors already key the cache. */
    unsigned lit=psx_mod_read_word(0x8013E188u)&(0x80000000u>>(offset/32u));
    uint16_t amounts[3]={0,0,0};unsigned subtract=0;
    if(lit) {
        amounts[0]=psx_mod_read_half(0x80175EA0u);
        amounts[1]=psx_mod_read_half(0x80141BE6u);
        amounts[2]=psx_mod_read_half(0x8013E1BEu);
        if(amounts[0]|amounts[1]|amounts[2])
            subtract=!!psx_mod_read_byte(0x80175E9Cu)|
                (!!psx_mod_read_byte(0x80141BE4u)<<1)|
                (!!psx_mod_read_byte(0x8013E1BCu)<<2);
    }
    for(unsigned i=0;i<16;++i) {
        uint16_t color=(uint16_t)(source[i*2]|source[i*2+1]<<8);
        colors[i]=(amounts[0]|amounts[1]|amounts[2])?
            mmx4_coop_palette_lighting(color,amounts,subtract):color;
    }
}
static int decode_bank(const SpriteBank *bank,uint8_t decoded[32768]) {
    if(!bank->tiles || bank->tiles>256 || bank->variant>2)return 0;
    unsigned character=bank->variant==2;
    const CharacterAssets *a=&assets[character];
    const Mmx4Asset *body=bank->variant==1?&ultimate_archive.compressed:&a->compressed;
    if(!a->loaded || (bank->variant==1 && !ultimate_archive.loaded) ||
       bank->source<a->sprites || bank->source-a->sprites>=body->size)return 0;
    size_t written=0;
    return mmx4_coop_decode_sprite(body->data+bank->source-a->sprites,
        body->size-(bank->source-a->sprites),decoded,32768,&written) &&
        written==(size_t)bank->tiles*128u;
}
static int define_sprite_bank(const SpriteBank *bank) {
    uint8_t decoded[32768];uint16_t pixels[256*256];
    if(!decode_bank(bank,decoded))return 0;
    memset(pixels,0,sizeof pixels);
    unsigned remaining=bank->tiles,row=0,in=0;
    while(remaining) {
        unsigned chunk=remaining<16?remaining:16,width=chunk*16;
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<width;++x) {
            unsigned index=(decoded[in+(y*width+x)/2]>>((x&1)*4))&15;
            pixels[(row+y)*256+x]=(uint16_t)(bank->colors[index*2]|bank->colors[index*2+1]<<8);
        }
        in+=chunk*128;row+=16;remaining-=chunk;
    }
    if(bank->tile!=UINT16_MAX) {
        uint16_t atlas[240*240];unsigned width,height;
        if(!mmx4_coop_sprite_atlas_pixels(pixels,bank->tiles,bank->tile,atlas,&width,&height))return 0;
        return psx_mod_define_texture_bank(bank->id,width,height,atlas);
    }
    return psx_mod_define_texture_bank(bank->id,256,256,pixels);
}
static uint16_t sprite_bank(unsigned character,uint32_t table,unsigned frame,uint16_t clut,uint16_t tile,uint16_t *atlas_uv) {
    unsigned *count=psx_mod_local_view_scope()?&view_bank_count:&bank_count;
    SpriteBank *cache=psx_mod_local_view_scope()?view_banks:banks;
    CharacterAssets *a=&assets[character];uint8_t colors[32];
    unsigned variant=character?2u:x_variant;
    const Mmx4Asset *body=character_compressed(character);
    if(table<a->sprites || table-a->sprites>=body->size || clut>=0x8000)return 0;
    size_t offset=table-a->sprites;
    if(frame*4+4>body->size-offset)return 0;
    uint32_t packed=le32(body->data+offset+frame*4),source=table+(packed&0xFFFFF);
    unsigned tiles=packed>>20;
    if(!tiles || tiles>256 || source<a->sprites || source-a->sprites>=body->size)return 0;
    *atlas_uv=0;
    if(tile!=UINT16_MAX && !mmx4_coop_sprite_atlas_tile(tiles,tile,&tile,atlas_uv))return 0;
    int palette_offset=mmx4_coop_character_palette_offset(clut);
    if(palette_offset>=0) {
        uint16_t lit[16];character_palette_colors(character,(unsigned)palette_offset,lit);
        for(unsigned i=0;i<16;++i) {colors[i*2]=(uint8_t)lit[i];colors[i*2+1]=(uint8_t)(lit[i]>>8);}
    }else {
        /* Capsule upgrade effects reuse character geometry with shared
         * animation CLUTs (e.g.78C0). Those colors are outside the private
         * character bank and must come from the native GPU palette. */
        const uint16_t *vram=gpu_get_vram();
        for(unsigned i=0;i<16;++i) {
            uint16_t color=vram[(unsigned)clut*16+i];
            colors[i*2]=(uint8_t)color;colors[i*2+1]=(uint8_t)(color>>8);
        }
    }
    for(unsigned i=0;i<*count;++i)if(cache[i].variant==variant && cache[i].source==source &&
        cache[i].tile==tile && !memcmp(cache[i].colors,colors,32))return cache[i].id;
    unsigned slot=mmx4_coop_texture_cache_slot(*count,
        psx_mod_local_view_scope()?0xA000u:0x6000u,
        psx_mod_read_word(0x80141BD8u),arena,hud_arena,psx_mod_read_word);
    if(slot>=4096)return 0;
    uint16_t id=(uint16_t)((psx_mod_local_view_scope()?0xA000u:0x6000u)+slot);
    SpriteBank bank={source,id,tile,(uint16_t)tiles,(uint8_t)variant,{0}};memcpy(bank.colors,colors,32);
    /* Forget drains queued GL work before reusing an unreferenced ID. Both
     * DMA buffers remain pinned; saved cache entries still reconstruct every
     * ID needed by restored packets, without changing the snapshot format. */
    if(slot<*count && !psx_mod_forget_texture_bank_range(id,1))return 0;
    if(!define_sprite_bank(&bank))return 0;
    cache[slot]=bank;
    if(slot==*count)++*count;else psx_mod_counter_add("mmx4.coop.sprite-cache-reuses",1);
    return id;
}
static void triangle(uint32_t dst,uint32_t next,const uint32_t *q,unsigned a,unsigned b,unsigned c,uint16_t bank) {
    unsigned v[3]={a,b,c};psx_mod_write_word(dst,0x09000000u|(next&0xFFFFFF));
    for(unsigned i=0;i<3;++i) {
        uint32_t tag=i==0?(0x34000000u|(q[1]&0x03000000u)):i==1?(uint32_t)(bank&255)<<24:(uint32_t)(bank>>8)<<24;
        uint32_t uv=q[3+v[i]*2]&0xFFFF;
        if(i==1)uv|=(0x100u|((q[5]>>16)&0x60u))<<16;
        psx_mod_write_word(dst+4+i*12,tag|(q[1]&0xFFFFFF));
        psx_mod_write_word(dst+8+i*12,q[2+v[i]*2]);psx_mod_write_word(dst+12+i*12,uv);
    }
}
static int define_ui_bank(const UiBank *bank) {
    if(bank->character>3 || bank->page&0x180)return 0;
    const uint16_t *image=bank->character<2?assets[bank->character].ui_pixels:
        board_assets[bank->character-2].pixels;
    if(!image)return 0;
    uint16_t pixels[256*256];
    unsigned bx=(bank->page&15)*64,by=(bank->page&16)*16;
    unsigned width=bank->uv==UINT16_MAX?256:16;
    for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x) {
        unsigned u=bank->uv==UINT16_MAX?x:((bank->uv&255)+x)&255;
        unsigned v=bank->uv==UINT16_MAX?y:((bank->uv>>8)+y)&255;
        uint16_t packed=image[(by+v)*1024+bx+u/4];
        pixels[y*width+x]=bank->colors[(packed>>((u&3)*4))&15];
    }
    return psx_mod_define_texture_bank(bank->id,width,width,pixels);
}
static int resolve_texture_bank(uint16_t id) {
    /* Restore exact DMA IDs from immutable ARC/UI pixels and stored CLUTs.
     * This callback never allocates guest memory or writes a guest palette. */
    if(id>=0x6000u && id-0x6000u<bank_count)return define_sprite_bank(&banks[id-0x6000u]);
    if(id>=0x8000u && id-0x8000u<ui_bank_count)return define_ui_bank(&ui_banks[id-0x8000u]);
    if(id>=0xA000u && id-0xA000u<view_bank_count)return define_sprite_bank(&view_banks[id-0xA000u]);
    if(id>=0xB000u && id-0xB000u<view_ui_bank_count)return define_ui_bank(&view_ui_banks[id-0xB000u]);
    return 0;
}
static uint16_t ui_tile(unsigned character,uint16_t page,uint16_t clut,uint16_t uv,int private_colors) {
    unsigned *count=psx_mod_local_view_scope()?&view_ui_bank_count:&ui_bank_count;
    UiBank *cache=psx_mod_local_view_scope()?view_ui_banks:ui_banks;
    if(character>3 || page&0x180 || (character<2?!assets[character].ui_pixels:
       !board_assets[character-2].pixels))return 0;
    uint16_t colors[16];
    unsigned cx=(clut&63)*16,cy=clut>>6;
    const uint16_t *vram=gpu_get_vram();
    int palette_offset=mmx4_coop_character_palette_offset(clut);
    if(private_colors && character>=2 && cx<256 && cy>=480 && cy<488) {
        const uint8_t *p=board_assets[character-2].colors+((cy-480)*256+cx)*2;
        for(unsigned i=0;i<16;++i)colors[i]=(uint16_t)(p[i*2]|p[i*2+1]<<8);
    }else if(private_colors && character<2 && palette_offset>=0)
        character_palette_colors(character,(unsigned)palette_offset,colors);
    else for(unsigned i=0;i<16;++i)colors[i]=vram[cy*1024+cx+i];
    for(unsigned i=0;i<*count;++i) {
        UiBank *b=&cache[i];
        if(b->character==character && b->page==page && b->clut==clut && b->uv==uv && !memcmp(b->colors,colors,32))return b->id;
    }
    unsigned slot=mmx4_coop_texture_cache_slot(*count,
        psx_mod_local_view_scope()?0xB000u:0x8000u,
        psx_mod_read_word(0x80141BD8u),arena,hud_arena,psx_mod_read_word);
    if(slot>=4096)return 0;
    uint16_t id=(uint16_t)((psx_mod_local_view_scope()?0xB000u:0x8000u)+slot);
    UiBank bank={page,clut,uv,id,{0},character};memcpy(bank.colors,colors,32);
    if(slot<*count && !psx_mod_forget_texture_bank_range(id,1))return 0;
    if(!define_ui_bank(&bank))return 0;
    cache[slot]=bank;
    if(slot==*count)++*count;else psx_mod_counter_add("mmx4.coop.ui-cache-reuses",1);
    return id;
}
#include "mmx4_coop_frontend.inc"
static int world_draw_visible(void) {
    /* Room setup commits mode6 before initializing the primary body. The
     * still-retained partner belongs to the old world until that boundary. */
    if(psx_mod_read_byte(PLAY)!=6 || !psx_mod_read_byte(PLAYER+4))return 0;
    unsigned phase=psx_mod_read_byte(PLAY+1);
    /* Native pause fades draw the world while gameplay remains in minor2.
     * Keep both seats in those world passes. Options and Quit Game own
     * frontend backdrops; neither may inherit a partner world sprite. */
    return phase==0 || (phase==2 && psx_mod_read_byte(0x801754A5u)<5);
}
static int coop_hud(CPUState *cpu,uint32_t address) {
    if(hud_call || projected || !mmx4_coop_ready() || !world_draw_visible())return 0;
    if(psx_mod_read_byte(PLAY+1)==2)psx_mod_counter_add("mmx4.coop.pause-world-hud-draws",1);
    int own_hud=split_views && psx_mod_local_view_scope();
    hud_call=1;uint32_t result=own_hud && local_view_seat?0:
        guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(own_hud && !local_view_seat) {hud_call=0;return mmx4_coop_finish(cpu,result);}
    if(psx_mod_read_byte(PLAY+0x1F) && second.body[0]) {
        /* Both layouts share the native left anchor. Own-player Split views
         * use the original position; Unified offsets only the second group. */
        int offset_x=!own_hud && hud_side_by_side?56:0;
        int offset_y=!own_hud && !hud_side_by_side?104:0;
        uint32_t pools[3],base=hud_arena+(psx_mod_read_word(SCRATCH)&1u)*0x8000;
        for(unsigned i=0;i<3;++i) {pools[i]=psx_mod_read_word(SCRATCH+0x108+i*4);psx_mod_write_word(SCRATCH+0x108+i*4,base+i*0x1000);}
        enter_second();
        guest(cpu,0x800253F0,PLAYER,0);
        guest(cpu,0x80025188,1,(psx_mod_read_byte(PLAY+0x44)+0x3Bu)&255u);
        guest(cpu,0x80025188,0,0x45+(psx_mod_read_byte(PLAY+0x46)-32)/2);
        guest(cpu,counterpart?0x8002509C:0x80024F5C,PLAYER,0);
        uint32_t end=psx_mod_read_word(SCRATCH+0x108),bar_end=psx_mod_read_word(SCRATCH+0x110);
        if(end<base || end>base+0x1000 || (end-base)%16 || bar_end<base+0x2000 || bar_end>base+0x3000 || (bar_end-base-0x2000)%24)failed=6;
        for(uint32_t at=base,mode=base+0x1000,dst=base+0x3000;at<end && !failed;at+=16,mode+=8,dst+=80) {
            uint32_t link=psx_mod_read_word(at),uv=psx_mod_read_word(at+12);
            uint16_t page=(uint16_t)psx_mod_read_word(mode+4),bank=ui_tile(counterpart,page,(uint16_t)(uv>>16),(uint16_t)uv,0);
            if(!bank || dst+80>base+0x8000) {failed=7;break;}
            int x=(int16_t)psx_mod_read_half(at+8)+offset_x,y=(int16_t)psx_mod_read_half(at+10)+offset_y;
            uint32_t q[10]={link,0x2C808080,0,0,0,0,0,0,0,0};
            for(unsigned i=0;i<4;++i) {
                q[2+i*2]=(uint16_t)(x+(i&1)*16)|((uint32_t)(uint16_t)(y+(i>>1)*16)<<16);
                q[3+i*2]=(i&1)*16|((i>>1)*16<<8);
            }
            triangle(dst,dst+40,q,0,1,2,bank);triangle(dst+40,link,q,2,1,3,bank);
            psx_mod_anchor_hud_primitive(dst,-1);psx_mod_anchor_hud_primitive(dst+40,-1);psx_mod_write_word(at,dst&0xFFFFFF);
        }
        /* Original 80025588 emits flat POLY_F4, six words including tag. */
        for(uint32_t at=base+0x2000;at<bar_end && !failed;at+=24) {
            for(unsigned xy=8;xy<=20;xy+=4) {
                psx_mod_write_half(at+xy,(uint16_t)(psx_mod_read_half(at+xy)+offset_x));
                psx_mod_write_half(at+xy+2,(uint16_t)(psx_mod_read_half(at+xy+2)+offset_y));
            }
            psx_mod_anchor_hud_primitive(at,-1);
        }
        leave_second();
        for(unsigned i=0;i<3;++i)psx_mod_write_word(SCRATCH+0x108+i*4,pools[i]);
    }
    if(own_hud && local_view_seat && psx_mod_read_byte(PLAY+0x1F) &&
       psx_mod_read_byte(PLAY+0x24)) {
        guest(cpu,0x800253F0u,psx_mod_read_word(PLAY+0x20),1);
        guest(cpu,0x80025188u,7,(psx_mod_read_byte(PLAY+0x25)+0x57u)&255u);
    }
    hud_call=0;return mmx4_coop_finish(cpu,result);
}
static void render_actor(CPUState *cpu,uint32_t actor,uint32_t base,unsigned character) {
    if(mmx4_coop_split_sprite_outside(actor))return;
    uint32_t start=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    /* Native800244D0/8002452C selects static artwork with flags40/20.
     * A reused shot can retain its previous weapon's +38: buster init
     * 80092328/80092840 installs flags21 without clearing that pointer.
     * Preserve the original UV/page mapping through the immutable raw cache. */
    unsigned raw_graphics=psx_mod_read_byte(actor)&0x60u;
    uint32_t table=raw_graphics?0:psx_mod_read_word(actor+0x38);
    unsigned frame=psx_mod_read_byte(actor+0x47);
    unsigned primary=character!=counterpart;
    uint32_t parent_assembly=primary || projected?psx_mod_read_word(PLAYER+0x3C):le32(second.body+0x3C);
    if(!raw_graphics && !table && psx_mod_read_word(actor+0x3C)==parent_assembly)
        table=primary || projected?psx_mod_read_word(PLAYER+0x38):le32(second.body+0x38);
    CharacterAssets *a=&assets[character];
    const Mmx4Asset *body=character_compressed(character);
    uint32_t native_sprites=psx_mod_read_word(SCRATCH+0x14);
    if(!raw_graphics && primary && table>=native_sprites && table-native_sprites<body->size)
        table=a->sprites+(table-native_sprites);
    int compressed=!raw_graphics && table>=a->sprites && table-a->sprites<body->size;
    /* Native 80024420/800244E8 adds the actor's VRAM allocation to each
     * assembly tile. Private decoded banks start at tile zero instead.
     * Weapons commonly occupy 0520/0530/0540 rather than the body's 0500;
     * retaining that row offset samples blank or unrelated decoded tiles.
     * Let the native emitter handle flips and page seams with a local origin,
     * then restore the allocation before any simulation or upload can run. */
    uint16_t tile_origin=psx_mod_read_half(actor+0x40);
    if(compressed)psx_mod_write_half(actor+0x40,0);
    guest(cpu,0x80024334,actor,0);
    /* Native80023E88/80023EA4 draws X's acquired armor after the body.
     * Its assembly tables use the same compressed frame and tile origin;
     * include those packets in P2's private-bank conversion below. P1's
     * original caller still emits its armor after our body filter returns. */
    if(actor==PLAYER && !character && projected) {
        uint32_t armor_start=psx_mod_read_word(SCRATCH+0x100);
        guest(cpu,0x800257BC,actor,0);
        uint32_t armor_end=psx_mod_read_word(SCRATCH+0x100);
        if(armor_end>armor_start)psx_mod_counter_add("mmx4.coop.x-armor-primitives",
            (armor_end-armor_start)/40);
    }
    if(compressed)psx_mod_write_half(actor+0x40,tile_origin);
    uint32_t end=psx_mod_read_word(SCRATCH+0x100);
    if(end<start || end-base>40000 || (end-start)%40) { failed=4;return; }
    uint32_t assembly=psx_mod_read_word(actor+0x3C);
    uint32_t assembly_base=primary?psx_mod_read_word(SCRATCH+0x1C):a->assembly;
    if(!compressed && (assembly<assembly_base || assembly-assembly_base>=0xC000))return;
    uint32_t expanded=base+40000+(start-base)*2;
    for(uint32_t at=start;at<end;at+=40,expanded+=80,mode+=8) {
        uint32_t q[10];for(unsigned i=0;i<10;++i)q[i]=psx_mod_read_word(at+i*4);
        unsigned command=q[1]>>24;uint16_t tile=UINT16_MAX;
        if((command&0xFC)==0x7C) {
            int x=(int16_t)q[2],y=(int16_t)(q[2]>>16);uint32_t clut=q[3]&0xFFFF0000u;tile=(uint16_t)q[3];
            q[1]=(q[1]&0x03FFFFFFu)|0x2C000000u;
            for(unsigned i=0;i<4;++i) {
                q[2+i*2]=(uint16_t)(x+(i&1)*16)|((uint32_t)(uint16_t)(y+(i>>1)*16)<<16);
                q[3+i*2]=(i&1)*16|((i>>1)*16<<8);
            }
            q[3]|=clut;q[5]|=(psx_mod_read_word(mode+4)&0x60u)<<16;
        }else if((command&0xFC)!=0x2C) {failed=5;return;}
        else if(compressed)tile=mmx4_coop_sprite_quad_tile(q);
        uint16_t page=tile==UINT16_MAX?(uint16_t)(q[5]>>16):(uint16_t)psx_mod_read_word(mode+4);
        uint16_t atlas_uv=0;
        uint16_t bank=compressed?sprite_bank(character,table,frame,(uint16_t)(q[3]>>16),tile,&atlas_uv):
            ui_tile(character,page,(uint16_t)(q[3]>>16),tile,1);
        if(!bank) {
            failed=8;psx_mod_counter_add("mmx4.coop.invalid-sprite",1);
            if(!psx_mod_local_view_scope())fprintf(stderr,
                "mmx4.coop: invalid-sprite actor=%08X character=%u table=%08X assembly=%08X "
                "frame=%u compressed=%u clut=%04X page=%04X tile=%04X banks=%u ui-banks=%u "
                "raw-flags=%02X\n",
                actor,character,table,assembly,frame,(unsigned)compressed,
                (unsigned)(q[3]>>16),(unsigned)page,(unsigned)tile,bank_count,ui_bank_count,
                raw_graphics);
            return;
        }
        if(compressed && tile!=UINT16_MAX)for(unsigned i=0;i<4;++i) {
            unsigned uv=q[3+i*2]&0xFFFFu;
            q[3+i*2]=(q[3+i*2]&0xFFFF0000u)|
                ((atlas_uv&255u)+(uv&255u))|(((atlas_uv>>8)+(uv>>8))<<8);
        }
        triangle(expanded,expanded+40,q,0,1,2,bank);triangle(expanded+40,q[0],q,2,1,3,bank);
        psx_mod_tag_world_primitive(expanded,1);psx_mod_tag_world_primitive(expanded+40,1);
        psx_mod_write_word(at,expanded&0xFFFFFF);
        unsigned priority=psx_mod_read_byte(actor+0x16);
        uint32_t tail=0x8013BC40u+(psx_mod_read_word(SCRATCH)&1u)*128+(priority>>4)*32+(priority&15)*4;
        if(psx_mod_read_word(tail)==at)psx_mod_write_word(tail,expanded+40);
    }
}
static void render_rider_component(CPUState *cpu,uint32_t actor,uint32_t base,unsigned character,unsigned owner) {
    uint8_t priority=psx_mod_read_byte(actor+0x16);
    psx_mod_write_byte(actor+0x16,mmx4_coop_ride_draw_priority(priority,owner));
    render_actor(cpu,actor,base,character);
    psx_mod_write_byte(actor+0x16,priority);
}
static int render_native_rider_component(CPUState *cpu,uint32_t address) {
    uint32_t actor=cpu->gpr[4];uint8_t priority=psx_mod_read_byte(actor+0x16);
    rendering=1;
    psx_mod_write_byte(actor+0x16,mmx4_coop_ride_draw_priority(priority,1));
    uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
    psx_mod_write_byte(actor+0x16,priority);rendering=0;
    return mmx4_coop_finish(cpu,result);
}
int mmx4_coop_render_rider_armor(CPUState *cpu,uint32_t address) {
    if(rendering || projected || !mmx4_coop_ready() || cpu->gpr[4]!=PLAYER ||
       psx_mod_read_byte(PLAYER+0xC5)!=0xFF || psx_mod_read_byte(MMX4_VEHICLE+1)!=0)return 0;
    return render_native_rider_component(cpu,address);
}
static int render(CPUState *cpu,uint32_t address) {
    if(rendering || inside || projected || failed || !enrolled || !world_draw_visible())return 0;
    if(psx_mod_read_byte(PLAY+1)==2)psx_mod_counter_add("mmx4.coop.pause-world-player-draws",1);
    rendering=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    enter_second();
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_ARENA;
    psx_mod_write_word(SCRATCH+0x100,base);
    /* Ride Armor also uses C5=FF, but its odd bucket3 must remain behind
     * the native rider bucket2. Only Ride Chaser has the four-part ordering. */
    unsigned chaser=second.body[0xC5]==0xFF && second.vehicle[1]==0?2u:0u;
    if(second.body[3])render_rider_component(cpu,PLAYER,base,counterpart,chaser);
    if(psx_mod_read_byte(0x80175D58u+3))render_actor(cpu,0x80175D58u,base,counterpart);
    if(!second.body[2])for(unsigned i=0;i<2 && !failed;++i)
        if(psx_mod_read_byte(CHARGE_GLOWS+i*0x50+3))render_actor(cpu,CHARGE_GLOWS+i*0x50,base,counterpart);
    /* The native renderer draws the three trails back to front. */
    for(unsigned i=3;i && !failed;--i)
        if(psx_mod_read_byte(TRAILS+(i-1)*0x60+3))render_actor(cpu,TRAILS+(i-1)*0x60,base,counterpart);
    if(psx_mod_read_byte(MMX4_VEHICLE+3))render_rider_component(cpu,MMX4_VEHICLE,base,counterpart,chaser);
    for(unsigned i=0;i<16 && !failed;++i)if(psx_mod_read_byte(SHOTS+i*0x9C+3)) {
        uint32_t shot=SHOTS+i*0x9C;
        render_rider_component(cpu,shot,base,counterpart,
            chaser && psx_mod_read_byte(shot+1)==57?chaser:0u);
    }
    world_packet_used=psx_mod_read_word(SCRATCH+0x100)-base;
    psx_mod_write_word(SCRATCH+0x100,original);leave_second();rendering=0;
    return mmx4_coop_finish(cpu,result);
}
static int render_owned_effect(CPUState *cpu,uint32_t address) {
    if(render_board_actor(cpu,address))return 1;
    uint32_t stock_actor=cpu->gpr[4];
    int stock_owner=mmx4_coop_stored_charge_owner(stock_actor);
    if(!rendering && !projected && mmx4_coop_ready() && world_draw_visible() && stock_owner>=0) {
        /* X's stored shots are a screen-relative particle, not part of
         * native24E70's health HUD. The world draw visits that same particle
         * for every Split view; only its allocating player's HUD owns it.
         * Unified uses the same offset as that player's health group. */
        int own_hud=split_views && psx_mod_local_view_scope();
        unsigned character=stock_owner?second.body[2]:psx_mod_read_byte(PLAYER+2);
        if(character || (own_hud && (unsigned)stock_owner!=local_view_seat)) {
            psx_mod_counter_add("mmx4.coop.stored-charge-hud-other-view-hidden",1);
            return mmx4_coop_finish(cpu,0);
        }
        uint16_t x=psx_mod_read_half(stock_actor+10),y=psx_mod_read_half(stock_actor+14);
        if(stock_owner && !own_hud) {
            psx_mod_write_half(stock_actor+10,(uint16_t)(x+(hud_side_by_side?56:0)));
            psx_mod_write_half(stock_actor+14,(uint16_t)(y+(hud_side_by_side?0:104)));
        }
        rendering=1;
        uint32_t original=psx_mod_read_word(SCRATCH+0x100);
        uint32_t base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_ARENA;
        uint32_t start=base+world_packet_used;
        psx_mod_write_word(SCRATCH+0x100,start);
        if(stock_owner)enter_second();
        render_actor(cpu,stock_actor,base,0);
        if(stock_owner)leave_second();
        uint32_t end=psx_mod_read_word(SCRATCH+0x100);
        if(!failed)for(uint32_t at=start;at<end;at+=40) {
            uint32_t expanded=base+40000+(at-base)*2;
            psx_mod_tag_world_primitive(expanded,0);psx_mod_tag_world_primitive(expanded+40,0);
            psx_mod_anchor_hud_primitive(expanded,-1);psx_mod_anchor_hud_primitive(expanded+40,-1);
        }
        world_packet_used=end-base;
        psx_mod_write_word(SCRATCH+0x100,original);
        psx_mod_write_half(stock_actor+10,x);psx_mod_write_half(stock_actor+14,y);
        rendering=0;
        psx_mod_counter_add(stock_owner?"mmx4.coop.p2-stored-charge-hud-draws":
            "mmx4.coop.p1-stored-charge-hud-draws",1);
        return mmx4_coop_finish(cpu,0);
    }
    uint8_t assessment[0x60];
    if(!rendering && diagnostic && cpu->gpr[4]>=0x80173CA0u &&
       cpu->gpr[4]<0x801754A0u && psx_mod_read_byte(cpu->gpr[4]+1)==34 && world_draw_visible() &&
       mmx4_coop_assessment_copy(cpu->gpr[4],assessment)) {
        /* Keep a distinct draw address for interpolation identity. This
         * region is outside all diagnostics and native actor pools. Restore
         * it before leaving the presentation scope, including private views. */
        uint32_t actor=cpu->gpr[4],ghost=diagnostic+0x800u;
        uint8_t saved[0x60];capture(ghost,saved,sizeof saved);
        project(ghost,assessment,sizeof assessment);
        rendering=1;
        uint32_t result=mmx4_coop_split_sprite_outside(actor)?0:
            guest(cpu,address,actor,cpu->gpr[5]);
        if(!mmx4_coop_split_sprite_outside(ghost)) {
            guest(cpu,address,ghost,cpu->gpr[5]);
            psx_mod_counter_add("mmx4.coop.p2-analysis-draws",1);
            if(assessment[5]<2)psx_mod_counter_add("mmx4.coop.p2-analysis-opening-draws",1);
        }
        project(ghost,saved,sizeof saved);rendering=0;
        return mmx4_coop_finish(cpu,result);
    }
    if(mmx4_coop_split_sprite_outside(cpu->gpr[4])) {
        psx_mod_counter_add("mmx4.coop.split-distant-sprite-culled",1);
        return mmx4_coop_finish(cpu,0);
    }
    if(!rendering && mmx4_coop_ready() && world_draw_visible()) {
        uint32_t actor=cpu->gpr[4];
        unsigned ticks=mmx4_coop_balance_flash_ticks(actor);
        if(ticks) {
            /* Separate hit admission keeps the shared native timer at the
             * earliest available seat. It can expire after one tick while
             * the attacker is still cooling down. Show the native white
             * hit palette for that remaining window in either view, without
             * changing collision tables, boss AI or saved simulation bytes.
             * 24334/24574 consumes +42 bit8000 solely for the drawn CLUT. */
            uint16_t color=psx_mod_read_half(actor+0x42);
            rendering=1;
            psx_mod_write_half(actor+0x42,(uint16_t)((color&0x7FFFu)|
                ((ticks&1u)?0u:0x8000u)));
            uint32_t result=guest(cpu,address,actor,cpu->gpr[5]);
            psx_mod_write_half(actor+0x42,color);
            rendering=0;
            psx_mod_counter_add("mmx4.coop.per-player-boss-flash-draws",1);
            return mmx4_coop_finish(cpu,result);
        }
    }
    if(rendering || projected || !mmx4_coop_ready() || !world_draw_visible())return 0;
    uint32_t actor=cpu->gpr[4],assembly=psx_mod_read_word(actor+0x3C);
    unsigned vehicle_owner=mmx4_coop_vehicle_effect_owner(actor);
    unsigned ride_owner=vehicle_owner &&
        (vehicle_owner==2?second.vehicle[1]:psx_mod_read_byte(MMX4_VEHICLE+1))==0?vehicle_owner:0;
    if(psx_mod_read_byte(PLAYER+0xC5)==0xFF &&
       psx_mod_read_byte(MMX4_VEHICLE+1)==0 &&
       (actor==PLAYER || actor==MMX4_VEHICLE ||
        (actor>=SHOTS && actor<SHOTS+sizeof first.shots && (actor-SHOTS)%0x9C==0 &&
         psx_mod_read_byte(actor+1)==57)))ride_owner=1;
    unsigned character=counterpart;
    if(assembly<assets[counterpart].assembly || assembly-assets[counterpart].assembly>=0xC000) {
        uint32_t primary_assembly=psx_mod_read_word(SCRATCH+0x1C);
        int primary_actor=actor==PLAYER || actor==0x80175D58u ||
            (actor>=SHOTS && actor<SHOTS+sizeof first.shots && (actor-SHOTS)%0x9C==0) ||
            (actor>=TRAILS && actor<TRAILS+sizeof first.trails && (actor-TRAILS)%0x60==0) ||
            (actor>=CHARGE_GLOWS && actor<CHARGE_GLOWS+sizeof first.charge_glows && (actor-CHARGE_GLOWS)%0x50==0);
        if(!primary_actor || assembly<primary_assembly || assembly-primary_assembly>=0xC000) {
            if(vehicle_owner!=2)return ride_owner==1?render_native_rider_component(cpu,address):0;
        }else {
            character=counterpart^1u;
            psx_mod_counter_add("mmx4.coop.primary-private-character-draws",1);
        }
    }
    rendering=1;
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_ARENA;
    psx_mod_write_word(SCRATCH+0x100,base+world_packet_used);
    render_rider_component(cpu,actor,base,character,ride_owner);
    world_packet_used=psx_mod_read_word(SCRATCH+0x100)-base;
    psx_mod_write_word(SCRATCH+0x100,original);rendering=0;
    return mmx4_coop_finish(cpu,0);
}
static uint32_t hash_bytes(uint32_t hash,const uint8_t *bytes,size_t size) {
    for(size_t i=0;i<size;++i)hash=(hash^bytes[i])*16777619u;
    return hash;
}
static uint32_t hash_word(uint32_t hash,uint32_t value) {
    uint8_t bytes[4];put32(bytes,value);return hash_bytes(hash,bytes,4);
}
static uint32_t hash_player(uint32_t hash,const PlayerContext *p) {
    /* Arrays are original guest records, not host structs/pointers/padding. */
    hash=hash_bytes(hash,p->body,sizeof p->body);hash=hash_bytes(hash,p->shots,sizeof p->shots);
    hash=hash_bytes(hash,p->trails,sizeof p->trails);hash=hash_bytes(hash,p->double_body,sizeof p->double_body);
    hash=hash_bytes(hash,p->charge_glows,sizeof p->charge_glows);
    return hash_bytes(hash,p->vehicle,sizeof p->vehicle);
}
static uint32_t state_digest(void) {
    uint32_t hash=2166136261u;
    hash=mmx4_coop_bosses_digest(hash);
    hash=mmx4_coop_balance_digest(hash);
    hash=hash_word(hash,split_views);
    hash=hash_word(hash,boss_door_both);
    hash=hash_word(hash,ultimate_armor_x);hash=hash_word(hash,black_zero);
    hash=hash_word(hash,mmx4_cheats_character_enabled(0));hash=hash_word(hash,mmx4_cheats_character_enabled(1));
    hash=hash_word(hash,x_variant);
    hash=hash_word(hash,board_heroes[0]);hash=hash_word(hash,board_heroes[1]);
    hash=hash_word(hash,board_companions[0]);hash=hash_word(hash,board_companions[1]);
    hash=hash_word(hash,enrolled);hash=hash_word(hash,failed);hash=hash_word(hash,projected);
    hash=hash_word(hash,counterpart);hash=hash_word(hash,previous_input);hash=hash_word(hash,frames);
#ifdef MMX4_COOP_NETPLAY_DEBUG
    hash=hash_word(hash,fixture_invulnerable);
    hash=hash_word(hash,development_decoder.next);
    hash=hash_word(hash,development_decoder.previous);
    hash=hash_word(hash,development_decoder.have_previous);
    hash=hash_word(hash,development_decoder.last_sequence);
    for(unsigned i=0;i<7;++i)hash=hash_word(hash,development_decoder.words[i]);
#endif
    hash=hash_player(hash,&second);
    if(projected) {
        hash=hash_player(hash,&first);hash=hash_word(hash,saved_dirty);
        hash=hash_word(hash,saved_cheat);
        for(unsigned i=0;i<9;++i)hash=hash_word(hash,saved_resources[i]);
    }
    return mmx4_coop_split_digest(mmx4_coop_combat_digest(mmx4_coop_lifecycle_digest(hash)));
}
typedef struct {
    PlayerContext first,second;
    uint32_t resources[9],world_packet_used;
    unsigned projected,rendering,hud_call,upload_guard,failed,menu_texture_owner;
    uint8_t dirty,cheat;
} LocalViewContext;
static void local_context_save(LocalViewContext *context) {
    *context=(LocalViewContext){first,second,{0},world_packet_used,
        projected,rendering,hud_call,upload_guard,failed,menu_texture_owner,saved_dirty,saved_cheat};
    memcpy(context->resources,saved_resources,sizeof saved_resources);
    mmx4_coop_lifecycle_view_save();mmx4_widescreen_view_save();
}
static void local_context_restore(const LocalViewContext *context) {
    first=context->first;second=context->second;
    memcpy(saved_resources,context->resources,sizeof saved_resources);
    world_packet_used=context->world_packet_used;saved_dirty=context->dirty;saved_cheat=context->cheat;
    projected=context->projected;rendering=context->rendering;hud_call=context->hud_call;
    upload_guard=context->upload_guard;failed=context->failed;
    menu_texture_owner=context->menu_texture_owner;
    mmx4_coop_lifecycle_view_restore();mmx4_widescreen_view_restore();
}
static int draw_local_view(CPUState *cpu,void *user,uint32_t alpha) {
    (void)alpha;
    uint32_t buffer=*(uint32_t *)user;
    uint8_t camera[0xFC];
    int menu=mmx4_coop_lifecycle_personal_menus();
    int board=board_active();
    if(!menu && !board) {
        if(!mmx4_coop_split_camera_copy(local_view_seat,camera))return 0;
        mmx4_coop_split_local_explosion_palette(cpu,camera);
        mmx4_coop_split_project_intro_boss();
        project(0x801419B0u,camera,sizeof camera);
    }
    gpu_set_gp0_source(0);
    gpu_write_gp0(0x02000000u|psx_mod_read_byte(buffer+0x2Du)|
        (uint32_t)psx_mod_read_byte(buffer+0x2Eu)<<8|
        (uint32_t)psx_mod_read_byte(buffer+0x2Fu)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x14u)|(uint32_t)psx_mod_read_half(buffer+0x16u)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x18u)|(uint32_t)psx_mod_read_half(buffer+0x1Au)<<16);
    guest(cpu,0x800EA714u,buffer+0x70u,12);
    if(menu) {
        if(!mmx4_coop_lifecycle_draw_menu(cpu,local_view_seat))return 0;
    }else guest(cpu,0x80023D68u,0,0);
    if(failed || projected || rendering || hud_call)return 0;
    guest(cpu,0x800EC228u,buffer+0x9Cu,0);
    unsigned shade=mmx4_coop_lifecycle_menu_fade(buffer);
    if(shade) {
        /* Clearing the private ordering table drops the shared fade task's
         * screen tile. Replay its current shade after the private image; the
         * render transaction restores GPU state, and never advances the fade. */
        unsigned index=buffer==0x80166CB0u;
        uint32_t mode=psx_mod_read_word(0x8012F49Cu+index*8u);
        if((mode>>24)!=0xE1u)mode=0xE1000045u;
        gpu_set_gp0_source(0);
        gpu_write_gp0(mode);
        gpu_write_gp0(0x62000000u|shade|(shade<<8)|(shade<<16));
        gpu_write_gp0(0);
        gpu_write_gp0(0x00F00140u);
    }
    return !failed;
}
static int draw_local_pair(CPUState *cpu,void *user,uint32_t seat) {
    LocalViewContext context;
    local_context_save(&context);
    int story=mmx4_coop_lifecycle_script_owner()>=0 && !mmx4_coop_lifecycle_paired_door();
    local_view_seat=mmx4_coop_lifecycle_personal_menus() || story || board_active()?seat:mmx4_coop_split_view_seat(seat);
    int ok=draw_local_view(cpu,user,0);
    /* Host plugin contexts must return to the common baseline between views,
     * as well as the framework's CPU/RAM/VRAM checkpoint. Neither draw ticks AI. */
    local_context_restore(&context);
    return ok;
}
static int present_local_view(CPUState *cpu,uint32_t address) {
    int board=board_active();
    unsigned paired=mmx4_coop_lifecycle_paired_door();
    int menu=mmx4_coop_lifecycle_personal_menus() &&
        psx_mod_read_byte(PLAY)==6 && psx_mod_read_byte(PLAY+1)==2;
    /* Continue/Quit initializes inside the personal menu's native call. */
    int shared_menu=mmx4_coop_lifecycle_shared_menu();
    int paused_world=mmx4_coop_lifecycle_paused_world();
    int story=mmx4_coop_lifecycle_script_owner()>=0 && !paired;
    int local_menu=menu && !psx_mod_netplay_is_active();
    if(!local_view_call && !projected && !psx_mod_local_view_scope() &&
       !psx_mod_netplay_is_active()) {
        if(local_board_view && !board) {
            /* Retire the board's two private images before the first native
             * introduction/loading frame, even if their cycle age is fresh. */
            psx_mod_set_local_pair_presentation(0);
            psx_mod_set_local_pair_presentation(local_split!=0);
        }
        local_board_view=(unsigned)board;
        if(shared_menu && !local_shared_menu) {
            /* Retire the private pause pair at the shared frontend boundary,
             * including nested presents while its assets initialize. A pair
             * may still be cycle-fresh after the fade has reached black;
             * exposing it again flashes the unfaded pause screen. Restore
             * local Split immediately so the native frontend keeps its usual
             * two panes, without redrawing its static backdrop as a stage. */
            psx_mod_set_local_pair_presentation(0);
            psx_mod_set_local_pair_presentation(local_split!=0);
        }
        local_shared_menu=(unsigned)shared_menu;
    }
    if(!local_split && !psx_mod_netplay_is_active())
        psx_mod_set_local_pair_presentation(local_menu);
    if(local_split && !local_aspect_set)
        local_aspect_set=(unsigned)psx_mod_set_fixed_display_aspect(4,3);
    /* An ordinary world freeze (including an ice bird's field effect) is
     * not a request to replace the partner's view with the canonical image.
     * The existing private transaction can redraw the frozen saved camera;
     * Story draws use the native scripted camera in both panes, with each
     * player's own HUD. Pause entry/exit likewise keep their private world
     * images until the character menus take over. */
    if((!split_views && !menu && !(board && psx_mod_netplay_is_active())) || local_view_call || projected || inside ||
       (!board && !mmx4_coop_ready()) ||
       (!board && (psx_mod_read_byte(PLAY)!=6 || (!menu && !paused_world && psx_mod_read_byte(PLAY+1)!=0))) ||
       cpu->gpr[31]!=0x80012080u ||
       (!board && psx_mod_read_byte(PLAY+0x0F)) ||
       (!board && !menu && psx_mod_read_byte(PLAY+0x1C) && !mmx4_coop_lifecycle_initial_arrival() &&
        !mmx4_coop_lifecycle_nova_active() && !paired && !story)) {
        return 0;
    }
    uint32_t buffer=psx_mod_read_word(0x80142F80u);
    if(cpu->gpr[4]!=buffer+0x9Cu)return 0;
    local_view_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    guest(cpu,0x800EA20Cu,0,0);
    int port=psx_netplay_local_port();
    unsigned seat=port>=0 && port<2?(unsigned)port:0;
    local_view_seat=menu || story || board?seat:mmx4_coop_split_view_seat(seat);
    PSXModRenderPass rectangle={sizeof rectangle,0,
        psx_mod_read_half(buffer+0x14u),psx_mod_read_half(buffer+0x16u),
        psx_mod_read_half(buffer+0x18u),psx_mod_read_half(buffer+0x1Au)};
    uint32_t before=state_digest();
    LocalViewContext context;local_context_save(&context);
    int committed;
    if(local_split || local_menu) {
        PSXModStereoFrame pair={sizeof pair,1,rectangle.x,rectangle.y,rectangle.w,rectangle.h};
        psx_mod_set_local_pair_presentation(1);
        committed=psx_mod_render_local_pair(cpu,&pair,draw_local_pair,&buffer);
        psx_mod_counter_add(committed?"mmx4.coop.local-pair-committed":"mmx4.coop.local-pair-refused",1);
    }else {
        committed=psx_mod_render_local_view(cpu,&rectangle,draw_local_view,&buffer);
    }
    /* Also restore outside callbacks: a transaction watchdog can unwind a
     * draw before its callback has a chance to restore host plugin state. */
    local_context_restore(&context);
    if(state_digest()!=before)psx_mod_counter_add("mmx4.coop.local-view-state-leak",1);
    psx_mod_counter_add(committed?"mmx4.coop.local-view-committed":"mmx4.coop.local-view-refused",1);
    if(committed)psx_mod_counter_add(local_view_seat?
        "mmx4.coop.local-view-seat-1":"mmx4.coop.local-view-seat-0",1);
    local_view_call=0;
    return mmx4_coop_finish(cpu,result);
}
static int snapshot_ready(void) {
#ifndef PSX_NO_DEBUG_TOOLS
    if(development_movie_call)return 0;
#endif
    return assets[0].loaded && assets[1].loaded && ultimate_archive.loaded && diagnostic && arena && hud_arena &&
        !(inside || rendering || projected || camera_call || camera_update_call || hud_call ||
          upload_guard || local_view_call || saved_cheat || board_call || psx_mod_local_view_scope()) &&
        mmx4_coop_lifecycle_snapshot_ready() && mmx4_coop_combat_snapshot_ready() &&
        mmx4_coop_web_snapshot_ready() && mmx4_coop_audio_snapshot_ready() &&
        mmx4_coop_split_snapshot_ready() && mmx4_widescreen_snapshot_ready() &&
        mmx4_diagnostics_snapshot_ready() && mmx4_cheats_snapshot_ready() &&
        mmx4_coop_bosses_snapshot_ready() && mmx4_coop_balance_snapshot_ready();
}
static void snapshot_player(Mmx4SnapshotIO *s,PlayerContext *player) {
    if(s->mode==MMX4_SNAPSHOT_VALIDATE && s->ok && s->pos<=s->len &&
       sizeof player->body<=s->len-s->pos && s->in[s->pos] && s->in[s->pos+2]>1)s->ok=0;
    mmx4_snapshot_bytes(s,player->body,sizeof player->body);
    mmx4_snapshot_bytes(s,player->shots,sizeof player->shots);
    mmx4_snapshot_bytes(s,player->trails,sizeof player->trails);
    mmx4_snapshot_bytes(s,player->charge_glows,sizeof player->charge_glows);
    mmx4_snapshot_bytes(s,player->double_body,sizeof player->double_body);
    mmx4_snapshot_bytes(s,player->vehicle,sizeof player->vehicle);
}
static void snapshot_sprite_banks(Mmx4SnapshotIO *s,SpriteBank *cache,unsigned *live_count,uint16_t base) {
    unsigned count=mmx4_snapshot_integer(s,*live_count,4,4096);
    if(s->mode==MMX4_SNAPSHOT_READ)*live_count=count;
    for(unsigned i=0;i<count && s->ok;++i) {
        SpriteBank bank=cache[i];
        bank.source=mmx4_snapshot_integer(s,bank.source,4,UINT32_MAX);
        bank.id=(uint16_t)mmx4_snapshot_integer(s,bank.id,2,UINT16_MAX);
        bank.tile=(uint16_t)mmx4_snapshot_integer(s,bank.tile,2,UINT16_MAX);
        bank.tiles=(uint16_t)mmx4_snapshot_integer(s,bank.tiles,2,256);
        bank.variant=(uint8_t)mmx4_snapshot_integer(s,bank.variant,1,2);
        mmx4_snapshot_bytes(s,bank.colors,sizeof bank.colors);
        if(bank.id!=base+i || !bank.tiles ||
           (bank.tile!=UINT16_MAX && (bank.tile>8 ||
            (bank.tile!=8 && (bank.tile/4u)*225u>=bank.tiles))))s->ok=0;
        if(s->mode==MMX4_SNAPSHOT_VALIDATE && s->ok) {
            uint8_t decoded[32768];if(!decode_bank(&bank,decoded))s->ok=0;
        }
        if(s->mode==MMX4_SNAPSHOT_READ)cache[i]=bank;
    }
    if(s->mode==MMX4_SNAPSHOT_READ)memset(cache+count,0,(4096-count)*sizeof cache[0]);
}
static void snapshot_ui_banks(Mmx4SnapshotIO *s,UiBank *cache,unsigned *live_count,uint16_t base) {
    unsigned count=mmx4_snapshot_integer(s,*live_count,4,4096);
    if(s->mode==MMX4_SNAPSHOT_READ)*live_count=count;
    for(unsigned i=0;i<count && s->ok;++i) {
        UiBank bank=cache[i];
        bank.page=(uint16_t)mmx4_snapshot_integer(s,bank.page,2,UINT16_MAX);
        bank.clut=(uint16_t)mmx4_snapshot_integer(s,bank.clut,2,UINT16_MAX);
        bank.uv=(uint16_t)mmx4_snapshot_integer(s,bank.uv,2,UINT16_MAX);
        bank.id=(uint16_t)mmx4_snapshot_integer(s,bank.id,2,UINT16_MAX);
        for(unsigned color=0;color<16;++color)
            bank.colors[color]=(uint16_t)mmx4_snapshot_integer(s,bank.colors[color],2,UINT16_MAX);
        bank.character=mmx4_snapshot_integer(s,bank.character,1,3);
        if(bank.id!=base+i || bank.page&0x180)s->ok=0;
        if(s->mode==MMX4_SNAPSHOT_READ)cache[i]=bank;
    }
    if(s->mode==MMX4_SNAPSHOT_READ)memset(cache+count,0,(4096-count)*sizeof cache[0]);
}
static int snapshot_walk(Mmx4SnapshotIO *s) {
    mmx4_snapshot_fixed(s,0x43503458u); /* X4PC, independent personal menus */
    mmx4_coop_bosses_snapshot(s);
    mmx4_coop_balance_snapshot(s);
    mmx4_snapshot_fixed(s,split_views);mmx4_snapshot_preference(s,hud_side_by_side);
    mmx4_snapshot_fixed(s,boss_door_both);
    mmx4_snapshot_preference(s,ultimate_armor_x);mmx4_snapshot_preference(s,black_zero);
    mmx4_snapshot_preference(s,mmx4_cheats_character_enabled(0));
    mmx4_snapshot_preference(s,mmx4_cheats_character_enabled(1));
    MMX4_SNAPSHOT_BOOL(s,x_variant);
    for(unsigned i=0;i<2;++i) {
        mmx4_snapshot_fixed(s,board_assets[i].assembly);
        MMX4_SNAPSHOT_U32(s,board_heroes[i]);
        MMX4_SNAPSHOT_U32(s,board_companions[i]);
        if(board_heroes[i] && (board_heroes[i]<0x80173CA0u || board_heroes[i]>=0x801754A0u ||
           (board_heroes[i]-0x80173CA0u)%0x60u))s->ok=0;
        if(board_companions[i] && (board_companions[i]<0x80173CA0u || board_companions[i]>=0x801754A0u ||
           (board_companions[i]-0x80173CA0u)%0x60u))s->ok=0;
    }
    if(!ultimate_archive.loaded)s->ok=0;
    mmx4_snapshot_fixed(s,ultimate_archive.size);
    mmx4_snapshot_fixed(s,arena);mmx4_snapshot_fixed(s,hud_arena);mmx4_snapshot_fixed(s,diagnostic);
    for(unsigned character=0;character<2;++character) {
        CharacterAssets *a=&assets[character];
        if(!a->loaded)s->ok=0;
        mmx4_snapshot_fixed(s,a->sprites);mmx4_snapshot_fixed(s,a->assembly);
        mmx4_snapshot_fixed(s,a->colors);mmx4_snapshot_fixed(s,a->palette);
        for(unsigned menu=0;menu<5;++menu)mmx4_snapshot_fixed(s,a->menu[menu]);
        mmx4_snapshot_fixed(s,a->menu_gfx);mmx4_snapshot_fixed(s,a->size);
        MMX4_SNAPSHOT_BOOL(s,a->initialized);
    }
    snapshot_player(s,&first);
    uint32_t second_offset=s->pos;
    snapshot_player(s,&second);
    unsigned saved_enrolled=mmx4_snapshot_integer(s,enrolled,1,1);
    if(s->mode==MMX4_SNAPSHOT_READ)enrolled=saved_enrolled;
    MMX4_SNAPSHOT_FIELD(s,failed,1,9);
    MMX4_SNAPSHOT_U32(s,frames);MMX4_SNAPSHOT_U16(s,previous_input);
    unsigned saved_counterpart=mmx4_snapshot_integer(s,counterpart,1,1);
    if(s->mode==MMX4_SNAPSHOT_READ)counterpart=saved_counterpart;
    if(s->mode==MMX4_SNAPSHOT_VALIDATE && s->ok && saved_enrolled &&
       s->in[second_offset+2]!=saved_counterpart)s->ok=0;
    MMX4_SNAPSHOT_FIELD(s,menu_texture_owner,1,2);
    MMX4_SNAPSHOT_BOOL(s,local_view_seat);MMX4_SNAPSHOT_FIELD(s,world_packet_used,4,FRAME_ARENA);
    MMX4_SNAPSHOT_U32(s,fixture_sequence);MMX4_SNAPSHOT_FIELD(s,fixture_invulnerable,1,3);
#ifdef MMX4_COOP_NETPLAY_DEBUG
    mmx4_snapshot_fixed(s,1);
    for(unsigned i=0;i<7;++i)MMX4_SNAPSHOT_U16(s,development_decoder.words[i]);
    MMX4_SNAPSHOT_U16(s,development_decoder.previous);
    MMX4_SNAPSHOT_FIELD(s,development_decoder.next,1,7);
    MMX4_SNAPSHOT_U32(s,development_decoder.last_sequence);
    MMX4_SNAPSHOT_BOOL(s,development_decoder.have_previous);
#else
    mmx4_snapshot_fixed(s,0);
#endif
    snapshot_sprite_banks(s,banks,&bank_count,0x6000u);
    snapshot_ui_banks(s,ui_banks,&ui_bank_count,0x8000u);
    snapshot_sprite_banks(s,view_banks,&view_bank_count,0xA000u);
    snapshot_ui_banks(s,view_ui_banks,&view_ui_bank_count,0xB000u);
    mmx4_coop_lifecycle_snapshot(s);mmx4_coop_combat_snapshot(s);
    mmx4_coop_web_snapshot(s);mmx4_coop_audio_snapshot(s);
    mmx4_coop_split_snapshot(s);mmx4_widescreen_snapshot(s);
    return s->ok;
}
static uint32_t snapshot_bytes(void) {
    Mmx4SnapshotIO s={NULL,NULL,0,0,MMX4_SNAPSHOT_COUNT,1};
    return snapshot_walk(&s)?s.pos:0;
}
static int snapshot_write(uint8_t *out,uint32_t len) {
    if(!out || !snapshot_ready())return 0;
    Mmx4SnapshotIO s={out,NULL,len,0,MMX4_SNAPSHOT_WRITE,1};
    return snapshot_walk(&s) && s.pos==len;
}
static int snapshot_validate(const uint8_t *data,uint32_t len) {
    if(!data || !snapshot_ready())return 0;
    Mmx4SnapshotIO s={NULL,data,len,0,MMX4_SNAPSHOT_VALIDATE,1};
    return snapshot_walk(&s) && s.pos==len;
}
static void snapshot_read(const uint8_t *data,uint32_t len) {
    Mmx4SnapshotIO s={NULL,data,len,0,MMX4_SNAPSHOT_READ,1};
    snapshot_walk(&s);
}
static void snapshot_resources_restored(void) {
    mmx4_cheats_restore();
    select_x_variant(counterpart==1?
        (psx_mod_read_byte(PLAY+0x37)!=0 || mmx4_cheats_character_enabled(0)):
        mmx4_cheats_character_enabled(0));
    if(enrolled && counterpart==1) {
        const CharacterAssets *a=&assets[1];
        unsigned row=mmx4_cheats_character_enabled(1)?13:0;
        for(unsigned i=0;i<16;++i)
            psx_mod_write_half(a->palette+i*2,psx_mod_read_half(a->colors+row*32+i*2));
    }
    /* Native DMA may still reference any of the four restored ID ranges. */
    psx_mod_forget_texture_bank_range(0x6000u,4096);
    psx_mod_forget_texture_bank_range(0x8000u,4096);
    psx_mod_forget_texture_bank_range(0xA000u,4096);
    psx_mod_forget_texture_bank_range(0xB000u,4096);
    psx_mod_set_texture_bank_resolver(resolve_texture_bank);
}
static void activate(void) {
    mmx4_diagnostics_reset();
    /* Immutable sprite/map banks can retain painter order in the shared
     * single-pass blend batch. Mask checking and subtractive draws keep
     * the conservative renderer path; Reference builds retain isolation. */
    psx_mod_set_texture_bank_batching(PSX_EXECUTION_ENHANCED);
    ui_layout_ready=0;memset(ui_layout,0,sizeof ui_layout);
    char cameras[16],local_cameras[16],layout[24],ultimate[8],black[8],doors[8];
    if(!psx_mod_current_option_value("cameras",cameras,sizeof cameras))strcpy(cameras,"unified");
    if(!psx_mod_current_option_value("local_cameras",local_cameras,sizeof local_cameras))strcpy(local_cameras,"unified");
    hud_side_by_side=psx_mod_current_option_value("hud_layout",layout,sizeof layout) &&
        !strcmp(layout,"side_by_side");
    ultimate_armor_x=psx_mod_current_option_value("ultimate_armor_x",ultimate,sizeof ultimate) &&
        !strcmp(ultimate,"true");
    black_zero=psx_mod_current_option_value("black_zero",black,sizeof black) && !strcmp(black,"true");
    boss_door_both=psx_mod_current_option_value("boss_doors",doors,sizeof doors) && !strcmp(doors,"both");
    mmx4_cheats_configure_coop((int)ultimate_armor_x,(int)black_zero);
    x_variant=mmx4_cheats_character_enabled(0)!=0;
    local_split=!psx_mod_netplay_is_active() && !strcmp(local_cameras,"split");
    local_aspect_set=local_shared_menu=local_board_view=0;
    if(local_split)(void)psx_mod_set_fixed_display_aspect(4,3);
    /* The selected layout belongs to the session, including menus, movies
     * and shared story frames. The presenter duplicates the canonical 4:3
     * view when no fresh private pair exists; never change the guest aspect. */
    psx_mod_set_local_pair_presentation(local_split);
    split_views=local_split || mmx4_coop_views_requested(psx_mod_netplay_is_active(),cameras);
    memset(&first,0,sizeof first);memset(&second,0,sizeof second);
    bank_count=ui_bank_count=inside=enrolled=failed=rendering=projected=camera_call=camera_update_call=hud_call=upload_guard=0;
    traced_failure=0;
    world_packet_used=0;saved_cheat=0;
    view_bank_count=view_ui_bank_count=local_view_call=local_view_seat=0;
    if(!mmx4_coop_split_activate() || !mmx4_coop_web_activate())failed=1;
    mmx4_coop_lifecycle_activate();mmx4_coop_combat_reset();
    mmx4_coop_bosses_activate();
    mmx4_coop_balance_activate();
    frames=fixture_sequence=fixture_invulnerable=previous_input=0;counterpart=0;menu_texture_owner=2;
#ifdef MMX4_COOP_NETPLAY_DEBUG
    memset(&development_decoder,0,sizeof development_decoder);
#endif
    free(ultimate_archive.file);memset(&ultimate_archive,0,sizeof ultimate_archive);
    for(unsigned i=0;i<2;++i) {
        free(assets[i].file);free(assets[i].ui_pixels);memset(&assets[i],0,sizeof assets[i]);
        uint32_t space=psx_mod_alloc_guest_memory(0x60000,16);
        assets[i].sprites=space;assets[i].assembly=space+0x34000;
        assets[i].colors=space+0x40000;assets[i].palette=space+0x41000;
        for(unsigned m=0;m<5;++m)assets[i].menu[m]=space+0x42000+m*0x1000;
        assets[i].menu_gfx=space+0x50000;
        if(!space)failed=1;
    }
    diagnostic=psx_mod_alloc_guest_memory(0x860,16); /* +0x800: scoped draw actor */
    arena=psx_mod_alloc_texture_packet_memory(2*FRAME_ARENA,16);
    hud_arena=psx_mod_alloc_texture_packet_memory(0x10000,16);
    if(!diagnostic || !arena || !hud_arena)failed=1;
    if(!mmx4_coop_audio_activate())failed=1;
    /* Host ARC/UI/SPU residency precedes a possible first-PC state load.
     * Guest character arrays are initialized only by normal native enrollment. */
    if(!failed && (!prepare_character(0,0) || !prepare_character(1,0) || !prepare_ultimate_archive()))
        psx_mod_counter_add("mmx4.coop.snapshot-assets-not-ready",1);
    if(!failed && !board_activate())psx_mod_counter_add("mmx4.coop.board-assets-unavailable",1);
    psx_mod_set_texture_bank_resolver(resolve_texture_bank);
    /* The provider captures both native contexts and persistent ownership.
     * Online user state jumps remain outside the lockstep match protocol. */
    psx_mod_set_rewind_blocked(psx_mod_netplay_is_active());
    psx_mod_set_savestate_blocked(psx_mod_netplay_is_active());
    psx_mod_counter_add("mmx4.coop.activated",1);
    psx_mod_counter_add("mmx4.coop.diagnostic-address",diagnostic);
    if(diagnostic) {
        diagnostics();
#ifdef MMX4_COOP_NETPLAY_DEBUG
        psx_mod_write_word(diagnostic+0x50,0x44455631u);
        psx_mod_write_word(diagnostic+0x48,0xFFFFFFFFu);
#endif
    }
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop) {
    static const PSXModSnapshotProvider snapshot={6,snapshot_bytes,snapshot_write,
        snapshot_validate,snapshot_read,snapshot_ready};
    psx_mod_register_snapshot_provider(ID,&snapshot);
    psx_mod_register_savestate_plugin(ID,snapshot_resources_restored);
    static const PSXModNetplayProfile profile={ID,MMX4_COOP_NETPLAY_COMPATIBILITY,0,0,1,1u,
        "mmx4.widescreen",state_digest,1,ID,"coop","cameras","split"};
    psx_mod_register_netplay_profile(&profile);
    psx_mod_register_activation_plugin(ID,activate);
    psx_mod_register_function_entry_plugin(ID,0x80035240,reset_player);
    psx_mod_register_function_entry_plugin(ID,0x80021340,second_tick);
    psx_mod_register_function_filter_plugin(ID,0x800241E8,render);
    psx_mod_register_function_filter_plugin(ID,0x80027850,camera_update);
    psx_mod_register_function_filter_plugin(ID,0x80027A5C,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x80027AAC,camera_target);
    psx_mod_register_function_filter_plugin(ID,0x8001FF50,development_fixture);
#ifndef PSX_NO_DEBUG_TOOLS
    psx_mod_register_function_filter_plugin(ID,0x80018000u,development_movie_observer);
#endif
    psx_mod_register_function_filter_plugin(ID,0x80024E70,coop_hud);
    psx_mod_register_function_filter_plugin(ID,0x80024334,render_owned_effect);
    psx_mod_register_function_filter_plugin(ID,0x800EA4D0,retained_upload);
    psx_mod_register_function_filter_plugin(ID,0x800EA80Cu,present_local_view);
    psx_mod_register_function_filter_plugin(ID,0x8002E698u,board_initialize);
    psx_mod_register_function_filter_plugin(ID,0x800C9EACu,board_character);
    psx_mod_register_function_entry_plugin(ID,0x80023DB8u,board_draw_begin);
}
