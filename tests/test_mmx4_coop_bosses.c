/* State/ownership checks complement the original-disc two-encounter run. */
#include "mmx4_coop_internal.h"
int mmx4_coop_award_story_instruction(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;return 0;
}
#include "mod_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t ram[0x200000];
static int enabled,projected,local,ready=1,capsule_text;
static int coop_active=1,complete_active;
static PSXModFunctionFilterCallback transition;
static PSXModFunctionFilterCallback solo_transition,memorial_return;
static PSXModActivationCallback story_activate;
static PSXModSnapshotProvider story_provider;
static struct {const char *id;uint32_t address,word;PSXModFunctionEntryCallback fn;} hooks[64];
static unsigned count;
uint32_t psx_mod_alloc_guest_memory(uint32_t bytes,uint32_t align) {
    CHECK(bytes==0xA000u && align==16);return 0x9f080000u;
}
int psx_mod_read_disc_file(const char *name,void *out,uint32_t capacity,uint32_t *size) {
    CHECK(!strcmp(name,"ARC/PL00_U.ARC"));*size=4096;
    if(!out)return 1;
    CHECK(capacity>=4096);memset(out,0,4096);
    uint8_t *p=out;p[0]=16;p[5]=16;p[8+15*8+4]=16;
    memcpy(p+2048,"Colonel fixture",16);return 1;
}
int mod_runtime_snapshot_package_active(const char *id) {
    return !strcmp(id,"mmx4.coop")?coop_active:!strcmp(id,"mmx4.complete-story")?complete_active:0;
}
int psx_mod_register_activation_plugin(const char *id,PSXModActivationCallback fn) {
    CHECK(!strcmp(id,"mmx4.complete-story"));story_activate=fn;return 1;
}
int psx_mod_register_snapshot_provider(const char *id,const PSXModSnapshotProvider *p) {
    CHECK(!strcmp(id,"mmx4.complete-story"));story_provider=*p;return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,PSXModFunctionEntryCallback fn) {
    CHECK(count<64);hooks[count].id=id;hooks[count].word=0;
    hooks[count].address=a;hooks[count++].fn=fn;return 1;
}
uint8_t psx_mod_read_byte(uint32_t a){return ram[a&0x1fffff];}
void psx_mod_write_byte(uint32_t a,uint8_t v){ram[a&0x1fffff]=v;}
void psx_mod_write_word(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)psx_mod_write_byte(a+i,(uint8_t)(v>>(8*i)));}
int psx_mod_local_view_scope(void){return local;}
int mmx4_coop_projected(void){return projected;}
int mmx4_coop_ready(void){return ready;}
uint32_t mmx4_coop_counterpart_story_text(void){return 0x9f046000u;}
int mmx4_coop_lifecycle_x_capsule_text(void){return capsule_text;}
int psx_mod_current_option_value(const char *key,char *value,uint32_t cap){
    CHECK(!strcmp(key,"both_story_bosses") && cap>=6);strcpy(value,enabled?"true":"false");return 1;
}
void psx_mod_counter_add(const char *name,uint32_t v){(void)name;CHECK(v==1);}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t a,PSXModFunctionFilterCallback fn){
    if(!strcmp(id,"mmx4.coop")) {CHECK(a==0x800200D4u);transition=fn;}
    else if(a==0x800200D4u)solo_transition=fn;
    else {CHECK(a==0x8002044Cu);memorial_return=fn;}
    return 1;
}
int psx_mod_register_instruction_plugin(const char *id,uint32_t a,uint32_t word,PSXModFunctionEntryCallback fn){
    CHECK(count<64);hooks[count].id=id;hooks[count].word=word;
    hooks[count].address=a;hooks[count++].fn=fn;return 1;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result){cpu->gpr[2]=result;return 1;}
static void hook(CPUState *cpu,uint32_t a){
    unsigned found=0;
    for(unsigned i=0;i<count;++i)if(hooks[i].address==a){
        found=1;if(mod_runtime_snapshot_package_active(hooks[i].id))hooks[i].fn(cpu,a);
    }
    CHECK(found);
}
static void capture(uint8_t bytes[7]){
    Mmx4SnapshotIO s={bytes,NULL,7,0,MMX4_SNAPSHOT_WRITE,1};
    CHECK(mmx4_coop_bosses_snapshot(&s) && s.pos==7);
}
int main(int argc,char **argv){
    if(argc==2) {
        FILE *exe=fopen(argv[1],"rb");CHECK(exe);
        for(unsigned i=0;i<count;++i)if(hooks[i].word) {
            uint8_t b[4];CHECK(!fseek(exe,(long)(hooks[i].address-0x80010000u+0x800u),SEEK_SET));
            CHECK(fread(b,1,4,exe)==4);
            uint32_t word=b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;
            CHECK(word==hooks[i].word);
        }
        fclose(exe);
    }
    CPUState cpu={0};uint8_t first[7],second[7];
    psx_mod_write_byte(MMX4_PLAY+0xC,11);
    psx_mod_write_byte(MMX4_PLAY+0xF,1);
    mmx4_coop_bosses_activate();
    hook(&cpu,0x8008D324u);CHECK(!transition(&cpu,0x800200D4u));
    for(unsigned campaign=0;campaign<2;++campaign){
        enabled=1;mmx4_coop_bosses_activate();
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAY+0xF,1);
        capture(first);
        CHECK(!transition(&cpu,0x800200D4u)); /* Death/menu exit is not victory. */
        hook(&cpu,campaign?0x800810A8u:0x8008D324u);
        projected=1;CHECK(!transition(&cpu,0x800200D4u));projected=0;
        local=1;CHECK(!transition(&cpu,0x800200D4u));local=0;
        CHECK(transition(&cpu,0x800200D4u));
        CHECK(psx_mod_read_byte(MMX4_PLAY)==4);
        mmx4_coop_bosses_new_stage();
        psx_mod_write_byte(MMX4_PLAY+0x1D,0); /* Native loader clears it. */
        psx_mod_write_byte(MMX4_PLAY,5);
        mmx4_coop_bosses_stage_initialization();
        CHECK(psx_mod_read_byte(MMX4_PLAY)==5 && psx_mod_read_byte(MMX4_PLAY+0x1D)==1);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        capture(second);
        for(unsigned i=0;i<4;++i){
            unsigned reg=hooks[i].address==0x8001302Cu?3u:2u;
            cpu.gpr[reg]=campaign;hooks[i].fn(&cpu,hooks[i].address);
            CHECK(cpu.gpr[reg]==(campaign^1u));
            CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        }
        hook(&cpu,0x800224D4u);CHECK(cpu.gpr[4]==(campaign^1u));
        hook(&cpu,0x80022554u);CHECK(cpu.gpr[4]==0x9f046000u);
        hook(&cpu,0x80022684u);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x800227B4u);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x8002284Cu);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x80022880u);CHECK(cpu.gpr[3]==(campaign^1u));
        hook(&cpu,0x800228B4u);CHECK(cpu.gpr[3]==(campaign^1u));
        hook(&cpu,0x800228F4u);CHECK(cpu.gpr[2]==(campaign^1u));
        hook(&cpu,0x80022928u);CHECK(cpu.gpr[4]==(campaign^1u));
        hook(&cpu,campaign?0x8008D324u:0x800810A8u);
        CHECK(cpu.gpr[2]==(campaign?0x40u:1u));
        CHECK(!transition(&cpu,0x800200D4u)); /* No third battle. */
        Mmx4SnapshotIO s={NULL,first,7,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_bosses_snapshot(&s));
        cpu.gpr[2]=campaign;hook(&cpu,0x80013148u);CHECK(cpu.gpr[2]==campaign);
        s=(Mmx4SnapshotIO){NULL,second,7,0,MMX4_SNAPSHOT_READ,1};
        CHECK(mmx4_coop_bosses_snapshot(&s));
        mmx4_coop_bosses_new_stage();
        cpu.gpr[2]=campaign;hook(&cpu,0x80013148u);CHECK(cpu.gpr[2]==campaign);
        enabled=0;mmx4_coop_bosses_activate();
        s=(Mmx4SnapshotIO){NULL,second,7,0,MMX4_SNAPSHOT_VALIDATE,1};
        CHECK(!mmx4_coop_bosses_snapshot(&s));
    }
    /* Capsules use X selectors with the elective story mod disabled. Only
     * text sites change: no boss-bank/campaign/progression selector does. */
    enabled=0;mmx4_coop_bosses_activate();capsule_text=1;
    psx_mod_write_byte(MMX4_PLAY+0xC,1);psx_mod_write_byte(MMX4_PLAY+0x43,1);
    for(unsigned i=4;i<13;++i) {
        unsigned reg=hooks[i].address==0x80022554u || hooks[i].address==0x800224D4u ||
            hooks[i].address==0x80022928u?4u:
            hooks[i].address==0x80022880u || hooks[i].address==0x800228B4u?3u:2u;
        cpu.gpr[reg]=0x5A;hooks[i].fn(&cpu,hooks[i].address);
        CHECK(cpu.gpr[reg]==(hooks[i].address==0x80022554u?0x9f046000u:0u));
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==1);
        local=1;cpu.gpr[reg]=0x5A;hooks[i].fn(&cpu,hooks[i].address);
        CHECK(cpu.gpr[reg]==0x5A);local=0;
    }
    cpu.gpr[3]=1;hook(&cpu,0x8001302Cu);CHECK(cpu.gpr[3]==1);
    capsule_text=0;cpu.gpr[4]=0x5A;hook(&cpu,0x80022554u);CHECK(cpu.gpr[4]==0x5A);
    /* New package: solo and co-op use the same two native encounter rounds,
     * with no doubled callback or campaign/save identity change. */
    for(unsigned coop=0;coop<2;++coop)for(unsigned campaign=0;campaign<2;++campaign) {
        coop_active=coop;complete_active=1;enabled=0;ready=coop;
        if(coop)mmx4_coop_bosses_activate();
        story_activate();
        memset(ram,0,sizeof ram);
        psx_mod_write_byte(MMX4_PLAY+0x43,(uint8_t)campaign);
        psx_mod_write_byte(MMX4_PLAY+0x59,15);psx_mod_write_byte(MMX4_PLAY+0x5F,3);
        cpu.gpr[2]=campaign;hook(&cpu,0x8002E54Cu);CHECK(cpu.gpr[2]==0);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        psx_mod_write_byte(MMX4_PLAY+0xC,9);psx_mod_write_byte(MMX4_PLAY+0x5F,4);
        cpu.gpr[2]=9;hook(&cpu,0x800201C4u);CHECK(cpu.gpr[2]==10);
        psx_mod_write_word(MMX4_PLAY,10);cpu.gpr[3]=9;
        hook(&cpu,0x800203C4u);CHECK(cpu.gpr[3]==11);
        cpu.gpr[2]=campaign;hook(&cpu,0x800203D4u);CHECK(cpu.gpr[2]==1);
        cpu.gpr[4]=8;hook(&cpu,0x80018000u);CHECK(cpu.gpr[4]==7);
        /* Memorial uses Colonel's authored encounter dialogue/portraits.
         * Zero's same numeric entry is an unrelated Sigma conversation. */
        if(campaign) {
            cpu.gpr[4]=1;hook(&cpu,0x800224D4u);CHECK(!cpu.gpr[4]);
            cpu.gpr[4]=0x1234;hook(&cpu,0x80022554u);
            CHECK(cpu.gpr[4]==(coop?0x9f046000u:0x9f080000u));
            if(!coop)CHECK(psx_mod_read_byte(cpu.gpr[4])=='C');
            cpu.gpr[2]=1;hook(&cpu,0x80022684u);CHECK(!cpu.gpr[2]);
            CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==1);
            local=1;cpu.gpr[4]=0x1234;hook(&cpu,0x80022554u);CHECK(cpu.gpr[4]==0x1234);local=0;
        }
        CHECK(memorial_return(&cpu,0x8002044Cu));
        CHECK(psx_mod_read_byte(MMX4_PLAY)==9 && !psx_mod_read_byte(MMX4_PLAY+0xD));
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x5F)==4 && psx_mod_read_byte(MMX4_PLAY+0x59)==15);
        psx_mod_write_byte(MMX4_PLAY+0xC,11);psx_mod_write_byte(MMX4_PLAY+0xF,1);
        hook(&cpu,campaign?0x800810A8u:0x8008D324u);
        CHECK((coop?transition:solo_transition)(&cpu,0x800200D4u));
        psx_mod_write_byte(MMX4_PLAY+0x1D,0);
        if(coop)mmx4_coop_bosses_stage_initialization();else hook(&cpu,0x8001FC20u);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x1D)==1);
        cpu.gpr[2]=campaign;hook(&cpu,0x80080714u);CHECK(cpu.gpr[2]==(campaign^1));
        uint8_t saved[7];CHECK(story_provider.bytes()==7 && story_provider.write(saved,7));
        CHECK(story_provider.validate(saved,7) && !story_provider.validate(saved,6));
        saved[4]=2;CHECK(!story_provider.validate(saved,7));saved[4]=1;
        story_provider.read(saved,7);
        psx_mod_write_byte(MMX4_PLAY+0xF,1);
        hook(&cpu,campaign?0x8008D324u:0x800810A8u);
        CHECK(cpu.gpr[2]==(campaign?0x40u:1u));
        CHECK(!(coop?transition:solo_transition)(&cpu,0x800200D4u));
        cpu.gpr[2]=campaign;hook(&cpu,0x800203D4u);CHECK(cpu.gpr[2]==1);
        psx_mod_write_byte(MMX4_PLAY+0xC,12);cpu.gpr[2]=campaign;
        hook(&cpu,0x800203FCu);CHECK(cpu.gpr[2]==1);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x43)==campaign);
        CHECK(psx_mod_read_byte(MMX4_PLAY+0x59)==15);
        CHECK(!memorial_return(&cpu,0x8002044Cu));
        /* Presentation replays and other stages do not admit new films. */
        local=1;cpu.gpr[2]=0;hook(&cpu,0x800203FCu);CHECK(!cpu.gpr[2]);local=0;
        psx_mod_write_byte(MMX4_PLAY+0xD,1);cpu.gpr[2]=0;
        hook(&cpu,0x800203FCu);CHECK(!cpu.gpr[2]);
    }
    /* Solo cross-story bosses receive the same narrow authored damage-table
     * admission as co-op. Keep the real attack and unrelated tables intact. */
    coop_active=0;complete_active=1;
    psx_mod_write_word(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+0xC,11);
    psx_mod_write_byte(MMX4_PLAY+0xD,0);
    uint32_t actor=0x8013BED0u,table=0x80107E84u;
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,66);psx_mod_write_byte(actor+2,1);
    psx_mod_write_byte(MMX4_PLAYER+2,0);psx_mod_write_byte(table+64,9);
    cpu.gpr[4]=actor;cpu.gpr[6]=0;cpu.gpr[7]=table;cpu.gpr[2]=255;
    hook(&cpu,0x8002DE84u);CHECK(cpu.gpr[2]==9 && cpu.gpr[7]==table+64 && !cpu.gpr[6]);
    table=0x80108204u;psx_mod_write_byte(actor+1,73);
    psx_mod_write_byte(MMX4_PLAYER+2,1);psx_mod_write_byte(table+26*2+1,2);
    cpu.gpr[6]=24;cpu.gpr[7]=table;cpu.gpr[2]=255;
    hook(&cpu,0x8002DE84u);CHECK(!cpu.gpr[2] && cpu.gpr[7]==table+4 && cpu.gpr[6]==24);
    cpu.gpr[7]=0x80100000u;cpu.gpr[2]=255;
    hook(&cpu,0x8002DE84u);CHECK(cpu.gpr[2]==255 && cpu.gpr[7]==0x80100000u);
    complete_active=0;coop_active=0;cpu.gpr[2]=1;
    hook(&cpu,0x8002E54Cu);CHECK(cpu.gpr[2]==1);
    puts("Story/capsule text selectors preserve campaign ownership.");return 0;
}
