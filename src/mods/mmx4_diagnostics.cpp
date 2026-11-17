/* Original-input boundary recorder. Host text/files are presentation only;
 * replay is explicitly requested and refuses network sessions. */
#include "mod_plugins.h"
#include "mod_netplay.h"
#include "mod_snapshot.h"
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
#include "mmx4_diagnostic_paths.h"
#include "mmx4_diagnostic_writer.h"
#include <array>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#ifndef MMX4_DIAGNOSTIC_BUILD_ID
#define MMX4_DIAGNOSTIC_BUILD_ID "diagnostic-test-build"
#endif
#ifndef MMX4_DIAGNOSTIC_RELEASE_VERSION
#define MMX4_DIAGNOSTIC_RELEASE_VERSION "unknown-development-version"
#endif

namespace {
constexpr uint32_t PAD=0x80166C08u, FRAME=0x80141BD8u;
using mmx4_diagnostic::Row;
using mmx4_diagnostic::State;
mmx4_diagnostic::Writer<> recording;
std::ifstream replay;
std::array<uint16_t,2> replay_buttons{0xFFFFu,0xFFFFu};
uint32_t sequence;
bool started,inside,replaying,replay_failed;
bool owns_sources;
bool recording_failed;
bool recording_enabled,display_enabled;
std::string replay_path;
std::string baseline_path;
bool baseline_loaded;
std::vector<uint8_t> expected_baseline;
std::vector<uint8_t> pending_baseline;
mmx4_diagnostic::Mode recorded_mode;
bool mode_recorded;

mmx4_diagnostic::Mode effective_mode() {
    bool coop=mod_runtime_snapshot_package_active("mmx4.coop")!=0;
    unsigned campaign=mmx4_coop_projected()?mmx4_coop_first_body()[2]:psx_mod_read_byte(MMX4_PLAY+0x43);
    return {psx_mod_netplay_is_active()!=0,coop,coop && mmx4_coop_split_views()!=0,
        coop && mmx4_coop_ready()!=0,campaign};
}
void json_string(std::ostream& out,const char *value) {
    static const char hex[]="0123456789abcdef";
    out<<'"';
    for(const unsigned char *p=(const unsigned char*)value;*p;++p) {
        if(*p=='"' || *p=='\\')out<<'\\'<<char(*p);
        else if(*p<32)out<<"\\u00"<<hex[*p>>4]<<hex[*p&15];
        else out<<char(*p);
    }
    out<<'"';
}
void option_metadata(std::ostream& out) {
    static const char *keys[]={"local_cameras","cameras","hud_layout","boss_doors",
        "boss_scaling","boss_cooldowns","simultaneous_menus","separate_etanks","both_story_bosses",
        "ultimate_armor_x","black_zero"};
    out<<'{';bool first=true;
    for(const char *key:keys) {
        out<<(first?"":", ");first=false;json_string(out,key);out<<": ";
        char value[128]{};
        if(!psx_mod_option_value("mmx4.coop","coop",key,value,sizeof value))out<<"null";
        else if(!strcmp(value,"true") || !strcmp(value,"false"))out<<value;
        else json_string(out,value);
    }
    char menus[16]{},tanks[16]{};
    bool paired=!psx_mod_option_value("mmx4.coop","coop","simultaneous_menus",menus,sizeof menus) || strcmp(menus,"false");
    bool separate=psx_mod_option_value("mmx4.coop","coop","separate_etanks",tanks,sizeof tanks) && !strcmp(tanks,"true");
    if(!first)out<<',';
    bool effective=mod_runtime_snapshot_package_active("mmx4.coop") && (paired || separate);
    out<<"\"separate_etanks_effective\": "<<(effective?"true":"false")<<'}';
}

#ifndef MMX4_DIAGNOSTIC_TEST
std::filesystem::path application_directory() {
    return mmx4_diagnostic_application_directory();
}
#endif

void recording_location(const std::string& message) {
    /* Independent of configured memory-card paths, mods paths and cwd. */
    try {
        auto root=application_directory();
        if(root.empty())return;
        std::ofstream out(root/"diagnostics-location.txt",std::ios::trunc);
        out<<message<<"\n";
    }catch(...) { /* Recording must not depend on this convenience pointer. */ }
}

void close_recording() {
    recording.close();
    if(!recording && !recording_failed) {
        recording_failed=true;
        recording_location(std::string("Recording FAILED: ")+recording.error_text());
        fprintf(stderr,"mmx4.diagnostics: %s\n",recording.error_text());
        fflush(stderr);
    }
}

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

unsigned capture_state(State& state) {
    unsigned size=0;
    auto copy=[&](uint32_t address,unsigned count) {
        for(unsigned i=0;i<count;++i)state[size++]=psx_mod_read_byte(address+i);
    };
    copy(MMX4_PLAY,0x61);copy(MMX4_PLAYER,0xE4);copy(0x8013E2E8u,2);
    if(mmx4_coop_ready()) {
        const uint8_t *p=mmx4_coop_second_body();
        std::copy_n(p,0xE4,state.begin()+size);size+=0xE4;
    }
    return size;
}
uint32_t state_hash() {
    State state{};unsigned size=capture_state(state);
    return mmx4_diagnostic::hash(state,size);
}
uint16_t live_input(unsigned seat) {
    if(mmx4_coop_ready()) {
        uint16_t native=mmx4_coop_input(seat);
        return (uint16_t)~(uint16_t)((native<<8)|(native>>8));
    }
    return sio_get_pad_buttons_slot((int)seat);
}
Row sample(bool with_hash=true) {
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
        (int16_t)(p2[10]|p2[11]<<8),(int16_t)(p2[14]|p2[15]<<8),with_hash?state_hash():0u};
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
        if(psx_mod_netplay_is_active()) {
            fail("offline replay only");return;
        }
        replay.open(std::filesystem::u8path(replay_path));
        std::string format,execution,plan,build;
        if(!std::getline(replay,format) || !std::getline(replay,execution) ||
           !std::getline(replay,plan) || !std::getline(replay,build)) {
            fail("recording header is incomplete");return;
        }
        if(format!="# mmx4-input-v2" ||
           execution!=std::string("# execution ")+PSX_EXECUTION_ID ||
           plan!="# plan "+plan_identity() || build!="# build " MMX4_DIAGNOSTIC_BUILD_ID) {
            fprintf(stderr,"mmx4.diagnostics: replay header mismatches: format=%d execution=%d plan=%d build=%d\n",
                format!="# mmx4-input-v2",execution!=std::string("# execution ")+PSX_EXECUTION_ID,
                plan!="# plan "+plan_identity(),build!="# build " MMX4_DIAGNOSTIC_BUILD_ID);
            fprintf(stderr,"mmx4.diagnostics: active plan %s\n",plan_identity().c_str());
            fail("recording version/build/mod settings do not match");return;
        }
        replaying=true;
        fprintf(stderr,"mmx4.diagnostics: offline input replay started\n");
        fflush(stderr);
        return;
    }
    try {
        // Register after startup logging is initialized, so accepted records
        // drain and final close errors are reported before that logger exits.
        // The writer destructor remains an idempotent early-exit fallback.
        static const bool cleanup_registered=std::atexit(close_recording)==0;
        if(!cleanup_registered)throw std::runtime_error("cannot register diagnostic shutdown");
        auto stamp=std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto folder=application_directory()/"saves"/"diagnostics"/("session-"+std::to_string(stamp));
        if(!std::filesystem::create_directories(folder))throw std::runtime_error("session directory unavailable");
        recording.open(folder/"inputs.csv",std::string("# mmx4-input-v2\n# execution ")+PSX_EXECUTION_ID+
            "\n# plan "+plan_identity()+"\n# build " MMX4_DIAGNOSTIC_BUILD_ID "\n",folder/"mode-changes.csv");
        if(!recording)throw std::runtime_error(recording.error_text());
        mode_recorded=false;
        std::ofstream metadata(folder/"session.json");
        metadata<<"{\n  \"format_version\": 1,\n  \"input_format\": \"mmx4-input-v2\",\n"
            <<"  \"writer\": \"bounded-background-v1\",\n  \"flush_interval_ms\": 100,\n"
            <<"  \"netplay\": "<<(psx_mod_netplay_is_active()?"true":"false")
            <<",\n  \"execution\": \""<<PSX_EXECUTION_ID
            <<"\",\n  \"build\": \"" MMX4_DIAGNOSTIC_BUILD_ID "\",\n  \"game_version\": ";
        json_string(metadata,MMX4_DIAGNOSTIC_RELEASE_VERSION);
        metadata<<",\n  \"initial_mode\": ";mmx4_diagnostic::mode_json(metadata,effective_mode());
        metadata<<",\n  \"mode_history\": \"mode-changes.csv\",\n  \"coop_options\": ";
        option_metadata(metadata);
        if(!pending_baseline.empty()) {
            std::ofstream baseline(folder/"baseline.pst",std::ios::binary);
            baseline.write((const char*)pending_baseline.data(),pending_baseline.size());
            baseline.close();
            if(!baseline)throw std::runtime_error("cannot capture restored-state baseline");
            metadata<<",\n  \"baseline\": \"baseline.pst\"";
        }
        metadata<<"\n}\n";
        metadata.close();
        if(!metadata)throw std::runtime_error("cannot capture session metadata");
        const auto settings=PSXRecompV4::mod_runtime_root().parent_path()/"settings.toml";
        if(std::filesystem::exists(settings))std::filesystem::copy_file(settings,folder/"settings.toml");
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
        recording_location("Recording: "+std::filesystem::absolute(folder).make_preferred().u8string()+
            "\nAfter reproducing the bug, quit normally and zip this entire session folder."
            "\nIt includes starting game saves; do not include the game disc or BIOS.");
        fprintf(stderr,"mmx4.diagnostics: recording %s\n",folder.u8string().c_str());
        fflush(stderr);
        pending_baseline.clear();
    }catch(const std::exception& error) {
        recording.close();
        recording_failed=true;
        recording_location(std::string("Recording FAILED: ")+error.what());
        fprintf(stderr,"mmx4.diagnostics: recording unavailable: %s\n",error.what());
        psx_mod_counter_add("mmx4.diagnostics.recording-failed",1);
    }
}
bool netplay_recording_enabled() {
    char value[8];
    return psx_mod_netplay_is_active() &&
        psx_mod_option_value("mmx4.coop","coop","diagnostic_recording",value,sizeof value) &&
        !strcmp(value,"true");
}
void state_loaded(const uint8_t *image,uint32_t size) {
    /* Co-op also registers this observer for the fixed netplay profile.
     * Offline co-op alone must not start recording when Diagnostics is off:
     * its native input hook is absent, producing only unplayable C rows. */
    if(psx_mod_netplay_is_active() ? !netplay_recording_enabled() :
       !recording_enabled)return;
    if(!replay_path.empty()) {
        // PSX_LOAD_SLOT restores a segment's baseline before its first input.
        // A later manual jump cannot be reconciled with that segment's rows.
        if(started)fail("unexpected state load during diagnostic playback");
        else if(baseline_path.empty() || expected_baseline.size()!=size ||
                !image || !std::equal(expected_baseline.begin(),expected_baseline.end(),image)) {
            started=true;fail("restored image does not match required diagnostic baseline");
        }else {
            baseline_loaded=true;
            fprintf(stderr,"mmx4.diagnostics: replay baseline restored\n");
            fflush(stderr);
            begin(); // A restored frame can read P2 before the next native pad poll.
        }
        return;
    }
    if(psx_mod_netplay_is_active()) {
        if(recording.is_open() && !recording_failed)recording.state_load();
        return;
    }
    close_recording();recording.clear();
    sequence=0;started=false;recording_failed=false;
    try {
        if(!image || !size)throw std::runtime_error("missing restored-state baseline");
        pending_baseline.assign(image,image+size);
        begin();
        psx_mod_counter_add("mmx4.diagnostics.state-segments",1);
    }catch(const std::exception& error) {
        recording_failed=true;started=true;
        recording_location(std::string("Recording FAILED: ")+error.what());
        fprintf(stderr,"mmx4.diagnostics: state segment failed: %s\n",error.what());
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
    if(replay_failed || recording_failed)text+=" ERROR";
    else if(replaying)text+=" PLAY";
    else if(recording.is_open())text+=" REC";
    return text;
}
int inputs(CPUState *cpu,uint32_t address) {
    if(inside || psx_mod_local_view_scope())return 0;
    inside=true;
    uint32_t result=mmx4_coop_call(cpu,address,cpu->gpr[4],cpu->gpr[5]);
    mmx4_coop_development_mask_pad();
    /* Auto-loading can be deferred until a safe guest boundary. Never admit
     * baseline segment rows from a normal boot while that load is pending. */
    if(!started && !baseline_path.empty() && !baseline_loaded) {
        inside=false;return mmx4_coop_finish(cpu,result);
    }
    const bool collect=recording_enabled || netplay_recording_enabled();
    if(collect && !started)begin();
    Row row=sample(replaying);
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
    }else if(recording.is_open() && !recording_failed) {
        const auto mode=effective_mode();
        if(!mode_recorded || !(mode==recorded_mode)) {
            recording.mode(sequence,(unsigned)row[1],mode);
            recorded_mode=mode;mode_recorded=true;
        }
        State state{};unsigned size=capture_state(state);
        recording.native(row,state,size);
        if(!recording) {
            psx_mod_counter_add("mmx4.diagnostics.recording-failed",1);
            recording_failed=true;recording_location(std::string("Recording FAILED: ")+recording.error_text());
            fprintf(stderr,"mmx4.diagnostics: %s\n",recording.error_text());
        }
    }
    if(display_enabled && !psx_mod_netplay_is_active()) {
        auto first=input_line(0,(uint16_t)row[2]),second=input_line(1,(uint16_t)row[3]);
        host_osd_set_diagnostics(first.c_str(),second.c_str());
    }
    ++sequence;inside=false;
    return mmx4_coop_finish(cpu,result);
}
void reset_recording() {
    release_sources();
    close_recording();replay.close();recording.clear();replay.clear();
    started=inside=replaying=replay_failed=recording_failed=false;sequence=0;
    mode_recorded=false;
    pending_baseline.clear();
    expected_baseline.clear();baseline_loaded=false;
    replay_buttons={0xFFFFu,0xFFFFu};
    const char *path=getenv("MMX4_DIAGNOSTIC_REPLAY");
    replay_path=recording_enabled && path?path:"";
    const char *baseline=getenv("MMX4_DIAGNOSTIC_BASELINE");
    baseline_path=recording_enabled && baseline?baseline:"";
    recording_location(replay_path.empty()?"Waiting for game inputs. Playtest Diagnostics must be enabled."
        :"Offline diagnostic replay active; no new recording is created.");
    /* Present digital pads during startup and between decoded game inputs;
     * playback also works in a hidden run without physical controllers. */
    if(!replay_path.empty() && !psx_mod_netplay_is_active()) {
        if(!baseline_path.empty()) {
            std::ifstream in(std::filesystem::u8path(baseline_path),std::ios::binary|std::ios::ate);
            auto size=in.tellg();
            if(!in || size<=0 || size>64*1024*1024) {
                started=true;fail("required diagnostic baseline is unavailable");return;
            }
            expected_baseline.resize(static_cast<size_t>(size));in.seekg(0);
            if(!in.read((char*)expected_baseline.data(),size)) {
                started=true;fail("cannot read required diagnostic baseline");return;
            }
        }
        psx_mod_set_controller_source(0,first_controller);
        psx_mod_set_controller_source(1,second_controller);owns_sources=true;
    }
    host_osd_set_diagnostics(nullptr,nullptr);
}
void reset_session() {
    recording_enabled=display_enabled=false;
    reset_recording();
}
void activate() {
    recording_enabled=true;
    reset_recording();
}
void activate_display() {display_enabled=true;}
int display_inputs(CPUState *cpu,uint32_t address) {
    // Both features may be enabled, but only one native input owner runs.
    return recording_enabled || netplay_recording_enabled()?0:inputs(cpu,address);
}
int coop_inputs(CPUState *cpu,uint32_t address) {
    return netplay_recording_enabled()?inputs(cpu,address):0;
}
uint32_t diagnostic_snapshot_bytes() {return 1;}
int diagnostic_snapshot_write(uint8_t *out,uint32_t len) {
    if(len!=1)return 0;out[0]=1;return 1;
}
int diagnostic_snapshot_validate(const uint8_t *data,uint32_t len) {
    return len==1 && data[0]==1;
}
void diagnostic_snapshot_read(const uint8_t*,uint32_t) {}
}
extern "C" int mmx4_diagnostics_snapshot_ready(void) {return !inside;}
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
    }else if(recording.is_open() && !recording_failed)recording.controller(sequence,seat,*buttons);
    return replaying?1:0;
}
extern "C" void mmx4_diagnostics_reset(void) {reset_recording();}
PSX_MOD_CONSTRUCTOR(mmx4_register_diagnostics) {
    // The stream deliberately starts a new baseline segment after restore.
    // Its provider gates single-player capture while its guest call is live.
    static const PSXModSnapshotProvider provider={1,diagnostic_snapshot_bytes,
        diagnostic_snapshot_write,diagnostic_snapshot_validate,
        diagnostic_snapshot_read,mmx4_diagnostics_snapshot_ready};
    psx_mod_register_snapshot_provider("mmx4.diagnostics",&provider);
    psx_mod_register_snapshot_loaded_observer("mmx4.diagnostics",state_loaded);
    psx_mod_register_snapshot_loaded_observer("mmx4.coop",state_loaded);
    psx_mod_register_session_reset_plugin("mmx4.diagnostics",reset_session);
    psx_mod_register_activation_plugin("mmx4.diagnostics",activate);
    psx_mod_register_activation_plugin("mmx4.diagnostics.osd",activate_display);
    psx_mod_register_function_filter_plugin("mmx4.diagnostics.osd",0x80012328u,display_inputs);
    psx_mod_register_function_filter_plugin("mmx4.diagnostics",0x80012328u,inputs);
    /* The executable's netplay profile installs only the co-op plugin.
     * This callback is also part of that fixed playtest profile. */
    psx_mod_register_function_filter_plugin("mmx4.coop",0x80012328u,coop_inputs);
}
