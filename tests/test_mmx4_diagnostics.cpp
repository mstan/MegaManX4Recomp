/* Verify replay boundaries, including a P2 input changing mid-frame. Native
 * gameplay replay is a separate original-disc check. No window is created. */
extern "C" {
#include "mmx4_coop_internal.h"
}
#include "host_osd.h"
#include "memcard.h"
#include "mod_netplay.h"
#include "mod_runtime.h"
#include "savestate.h"
#include "sio.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <cstdlib>
#include <memory>
static std::array<uint8_t,0x200000> memory;
static std::array<uint8_t,0xE4> partner;
static std::array<uint16_t,2> pads{0xFFFF,0xFFFF};
static std::map<std::string,unsigned> counters;
static std::string save_root,shown[2];
static bool online;
static bool projected_context;
static std::array<uint8_t,0xE4> canonical_first;
static std::array<PSXModControllerSource,2> sources{};
uint8_t psx_mod_read_byte(uint32_t a) {return memory[a&0x1FFFFF];}
void psx_mod_write_byte(uint32_t a,uint8_t v) {memory[a&0x1FFFFF]=v;}
uint16_t psx_mod_read_half(uint32_t a) {return psx_mod_read_byte(a)|(psx_mod_read_byte(a+1)<<8);}
void psx_mod_write_half(uint32_t a,uint16_t v) {psx_mod_write_byte(a,(uint8_t)v);psx_mod_write_byte(a+1,(uint8_t)(v>>8));}
uint32_t psx_mod_read_word(uint32_t a) {return psx_mod_read_half(a)|(uint32_t(psx_mod_read_half(a+2))<<16);}
void psx_mod_write_word(uint32_t a,uint32_t v) {psx_mod_write_half(a,(uint16_t)v);psx_mod_write_half(a+2,(uint16_t)(v>>16));}
int psx_mod_register_activation_plugin(const char*,PSXModActivationCallback) {return 1;}
int psx_mod_register_function_filter_plugin(const char*,uint32_t,PSXModFunctionFilterCallback) {return 1;}
int psx_mod_local_view_scope(void) {return 0;}
int psx_mod_netplay_is_active(void) {return online;}
int psx_mod_set_controller_source(uint32_t seat,PSXModControllerSource source) {sources.at(seat)=source;return 1;}
void psx_mod_counter_add(const char* name,uint32_t n) {counters[name]+=n;}
uint16_t sio_get_pad_buttons_slot(int slot) {return pads.at(slot);}
int memcard_is_present(int slot) {return slot==0;}
int memcard_export_raw(int,uint8_t *out) {memset(out,0xAB,MEMCARD_SIZE);return 0;}
const char *savestate_root_dir(void) {return save_root.c_str();}
void host_osd_set_diagnostics(const char *a,const char *b) {shown[0]=a?a:"";shown[1]=b?b:"";}
namespace PSXRecompV4 {
std::string mod_runtime_plan_fingerprint_portable() {return {};}
const std::string& mod_runtime_fingerprint() {static const std::string fp="fixture-plan";return fp;}
const std::filesystem::path& mod_runtime_root() {static const auto root=std::filesystem::path(save_root)/"mods";return root;}
}
int mmx4_coop_ready(void) {return 1;}
int mmx4_coop_projected(void) {return projected_context;}
uint8_t *mmx4_coop_first_body(void) {return canonical_first.data();}
void mmx4_coop_development_mask_pad(void) {}
uint8_t *mmx4_coop_second_body(void) {return partner.data();}
int mmx4_coop_finish(CPUState *cpu,uint32_t v) {cpu->gpr[2]=v;return 1;}
uint32_t mmx4_coop_call(CPUState*,uint32_t,uint32_t,uint32_t) {
    uint16_t held=(uint16_t)~pads[0];held=(uint16_t)((held<<8)|(held>>8));
    uint16_t old=psx_mod_read_half(0x80166C08u);
    psx_mod_write_half(0x80166C08u,held);psx_mod_write_half(0x80166C0Au,old);
    psx_mod_write_half(0x80166C0Cu,(uint16_t)(held&~old));return 77;
}
uint16_t mmx4_coop_input(unsigned seat);
#include "../src/mods/mmx4_diagnostics.cpp"
uint16_t mmx4_coop_input(unsigned seat) {
    uint16_t buttons=pads.at(seat);mmx4_diagnostics_resolve_input(seat,&buttons);
    uint16_t native=(uint16_t)~buttons;return (uint16_t)((native<<8)|(native>>8));
}
#define CHECK(x) do {if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}} while(0)
static void env(const char *v) {
#ifdef _WIN32
    _putenv_s("MMX4_DIAGNOSTIC_REPLAY",v);
#else
    setenv("MMX4_DIAGNOSTIC_REPLAY",v,1);
#endif
}
int main(int argc,char **argv) {
    save_root=argc>1?argv[1]:"diagnostic-test-output";
    save_root+="/"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(save_root);CPUState cpu{};env("");activate();
    CHECK(!sources[0] && !sources[1]);
    std::filesystem::create_directories(PSXRecompV4::mod_runtime_root());
    {std::ofstream state(PSXRecompV4::mod_runtime_root()/"state.toml");state<<"format_version = 2\n";}
    auto initial=std::make_unique<decltype(memory)>(memory);
    pads={0xFFEF,0xBFFF};CHECK(inputs(&cpu,0x80012328u));
    CHECK(shown[0].find("U")!=std::string::npos && shown[1].find("X")!=std::string::npos);
    uint16_t raw=0xFFEF;mmx4_diagnostics_resolve_input(0,&raw);
    raw=0xFF7F;mmx4_diagnostics_resolve_input(1,&raw);
    psx_mod_write_word(FRAME,1);pads={0xFFFF,0xFFF7};CHECK(inputs(&cpu,0x80012328u));
    raw=0xFFF7;mmx4_diagnostics_resolve_input(1,&raw);
    recording.flush();auto final=std::make_unique<decltype(memory)>(memory);recording.close();
    std::filesystem::path file;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(save_root)/"diagnostics"))
        if(std::filesystem::exists(entry.path()/"inputs.csv"))file=entry.path()/"inputs.csv";
    CHECK(!file.empty() && std::filesystem::file_size(file)>100);
    CHECK(std::filesystem::file_size(file.parent_path()/"card1.mcd")==MEMCARD_SIZE);
    CHECK(std::filesystem::exists(file.parent_path()/"card2.absent"));
    CHECK(std::filesystem::exists(file.parent_path()/"mods-state.toml"));
    memory=*initial;env(file.string().c_str());activate();pads={0xFFFF,0xFFFF};
    CHECK(sources[0] && sources[1]);
    CHECK(inputs(&cpu,0x80012328u) && replaying && !replay_failed);
    raw=0xFFFF;CHECK(mmx4_diagnostics_resolve_input(0,&raw) && raw==0xFFEF);
    raw=0xFFFF;CHECK(mmx4_diagnostics_resolve_input(1,&raw) && raw==0xFF7F);
    psx_mod_write_word(FRAME,1);CHECK(inputs(&cpu,0x80012328u));
    raw=0xFFFF;CHECK(mmx4_diagnostics_resolve_input(1,&raw) && raw==0xFFF7);
    CHECK(memory==*final && counters["mmx4.diagnostics.replayed-inputs"]==2);
    psx_mod_write_word(FRAME,2);CHECK(inputs(&cpu,0x80012328u) && !replaying && !replay_failed);
    CHECK(counters["mmx4.diagnostics.replay-complete"]==1);
    // Divergence must be surfaced rather than silently accepting the trace.
    memory=*initial;activate();psx_mod_write_byte(MMX4_PLAY+0x59,1);
    inputs(&cpu,0x80012328u);CHECK(replay_failed && !replaying);
    memory=*initial;activate();inputs(&cpu,0x80012328u);
    raw=0xFFFF;mmx4_diagnostics_resolve_input(1,&raw);CHECK(replay_failed);
    memory=*initial;activate();online=true;inputs(&cpu,0x80012328u);CHECK(replay_failed && !replaying);
    env("");activate();CHECK(!sources[0] && !sources[1]);
    // First-time installs have no mod state file. Their defaults still need
    // a portable starting-mod capture for the replay launcher.
    std::filesystem::remove(PSXRecompV4::mod_runtime_root()/"state.toml");
    begin();recording.close();
    unsigned captures=0;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(save_root)/"diagnostics")) {
        if(std::filesystem::exists(entry.path()/"inputs.csv")) {
            CHECK(std::filesystem::exists(entry.path()/"mods-state.toml"));
            ++captures;
        }
    }
    CHECK(captures==2);
    // A P2 callback temporarily occupies PLAYER/PLAY; labels must still name
    // the actual P1 rather than claiming the campaign switched characters.
    projected_context=true;canonical_first[2]=0;canonical_first[0x5C]=17;
    canonical_first[10]=123;canonical_first[14]=77;
    psx_mod_write_byte(MMX4_PLAY+0x43,1);psx_mod_write_byte(MMX4_PLAYER+0x5C,3);
    psx_mod_write_half(MMX4_PLAYER+10,400);psx_mod_write_half(MMX4_PLAYER+14,200);
    auto projected_row=sample();
    CHECK(projected_row[9]==0 && projected_row[11]==17 && projected_row[13]==123 && projected_row[14]==77);
    projected_context=false;
    printf("diagnostic recording/replay boundary checks passed\n");return 0;
}
