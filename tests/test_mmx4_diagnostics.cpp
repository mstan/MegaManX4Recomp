/* Verify replay boundaries, including a P2 input changing mid-frame. Native
 * gameplay replay is a separate original-disc check. No window is created. */
extern "C" {
#include "mmx4_coop_internal.h"
}
#include "host_osd.h"
#include "memcard.h"
#include "mod_netplay.h"
#include "mod_snapshot.h"
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
static std::string application_root;
static bool online;
static bool playable_replays_requested;
extern "C" void replay_recording_request(int enabled) {playable_replays_requested=enabled!=0;}
static unsigned native_calls;
static bool coop_active=true,split_views_config;
static std::map<std::string,std::string> coop_options{{"local_cameras","unified"},
    {"cameras","unified"},{"hud_layout","stacked"},{"boss_scaling","false"}};
static bool diagnostics_active=true;
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
int psx_mod_register_session_reset_plugin(const char*,PSXModActivationCallback) {return 1;}
int psx_mod_register_function_filter_plugin(const char*,uint32_t,PSXModFunctionFilterCallback) {return 1;}
int psx_mod_register_snapshot_loaded_observer(const char*,PSXModSnapshotLoadedObserver) {return 1;}
int psx_mod_register_snapshot_provider(const char*,const PSXModSnapshotProvider*) {return 1;}
int psx_mod_local_view_scope(void) {return 0;}
int psx_mod_netplay_is_active(void) {return online;}
int psx_mod_option_value(const char *package,const char *feature,const char *key,char *out,uint32_t size) {
    if(size)out[0]=0;
    auto it=coop_options.find(key);
    if(strcmp(package,"mmx4.coop") || strcmp(feature,"coop") || !coop_active ||
       it==coop_options.end() || it->second.size()>=size)return 0;
    memcpy(out,it->second.c_str(),it->second.size()+1);return 1;
}
int mod_runtime_snapshot_package_active(const char *id) {
    return (!strcmp(id,"mmx4.diagnostics") && diagnostics_active) ||
        (!strcmp(id,"mmx4.coop") && coop_active);
}
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
int mmx4_coop_split_views(void) {return split_views_config;}
int mmx4_coop_projected(void) {return projected_context;}
uint8_t *mmx4_coop_first_body(void) {return canonical_first.data();}
void mmx4_coop_development_mask_pad(void) {}
uint8_t *mmx4_coop_second_body(void) {return partner.data();}
int mmx4_coop_finish(CPUState *cpu,uint32_t v) {cpu->gpr[2]=v;return 1;}
uint32_t mmx4_coop_call(CPUState*,uint32_t,uint32_t,uint32_t) {
    ++native_calls;
    uint16_t held=(uint16_t)~pads[0];held=(uint16_t)((held<<8)|(held>>8));
    uint16_t old=psx_mod_read_half(0x80166C08u);
    psx_mod_write_half(0x80166C08u,held);psx_mod_write_half(0x80166C0Au,old);
    psx_mod_write_half(0x80166C0Cu,(uint16_t)(held&~old));return 77;
}
uint16_t mmx4_coop_input(unsigned seat);
namespace {std::filesystem::path application_directory() {return std::filesystem::u8path(application_root);}}
#define MMX4_DIAGNOSTIC_TEST
#define MMX4_DIAGNOSTIC_RELEASE_VERSION "fixture-release-for-test"
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
static void baseline_env(const char *v) {
#ifdef _WIN32
    _putenv_s("MMX4_DIAGNOSTIC_BASELINE",v);
#else
    setenv("MMX4_DIAGNOSTIC_BASELINE",v,1);
#endif
}
int main(int argc,char **argv) {
    auto original_options=coop_options;
    coop_options.clear();coop_active=false;
    {std::ostringstream meta;option_metadata(meta);
     CHECK(meta.str().find("\"local_cameras\": null")!=std::string::npos);
     CHECK(meta.str().find("\"separate_etanks_effective\": false")!=std::string::npos);}
    coop_active=true;coop_options["simultaneous_menus"]="true";
    coop_options["separate_etanks"]="false";
    {std::ostringstream meta;option_metadata(meta);
     CHECK(meta.str().find("\"separate_etanks_effective\": true")!=std::string::npos);}
    coop_options["simultaneous_menus"]="false";
    {std::ostringstream meta;option_metadata(meta);
     CHECK(meta.str().find("\"separate_etanks_effective\": false")!=std::string::npos);}
    coop_options=original_options;
    CHECK(mmx4_diagnostic_application_directory()==
          std::filesystem::canonical(std::filesystem::u8path(argv[0])).parent_path());
#ifndef _WIN32
    setenv("RECOMP_APPIMAGE_PATH","/tmp/mmx4-path-test/MegaManX4Recomp.AppImage",1);
    CHECK(mmx4_diagnostic_application_directory()==std::filesystem::path("/tmp/mmx4-path-test"));
    unsetenv("RECOMP_APPIMAGE_PATH");
    setenv("APPIMAGE","/tmp/mmx4-fallback/Game.AppImage",1);
    CHECK(mmx4_diagnostic_application_directory()==std::filesystem::path("/tmp/mmx4-fallback"));
    unsetenv("APPIMAGE");
#endif
    save_root=argc>1?argv[1]:"diagnostic-test-output";
    save_root+="/"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    application_root=save_root;
    std::filesystem::create_directories(save_root);CPUState cpu{};env("");reset_session();activate();
    CHECK(recording_enabled && !display_enabled);
    CHECK(!playable_replays_requested); // Co-op diagnostics remain on.
    coop_active=false;activate();
    CHECK(recording_enabled && playable_replays_requested);
    reset_session();CHECK(!recording_enabled && !playable_replays_requested);
    coop_active=true;activate();
    CHECK(recording_enabled && !playable_replays_requested);
    CHECK(shown[0].empty() && shown[1].empty());
    activate_display();
    CHECK(!sources[0] && !sources[1]);
    std::filesystem::create_directories(PSXRecompV4::mod_runtime_root());
    {std::ofstream state(PSXRecompV4::mod_runtime_root()/"state.toml");state<<"format_version = 2\n";}
    reset_session();activate_display();
    pads={0xFFEF,0xBFFF};
    CHECK(display_inputs(&cpu,0x80012328u) && sequence==1);
    CHECK(!recording_enabled && !started && !recording.is_open());
    CHECK(shown[0].find("U")!=std::string::npos && shown[0].find("REC")==std::string::npos);
    activate(); // Existing saved inputs choice enables recording only.
    CHECK(display_enabled && recording_enabled && sequence==0);
    CHECK(!display_inputs(&cpu,0x80012328u) && sequence==0);
    reset_session();activate();
    CHECK(!display_enabled);
    auto initial=std::make_unique<decltype(memory)>(memory);
    pads={0xFFEF,0xBFFF};CHECK(inputs(&cpu,0x80012328u));
    CHECK(recording.is_open() && shown[0].empty() && shown[1].empty());
    activate_display();
    auto calls_before=native_calls;
    CHECK(!display_inputs(&cpu,0x80012328u) && sequence==1 && native_calls==calls_before);
    uint16_t raw=0xFFEF;mmx4_diagnostics_resolve_input(0,&raw);
    raw=0xFF7F;mmx4_diagnostics_resolve_input(1,&raw);
    psx_mod_write_word(FRAME,1);pads={0xFFFF,0xFFF7};CHECK(inputs(&cpu,0x80012328u));
    CHECK(shown[1].find("St")!=std::string::npos);
    raw=0xFFF7;mmx4_diagnostics_resolve_input(1,&raw);
    recording.flush();auto final=std::make_unique<decltype(memory)>(memory);recording.close();
    std::filesystem::path file;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics"))
        if(std::filesystem::exists(entry.path()/"inputs.csv"))file=entry.path()/"inputs.csv";
    CHECK(!file.empty() && std::filesystem::file_size(file)>100);
    CHECK(std::filesystem::file_size(file.parent_path()/"card1.mcd")==MEMCARD_SIZE);
    CHECK(std::filesystem::exists(file.parent_path()/"card2.absent"));
    CHECK(std::filesystem::exists(file.parent_path()/"mods-state.toml"));
    CHECK(std::filesystem::exists(file.parent_path()/"session.json"));
    {std::ifstream meta(file.parent_path()/"session.json");
     std::string text((std::istreambuf_iterator<char>(meta)),{});
     CHECK(text.find("\"game_version\": \"fixture-release-for-test\"")!=std::string::npos);
     CHECK(text.find("\"camera_mode\": \"local_unified\"")!=std::string::npos);
     CHECK(text.find("\"local_cameras\": \"unified\"")!=std::string::npos);
     CHECK(text.find("\"boss_scaling\": false")!=std::string::npos);
     CHECK(text.find("\"mode_history\": \"mode-changes.csv\"")!=std::string::npos);}
    CHECK(std::filesystem::exists(file.parent_path()/"mode-changes.csv"));
    {std::ifstream location(std::filesystem::path(save_root)/"diagnostics-location.txt");
     std::string text((std::istreambuf_iterator<char>(location)),{});
     CHECK(text.find(std::filesystem::absolute(file.parent_path()).make_preferred().u8string())!=std::string::npos);}
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
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
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
    CHECK(effective_mode().campaign==0);
    projected_context=false;
    // Netplay traces must identify their mode, rather than masquerading as
    // independently playable offline recordings.
    online=true;activate();
    const uint8_t network_baseline[]={0x50,0x53,0x54};
    auto network_folder_count=std::distance(std::filesystem::directory_iterator(
        std::filesystem::path(application_root)/"saves/diagnostics"),
        std::filesystem::directory_iterator{});
    // The netplay profile must not start recording, including initial sync
    // loads, until the host explicitly enables the shared option.
    for(const char* value:{"false","true-ish"}) {
        coop_options["diagnostic_recording"]=value;
        CHECK(!coop_inputs(&cpu,0x80012328u));
        state_loaded(network_baseline,sizeof network_baseline);
        CHECK(!started && !recording.is_open() && sequence==0);
    }
    coop_options.erase("diagnostic_recording");
    CHECK(!coop_inputs(&cpu,0x80012328u));
    CHECK(std::distance(std::filesystem::directory_iterator(
        std::filesystem::path(application_root)/"saves/diagnostics"),
        std::filesystem::directory_iterator{})==network_folder_count);
    coop_options["diagnostic_recording"]="true";
    CHECK(coop_inputs(&cpu,0x80012328u));recording.flush();
    bool network_metadata=false;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        std::ifstream meta(entry.path()/"session.json");
        std::string text((std::istreambuf_iterator<char>(meta)),{});
        if(text.find("\"netplay\": true")!=std::string::npos)network_metadata=true;
    }
    CHECK(network_metadata);
    // Effective runtime flags take precedence over saved preferences, which
    // still say Unified here. Changes are queued into a separate sidecar;
    // the existing N/C input trace remains replay-compatible.
    split_views_config=true;inputs(&cpu,0x80012328u);
    online=false;inputs(&cpu,0x80012328u);
    split_views_config=false;inputs(&cpu,0x80012328u);
    coop_active=false;inputs(&cpu,0x80012328u);recording.flush();
    bool found_modes=false;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        std::ifstream in(entry.path()/"mode-changes.csv");
        std::string text((std::istreambuf_iterator<char>(in)),{});
        if(text.find("netplay_separate")==std::string::npos)continue;
        CHECK(text.find("netplay_unified")!=std::string::npos);
        CHECK(text.find("local_split_8_3")!=std::string::npos);
        CHECK(text.find("local_unified")!=std::string::npos);
        CHECK(text.find("single_player,1,0,")!=std::string::npos);
        CHECK(std::count(text.begin(),text.end(),'\n')==6);
        found_modes=true;
    }
    CHECK(found_modes);coop_active=true;
    // Loading or rewinding starts a new replayable segment with its exact
    // restored machine image. It must not append a time jump to old inputs.
    online=false;activate();inputs(&cpu,0x80012328u);
    const uint8_t restored_image[]={0x50,0x53,0x54,0x0A,0xFE,0x01};
    diagnostics_active=false;reset_session();
    activate_display(); // Display-only must not claim a recording or restore segment.
    auto folder_count=std::distance(std::filesystem::directory_iterator(
        std::filesystem::path(application_root)/"saves/diagnostics"),
        std::filesystem::directory_iterator{});
    state_loaded(restored_image,sizeof restored_image);
    uint16_t disabled_buttons=0xFF7F;mmx4_diagnostics_resolve_input(1,&disabled_buttons);
    CHECK(!started && !recording.is_open() && pending_baseline.empty() && sequence==0);
    CHECK(std::distance(std::filesystem::directory_iterator(
        std::filesystem::path(application_root)/"saves/diagnostics"),
        std::filesystem::directory_iterator{})==folder_count);
    diagnostics_active=true;activate();
    state_loaded(restored_image,sizeof restored_image);
    CHECK(sequence==0 && started && recording.is_open() && pending_baseline.empty());
    CHECK(counters["mmx4.diagnostics.state-segments"]==1);
    auto segment_initial=std::make_unique<decltype(memory)>(memory);
    uint16_t prefix_buttons=0xFF7F;mmx4_diagnostics_resolve_input(1,&prefix_buttons);
    inputs(&cpu,0x80012328u);
    auto segment_frame=psx_mod_read_word(FRAME);
    psx_mod_write_word(FRAME,segment_frame+1);inputs(&cpu,0x80012328u);recording.flush();
    bool found_baseline=false;
    std::filesystem::path segment_inputs;
    for(const auto& entry:std::filesystem::directory_iterator(std::filesystem::path(application_root)/"saves/diagnostics")) {
        auto baseline=entry.path()/"baseline.pst";
        if(!std::filesystem::exists(baseline))continue;
        std::ifstream in(baseline,std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(in)),{});
        CHECK(bytes.size()==sizeof restored_image && !memcmp(bytes.data(),restored_image,bytes.size()));
        std::ifstream meta(entry.path()/"session.json");
        std::string text((std::istreambuf_iterator<char>(meta)),{});
        CHECK(text.find("\"baseline\": \"baseline.pst\"")!=std::string::npos);
        segment_inputs=entry.path()/"inputs.csv";
        found_baseline=true;
    }
    CHECK(found_baseline);
    // A loaded PC can read P2 before the first pad decoder invocation. Start
    // playback on restore so leading C rows are consumed at that boundary.
    recording.close();memory=*segment_initial;env(segment_inputs.string().c_str());
    baseline_env((segment_inputs.parent_path()/"baseline.pst").string().c_str());activate();
    // A deferred or failed auto-load must never consume ordinary-boot rows.
    CHECK(inputs(&cpu,0x80012328u) && !started && !replaying && !replay_failed);
    CHECK(!baseline_loaded && sequence==0);
    memory=*segment_initial;
    state_loaded(restored_image,sizeof restored_image);
    CHECK(started && replaying && !replay_failed && baseline_loaded);
    prefix_buttons=0xFFFF;CHECK(mmx4_diagnostics_resolve_input(1,&prefix_buttons) && prefix_buttons==0xFF7F);
    CHECK(inputs(&cpu,0x80012328u) && !replay_failed);
    psx_mod_write_word(FRAME,segment_frame+1);CHECK(inputs(&cpu,0x80012328u) && !replay_failed);
    // A different restored image is a refusal, even if its input trace could
    // accidentally match the short segment.
    memory=*segment_initial;activate();
    uint8_t wrong_image[sizeof restored_image];memcpy(wrong_image,restored_image,sizeof wrong_image);
    wrong_image[sizeof wrong_image-1]^=1;
    state_loaded(wrong_image,sizeof wrong_image);
    CHECK(replay_failed && !replaying && !baseline_loaded);
    baseline_env("");
    env("");activate();
    // A directory/write failure is visible both on the input display and in
    // the location pointer. It must never show REC with an unusable recording.
    online=false;activate();activate_display();
    application_root+="/blocked-app";std::filesystem::create_directories(application_root);
    {std::ofstream blocked(std::filesystem::path(application_root)/"saves");blocked<<"file, not a directory";}
    inputs(&cpu,0x80012328u);
    CHECK(recording_failed && !recording.is_open());
    CHECK(shown[0].find("ERROR")!=std::string::npos && shown[0].find("REC")==std::string::npos);
    {std::ifstream location(std::filesystem::path(application_root)/"diagnostics-location.txt");
     std::string text((std::istreambuf_iterator<char>(location)),{});CHECK(text.find("FAILED")!=std::string::npos);}
    printf("diagnostic recording/replay boundary checks passed\n");return 0;
}
