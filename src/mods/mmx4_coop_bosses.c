/* Optional story encounters. The campaign byte always belongs to P1;
 * stage bank selectors, encounter admission and dialogue use the other
 * story during the second round without changing that campaign byte. */
#include "mmx4_coop_internal.h"
#include "mod_runtime.h"
#include "mod_snapshot.h"
#include "mmx4_story_damage.h"
#include "mmx4_coop_assets.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define ID "mmx4.coop"
#define STORY_ID "mmx4.complete-story"
#define PLAY MMX4_PLAY
static unsigned both_stories,second_round,defeated,reloading;
static uint32_t memorial_text;
static uint8_t *memorial_text_data;
static size_t memorial_text_size;

static int complete_story(void) {return mod_runtime_snapshot_package_active(STORY_ID);}
static int coop_active(void) {return mod_runtime_snapshot_package_active(ID);}
static int both_encounters(void) {return both_stories || complete_story();}
static int projected(void) {return coop_active() && mmx4_coop_projected();}
static int encounter_ready(void) {return !coop_active() || mmx4_coop_ready();}
static uint32_t memorial_story_text(void) {
    if(coop_active())return mmx4_coop_counterpart_story_text();
    if(!memorial_text)return 0;
    if(!memorial_text_data) {
        uint32_t bytes=0;
        if(!psx_mod_read_disc_file("ARC/PL00_U.ARC",NULL,0,&bytes) || !bytes || bytes>0x200000u)return 0;
        uint8_t *file=malloc(bytes);
        Mmx4Asset text;
        int valid=file && psx_mod_read_disc_file("ARC/PL00_U.ARC",file,bytes,&bytes) &&
            mmx4_coop_arc_asset(file,bytes,15,&text) && text.size>=2 && text.size<=0xA000u;
        if(valid) {
            memorial_text_data=malloc(text.size);
            if(memorial_text_data) {memcpy(memorial_text_data,text.data,text.size);memorial_text_size=text.size;}
        }
        free(file);
        if(!memorial_text_data)return 0;
    }
    /* A loaded state may precede the first dialogue. Refill its immutable
     * text bank here; no host cache flag may assume restored guest bytes. */
    for(size_t i=0;i<memorial_text_size;++i)psx_mod_write_byte(memorial_text+(uint32_t)i,memorial_text_data[i]);
    return memorial_text;
}
static int story_room(void) {
    return psx_mod_read_byte(PLAY+0x0C)==11 && !psx_mod_read_byte(PLAY+0x0D);
}
static unsigned story_character(void) {
    return psx_mod_read_byte(PLAY+0x43) ^ second_round;
}
void mmx4_coop_bosses_activate(void) {
    char value[8];
    both_stories=psx_mod_current_option_value("both_story_bosses",value,sizeof value) &&
        !strcmp(value,"true");
    second_round=defeated=reloading=0;
    if(both_stories)psx_mod_counter_add("mmx4.coop.both-story-bosses-enabled",1);
}
int mmx4_coop_bosses_snapshot_ready(void) {return 1;}
int mmx4_coop_bosses_snapshot(Mmx4SnapshotIO *s) {
    mmx4_snapshot_fixed(s,both_stories);
    MMX4_SNAPSHOT_BOOL(s,second_round);MMX4_SNAPSHOT_BOOL(s,defeated);
    MMX4_SNAPSHOT_BOOL(s,reloading);
    return s->ok;
}
uint32_t mmx4_coop_bosses_digest(uint32_t seed) {
    seed=(seed^both_stories)*16777619u;
    seed=(seed^second_round)*16777619u;
    seed=(seed^defeated)*16777619u;
    return (seed^reloading)*16777619u;
}
void mmx4_coop_bosses_new_stage(void) {
    if(!projected() && !psx_mod_local_view_scope() && !(reloading && story_room()))
        second_round=defeated=reloading=0;
}
void mmx4_coop_bosses_stage_initialization(void) {
    if(!reloading || !story_room() || projected() || psx_mod_local_view_scope())return;
    psx_mod_write_byte(PLAY+0x1D,1);
    psx_mod_write_byte(PLAY+0x1E,0);
    reloading=0;
}
static void select_story(CPUState *cpu,uint32_t address) {
    if(mmx4_coop_award_story_instruction(cpu,address))return;
    if(psx_mod_local_view_scope())return;
    unsigned capsule=coop_active() && address>=0x800224D4u && address<=0x80022928u &&
        mmx4_coop_lifecycle_x_capsule_text();
    unsigned memorial=complete_story() && psx_mod_read_byte(PLAY+0x0C)==9 &&
        psx_mod_read_byte(PLAY+0x43)==1 && address>=0x800224D4u && address<=0x80022928u;
    if(!capsule && !memorial && (!both_encounters() || !second_round || !story_room()))return;
    /* The shared textbox keeps the campaign's palette, but X's capsule
     * supplies X's text/portraits/voice selectors even in Zero's story.
     * This correctness path is independent of the optional second boss. */
    if(address==0x80022554u) {
        uint32_t text=memorial?memorial_story_text():coop_active()?mmx4_coop_counterpart_story_text():0;
        if(text)cpu->gpr[4]=text;
    }else {
        unsigned reg=2;
        if(address==0x800224D4u || address==0x80022928u)reg=4;
        if(address==0x8001302Cu || address==0x80022880u || address==0x800228B4u)reg=3;
        cpu->gpr[reg]=(capsule || memorial)?0u:story_character();
    }
}
static void story_defeated(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!both_encounters() || !story_room() || !encounter_ready() || psx_mod_local_view_scope())return;
    defeated=1;
    psx_mod_counter_add("mmx4.coop.story-boss-defeated",1);
    /* Finish with P1's original post-battle movie/story route even when
     * the second fight was the other character's encounter. */
    if(second_round)cpu->gpr[2]=psx_mod_read_byte(PLAY+0x43)?0x40u:1u;
}
static int story_transition(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!both_encounters() || second_round || !defeated || !story_room() ||
       !encounter_ready() || projected() || psx_mod_local_view_scope() ||
       !(psx_mod_read_byte(PLAY+0x0F)&0x7Fu))return 0;
    second_round=1;defeated=0;reloading=1;
    /* Schedule the ordinary loader; CD loading can suspend and must never
     * run recursively inside a synchronous host callback. The subsequent
     * stage initializer selects the verified boss-door checkpoint. */
    psx_mod_write_byte(PLAY+0x0F,0);
    psx_mod_write_word(PLAY,4);
    fprintf(stderr,"mmx4.coop: story-sequel campaign=%u mode=%u checkpoint=%u lives=%u\n",
        psx_mod_read_byte(PLAY+0x43),psx_mod_read_byte(PLAY),
        psx_mod_read_byte(PLAY+0x1D),psx_mod_read_byte(PLAY+0x44));
    psx_mod_counter_add("mmx4.coop.second-story-encounter",1);
    return mmx4_coop_finish(cpu,0);
}

/* One optional story package serves solo and co-op. The co-op callbacks above
 * already own their encounter state when both packages are active; the solo
 * wrappers must not count a defeat or reset a reload for a second time. */
static void solo_activate(void) {
    free(memorial_text_data);memorial_text_data=NULL;memorial_text_size=0;
    memorial_text=coop_active()?0:psx_mod_alloc_guest_memory(0xA000u,16);
    if(!coop_active())both_stories=second_round=defeated=reloading=0;
}
static void solo_stage(CPUState *cpu,uint32_t address) {
    (void)cpu;
    if(coop_active())return;
    if(address==0x8001FC20u)mmx4_coop_bosses_stage_initialization();
    else mmx4_coop_bosses_new_stage();
}
static void solo_selector(CPUState *cpu,uint32_t address) {
    if(!coop_active())select_story(cpu,address);
}
static void solo_defeated(CPUState *cpu,uint32_t address) {
    if(!coop_active())story_defeated(cpu,address);
}
static int solo_transition(CPUState *cpu,uint32_t address) {
    return coop_active()?0:story_transition(cpu,address);
}
static void solo_hit(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!coop_active())mmx4_story_counterpart_hit(cpu);
}
static void complete_route(CPUState *cpu,uint32_t address) {
    if(psx_mod_local_view_scope() || projected())return;
    unsigned stage=psx_mod_read_byte(PLAY+0x0C);
    if(address==0x8002E54Cu) {
        /* Four clears: choose the native playable Memorial Hall route. Only
         * the route register changes; Zero's campaign, body and save stay Zero. */
        cpu->gpr[2]=0;
    }else if(address==0x8002EE84u) {
        /* The frontend has a second Zero-specific bypass after its fade.
         * Enter the loader here instead of returning directly to the board. */
        if(stage==9)cpu->gpr[2]=0;
    }else if(address==0x800201C4u) {
        /* The completed Memorial Hall duel normally goes straight to mode9.
         * Use mode10's native fade/movie gateway before returning there. */
        if(stage==9 && cpu->gpr[2]==9)cpu->gpr[2]=10;
    }else if(address==0x800203C4u) {
        if(stage==9)cpu->gpr[3]=11;
    }else if(address==0x800203D4u) {
        if(stage==9 || (story_room() && second_round && defeated))cpu->gpr[2]=1;
    }else if(address==0x800203FCu) {
        /* The native pre-Sigma gateway already waits for this movie in Zero's
         * route. Admit X too, without borrowing Zero's campaign byte. */
        if(stage==12 && !psx_mod_read_byte(PLAY+0x0D))cpu->gpr[2]=1;
    }
}
static void memorial_movie(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!psx_mod_local_view_scope() && !projected() &&
       psx_mod_read_byte(PLAY)==10 && psx_mod_read_byte(PLAY+0x0C)==9 && cpu->gpr[4]==8)
        cpu->gpr[4]=7; /* Z2.STR: Colonel and Zero, interrupted by Iris. */
    if(psx_mod_local_view_scope() || projected())return;
    switch(cpu->gpr[4]) {
    case 7:psx_mod_counter_add("mmx4.story.colonel-movie",1);break;
    case 8:psx_mod_counter_add("mmx4.story.iris-movie",1);break;
    case 9:psx_mod_counter_add("mmx4.story.sigma-movie",1);break;
    }
}
static int memorial_return(CPUState *cpu,uint32_t address) {
    (void)address;
    if(psx_mod_local_view_scope() || projected() || psx_mod_read_byte(PLAY)!=10 ||
       psx_mod_read_byte(PLAY+0x0C)!=9)return 0;
    /* Unlike a fortress interstitial, this returns to the native stage board,
     * not section1 of Memorial Hall. The original movie has already returned. */
    psx_mod_write_word(PLAY,9);
    cpu->gpr[2]=9;
    return 1;
}
static uint32_t story_bytes(void) {return 7;}
static int story_write(uint8_t *out,uint32_t len) {
    if(!out || len!=7)return 0;
    Mmx4SnapshotIO s={out,NULL,len,0,MMX4_SNAPSHOT_WRITE,1};
    return mmx4_coop_bosses_snapshot(&s) && s.pos==len;
}
static int story_validate(const uint8_t *in,uint32_t len) {
    if(!in || len!=7)return 0;
    Mmx4SnapshotIO s={NULL,in,len,0,MMX4_SNAPSHOT_VALIDATE,1};
    return mmx4_coop_bosses_snapshot(&s) && s.pos==len;
}
static void story_read(const uint8_t *in,uint32_t len) {
    Mmx4SnapshotIO s={NULL,in,len,0,MMX4_SNAPSHOT_READ,1};
    mmx4_coop_bosses_snapshot(&s);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_bosses) {
    psx_mod_register_function_filter_plugin(ID,0x800200D4u,story_transition);
    /* The loader chooses stage11 sprites through V1 at 1302C. The later
     * V0 selector at 130D4 only fills stage13's table entry. */
    psx_mod_register_instruction_plugin(ID,0x8001302Cu,0x00230821u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80013148u,0x14400005u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80080714u,0x14400005u,select_story);
    psx_mod_register_instruction_plugin(ID,0x8008BA54u,0x10400005u,select_story);
    /* The script text and portrait/voice indices also belong to the current
     * encounter, independent of the campaign or the player speaking. */
    psx_mod_register_instruction_plugin(ID,0x800224D4u,0x00041880u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022554u,0x00641821u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022684u,0x14400009u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800227B4u,0x00021080u,select_story);
    psx_mod_register_instruction_plugin(ID,0x8002284Cu,0x00021080u,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022880u,0x00431021u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800228B4u,0x00431021u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800228F4u,0x2442001Au,select_story);
    psx_mod_register_instruction_plugin(ID,0x80022928u,0x0C00A4E3u,select_story);
    psx_mod_register_instruction_plugin(ID,0x800810A8u,0xA02221CFu,story_defeated);
    psx_mod_register_instruction_plugin(ID,0x8008D324u,0xA02221CFu,story_defeated);

    static const PSXModSnapshotProvider provider={1,story_bytes,story_write,
        story_validate,story_read,mmx4_coop_bosses_snapshot_ready};
    psx_mod_register_snapshot_provider(STORY_ID,&provider);
    psx_mod_register_activation_plugin(STORY_ID,solo_activate);
    const uint32_t stages[]={0x8001FBB8u,0x8001FBE0u,0x8001FC20u,0x8002E420u};
    for(unsigned i=0;i<sizeof stages/sizeof stages[0];++i)
        psx_mod_register_function_entry_plugin(STORY_ID,stages[i],solo_stage);
    psx_mod_register_function_filter_plugin(STORY_ID,0x800200D4u,solo_transition);
    psx_mod_register_function_entry_plugin(STORY_ID,0x80018000u,memorial_movie);
    psx_mod_register_function_filter_plugin(STORY_ID,0x8002044Cu,memorial_return);
    psx_mod_register_instruction_plugin(STORY_ID,0x8002DE84u,0xA0820063u,solo_hit);
    const struct {uint32_t address,word;} selectors[]={
        {0x8001302Cu,0x00230821u},{0x80013148u,0x14400005u},
        {0x80080714u,0x14400005u},{0x8008BA54u,0x10400005u},
        {0x800224D4u,0x00041880u},{0x80022554u,0x00641821u},
        {0x80022684u,0x14400009u},{0x800227B4u,0x00021080u},
        {0x8002284Cu,0x00021080u},{0x80022880u,0x00431021u},
        {0x800228B4u,0x00431021u},{0x800228F4u,0x2442001Au},
        {0x80022928u,0x0C00A4E3u}};
    for(unsigned i=0;i<sizeof selectors/sizeof selectors[0];++i)
        psx_mod_register_instruction_plugin(STORY_ID,selectors[i].address,selectors[i].word,solo_selector);
    psx_mod_register_instruction_plugin(STORY_ID,0x800810A8u,0xA02221CFu,solo_defeated);
    psx_mod_register_instruction_plugin(STORY_ID,0x8008D324u,0xA02221CFu,solo_defeated);
    psx_mod_register_instruction_plugin(STORY_ID,0x8002E54Cu,0x10400002u,complete_route);
    psx_mod_register_instruction_plugin(STORY_ID,0x8002EE84u,0x10400009u,complete_route);
    psx_mod_register_instruction_plugin(STORY_ID,0x800201C4u,0xA2020000u,complete_route);
    psx_mod_register_instruction_plugin(STORY_ID,0x800203C4u,0x14620009u,complete_route);
    psx_mod_register_instruction_plugin(STORY_ID,0x800203D4u,0x10400005u,complete_route);
    psx_mod_register_instruction_plugin(STORY_ID,0x800203FCu,0x10400003u,complete_route);
}
