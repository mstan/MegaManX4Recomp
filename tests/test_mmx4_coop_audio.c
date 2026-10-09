#include "mmx4_coop_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
#define SOUND 0x8001540Cu
#define KEY_ON 0x800DFCFCu
static uint8_t ram[0x200000],private_ram[0x10000];
static uint32_t allocated,registered,opened,native_sound,pending_bank;
static unsigned projected=1,ready=1,local_view;
static unsigned transfer_busy;
static PSXModFunctionFilterCallback sound_filter,key_filter;
static uint8_t *ptr(uint32_t a) {
    if((a&0xFF000000u)==0x8F000000u) {CHECK(a-0x8F000000u<sizeof private_ram);return private_ram+a-0x8F000000u;}
    CHECK((a&0x1FFFFFFFu)<sizeof ram);return ram+(a&0x1FFFFFFFu);
}
uint8_t psx_mod_read_byte(uint32_t a) {return *ptr(a);}
void psx_mod_write_byte(uint32_t a,uint8_t v) {*ptr(a)=v;}
uint32_t psx_mod_read_word(uint32_t a) {return (uint32_t)*ptr(a)|(uint32_t)*ptr(a+1)<<8|(uint32_t)*ptr(a+2)<<16|(uint32_t)*ptr(a+3)<<24;}
void psx_mod_write_word(uint32_t a,uint32_t v) {for(unsigned i=0;i<4;++i)*ptr(a+i)=(uint8_t)(v>>(i*8));}
int psx_mod_host_write_ram(uint32_t a,const void *p,uint32_t n) {memcpy(ptr(a),p,n);return 1;}
uint32_t psx_mod_alloc_guest_memory(uint32_t n,uint32_t align) {(void)align;uint32_t a=0x8F000000u+allocated;allocated+=n;CHECK(allocated<=sizeof private_ram);return a;}
int psx_mod_spu_sample_bank(uint32_t b,const void *p,uint32_t n) {CHECK(b>=12 && b<16 && p && n==32);registered|=1u<<b;return 1;}
int psx_mod_spu_bind_voice_bank(unsigned v,uint32_t b) {CHECK(v==20);pending_bank=b;return 1;}
int psx_mod_local_view_scope(void) {return local_view;}
void psx_mod_counter_add(const char *n,uint32_t v) {(void)n;(void)v;}
int mmx4_coop_ready(void) {return ready;}
int mmx4_coop_projected(void) {return projected;}
int mmx4_coop_finish(CPUState *c,uint32_t v) {c->gpr[2]=v;return 1;}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t a,PSXModFunctionFilterCallback cb) {
    CHECK(!strcmp(id,"mmx4.coop"));if(a==SOUND)sound_filter=cb;else {CHECK(a==KEY_ON);key_filter=cb;}return 1;
}
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t a,uint32_t a0,uint32_t a1) {
    if(a==0x800E4398u) {
        CHECK(cpu->gpr[6]==0 && !memcmp(ptr(a0),"pBAV",4));
        CHECK(!transfer_busy);transfer_busy=1;
        psx_mod_write_byte(0x80166D58u+a1,2);++opened;return a1;
    }
    if(a==0x800E1A60u) {CHECK(!a0 && transfer_busy);transfer_busy=0;return 0;}
    if(a==KEY_ON) {CHECK(!key_filter(cpu,a));return 0x1234;}
    CHECK(a==SOUND && !sound_filter(cpu,a));++native_sound;
    unsigned group=a0&0x7F,character=psx_mod_read_byte(MMX4_PLAYER+2);
    uint32_t record=psx_mod_read_word(0x80141F50u+group*4);
    CHECK((psx_mod_read_byte(record)&0xC0)==0x80);
    unsigned id=psx_mod_read_byte(record)&0x3F;
    CHECK(id==12+character*2+(group==3));
    CHECK(!transfer_busy && psx_mod_read_byte(0x80166D58u+id)==1);
    CHECK(psx_mod_read_word(0x80141EE8u+group*4)==record+4);
    CPUState voice={0};voice.gpr[4]=20;voice.gpr[5]=id;
    CHECK(key_filter(&voice,KEY_ON) && pending_bank==id);return 0xABCD;
}
static void put(uint8_t *p,uint32_t n) {for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(n>>(i*8));}
int main(void) {
    CHECK(sound_filter && key_filter);mmx4_coop_audio_reset();
    uint8_t full[10240]={0};put(full,14);put(full+4,sizeof full);
    /* Zero-sized unrelated members, then two typed header/body pairs. */
    for(unsigned i=10;i<14;++i) {put(full+8+i*8,(i&1)?(i==11?0x20101:0x20301):6);put(full+12+i*8,(i&1)?32:44);}
    for(unsigned at=2048;at<=6144;at+=4096) {
        put(full+at,12);put(full+at+4,32);full[at+8]=0x83;memcpy(full+at+12,"pBAV",4);
    }
    for(unsigned character=0;character<2;++character) {
        CHECK(mmx4_coop_audio_load(character,full,sizeof full));
        CHECK(mmx4_coop_audio_load(character,full,sizeof full));
    }
    CHECK(registered==0xF000 && allocated==0x8000);
    psx_mod_write_byte(MMX4_PLAY,6);
    for(unsigned character=0;character<2;++character)for(unsigned group=1;group<=3;group+=2) {
        psx_mod_write_byte(MMX4_PLAYER+2,(uint8_t)character);
        psx_mod_write_word(0x80141F50u+group*4,0x80101000u);
        psx_mod_write_word(0x80141EE8u+group*4,0x80102000u);
        CPUState cpu={0};cpu.gpr[4]=group;cpu.gpr[5]=0;cpu.gpr[6]=MMX4_PLAYER;
        CHECK(sound_filter(&cpu,SOUND) && cpu.gpr[2]==0xABCD && cpu.gpr[6]==MMX4_PLAYER);
        CHECK(psx_mod_read_word(0x80141F50u+group*4)==0x80101000u);
        CHECK(psx_mod_read_word(0x80141EE8u+group*4)==0x80102000u);
        CHECK(sound_filter(&cpu,SOUND));
        unsigned before=opened;psx_mod_write_byte(0x80166D58u+12+character*2+(group==3),0);
        CHECK(sound_filter(&cpu,SOUND) && opened==before+1);
        projected=0;CHECK(!sound_filter(&cpu,SOUND));projected=1;
        local_view=1;CHECK(!sound_filter(&cpu,SOUND));local_view=0;
    }
    CHECK(opened==8 && native_sound==12);
    CPUState voice={0};voice.gpr[4]=20;voice.gpr[5]=3;
    CHECK(key_filter(&voice,KEY_ON) && !pending_bank);
    /* Truncated or wrong typed archives cannot publish a partial bank. */
    mmx4_coop_audio_reset();CHECK(!mmx4_coop_audio_load(0,full,sizeof full-1));
    full[8+11*8]=0;CHECK(!mmx4_coop_audio_load(0,full,sizeof full));
    puts("X4 co-op private audio routing checks passed");return 0;
}
