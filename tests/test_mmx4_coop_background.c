#include "mmx4_coop_internal.h"
#include "mod_memory.h"
#include "gpu.h"
#include "ws_view_anchor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t memory[32u*1024u*1024u];
static unsigned split=1,local=1,tiles;
uint8_t psx_mod_read_byte(uint32_t p){return memory[p&0x1ffffffu];}
uint16_t psx_mod_read_half(uint32_t p){return psx_mod_read_byte(p)|(uint16_t)psx_mod_read_byte(p+1)<<8;}
uint32_t psx_mod_read_word(uint32_t p){return psx_mod_read_half(p)|(uint32_t)psx_mod_read_half(p+2)<<16;}
void psx_mod_write_byte(uint32_t p,uint8_t v){memory[p&0x1ffffffu]=v;}
void psx_mod_write_half(uint32_t p,uint16_t v){psx_mod_write_byte(p,(uint8_t)v);psx_mod_write_byte(p+1,(uint8_t)(v>>8));}
void psx_mod_write_word(uint32_t p,uint32_t v){psx_mod_write_half(p,(uint16_t)v);psx_mod_write_half(p+2,(uint16_t)(v>>16));}
void psx_mod_counter_add(const char *name,uint32_t amount){if(!strcmp(name,"mmx4.renderer.tiles"))tiles+=amount;}
int mmx4_coop_split_views(void){return (int)split;}
int mmx4_coop_ready(void){return 1;}
int mmx4_coop_projected(void){return 0;}
int mmx4_coop_alive(unsigned seat){(void)seat;return 1;}
int mmx4_coop_lifecycle_script_owner(void){return -1;}
int mmx4_coop_split_camera_copy(unsigned seat,uint8_t camera[0xFC]){(void)seat;(void)camera;return 0;}
int psx_mod_local_view_scope(void){return (int)local;}
int32_t psx_mod_widescreen_x_margin(void){return 0;}
int gpu_ws_bg2d_get_view(unsigned layer,WsViewAnchor *view){(void)layer;(void)view;return 0;}
void gpu_ws_bg2d_begin_view_layer(unsigned l,uint32_t p,unsigned m){(void)l;(void)p;(void)m;}
void gpu_ws_bg2d_end_view_layer(unsigned l,uint32_t p){(void)l;(void)p;}
void gpu_ws_bg2d_set_host_arena(uint32_t p,uint32_t n){(void)p;(void)n;}
void gpu_ws_bg2d_set_parent_links(int enabled){(void)enabled;}
void gpu_ws_set_view_anchor(uint32_t a,uint32_t b,uint32_t c,uint32_t d){(void)a;(void)b;(void)c;(void)d;}
void gpu_ws_bg2d_configure(uint32_t a,uint32_t b,uint32_t c,uint32_t d,uint32_t e,uint32_t f,uint32_t g,uint32_t h){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;}
void gpu_ws_tag_hud_prim(uint32_t p,int a){(void)p;(void)a;}
void psx_mod_tag_world_primitive(uint32_t p,int w){(void)p;(void)w;}
uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t n,uint32_t a){(void)n;(void)a;return 0x01000000u;}
uint32_t psx_mod_memory_alloc(uint32_t n,uint32_t a){(void)n;(void)a;return 0x801E0000u;}
int psx_mod_register_activation_plugin(const char *id,PSXModActivationCallback fn){(void)id;(void)fn;return 1;}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t p,PSXModFunctionEntryCallback fn){(void)id;(void)p;(void)fn;return 1;}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t p,PSXModFunctionFilterCallback fn){(void)id;(void)p;(void)fn;return 1;}
int psx_mod_register_instruction_plugin(const char *id,uint32_t p,uint32_t w,PSXModFunctionEntryCallback fn){(void)id;(void)p;(void)w;(void)fn;return 1;}
int psx_mod_netplay_is_active(void){return 1;}
int psx_mod_netplay_aspect(void){return 0;}
int psx_mod_set_fixed_display_aspect(uint32_t n,uint32_t d){(void)n;(void)d;return 1;}
int psx_mod_set_adaptive_display_aspect(uint32_t n,uint32_t d){(void)n;(void)d;return 1;}
int psx_mod_option_value(const char *p,const char *f,const char *o,char *v,uint32_t n){(void)p;(void)f;(void)o;(void)v;(void)n;return 0;}
void psx_dispatch_call(CPUState *cpu,uint32_t p,uint32_t c){(void)cpu;(void)p;(void)c;}
#include "../src/mods/mmx4_widescreen_plugin.c"
int main(void){
    arena=0x01000000u;
    const uint32_t map=0x80100000u,blocks=0x80101000u,descriptors=0x80102000u,packet=0x80103000u;
    psx_mod_write_word(0x1F800004u,map);psx_mod_write_word(0x1F800008u,blocks);
    psx_mod_write_word(0x1F80000Cu,descriptors);psx_mod_write_word(0x1F800108u,packet);
    psx_mod_write_byte(0x80172224u,2);psx_mod_write_half(0x8013BD48u,2);
    psx_mod_write_byte(LAYERS+0x4Eu,1);
    psx_mod_write_byte(map,1);psx_mod_write_byte(map+1,2);
    for(unsigned i=0;i<256;++i){psx_mod_write_half(blocks+512u+i*2,1);psx_mod_write_half(blocks+1024u+i*2,2);}
    psx_mod_write_word(descriptors+4,0x00010000u);psx_mod_write_word(descriptors+8,0x00020000u);
    CPUState cpu={0};cpu.gpr[4]=0;
    mmx4_bg_end(&cpu,0x80026894u);
    CHECK(tiles==22u*16u);
    CHECK((psx_mod_read_word(arena+12)&0xFFFFu)==0x10u);
    /* Canonical scrolling replaces the shared ring, while this camera and
     * its authored world position stay fixed. The local pixels keep tile 1. */
    for(unsigned i=0;i<32u*32u;++i)psx_mod_write_half(0x801441C8u+i*2,2);
    mmx4_bg_end(&cpu,0x80026894u);
    CHECK(tiles==2u*22u*16u && (psx_mod_read_word(arena+12)&0xFFFFu)==0x10u);
    split=0;mmx4_bg_end(&cpu,0x80026894u);CHECK(tiles==2u*22u*16u);
    split=1;local=0;mmx4_bg_end(&cpu,0x80026894u);CHECK(tiles==2u*22u*16u);
    puts("mmx4_coop_background_test: PASS");return 0;
}
