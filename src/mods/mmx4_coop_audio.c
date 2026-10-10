/* SLUS-00561 private character audio. The original VAB parser and voice
 * driver retain their metadata/voice selection; only sample residency differs.
 * See docs/COOP_AUDIO_EVIDENCE.md. No derived assets ship with the program. */
#include "mmx4_coop_internal.h"
#include "mmx4_coop_assets.h"
#include <string.h>
#include <stdio.h>

#define ID "mmx4.coop"
#define SOUND 0x8001540Cu
#define VOICE_KEY_ON 0x800DFCFCu
#define VAB_OPEN 0x800E4398u
#define VAB_STATUS 0x80166D58u
#define SOUND_END 0x80141EE8u
#define SOUND_RECORDS 0x80141F50u
#define PRIORITY 0x80139234u
#define VOICE_REGS 0x80166D90u
#define VOICE_META 0x8013DCA8u
#define VOICE_DIRTY 0x8013E1D0u
#define QUEUE 0x80175EF0u
#define KEY_MASKS 0x8013BC08u
typedef struct { Mmx4Asset header; uint32_t guest,offset,id; unsigned loaded; } AudioBank;
static AudioBank banks[2][2];
static unsigned sound_call,voice_call,stop_call;
static uint32_t audio_state;
static void read_bytes(uint32_t address,uint8_t *out,unsigned n) {
    for(unsigned i=0;i<n;++i)out[i]=psx_mod_read_byte(address+i);
}
static void write_bytes(uint32_t address,const uint8_t *in,unsigned n) {
    for(unsigned i=0;i<n;++i)psx_mod_write_byte(address+i,in[i]);
}
static uint32_t le32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void copy_header(AudioBank *bank) {
    /* Mod allocations use the private guest aperture, outside hardware RAM. */
    uint32_t at=0;
    for(;at+4<=bank->header.size;at+=4)
        psx_mod_write_word(bank->guest+at,le32(bank->header.data+at));
    for(;at<bank->header.size;++at)
        psx_mod_write_byte(bank->guest+at,bank->header.data[at]);
    /* The record's upper two bits select direct/sequence sound routing.
     * Preserve them; the lower six bits select the original VAB handle. */
    for(at=8;at<bank->offset;at+=4)
        psx_mod_write_byte(bank->guest+at,
            (uint8_t)((bank->header.data[at]&0xC0u)|bank->id));
}
void mmx4_coop_audio_reset(void) {
    memset(banks,0,sizeof banks);sound_call=voice_call=stop_call=0;audio_state=0;
}
int mmx4_coop_audio_load(unsigned character,const uint8_t *file,uint32_t size) {
    if(character>1)return 0;
    static const unsigned members[2]={10,12};
    static const uint32_t types[2]={0x20101u,0x20301u};
    for(unsigned group=0;group<2;++group) {
        AudioBank *bank=&banks[character][group];if(bank->loaded)continue;
        Mmx4Asset head,body;
        if(!mmx4_coop_arc_asset(file,size,members[group],&head) || head.type!=6 ||
           !mmx4_coop_arc_asset(file,size,members[group]+1,&body) || body.type!=types[group] ||
           head.size<40 || head.size>0x2000 || !body.size || body.size%16)return 0;
        uint32_t offset=le32(head.data)&0xFFFFFFu;
        if(offset<8 || offset%4 || offset>head.size-32 ||
           le32(head.data+4)!=body.size || memcmp(head.data+offset,"pBAV",4))return 0;
        bank->id=12u+character*2u+group;
        if(!psx_mod_spu_sample_bank(bank->id,body.data,(uint32_t)body.size))return 0;
        bank->guest=psx_mod_alloc_guest_memory(0x2000,16);if(!bank->guest)return 0;
        bank->header=head;bank->offset=offset;copy_header(bank);bank->loaded=1;
    }
    if(!audio_state) {
        audio_state=psx_mod_alloc_guest_memory(32,16);if(!audio_state)return 0;
        for(unsigned i=0;i<32;++i)psx_mod_write_byte(audio_state+i,0);
    }
    return 1;
}
static int open_bank(CPUState *cpu,AudioBank *bank) {
    if(psx_mod_read_byte(VAB_STATUS+bank->id)==1)return 1;
    copy_header(bank);
    /* Sticky open parses the original header at a logical sample base of 0.
     * It does not allocate or upload SPU RAM. Private KEYON bindings resolve
     * those addresses against the immutable host bank instead. */
    uint32_t a2=cpu->gpr[6];cpu->gpr[6]=0;
    uint32_t result=mmx4_coop_call(cpu,VAB_OPEN,bank->guest+bank->offset,bank->id);
    cpu->gpr[6]=a2;
    if(result!=bank->id)return 0;
    /* Native open leaves status 2 (awaiting body) and the shared transfer
     * gate set. Our body already resides in the immutable bank: publish the
     * same ready status 1 and release that gate without issuing a DMA. */
    psx_mod_write_byte(VAB_STATUS+bank->id,1);
    mmx4_coop_call(cpu,0x800E1A60u,0,0);
    fprintf(stderr,"mmx4.coop: private-audio bank=%u header=%08X ready=1\n",
        bank->id,bank->guest);
    return 1;
}
static int character_sound(CPUState *cpu,uint32_t address) {
    unsigned group=cpu->gpr[4]&0x7Fu;
    if(sound_call || !mmx4_coop_ready() || !mmx4_coop_projected() ||
       psx_mod_local_view_scope() || psx_mod_read_byte(MMX4_PLAY)!=6 ||
       (group!=1 && group!=3))return 0;
    unsigned character=psx_mod_read_byte(MMX4_PLAYER+2);
    if(character>1)return 0;
    AudioBank *bank=&banks[character][group==3];if(!bank->loaded)return 0;
    sound_call=1;
    if(!open_bank(cpu,bank)) {
        sound_call=0;psx_mod_counter_add("mmx4.coop.audio-bank-open-failed",1);
        return mmx4_coop_finish(cpu,(uint32_t)-1);
    }
    uint32_t end=SOUND_END+group*4,records=SOUND_RECORDS+group*4;
    uint32_t saved_end=psx_mod_read_word(end),saved_records=psx_mod_read_word(records);
    psx_mod_write_word(end,bank->guest+bank->offset);
    psx_mod_write_word(records,bank->guest+8);
    uint8_t priority[24];read_bytes(PRIORITY,priority,sizeof priority);
    for(unsigned voice=0;voice<24;++voice)
        psx_mod_write_byte(PRIORITY+voice,psx_mod_read_byte(audio_state+voice));
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    for(unsigned voice=0;voice<24;++voice)
        psx_mod_write_byte(audio_state+voice,psx_mod_read_byte(PRIORITY+voice));
    write_bytes(PRIORITY,priority,sizeof priority);
    psx_mod_write_word(end,saved_end);psx_mod_write_word(records,saved_records);
    sound_call=0;return mmx4_coop_finish(cpu,result);
}
static int voice_key_on(CPUState *cpu,uint32_t address) {
    if(voice_call || !mmx4_coop_ready() || psx_mod_local_view_scope())return 0;
    uint32_t id=cpu->gpr[5];uint32_t bank=0;
    if(id>=12 && id<16 && banks[(id-12)/2][(id-12)%2].loaded)bank=id;
    unsigned voice=cpu->gpr[4];
    if(bank && voice<24u) {
        /* Let the original VAB/tone driver compute pitch, stereo gain and
         * ADSR, then start P2's private voice. Restore its queued hardware
         * record before the next native flush, including P1's pending KEYON.
         * The finite calculation is uncharged so no interrupt can flush this
         * temporary record between calculation and restoration. */
        uint8_t regs[16],meta[0x34],queue[0x48],masks[12];
        uint8_t dirty=psx_mod_read_byte(VOICE_DIRTY+voice);
        read_bytes(VOICE_REGS+voice*16u,regs,sizeof regs);
        read_bytes(VOICE_META+voice*0x34u,meta,sizeof meta);
        read_bytes(QUEUE,queue,sizeof queue);read_bytes(KEY_MASKS,masks,sizeof masks);
        voice_call=1;
        uint32_t result=psx_mod_call_guest_uncharged(cpu,address,cpu->gpr[31],
            voice,id,cpu->gpr[6],cpu->gpr[7],0,NULL);
        if(result!=(uint32_t)-1) {
            uint16_t calculated[8];
            for(unsigned r=0;r<8;++r)calculated[r]=psx_mod_read_half(VOICE_REGS+voice*16u+r*2u);
            calculated[7]=calculated[3];
            unsigned mode=(psx_mod_read_half(0x80166C00u)==0xFFu?1u:0u)|
                (psx_mod_read_byte(0x80166BFCu)&4u?2u:0u);
            if(!psx_mod_spu_private_voice_play(voice,bank,calculated,mode)) {
                result=(uint32_t)-1;psx_mod_counter_add("mmx4.coop.private-voice-failed",1);
            }
        }
        write_bytes(VOICE_REGS+voice*16u,regs,sizeof regs);
        write_bytes(VOICE_META+voice*0x34u,meta,sizeof meta);
        write_bytes(QUEUE,queue,sizeof queue);write_bytes(KEY_MASKS,masks,sizeof masks);
        psx_mod_write_byte(VOICE_DIRTY+voice,dirty);
        voice_call=0;return mmx4_coop_finish(cpu,result);
    }
    /* The original driver queues KEYON for a later SPU update. Bind its
     * explicit voice slot now; the binding survives that delay and rollback. */
    psx_mod_spu_bind_voice_bank(cpu->gpr[4],bank);
    voice_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    if(result==(uint32_t)-1)psx_mod_spu_bind_voice_bank(cpu->gpr[4],0);
    voice_call=0;return mmx4_coop_finish(cpu,result);
}
static int voice_status(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!sound_call || !mmx4_coop_projected())return 0;
    return mmx4_coop_finish(cpu,(psx_mod_spu_private_voice_active()&cpu->gpr[4])?1u:0u);
}
static int private_stop(CPUState *cpu,uint32_t address) {
    if(stop_call!=1 || cpu->gpr[4]>=24u)return 0;
    unsigned voice=cpu->gpr[4];
    if(address==0x800E0090u)psx_mod_spu_private_voice_stop(1u<<voice);
    else psx_mod_spu_private_voice_volume(voice,
        (uint16_t)((int16_t)cpu->gpr[5]*129),(uint16_t)((int16_t)cpu->gpr[6]*129));
    return mmx4_coop_finish(cpu,0);
}
static int character_stop(CPUState *cpu,uint32_t address) {
    unsigned group=cpu->gpr[4]&255u;
    if(stop_call || !mmx4_coop_ready() || psx_mod_local_view_scope())return 0;
    if(group==255u) {
        stop_call=2;
        uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
        for(unsigned voice=0;voice<24;++voice)psx_mod_spu_private_voice_volume(voice,0,0);
        psx_mod_spu_private_voice_stop(0xFFFFFFu);stop_call=0;
        return mmx4_coop_finish(cpu,result);
    }
    if(!mmx4_coop_projected() || (group!=1 && group!=3))return 0;
    unsigned character=psx_mod_read_byte(MMX4_PLAYER+2);
    if(character>1 || !banks[character][group==3].loaded)return 0;
    uint32_t pointer=SOUND_RECORDS+group*4u,saved=psx_mod_read_word(pointer);
    psx_mod_write_word(pointer,banks[character][group==3].guest+8u);stop_call=1;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    stop_call=0;psx_mod_write_word(pointer,saved);
    return mmx4_coop_finish(cpu,result);
}
PSX_MOD_CONSTRUCTOR(mmx4_register_coop_audio) {
    psx_mod_register_function_filter_plugin(ID,SOUND,character_sound);
    psx_mod_register_function_filter_plugin(ID,VOICE_KEY_ON,voice_key_on);
    psx_mod_register_function_filter_plugin(ID,0x80015930u,character_stop);
    psx_mod_register_function_filter_plugin(ID,0x800DBF34u,voice_status);
    psx_mod_register_function_filter_plugin(ID,0x800E0090u,private_stop);
    psx_mod_register_function_filter_plugin(ID,0x800E0E7Cu,private_stop);
}
