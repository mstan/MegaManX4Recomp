#include "mmx4_coop_internal.h"
static PSXModFunctionEntryCallback mushroom_room_unlocked;
static unsigned train_delegate;
int mmx4_coop_lifecycle_train(CPUState *cpu,uint32_t address) {
    if(!train_delegate)return 0;
    /* The production lifecycle binds one train update to its progressing
     * hero and restores an uncommitted P2 scene's canonical camera. Model
     * that composition too; a no-op delegate cannot catch discarded scroll. */
    unsigned owner=train_delegate-1u;
    uint8_t canonical[0xFC];
    int own=owner && mmx4_coop_split_scene_camera_begin(owner,canonical);
    if(owner)mmx4_coop_enter_second();
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(owner)mmx4_coop_leave_second();
    if(own)mmx4_coop_split_scene_camera_end(owner,canonical,0);
    return mmx4_coop_finish(cpu,result);
}
int mmx4_coop_lifecycle_lift_start(CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value) do {if(!(value)) {fprintf(stderr,"line %d: %s\n",__LINE__,#value);exit(1);}} while(0)
#define CAMERA 0x801419B0u
#define TABLE 0x800F5000u
#define ACTOR 0x80170000u

static uint8_t ram[0x200000],second[0xE4],backup[0xE4];
static unsigned split=1,projected,local_scope,ready=1,capacity=16,allocations,scans;
static unsigned native_directional_scans,scan_direction;
static uint32_t native_table=TABLE;
static int script_owner=-1;
static unsigned paired_door;
static unsigned assessment_override;
int mmx4_coop_lifecycle_assessment_bounds(CPUState *cpu,uint32_t address) {
    CHECK(address==0x8002B160u);
    return assessment_override?mmx4_coop_finish(cpu,0):0;
}
static unsigned palette_rebuilds,palette_uploads;
int mmx4_coop_lifecycle_paired_door(void) {return (int)paired_door;}
static unsigned nova_active;
static unsigned rider_armor_calls;
int mmx4_coop_render_rider_armor(CPUState *cpu,uint32_t address) {
    CHECK(address==0x800257BCu && cpu->gpr[4]==ACTOR);
    ++rider_armor_calls;
    return mmx4_coop_finish(cpu,123);
}
int mmx4_coop_lifecycle_nova_active(void) {return (int)nova_active;}
static unsigned room_entries,room_owner;
void mmx4_coop_lifecycle_room_entry(unsigned owner) {++room_entries;room_owner=owner;}
typedef struct {uint32_t address;PSXModFunctionFilterCallback callback;} Filter;
static Filter filters[40];static unsigned filter_count;
static PSXModFunctionEntryCallback spawned,owl_room_unlocked,frost_shaft_unlocked,laser_origin;
static PSXModFunctionEntryCallback dragon_origin;
uint8_t psx_mod_read_byte(uint32_t address) {return ram[address&0x1FFFFFu];}
void psx_mod_write_byte(uint32_t address,uint8_t value) {ram[address&0x1FFFFFu]=value;}
uint16_t psx_mod_read_half(uint32_t address) {
    return psx_mod_read_byte(address)|(uint16_t)psx_mod_read_byte(address+1)<<8;
}
void psx_mod_write_half(uint32_t address,uint16_t value) {
    psx_mod_write_byte(address,(uint8_t)value);psx_mod_write_byte(address+1,(uint8_t)(value>>8));
}
uint32_t psx_mod_read_word(uint32_t address) {
    return psx_mod_read_half(address)|(uint32_t)psx_mod_read_half(address+2)<<16;
}
void psx_mod_write_word(uint32_t address,uint32_t value) {
    psx_mod_write_half(address,(uint16_t)value);psx_mod_write_half(address+2,(uint16_t)(value>>16));
}
uint32_t psx_mod_memory_alloc(uint32_t size,uint32_t alignment) {
    CHECK(size<4096 && alignment==4);return 0x801F0000u;
}
int psx_mod_register_function_filter_plugin(const char *id,uint32_t address,PSXModFunctionFilterCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop") && filter_count<40);
    for(unsigned i=0;i<filter_count;++i)CHECK(((filters[i].address^address)&0x1FFFFFFFu)!=0);
    filters[filter_count++]=(Filter){address,callback};return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t address,PSXModFunctionEntryCallback callback) {
    (void)id;(void)address;(void)callback;return 1;
}
int psx_mod_register_instruction_plugin(const char *id,uint32_t address,uint32_t word,PSXModFunctionEntryCallback callback) {
    CHECK(!strcmp(id,"mmx4.coop"));
    if(address==0x80029284u) {CHECK(word==0xA2620000u);spawned=callback;}
    else if(address==0x8005D7C4u || address==0x8005D80Cu) {CHECK(word==0xA42219D6u);mushroom_room_unlocked=callback;}
    else if(address==0x8009D104u || address==0x800A3B08u || address==0x800A3C00u) {
        CHECK(word==(address==0x800A3C00u?0x8622000Au:0x8623000Au));laser_origin=callback;
    }
    else if(address==0x80099634u || address==0x80099650u) {
        CHECK(word==(address==0x80099634u?0x8E240008u:0u));dragon_origin=callback;
    }
    else if(address==0x800B8D64u) {CHECK(word==0xA42219D4u);frost_shaft_unlocked=callback;}
    else {CHECK(address==0x80089204u && word==0xA42219D4u);owl_room_unlocked=callback;}
    return 1;
}
void psx_mod_counter_add(const char *name,uint32_t amount) {(void)name;(void)amount;}
int32_t psx_mod_widescreen_x_margin(void) {return 0;}
int psx_mod_local_view_scope(void) {return (int)local_scope;}
int mmx4_coop_ready(void) {return (int)ready;}
int mmx4_coop_projected(void) {return (int)projected;}
int mmx4_coop_split_views(void) {return (int)split;}
uint8_t *mmx4_coop_second_body(void) {return second;}
uint8_t *mmx4_coop_first_body(void) {return backup;}
static unsigned actor_context;
static unsigned area_calls,checkpoint_calls,shared_area_writes;
static unsigned mushroom_calls;
static unsigned stair_calls;
static int mushroom_x,mushroom_y;
static unsigned train_calls,train_foreground_calls;
static unsigned snow_calls,fortress_calls;
static unsigned destructible_calls;
static unsigned lift_calls;
static unsigned shake_calls,explosion_calls;
static unsigned dragon_calls;
static int dragon_camera_x;
static unsigned volcano_calls,fireball_calls,fireball_owner;
static int volcano_player_x,fireball_camera_x;
unsigned mmx4_coop_secondary_allocation_owner(uint32_t actor) {
    return actor==0x8013F328u?fireball_owner:0;
}
static unsigned frost_camera_calls;
static int frost_leader_x;
static unsigned jungle_calls,jungle_private_calls;
int mmx4_coop_combat_actor_context(void) {return (int)actor_context;}
int mmx4_coop_lifecycle_script_owner(void) {return script_owner;}
void mmx4_coop_lifecycle_camera_bounds(CPUState *cpu,unsigned owner) {
    (void)cpu;unsigned other=owner^1u;
    if(other)mmx4_coop_enter_second();
    int minimum=(int16_t)psx_mod_read_half(CAMERA+0x1E);
    if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)<minimum+8)
        psx_mod_write_half(MMX4_PLAYER+10,(uint16_t)(minimum+8));
    if(other)mmx4_coop_leave_second();
}
int mmx4_coop_alive(unsigned seat) {
    const uint8_t *body=seat?second:backup;
    if(seat==projected)return psx_mod_read_byte(MMX4_PLAYER) && psx_mod_read_byte(MMX4_PLAYER+4)<2;
    return body[0] && body[4]<2;
}
void mmx4_coop_enter_second(void) {
    CHECK(!projected);
    for(unsigned offset=0;offset<sizeof second;++offset) {
        backup[offset]=psx_mod_read_byte(MMX4_PLAYER+offset);
        psx_mod_write_byte(MMX4_PLAYER+offset,second[offset]);
    }
    projected=1;
}
void mmx4_coop_leave_second(void) {
    CHECK(projected);
    for(unsigned offset=0;offset<sizeof second;++offset) {
        second[offset]=psx_mod_read_byte(MMX4_PLAYER+offset);
        psx_mod_write_byte(MMX4_PLAYER+offset,backup[offset]);
    }
    projected=0;
}
int mmx4_coop_finish(CPUState *cpu,uint32_t result) {cpu->gpr[2]=result;return 1;}
static void native_scan(CPUState *cpu);
static uint32_t invoke(CPUState *cpu,uint32_t address) {
    for(unsigned index=0;index<filter_count;++index)
        if(filters[index].address==address && filters[index].callback(cpu,address))return cpu->gpr[2];
    uint32_t actor=cpu->gpr[4];
    if(address==0x800BC748u) {
        ++volcano_calls;volcano_player_x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10);
        fireball_owner=projected+1u;
        CHECK(!mmx4_coop_split_snapshot_ready());
        psx_mod_write_byte(actor+4,1);return 78;
    }
    if(address==0x800A03B8u) {
        ++fireball_calls;fireball_camera_x=(int16_t)psx_mod_read_half(CAMERA+10);
        CHECK(!mmx4_coop_split_snapshot_ready());
        if(!psx_mod_read_byte(actor+4)) {
            psx_mod_write_half(actor+10,psx_mod_read_half(CAMERA+10)+100);
            psx_mod_write_half(actor+14,psx_mod_read_half(CAMERA+14)+100);
            psx_mod_write_byte(actor+4,1);
        }else if(!(psx_mod_read_half(CAMERA+0x34)&16u)) {
            cpu->gpr[16]=actor;cpu->gpr[31]=0x800A0858u;cpu->gpr[6]=2;
            mmx4_coop_call(cpu,0x80028B68u,16,6);
        }
        return 79;
    }
    if(address==0x80041DF0u || address==0x8004205Cu) {
        ++dragon_calls;dragon_camera_x=(int16_t)psx_mod_read_half(CAMERA+10);
        CHECK(!mmx4_coop_split_snapshot_ready());
        if(address==0x80041DF0u) {
            psx_mod_write_byte(actor+0x14,255);psx_mod_write_half(actor+10,38);psx_mod_write_half(actor+14,180);
        }else {
            psx_mod_write_half(actor+10,psx_mod_read_half(CAMERA+10)+38);
            psx_mod_write_byte(actor+0x14,0);
        }
        return 77;
    }
    if(address==0x80028B68u || address==0x80028BACu) {
        unsigned axis=address==0x80028BACu;
        ++shake_calls;
        psx_mod_write_half(CAMERA+0x34,psx_mod_read_half(CAMERA+0x34)|(axis?1u:16u));
        psx_mod_write_byte(CAMERA+0x36+axis,(uint8_t)cpu->gpr[4]);
        psx_mod_write_byte(CAMERA+0x45+axis,(uint8_t)cpu->gpr[5]);
        psx_mod_write_byte(CAMERA+0x3Eu+axis,(uint8_t)cpu->gpr[5]);
        psx_mod_write_byte(CAMERA+0x3A+axis,(uint8_t)cpu->gpr[6]);
        psx_mod_write_byte(CAMERA+0x3C+axis,(uint8_t)cpu->gpr[6]);
        return 0;
    }
    if(address==0x800BBC14u) {
        ++explosion_calls;
        psx_mod_write_half(actor+10,psx_mod_read_half(CAMERA+10)+160);
        psx_mod_write_half(actor+14,psx_mod_read_half(CAMERA+14)+128);
        cpu->gpr[31]=0x800BBE58u;cpu->gpr[6]=2;
        return mmx4_coop_call(cpu,0x80028BACu,10,4);
    }
    if(address==0x800B8AF8u) {
        ++frost_camera_calls;
        frost_leader_x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10);
        psx_mod_write_byte(actor+0x14,psx_mod_read_byte(actor+0x14)+1u);
        return 0;
    }
    if(address==0x8002A484u) {
        CHECK(local_scope && !psx_mod_read_half(0x80175EA0u));
        CHECK(psx_mod_read_byte(0x80166BB0u)&4u);
        psx_mod_write_byte(0x80166BB0u,2);++palette_rebuilds;return 0;
    }
    if(address==0x80016004u) {
        CHECK(local_scope && psx_mod_read_byte(0x80166BB0u)==2);
        psx_mod_write_byte(0x80166BB0u,0);++palette_uploads;return 0;
    }
    if(address==0x800DA7C0u || address==0x800DA878u) {
        ++fortress_calls;
        if(psx_mod_read_byte(actor+4)>1)return 0xF00Du;
        if(address==0x800DA7C0u) {
            unsigned t=(psx_mod_read_byte(actor+0x14)+1u)%8u;
            psx_mod_write_byte(actor+0x14,(uint8_t)t);
            if(!t) {
                unsigned frame=(psx_mod_read_byte(actor+0x15)+1u)%4u;
                psx_mod_write_byte(actor+0x15,(uint8_t)frame);
                psx_mod_write_half(CAMERA+0x54u+10,(uint16_t)(frame*512u));
                psx_mod_write_byte(CAMERA+0x54u+0x4C,1);
            }
        }else {
            if(!psx_mod_read_byte(actor+4)) {
                psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+0x14,0x8000u);
                psx_mod_write_byte(CAMERA+0x54u+0x4D,1);
                psx_mod_write_byte(CAMERA+0x54u+0x4E,6);
            }
            int32_t reverse=(int32_t)(psx_mod_read_word(CAMERA+20)-psx_mod_read_word(CAMERA+8));
            psx_mod_write_word(CAMERA+0x54u+8,psx_mod_read_word(CAMERA+0x54u+8)+
                psx_mod_read_word(actor+0x14)-(uint32_t)(reverse/2));
            psx_mod_write_word(CAMERA+0x54u+12,psx_mod_read_word(CAMERA+12));
        }
        return 0xF00Du;
    }
    if(address==0x800B75C8u) {
        ++snow_calls;
        if(!psx_mod_read_byte(actor+4)) {
            /* B7604 resets the atlas before its one-tick startup delay. */
            psx_mod_write_word(CAMERA+2u*0x54u+8,0);
            psx_mod_write_word(CAMERA+2u*0x54u+12,0);
            psx_mod_write_byte(actor+4,1);psx_mod_write_half(actor+0x14,1);
            return 0xB75C8u;
        }
        /* B7764 adds fractional horizontal velocity, subtracts two pixels
         * vertically, then wraps256 back to384 and marks the layer dirty. */
        uint32_t snow=CAMERA+2u*0x54u;
        psx_mod_write_word(snow+8,psx_mod_read_word(snow+8)+0x18000u);
        uint32_t y=psx_mod_read_word(snow+12)-0x20000u;
        if(y==0x1000000u) {y=0x1800000u;psx_mod_write_byte(snow+0x4C,1);}
        psx_mod_write_word(snow+12,y);
        psx_mod_write_half(actor+0x16,psx_mod_read_half(actor+0x16)-1u);
        return 0xB75C8u;
    }
    if(address==0x800293E8u)return allocations<capacity?0x80160000u+allocations++*0x100u:0;
    if(address==0x80028E24u) {
        CPUState saved=*cpu;++scans;
        int x=(int16_t)psx_mod_read_half(CAMERA+10),y=(int16_t)psx_mod_read_half(CAMERA+14);
        if(native_directional_scans) {
            /* Original 28E24 scans only the 32px moving edges; 28FEC
             * applies each record's offset in that movement direction. */
            int old_x=(int16_t)psx_mod_read_half(CAMERA+0x16);
            int old_y=(int16_t)psx_mod_read_half(CAMERA+0x1A);
            if(x!=old_x) {
                scan_direction=x>old_x?1u:2u;
                cpu->gpr[4]=(uint32_t)(x+(x>old_x?336:-48));
                cpu->gpr[5]=(uint32_t)(x+(x>old_x?368:-16));
                cpu->gpr[6]=(uint32_t)(y-48);cpu->gpr[7]=(uint32_t)(y+288);
                native_scan(cpu);
            }
            if(y!=old_y) {
                *cpu=saved;scan_direction=y>old_y?3u:4u;
                cpu->gpr[4]=(uint32_t)(x-48);cpu->gpr[5]=(uint32_t)(x+368);
                cpu->gpr[6]=(uint32_t)(y+(y>old_y?256:-48));
                cpu->gpr[7]=(uint32_t)(y+(y>old_y?288:-16));
                native_scan(cpu);
            }
            scan_direction=0;*cpu=saved;return 0;
        }
        cpu->gpr[4]=(uint32_t)(x-48);cpu->gpr[5]=(uint32_t)(x+368);
        cpu->gpr[6]=(uint32_t)(y-48);cpu->gpr[7]=(uint32_t)(y+288);
        native_scan(cpu);*cpu=saved;return 0;
    }
    if(address==0x80027850u) {
        int x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10)-160;
        int y=(int16_t)psx_mod_read_half(MMX4_PLAYER+14)-128;
        psx_mod_write_word(CAMERA+0x14,psx_mod_read_word(CAMERA+8));
        psx_mod_write_word(CAMERA+0x18,psx_mod_read_word(CAMERA+12));
        if(x<0)x=0;if(y<0)y=0;
        int minimum=(int16_t)psx_mod_read_half(CAMERA+0x26);
        if((int16_t)psx_mod_read_half(MMX4_PLAYER+10)<minimum+8)
            psx_mod_write_half(MMX4_PLAYER+10,(uint16_t)(minimum+8));
        psx_mod_write_half(CAMERA+10,(uint16_t)x);psx_mod_write_half(CAMERA+14,(uint16_t)y);return 0;
    }
    if(address==0x800C0D98u) {
        ++destructible_calls;
        /* Original finalizer's subtype0 branch releases both maximum-X
         * halfwords before the shared actor is retired once. */
        if(!psx_mod_read_byte(actor+2)) {
            psx_mod_write_half(CAMERA+0x1C,4544);
            psx_mod_write_half(CAMERA+0x24,4544);
        }
        psx_mod_write_byte(actor,0);psx_mod_write_byte(actor+3,0);
        return 0xABCD1234u;
    }
    if(address==0x80027D40u || address==0x800281E8u) {
        unsigned layer=address==0x80027D40u?1:2;
        uint32_t background=CAMERA+layer*0x54u;
        psx_mod_write_word(background+20,psx_mod_read_word(background+8));
        psx_mod_write_word(background+24,psx_mod_read_word(background+12));
        if(psx_mod_read_byte(background+4)==5)return 0; /* Native train helpers are no-ops. */
        if(psx_mod_read_byte(background+4)==2) {
            psx_mod_write_half(background+10,psx_mod_read_half(CAMERA+10)+psx_mod_read_half(background+0x40));
            psx_mod_write_half(background+14,psx_mod_read_half(CAMERA+14)+psx_mod_read_half(background+0x42));return 0;
        }
        psx_mod_write_half(CAMERA+layer*0x54u+10,psx_mod_read_half(CAMERA+10)/2);
        psx_mod_write_half(CAMERA+layer*0x54u+14,psx_mod_read_half(CAMERA+14)/4);return 0;
    }
    if(address==0x800DA298u) {
        ++train_calls;
        if(psx_mod_read_byte(actor+4)>1)return 0;
        int32_t reverse=(int32_t)(psx_mod_read_word(CAMERA+20)-psx_mod_read_word(CAMERA+8));
        uint32_t near=CAMERA+0x54u,far=CAMERA+0xA8u;
        psx_mod_write_word(near+8,psx_mod_read_word(near+8)+0x8000u-(uint32_t)(reverse/2));
        psx_mod_write_word(far+8,psx_mod_read_word(far+8)-0x4000u-(uint32_t)((reverse/2)/2));
        psx_mod_write_byte(near+0x4E,7);psx_mod_write_byte(far+0x4E,8);
        return 0;
    }
    if(address==0x800D8ED4u) {
        if(actor>=0x80169CB8u && actor<0x80169D78u)++jungle_calls;
        else ++jungle_private_calls;
        if(psx_mod_read_byte(actor+4)>1)return 0;
        /* Retail D8F10/D8F48/D91A8 and their three phase callbacks. Mode 5
         * has no ordinary follow update; each phase's initialization takes
         * one tick before its origin mapping or mode-2 follow takes effect. */
        if(!psx_mod_read_byte(actor+4)) {
            psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+5,2);
            psx_mod_write_byte(actor+0x14,3);
        }
        unsigned old=psx_mod_read_byte(actor+0x14);psx_mod_write_byte(actor+0x15,(uint8_t)old);
        int x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10),y=(int16_t)psx_mod_read_half(MMX4_PLAYER+14);
        unsigned phase=y<=512?(x>=1104):x>=2560?2u:1u;
        psx_mod_write_byte(actor+0x14,(uint8_t)phase);
        if(phase!=old){psx_mod_write_byte(actor+5,(uint8_t)phase);psx_mod_write_byte(actor+6,0);}
        unsigned callback=psx_mod_read_byte(actor+5);
        uint32_t background=CAMERA+0x54u;
        if(callback==3)return 0xB8ED4u;
        if(!psx_mod_read_byte(actor+6)) {
            psx_mod_write_byte(background+4,(uint8_t)(callback==1?2:5));
            psx_mod_write_half(background+0x40,(uint16_t)(callback==0?40:callback==1?512:2912));
            psx_mod_write_byte(actor+6,1);return 0xB8ED4u;
        }
        if(callback==1){psx_mod_write_byte(actor+5,3);psx_mod_write_byte(actor+6,0);return 0xB8ED4u;}
        int cx=(int16_t)psx_mod_read_half(CAMERA+10),cy=(int16_t)psx_mod_read_half(CAMERA+14);
        int half=callback==0?cx>>1:(int16_t)(cx-2400)>>1;
        int bx=callback==0?cx+half:half+(half>>1);
        int by=callback==0?cy:cy-((int16_t)(cy-1280)>>2);
        psx_mod_write_half(background+10,(uint16_t)(bx+psx_mod_read_half(background+0x40)));
        psx_mod_write_half(background+14,(uint16_t)(by+(callback==2?psx_mod_read_half(background+0x42):0)));
        return 0xB8ED4u;
    }
    if(address==0x800D964Cu) {
        ++train_foreground_calls;
        if(!psx_mod_read_byte(actor+7)) {
            uint32_t speed=psx_mod_read_word(actor+0x18);
            if(speed)psx_mod_write_word(actor+0x18,speed-2048u);
            else {
                psx_mod_write_half(CAMERA+0x1C,9440);psx_mod_write_half(CAMERA+0x24,9440);
                psx_mod_write_byte(actor+7,1);psx_mod_write_byte(actor+0x17,0);
            }
        }else if(!psx_mod_read_byte(MMX4_PLAYER+0xBC)) {
            int32_t dx=(int32_t)(psx_mod_read_word(CAMERA+8)-psx_mod_read_word(CAMERA+20));
            psx_mod_write_word(CAMERA+0x54u+8,psx_mod_read_word(CAMERA+0x54u+8)+(uint32_t)dx);
            psx_mod_write_word(CAMERA+0xA8u+8,psx_mod_read_word(CAMERA+0xA8u+8)+(uint32_t)(dx>>1));
        }
        return 0xABCDu;
    }
    if(address==0x800D8648u || address==0x800D9218u) {
        ++train_calls;
        if(psx_mod_read_byte(actor+4)>1)return 0;
        uint32_t first=CAMERA+0x54u,second_layer=CAMERA+0xA8u;
        if(!psx_mod_read_byte(actor+4)) {
            psx_mod_write_byte(actor+4,1);psx_mod_write_word(actor+0x18,0x80000u);
            psx_mod_write_byte(actor+0x14,5);psx_mod_write_byte(actor+5,5);
            psx_mod_write_byte(first+0x4D,2);psx_mod_write_byte(second_layer+0x4D,2);
            psx_mod_write_byte(first+0x4E,address==0x800D8648u?7:13);
            psx_mod_write_byte(second_layer+0x4E,5);
            psx_mod_write_half(CAMERA+0x2C,48);psx_mod_write_half(CAMERA+0x2E,160);
        }
        if(address==0x800D9218u) {
            /* Retail D993C resets subphases before the selected callback. */
            const int thresholds[]={5808,7648,9984,10000};unsigned region=0;
            int x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10);
            while(region<4 && x>=thresholds[region])++region;
            if(region!=psx_mod_read_byte(actor+0x14)) {
                psx_mod_write_byte(actor+5,(uint8_t)region);
                psx_mod_write_byte(actor+6,0);psx_mod_write_byte(actor+7,0);
            }
            psx_mod_write_byte(actor+0x14,(uint8_t)region);
            if(psx_mod_read_byte(actor+5)==1 && psx_mod_read_byte(actor+6)==2)
                invoke(cpu,0x800D964Cu);
        }
        int32_t speed=(int32_t)psx_mod_read_word(actor+0x18);
        psx_mod_write_word(first+8,psx_mod_read_word(first+8)+(uint32_t)speed);
        psx_mod_write_word(second_layer+8,psx_mod_read_word(second_layer+8)+(uint32_t)(speed>>1));
        return 0x12345678u;
    }
    if(address==0x800B56F4u) {
        ++area_calls;uint32_t parameters=psx_mod_read_word(actor+0x18u);
        int x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10),y=(int16_t)psx_mod_read_half(MMX4_PLAYER+14);
        if(x>=psx_mod_read_half(parameters+2) && x<=psx_mod_read_half(parameters) &&
           y>=psx_mod_read_half(parameters+6) && y<=psx_mod_read_half(parameters+4)) {
            psx_mod_write_byte(CAMERA+0x47,2);
            psx_mod_write_half(CAMERA+0x26,512);psx_mod_write_half(CAMERA+0x2C,64);
            psx_mod_write_half(MMX4_PLAY+0x30,7);++shared_area_writes;
        }
        return 0;
    }
    if(address==0x800B6A0Cu) {
        ++checkpoint_calls;
        if((int16_t)psx_mod_read_half(MMX4_PLAYER+14)>=(int16_t)psx_mod_read_half(actor+14))
            psx_mod_write_byte(MMX4_PLAY+0x1D,2);
        return 0;
    }
    if(address==0x800C3880u) {
        ++lift_calls;
        psx_mod_write_half(CAMERA+0x28,720);psx_mod_write_half(CAMERA+0x2A,720);
        psx_mod_write_byte(CAMERA+0x47,1);
        psx_mod_write_byte(actor+5,1); /* One shared lift step, never per view. */
        return 0;
    }
    if(address==0x800B7078u || address==0x800BA57Cu) {
        ++mushroom_calls;
        mushroom_x=(int16_t)psx_mod_read_half(MMX4_PLAYER+10);
        mushroom_y=(int16_t)psx_mod_read_half(MMX4_PLAYER+14);
        if(mushroom_x>=3536)psx_mod_write_byte(MMX4_PLAY+0x1D,2);
        return 0;
    }
    if(address==0x800B7140u || address==0x800BA644u) {
        ++stair_calls;
        if(!psx_mod_read_byte(actor+6)) {
            psx_mod_write_byte(actor+6,1);psx_mod_write_byte(CAMERA+4,2);
            psx_mod_write_byte(actor+0x16,0);psx_mod_write_half(actor+0x18,0);return 0;
        }
        int dx=(int16_t)psx_mod_read_half(CAMERA+10)-(int16_t)psx_mod_read_half(CAMERA+22);
        if(dx>8)dx=8;if(dx<-8)dx=-8;
        int fraction=(int8_t)psx_mod_read_byte(actor+0x17)+dx;
        psx_mod_write_byte(actor+0x17,(uint8_t)fraction);
        if(fraction>=8 || fraction<=-8) {
            unsigned phase=(psx_mod_read_byte(actor+0x16)+(fraction<0?2:1))%3;
            psx_mod_write_byte(actor+0x16,(uint8_t)phase);psx_mod_write_byte(actor+0x17,0);
            psx_mod_write_half(CAMERA+0x54u+10,(uint16_t)(phase*512));
            psx_mod_write_byte(CAMERA+0x54u+0x4C,1);
        }
        return 0;
    }
    if(address==0x8002B1E8u || address==0x8002B318u) {
        unsigned layer=psx_mod_read_byte(actor+0x14u);
        int x=(int16_t)psx_mod_read_half(actor+10)-(int16_t)psx_mod_read_half(CAMERA+layer*0x54u+10);
        int y=(int16_t)psx_mod_read_half(actor+14)-(int16_t)psx_mod_read_half(CAMERA+layer*0x54u+14);
        unsigned inside=(uint16_t)(x+cpu->gpr[5])<(uint16_t)(320+2*cpu->gpr[5]) &&
            (uint16_t)(y+cpu->gpr[6])<(uint16_t)(240+2*cpu->gpr[6]);
        if(address==0x8002B318u)psx_mod_write_byte(actor+3,(uint8_t)inside);
        return address==0x8002B318u?inside:!inside;
    }
    CHECK(0);return 0;
}
uint32_t mmx4_coop_call(CPUState *cpu,uint32_t address,uint32_t arg0,uint32_t arg1) {
    CPUState saved=*cpu;cpu->gpr[4]=arg0;cpu->gpr[5]=arg1;
    uint32_t result=invoke(cpu,address);*cpu=saved;return result;
}
void psx_dispatch_call(CPUState *cpu,uint32_t address,uint32_t caller) {
    (void)caller;CHECK(address==0x80028FECu);native_scan(cpu);
}
static void native_scan(CPUState *cpu) {
    CPUState saved=*cpu;
    for(unsigned index=0;index<8;++index) {
        uint32_t record=native_table+index*8u;
        if(psx_mod_read_byte(record+3)==255)break;
        if(psx_mod_read_byte(record)&0x81u)continue;
        int x=(int16_t)psx_mod_read_half(record+4),y=(int16_t)psx_mod_read_half(record+6);
        int left=(int32_t)saved.gpr[4],right=(int32_t)saved.gpr[5];
        int top=(int32_t)saved.gpr[6],bottom=(int32_t)saved.gpr[7];
        if(native_directional_scans && scan_direction) {
            unsigned flag=psx_mod_read_byte(record)&0x70u;
            int offset=flag?psx_mod_read_half(0x800F4332u+(flag>>4)*2u):0;
            if(scan_direction==1) {left+=offset;right+=offset;}
            else if(scan_direction==2) {left-=offset;right-=offset;}
            else if(scan_direction==3) {top+=offset;bottom+=offset;}
            else if(scan_direction==4) {top-=offset;bottom-=offset;}
        }
        if(x<=left || x>=right || y<=top || y>=bottom)continue;
        *cpu=saved;cpu->gpr[19]=record;cpu->gpr[18]=record+3u;
        cpu->gpr[31]=0x8002916Cu;cpu->gpr[4]=psx_mod_read_byte(record+3u);
        cpu->gpr[17]=invoke(cpu,0x800293E8u);
        if(cpu->gpr[17]) {
            spawned(cpu,0x80029284u);psx_mod_write_byte(record,psx_mod_read_byte(record)|1u);
        }
    }
    *cpu=saved;
}
static void actor_position(uint32_t actor,int x,int y) {
    psx_mod_write_half(actor+10,(uint16_t)x);psx_mod_write_half(actor+14,(uint16_t)y);
}
static void second_position(int x,int y) {
    second[10]=(uint8_t)x;second[11]=(uint8_t)(x>>8);
    second[14]=(uint8_t)y;second[15]=(uint8_t)(y>>8);
}
static void placement(unsigned index,int x,int y,unsigned category) {
    uint32_t record=native_table+index*8u;
    psx_mod_write_half(record+4,(uint16_t)x);psx_mod_write_half(record+6,(uint16_t)y);
    psx_mod_write_byte(record+3,(uint8_t)category);
}
static uint32_t saved_camera(unsigned seat,unsigned layer) {
    return 0x801F0000u+16u+seat*0xFCu+layer*0x54u;
}
static void mushroom_lift_high_jump(CPUState *cpu) {
    for(unsigned owner=0;owner<2;++owner)for(unsigned excluded=0;excluded<6;++excluded) {
        const uint32_t lift=0x80165A30u;
        split=ready=1;local_scope=projected=0;script_owner=-1;paired_door=0;
        memset(ram+(CAMERA&0x1FFFFFu),0,0xFC);
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,3);
        psx_mod_write_byte(MMX4_PLAY+13,1);psx_mod_write_byte(MMX4_PLAY+0x10,0);
        psx_mod_write_byte(MMX4_PLAY+0x1C,0);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        psx_mod_write_byte(MMX4_PLAYER+0x5C,32);
        second[0]=second[4]=1;second[0x5C]=32;
        actor_position(MMX4_PLAYER,6048,1420);second_position(6072,1419);
        psx_mod_write_byte(lift,1);psx_mod_write_byte(lift+1,14);
        psx_mod_write_byte(lift+4,1);psx_mod_write_byte(lift+5,1);
        psx_mod_write_half(CAMERA+0x1C,5888);psx_mod_write_half(CAMERA+0x1E,5888);
        psx_mod_write_half(CAMERA+0x24,5888);psx_mod_write_half(CAMERA+0x26,5888);
        psx_mod_write_word(CAMERA+12,1500u<<16|0x1234u);
        psx_mod_write_word(CAMERA+24,1501u<<16|0x5678u);
        psx_mod_write_half(CAMERA+0x20,1500);psx_mod_write_half(CAMERA+0x22,720);
        mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(owner);
        if(excluded==1)psx_mod_write_byte(MMX4_PLAY+12,12);
        if(excluded==2)psx_mod_write_byte(MMX4_PLAY+13,0);
        if(excluded==3)psx_mod_write_byte(lift+5,2); /* Original top exit releases follow. */
        if(excluded==4)psx_mod_write_byte(lift+1,13);
        if(excluded==5)psx_mod_write_half(saved_camera(owner^1u,0)+0x26,0);
        unsigned calls_before=lift_calls;
        mmx4_coop_split_camera(cpu,owner);
        uint32_t camera=saved_camera(owner^1u,0);
        CHECK(lift_calls==calls_before); /* Camera follow cannot repeat world mechanics. */
        if(!excluded) {
            CHECK(psx_mod_read_word(camera+12)==(1500u<<16|0x1234u));
            CHECK(psx_mod_read_word(camera+24)==(1501u<<16|0x5678u));
            CHECK(psx_mod_read_half(camera+0x20)==1500);
            CHECK(psx_mod_read_half(saved_camera(owner^1u,1)+14)==1500/4);
        }else CHECK(psx_mod_read_half(camera+14)==(owner?1420:1419)-128);
        CHECK(!projected && mmx4_coop_split_snapshot_ready());
    }
    psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_byte(0x80165A30u,0);
}
static void jet_prefetched_placement(CPUState *cpu) {
    split=ready=1;local_scope=projected=0;script_owner=-1;paired_door=0;
    native_directional_scans=1;native_table=TABLE;allocations=0;capacity=16;
    memset(ram+(CAMERA&0x1FFFFFu),0,0xFC);
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,5);
    psx_mod_write_byte(MMX4_PLAY+13,1);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    psx_mod_write_byte(MMX4_PLAYER+0x5C,32);second[0]=second[4]=1;second[0x5C]=32;
    actor_position(MMX4_PLAYER,16940,170);second_position(16964,169);
    psx_mod_write_word(CAMERA+8,16780u<<16);psx_mod_write_word(CAMERA+20,16780u<<16);
    psx_mod_write_half(CAMERA+0x26,16384);
    psx_mod_write_word(0x800F43C8u+5u*8u+4u,TABLE);
    memset(ram+(TABLE&0x1FFFFFu),0,16);
    placement(0,17146,138,0);psx_mod_write_byte(TABLE+1,56);
    psx_mod_write_byte(TABLE+8+3,255);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    invoke(cpu,0x80028E24u);
    CHECK(!allocations && !(psx_mod_read_byte(TABLE)&1u));
    /* The camera can be stationary when the door commits its new bounds.
     * An unvisited rejected record must retry and allocate exactly once. */
    psx_mod_write_half(CAMERA+0x26,17024);
    invoke(cpu,0x80028E24u);
    CHECK(allocations==1 && (psx_mod_read_byte(TABLE)&1u));
    invoke(cpu,0x80028E24u);CHECK(allocations==1);
    /* Original moving-edge allocation still follows retail timing, even
     * before the new minimum commits; only supplemental prefetch waits. */
    allocations=0;psx_mod_write_byte(TABLE,0);
    psx_mod_write_half(CAMERA+0x26,16384);
    psx_mod_write_word(CAMERA+20,16776u<<16);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    invoke(cpu,0x80028E24u);
    CHECK(allocations==1 && (psx_mod_read_byte(TABLE)&1u));
    for(unsigned excluded=0;excluded<5;++excluded)
        CHECK(mmx4_coop_view_prefetch_allowed(excluded==0?12:5,excluded==1?0:1,
            excluded==2?1:0,excluded==3?55:56,excluded==4?17024:16384));
    native_directional_scans=0;
}
static void fortress_backgrounds(CPUState *cpu) {
    const uint32_t actor=0x80169CB8u;
    split=ready=1;local_scope=projected=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    for(unsigned seat=0;seat<2;++seat) {
        psx_mod_write_word(saved_camera(seat,0)+8,seat?0xFFFFFFFDu:3u);
        psx_mod_write_word(saved_camera(seat,0)+20,0);
        psx_mod_write_word(saved_camera(seat,0)+12,(300u+seat*200u)<<16);
        psx_mod_write_word(saved_camera(seat,1)+8,(100u+seat*200u)<<16);
    }
    psx_mod_write_word(CAMERA+8,3);psx_mod_write_word(CAMERA+20,0);
    psx_mod_write_word(CAMERA+12,300u<<16);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,7);psx_mod_write_byte(actor+4,0);
    unsigned calls=fortress_calls;cpu->gpr[4]=actor;
    CHECK(invoke(cpu,0x800DA878u)==0xF00Du && fortress_calls==calls+1);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==(100u<<16)+0x8001u);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==(300u<<16)+0x7FFFu);
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(psx_mod_read_word(saved_camera(seat,1)+12)==(300u+seat*200u)<<16);
        CHECK(psx_mod_read_byte(saved_camera(seat,1)+0x4D)==1);
        CHECK(psx_mod_read_byte(saved_camera(seat,1)+0x4E)==6);
    }
    uint32_t digest=mmx4_coop_split_digest(2166136261u);
    psx_mod_write_byte(actor+4,2);invoke(cpu,0x800DA878u);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(actor+4,1);local_scope=1;invoke(cpu,0x800DA878u);local_scope=0;
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(actor+1,6);psx_mod_write_byte(actor+0x14,0);psx_mod_write_byte(actor+0x15,0);
    psx_mod_write_half(CAMERA+0x54u+10,0);
    for(unsigned seat=0;seat<2;++seat)psx_mod_write_half(saved_camera(seat,1)+10,0);
    calls=fortress_calls;
    for(unsigned tick=0;tick<32;++tick) {
        invoke(cpu,0x800DA7C0u);
        for(unsigned seat=0;seat<2;++seat)CHECK(psx_mod_read_half(saved_camera(seat,1)+10)==((tick+1u)/8u%4u)*512u);
    }
    CHECK(fortress_calls==calls+32 && !psx_mod_read_byte(actor+0x15));
    CHECK(mmx4_coop_split_snapshot_ready());
}
static void snow_backgrounds(CPUState *cpu) {
    const uint32_t actor=0x80142FC8u,snow=CAMERA+2u*0x54u;
    split=ready=1;local_scope=projected=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,2);
    psx_mod_write_byte(MMX4_PLAY+13,0);psx_mod_write_byte(MMX4_PLAY+0x10,0);
    psx_mod_write_byte(MMX4_PLAY+0x1C,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;actor_position(MMX4_PLAYER,144,347);second_position(3281,378);
    actor_position(CAMERA,0,155);psx_mod_write_byte(snow+4,5);
    psx_mod_write_word(snow+8,0x123400u);psx_mod_write_word(snow+12,0x1020000u);
    psx_mod_write_byte(snow+0x4C,0);mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    psx_mod_write_word(saved_camera(1,2)+8,0x345600u);
    psx_mod_write_word(saved_camera(1,2)+12,0x1020000u);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,9);psx_mod_write_byte(actor+4,2);
    psx_mod_write_half(actor+0x16,10);cpu->gpr[4]=actor;
    unsigned calls=snow_calls;CHECK(invoke(cpu,0x800B75C8u)==0xB75C8u);
    CHECK(snow_calls==calls+1 && psx_mod_read_half(actor+0x16)==9);
    CHECK(psx_mod_read_word(snow+8)==0x13B400u && psx_mod_read_word(snow+12)==0x1800000u);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==0x13B400u);
    /* Weather is an atlas phase, not a foreground-relative origin. A stale
     * view must receive the exact native phase even after an absolute reset. */
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==0x13B400u);
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(psx_mod_read_word(saved_camera(seat,2)+12)==0x1800000u);
        CHECK(psx_mod_read_byte(saved_camera(seat,2)+0x4C)==1);
    }
    /* The later native camera snapshot must not apply this clock twice. */
    mmx4_coop_split_camera(cpu,0);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==0x13B400u);
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==0x13B400u);
    cpu->gpr[4]=actor;invoke(cpu,0x800B75C8u);
    CHECK(psx_mod_read_half(actor+0x16)==8 && snow_calls==calls+2);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==0x153400u);
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==0x153400u);
    psx_mod_write_byte(actor+4,0);invoke(cpu,0x800B75C8u);
    CHECK(snow_calls==calls+3 && psx_mod_read_byte(actor+4)==1);
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(!psx_mod_read_word(saved_camera(seat,2)+8));
        CHECK(!psx_mod_read_word(saved_camera(seat,2)+12));
    }
    uint32_t digest=mmx4_coop_split_digest(2166136261u);
    local_scope=1;invoke(cpu,0x800B75C8u);local_scope=0;
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    split=0;invoke(cpu,0x800B75C8u);split=1;
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(MMX4_PLAY+12,1);invoke(cpu,0x800B75C8u);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(MMX4_PLAY+12,2);
    mmx4_coop_enter_second();invoke(cpu,0x800B75C8u);mmx4_coop_leave_second();
    CHECK(mmx4_coop_split_digest(2166136261u)==digest && mmx4_coop_split_snapshot_ready());
}
static void jungle_frame(CPUState *cpu,unsigned owner,uint32_t actor) {
    mmx4_coop_split_camera_prepare(owner);
    if(owner)mmx4_coop_enter_second();
    invoke(cpu,0x80027850u);
    if(owner)mmx4_coop_leave_second();
    mmx4_coop_split_camera(cpu,owner);
    invoke(cpu,0x80027D40u);
    cpu->gpr[4]=actor;invoke(cpu,0x800D8ED4u);
}
static void jungle_backgrounds(CPUState *cpu) {
    const uint32_t actor=0x80169CB8u,background=CAMERA+0x54u;
    script_owner=-1;split=ready=1;local_scope=projected=0;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
    psx_mod_write_byte(MMX4_PLAY+12,1);psx_mod_write_byte(MMX4_PLAY+13,1);
    psx_mod_write_byte(MMX4_PLAY+0x10,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);second[0]=second[4]=1;
    psx_mod_write_half(CAMERA+0x26,0);psx_mod_write_half(CAMERA+0x42,0);
    psx_mod_write_byte(background+4,5);psx_mod_write_half(background+0x40,40);
    psx_mod_write_half(background+0x42,0);psx_mod_write_word(background+8,40u<<16);
    psx_mod_write_word(background+12,256u<<16);
    actor_position(MMX4_PLAYER,769,843);second_position(1094,954);
    psx_mod_write_half(CAMERA+10,609);psx_mod_write_half(CAMERA+14,651);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    psx_mod_write_half(saved_camera(1,0)+10,915);psx_mod_write_half(saved_camera(1,0)+14,762);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,1);psx_mod_write_byte(actor+4,0);
    cpu->gpr[4]=actor;unsigned calls=jungle_calls,private_calls=jungle_private_calls;
    CHECK(invoke(cpu,0x800D8ED4u)==0xB8ED4u);
    CHECK(jungle_calls==calls+1 && jungle_private_calls==private_calls+2);
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(psx_mod_read_byte(saved_camera(seat,1)+4)==2);
        CHECK(psx_mod_read_half(saved_camera(seat,1)+0x40)==512);
    }
    /* The actual failing room: P2's native mode-2 follow now draws its
     * own terrain instead of retaining the opening forest at 40/256. */
    mmx4_coop_split_camera(cpu,0);invoke(cpu,0x80027D40u);invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(saved_camera(1,1)+10)==psx_mod_read_half(saved_camera(1,0)+10)+512);
    CHECK(psx_mod_read_half(saved_camera(1,1)+14)==psx_mod_read_half(saved_camera(1,0)+14));
    CHECK(psx_mod_read_half(background+10)==psx_mod_read_half(saved_camera(0,1)+10));
    CHECK(jungle_calls==calls+2 && !projected && mmx4_coop_split_snapshot_ready());
    /* Separated authored regions keep independent phase/offset state. */
    actor_position(MMX4_PLAYER,56,139);second_position(3001,1419);
    psx_mod_write_half(CAMERA+10,0);psx_mod_write_half(CAMERA+14,0);
    psx_mod_write_half(saved_camera(0,0)+10,0);psx_mod_write_half(saved_camera(0,0)+14,0);
    psx_mod_write_half(saved_camera(1,0)+10,2841);psx_mod_write_half(saved_camera(1,0)+14,1227);
    invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(saved_camera(0,1)+0x40)==40);
    CHECK(psx_mod_read_half(saved_camera(1,1)+0x40)==2912);
    invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(saved_camera(0,1)+10)==40 && !psx_mod_read_half(saved_camera(0,1)+14));
    CHECK(psx_mod_read_half(saved_camera(1,1)+10)==3242 && psx_mod_read_half(saved_camera(1,1)+14)==1241);
    /* Exchange which player leads. Only that view changes controller phase. */
    actor_position(MMX4_PLAYER,3100,1419);second_position(56,139);
    psx_mod_write_half(CAMERA+10,2940);psx_mod_write_half(CAMERA+14,1227);
    psx_mod_write_half(saved_camera(0,0)+10,2940);psx_mod_write_half(saved_camera(0,0)+14,1227);
    psx_mod_write_half(saved_camera(1,0)+10,0);psx_mod_write_half(saved_camera(1,0)+14,0);
    invoke(cpu,0x800D8ED4u);invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(saved_camera(0,1)+10)==3317 && psx_mod_read_half(saved_camera(0,1)+14)==1241);
    CHECK(psx_mod_read_half(saved_camera(1,1)+10)==40 && !psx_mod_read_half(saved_camera(1,1)+14));
    /* Every native pool slot gets its own pair of phase records. */
    uint32_t last=actor+3u*0x30u;psx_mod_write_byte(last,1);psx_mod_write_byte(last+1,1);
    psx_mod_write_byte(last+4,0);cpu->gpr[4]=last;calls=jungle_calls;
    invoke(cpu,0x800D8ED4u);CHECK(jungle_calls==calls+1);
    CHECK(psx_mod_read_half(saved_camera(0,1)+0x40)==2912 && psx_mod_read_half(saved_camera(1,1)+0x40)==40);
    cpu->gpr[4]=actor;
    /* A surviving P2 owns the canonical selection too; dead P1's distant
     * position cannot change P2's authored scenery phase. */
    psx_mod_write_byte(MMX4_PLAYER+4,2);mmx4_coop_split_camera_prepare(1);
    psx_mod_write_byte(actor+4,0);calls=jungle_calls;private_calls=jungle_private_calls;
    invoke(cpu,0x800D8ED4u);jungle_frame(cpu,1,actor);jungle_frame(cpu,1,actor);
    CHECK(jungle_calls==calls+3 && jungle_private_calls==private_calls+3);
    CHECK(psx_mod_read_half(background+0x40)==40 && psx_mod_read_half(background+10)==40);
    CHECK(psx_mod_read_half(saved_camera(1,1)+10)==40);
    CHECK(psx_mod_read_half(saved_camera(1,1)+14)==psx_mod_read_half(saved_camera(1,0)+14));
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==3100 && (second[10]|(unsigned)second[11]<<8)==56);
    psx_mod_write_byte(MMX4_PLAYER+4,1);second[4]=2;mmx4_coop_split_camera_prepare(0);
    psx_mod_write_byte(actor+4,0);calls=jungle_calls;private_calls=jungle_private_calls;
    invoke(cpu,0x800D8ED4u);jungle_frame(cpu,0,actor);jungle_frame(cpu,0,actor);
    CHECK(jungle_calls==calls+3 && jungle_private_calls==private_calls+3);
    CHECK(psx_mod_read_half(background+0x40)==2912 && psx_mod_read_half(background+10)==3317);
    CHECK(psx_mod_read_half(saved_camera(0,1)+10)==3317);
    second[4]=1;
    /* Dead P2 inherited the far camera, but revives in its original opening
     * region. Its old phase cannot retain the survivor's offset. */
    psx_mod_write_half(saved_camera(1,0)+10,0);psx_mod_write_half(saved_camera(1,0)+14,0);
    invoke(cpu,0x800D8ED4u);invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(saved_camera(1,1)+0x40)==40 && psx_mod_read_half(saved_camera(1,1)+10)==40);
    /* Render transactions and unrelated sections leave all cached state
     * alone. Native shared scenes remain one call and preserve the cameras. */
    uint32_t digest=mmx4_coop_split_digest(2166136261u);calls=jungle_calls;private_calls=jungle_private_calls;
    local_scope=1;invoke(cpu,0x800D8ED4u);local_scope=0;
    CHECK(mmx4_coop_split_digest(2166136261u)==digest && jungle_private_calls==private_calls);
    psx_mod_write_byte(MMX4_PLAY+13,0);invoke(cpu,0x800D8ED4u);psx_mod_write_byte(MMX4_PLAY+13,1);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest && jungle_private_calls==private_calls);
    uint8_t before[2][0xFC],after[0xFC];
    for(unsigned seat=0;seat<2;++seat)CHECK(mmx4_coop_split_camera_copy(seat,before[seat]));
    script_owner=0;invoke(cpu,0x800D8ED4u);script_owner=-1;
    CHECK(jungle_private_calls==private_calls && jungle_calls==calls+3);
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(mmx4_coop_split_camera_copy(seat,after));CHECK(!memcmp(before[seat],after,sizeof after));
    }
    /* Scene synchronization can copy P1's far-region mode/offset into P2's
     * opening view. Its phase must initialize again after the scene ends. */
    psx_mod_write_half(saved_camera(1,0)+10,0);psx_mod_write_half(saved_camera(1,0)+14,0);
    psx_mod_write_byte(saved_camera(1,1)+4,5);psx_mod_write_half(saved_camera(1,1)+0x40,2912);
    invoke(cpu,0x800D8ED4u);invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(saved_camera(1,1)+0x40)==40 && psx_mod_read_half(saved_camera(1,1)+10)==40);
    /* Checkpoint/section reset discards the prior phases. A late first
     * encounter at native state1 still initializes this view's mode. */
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    psx_mod_write_byte(saved_camera(1,1)+4,5);psx_mod_write_half(saved_camera(1,1)+0x40,40);
    psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+5,3);psx_mod_write_byte(actor+0x14,2);
    second_position(1094,954);calls=jungle_calls;
    invoke(cpu,0x800D8ED4u);
    CHECK(jungle_calls==calls+1 && psx_mod_read_byte(saved_camera(1,1)+4)==2);
    CHECK(psx_mod_read_half(saved_camera(1,1)+0x40)==512 && mmx4_coop_split_snapshot_ready());
    /* Unified also needs the survivor's position: kind7 chooses its layer
     * directly from PLAYER, outside the ordinary camera-follow helpers. */
    split=0;psx_mod_write_byte(MMX4_PLAYER+4,2);
    actor_position(MMX4_PLAYER,56,139);second_position(3001,1419);
    actor_position(CAMERA,2841,1227);psx_mod_write_byte(actor+4,0);
    calls=jungle_calls;private_calls=jungle_private_calls;
    invoke(cpu,0x800D8ED4u);invoke(cpu,0x800D8ED4u);
    CHECK(jungle_calls==calls+2 && jungle_private_calls==private_calls && !projected);
    CHECK(psx_mod_read_byte(background+4)==5 && psx_mod_read_half(background+0x40)==2912);
    CHECK(psx_mod_read_half(background+10)==3242 && psx_mod_read_half(background+14)==1241);
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==56 && (second[10]|(unsigned)second[11]<<8)==3001);
    /* Once P1 is active again, retain the ordinary canonical selection. */
    psx_mod_write_byte(MMX4_PLAYER+4,1);
    invoke(cpu,0x800D8ED4u);invoke(cpu,0x800D8ED4u);
    CHECK(psx_mod_read_half(background+0x40)==40);
    split=1;
}
static void storm_backgrounds(CPUState *cpu) {
    uint32_t actor=0x80169CB8u;
    split=ready=1;local_scope=projected=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,7);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);second[0]=second[4]=1;
    CHECK(mmx4_coop_split_activate());
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,5);psx_mod_write_byte(actor+4,1);
    cpu->gpr[4]=actor;
    /* Opposite odd fixed-unit movements exercise native signed truncation;
     * shared speed advances exactly once even with different camera origins. */
    psx_mod_write_word(CAMERA+8,3);psx_mod_write_word(CAMERA+20,0);
    for(unsigned seat=0;seat<2;++seat) {
        psx_mod_write_word(saved_camera(seat,0)+8,seat?0xFFFFFFFDu:3);
        psx_mod_write_word(saved_camera(seat,0)+20,0);
        for(unsigned layer=1;layer<3;++layer)
            psx_mod_write_word(saved_camera(seat,layer)+8,seat?0x200000u:0x100000u);
    }
    unsigned calls=train_calls;invoke(cpu,0x800DA298u);CHECK(train_calls==calls+1);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==0x108001u);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==0x207FFFu);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==0x0FC000u);
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==0x1FC000u);
    CHECK(psx_mod_read_byte(saved_camera(1,1)+0x4E)==7);
    CHECK(psx_mod_read_byte(saved_camera(1,2)+0x4E)==8);
    uint32_t digest=mmx4_coop_split_digest(2166136261u);
    local_scope=1;invoke(cpu,0x800DA298u);local_scope=0;
    projected=1;invoke(cpu,0x800DA298u);projected=0;
    psx_mod_write_byte(actor+4,2);invoke(cpu,0x800DA298u);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
}
static void dragoon_camera_release(CPUState *cpu) {
    (void)cpu;
    const uint32_t actor=0x8013BED0u;
    for(unsigned test=0;test<19;++test) {
        script_owner=-1;split=ready=1;local_scope=projected=actor_context=0;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
        psx_mod_write_byte(MMX4_PLAY+12,12);psx_mod_write_byte(MMX4_PLAY+13,0);
        psx_mod_write_byte(MMX4_PLAY+0x1D,2);
        for(unsigned i=0;i<0xFC;++i)psx_mod_write_byte(CAMERA+i,(uint8_t)(i+17));
        psx_mod_write_half(CAMERA+0x24,432);psx_mod_write_half(CAMERA+0x26,432);
        psx_mod_write_half(CAMERA+0x28,1531);psx_mod_write_half(CAMERA+0x2A,1531);
        psx_mod_write_byte(CAMERA+0x48,8);
        mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,65);
        psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+5,3);
        /* Original7E400/7E424/7E430 have completed, before owner restore. */
        psx_mod_write_half(CAMERA+0x26,272);psx_mod_write_byte(CAMERA+0x48,4);
        if(test==1)projected=actor_context=1;
        if(test==2)local_scope=1;
        if(test==3)split=0;
        if(test==4)ready=0;
        if(test==5)psx_mod_write_byte(MMX4_PLAY+12,4);
        if(test==6)psx_mod_write_byte(MMX4_PLAY+13,1);
        if(test==7)psx_mod_write_byte(MMX4_PLAY+0x1D,4);
        if(test==8)psx_mod_write_byte(actor+1,61);
        if(test==9)psx_mod_write_byte(actor+4,0);
        if(test==10)psx_mod_write_byte(actor+5,2);
        if(test==11)psx_mod_write_half(CAMERA+0x24,448);
        if(test==12)psx_mod_write_half(CAMERA+0x26,273);
        if(test==13)psx_mod_write_byte(CAMERA+0x48,8);
        if(test==14)psx_mod_write_half(saved_camera(1,0)+0x28,2048);
        if(test==15)psx_mod_write_half(saved_camera(1,0)+0x26,256);
        if(test==16)projected=1;
        if(test==17)psx_mod_write_byte(actor,0);
        if(test==18)psx_mod_write_word(0x801F0000u,0);
        uint8_t before[2][0xFC],canonical[0xFC];
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i)
            before[seat][i]=psx_mod_read_byte(saved_camera(seat,0)+i);
        for(unsigned i=0;i<0xFC;++i)canonical[i]=psx_mod_read_byte(CAMERA+i);
        mmx4_coop_split_dragoon_camera_release(actor);
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i) {
            unsigned admitted=test<2 || ((test==14 || test==15) && !seat);
            unsigned expected=before[seat][i];
            if(admitted && i==0x26)expected=0x10;
            if(admitted && i==0x27)expected=1;
            if(admitted && i==0x48)expected=4;
            CHECK(psx_mod_read_byte(saved_camera(seat,0)+i)==expected);
        }
        for(unsigned i=0;i<0xFC;++i)CHECK(psx_mod_read_byte(CAMERA+i)==canonical[i]);
        CHECK(mmx4_coop_split_snapshot_ready());
    }
    split=ready=1;local_scope=projected=actor_context=0;
}
static void destructible_camera_unlocks(CPUState *cpu) {
    uint32_t actor=0x80165A30u+3u*0x8Cu;
    uint8_t before[2][0xFC];
    script_owner=-1;split=ready=1;local_scope=projected=0;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,0);
    psx_mod_write_byte(MMX4_PLAY+13,0);psx_mod_write_byte(MMX4_PLAY+0x10,0);
    psx_mod_write_byte(MMX4_PLAY+0x1C,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;
    actor_position(MMX4_PLAYER,1367,427);second_position(3288,426);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t shadow=saved_camera(seat,0);
        psx_mod_write_half(shadow+0x1C,2976);psx_mod_write_half(shadow+0x24,2976);
        psx_mod_write_half(shadow+0x1E,(uint16_t)(seat*512));
        psx_mod_write_half(shadow+0x26,(uint16_t)(seat*768));
        psx_mod_write_half(shadow+10,(uint16_t)(seat?2976:1207));
        CHECK(mmx4_coop_split_camera_copy(seat,before[seat]));
    }
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,4);
    psx_mod_write_byte(actor+2,0);psx_mod_write_byte(actor+3,1);psx_mod_write_byte(actor+4,3);
    cpu->gpr[4]=actor;unsigned calls=destructible_calls;
    CHECK(invoke(cpu,0x800C0D98u)==0xABCD1234u);
    CHECK(destructible_calls==calls+1 && !psx_mod_read_byte(actor));
    CHECK(psx_mod_read_half(CAMERA+0x1C)==4544 && psx_mod_read_half(CAMERA+0x24)==4544);
    for(unsigned seat=0;seat<2;++seat) {
        uint8_t after[0xFC];CHECK(mmx4_coop_split_camera_copy(seat,after));
        CHECK(psx_mod_read_half(saved_camera(seat,0)+0x1C)==4544);
        CHECK(psx_mod_read_half(saved_camera(seat,0)+0x24)==4544);
        for(unsigned offset=0;offset<sizeof after;++offset)
            if(offset!=0x1C && offset!=0x1D && offset!=0x24 && offset!=0x25)
                CHECK(after[offset]==before[seat][offset]);
    }
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==1367 && second[10]==(3288&255) && second[11]==(3288>>8));
    /* 21158 dispatches the door pool before its 27850 camera tick. Saving
     * P1's ordinary camera and ticking P2 must retain the published release. */
    invoke(cpu,0x80027850u);mmx4_coop_split_camera(cpu,0);
    CHECK(psx_mod_read_half(saved_camera(0,0)+0x24)==4544);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x24)==4544);
    mmx4_coop_split_camera_prepare(1);
    CHECK(psx_mod_read_half(CAMERA+0x1C)==4544 && psx_mod_read_half(CAMERA+0x24)==4544);
    /* A surviving P2 still receives the shared opening. Other subtypes
     * finalize without importing canonical limits into either saved view. */
    psx_mod_write_byte(MMX4_PLAYER+4,2);psx_mod_write_byte(actor,1);
    psx_mod_write_half(saved_camera(1,0)+0x1C,2976);
    psx_mod_write_half(saved_camera(1,0)+0x24,2976);
    CHECK(invoke(cpu,0x800C0D98u)==0xABCD1234u);
    CHECK(destructible_calls==calls+2 && psx_mod_read_half(saved_camera(1,0)+0x24)==4544);
    psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+2,3);
    psx_mod_write_half(saved_camera(1,0)+0x1C,1024);
    psx_mod_write_half(saved_camera(1,0)+0x24,1024);
    CHECK(invoke(cpu,0x800C0D98u)==0xABCD1234u);
    CHECK(destructible_calls==calls+3 && psx_mod_read_half(saved_camera(1,0)+0x24)==1024);
    CHECK(mmx4_coop_split_snapshot_ready());
}
/* Native Generaid Core death writes current/target maximum X=2560. A
 * canonical-only store strands the second view at2240, clamping Zero2552.
 * Model those native stores even when the new hook is absent: the old code
 * must fail the actual saved-bound assertion, not merely lack a registration. */
static void owl_midboss_unlocks(CPUState *cpu) {
    const uint32_t actor=0x8013BED0u+5u*0x9Cu;
    for(unsigned test=0;test<15;++test) {
        script_owner=-1;split=ready=1;local_scope=projected=actor_context=0;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
        psx_mod_write_byte(MMX4_PLAY+12,7);psx_mod_write_byte(MMX4_PLAY+13,1);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        second[0]=second[4]=1;
        mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,70);
        psx_mod_write_byte(actor+4,2);psx_mod_write_byte(actor+0x5C,0);
        cpu->gpr[16]=actor;cpu->gpr[2]=2560;
        uint8_t before[2][0xFC];
        for(unsigned seat=0;seat<2;++seat) {
            uint32_t c=saved_camera(seat,0);
            for(unsigned i=0;i<0xFC;++i)psx_mod_write_byte(c+i,(uint8_t)(i+seat*7));
            psx_mod_write_half(c+0x1C,2240);psx_mod_write_half(c+0x24,2240);
            psx_mod_write_half(c+0x1E,2240);psx_mod_write_half(c+0x26,2240);
        }
        if(test==1)projected=actor_context=1; /* A P2-targeted native actor update. */
        if(test==2)split=0;
        if(test==3)ready=0;
        if(test==4)local_scope=1;
        if(test==5)psx_mod_write_byte(MMX4_PLAY+12,1);
        if(test==6)psx_mod_write_byte(MMX4_PLAY+13,0);
        if(test==7)psx_mod_write_byte(actor+1,60);
        if(test==8)psx_mod_write_byte(actor+4,1);
        if(test==9)psx_mod_write_byte(actor+0x5C,1);
        if(test==10)cpu->gpr[2]=2544;
        if(test==11)cpu->gpr[16]=actor+1;
        if(test==12)projected=1; /* Outside the shared actor dispatcher. */
        if(test==13)psx_mod_write_half(saved_camera(1,0)+0x24,1936);
        if(test==14)psx_mod_write_half(saved_camera(1,0)+0x26,0);
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i)before[seat][i]=psx_mod_read_byte(saved_camera(seat,0)+i);
        psx_mod_write_half(CAMERA+0x1C,2560); /* Native891FC already executed. */
        psx_mod_write_half(CAMERA+0x24,2240);
        if(owl_room_unlocked)owl_room_unlocked(cpu,0x80089204u);
        CHECK(psx_mod_read_half(CAMERA+0x24)==2240); /* Original SH remains responsible. */
        psx_mod_write_half(CAMERA+0x24,(uint16_t)cpu->gpr[2]);
        for(unsigned seat=0;seat<2;++seat) {
            unsigned released=test<2 || (test>=13 && seat==0);
            for(unsigned i=0;i<0xFC;++i) {
                unsigned expected=before[seat][i];
                if(released && (i==0x1C || i==0x24))expected=0;
                if(released && (i==0x1D || i==0x25))expected=10;
                CHECK(psx_mod_read_byte(saved_camera(seat,0)+i)==expected);
            }
        }
        CHECK(cpu->gpr[16]==(test==11?actor+1:actor));
        CHECK(cpu->gpr[2]==(test==10?2544:2560));
        CHECK(mmx4_coop_split_snapshot_ready());
    }
    projected=actor_context=local_scope=0;split=ready=1;
}
static void mushroom_midboss_unlocks(CPUState *cpu) {
    const uint32_t actor=0x8013BED0u+5u*0x9Cu;
    for(unsigned final=0;final<2;++final)for(unsigned test=0;test<14;++test) {
        script_owner=-1;split=ready=1;local_scope=projected=actor_context=0;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,3);
        psx_mod_write_byte(MMX4_PLAY+13,0);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        second[0]=second[4]=1;
        mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,33);
        psx_mod_write_byte(actor+4,final?3:2);psx_mod_write_byte(actor+0x5C,0);
        cpu->gpr[final?4:18]=actor;cpu->gpr[2]=final?4352:4500;
        for(unsigned seat=0;seat<2;++seat) {
            uint32_t c=saved_camera(seat,0);
            for(unsigned i=0;i<0xFC;++i)psx_mod_write_byte(c+i,(uint8_t)(i+seat*7));
            psx_mod_write_half(c+0x24,4608);psx_mod_write_half(c+0x26,4608);
        }
        if(test==1)projected=actor_context=1;
        if(test==2)split=0;
        if(test==3)ready=0;
        if(test==4)local_scope=1;
        if(test==5)psx_mod_write_byte(MMX4_PLAY+12,7);
        if(test==6)psx_mod_write_byte(MMX4_PLAY+13,1);
        if(test==7)psx_mod_write_byte(actor+1,70);
        if(test==8)psx_mod_write_byte(actor+4,1);
        if(test==9)psx_mod_write_byte(actor+0x5C,1);
        if(test==10)cpu->gpr[2]=4350;
        if(test==11)cpu->gpr[final?4:18]=actor+1;
        if(test==12)projected=1;
        if(test==13)psx_mod_write_half(saved_camera(1,0)+0x24,5312);
        uint8_t before[2][0xFC];
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i)
            before[seat][i]=psx_mod_read_byte(saved_camera(seat,0)+i);
        psx_mod_write_half(CAMERA+0x26,4608);
        if(mushroom_room_unlocked)mushroom_room_unlocked(cpu,final?0x8005D80Cu:0x8005D7C4u);
        CHECK(psx_mod_read_half(CAMERA+0x26)==4608);
        psx_mod_write_half(CAMERA+0x26,(uint16_t)cpu->gpr[2]); /* Original SH. */
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i) {
            unsigned expected=before[seat][i];
            if((test<2 || (test==13 && seat==0)) && (i==0x26 || i==0x27))
                expected=(cpu->gpr[2]>>(8u*(i-0x26u)))&255u;
            CHECK(psx_mod_read_byte(saved_camera(seat,0)+i)==expected);
        }
        CHECK(mmx4_coop_split_snapshot_ready());
    }
    projected=actor_context=local_scope=0;split=ready=1;
}
static void frost_shaft_unlocks(CPUState *cpu) {
    CHECK(frost_shaft_unlocked);
    for(unsigned test=0;test<8;++test) {
        split=ready=1;projected=local_scope=actor_context=0;script_owner=-1;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,2);
        psx_mod_write_byte(MMX4_PLAY+13,0);
        mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
        uint8_t before[2][0xFC];
        for(unsigned seat=0;seat<2;++seat) {
            uint32_t c=saved_camera(seat,0);
            for(unsigned i=0;i<0xFC;++i)psx_mod_write_byte(c+i,(uint8_t)(i+seat*11));
            psx_mod_write_half(c+0x1C,5312);psx_mod_write_half(c+0x24,5312);
        }
        psx_mod_write_half(CAMERA+0x24,5312);cpu->gpr[2]=5728;
        if(test==1)local_scope=1;
        if(test==2)split=0;
        if(test==3)ready=0;
        if(test==4)psx_mod_write_byte(MMX4_PLAY+13,1);
        if(test==5)cpu->gpr[2]=5888;
        if(test==6)psx_mod_write_half(saved_camera(1,0)+0x24,5888);
        if(test==7)psx_mod_write_half(saved_camera(1,0)+0x1C,5000);
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i)
            before[seat][i]=psx_mod_read_byte(saved_camera(seat,0)+i);
        frost_shaft_unlocked(cpu,0x800B8D64u);
        CHECK(psx_mod_read_half(CAMERA+0x24)==5312);
        for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i) {
            unsigned expected=before[seat][i];
            if((test==0 || (test>=6 && seat==0)) && (i==0x1C || i==0x24))expected=0x60;
            if((test==0 || (test>=6 && seat==0)) && (i==0x1D || i==0x25))expected=0x16;
            CHECK(psx_mod_read_byte(saved_camera(seat,0)+i)==expected);
        }
    }
    split=ready=1;projected=local_scope=0;
}
static void frost_shaft_follow(CPUState *cpu) {
    static const uint16_t thresholds[]={1888,2976,5104,5888,6005};
    for(unsigned i=0;i<5;++i)psx_mod_write_half(0x8010B6C4u+i*2u,thresholds[i]);
    split=ready=1;local_scope=projected=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,2);
    psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;
    uint32_t controller=0x80142F98u;
    psx_mod_write_byte(controller+1,15);psx_mod_write_byte(controller+0x14,0);
    actor_position(MMX4_PLAYER,5500,1420);second_position(5000,1480);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    uint8_t before[2][0xFC];
    for(unsigned seat=0;seat<2;++seat) {
        psx_mod_write_half(saved_camera(seat,0)+0x2E,192);
        for(unsigned i=0;i<0xFC;++i)before[seat][i]=psx_mod_read_byte(saved_camera(seat,0)+i);
    }
    cpu->gpr[4]=controller;frost_camera_calls=0;invoke(cpu,0x800B8AF8u);
    CHECK(frost_camera_calls==1 && frost_leader_x==5500);
    CHECK(psx_mod_read_byte(controller+0x14)==1);
    for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<0xFC;++i)
        CHECK(psx_mod_read_byte(saved_camera(seat,0)+i)==
            (seat==0 && i==0x2E?112u:before[seat][i]));
    /* The survivor's position selects the shared phase without moving P1.
     * Both independent views then receive their own native follow setting. */
    psx_mod_write_byte(MMX4_PLAYER+4,3);second_position(5600,1420);
    invoke(cpu,0x800B8AF8u);
    CHECK(frost_camera_calls==2 && frost_leader_x==5600);
    CHECK(psx_mod_read_byte(controller+0x14)==2 && !projected);
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==5500);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x2E)==112);
    /* Passing the shaft restores the ordinary192px bottom dead zone. */
    second_position(5900,1450);invoke(cpu,0x800B8AF8u);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x2E)==192);
    psx_mod_write_byte(MMX4_PLAYER+4,1);
    actor_position(MMX4_PLAYER,5000,1420);second_position(5500,1420);
    invoke(cpu,0x800B8AF8u);
    CHECK(frost_camera_calls==4 && frost_leader_x==5500);
    CHECK(psx_mod_read_half(saved_camera(0,0)+0x2E)==192);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x2E)==112);
    CHECK(psx_mod_read_half(CAMERA+0x2E)==192);
    CHECK(mmx4_coop_split_snapshot_ready());
}
static void four_aligned_placements(CPUState *cpu) {
    uint32_t table=native_table=0x800F848Cu;
    split=ready=1;local_scope=projected=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,11);
    psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;
    actor_position(MMX4_PLAYER,360,459);second_position(1000,400);
    memset(ram+(CAMERA&0x1FFFFFu),0,0xFCu);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t view=saved_camera(seat,0);
        psx_mod_write_half(view+10,(uint16_t)(seat?840:40));
        psx_mod_write_half(view+0x16,(uint16_t)(seat?840:40));
        psx_mod_write_half(view+14,256);psx_mod_write_half(view+0x1A,256);
    }
    memset(ram+(table&0x1FFFFFu),0,9u*8u);
    psx_mod_write_word(0x800F43C8u+11u*8u,table);
    for(unsigned offset=1;offset<=4;++offset)
        psx_mod_write_half(0x800F4332u+offset*2u,(uint16_t)(offset*32u));
    /* The real table is four-byte aligned. First use ordinary unflagged
     * records to prove table admission independently of scan offsets. */
    placement(0,944,398,0);placement(1,944,338,0);placement(2,1108,410,0);
    for(unsigned index=0;index<3;++index) {
        psx_mod_write_byte(table+index*8u+1,index==2?51:58);
    }
    /* Other offsets and script/destructible categories keep native admission. */
    placement(3,1024,400,0);psx_mod_write_byte(table+24,0x10);
    placement(4,1024,400,0);psx_mod_write_byte(table+32,0x30);
    /* Keep this negative inside the supplemental view, outside the native
     * first strip shifted by128px: (998,1030). */
    placement(5,992,400,0);psx_mod_write_byte(table+40,0x40);
    placement(6,1024,400,3);psx_mod_write_byte(table+48,0x20);
    placement(7,1024,400,4);psx_mod_write_byte(table+56,0x20);
    psx_mod_write_byte(table+57,4);
    psx_mod_write_byte(table+8u*8u+3u,255);
    native_directional_scans=1;allocations=0;capacity=16;
    invoke(cpu,0x80028E24u);CHECK(allocations==3);
    /* Actual records from800F84CC/84D4/8514 carry20, a 64px scan offset.
     * A stationary P1 cannot reach them; only P2's native moving strips do.
     * Keep the full-view ban on all offset records and scripted categories. */
    for(unsigned index=0;index<3;++index)psx_mod_write_byte(table+index*8u,0x20);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    psx_mod_write_half(saved_camera(0,0)+10,40);
    psx_mod_write_half(saved_camera(0,0)+0x16,40);
    psx_mod_write_half(saved_camera(1,0)+10,534);
    psx_mod_write_half(saved_camera(1,0)+0x16,526);
    for(unsigned seat=0;seat<2;++seat) {
        psx_mod_write_half(saved_camera(seat,0)+14,256);
        psx_mod_write_half(saved_camera(seat,0)+0x1A,256);
    }
    native_directional_scans=1;allocations=0;capacity=0;
    invoke(cpu,0x80028E24u);CHECK(!allocations);
    for(unsigned index=0;index<8;++index)
        CHECK(!(psx_mod_read_byte(table+index*8u)&1u));
    capacity=16;invoke(cpu,0x80028E24u);CHECK(allocations==2);
    psx_mod_write_half(saved_camera(1,0)+10,698);
    psx_mod_write_half(saved_camera(1,0)+0x16,690);
    invoke(cpu,0x80028E24u);CHECK(allocations==3);
    for(unsigned index=0;index<8;++index)
        CHECK((psx_mod_read_byte(table+index*8u)&1u)==(index<3));
    invoke(cpu,0x80028E24u);CHECK(allocations==3);
    /* Native retirement clears the live placement flag. The shared visit
     * latch prevents a second copy when P1 reaches the partner's region. */
    actor_position(MMX4_PLAYER,1000,400);
    for(unsigned seat=0;seat<2;++seat) {
        psx_mod_write_half(saved_camera(seat,0)+10,840);
        psx_mod_write_half(saved_camera(seat,0)+0x16,832);
    }
    invoke(cpu,0x80028E24u);CHECK(allocations==3);
    for(unsigned index=0;index<3;++index)psx_mod_write_byte(table+index*8u,0x20);
    invoke(cpu,0x80028E24u);CHECK(allocations==3);
    for(unsigned seat=0;seat<2;++seat) {
        psx_mod_write_half(saved_camera(seat,0)+10,3000);
        psx_mod_write_half(saved_camera(seat,0)+0x16,3000);
    }
    invoke(cpu,0x80028E24u);CHECK(allocations==3);
    psx_mod_write_half(saved_camera(1,0)+10,534);
    psx_mod_write_half(saved_camera(1,0)+0x16,526);
    invoke(cpu,0x80028E24u);CHECK(allocations==5);
    psx_mod_write_half(saved_camera(1,0)+10,698);
    psx_mod_write_half(saved_camera(1,0)+0x16,690);
    invoke(cpu,0x80028E24u);CHECK(allocations==6);
    /* A subsequent native movement strip still admits the unchanged10
     * offset normally. It is excluded only from the supplemental scan. */
    psx_mod_write_half(saved_camera(1,0)+10,640);
    psx_mod_write_half(saved_camera(1,0)+0x16,632);
    invoke(cpu,0x80028E24u);CHECK(allocations==7);
    CHECK(psx_mod_read_byte(table+24)==0x11 && !scan_direction);
    native_directional_scans=0;native_table=TABLE;
}
static void stationary_second_door(CPUState *cpu) {
    /* Cyber Peacock's real paired-door handoff fixes the camera at688.
     * The second door at992 is already inside that view, outside all moving
     * edge strips. Its native category4/type8 placement must still spawn. */
    uint32_t table=native_table=0x800F7480u;
    split=ready=1;local_scope=projected=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,6);
    psx_mod_write_byte(MMX4_PLAY+13,1);
    psx_mod_write_byte(MMX4_PLAY+0x10,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;
    actor_position(MMX4_PLAYER,900,2507);second_position(870,2506);
    memset(ram+(CAMERA&0x1FFFFFu),0,0xFCu);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t view=saved_camera(seat,0);
        psx_mod_write_half(view+10,688);psx_mod_write_half(view+0x16,688);
        psx_mod_write_half(view+14,2304);psx_mod_write_half(view+0x1A,2304);
    }
    memset(ram+(table&0x1FFFFFu),0,7u*8u);
    psx_mod_write_word(0x800F43C8u+6u*8u+4u,table);
    placement(0,688,2496,4);psx_mod_write_byte(table,1);psx_mod_write_byte(table+1,8);
    placement(1,992,2496,4);psx_mod_write_byte(table+9,8);psx_mod_write_byte(table+10,4);
    /* Do not extend full-view admission to switches, script regions, or
     * placements with authored movement-direction offsets. */
    placement(2,992,2496,4);psx_mod_write_byte(table+17,20);
    placement(3,992,2496,3);
    placement(4,992,2496,4);psx_mod_write_byte(table+32,0x20);psx_mod_write_byte(table+33,8);
    placement(5,1300,2496,4);psx_mod_write_byte(table+41,8);
    psx_mod_write_byte(table+6u*8u+3u,255);
    native_directional_scans=1;allocations=capacity=0;
    invoke(cpu,0x80028E24u);CHECK(!allocations && !psx_mod_read_byte(table+8));
    capacity=16;invoke(cpu,0x80028E24u);CHECK(allocations==1);
    CHECK(psx_mod_read_byte(table+8)&1u);
    for(unsigned i=2;i<6;++i)CHECK(!(psx_mod_read_byte(table+i*8u)&1u));
    invoke(cpu,0x80028E24u);CHECK(allocations==1); /* Both views, one actor. */
    native_directional_scans=0;native_table=TABLE;
}
static void stationary_refight_portal(CPUState *cpu) {
    for(unsigned scope=0;scope<3;++scope) {
        uint32_t table=native_table=0x800F87E0u;
        split=ready=1;local_scope=projected=0;script_owner=-1;
        unsigned stage=scope==2?6u:12u,section=scope==0?0u:1u;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,(uint8_t)stage);
        psx_mod_write_byte(MMX4_PLAY+13,(uint8_t)section);
        psx_mod_write_byte(MMX4_PLAY+0x10,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        second[0]=second[4]=1;
        actor_position(MMX4_PLAYER,80,2507);second_position(870,2506);
        memset(ram+(CAMERA&0x1FFFFFu),0,0xFCu);
        mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
        for(unsigned seat=0;seat<2;++seat) {
            uint32_t view=saved_camera(seat,0);unsigned x=seat?688u:0u;
            psx_mod_write_half(view+10,(uint16_t)x);psx_mod_write_half(view+0x16,(uint16_t)x);
            psx_mod_write_half(view+14,2304);psx_mod_write_half(view+0x1A,2304);
        }
        memset(ram+(table&0x1FFFFFu),0,3u*8u);
        psx_mod_write_word(0x800F43C8u+stage*8u+section*4u,table);
        placement(0,992,2496,4);psx_mod_write_byte(table+1,27);
        placement(1,992,2496,4);psx_mod_write_byte(table+8,0x20);psx_mod_write_byte(table+9,27);
        psx_mod_write_byte(table+19,255);
        native_directional_scans=1;allocations=0;capacity=16;
        invoke(cpu,0x80028E24u);CHECK(allocations==(scope==0));
        invoke(cpu,0x80028E24u);CHECK(allocations==(scope==0));
        CHECK(!(psx_mod_read_byte(table+8)&1u)); /* Authored directional offset remains native. */
    }
    native_directional_scans=0;native_table=TABLE;
}
static uint32_t train_pool_tick(CPUState *cpu) {
    /* QA87 observed D9218 with a0=80169CB8, ra=80021BDC. Original21B34
     * dispatches four30-byte actors via800F2AB4 AFTER27850/27D40/281E8. */
    CPUState saved=*cpu;uint32_t result=0;
    for(unsigned slot=0;slot<4;++slot) {
        uint32_t actor=0x80169CB8u+slot*0x30u;
        if(!psx_mod_read_byte(actor))continue;
        cpu->gpr[4]=actor;
        unsigned kind=psx_mod_read_byte(actor+1);
        CHECK(kind==0 || kind==2);
        result=invoke(cpu,kind==0?0x800D8648u:0x800D9218u);
    }
    *cpu=saved;return result;
}
static void train_backgrounds(CPUState *cpu) {
    uint8_t view[0xFC];uint32_t actor=0x80169CB8u;
    script_owner=-1;split=ready=1;local_scope=projected=0;
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;actor_position(MMX4_PLAYER,9472,395);second_position(9072,395);
    psx_mod_write_byte(MMX4_PLAY+12,8);psx_mod_write_byte(MMX4_PLAY+13,1);
    memset(ram+(CAMERA&0x1FFFFFu),0,0xFCu);
    psx_mod_write_word(CAMERA+8,9392u<<16);psx_mod_write_word(CAMERA+20,9392u<<16);
    psx_mod_write_word(CAMERA+12,208u<<16);psx_mod_write_word(CAMERA+24,208u<<16);
    for(unsigned layer=1;layer<3;++layer)psx_mod_write_byte(CAMERA+layer*0x54u+4,5);
    psx_mod_write_word(CAMERA+0x54u+8,3408u<<16);
    psx_mod_write_word(CAMERA+0x54u+12,256u<<16);
    psx_mod_write_word(CAMERA+0xA8u+8,1448u<<16);
    psx_mod_write_word(CAMERA+0xA8u+12,256u<<16);
    mmx4_coop_split_reset();mmx4_coop_split_camera_prepare(0);
    psx_mod_write_word(saved_camera(1,0)+8,8912u<<16);
    psx_mod_write_word(saved_camera(1,0)+20,8912u<<16);
    /* The real QA79 train loop advances 8/4 px while the boss FG is fixed.
     * Each pane can start with distinct scenery and foreground origins. */
    psx_mod_write_word(saved_camera(1,1)+8,3500u<<16);
    psx_mod_write_word(saved_camera(1,2)+8,1500u<<16);
    /* Exact QA87 body: base slot, live kind2, D9218's ordinary boss loop. */
    psx_mod_write_byte(actor,0x81);psx_mod_write_byte(actor+1,2);psx_mod_write_byte(actor+4,1);
    psx_mod_write_byte(actor+0x14,2);psx_mod_write_byte(actor+5,5);
    psx_mod_write_word(actor+0x18,0x80000u);cpu->gpr[4]=actor;
    /* The current native camera snapshots precede the four-slot world pool. */
    invoke(cpu,0x80027850u);mmx4_coop_split_camera(cpu,0);
    uint32_t fg0=psx_mod_read_word(saved_camera(0,0)+8);
    uint32_t fg1=psx_mod_read_word(saved_camera(1,0)+8);
    unsigned calls=train_calls,initial_foreground_calls=train_foreground_calls;
    CHECK(train_pool_tick(cpu)==0x12345678u);
    CHECK(train_calls==calls+1);
    CHECK(train_foreground_calls==initial_foreground_calls); /* Absent in QA87. */
    CHECK(psx_mod_read_word(CAMERA+0x54u+8)==3416u<<16);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==3416u<<16);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==3508u<<16);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==1452u<<16);
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==1504u<<16);
    CHECK(psx_mod_read_word(saved_camera(0,0)+8)==fg0);
    CHECK(psx_mod_read_word(saved_camera(1,0)+8)==fg1);
    CHECK(psx_mod_read_word(saved_camera(1,1)+12)==256u<<16);
    /* The next frame's camera snapshots preserve the preceding pool's delta,
     * then one new world pass advances both backgrounds by another8/4px. */
    invoke(cpu,0x80027850u);mmx4_coop_split_camera(cpu,0);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==3416u<<16);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==3508u<<16);
    train_pool_tick(cpu);CHECK(train_calls==calls+2);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==3424u<<16);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==3516u<<16);
    /* Native signed arithmetic half includes negative odd fixed units and
     * uint32 camera wrap, without a second transform on the canonical view. */
    psx_mod_write_word(actor+0x18,0xFFFFFFFDu);
    uint32_t first_x=psx_mod_read_word(saved_camera(0,1)+8);
    uint32_t second_x=psx_mod_read_word(saved_camera(1,1)+8);
    uint32_t second_layer=psx_mod_read_word(saved_camera(1,2)+8);
    train_pool_tick(cpu);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==first_x-3u);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==second_x-3u);
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==second_layer-2u);
    /* Canonical owner changes start from that owner's scrolling background. */
    mmx4_coop_split_camera_prepare(1);
    CHECK(psx_mod_read_word(CAMERA+0x54u+8)==second_x-3u);
    psx_mod_write_word(CAMERA+20,psx_mod_read_word(CAMERA+8));
    mmx4_coop_split_camera(cpu,1);
    psx_mod_write_byte(actor+1,2);psx_mod_write_byte(actor+0x14,2);
    psx_mod_write_byte(actor+5,5);psx_mod_write_word(actor+0x18,0x80000u);
    second_x=psx_mod_read_word(saved_camera(1,1)+8);
    invoke(cpu,0x800D9218u);
    CHECK(psx_mod_read_word(CAMERA+0x54u+8)==second_x+0x80000u);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==second_x+0x80000u);
    /* Earlier type2 phase1 adds each pane's own current-minus-previous FG
     * movement, with sra (floor), including opposite odd fractional motion. */
    actor_position(MMX4_PLAYER,7000,395);
    psx_mod_write_byte(actor+0x14,1);psx_mod_write_byte(actor+5,1);
    psx_mod_write_byte(actor+6,2);psx_mod_write_byte(actor+7,1);
    psx_mod_write_word(CAMERA+8,0x03E80003u);psx_mod_write_word(CAMERA+20,0x03E80000u);
    psx_mod_write_word(saved_camera(1,0)+8,0x03E80003u);
    psx_mod_write_word(saved_camera(1,0)+20,0x03E80000u);
    psx_mod_write_word(saved_camera(0,0)+8,0x06400000u);
    psx_mod_write_word(saved_camera(0,0)+20,0x06400003u);
    first_x=psx_mod_read_word(saved_camera(0,1)+8);
    second_x=psx_mod_read_word(saved_camera(1,1)+8);
    uint32_t first_far=psx_mod_read_word(saved_camera(0,2)+8);
    uint32_t second_far=psx_mod_read_word(saved_camera(1,2)+8);
    unsigned foreground_calls=train_foreground_calls;calls=train_calls;
    invoke(cpu,0x800D9218u);
    CHECK(train_calls==calls+1 && train_foreground_calls==foreground_calls+1);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==second_x+0x80003u);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==first_x+0x7FFFDu);
    CHECK(psx_mod_read_word(saved_camera(1,2)+8)==second_far+0x40001u);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==first_far+0x3FFFEu);
    CHECK(psx_mod_read_word(CAMERA+0x54u+8)==second_x+0x80003u);
    CHECK(psx_mod_read_word(saved_camera(0,0)+8)==0x06400000u);
    CHECK(psx_mod_read_word(saved_camera(1,0)+8)==0x03E80003u);
    /* BC freeze excludes foreground contribution, while world speed remains. */
    psx_mod_write_byte(MMX4_PLAYER+0xBC,1);
    first_x=psx_mod_read_word(saved_camera(0,1)+8);
    second_x=psx_mod_read_word(saved_camera(1,1)+8);
    invoke(cpu,0x800D9218u);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==first_x+0x80000u);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==second_x+0x80000u);
    psx_mod_write_byte(MMX4_PLAYER+0xBC,0);
    /* +7 transitions from0 to1 only after stopping. That tick adds no
     * foreground motion despite the resulting +7, but shares authored limits. */
    psx_mod_write_byte(actor+7,0);psx_mod_write_word(actor+0x18,0);
    first_x=psx_mod_read_word(saved_camera(0,1)+8);
    second_x=psx_mod_read_word(saved_camera(1,1)+8);
    invoke(cpu,0x800D9218u);CHECK(psx_mod_read_byte(actor+7)==1);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==first_x);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==second_x);
    CHECK(psx_mod_read_half(saved_camera(0,0)+0x1C)==9440);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x24)==9440);
    /* Crossing a region resets stale phase1 flags before native dispatch. */
    psx_mod_write_word(actor+0x18,0x80000u);actor_position(MMX4_PLAYER,9472,395);
    first_x=psx_mod_read_word(saved_camera(0,1)+8);foreground_calls=train_foreground_calls;
    invoke(cpu,0x800D9218u);
    CHECK(train_foreground_calls==foreground_calls);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==first_x+0x80000u);
    /* Local draw, player projection, cleanup, wrong kind, and another stage
     * never apply a shared delta to private camera state. */
    uint32_t digest=mmx4_coop_split_digest(2166136261u);
    local_scope=1;invoke(cpu,0x800D9218u);local_scope=0;
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    projected=1;invoke(cpu,0x800D9218u);projected=0;
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(actor+4,2);invoke(cpu,0x800D9218u);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(actor+4,1);psx_mod_write_byte(actor+1,0);
    invoke(cpu,0x800D9218u);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    psx_mod_write_byte(actor+1,2);psx_mod_write_byte(MMX4_PLAY+12,1);
    invoke(cpu,0x800D9218u);
    CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    /* A native scene shares complete cameras, retaining one shared actor pass. */
    psx_mod_write_byte(MMX4_PLAY+12,8);script_owner=1;mmx4_coop_split_camera(cpu,1);
    invoke(cpu,0x800D9218u);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==psx_mod_read_word(saved_camera(1,1)+8));
    script_owner=-1;
    /* First update after section reset initializes both before the native
     * call: initialization metadata/8px delta are applied exactly once. */
    psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_word(CAMERA+0x54u+8,0xFFFFFFFEu);
    psx_mod_write_word(CAMERA+0xA8u+8,0x00000002u);
    psx_mod_write_word(CAMERA+20,psx_mod_read_word(CAMERA+8));
    psx_mod_write_byte(actor+1,0);psx_mod_write_byte(actor+4,0);
    mmx4_coop_split_reset();calls=train_calls;invoke(cpu,0x800D8648u);
    CHECK(train_calls==calls+1);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==0x0007FFFEu);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==0x0007FFFEu);
    CHECK(psx_mod_read_word(saved_camera(0,2)+8)==0x00040002u);
    CHECK(psx_mod_read_word(CAMERA+0x54u+8)==0x0007FFFEu);
    CHECK(psx_mod_read_byte(saved_camera(0,1)+0x4E)==7);
    CHECK(psx_mod_read_byte(saved_camera(1,2)+0x4E)==5);
    CHECK(psx_mod_read_half(saved_camera(0,0)+0x2C)==48);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x2E)==160);
    CHECK(psx_mod_read_word(saved_camera(0,0)+8)==psx_mod_read_word(CAMERA+8));
    CHECK(psx_mod_read_word(saved_camera(1,0)+8)==psx_mod_read_word(CAMERA+8));
    /* Exercise the last actual four-slot scenery body via original dispatch.
     * Neither earlier20/32 pool nor a wrong end address satisfies this. */
    uint32_t last=0x80169D48u;
    psx_mod_write_byte(actor,0);
    psx_mod_write_byte(last,0x81);psx_mod_write_byte(last+1,0);
    psx_mod_write_byte(last+4,1);psx_mod_write_word(last+0x18,0x80000u);
    first_x=psx_mod_read_word(saved_camera(0,1)+8);
    second_x=psx_mod_read_word(saved_camera(1,1)+8);calls=train_calls;
    train_pool_tick(cpu);CHECK(train_calls==calls+1);
    CHECK(psx_mod_read_word(saved_camera(0,1)+8)==first_x+0x80000u);
    CHECK(psx_mod_read_word(saved_camera(1,1)+8)==second_x+0x80000u);
    psx_mod_write_byte(last,0);
    /* Reject the old empty pool, both bounds, and a misaligned body even
     * when each has matching kind/state/speed and original camera writes. */
    const uint32_t outside[]={0x801410C0u,0x801435B0u,0x80169C88u,0x80169D78u,0x80169D49u};
    digest=mmx4_coop_split_digest(2166136261u);
    for(unsigned index=0;index<sizeof outside/sizeof outside[0];++index) {
        cpu->gpr[4]=outside[index];
        psx_mod_write_byte(outside[index]+1,0);psx_mod_write_byte(outside[index]+4,1);
        psx_mod_write_word(outside[index]+0x18,0x80000u);calls=train_calls;
        invoke(cpu,0x800D8648u);CHECK(train_calls==calls+1);
        CHECK(mmx4_coop_split_digest(2166136261u)==digest);
    }
    cpu->gpr[4]=actor;psx_mod_write_byte(actor,0x81);
    /* Native local8:3 reproduces a frozen canonical/P1 layer while P2's
     * private layer advances. The P2 trigger-owner transaction must retain
     * the world pool's delta across the next P1 camera snapshot, once only. */
    psx_mod_write_byte(actor+1,0);psx_mod_write_byte(actor+4,1);
    psx_mod_write_word(actor+0x18,0x80000u);
    mmx4_coop_split_camera_prepare(0);
    for(unsigned owner=0;owner<2;++owner) {
        train_delegate=owner+1u;
        for(unsigned tick=0;tick<3;++tick) {
            uint32_t origins[2][2];
            for(unsigned seat=0;seat<2;++seat)for(unsigned layer=1;layer<3;++layer)
                origins[seat][layer-1]=psx_mod_read_word(saved_camera(seat,layer)+8);
            uint32_t fg=psx_mod_read_word(CAMERA+8);calls=train_calls;
            invoke(cpu,0x800D8648u);CHECK(train_calls==calls+1);
            CHECK(psx_mod_read_word(CAMERA+8)==fg);
            for(unsigned seat=0;seat<2;++seat)for(unsigned layer=1;layer<3;++layer)
                CHECK(psx_mod_read_word(saved_camera(seat,layer)+8)==
                    origins[seat][layer-1]+(0x80000u>>(layer-1)));
            CHECK(psx_mod_read_word(CAMERA+0x54u+8)==psx_mod_read_word(saved_camera(0,1)+8));
            CHECK(psx_mod_read_word(CAMERA+0xA8u+8)==psx_mod_read_word(saved_camera(0,2)+8));
            invoke(cpu,0x80027850u);mmx4_coop_split_camera(cpu,0);
            CHECK(psx_mod_read_word(saved_camera(0,1)+8)==origins[0][0]+0x80000u);
        }
    }
    train_delegate=0;
    /* Unified calls the original once and allocates no private camera state. */
    split=0;CHECK(mmx4_coop_split_activate());
    calls=train_calls;invoke(cpu,0x800D8648u);CHECK(train_calls==calls+1);
    CHECK(!mmx4_coop_split_camera_copy(1,view));
    psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,0);
}
static void train_panorama_cycles(CPUState *cpu) {
    const uint32_t actor=0x80169CB8u;
    for(unsigned owner=0;owner<2;++owner) {
        split=ready=1;projected=local_scope=0;script_owner=-1;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
        psx_mod_write_byte(MMX4_PLAY+12,8);psx_mod_write_byte(MMX4_PLAY+13,0);
        psx_mod_write_byte(MMX4_PLAY+0x10,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        second[0]=second[4]=1;psx_mod_write_byte(0x80172224u,32);
        memset(ram+(CAMERA&0x1FFFFFu),0,0xFCu);
        for(unsigned layer=1;layer<3;++layer) {
            uint32_t c=CAMERA+layer*0x54u;
            psx_mod_write_byte(c+4,5);psx_mod_write_byte(c+0x4D,2);
            psx_mod_write_byte(c+0x4E,layer==1?7:5);
        }
        CHECK(mmx4_coop_split_activate());mmx4_coop_split_camera_prepare(0);
        uint8_t before[2][0xFC];
        unsigned initial[2][2]={{2000,1504},{1936,1440}};
        for(unsigned seat=0;seat<2;++seat) {
            for(unsigned layer=1;layer<3;++layer)
                psx_mod_write_word(saved_camera(seat,layer)+8,
                    (initial[seat][layer-1]<<16)|0x1234u);
            for(unsigned i=0;i<0xFC;++i)before[seat][i]=psx_mod_read_byte(saved_camera(seat,0)+i);
        }
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,0);psx_mod_write_byte(actor+4,1);
        psx_mod_write_word(actor+0x18,8u<<16);cpu->gpr[4]=actor;
        train_delegate=owner+1u;
        /* Several authored panorama cycles, with distinct saved origins.
         * Rendering alone cannot retain these wraps in the private view.
         * The expected period comes from the native inclusive screen range. */
        for(unsigned tick=0;tick<600;++tick) {
            unsigned calls=train_calls;invoke(cpu,0x800D8648u);CHECK(train_calls==calls+1);
            for(unsigned seat=0;seat<2;++seat) {
                for(unsigned layer=1;layer<3;++layer) {
                    unsigned speed=layer==1?8u:4u,period=layer==1?1536u:1024u;
                    unsigned x=512u+(initial[seat][layer-1]-512u+(tick+1u)*speed)%period;
                    if(psx_mod_read_word(saved_camera(seat,layer)+8)!=(x<<16|0x1234u))
                        fprintf(stderr,"train wrap owner%u seat%u layer%u tick%u: %08x expected%08x\n",
                            owner,seat,layer,tick,psx_mod_read_word(saved_camera(seat,layer)+8),x<<16|0x1234u);
                    CHECK(psx_mod_read_word(saved_camera(seat,layer)+8)==(x<<16|0x1234u));
                }
                for(unsigned i=0;i<0xFC;++i) {
                    if((i>=0x54u+8u && i<0x54u+12u) ||
                       (i>=0xA8u+8u && i<0xA8u+12u))continue;
                    CHECK(psx_mod_read_byte(saved_camera(seat,0)+i)==before[seat][i]);
                }
            }
        }
    }
    train_delegate=0;psx_mod_write_byte(0x80172224u,0);
}
static void frost_ice_ranges(CPUState *cpu) {
    mmx4_coop_split_reset();split=ready=1;projected=local_scope=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,2);
    psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    psx_mod_write_byte(MMX4_PLAYER+0x14,0);second[0]=second[4]=1;second[0x14]=0;
    mmx4_coop_split_camera_prepare(0);
    actor_position(saved_camera(0,0),0,0);actor_position(saved_camera(1,0),2000,900);
    const uint32_t point=0x800F5000u;cpu->gpr[4]=point;
    static const int points[][3]={
        {2600,1000,1},{1360,420,1},{2959,1619,1},{2960,900,0},
        {1359,900,0},{2000,419,0},{2000,1620,0},{100,100,1},{4000,2000,0}
    };
    for(unsigned i=0;i<sizeof points/sizeof points[0];++i) {
        psx_mod_write_half(point,(uint16_t)points[i][0]);
        psx_mod_write_half(point+2,(uint16_t)points[i][1]);
        CHECK(invoke(cpu,0x800B9E54u)==(unsigned)points[i][2]);
    }
    psx_mod_write_half(point,2600);psx_mod_write_half(point+2,1000);
    script_owner=0;CHECK(!invoke(cpu,0x800B9E54u));
    script_owner=1;CHECK(invoke(cpu,0x800B9E54u));script_owner=-1;
    second[4]=2;CHECK(!invoke(cpu,0x800B9E54u));second[4]=1;
    psx_mod_write_byte(MMX4_PLAYER+4,2);CHECK(invoke(cpu,0x800B9E54u));
    second[4]=2;CHECK(!invoke(cpu,0x800B9E54u));second[4]=1;
    second[0x14]=1;actor_position(saved_camera(1,1),4000,1800);
    CHECK(!invoke(cpu,0x800B9E54u));
    psx_mod_write_half(point,4600);psx_mod_write_half(point+2,1900);
    uint8_t cameras[0xFC];memcpy(cameras,ram+(CAMERA&0x1FFFFFu),sizeof cameras);
    CHECK(invoke(cpu,0x800B9E54u));
    CHECK(!memcmp(cameras,ram+(CAMERA&0x1FFFFFu),sizeof cameras) && !projected);
    CHECK(mmx4_coop_split_snapshot_ready());
}
static void local_explosion_flash(CPUState *cpu) {
    uint8_t camera[0xFC]={0};
    const uint32_t actor=0x80142F98u;
    for(unsigned test=0;test<17;++test) {
        memset(ram+(actor&0x1FFFFFu),0,32*0x30);
        split=ready=local_scope=1;projected=0;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
        psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,(uint8_t)(test==1));
        psx_mod_write_byte(MMX4_PLAY+0x0F,0);psx_mod_write_byte(MMX4_PLAY+0x10,0);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,28);psx_mod_write_byte(actor+4,1);
        psx_mod_write_half(actor+10,4000);psx_mod_write_half(actor+14,128);
        psx_mod_write_half(0x80175EA0u,31);psx_mod_write_byte(0x80175E9Cu,0);
        psx_mod_write_half(0x80141BE6u,0);psx_mod_write_half(0x8013E1BEu,0);
        if(test==2)psx_mod_write_half(actor+10,160); /* Same viewport. */
        if(test==3)local_scope=0;
        if(test==4)split=0;
        if(test==5)ready=0;
        if(test==6)psx_mod_write_byte(MMX4_PLAY+12,1);
        if(test==7)psx_mod_write_byte(actor,0);
        if(test==8)psx_mod_write_byte(actor+1,36);
        if(test==9)psx_mod_write_byte(actor+4,2);
        if(test==10)psx_mod_write_half(0x80141BE6u,0x3E0); /* White flash. */
        if(test==11)psx_mod_write_byte(MMX4_PLAY+0x0F,1);
        if(test==12)psx_mod_write_byte(MMX4_PLAY+0x10,1);
        if(test==13)psx_mod_write_byte(0x80175E9Cu,1); /* Subtractive fade. */
        if(test==14)projected=1;
        if(test==15)psx_mod_write_byte(MMX4_PLAY+1,1);
        if(test==16) { /* A second nearby explosion must retain the flash. */
            psx_mod_write_byte(actor+0x30,1);psx_mod_write_byte(actor+0x31,28);
            psx_mod_write_byte(actor+0x34,1);psx_mod_write_half(actor+0x3A,160);
            psx_mod_write_half(actor+0x3E,128);
        }
        CPUState before=*cpu;palette_rebuilds=palette_uploads=0;
        mmx4_coop_split_local_explosion_palette(cpu,camera);
        CHECK(palette_rebuilds==(test<2) && palette_uploads==(test<2));
        CHECK(psx_mod_read_half(0x80175EA0u)==(test<2?0:31));
        CHECK(!memcmp(cpu,&before,sizeof before));
    }
    /* The small looping red pulse uses the native camera rather than the
     * palette controller's stale allocation coordinates. Preserve other
     * colors, palette programs, larger red effects and overlapping views. */
    for(unsigned test=0;test<6;++test) {
        memset(ram+(actor&0x1FFFFFu),0,32*0x30);
        split=ready=local_scope=1;projected=0;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
        psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,1);
        psx_mod_write_byte(MMX4_PLAY+0x0F,0);psx_mod_write_byte(MMX4_PLAY+0x10,0);
        psx_mod_write_byte(actor,1);psx_mod_write_byte(actor+1,2);psx_mod_write_byte(actor+4,1);
        psx_mod_write_byte(actor+2,(uint8_t)(test==2));
        psx_mod_write_half(0x80175EA0u,(uint16_t)(test==3?4:3));
        psx_mod_write_byte(0x80175E9Cu,0);psx_mod_write_half(0x80141BE6u,0);
        psx_mod_write_half(0x8013E1BEu,(uint16_t)(test==4?1024:0));
        psx_mod_write_half(CAMERA+10,(uint16_t)(test==1?0:3840));psx_mod_write_half(CAMERA+14,0);
        if(test==5)psx_mod_write_byte(MMX4_PLAY+13,0);
        palette_rebuilds=palette_uploads=0;
        mmx4_coop_split_local_explosion_palette(cpu,camera);
        CHECK(palette_rebuilds==(test==0) && palette_uploads==(test==0));
    }
    split=ready=1;projected=local_scope=0;
}
static void intro_effect_views(CPUState *cpu) {
    for(unsigned axis=0;axis<2;++axis) {
        split=ready=1;projected=local_scope=0;script_owner=-1;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
        psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,1);
        psx_mod_write_byte(MMX4_PLAY+0x10,0);psx_mod_write_byte(MMX4_PLAY+0x1C,0);
        actor_position(MMX4_PLAYER,100,128);second_position(200,128);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        second[0]=second[4]=1;
        memset(ram+(CAMERA&0x1FFFFFu),0,0xFC);
        CHECK(mmx4_coop_split_activate());mmx4_coop_split_camera_prepare(0);
        const uint32_t object=0x80165A30u;
        actor_position(object,160,128);psx_mod_write_byte(object,1);
        cpu->gpr[16]=object;cpu->gpr[31]=axis?0x800BF104u:0x800BEF58u;
        cpu->gpr[6]=2;shake_calls=0;
        /* Use the real native command contract, with a duration and amplitude. */
        mmx4_coop_call(cpu,axis?0x80028BACu:0x80028B68u,32,4);
        CHECK(shake_calls==1 && !projected);
        for(unsigned seat=0;seat<2;++seat) {
            uint32_t view=saved_camera(seat,0);
            CHECK(psx_mod_read_half(view+0x34)&(axis?1u:16u));
            CHECK(psx_mod_read_byte(view+0x36+axis)==32);
            CHECK(psx_mod_read_byte(view+0x45+axis)==4);
            CHECK(psx_mod_read_half(view+10)==0);
        }
        /* A distant view keeps its own origins, bounds and other quake axis. */
        psx_mod_write_half(saved_camera(1,0)+10,4000);
        psx_mod_write_half(saved_camera(1,0)+0x24,4800);
        actor_position(object,4160,128);
        for(unsigned seat=0;seat<2;++seat) {
            psx_mod_write_half(saved_camera(seat,0)+0x34,0x200);
            psx_mod_write_byte(saved_camera(seat,0)+0x36+axis,0);
        }
        psx_mod_write_half(CAMERA+0x34,0x200);psx_mod_write_byte(CAMERA+0x36+axis,7);
        mmx4_coop_call(cpu,axis?0x80028BACu:0x80028B68u,24,3);
        CHECK(psx_mod_read_half(saved_camera(0,0)+0x34)==0x200);
        CHECK(psx_mod_read_half(CAMERA+0x34)==0x200 && psx_mod_read_byte(CAMERA+0x36+axis)==7);
        CHECK(psx_mod_read_half(saved_camera(1,0)+0x34)==(0x200|(axis?1u:16u)));
        CHECK(psx_mod_read_byte(saved_camera(1,0)+0x36+axis)==24);
        CHECK(psx_mod_read_half(saved_camera(1,0)+10)==4000);
        CHECK(psx_mod_read_half(saved_camera(1,0)+0x24)==4800);
        /* Other stages, private redraws and unrelated callers cannot publish. */
        for(unsigned excluded=0;excluded<3;++excluded) {
            psx_mod_write_byte(MMX4_PLAY+12,excluded==0?1:0);
            local_scope=excluded==1;cpu->gpr[31]=excluded==2?0x80010000u:0x800BF104u;
            psx_mod_write_byte(saved_camera(1,0)+0x36+axis,0);
            mmx4_coop_call(cpu,axis?0x80028BACu:0x80028B68u,24,3);
            CHECK(!psx_mod_read_byte(saved_camera(1,0)+0x36+axis));
        }
    }
    /* The closing trap's top emitter represents the entire432px wall.
     * Its bottom view receives both native axes, while the distant partner
     * and a view outside the full vertical footprint remain unaffected. */
    local_scope=0;psx_mod_write_byte(MMX4_PLAY+12,0);
    for(unsigned axis=0;axis<2;++axis)for(unsigned distant=0;distant<2;++distant) {
        psx_mod_write_byte(MMX4_PLAY+13,1);
        const uint32_t object=0x80165A30u;
        actor_position(object,3552,536);psx_mod_write_byte(object,1);
        psx_mod_write_half(CAMERA+10,1456);psx_mod_write_half(CAMERA+14,256);
        psx_mod_write_half(CAMERA+0x34,0);
        psx_mod_write_half(saved_camera(0,0)+10,1456);
        psx_mod_write_half(saved_camera(0,0)+14,256);
        psx_mod_write_half(saved_camera(0,0)+0x34,0);
        uint32_t view=saved_camera(1,0);
        psx_mod_write_half(view+10,3584);psx_mod_write_half(view+14,distant?1600:762);
        psx_mod_write_half(view+0x34,0);psx_mod_write_byte(view+0x36+axis,0);
        cpu->gpr[16]=object;cpu->gpr[31]=axis?0x800C07F8u:0x800C0750u;
        unsigned before=shake_calls;
        mmx4_coop_call(cpu,axis?0x80028BACu:0x80028B68u,32,axis?2:3);
        CHECK(shake_calls==before+1 && !psx_mod_read_half(CAMERA+0x34));
        CHECK(!psx_mod_read_half(saved_camera(0,0)+0x34));
        CHECK(psx_mod_read_byte(view+0x36+axis)==(distant?0:32));
        CHECK(psx_mod_read_half(view+10)==3584 &&
            psx_mod_read_half(view+14)==(distant?1600:762));
    }
    local_scope=0;psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_half(CAMERA+10,0);psx_mod_write_half(CAMERA+14,0);
    actor_position(MMX4_PLAYER,100,128);second_position(4160,128);
    CHECK(mmx4_coop_split_activate());mmx4_coop_split_camera_prepare(0);
    psx_mod_write_half(saved_camera(1,0)+10,4000);
    const uint32_t effects=0x80142F98u;
    psx_mod_write_byte(effects,1);psx_mod_write_byte(effects+1,28);
    cpu->gpr[4]=effects;explosion_calls=0;shake_calls=0;
    invoke(cpu,0x800BBC14u);
    CHECK(explosion_calls==1 && shake_calls==1);
    CHECK(psx_mod_read_half(effects+10)==4160 && psx_mod_read_half(CAMERA+10)==0);
    CHECK(psx_mod_read_byte(saved_camera(1,0)+0x37)==10);
    /* A nearby P1 must keep a new command after the P2 camera scope is
     * restored. Its next canonical snapshot must not erase that command. */
    actor_position(MMX4_PLAYER,4040,128);
    psx_mod_write_half(CAMERA+10,4000);psx_mod_write_half(saved_camera(0,0)+10,4000);
    psx_mod_write_byte(CAMERA+0x37,0);cpu->gpr[4]=effects;
    invoke(cpu,0x800BBC14u);
    CHECK(psx_mod_read_byte(CAMERA+0x37)==10);
    CHECK(psx_mod_read_byte(saved_camera(0,0)+0x37)==10);
    mmx4_coop_split_camera(cpu,0);
    CHECK(psx_mod_read_byte(saved_camera(0,0)+0x37)==10);
    /* The same transaction retains the actual canonical survivor's command. */
    psx_mod_write_byte(MMX4_PLAYER+4,3);mmx4_coop_split_camera_prepare(1);
    psx_mod_write_byte(CAMERA+0x37,0);cpu->gpr[4]=effects;
    invoke(cpu,0x800BBC14u);CHECK(psx_mod_read_byte(CAMERA+0x37)==10);
    psx_mod_write_byte(MMX4_PLAYER+4,1);
    /* WARNING may freeze follow before the actual native boss command.
     * Ordinary speculative camera scopes still reject that freeze. */
    psx_mod_write_byte(MMX4_PLAY+0x10,1);psx_mod_write_byte(MMX4_PLAY+0x1C,1);
    uint8_t old[0xFC];CHECK(!mmx4_coop_split_scene_camera_begin(1,old));
    mmx4_coop_split_commit_scene_camera(1);
    CHECK(psx_mod_read_half(CAMERA+10)==4000 && psx_mod_read_word(0x801F0008u)==2);
    local_scope=1;psx_mod_write_half(CAMERA+10,123);
    mmx4_coop_split_commit_scene_camera(1);CHECK(psx_mod_read_half(CAMERA+10)==123);
    local_scope=0;
    CHECK(mmx4_coop_split_snapshot_ready());
}

static void volcano_rolling_rumble(CPUState *cpu) {
    const uint32_t ball=0x8013F328u,emitter=0x80142F98u;
    split=ready=1;projected=local_scope=0;script_owner=-1;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+1,0);
    psx_mod_write_byte(MMX4_PLAY+12,4);psx_mod_write_byte(MMX4_PLAY+13,0);
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=second[4]=1;
    actor_position(MMX4_PLAYER,128,651);second_position(1120,954);
    memset(ram+(CAMERA&0x1FFFFFu),0,0xFC);
    actor_position(CAMERA,0,512);
    CHECK(mmx4_coop_split_activate());mmx4_coop_split_camera_prepare(0);
    actor_position(saved_camera(1,0),960,768);
    psx_mod_write_half(saved_camera(1,0)+0x24,800);
    psx_mod_write_byte(ball,1);psx_mod_write_byte(ball+1,33);
    psx_mod_write_byte(ball+4,4);psx_mod_write_byte(ball+0x15,0);actor_position(ball,200,654);
    actor_position(emitter,36,649);psx_mod_write_byte(emitter,1);
    psx_mod_write_byte(emitter+1,33);psx_mod_write_byte(emitter+2,0x10);
    psx_mod_write_byte(emitter+4,1);
    cpu->gpr[16]=ball;cpu->gpr[31]=0x800A0CB8u;cpu->gpr[6]=2;
    unsigned before=shake_calls;
    mmx4_coop_call(cpu,0x80028B68u,8,4);
    CHECK(shake_calls==before+1);
    for(unsigned seat=0;seat<2;++seat) {
        uint32_t view=saved_camera(seat,0);
        CHECK(psx_mod_read_half(view+0x34)&16u);
        CHECK(psx_mod_read_byte(view+0x36)==8);
        CHECK(psx_mod_read_byte(view+0x45)==4);
    }
    CHECK(psx_mod_read_half(saved_camera(1,0)+10)==960);
    CHECK(psx_mod_read_half(saved_camera(1,0)+14)==768);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x24)==800);
    /* A fresh command admitted by one view cannot restart the other view's
     * independent running timer, phase, vertical shake or room bounds. */
    psx_mod_write_half(saved_camera(0,0)+0x34,0);
    psx_mod_write_byte(saved_camera(1,0)+0x36,5);
    psx_mod_write_byte(saved_camera(1,0)+0x3E,0x84);
    psx_mod_write_byte(saved_camera(1,0)+0x37,19);
    mmx4_coop_call(cpu,0x80028B68u,8,4);
    CHECK(psx_mod_read_byte(saved_camera(0,0)+0x36)==8);
    CHECK(psx_mod_read_byte(saved_camera(1,0)+0x36)==5);
    CHECK(psx_mod_read_byte(saved_camera(1,0)+0x3E)==0x84);
    CHECK(psx_mod_read_byte(saved_camera(1,0)+0x37)==19);
    /* Other part/caller/phase, private redraw, Unified, and a dead partner
     * must not gain a remote rolling-rumble command. */
    for(unsigned excluded=0;excluded<10;++excluded) {
        split=excluded!=4;local_scope=excluded==3;
        second[4]=(uint8_t)(excluded==5?3:1);
        psx_mod_write_byte(MMX4_PLAY+13,(uint8_t)(excluded==0));
        psx_mod_write_byte(ball+4,(uint8_t)(excluded==2?1:4));
        psx_mod_write_byte(ball+0x15,(uint8_t)(excluded==6?0x40:0));
        psx_mod_write_byte(emitter+4,(uint8_t)(excluded==7?2:1));
        psx_mod_write_byte(emitter+2,(uint8_t)(excluded==8?0x11:0x10));
        actor_position(saved_camera(1,0),excluded==9?4000:960,768);
        cpu->gpr[31]=excluded==1?0x800A0858u:0x800A0CB8u;
        psx_mod_write_half(saved_camera(1,0)+0x34,0);
        psx_mod_write_byte(saved_camera(1,0)+0x36,0);
        mmx4_coop_call(cpu,0x80028B68u,8,4);
        CHECK(!psx_mod_read_byte(saved_camera(1,0)+0x36));
    }
    split=ready=1;local_scope=projected=0;second[4]=1;
    psx_mod_write_byte(MMX4_PLAY+13,0);
}
int main(void) {
    CPUState cpu={0};cpu.gpr[29]=0x801FF000u;
    psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAYER,1);
    /* The existing bounds dispatcher owns the registration. Its course
     * completion exception must run even when player projection excludes
     * ordinary Split bounds handling. No second handler can be registered. */
    projected=1;split=0;assessment_override=1;
    CHECK(mmx4_coop_call(&cpu,0x8002B160u,ACTOR,0)==0);
    projected=0;split=1;assessment_override=0;
    /* The native intro flyby is screen-relative only to the canonical
     * camera. Its body and attached type10 sprites must become world
     * coordinates in a private draw, while UI and unrelated effects stay. */
    const uint32_t boss=0x8013BED0u,effect=0x8013E510u,unrelated=effect+0x70u;
    actor_position(CAMERA,2976,256);actor_position(boss,38,180);
    psx_mod_write_byte(boss,1);psx_mod_write_byte(boss+0x14,255);
    psx_mod_write_half(boss+8,0xF000);psx_mod_write_half(CAMERA+8,0xF000);
    actor_position(effect,45,170);psx_mod_write_byte(effect,1);
    psx_mod_write_byte(effect+1,10);psx_mod_write_byte(effect+0x14,255);
    psx_mod_write_word(effect+0x50,boss);
    actor_position(unrelated,20,30);psx_mod_write_byte(unrelated,1);
    psx_mod_write_byte(unrelated+1,11);psx_mod_write_byte(unrelated+0x14,255);
    psx_mod_write_word(unrelated+0x50,boss);
    mmx4_coop_split_project_intro_boss();CHECK(psx_mod_read_half(boss+10)==38);
    local_scope=1;mmx4_coop_split_project_intro_boss();
    CHECK(psx_mod_read_half(boss+10)==3014 && psx_mod_read_half(boss+14)==436);
    CHECK(psx_mod_read_half(boss+8)==0xF000 && !psx_mod_read_byte(boss+0x14));
    CHECK(psx_mod_read_half(effect+10)==3021 && psx_mod_read_half(effect+14)==426);
    CHECK(!psx_mod_read_byte(effect+0x14) && psx_mod_read_byte(unrelated+0x14)==255);
    CHECK(!mmx4_coop_split_sprite_outside(boss));
    actor_position(CAMERA,64,256);CHECK(mmx4_coop_split_sprite_outside(boss));
    CHECK(mmx4_coop_split_sprite_outside(effect));
    CHECK(psx_mod_read_half(unrelated+10)==20 && psx_mod_read_half(unrelated+14)==30);
    local_scope=0;psx_mod_write_byte(boss,0);psx_mod_write_byte(effect,0);
    psx_mod_write_byte(unrelated,0);psx_mod_write_word(CAMERA+8,0);
    /* Far world sprites alias into a view after 11-bit GPU truncation.
     * Cull using the camera currently being drawn, not both live regions. */
    actor_position(CAMERA,1900,1200);actor_position(ACTOR,1900+160+2048,1200+120);
    psx_mod_write_byte(ACTOR+3,1);
    CHECK(mmx4_coop_split_sprite_outside(ACTOR));
    cpu.gpr[4]=ACTOR;CHECK(invoke(&cpu,0x800257BCu)==0);
    CHECK(rider_armor_calls==0); /* Culled armor must not reach the emitter. */
    CHECK(psx_mod_read_byte(ACTOR+3)==1); /* Still alive/visible to the other seat. */
    actor_position(CAMERA,3948,1200);CHECK(!mmx4_coop_split_sprite_outside(ACTOR));
    CHECK(invoke(&cpu,0x800257BCu)==123 && rider_armor_calls==1);
    actor_position(ACTOR,3948+160,1200+120-2048);CHECK(mmx4_coop_split_sprite_outside(ACTOR));
    actor_position(ACTOR,3948-144,1200-144);CHECK(!mmx4_coop_split_sprite_outside(ACTOR));
    actor_position(ACTOR,3948+320+144,1200+240+144);CHECK(!mmx4_coop_split_sprite_outside(ACTOR));
    actor_position(ACTOR,3948+4096,1200);CHECK(mmx4_coop_split_sprite_outside(ACTOR));
    psx_mod_write_byte(ACTOR+0x14,1);actor_position(CAMERA+0x54,3948+4096,1200);
    CHECK(!mmx4_coop_split_sprite_outside(ACTOR)); /* Native per-layer camera. */
    psx_mod_write_byte(ACTOR+0x14,255);CHECK(!mmx4_coop_split_sprite_outside(ACTOR));
    psx_mod_write_byte(ACTOR+0x14,0);split=0;CHECK(!mmx4_coop_split_sprite_outside(ACTOR));
    CHECK(invoke(&cpu,0x800257BCu)==123 && rider_armor_calls==2);
    split=1;local_scope=1;projected=1;CHECK(mmx4_coop_split_sprite_outside(ACTOR));
    local_scope=projected=0;actor_position(CAMERA,0,0);
    actor_position(CAMERA+0x54,0,0);
    psx_mod_write_byte(MMX4_PLAYER+4,1);second[0]=second[4]=1;
    psx_mod_write_byte(CAMERA+0x44,1);actor_position(MMX4_PLAYER,100,140);
    second_position(1200,2000);
    CHECK(mmx4_coop_split_activate());
    uint8_t canonical[0xFC],view[0xFC];
    for(unsigned offset=0;offset<sizeof canonical;++offset)canonical[offset]=psx_mod_read_byte(CAMERA+offset);
    mmx4_coop_split_camera(&cpu,0);
    for(unsigned offset=0;offset<sizeof canonical;++offset)CHECK(canonical[offset]==psx_mod_read_byte(CAMERA+offset));
    CHECK(!projected && mmx4_coop_split_camera_copy(1,view));
    CHECK((view[10]|(unsigned)view[11]<<8)==1040 && (view[14]|(unsigned)view[15]<<8)==1872);
    CHECK((view[0x54+10]|(unsigned)view[0x54+11]<<8)==520);
    actor_position(MMX4_PLAYER,600,300);actor_position(CAMERA,440,172);
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && (view[10]|(unsigned)view[11]<<8)==1040);
    actor_position(MMX4_PLAYER,100,140);actor_position(CAMERA,0,0);
    mmx4_coop_split_camera(&cpu,0);
    cpu.gpr[4]=ACTOR;cpu.gpr[5]=cpu.gpr[6]=64;
    actor_position(ACTOR,1200,2000);CHECK(!invoke(&cpu,0x8002B1E8u));
    CHECK(invoke(&cpu,0x8002B318u) && psx_mod_read_byte(ACTOR+3));
    actor_position(ACTOR,600,1000);CHECK(invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,100,2000);CHECK(invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,1200,140);CHECK(invoke(&cpu,0x8002B1E8u));
    mmx4_coop_enter_second();actor_context=1;
    Mmx4CoopViewActor actors[2];mmx4_coop_split_actors(actors);
    CHECK(actors[0].world_x==100 && actors[0].world_y==140);
    CHECK(actors[1].world_x==1200 && actors[1].world_y==2000);
    actor_position(ACTOR,1200,2000);CHECK(!invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,100,140);CHECK(!invoke(&cpu,0x8002B1E8u));
    actor_position(ACTOR,100,2000);CHECK(invoke(&cpu,0x8002B1E8u));
    /* Attached water effects run projected too. B288 must retain a ripple
     * near either actual view and clear it outside both, restoring camera. */
    actor_position(ACTOR,1200,2000);invoke(&cpu,0x8002B288u);
    CHECK(psx_mod_read_byte(ACTOR+3) && !psx_mod_read_half(CAMERA+10));
    actor_position(ACTOR,100,140);invoke(&cpu,0x8002B288u);
    CHECK(psx_mod_read_byte(ACTOR+3) && !psx_mod_read_half(CAMERA+10));
    actor_position(ACTOR,100,2000);invoke(&cpu,0x8002B288u);
    CHECK(!psx_mod_read_byte(ACTOR+3) && !psx_mod_read_half(CAMERA+10));
    actor_context=0;mmx4_coop_leave_second();
    actor_position(ACTOR,1200,2000);local_scope=1;
    CHECK(invoke(&cpu,0x8002B1E8u));local_scope=0;
    script_owner=0;CHECK(invoke(&cpu,0x8002B1E8u));
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[10]==0 && view[11]==0);script_owner=-1;
    /* A physical paired crossing keeps separate camera origins, including
     * the native opening lock. Real boss scenes still share their view. */
    script_owner=0;paired_door=1;psx_mod_write_byte(MMX4_PLAY+0x1C,1);
    CHECK(mmx4_coop_split_view_seat(1)==1);
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view));
    CHECK((view[10]|(unsigned)view[11]<<8)==1040);
    CHECK(psx_mod_read_half(CAMERA+10)==0 && !projected);
    paired_door=0;CHECK(mmx4_coop_split_view_seat(1)==0);
    psx_mod_write_byte(MMX4_PLAY+0x1C,0);script_owner=-1;
    psx_mod_write_word(0x800F43C8u,TABLE);
    placement(0,100,140,0);placement(1,1200,2000,0);
    placement(2,600,1000,0);placement(3,100,2000,0);placement(4,1300,2100,3);
    psx_mod_write_byte(TABLE+5u*8u+3u,255);
    capacity=1;mmx4_coop_split_reset();mmx4_coop_split_camera(&cpu,0);
    invoke(&cpu,0x80028E24u);
    CHECK(allocations==1 && (psx_mod_read_byte(TABLE)&1u) && !psx_mod_read_byte(TABLE+8));
    capacity=16;mmx4_coop_split_camera(&cpu,0);invoke(&cpu,0x80028E24u);
    CHECK(allocations==3 && (psx_mod_read_byte(TABLE+8)&1u) && (psx_mod_read_byte(TABLE+32)&1u));
    CHECK(!psx_mod_read_byte(TABLE+16) && !psx_mod_read_byte(TABLE+24));
    psx_mod_write_byte(TABLE,0);invoke(&cpu,0x80028E24u);CHECK(allocations==3);
    uint32_t digest=mmx4_coop_split_digest(2166136261u);
    actor_position(CAMERA,3000,3000);second_position(3200,3200);
    mmx4_coop_split_camera(&cpu,0);invoke(&cpu,0x80028E24u);
    CHECK(mmx4_coop_split_digest(2166136261u)!=digest);
    actor_position(CAMERA,0,0);second_position(1200,2000);
    mmx4_coop_split_camera(&cpu,0);invoke(&cpu,0x80028E24u);CHECK(allocations==4);
    second_position(-100,140);mmx4_coop_split_camera(&cpu,0);
    CHECK(second[10]==8 && second[11]==0 && psx_mod_read_half(MMX4_PLAYER+10)==100);
    CHECK(scans>0);
    /* Native room following eases +1E toward the authored target +26 and
     * 27BE4 clamps a player to that lower X limit. P1 entering a different
     * camera region must not import its target into stationary P2's view. */
    mmx4_coop_split_reset();actor_position(MMX4_PLAYER,100,140);
    actor_position(CAMERA,0,0);second_position(100,140);
    psx_mod_write_half(CAMERA+0x26,0);mmx4_coop_split_camera(&cpu,0);
    psx_mod_write_half(CAMERA+0x26,512);mmx4_coop_split_camera(&cpu,0);
    CHECK(second[10]==100 && second[11]==0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x26]==0 && view[0x27]==0);
    psx_mod_write_byte(MMX4_PLAYER+4,3);mmx4_coop_split_camera_prepare(1);
    CHECK(psx_mod_read_half(CAMERA+0x26)==0 && second[10]==100 && second[11]==0);
    psx_mod_write_byte(MMX4_PLAYER+4,1);psx_mod_write_half(CAMERA+0x26,512);
    script_owner=0;mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x26]==0 && view[0x27]==2);
    script_owner=-1;
    /* The native health refill sets PLAY+10..17 while counting HP.
     * It freezes the world without making the distant partner share a room. */
    mmx4_coop_split_reset();
    actor_position(MMX4_PLAYER,1200,2000);second_position(100,140);
    psx_mod_write_half(CAMERA+0x1E,0);
    psx_mod_write_half(CAMERA+0x26,0);
    mmx4_coop_split_camera(&cpu,0);
    psx_mod_write_half(CAMERA+0x26,512);
    psx_mod_write_half(CAMERA+0x1E,512);
    psx_mod_write_byte(MMX4_PLAY+0x10,1);
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && !view[0x26] && !view[0x27]);
    CHECK(second[10]==100 && !second[11]);
    psx_mod_write_byte(MMX4_PLAY+0x10,0);
    /* Nova Strike's native global lock must not merge distant cameras. */
    psx_mod_write_byte(MMX4_PLAY+0x1C,1);nova_active=1;
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && !view[0x26] && !view[0x27]);
    CHECK(second[10]==100 && !second[11]);
    nova_active=0;psx_mod_write_byte(MMX4_PLAY+0x1C,0);
    /* One spatial script actor, two camera areas. P2 can activate the
     * original region while P1 stays outside; shared writes happen once. */
    mmx4_coop_split_reset();actor_position(MMX4_PLAYER,100,140);second_position(1200,2000);
    psx_mod_write_half(CAMERA+0x26,0);psx_mod_write_half(CAMERA+0x2C,160);
    mmx4_coop_split_camera(&cpu,0);
    uint32_t stage_actor=0x80142F98u,parameters=0x800F6800u;
    psx_mod_write_byte(stage_actor,1);psx_mod_write_byte(stage_actor+1,0);psx_mod_write_byte(stage_actor+4,1);
    actor_position(stage_actor,1300,2000);psx_mod_write_word(stage_actor+0x18,parameters);
    const uint16_t area[]={1500,1000,2500,1500,1,2,3,0};
    for(unsigned i=0;i<8;++i)psx_mod_write_half(parameters+i*2,area[i]);
    psx_mod_write_half(0x8010AE74u,0);psx_mod_write_half(0x8010AE76u,512);
    psx_mod_write_half(0x8010AE78u,1);psx_mod_write_half(0x8010AE7Au,64);
    psx_mod_write_half(0x8010AE7Cu,2);psx_mod_write_half(0x8010AE7Eu,7);
    psx_mod_write_word(0x8010AF9Cu,CAMERA+0x26);psx_mod_write_word(0x8010AFA0u,CAMERA+0x2C);
    psx_mod_write_word(0x8010AFA4u,MMX4_PLAY+0x30);
    cpu.gpr[4]=stage_actor;invoke(&cpu,0x800B56F4u);
    CHECK(area_calls==1 && shared_area_writes==1 && !projected);
    CHECK(psx_mod_read_half(CAMERA+0x26)==0 && psx_mod_read_half(CAMERA+0x2C)==160);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x26]==0 && view[0x27]==2 && view[0x2C]==64);
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x2C]==64 && view[0x26]==0 && view[0x27]==2);
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==100 && second[10]==(1200&255));
    actor_position(MMX4_PLAYER,1100,2000);
    actor_position(stage_actor,1100,2000);
    psx_mod_write_byte(0x801F0000u+16u+0xFCu+0x47u,0);
    cpu.gpr[4]=stage_actor;invoke(&cpu,0x800B56F4u);
    CHECK(area_calls==2 && shared_area_writes==2 && psx_mod_read_half(CAMERA+0x26)==512);
    CHECK(mmx4_coop_split_camera_copy(1,view) && view[0x47]==2);
    psx_mod_write_byte(stage_actor+1,6);psx_mod_write_byte(stage_actor+2,0x12);
    actor_position(stage_actor,1200,1800);actor_position(MMX4_PLAYER,100,140);
    cpu.gpr[4]=stage_actor;invoke(&cpu,0x800B6A0Cu);
    CHECK(checkpoint_calls==1 && psx_mod_read_byte(MMX4_PLAY+0x1D)==2);
    /* An event starts in the initiating camera. A nontriggering query
     * restores canonical state; a committed event retains authored changes. */
    uint16_t original_x=psx_mod_read_half(CAMERA+10);
    CHECK(mmx4_coop_split_scene_camera_begin(1,canonical));
    CHECK(psx_mod_read_half(CAMERA+10)==(uint16_t)(1200-160));
    psx_mod_write_half(CAMERA+0x26,768);mmx4_coop_split_scene_camera_end(1,canonical,0);
    CHECK(psx_mod_read_half(CAMERA+10)==original_x && psx_mod_read_half(CAMERA+0x26)==512);
    /* An unrelated scene lock still rejects, but a native Nova lock must
     * retain the waiting partner's door culling scope without committing it. */
    psx_mod_write_byte(MMX4_PLAY+0x1C,1);nova_active=0;
    CHECK(!mmx4_coop_split_scene_camera_begin(1,canonical));
    nova_active=1;CHECK(mmx4_coop_split_scene_camera_begin(1,canonical));
    CHECK(psx_mod_read_half(CAMERA+10)==(uint16_t)(1200-160));
    mmx4_coop_split_scene_camera_end(1,canonical,0);
    CHECK(psx_mod_read_half(CAMERA+10)==original_x);
    nova_active=0;psx_mod_write_byte(MMX4_PLAY+0x1C,0);
    CHECK(mmx4_coop_split_scene_camera_begin(1,canonical));
    psx_mod_write_half(CAMERA+0x26,1024);mmx4_coop_split_scene_camera_end(1,canonical,1);
    script_owner=1;mmx4_coop_split_camera_prepare(1);CHECK(psx_mod_read_half(CAMERA+0x26)==1024);
    /* The Cyber world flips once, and both saved views must follow it.
     * Fractional current/target Y survives two reflections exactly. */
    for(unsigned seat=0;seat<2;++seat)for(unsigned layer=0;layer<3;++layer) {
        uint32_t camera=0x801F0000u+16u+seat*0xFCu+layer*0x54u;
        psx_mod_write_word(camera+12,0x01234567u);psx_mod_write_word(camera+24,0x02345678u);
        psx_mod_write_half(camera+0x42,1000+seat*10+layer);
        if(!layer) {
            psx_mod_write_half(camera+0x24,400+seat);psx_mod_write_half(camera+0x26,100+seat);
            psx_mod_write_half(camera+0x28,768);psx_mod_write_half(camera+0x2A,768);
        }
    }
    for(unsigned i=0;i<4;++i)psx_mod_write_half(CAMERA+(const unsigned[]){0x20,0x22,0x28,0x2A}[i],1024);
    psx_mod_write_byte(MMX4_PLAY+0x26,1);
    mmx4_coop_split_flip_cameras();
    for(unsigned seat=0;seat<2;++seat)for(unsigned layer=0;layer<3;++layer) {
        uint32_t camera=0x801F0000u+16u+seat*0xFCu+layer*0x54u;
        uint32_t axis=layer?0x09000000u:0x07000000u;
        CHECK(psx_mod_read_word(camera+12)==axis-0x01234567u);
        CHECK(psx_mod_read_word(camera+24)==axis-0x02345678u);
        CHECK(psx_mod_read_byte(camera+0x4C)==1);
        if(!layer) {
            CHECK(psx_mod_read_half(camera+0x24)==400+seat && psx_mod_read_half(camera+0x26)==100+seat);
            for(unsigned i=0;i<4;++i)CHECK(psx_mod_read_half(camera+(const unsigned[]){0x20,0x22,0x28,0x2A}[i])==1024);
        }else CHECK(psx_mod_read_half(camera+0x42)==1000+seat*10+layer+(layer==1?128:192));
    }
    psx_mod_write_byte(MMX4_PLAY+0x26,0);
    mmx4_coop_split_flip_cameras();
    CHECK(psx_mod_read_word(0x801F0000u+16u+12u)==0x01234567u);
    CHECK(psx_mod_read_half(0x801F0000u+16u+0xFCu+0x54u+0x42u)==1011);
    script_owner=-1;train_backgrounds(&cpu);train_panorama_cycles(&cpu);storm_backgrounds(&cpu);snow_backgrounds(&cpu);fortress_backgrounds(&cpu);split=0;CHECK(mmx4_coop_split_activate());
    /* Unified has no private camera storage, but either living seat must
     * activate the authored secret-room bounds/checkpoint once. */
    psx_mod_write_byte(stage_actor+1,0);
    actor_position(MMX4_PLAYER,100,140);second_position(1200,2000);
    psx_mod_write_half(CAMERA+0x26,0);psx_mod_write_half(CAMERA+0x2C,160);
    area_calls=shared_area_writes=0;cpu.gpr[4]=stage_actor;
    psx_mod_write_half(CAMERA+0x24,4096);psx_mod_write_half(CAMERA+0x28,2500);
    psx_mod_write_half(CAMERA+0x2A,0);
    invoke(&cpu,0x800B56F4u);
    CHECK(area_calls==1 && shared_area_writes==1 && !projected);
    CHECK(room_entries==1 && room_owner==1); /* New shared min-X excludes P1. */
    CHECK(psx_mod_read_half(CAMERA+0x26)==512 && psx_mod_read_half(CAMERA+0x2C)==64);
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==100);
    psx_mod_write_byte(stage_actor+1,6);checkpoint_calls=0;
    invoke(&cpu,0x800B6A0Cu);
    CHECK(checkpoint_calls==1 && psx_mod_read_byte(MMX4_PLAY+0x1D)==2);
    CHECK(!mmx4_coop_split_camera_copy(1,view));
    psx_mod_write_byte(MMX4_PLAY+12,3);psx_mod_write_byte(MMX4_PLAY+0x1D,1);
    psx_mod_write_byte(stage_actor+1,8);actor_position(MMX4_PLAYER,3380,203);second_position(3540,138);
    mushroom_calls=0;invoke(&cpu,0x800B7078u);
    CHECK(mushroom_calls==1 && mushroom_x==3540 && mushroom_y==138);
    CHECK(psx_mod_read_byte(MMX4_PLAY+0x1D)==2);
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==3380 && psx_mod_read_half(MMX4_PLAYER+14)==203);
    actor_position(MMX4_PLAYER,6048,2304);second_position(5890,139);
    invoke(&cpu,0x800B7078u);CHECK(mushroom_calls==2 && mushroom_x==5890);
    CHECK(psx_mod_read_half(MMX4_PLAYER+10)==6048 && psx_mod_read_half(MMX4_PLAYER+14)==2304);
    /* Each independent staircase follows its own camera motion, even after
     * the leading player leaves it. Shared controller/checkpoint runs once. */
    split=1;CHECK(mmx4_coop_split_activate());
    psx_mod_write_byte(MMX4_PLAY+1,0);psx_mod_write_byte(MMX4_PLAY+0x10,0);
    psx_mod_write_byte(MMX4_PLAY+0x1C,0);psx_mod_write_byte(MMX4_PLAY+0x1D,0);
    psx_mod_write_half(0x8010B4D0u,1536);psx_mod_write_byte(stage_actor+4,1);
    actor_position(MMX4_PLAYER,1570,322);second_position(800,512);
    mmx4_coop_split_camera_prepare(0);
    cpu.gpr[4]=stage_actor;stair_calls=0;mushroom_calls=0;
    invoke(&cpu,0x800B7078u);CHECK(stair_calls==1 && mushroom_calls==1);
    psx_mod_write_half(saved_camera(1,0)+10,640);psx_mod_write_half(saved_camera(1,0)+22,632);
    for(unsigned tick=0;tick<3;++tick) {
        invoke(&cpu,0x800B7078u);
        CHECK(psx_mod_read_half(saved_camera(1,1)+10)==((tick+1)%3)*512);
        CHECK(psx_mod_read_half(MMX4_PLAYER+10)==1570);
    }
    CHECK(mushroom_calls==4 && stair_calls==4 && !psx_mod_read_byte(MMX4_PLAY+0x1D));
    /* Reverse direction and roles; the old P2 camera must stop cycling. */
    actor_position(MMX4_PLAYER,800,512);second_position(1570,322);
    invoke(&cpu,0x800B7078u);
    psx_mod_write_half(saved_camera(0,0)+10,632);psx_mod_write_half(saved_camera(0,0)+22,640);
    invoke(&cpu,0x800B7078u);
    CHECK(psx_mod_read_half(saved_camera(0,1)+10)==1024);
    CHECK(psx_mod_read_half(saved_camera(1,1)+10)==0);
    unsigned before=stair_calls;
    psx_mod_write_byte(MMX4_PLAY+0x10,1);invoke(&cpu,0x800B7078u);CHECK(stair_calls==before);
    CHECK(mmx4_coop_split_snapshot_ready());
    /* The shared lift releases each rider's vertical camera without
     * replacing independent origins or an absent partner's room limits. */
    mmx4_coop_split_reset();psx_mod_write_byte(MMX4_PLAY+12,3);
    psx_mod_write_byte(MMX4_PLAY+0x10,0);
    const uint32_t lift=0x80165A30u;
    psx_mod_write_byte(lift,1);psx_mod_write_byte(lift+1,14);
    actor_position(MMX4_PLAYER,6048,2200);second_position(6072,2199);
    psx_mod_write_half(CAMERA+0x1C,5888);psx_mod_write_half(CAMERA+0x1E,5888);
    psx_mod_write_half(CAMERA+0x24,5888);psx_mod_write_half(CAMERA+0x26,5888);
    psx_mod_write_half(CAMERA+0x28,2048);psx_mod_write_half(CAMERA+0x2A,2048);
    mmx4_coop_split_camera_prepare(0);
    psx_mod_write_half(saved_camera(1,0)+14,2000);
    cpu.gpr[4]=lift;lift_calls=0;invoke(&cpu,0x800C3880u);
    CHECK(lift_calls==1 && psx_mod_read_byte(lift+5)==1);
    for(unsigned seat=0;seat<2;++seat) {
        CHECK(psx_mod_read_half(saved_camera(seat,0)+0x28)==720);
        CHECK(psx_mod_read_half(saved_camera(seat,0)+0x2A)==720);
    }
    CHECK(psx_mod_read_half(saved_camera(1,0)+14)==2000);
    psx_mod_write_half(saved_camera(1,0)+0x26,0);
    psx_mod_write_half(saved_camera(1,0)+0x28,512);
    psx_mod_write_half(CAMERA+0x28,2048);
    invoke(&cpu,0x800C3880u);CHECK(lift_calls==2);
    CHECK(psx_mod_read_half(saved_camera(1,0)+0x28)==512);
    CHECK(mmx4_coop_split_snapshot_ready());
    destructible_camera_unlocks(&cpu);
    dragoon_camera_release(&cpu);
    owl_midboss_unlocks(&cpu);
    mushroom_midboss_unlocks(&cpu);
    frost_shaft_unlocks(&cpu);
    frost_shaft_follow(&cpu);
    four_aligned_placements(&cpu);
    stationary_second_door(&cpu);
    stationary_refight_portal(&cpu);
    jungle_backgrounds(&cpu);
    frost_ice_ranges(&cpu);
    local_explosion_flash(&cpu);
    mushroom_lift_high_jump(&cpu);
    jet_prefetched_placement(&cpu);
    intro_effect_views(&cpu);
    volcano_rolling_rumble(&cpu);
    const unsigned laser_stages[]={7,8,11};
    for(unsigned site=0;site<3;++site)for(unsigned stage_index=0;stage_index<3;++stage_index)for(unsigned owner=0;owner<2;++owner) {
        unsigned stage=laser_stages[stage_index];
        if(stage==11 && !site)continue;
        const uint32_t sites[]={0x8009D104u,0x800A3B08u,0x800A3C00u};
        unsigned value_register=site==2?3u:2u;
        mmx4_coop_split_reset();script_owner=-1;local_scope=0;projected=0;
        psx_mod_write_byte(MMX4_PLAY+12,(uint8_t)stage);
        const uint32_t beam=0x8013F328u,parent=0x8013BED0u;
        psx_mod_write_byte(beam,1);psx_mod_write_byte(beam+1,site?36:20);
        psx_mod_write_word(beam+0x7C,parent);psx_mod_write_byte(parent,1);
        actor_position(MMX4_PLAYER,1000,350);second_position(4500,350);
        actor_position(parent,owner?4520:1020,320);
        mmx4_coop_split_camera_prepare(0);
        psx_mod_write_half(saved_camera(0,0)+10,840);
        psx_mod_write_half(saved_camera(1,0)+10,4340);
        cpu.gpr[16]=beam;cpu.gpr[17]=parent;cpu.gpr[value_register]=840;
        laser_origin(&cpu,sites[site]);CHECK(cpu.gpr[value_register]==(owner?4340:840));
        CHECK(psx_mod_read_half(CAMERA+10)!=4340); /* No canonical camera mutation. */
        local_scope=1;cpu.gpr[value_register]=123;laser_origin(&cpu,sites[site]);
        CHECK(cpu.gpr[value_register]==123);local_scope=0;
        psx_mod_write_byte(beam+1,21);laser_origin(&cpu,sites[site]);
        CHECK(cpu.gpr[value_register]==123);
    }
    /* Native world/screen conversions and private drawing retain the same
     * participant even after screen XY moves nearest to the waiting hero. */
    for(unsigned leader=0;leader<2;++leader) {
        mmx4_coop_split_reset();script_owner=-1;projected=local_scope=0;
        psx_mod_write_byte(MMX4_PLAY+12,0);psx_mod_write_byte(MMX4_PLAY+13,0);
        psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
        second[0]=second[4]=1;actor_position(MMX4_PLAYER,leader?200:3238,427);
        second_position(leader?3238:200,427);
        mmx4_coop_split_camera_prepare(0);
        actor_position(saved_camera(0,0),leader?40:2976,256);
        actor_position(saved_camera(1,0),leader?2976:40,256);
        actor_position(CAMERA,leader?40:2976,256);
        actor_position(boss,3284,385);psx_mod_write_byte(boss,1);
        psx_mod_write_byte(boss+1,0);psx_mod_write_byte(boss+0x14,0);
        uint8_t before[0xFC];
        for(unsigned i=0;i<sizeof before;++i)before[i]=psx_mod_read_byte(CAMERA+i);
        cpu.gpr[4]=boss;dragon_calls=0;
        CHECK(invoke(&cpu,0x80041DF0u)==77 && dragon_calls==1 && dragon_camera_x==2976);
        for(unsigned i=0;i<sizeof before;++i)CHECK(before[i]==psx_mod_read_byte(CAMERA+i));
        CHECK(mmx4_coop_split_snapshot_ready());
        /* Parent and type10 muzzle share the recorded camera, including
         * integer-only rebase with no fractional carry. */
        psx_mod_write_half(boss+8,0xF000);actor_position(effect,45,170);
        psx_mod_write_byte(effect,1);psx_mod_write_byte(effect+1,10);
        psx_mod_write_byte(effect+0x14,255);psx_mod_write_word(effect+0x50,boss);
        local_scope=1;mmx4_coop_split_project_intro_boss();local_scope=0;
        CHECK(psx_mod_read_half(boss+10)==3014 && psx_mod_read_half(boss+8)==0xF000);
        CHECK(psx_mod_read_half(effect+10)==3021);
        const uint32_t shot=0x8013F328u;
        psx_mod_write_byte(shot,1);psx_mod_write_byte(shot+1,0);
        psx_mod_write_word(shot+0x7C,boss);
        cpu.gpr[16]=shot;cpu.gpr[17]=boss;cpu.gpr[3]=123;
        psx_mod_write_byte(boss+0x14,255);
        dragon_origin(&cpu,0x80099634u);CHECK(cpu.gpr[3]==(2976u<<16));
        dragon_origin(&cpu,0x80099650u);CHECK(cpu.gpr[3]==(256u<<16));
        local_scope=1;cpu.gpr[3]=123;dragon_origin(&cpu,0x80099634u);
        CHECK(cpu.gpr[3]==123);local_scope=0;
        psx_mod_write_byte(boss+0x14,0);dragon_origin(&cpu,0x80099634u);
        CHECK(cpu.gpr[3]==123);psx_mod_write_byte(shot,0);
        /* Model the private renderer's rollback, then move its live camera.
         * Reentry must use that current view, never canonical P1 or screen XY. */
        actor_position(boss,38,180);psx_mod_write_byte(boss+0x14,255);
        actor_position(saved_camera(leader,0),3000,256);
        CHECK(invoke(&cpu,0x8004205Cu)==77 && dragon_calls==2 && dragon_camera_x==3000);
        CHECK(psx_mod_read_half(boss+10)==3038 && !psx_mod_read_byte(boss+0x14));
        for(unsigned i=0;i<sizeof before;++i)CHECK(before[i]==psx_mod_read_byte(CAMERA+i));
        /* A reused primary slot chooses its new world-space participant. */
        actor_position(boss,200,427);CHECK(invoke(&cpu,0x80041DF0u)==77);
        CHECK(dragon_camera_x==40);
        psx_mod_write_byte(boss,0);psx_mod_write_byte(effect,0);
    }
    for(unsigned leader=0;leader<2;++leader) {
        mmx4_coop_split_reset();script_owner=-1;projected=local_scope=0;
        psx_mod_write_byte(MMX4_PLAY+12,4);psx_mod_write_byte(MMX4_PLAY+13,0);
        memset(ram+(CAMERA&0x1FFFFFu),0,0xFC);
        actor_position(MMX4_PLAYER,leader?200:1800,427);second_position(leader?1800:200,427);
        mmx4_coop_split_camera_prepare(0);
        actor_position(saved_camera(0,0),leader?40:1640,256);
        actor_position(saved_camera(1,0),leader?1640:40,256);
        actor_position(CAMERA,leader?40:1640,256);
        const uint32_t emitter=0x80142F98u,ball=0x8013F328u;
        actor_position(emitter,1536,400);psx_mod_write_byte(emitter,1);
        psx_mod_write_byte(emitter+1,33);psx_mod_write_byte(emitter+2,0);
        volcano_calls=fireball_calls=0;cpu.gpr[4]=emitter;
        CHECK(invoke(&cpu,0x800BC748u)==78 && volcano_calls==1);
        CHECK(volcano_player_x==1800 && fireball_owner==leader+1u && !projected);
        CHECK(psx_mod_read_half(CAMERA+10)==(leader?40:1640));
        psx_mod_write_byte(ball,1);psx_mod_write_byte(ball+1,33);
        psx_mod_write_byte(ball+4,0);actor_position(ball,0,0);cpu.gpr[4]=ball;
        CHECK(invoke(&cpu,0x800A03B8u)==79 && fireball_calls==1 && fireball_camera_x==1640);
        CHECK(psx_mod_read_half(ball+10)==1740);
        CHECK(invoke(&cpu,0x800A03B8u)==79 && fireball_calls==2);
        CHECK(psx_mod_read_byte(saved_camera(leader,0)+0x36)==16);
        CHECK(!psx_mod_read_byte(saved_camera(leader^1u,0)+0x36));
        CHECK(psx_mod_read_half(CAMERA+10)==(leader?40:1640));
        CHECK(mmx4_coop_split_snapshot_ready());
        /* A placed ball can hit beside the non-allocating hero. Its native
         * quake guard must see that nearby view's running timer, rather than
         * restart it from a distant view's clear bit on the next update. */
        unsigned other=leader^1u;
        actor_position(ball,200,427);
        psx_mod_write_half(saved_camera(other,0)+0x34,0);
        psx_mod_write_half(saved_camera(leader,0)+0x34,0);
        unsigned issued=shake_calls;cpu.gpr[4]=ball;
        invoke(&cpu,0x800A03B8u);
        CHECK(fireball_camera_x==40 && shake_calls==issued+1);
        psx_mod_write_byte(saved_camera(other,0)+0x36,7);
        invoke(&cpu,0x800A03B8u);
        CHECK(shake_calls==issued+1 && psx_mod_read_byte(saved_camera(other,0)+0x36)==7);
        CHECK(psx_mod_read_half(CAMERA+10)==(leader?40:1640));
        /* One hero already beyond the emitter cannot retire it while the
         * other is still in its authored falling-fireball window. */
        actor_position(MMX4_PLAYER,1800,427);second_position(4000,427);cpu.gpr[4]=emitter;
        invoke(&cpu,0x800BC748u);CHECK(volcano_player_x==1800 && fireball_owner==1);
    }
    /* A stationary independent view can discover only the admitted Volcano
     * emitter family among scripted stage actors, without duplicate spawns. */
    for(unsigned stage=3;stage<=4;++stage) {
        split=ready=1;projected=local_scope=0;script_owner=-1;paired_door=0;
        mmx4_coop_split_reset();native_directional_scans=1;native_table=TABLE;
        allocations=0;capacity=16;scan_direction=0;
        memset(ram+(CAMERA&0x1FFFFFu),0,0xFC);
        memset(ram+(TABLE&0x1FFFFFu),0,32);
        psx_mod_write_byte(MMX4_PLAY+12,(uint8_t)stage);psx_mod_write_byte(MMX4_PLAY+13,0);
        psx_mod_write_word(0x800F43C8u+stage*8u,TABLE);
        actor_position(MMX4_PLAYER,100,140);second_position(1200,2000);
        placement(0,1200,2000,3);psx_mod_write_byte(TABLE+1,33);
        placement(1,1210,2000,3);psx_mod_write_byte(TABLE+9,34);
        psx_mod_write_byte(TABLE+16+3,255);
        mmx4_coop_split_camera_prepare(0);
        actor_position(saved_camera(1,0),1040,1872);
        for(unsigned seat=0;seat<2;++seat) {
            uint32_t c=saved_camera(seat,0);
            psx_mod_write_word(c+20,psx_mod_read_word(c+8));
            psx_mod_write_word(c+24,psx_mod_read_word(c+12));
        }
        invoke(&cpu,0x80028E24u);CHECK(allocations==(stage==4));
        invoke(&cpu,0x80028E24u);CHECK(allocations==(stage==4));
        CHECK(!(psx_mod_read_byte(TABLE+8)&1u));
    }
    /* These new bindings leave non-gameplay, private draws, Unified and
     * unrelated stages to their original entry, with no native extra pass. */
    const uint32_t sites[]={0x800BC748u,0x800A03B8u,0x80041DF0u,0x8004205Cu};
    for(unsigned n=0;n<4;++n)for(unsigned excluded=0;excluded<5;++excluded) {
        split=ready=1;projected=local_scope=0;
        psx_mod_write_byte(MMX4_PLAY,6);psx_mod_write_byte(MMX4_PLAY+12,n<2?4:0);
        psx_mod_write_byte(MMX4_PLAY+13,0);
        cpu.gpr[4]=n==0?0x80142F98u:n==1?0x8013F328u:boss;
        psx_mod_write_byte(cpu.gpr[4],1);psx_mod_write_byte(cpu.gpr[4]+1,n<2?33:0);
        if(excluded==0)split=0;
        if(excluded==1)ready=0;
        if(excluded==2)local_scope=1;
        if(excluded==3)psx_mod_write_byte(MMX4_PLAY,7);
        if(excluded==4)psx_mod_write_byte(MMX4_PLAY+12,2);
        unsigned matched=0;
        for(unsigned f=0;f<filter_count;++f)if(filters[f].address==sites[n]) {
            CHECK(!filters[f].callback(&cpu,sites[n]));++matched;
        }
        CHECK(matched==1);
    }
    /* An opening publishes the room once. A later stair return changes only
     * its own view; touching the still-open door can restore that view alone. */
    split=ready=1;projected=local_scope=0;script_owner=-1;paired_door=0;
    psx_mod_write_byte(MMX4_PLAYER,1);psx_mod_write_byte(MMX4_PLAYER+4,1);
    second[0]=1;second[4]=1;
    actor_position(MMX4_PLAYER,4130,923);second_position(4300,922);
    actor_position(CAMERA,3970,736);mmx4_coop_split_reset();
    mmx4_coop_split_camera(&cpu,0);
    const uint16_t doorway_bounds[]={4160,3776,736,736};
    mmx4_coop_split_door_camera_bounds(3,doorway_bounds);
    script_owner=0;paired_door=1;
    psx_mod_write_half(CAMERA+0x1C,3776);psx_mod_write_half(CAMERA+0x24,3776);
    mmx4_coop_split_camera(&cpu,0);
    CHECK(mmx4_coop_split_camera_copy(1,view));
    CHECK((view[0x24]|(unsigned)view[0x25]<<8)==4160);
    CHECK(second[10]==(4300&255) && second[11]==(4300>>8));
    mmx4_coop_split_door_camera_bounds(1,doorway_bounds);
    CHECK(psx_mod_read_half(CAMERA+0x24)==4160);
    CHECK(mmx4_coop_split_camera_copy(1,view) && (view[0x24]|(unsigned)view[0x25]<<8)==4160);
    puts("mmx4_coop_split_test: PASS");return 0;
}
