/* SLUS-00561 co-op development spike. All hook addresses are verified against
 * the original executable; generated C is never patched. The world runs once,
 * with a second native player/controller/projectile pass over private context.
 */
#include "mod_plugins.h"
#include "cpu_state.h"
#include "sio.h"
#include "mmx4_coop_assets.h"
#include "mmx4_coop_internal.h"
#include "mmx4_coop_views.h"
#include "mod_netplay.h"
#include "psx_netplay.h"
#include "gpu.h"
#include <stdlib.h>
#include <string.h>

#define ID "mmx4.coop"
#define PLAYER 0x801418C8u
#define PLAY 0x801721C0u
#define SCRATCH 0x1F800000u
#define SHOTS 0x801406F8u
#define TRAILS 0x80141AB0u
#define PAD 0x80166C08u
#define FRAME_ARENA 0x40000u
typedef struct {
    uint8_t body[0xE4], shots[0x9C0], trails[0x120], double_body[0xE4],vehicle[0xB0];
} PlayerContext;
typedef struct {
    uint32_t sprites, assembly, colors, palette;
    uint32_t menu[5],menu_gfx;
    uint16_t *ui_pixels;
    uint8_t *file; uint32_t size; Mmx4Asset compressed; unsigned loaded;
} CharacterAssets;
typedef struct { uint32_t source; uint16_t id,tile; uint8_t colors[32]; } SpriteBank;
typedef struct {uint16_t page,clut,uv,id,colors[16];unsigned character;} UiBank;
static PlayerContext second,first;
static CharacterAssets assets[2];
static SpriteBank banks[4096];
static UiBank ui_banks[4096];
static SpriteBank view_banks[4096];
static UiBank view_ui_banks[4096];
static unsigned view_bank_count,view_ui_bank_count,local_view_call,local_view_seat;
static unsigned ui_bank_count,hud_call;
static unsigned hud_side_by_side;
static uint32_t hud_arena;
static uint32_t world_packet_used;
static unsigned bank_count,inside,enrolled,failed,rendering;
static uint32_t arena,diagnostic,frames;
static uint32_t saved_resources[9];
static const unsigned resource_offsets[9]={0x14,0x1C,0x24,0x28,0x34,0x38,0x3C,0x44,0x48};
static uint8_t saved_dirty;
static uint16_t previous_input;
static unsigned counterpart;
static unsigned projected;
static unsigned camera_call;
static unsigned camera_update_call;
static unsigned split_views;
static uint32_t fixture_sequence;
static unsigned menu_texture_owner=2;
static unsigned upload_guard;
static void diagnostics(void);

static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void put32(uint8_t *p,uint32_t value) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8));
}
static void capture(uint32_t a,void *data,size_t n) {
    uint8_t *p=data;for(size_t i=0;i<n;++i)p[i]=psx_mod_read_byte(a+(uint32_t)i);
}
static void project(uint32_t a,const void *data,size_t n) {
    const uint8_t *p=data;for(size_t i=0;i<n;++i)psx_mod_write_byte(a+(uint32_t)i,p[i]);
}
static void capture_player(PlayerContext *p) {
    capture(PLAYER,p->body,sizeof p->body);capture(SHOTS,p->shots,sizeof p->shots);
    capture(TRAILS,p->trails,sizeof p->trails);capture(0x80175D58,p->double_body,sizeof p->double_body);
    capture(MMX4_VEHICLE,p->vehicle,sizeof p->vehicle);
}
static void project_player(const PlayerContext *p) {
    project(PLAYER,p->body,sizeof p->body);project(SHOTS,p->shots,sizeof p->shots);
    project(TRAILS,p->trails,sizeof p->trails);project(0x80175D58,p->double_body,sizeof p->double_body);
    project(MMX4_VEHICLE,p->vehicle,sizeof p->vehicle);
}
static uint32_t guest(CPUState *cpu,uint32_t address,uint32_t a0,uint32_t a1) {
    uint32_t r[32],data[32],ctrl[32],pc=cpu->pc,hi=cpu->hi,lo=cpu->lo;
    memcpy(r,cpu->gpr,sizeof r);memcpy(data,cpu->gte_data,sizeof data);memcpy(ctrl,cpu->gte_ctrl,sizeof ctrl);
    cpu->pc=0;cpu->gpr[4]=a0;cpu->gpr[5]=a1;
    psx_dispatch_call(cpu,address,r[31]);uint32_t result=cpu->gpr[2];
    memcpy(cpu->gpr,r,sizeof r);memcpy(cpu->gte_data,data,sizeof data);memcpy(cpu->gte_ctrl,ctrl,sizeof ctrl);
    cpu->pc=pc;cpu->hi=hi;cpu->lo=lo;return result;
}
static int retain_ui(CharacterAssets *a,const Mmx4Asset *v) {
    unsigned type=v->type&255,layout=(v->type>>8)&255;
    if(type>10 || layout>2 || v->size%2048)return 0;
    unsigned x=psx_mod_read_half(0x800F1614u+type*4),y=psx_mod_read_half(0x800F1616u+type*4),base_y=y&256;
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
static int load_character(unsigned character) {
    CharacterAssets *a=&assets[character];
    if(a->file)return (int)a->loaded;
    const char *name=character?"ARC/PL01_U.ARC":"ARC/PL00_U.ARC";
    if(!psx_mod_read_disc_file(name,NULL,0,&a->size) || !(a->file=malloc(a->size)))return 0;
    if(!psx_mod_read_disc_file(name,a->file,a->size,&a->size))return 0;
    Mmx4Asset body,assembly,colors;
    if(!mmx4_coop_arc_asset(a->file,a->size,2,&body) || body.type!=2 ||
       !mmx4_coop_arc_asset(a->file,a->size,9,&assembly) || assembly.type!=3 ||
       !mmx4_coop_arc_asset(a->file,a->size,0,&colors) || colors.type!=9)return 0;
    a->compressed=body;
    if(body.size>0x34000 || assembly.size>0xC000 || colors.size>0x1000)return 0;
    project(a->sprites,body.data,body.size);project(a->assembly,assembly.data,assembly.size);
    project(a->colors,colors.data,colors.size);
    static const unsigned members[5]={6,7,8,14,15};
    static const uint32_t limits[5]={0x1000,0x1000,0x1000,0x1000,0xA000};
    for(unsigned i=0;i<5;++i) {
        Mmx4Asset v;if(!mmx4_coop_arc_asset(a->file,a->size,members[i],&v) || v.size>limits[i])return 0;
        project(a->menu[i],v.data,v.size);
    }
    Mmx4Asset ui,gfx;
    if(!mmx4_coop_arc_asset(a->file,a->size,1,&ui) ||
       !mmx4_coop_arc_asset(a->file,a->size,5,&gfx) || gfx.size!=0x8000)return 0;
    project(a->menu_gfx,gfx.data,gfx.size);
    a->ui_pixels=calloc(1024*512,sizeof(uint16_t));
    a->loaded=a->ui_pixels && retain_ui(a,&ui) && retain_ui(a,&gfx) &&
        mmx4_coop_audio_load(character,a->file,a->size);
    return (int)a->loaded;
}
static void enter_second(void) {
    projected=1;
    capture_player(&first);project_player(&second);
    mmx4_coop_lifecycle_project();
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
    for(unsigned i=0;i<9;++i)psx_mod_write_word(SCRATCH+resource_offsets[i],saved_resources[i]);
    psx_mod_write_byte(0x80166BB0,saved_dirty);
    projected=0;
    diagnostics();
}
int mmx4_coop_ready(void) { return enrolled && !failed; }
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
    for(unsigned i=0;i<sizeof second.shots;++i)psx_mod_write_byte(SHOTS+i,0);
    for(unsigned i=0;i<sizeof second.trails;++i)psx_mod_write_byte(TRAILS+i,0);
    for(unsigned i=0;i<sizeof second.double_body;++i)psx_mod_write_byte(0x80175D58u+i,0);
    for(unsigned i=0x8C;i<=0x92;++i)psx_mod_write_byte(PLAYER+i,0);
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
    return (uint16_t)((raw<<8)|(raw>>8));
}
void mmx4_coop_menu_assets(CPUState *cpu,unsigned seat) {
    unsigned character=seat?counterpart:(counterpart^1u);
    if(character==menu_texture_owner || character>1 || !load_character(character))return;
    uint32_t rect=diagnostic+0x80;
    psx_mod_write_half(rect,832);psx_mod_write_half(rect+2,256);
    psx_mod_write_half(rect+4,64);psx_mod_write_half(rect+6,256);
    guest(cpu,0x800EA4D0,rect,assets[character].menu_gfx);
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
    psx_mod_write_word(diagnostic,0x5834434Fu);psx_mod_write_word(diagnostic+4,frames);
    project(diagnostic+0x100,second.body,sizeof second.body);
    project(diagnostic+0x200,second.vehicle,sizeof second.vehicle);
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
    (void)cpu;(void)address;if(inside)return;
    mmx4_coop_lifecycle_reset();
    mmx4_coop_combat_reset();
    mmx4_coop_split_reset();
    menu_texture_owner=2;
    enrolled=0;memset(&second,0,sizeof second);previous_input=0;
    if(diagnostic)diagnostics();
}
static int enroll(CPUState *cpu) {
    if(!psx_mod_texture_banks_supported())return 0;
    counterpart=psx_mod_read_byte(PLAYER+2)^1;
    if(counterpart>1 || !load_character(counterpart))return 0;
    uint8_t palette[0x1000],play[0x64];
    capture(psx_mod_read_word(SCRATCH+0x28),palette,sizeof palette);
    project(assets[counterpart].palette,palette,sizeof palette);
    capture(PLAY,play,sizeof play);
    uint32_t x=psx_mod_read_word(PLAYER+8),y=psx_mod_read_word(PLAYER+12);
    unsigned stage=psx_mod_read_byte(PLAY+0xC);
    unsigned on_chaser=psx_mod_read_byte(PLAYER+0xC5)==0xFF;
    /* Native player initialization disables the shared camera at 8003558C.
     * P1 already completed its spawn; P2 must not disable that camera again. */
    uint8_t camera_enabled[3]={psx_mod_read_byte(0x801419F4u),
        psx_mod_read_byte(0x80141A48u),psx_mod_read_byte(0x80141A9Cu)};
    uint32_t spawn_x=x+((stage==0 || stage==5)?24u<<16:0);
    enter_second();psx_mod_write_byte(PLAY+0x43,(uint8_t)counterpart);
    psx_mod_write_byte(PLAY+0x1E,0);
    guest(cpu,0x80035240,0,0);
    psx_mod_write_word(PLAYER+8,spawn_x);psx_mod_write_word(PLAYER+12,y);
    psx_mod_write_byte(PLAYER+3,1);psx_mod_write_byte(PLAYER+4,1);psx_mod_write_byte(PLAYER+5,2);
    guest(cpu,0x800350A4,PLAYER,0);guest(cpu,0x80035EA4,PLAYER,0);
    psx_mod_write_word(PLAYER+0x18,spawn_x);psx_mod_write_word(PLAYER+0x1C,y);
    if(on_chaser) {
        guest(cpu,0x80035A24,PLAYER,0);guest(cpu,0x80021C14,0,0);
    }
    guest(cpu,0x8002C614,PLAYER,0);
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
    if(!enrolled && psx_mod_read_byte(PLAYER+4)==1 && !psx_mod_read_byte(PLAY+0x1C)) {
        if(!enroll(cpu))failed=1;
    }
    if(enrolled && !failed)mmx4_coop_lifecycle_tick(cpu);
    if(enrolled && !failed && mmx4_coop_lifecycle_can_tick()) {
        uint8_t input[6];capture(PAD,input,sizeof input);
        uint8_t camera_view[0xFC],camera_origins[3][20];
        int own_camera=split_views && mmx4_coop_split_camera_copy(1,camera_view);
        if(own_camera)for(unsigned layer=0;layer<3;++layer) {
            uint32_t camera=0x801419B0u+layer*0x54u+8u;
            capture(camera,camera_origins[layer],20);
            project(camera,camera_view+layer*0x54u+8u,20);
        }
        uint16_t raw=mmx4_coop_input(1);
        enter_second();
        psx_mod_write_half(PAD,raw);psx_mod_write_half(PAD+2,previous_input);
        psx_mod_write_half(PAD+4,(uint16_t)(raw&~previous_input));previous_input=raw;
        guest(cpu,0x80035EF0,0,0);guest(cpu,0x80021C14,0,0);guest(cpu,0x800311EC,0,0);
        guest(cpu,0x80021340,0,0);guest(cpu,0x8002C614,PLAYER,0);
        leave_second();project(PAD,input,sizeof input);++frames;
        if(own_camera)for(unsigned layer=0;layer<3;++layer)
            project(0x801419B0u+layer*0x54u+8u,camera_origins[layer],20);
        psx_mod_counter_add("mmx4.coop.player-ticks",1);
    }
    diagnostics();inside=0;
}

static int camera_target(CPUState *cpu,uint32_t address) {
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
    if(split_views || projected || !mmx4_coop_ready() || mmx4_coop_lifecycle_script_owner()>=0 ||
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
    int bottom=(int16_t)psx_mod_read_half(0x801419B0u+0x20)+256;
    for(unsigned seat=0;seat<2;++seat) {
        int y=seat?(int16_t)(le32(second.body+12)>>16):(int16_t)psx_mod_read_half(PLAYER+14);
        if(mmx4_coop_alive(seat) && y-8>=bottom) {
            if(seat)second.body[0x5C]=0x80;else psx_mod_write_byte(PLAYER+0x5C,0x80);
        }
    }
}
static int camera_update(CPUState *cpu,uint32_t address) {
    if(camera_update_call || projected)return 0;
    constrain_team(cpu,address);
    if(!mmx4_coop_camera_scope_requested((int)split_views,mmx4_coop_ready(),
        mmx4_coop_alive(0),mmx4_coop_alive(1)))return 0;
    unsigned owner=mmx4_coop_lifecycle_script_owner()==1 || !mmx4_coop_alive(0);
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

/* Private QA requests are committed at the native gameplay dispatch, never
 * by interleaving multiple TCP RAM writes with a running world update. */
static int development_fixture(CPUState *cpu,uint32_t address) {
    (void)address;
#ifndef PSX_NO_DEBUG_TOOLS
    if(psx_mod_netplay_is_active() || projected || !diagnostic)return 0;
    uint32_t sequence=psx_mod_read_word(diagnostic+0x30);
    if(sequence==fixture_sequence)return 0;
    fixture_sequence=sequence;
    unsigned command=psx_mod_read_byte(diagnostic+0x34);
    if(command==1) {
        unsigned stage=psx_mod_read_byte(diagnostic+0x35),section=psx_mod_read_byte(diagnostic+0x36);
        unsigned campaign=psx_mod_read_byte(diagnostic+0x37);
        if(stage<=12 && section<=1 && campaign<=1) {
            psx_mod_write_byte(PLAY+0x43,(uint8_t)campaign);
            psx_mod_write_byte(PLAY+0xC,(uint8_t)stage);psx_mod_write_byte(PLAY+0xD,(uint8_t)section);
            psx_mod_write_word(PLAY,4);psx_mod_write_byte(PLAY+0x1D,0);psx_mod_write_byte(PLAY+0x1E,0);
            psx_mod_write_word(diagnostic+0x38,sequence);
            return mmx4_coop_finish(cpu,0);
        }
    }else if(command==3 && mmx4_coop_ready()) {
        unsigned checkpoint=psx_mod_read_byte(diagnostic+0x35);
        unsigned stage=psx_mod_read_byte(PLAY+0xC),section=psx_mod_read_byte(PLAY+0xD);
        /* Only original spawn lists whose bounds have been verified. */
        if((stage==0 && section==1 && checkpoint<3) ||
           (stage==1 && section==1 && checkpoint<6) ||
           (stage==12 && section==1 && checkpoint<7)) {
            psx_mod_write_byte(PLAY+0x1D,(uint8_t)checkpoint);
            psx_mod_write_byte(PLAY+0x1E,0);psx_mod_write_word(PLAY,5);
            psx_mod_write_word(diagnostic+0x38,sequence);
            return mmx4_coop_finish(cpu,0);
        }
    }else if(command==2 && mmx4_coop_ready()) {
        unsigned seat=psx_mod_read_byte(diagnostic+0x35),hp=psx_mod_read_byte(diagnostic+0x36);
        if(seat==0)psx_mod_write_byte(PLAYER+0x5C,(uint8_t)hp);
        else if(seat==1)second.body[0x5C]=(uint8_t)hp;
    }
    psx_mod_write_word(diagnostic+0x38,sequence);
#else
    (void)cpu;
#endif
    return 0;
}

static uint16_t sprite_bank(uint32_t table,unsigned frame,uint16_t clut,uint16_t tile) {
    unsigned *count=psx_mod_local_view_scope()?&view_bank_count:&bank_count;
    SpriteBank *cache=psx_mod_local_view_scope()?view_banks:banks;
    CharacterAssets *a=&assets[counterpart];uint8_t decoded[32768],colors[32];uint16_t pixels[256*256];
    if(table<a->sprites || table-a->sprites>=a->compressed.size || clut<0x7800)return 0;
    size_t offset=table-a->sprites;
    if(frame*4+4>a->compressed.size-offset)return 0;
    uint32_t packed=le32(a->compressed.data+offset+frame*4),source=table+(packed&0xFFFFF);
    unsigned tiles=packed>>20;size_t pal=(size_t)(clut-0x7800)*32,written;
    if(!tiles || tiles>256 || source<a->sprites || source-a->sprites>=a->compressed.size || pal+32>0x1000)return 0;
    capture(a->palette+(uint32_t)pal,colors,sizeof colors);
    for(unsigned i=0;i<*count;++i)if(cache[i].source==source && cache[i].tile==tile && !memcmp(cache[i].colors,colors,32))return cache[i].id;
    if(*count==4096 || !mmx4_coop_decode_sprite(a->compressed.data+source-a->sprites,
        a->compressed.size-(source-a->sprites),decoded,sizeof decoded,&written) || written!=tiles*128u)return 0;
    memset(pixels,0,sizeof pixels);
    unsigned remaining=tiles,row=0,in=0;
    while(remaining) {
        unsigned chunk=remaining<16?remaining:16,width=chunk*16;
        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<width;++x) {
            unsigned index=(decoded[in+(y*width+x)/2]>>((x&1)*4))&15;
            pixels[(row+y)*256+x]=(uint16_t)(colors[index*2]|colors[index*2+1]<<8);
        }
        in+=chunk*128;row+=16;remaining-=chunk;
    }
    uint16_t id=(uint16_t)((psx_mod_local_view_scope()?0xA000u:0x6000u)+*count);
    if(tile!=UINT16_MAX) {
        uint16_t part[256];for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x)
            part[y*16+x]=pixels[(((tile>>8)+y)&255)*256+(((tile&255)+x)&255)];
        if(!psx_mod_define_texture_bank(id,16,16,part))return 0;
    }else if(!psx_mod_define_texture_bank(id,256,256,pixels))return 0;
    cache[*count]=(SpriteBank){source,id,tile,{0}};memcpy(cache[(*count)++].colors,colors,32);return id;
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
static uint16_t ui_tile(uint16_t page,uint16_t clut,uint16_t uv,int private_colors) {
    unsigned *count=psx_mod_local_view_scope()?&view_ui_bank_count:&ui_bank_count;
    UiBank *cache=psx_mod_local_view_scope()?view_ui_banks:ui_banks;
    if(page&0x180 || !assets[counterpart].ui_pixels)return 0;
    uint16_t colors[16],pixels[256*256];
    unsigned cx=(clut&63)*16,cy=clut>>6;
    const uint16_t *vram=gpu_get_vram();
    for(unsigned i=0;i<16;++i)colors[i]=private_colors && clut>=0x7800 && clut<0x7880?
        psx_mod_read_half(assets[counterpart].palette+(clut-0x7800)*32+i*2):vram[cy*1024+cx+i];
    for(unsigned i=0;i<*count;++i) {
        UiBank *b=&cache[i];
        if(b->character==counterpart && b->page==page && b->clut==clut && b->uv==uv && !memcmp(b->colors,colors,32))return b->id;
    }
    if(*count>=4096)return 0;
    unsigned bx=(page&15)*64,by=(page&16)*16;
    unsigned width=uv==UINT16_MAX?256:16;
    for(unsigned y=0;y<width;++y)for(unsigned x=0;x<width;++x) {
        unsigned u=uv==UINT16_MAX?x:((uv&255)+x)&255,v=uv==UINT16_MAX?y:((uv>>8)+y)&255;
        uint16_t packed=assets[counterpart].ui_pixels[(by+v)*1024+bx+u/4];
        pixels[y*width+x]=colors[(packed>>((u&3)*4))&15];
    }
    uint16_t id=(uint16_t)((psx_mod_local_view_scope()?0xB000u:0x8000u)+*count);
    if(!psx_mod_define_texture_bank(id,width,width,pixels))return 0;
    UiBank *b=&cache[(*count)++];
    *b=(UiBank){page,clut,uv,id,{0},counterpart};memcpy(b->colors,colors,32);return id;
}
static int coop_hud(CPUState *cpu,uint32_t address) {
    if(hud_call || projected || !mmx4_coop_ready() || psx_mod_read_byte(PLAY)!=6 ||
       psx_mod_read_byte(PLAY+1)!=0)return 0;
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
            uint16_t page=(uint16_t)psx_mod_read_word(mode+4),bank=ui_tile(page,(uint16_t)(uv>>16),(uint16_t)uv,0);
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
static void render_actor(CPUState *cpu,uint32_t actor,uint32_t base) {
    uint32_t start=psx_mod_read_word(SCRATCH+0x100),mode=psx_mod_read_word(SCRATCH+0x104);
    guest(cpu,0x80024334,actor,0);
    uint32_t end=psx_mod_read_word(SCRATCH+0x100);
    if(end<start || end-base>40000 || (end-start)%40) { failed=4;return; }
    uint32_t table=psx_mod_read_word(actor+0x38);unsigned frame=psx_mod_read_byte(actor+0x47);
    if(!table && psx_mod_read_word(actor+0x3C)==(projected?psx_mod_read_word(PLAYER+0x3C):le32(second.body+0x3C))) {
        table=projected?psx_mod_read_word(PLAYER+0x38):le32(second.body+0x38);
    }
    int compressed=table>=assets[counterpart].sprites && table-assets[counterpart].sprites<assets[counterpart].compressed.size;
    uint32_t assembly=psx_mod_read_word(actor+0x3C);
    if(!compressed && (assembly<assets[counterpart].assembly || assembly-assets[counterpart].assembly>=0xC000))return;
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
        uint16_t page=tile==UINT16_MAX?(uint16_t)(q[5]>>16):(uint16_t)psx_mod_read_word(mode+4);
        uint16_t bank=compressed?sprite_bank(table,frame,(uint16_t)(q[3]>>16),tile):
            ui_tile(page,(uint16_t)(q[3]>>16),tile,1);
        if(!bank) {failed=8;psx_mod_counter_add("mmx4.coop.invalid-sprite",1);return;}
        triangle(expanded,expanded+40,q,0,1,2,bank);triangle(expanded+40,q[0],q,2,1,3,bank);
        psx_mod_tag_world_primitive(expanded,1);psx_mod_tag_world_primitive(expanded+40,1);
        psx_mod_write_word(at,expanded&0xFFFFFF);
        unsigned priority=psx_mod_read_byte(actor+0x16);
        uint32_t tail=0x8013BC40u+(psx_mod_read_word(SCRATCH)&1u)*128+(priority>>4)*32+(priority&15)*4;
        if(psx_mod_read_word(tail)==at)psx_mod_write_word(tail,expanded+40);
    }
}
static int render(CPUState *cpu,uint32_t address) {
    if(rendering || inside || projected || failed || !enrolled || psx_mod_read_byte(PLAY)!=6 ||
       psx_mod_read_byte(PLAY+1)!=0)return 0;
    rendering=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    enter_second();
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_ARENA;
    psx_mod_write_word(SCRATCH+0x100,base);
    if(second.body[3])render_actor(cpu,PLAYER,base);
    if(psx_mod_read_byte(MMX4_VEHICLE+3))render_actor(cpu,MMX4_VEHICLE,base);
    for(unsigned i=0;i<16 && !failed;++i)if(psx_mod_read_byte(SHOTS+i*0x9C+3))render_actor(cpu,SHOTS+i*0x9C,base);
    world_packet_used=psx_mod_read_word(SCRATCH+0x100)-base;
    psx_mod_write_word(SCRATCH+0x100,original);leave_second();rendering=0;
    return mmx4_coop_finish(cpu,result);
}
static int render_owned_effect(CPUState *cpu,uint32_t address) {
    (void)address;
    if(rendering || projected || !mmx4_coop_ready() || psx_mod_read_byte(PLAY)!=6 ||
       psx_mod_read_byte(PLAY+1)!=0)return 0;
    uint32_t actor=cpu->gpr[4],assembly=psx_mod_read_word(actor+0x3C);
    if(assembly<assets[counterpart].assembly || assembly-assets[counterpart].assembly>=0xC000)return 0;
    rendering=1;
    uint32_t original=psx_mod_read_word(SCRATCH+0x100),base=arena+(psx_mod_read_word(SCRATCH)&1u)*FRAME_ARENA;
    psx_mod_write_word(SCRATCH+0x100,base+world_packet_used);
    render_actor(cpu,actor,base);world_packet_used=psx_mod_read_word(SCRATCH+0x100)-base;
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
    return hash_bytes(hash,p->vehicle,sizeof p->vehicle);
}
static uint32_t state_digest(void) {
    uint32_t hash=2166136261u;
    hash=hash_word(hash,split_views);
    hash=hash_word(hash,enrolled);hash=hash_word(hash,failed);hash=hash_word(hash,projected);
    hash=hash_word(hash,counterpart);hash=hash_word(hash,previous_input);hash=hash_word(hash,frames);
    hash=hash_player(hash,&second);
    if(projected) {
        hash=hash_player(hash,&first);hash=hash_word(hash,saved_dirty);
        for(unsigned i=0;i<9;++i)hash=hash_word(hash,saved_resources[i]);
    }
    return mmx4_coop_split_digest(mmx4_coop_combat_digest(mmx4_coop_lifecycle_digest(hash)));
}
typedef struct {
    PlayerContext first,second;
    uint32_t resources[9],world_packet_used;
    unsigned projected,rendering,hud_call,upload_guard,failed;
    uint8_t dirty;
} LocalViewContext;
static int draw_local_view(CPUState *cpu,void *user,uint32_t alpha) {
    (void)alpha;
    uint32_t buffer=*(uint32_t *)user;
    uint8_t camera[0xFC];
    if(!mmx4_coop_split_camera_copy(local_view_seat,camera))return 0;
    project(0x801419B0u,camera,sizeof camera);
    gpu_set_gp0_source(0);
    gpu_write_gp0(0x02000000u|psx_mod_read_byte(buffer+0x2Du)|
        (uint32_t)psx_mod_read_byte(buffer+0x2Eu)<<8|
        (uint32_t)psx_mod_read_byte(buffer+0x2Fu)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x14u)|(uint32_t)psx_mod_read_half(buffer+0x16u)<<16);
    gpu_write_gp0(psx_mod_read_half(buffer+0x18u)|(uint32_t)psx_mod_read_half(buffer+0x1Au)<<16);
    guest(cpu,0x800EA714u,buffer+0x70u,12);
    guest(cpu,0x80023D68u,0,0);
    if(failed || projected || rendering || hud_call)return 0;
    guest(cpu,0x800EC228u,buffer+0x9Cu,0);
    return !failed;
}
static int present_local_view(CPUState *cpu,uint32_t address) {
    if(!split_views || local_view_call || projected || inside || !mmx4_coop_ready() ||
       psx_mod_read_byte(PLAY)!=6 || psx_mod_read_byte(PLAY+1)!=0 ||
       cpu->gpr[31]!=0x80012080u || mmx4_coop_lifecycle_script_owner()>=0 ||
       psx_mod_read_byte(PLAY+0x0F) || psx_mod_read_byte(PLAY+0x10) ||
       psx_mod_read_byte(PLAY+0x1C))return 0;
    uint32_t buffer=psx_mod_read_word(0x80142F80u);
    if(cpu->gpr[4]!=buffer+0x9Cu)return 0;
    local_view_call=1;
    uint32_t result=guest(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    guest(cpu,0x800EA20Cu,0,0);
    int port=psx_netplay_local_port();
    local_view_seat=mmx4_coop_split_view_seat(port>=0 && port<2?(unsigned)port:0);
    PSXModRenderPass rectangle={sizeof rectangle,0,
        psx_mod_read_half(buffer+0x14u),psx_mod_read_half(buffer+0x16u),
        psx_mod_read_half(buffer+0x18u),psx_mod_read_half(buffer+0x1Au)};
    LocalViewContext context={first,second,{0},world_packet_used,
        projected,rendering,hud_call,upload_guard,failed,saved_dirty};
    memcpy(context.resources,saved_resources,sizeof saved_resources);
    mmx4_coop_lifecycle_view_save();mmx4_widescreen_view_save();
    uint32_t before=state_digest();
    int committed=psx_mod_render_local_view(cpu,&rectangle,draw_local_view,&buffer);
    first=context.first;second=context.second;
    memcpy(saved_resources,context.resources,sizeof saved_resources);
    world_packet_used=context.world_packet_used;saved_dirty=context.dirty;
    projected=context.projected;rendering=context.rendering;hud_call=context.hud_call;
    upload_guard=context.upload_guard;failed=context.failed;
    mmx4_coop_lifecycle_view_restore();mmx4_widescreen_view_restore();
    if(state_digest()!=before)psx_mod_counter_add("mmx4.coop.local-view-state-leak",1);
    psx_mod_counter_add(committed?"mmx4.coop.local-view-committed":"mmx4.coop.local-view-refused",1);
    if(committed)psx_mod_counter_add(local_view_seat?
        "mmx4.coop.local-view-seat-1":"mmx4.coop.local-view-seat-0",1);
    local_view_call=0;
    return mmx4_coop_finish(cpu,result);
}
static void activate(void) {
    char cameras[16],layout[24];
    if(!psx_mod_current_option_value("cameras",cameras,sizeof cameras))strcpy(cameras,"unified");
    hud_side_by_side=psx_mod_current_option_value("hud_layout",layout,sizeof layout) &&
        !strcmp(layout,"side_by_side");
    split_views=mmx4_coop_views_requested(psx_mod_netplay_is_active(),cameras);
    memset(&first,0,sizeof first);memset(&second,0,sizeof second);
    mmx4_coop_audio_reset();
    bank_count=ui_bank_count=inside=enrolled=failed=rendering=projected=camera_call=camera_update_call=hud_call=upload_guard=0;
    view_bank_count=view_ui_bank_count=local_view_call=local_view_seat=0;
    if(!mmx4_coop_split_activate())failed=1;
    frames=fixture_sequence=previous_input=0;counterpart=0;menu_texture_owner=2;
    for(unsigned i=0;i<2;++i) {
        free(assets[i].file);free(assets[i].ui_pixels);memset(&assets[i],0,sizeof assets[i]);
        uint32_t space=psx_mod_alloc_guest_memory(0x60000,16);
        assets[i].sprites=space;assets[i].assembly=space+0x34000;
        assets[i].colors=space+0x40000;assets[i].palette=space+0x41000;
        for(unsigned m=0;m<5;++m)assets[i].menu[m]=space+0x42000+m*0x1000;
        assets[i].menu_gfx=space+0x50000;
        if(!space)failed=1;
    }
    diagnostic=psx_mod_alloc_guest_memory(0x500,16);
    arena=psx_mod_alloc_texture_packet_memory(2*FRAME_ARENA,16);
    hud_arena=psx_mod_alloc_texture_packet_memory(0x10000,16);
    if(!diagnostic || !arena || !hud_arena)failed=1;
    psx_mod_set_rewind_blocked(1);
    psx_mod_set_savestate_blocked(1);
    psx_mod_counter_add("mmx4.coop.activated",1);
    psx_mod_counter_add("mmx4.coop.diagnostic-address",diagnostic);
    if(diagnostic)diagnostics();
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop) {
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
    psx_mod_register_function_filter_plugin(ID,0x80024E70,coop_hud);
    psx_mod_register_function_filter_plugin(ID,0x80024334,render_owned_effect);
    psx_mod_register_function_filter_plugin(ID,0x800EA4D0,retained_upload);
    psx_mod_register_function_filter_plugin(ID,0x800EA80Cu,present_local_view);
}
