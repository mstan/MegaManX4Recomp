#include "cpu_state.h"
#include "mod_plugins.h"
#include "mod_runtime.h"
#include "mod_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
#define REQUIRE(x) do {if(!(x))abort();}while(0)
#define PLAY 0x801721C0u
#define PLAYER 0x801418C8u
static uint8_t ram[0x200000],heap[0x100000],scratch[1024];
static PSXModFunctionEntryCallback entry;
static PSXModActivationCallback restore;
static unsigned enabled,writes;
int mod_runtime_snapshot_package_active(const char *id) {
    REQUIRE(!strcmp(id,"mmx4.palette.nightmare-zero"));return enabled;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t p,PSXModFunctionEntryCallback cb) {
    REQUIRE(!entry && !strcmp(id,"mmx4.nightmare-zero") && p==0x800361F8u);entry=cb;return 1;
}
int psx_mod_register_savestate_plugin(const char *id,PSXModActivationCallback cb) {
    REQUIRE(!restore && !strcmp(id,"mmx4.nightmare-zero"));restore=cb;return 1;
}
static uint8_t *at(uint32_t p) {
    if(p>=0x80000000u && p<0x80200000u)return ram+p-0x80000000u;
    if(p>=0x9F000000u && p<0x9F100000u)return heap+p-0x9F000000u;
    REQUIRE(p>=0x1F800000u && p<0x1F800400u);return scratch+p-0x1F800000u;
}
uint8_t psx_mod_read_byte(uint32_t p){return *at(p);}
void psx_mod_write_byte(uint32_t p,uint8_t v){*at(p)=v;++writes;}
uint16_t psx_mod_read_half(uint32_t p){return (uint16_t)(*at(p)|*at(p+1)<<8);}
void psx_mod_write_half(uint32_t p,uint16_t v){psx_mod_write_byte(p,(uint8_t)v);psx_mod_write_byte(p+1,(uint8_t)(v>>8));}
uint32_t psx_mod_read_word(uint32_t p){return psx_mod_read_half(p)|(uint32_t)psx_mod_read_half(p+2)<<16;}
void psx_mod_write_word(uint32_t p,uint32_t v){psx_mod_write_half(p,(uint16_t)v);psx_mod_write_half(p+2,(uint16_t)(v>>16));}
static void seed(uint32_t source,uint32_t palette,unsigned stage) {
    memset(ram,0xA5,sizeof ram);memset(heap,0xA5,sizeof heap);memset(scratch,0,sizeof scratch);
    enabled=1;psx_mod_write_byte(PLAY,6);psx_mod_write_byte(PLAYER+2,1);
    psx_mod_write_half(PLAY+12,(uint16_t)stage);psx_mod_write_byte(PLAY+0x37,0);
    psx_mod_write_word(0x1F800024,source);psx_mod_write_word(0x1F800028,palette);
    psx_mod_write_word(0x800361F8,0x27BDFFE8);psx_mod_write_word(0x80036200,0x80820002);
    psx_mod_write_word(0x800361B8,0x8C420024);psx_mod_write_byte(0x80166BB0,0x42);writes=0;
}
/* Independent public code reference: 22 complete D0 stage/section values and
 * first 80-write addresses, including the absent section0/stage5 condition. */
static const struct {uint16_t stage;uint32_t first;} cases[]={
    {0x0000,0x801D1D92},{0x0100,0x801D0D96},{0x0001,0x801D129A},{0x0101,0x801D24E6},
    {0x0002,0x801D68F6},{0x0102,0x801D9D9A},{0x0003,0x801CE3AA},{0x0103,0x801CE8DA},
    {0x0004,0x801CB11A},{0x0104,0x801D3A22},{0x0105,0x801D0C86},{0x0006,0x801CF966},
    {0x0106,0x801D943E},{0x0007,0x801CF8D6},{0x0107,0x801CAB8E},{0x0008,0x801C73DE},
    {0x0108,0x801D1B3E},{0x000A,0x801CB2E2},{0x000B,0x801C7DF2},{0x010B,0x801D1C1A},
    {0x000C,0x801CBDD2},{0x010C,0x801C7E4E}
};
static int verify(uint32_t source,uint32_t palette,int trails) {
    static const uint16_t body[]={0xBB66,0xBB66,0xE657,0xDDB3,0xD90F,0xC88B,0xEF1C,
        0xF30C,0xDDCB,0xC88B,0xE69A,0xE1D8,0xD575,0xE238};
    for(unsigned i=0;i<14;++i) {
        CHECK(psx_mod_read_half(source+4+i*2)==body[i]);
        CHECK(psx_mod_read_half(source+13*32+4+i*2)==body[i]);
    }
    CHECK(psx_mod_read_half(source)==0xA5A5 && psx_mod_read_half(source+2)==0xA5A5);
    CHECK(psx_mod_read_half(source+13*32)==0xA5A5 && psx_mod_read_half(source+13*32+2)==0xA5A5);
    static const uint16_t saber[]={0xFBDE,0xFBDE,0xE2D4,0xC18C,0xB108,0xA084,0xFBDE,
        0xE2D4,0xC18C,0xB108,0xA084,0xFBDE,0xE2D4,0xC18C,0xA084};
    static const uint16_t shippuuga[]={0xD673,0xC1CE,0xB96B,0xB129,0xA084};
    static const uint16_t hit[]={0xFF71,0xFF72,0xFF73,0xFF74,0xFF95,0xFF96,0xFF97,0xFF98,
        0xFFB9,0xFFBA,0xFFBB,0xFFDC,0xFFDD,0xFFDE,0xFFFF};
    for(unsigned i=0;i<15;++i) {
        CHECK(psx_mod_read_half(source+0x62+i*2)==saber[i]);
        CHECK(psx_mod_read_half(source+0x82+i*2)==saber[i]);
        CHECK(psx_mod_read_half(source+0x462+i*2)==hit[i]);
    }
    for(unsigned i=0;i<5;++i)CHECK(psx_mod_read_half(source+0x12Eu+i*2)==shippuuga[i]);
    for(unsigned i=0;i<0x200;i+=2) {
        unsigned row=i/32,color=i%32/2;
        CHECK(psx_mod_read_half(palette+i)==(trails && row>=13 && color?0xC412:0xA5A5));
    }
    CHECK(psx_mod_read_half(source+0x20)==0xA5A5 && psx_mod_read_half(source+0x480)==0xA5A5);
    CHECK(psx_mod_read_half(palette-2)==0xA5A5 && psx_mod_read_half(palette+0x200)==0xA5A5);
    CHECK(psx_mod_read_byte(PLAYER+0xA7)==0xA5 && psx_mod_read_byte(PLAYER+0xB8)==0xA5);
    CHECK(psx_mod_read_byte(0x80166BB0)==0x43);return 0;
}
int main(int argc,char **argv) {
    CHECK(entry && restore);CPUState cpu={0};cpu.gpr[4]=PLAYER;cpu.gpr[31]=0x80012340;
    if(argc==2) {
        FILE *f=fopen(argv[1],"rb");CHECK(f);uint8_t header[2048];CHECK(fread(header,1,sizeof header,f)==sizeof header);
        CHECK(!memcmp(header,"PS-X EXE",8));
        const uint32_t pc[]={0x800361F8,0x80036200,0x800361B8},word[]={0x27BDFFE8,0x80820002,0x8C420024};
        for(unsigned i=0;i<3;++i) {
            CHECK(!fseek(f,(long)(pc[i]-0x80010000u+2048u),SEEK_SET));uint8_t bytes[4];CHECK(fread(bytes,1,4,f)==4);
            CHECK((bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24)==word[i]);
        }
        CHECK(!fclose(f));
    }
    /* Actual original SoloZero entry: mode5, actor801418C8, source80178000,
     * COL801D1BF0. Startup must recolor before the native row0/row13 copier;
     * gameplay and all stage conditions retain the same guards. */
    for(unsigned mode=5;mode<=6;++mode)
    for(unsigned i=0;i<sizeof cases/sizeof cases[0];++i) {
        uint32_t palette=cases[i].first-0x1A2;seed(0x80178000,palette,cases[i].stage);
        psx_mod_write_byte(PLAY,(uint8_t)mode);writes=0;
        CPUState before=cpu;entry(&cpu,0x800361F8);CHECK(!memcmp(&cpu,&before,sizeof cpu));
        CHECK(!verify(0x80178000,palette,1));CHECK(writes==247);CHECK(!psx_mod_read_byte(PLAY+0x37));
    }
    /* Both P1/P2 resource ownership and independent native Black Zero flags.
     * Canonical campaign is X; the current projected actor is still Zero. */
    for(unsigned black=0;black<2;++black) {
        seed(0x9F040000,0x9F041000,0x010B);psx_mod_write_byte(PLAY+0x43,0);psx_mod_write_byte(PLAY+0x37,(uint8_t)black);writes=0;
        entry(&cpu,0x800361F8);CHECK(!verify(0x9F040000,0x9F041000,1));CHECK(psx_mod_read_byte(PLAY+0x37)==black);
        CHECK(psx_mod_read_half(0x80178004)==0xA5A5);restore();CHECK(psx_mod_read_byte(PLAY+0x37)==black);
    }
    for(unsigned stage=0;stage<2;++stage) {
        seed(0x80178000,0x801D1BF0,stage?0x0005:0x0200);entry(&cpu,0x800361F8);
        CHECK(!verify(0x80178000,0x801D1BF0,0));CHECK(writes==157);
    }
    for(unsigned bad=0;bad<11;++bad) {
        seed(0x80178000,0x801D1BF0,0);
        switch(bad) {
        case 0:enabled=0;break;
        case 1:psx_mod_write_byte(PLAY,4);break;
        case 2:psx_mod_write_byte(PLAYER+2,0);break;
        case 3:psx_mod_write_word(0x800361F8,0);break;
        case 4:psx_mod_write_word(0x80036200,0);break;
        case 5:psx_mod_write_word(0x800361B8,0);break;
        case 6:psx_mod_write_word(0x1F800024,0x80178001);break;
        case 7:psx_mod_write_word(0x1F800028,0);break;
        case 8:psx_mod_write_word(0x1F800024,0x801FFB82);break;
        case 9:psx_mod_write_word(0x1F800028,0x801D1BF2);break;
        case 10:psx_mod_write_word(0x1F800024,0x801D1BF0);break;
        }
        writes=0;entry(&cpu,0x800361F8);CHECK(!writes);restore();CHECK(!writes);
    }
    /* State-loaded notification must not mutate an unfinished mode5 COL.
     * Other front-end/loading/exit modes are never valid entry contexts. */
    seed(0x80178000,0x801D1BF0,0);psx_mod_write_byte(PLAY,5);writes=0;
    restore();CHECK(!writes);
    for(unsigned mode=0;mode<12;++mode) {
        if(mode==5 || mode==6)continue;
        seed(0x80178000,0x801D1BF0,0);psx_mod_write_byte(PLAY,(uint8_t)mode);writes=0;
        entry(&cpu,0x800361F8);CHECK(!writes);restore();CHECK(!writes);
    }
    puts("Nightmare Zero: native mode5 initialization, all22conditions, colors, transparency, native guards, projected ownership, Black Zero priority and gameplay-only restore PASS");return 0;
}
