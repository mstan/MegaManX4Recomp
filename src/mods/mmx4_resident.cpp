// SLUS-00561 resident ARC producer. The original receive callbacks, member
// decoders, RAM copies, GPU/SPU upload queue and final Pause remain authoritative.
#include "mod_plugins.h"
#include "mod_resident.h"
#include "mod_resident_sector.hpp"
#include "cpu_state.h"
#include "cdrom.h"
#include "crash_trace.h"
#include "mmx4_resident_catalog.h"

namespace {
constexpr const char* Plugin = "mmx4.resident-loading";
constexpr uint32_t Status=0x801406AC, Pending=0x8013BD40;
constexpr uint32_t Remain=0x80137CBC, Lba=0x80137CCC, Packed=0x80137CD8;
constexpr uint32_t Count=0x80137CEC, File=0x80137DD8, Table=0x800F0E18;
const PSXResidentPack* pack = nullptr;
PSXResidentMode2Sector sector;
bool pumping = false;
uint32_t sector_lba = 0;
uint32_t word(uint32_t a) { return psx_mod_read_word(a); }
void call(CPUState* cpu,uint32_t pc,uint32_t a0=0,uint32_t a1=0) {
    psx_mod_call_guest(cpu,pc,0x8000FD00,a0,a1,0,0);
}
bool ram(uint32_t a,uint32_t bytes) {
    return !(a&3) && a>=0x80010000 && a<0x80200000 && bytes<=0x80200000-a;
}
bool loader_caller(CPUState* cpu) {
    return pumping && cpu->gpr[31]>=0x800136B0 && cpu->gpr[31]<0x80014780;
}
int ready(CPUState* cpu,uint32_t) {
    if (!loader_caller(cpu)) return 0;
    if (cpu->gpr[4]!=1 || cpu->gpr[5])
        psx_fatal_halt("X4 resident callback changed its CdReady ABI");
    cpu->gpr[2]=1; return 1;
}
int get_sector(CPUState* cpu,uint32_t) {
    if (!loader_caller(cpu)) return 0;
    const uint32_t words=cpu->gpr[5];
    if (words>585 || !ram(cpu->gpr[4],words*4))
        psx_fatal_halt("X4 resident sector destination is out of bounds");
    const auto* bytes=sector.take_words(words);
    if (!bytes || !psx_mod_dma_write_ram(cpu->gpr[4],bytes,words*4,int(sector_lba)))
        psx_fatal_halt("X4 resident sector read exceeded its verified view");
    cpu->gpr[2]=1; return 1;
}
template<unsigned N> bool caller(const MMX4BlockingCaller (&list)[N],uint32_t ra) {
    for (const auto& c:list) {
        if (c.ra!=ra) continue;
        for (unsigned i=0;i<6;++i) if (word(ra-8+i*4)!=c.words[i]) return false;
        return true;
    }
    return false;
}
int start(CPUState* cpu,uint32_t pc) {
    if (!pack || pumping || !psx_mod_game_started()) return 0;
    const bool packed=pc==0x80013DA8;
    const uint32_t sp=cpu->gpr[29];
    if (!ram(sp,40) || cpu->gpr[31]!=(packed?0x80013D84u:0x80013950u) ||
        !(packed ? caller(MMX4PackedCallers,word(sp+36)) : caller(MMX4RawCallers,word(sp+24)))) {
        psx_mod_counter_add("mmx4.resident.background_original",1); return 0;
    }
    const unsigned file=psx_mod_read_byte(File);
    const uint32_t lba=word(Lba),bytes=word(Remain);
    if (file>=138) return 0;
    const auto& c=MMX4FileContracts[file];
    uint32_t size=0,padded=0;
    const auto* data=psx_resident_file(pack,file,&size,&padded);
    if (!data || c.packed!=packed || word(Packed)!=unsigned(packed) ||
        lba!=psx_resident_file_lba(pack,file) || lba!=c.lba || size!=c.size || bytes!=size ||
        word(Table+file*12)!=lba || word(Table+file*12+4)!=size || word(Table+file*12+8)!=c.first ||
        (!packed && !ram(word(0x80137CC4),(size+3)&~3u)) ||
        cdrom_data_read_active() || cdrom_fmv_stream_pending() ||
        !psx_resident_guest_ranges_match(MMX4LoaderRanges,4,MMX4LoaderSha)) {
        psx_resident_record("mmx4.arc-request",file,lba,bytes,0); return 0;
    }
    // Publish the original start routine's state without issuing ReadN or
    // changing either guest CD deadlines or global host pacing.
    psx_mod_write_word(Count,0);
    call(cpu,0x800E61BC,lba,0x80137CF8); // original CdlLOC
    if (!packed) psx_mod_write_word(Lba,lba-1);
    psx_mod_write_byte(Status,1);
    pumping=true;
    for (uint32_t offset=0;psx_mod_read_byte(Status)==1;offset+=2048) {
        if (offset>=padded || !sector.assign(lba+offset/2048,data+offset,offset+2048==padded))
            psx_fatal_halt("X4 resident loader left its verified ARC");
        sector_lba=lba+offset/2048;
        call(cpu,packed?0x80013E68:0x80013A20);
        if (sector.consumed()!=2060)
            psx_fatal_halt("X4 resident callback did not consume one data sector");
        if (psx_mod_read_byte(Pending)) {
            call(cpu,0x800147AC); // original deferred VRAM/SPU uploads
            call(cpu,0x800EA20C); // original DrawSync(0), before buffer reuse
        }
        psx_mod_counter_add("mmx4.resident.sectors",1);
    }
    pumping=false;
    if (psx_mod_read_byte(Status)!=2 || psx_mod_read_byte(Pending))
        psx_fatal_halt("X4 resident loader violated its completion contract");
    psx_resident_record("mmx4.arc-request",file,lba,bytes,1);
    psx_mod_counter_add("mmx4.resident.requests",1);
    cpu->gpr[2]=1; return 1;
}
void activate() {
    PSXResidentSpec spec{}; spec.struct_size=sizeof spec;
    spec.title="MegaManX4Recomp"; spec.format="slus00561-arc-sectors-v1";
    spec.files=MMX4Files;spec.file_count=138;spec.policy=PSX_RESIDENT_REQUIRE_STOCK;
    pack=psx_resident_prepare(&spec); pumping=false;
}
}
PSX_MOD_CONSTRUCTOR(mmx4_register_resident) {
    psx_mod_register_activation_plugin(Plugin,activate);
    psx_mod_register_function_filter_plugin(Plugin,0x80013968,start);
    psx_mod_register_function_filter_plugin(Plugin,0x80013DA8,start);
    psx_mod_register_function_filter_plugin(Plugin,0x800E5D40,ready);
    psx_mod_register_function_filter_plugin(Plugin,0x800E6158,get_sector);
}
