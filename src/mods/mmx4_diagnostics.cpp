/* Original-input boundary recorder. Host text/files are presentation only;
 * replay is explicitly requested and refuses network sessions. */
#include "mod_plugins.h"
#include "mod_netplay.h"
#include "mod_runtime.h"
#include "cpu_state.h"
#include "execution_profile.h"
#include "host_osd.h"
#include "memcard.h"
#include "savestate.h"
#include "sio.h"
extern "C" {
#include "mmx4_coop_internal.h"
}
#include "mmx4_diagnostics.h"
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#ifndef MMX4_DIAGNOSTIC_BUILD_ID
#define MMX4_DIAGNOSTIC_BUILD_ID "diagnostic-test-build"
#endif

namespace {
constexpr uint32_t PAD=0x80166C08u, FRAME=0x80141BD8u;
using Row=std::array<int64_t,18>;
std::ofstream recording;
std::ifstream replay;
std::array<uint16_t,2> replay_buttons{0xFFFFu,0xFFFFu};
uint32_t sequence;
bool started,inside,replaying,replay_failed;
bool owns_sources;
std::string replay_path;

std::string plan_identity() {
    auto portable=PSXRecompV4::mod_runtime_plan_fingerprint_portable();
    return portable.empty()?PSXRecompV4::mod_runtime_fingerprint():portable;
}

int controller_source(unsigned seat,PSXModControllerState *state) {
    state->buttons=replay_buttons[seat];state->analog=0;
    state->lx=state->ly=state->rx=state->ry=0x80;
    return 1;
}
int first_controller(PSXModControllerState *state) {return controller_source(0,state);}
int second_controller(PSXModControllerState *state) {return controller_source(1,state);}
void release_sources() {
    if(!owns_sources)return;
    psx_mod_set_controller_source(0,nullptr);psx_mod_set_controller_source(1,nullptr);
    owns_sources=false;
}

uint32_t state_hash() {
    uint32_t h=2166136261u;
    auto bytes=[&](uint32_t address,unsigned size) {
        for(unsigned i=0;i<size;++i)h=(h^psx_mod_read_byte(address+i))*16777619u;
    };
    bytes(MMX4_PLAY,0x61);bytes(MMX4_PLAYER,0xE4);bytes(0x8013E2E8u,2);
    if(mmx4_coop_ready()) {
        const uint8_t *p=mmx4_coop_second_body();
        for(unsigned i=0;i<0xE4;++i)h=(h^p[i])*16777619u;
    }
    return h;
}
uint16_t live_input(unsigned seat) {
    if(mmx4_coop_ready()) {
        uint16_t native=mmx4_coop_input(seat);
        return (uint16_t)~(uint16_t)((native<<8)|(native>>8));
    }
    return sio_get_pad_buttons_slot((int)seat);
}
Row sample() {
    const uint8_t *p2=mmx4_coop_second_body();
    bool projected=mmx4_coop_projected()!=0;
    const uint8_t *p1=mmx4_coop_first_body();
    auto first_byte=[&](unsigned at) {
        return projected?p1[at]:psx_mod_read_byte(MMX4_PLAYER+at);
    };
    auto first_half=[&](unsigned at) {
        return (int16_t)(first_byte(at)|(first_byte(at+1)<<8));
    };
    return {sequence,psx_mod_read_word(FRAME),live_input(0),live_input(1),
        psx_mod_read_half(PAD),psx_mod_read_half(PAD+2),psx_mod_read_half(PAD+4),
        psx_mod_read_word(MMX4_PLAY),psx_mod_read_half(MMX4_PLAY+0x0C),
        projected?p1[2]:psx_mod_read_byte(MMX4_PLAY+0x43),psx_mod_read_byte(MMX4_PLAY+0x59),
        first_byte(0x5C)&127u,p2[0x5C]&127u,first_half(10),first_half(14),
        (int16_t)(p2[10]|p2[11]<<8),(int16_t)(p2[14]|p2[15]<<8),state_hash()};
}
bool parse_row(const std::string& line,Row& row) {
    std::string fields=line;
    for(char& c:fields)if(c==',')c=' ';
    std::istringstream in(fields);
    for(auto& field:row)if(!(in>>field))return false;
    in>>std::ws;
    if(!in.eof() || row[0]<0 || row[1]<0 || row[17]<0 || row[17]>UINT32_MAX)return false;
    for(unsigned i=2;i<=6;++i)if(row[i]<0 || row[i]>65535)return false;
    return true;
}
void fail(const char *reason) {
    fprintf(stderr,"mmx4.diagnostics: replay stopped at input %u: %s\n",sequence,reason);
    fflush(stderr);
    psx_mod_counter_add("mmx4.diagnostics.replay-failed",1);
    replay.close();replaying=false;replay_failed=true;
    release_sources();
}
void begin() {
    started=true;
    if(!replay_path.empty()) {
        if(psx_mod_netplay_is_active()) {fail("offline replay only");return;}
        replay.open(std::filesystem::u8path(replay_path));
        std::string format,execution,plan,build;
        if(!std::getline(replay,format) || format!="# mmx4-input-v2" ||
           !std::getline(replay,execution) || execution!=std::string("# execution ")+PSX_EXECUTION_ID ||
           !std::getline(replay,plan) || plan!="# plan "+plan_identity() ||
           !std::getline(replay,build) || build!="# build " MMX4_DIAGNOSTIC_BUILD_ID) {
            fail("recording version/build/mod settings do not match");return;
        }
        replaying=true;
        fprintf(stderr,"mmx4.diagnostics: offline input replay started\n");
        fflush(stderr);
        return;
    }
    try {
        const char *root=savestate_root_dir();
        if(!root || !root[0])throw std::runtime_error("save root unavailable");
        auto stamp=std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto folder=std::filesystem::u8path(root)/"diagnostics"/("session-"+std::to_string(stamp));
        if(!std::filesystem::create_directories(folder))throw std::runtime_error("session directory unavailable");
        recording.open(folder/"inputs.csv",std::ios::out|std::ios::trunc);
        if(!recording)throw std::runtime_error("cannot create input recording");
        recording<<"# mmx4-input-v2\n# execution "<<PSX_EXECUTION_ID
            <<"\n# plan "<<plan_identity()<<"\n# build " MMX4_DIAGNOSTIC_BUILD_ID "\n";
        const auto state=PSXRecompV4::mod_runtime_root()/"state.toml";
        if(std::filesystem::exists(state))
            std::filesystem::copy_file(state,folder/"mods-state.toml");
        else {
            // An untouched mod catalog uses defaults without a state file.
            // Capture that choice explicitly so fresh installs can replay.
            std::ofstream defaults(folder/"mods-state.toml");
            defaults<<"format_version = 2\n";
            if(!defaults)throw std::runtime_error("cannot capture default mod choices");
        }
        /* Copy initial cards, including explicit absence, before the title
         * can change either. Replay launchers bind copies of these files. */
        for(unsigned slot=0;slot<2;++slot) {
            std::array<uint8_t,MEMCARD_SIZE> card{};
            if(memcard_is_present((int)slot)) {
                if(memcard_export_raw((int)slot,card.data()))throw std::runtime_error("cannot capture initial card");
                std::ofstream out(folder/("card"+std::to_string(slot+1)+".mcd"),std::ios::binary);
                out.write((const char*)card.data(),card.size());
                if(!out)throw std::runtime_error("cannot write initial card");
            }else {
                std::ofstream out(folder/("card"+std::to_string(slot+1)+".absent"));
                if(!out)throw std::runtime_error("cannot mark absent card");
            }
        }
        recording.flush();
        fprintf(stderr,"mmx4.diagnostics: recording %s\n",folder.u8string().c_str());
        fflush(stderr);
    }catch(const std::exception& error) {
        recording.close();
        fprintf(stderr,"mmx4.diagnostics: recording unavailable: %s\n",error.what());
        psx_mod_counter_add("mmx4.diagnostics.recording-failed",1);
    }
}
std::string input_line(unsigned seat,uint16_t buttons) {
    static const char *names[]={"Se","L3","R3","St","U","R","D","L","L2","R2","L1","R1","T","O","X","[]"};
    std::string text="P"+std::to_string(seat+1)+":";
    bool held=false;
    for(unsigned bit=0;bit<16;++bit)if(!(buttons&(1u<<bit))) {
        text+=' ';text+=names[bit];held=true;
    }
    if(!held)text+=" -";
    text+="  F"+std::to_string(sequence);
    if(replay_failed)text+=" ERROR";
    else if(replaying)text+=" PLAY";
    else if(recording.is_open())text+=" REC";
    return text;
}
int inputs(CPUState *cpu,uint32_t address) {
    if(inside || psx_mod_local_view_scope())return 0;
    inside=true;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    mmx4_coop_development_mask_pad();
    if(!started)begin();
    Row row=sample();
    if(replaying) {
        std::string line;
        Row expected{};
        if(!std::getline(replay,line)) {
            if(replay.eof()) {
                replay.close();replaying=false;
                psx_mod_counter_add("mmx4.diagnostics.replay-complete",1);
                fprintf(stderr,"mmx4.diagnostics: input replay complete (%u inputs verified)\n",sequence);
                fflush(stderr);release_sources();
            }else fail("read error");
        }else if(line.compare(0,2,"N ") || !parse_row(line.substr(2),expected))fail("malformed native input row");
        else if(expected[0]!=sequence || expected[1]!=row[1] || expected[17]!=row[17]) {
            fprintf(stderr,"mmx4.diagnostics: expected frame/hash %lld/%08X, got %lld/%08X\n",
                (long long)expected[1],(unsigned)expected[17],(long long)row[1],(unsigned)row[17]);
            fail("game state diverged; replay is not a successful reproduction");
        }else {
            row=expected;
            for(unsigned i=0;i<3;++i)psx_mod_write_half(PAD+i*2u,(uint16_t)row[4+i]);
            replay_buttons={(uint16_t)row[2],(uint16_t)row[3]};
            psx_mod_counter_add("mmx4.diagnostics.replayed-inputs",1);
        }
    }else if(recording.is_open()) {
        recording<<"N ";
        for(unsigned i=0;i<row.size();++i)recording<<(i?",":"")<<row[i];
        recording<<'\n';
        if(!(sequence%60))recording.flush();
        if(!recording) {
            recording.close();psx_mod_counter_add("mmx4.diagnostics.recording-failed",1);
            fprintf(stderr,"mmx4.diagnostics: recording write failed\n");
        }
    }
    auto first=input_line(0,(uint16_t)row[2]),second=input_line(1,(uint16_t)row[3]);
    host_osd_set_diagnostics(first.c_str(),second.c_str());
    ++sequence;inside=false;
    return mmx4_coop_finish(cpu,result);
}
void activate() {
    release_sources();
    recording.close();replay.close();recording.clear();replay.clear();
    started=inside=replaying=replay_failed=false;sequence=0;
    replay_buttons={0xFFFFu,0xFFFFu};
    const char *path=getenv("MMX4_DIAGNOSTIC_REPLAY");
    replay_path=path?path:"";
    /* Present digital pads during startup and between decoded game inputs;
     * playback also works in a hidden run without physical controllers. */
    if(!replay_path.empty() && !psx_mod_netplay_is_active()) {
        psx_mod_set_controller_source(0,first_controller);
        psx_mod_set_controller_source(1,second_controller);owns_sources=true;
    }
    host_osd_set_diagnostics(nullptr,nullptr);
}
int coop_inputs(CPUState *cpu,uint32_t address) {
    return psx_mod_netplay_is_active()?inputs(cpu,address):0;
}
}
extern "C" int mmx4_diagnostics_resolve_input(unsigned seat,uint16_t *buttons) {
    if(inside || !started || seat>=2 || !buttons)return 0;
    if(replaying) {
        std::string line;std::array<unsigned,3> event{};char trailing;
        if(!std::getline(replay,line) ||
           sscanf(line.c_str(),"C %u %u %u %c",&event[0],&event[1],&event[2],&trailing)!=3 ||
           event[0]!=sequence || event[1]!=seat || event[2]>65535) {
            fail("co-op input read order diverged");return 0;
        }
        *buttons=(uint16_t)event[2];replay_buttons[seat]=*buttons;
    }else if(recording.is_open())recording<<"C "<<sequence<<' '<<seat<<' '<<*buttons<<'\n';
    return replaying?1:0;
}
extern "C" void mmx4_diagnostics_reset(void) {activate();}
PSX_MOD_CONSTRUCTOR(mmx4_register_diagnostics) {
    psx_mod_register_activation_plugin("mmx4.diagnostics",activate);
    psx_mod_register_function_filter_plugin("mmx4.diagnostics",0x80012328u,inputs);
    /* The executable's netplay profile installs only the co-op plugin.
     * This callback is also part of that fixed playtest profile. */
    psx_mod_register_function_filter_plugin("mmx4.coop",0x80012328u,coop_inputs);
}
