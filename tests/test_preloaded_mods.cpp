#include "mod_packages.h"
#include "mod_plugins.h"
#include "cpu_state.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>

namespace fs = std::filesystem;

namespace {

constexpr const char* kGameId = "SLUS-00561";
constexpr const char* kDiscSha256 =
    "3ceab06ac99add4035f912188bcbf5b16c02056f46a33d38ff0c8dbce6cb613b";

int fail(const std::string& message) {
    std::cerr << "FAIL: " << message << "\n";
    return 1;
}

void no_op_plugin() {}

std::array<uint8_t, 0x60> escape_play;
std::array<uint8_t, 0x40> escape_menu;
std::array<uint8_t, 0x8C> rush_actor;
bool rush_local_view = false;
const char* rush_online_option = "false";
bool escape_bad_write = false;
bool escape_online = false;
bool escape_option_present = true;
const char* escape_online_option = "false";

}  // namespace

// Compile the real title plugin and use the production registration owner.
// Guest-memory access and the supplied committed option context are replaced
// here; saved selections and plan identity still use the production manager.
extern "C" int psx_mod_register_instruction_plugin(
    const char* id, uint32_t address, uint32_t expected,
    PSXModFunctionEntryCallback callback) {
    return id && PSXRecompV4::mod_register_instruction_plugin(id, address, expected, callback);
}

extern "C" uint8_t psx_mod_read_byte(uint32_t address) {
    if (address >= 0x801721C0u && address < 0x801721C0u + escape_play.size())
        return escape_play[address - 0x801721C0u];
    if (address >= 0x80165A30u && address < 0x80165A30u + rush_actor.size())
        return rush_actor[address - 0x80165A30u];
    return 0;
}

extern "C" int psx_mod_local_view_scope(void) { return rush_local_view; }

extern "C" void psx_mod_write_byte(uint32_t address, uint8_t value) {
    if (address >= 0x801754A0u && address < 0x801754A0u + escape_menu.size())
        escape_menu[address - 0x801754A0u] = value;
    else
        escape_bad_write = true;
}

extern "C" int psx_mod_netplay_is_active(void) { return escape_online; }
extern "C" int psx_mod_current_option_value(
    const char* option, char* out, uint32_t size) {
    const char* value = !std::strcmp(option, "stage_escape") ? escape_online_option :
        !std::strcmp(option, "boss_rush_skip") ? rush_online_option : nullptr;
    if (!escape_option_present || !value || size <= std::strlen(value)) return 0;
    std::strcpy(out, value);
    return 1;
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 5) return fail("expected mods root [English.x4fmv Spanish.x4fmv Japanese-BIN]");

    const fs::path source(argv[1]);
    const fs::path root =
        fs::weakly_canonical(fs::temp_directory_path()) /
        ("mmx4-preloaded-mods-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    if(fs::exists(root))return fail("unique test directory already exists");
    fs::copy(source, root, fs::copy_options::recursive);

    size_t manifest_count = 0;
    for (const fs::directory_entry& entry :
         fs::recursive_directory_iterator(root / "packages")) {
        if (!entry.is_regular_file() ||
            entry.path().filename() != "manifest.toml") {
            continue;
        }
        ++manifest_count;
        PSXRecompV4::ModPackage package;
        std::string error;
        if (!PSXRecompV4::ModPackageManager::read_manifest(
                entry.path(), package, &error)) {
            return fail("manifest parse failed: " + error);
        }
        if(package.id=="mmx4.diagnostics") {
            for(const auto& feature:package.features)
                if(feature.hidden || feature.quick || feature.group!="Diagnostics")
                    return fail("diagnostics must be visible regular Mods controls");
        }
        for (const auto& resource : package.resources) {
            if (resource.id == "japanese-disc" &&
                resource.file_patterns != "*.cue,*.bin,*.chd,*.iso") {
                return fail("Japanese disc filters must use the launcher's comma-separated patterns");
            }
        }
    }
    if (manifest_count != 16) return fail("expected sixteen package manifests, including community patches and Nightmare Zero palette");

    const auto escape_hooks = PSXRecompV4::mod_instruction_hooks("mmx4.stage-escape");
    if (!PSXRecompV4::mod_plugin_registered("mmx4.stage-escape") ||
        escape_hooks.size() != 1 || escape_hooks[0].address != 0x2FF7Cu ||
        escape_hooks[0].expected != 0xA2020024u || !escape_hooks[0].callback)
        return fail("actual stage-escape instruction-only plugin was not registered");
    CPUState escape_cpu{};
    escape_cpu.gpr[16] = 0x801754A0u;
    escape_play.fill(0xA5);
    escape_play[0] = 6;
    escape_play[1] = 2;
    const auto original_play = escape_play;
    const CPUState original_cpu = escape_cpu;
    for (unsigned inactive = 0; inactive < 3; ++inactive) {
        escape_menu.fill(0x5A);
        escape_cpu.gpr[16] = inactive == 0 ? 0 : 0x801754A0u;
        escape_play[0] = inactive == 1 ? 3 : 6;
        escape_play[1] = inactive == 2 ? 1 : 2;
        escape_hooks[0].callback(&escape_cpu, 0x8002FF7Cu);
        for (uint8_t byte : escape_menu)
            if (byte != 0x5A) return fail("stage-escape callback changed a non-gameplay menu");
    }
    escape_cpu = original_cpu;
    escape_play = original_play;
    escape_menu.fill(0x5A);
    escape_hooks[0].callback(&escape_cpu, 0x8002FF7Cu);
    for (size_t i = 0; i < escape_menu.size(); ++i)
        if (escape_menu[i] != (i == 0x23 ? 1 : 0x5A))
            return fail("stage-escape callback must change only pause eligibility");
    if (escape_bad_write || escape_play != original_play ||
        std::memcmp(&escape_cpu, &original_cpu, sizeof(escape_cpu)))
        return fail("stage-escape callback changed campaign or CPU state");

    auto online_escape_hooks = PSXRecompV4::mod_instruction_hooks("mmx4.coop");
    online_escape_hooks.erase(std::remove_if(online_escape_hooks.begin(), online_escape_hooks.end(),
        [](const auto& hook) { return hook.address != 0x2FF7Cu; }), online_escape_hooks.end());
    if (online_escape_hooks.size() != 1 || online_escape_hooks[0].address != 0x2FF7Cu ||
        online_escape_hooks[0].expected != 0xA2020024u || !online_escape_hooks[0].callback)
        return fail("Co-op's online Escape callback was not registered");
    for (unsigned choice = 0; choice < 4; ++choice) {
        escape_online = choice != 0;
        escape_option_present = choice != 1;
        escape_online_option = choice == 2 ? "false" : "true";
        escape_menu.fill(0x5A);
        online_escape_hooks[0].callback(&escape_cpu, 0x8002FF7Cu);
        for (size_t i = 0; i < escape_menu.size(); ++i)
            if (escape_menu[i] != (choice == 3 && i == 0x23 ? 1 : 0x5A))
                return fail("online Escape did not follow its committed host option");
    }
    escape_online = false;

    // Execute the real portal callback. Its only allowed effect is the
    // native eligibility operand; original campaign/refight/actor bytes stay.
    const auto rush_hooks = PSXRecompV4::mod_instruction_hooks("mmx4.boss-rush-skip");
    auto online_rush_hooks = PSXRecompV4::mod_instruction_hooks("mmx4.coop");
    online_rush_hooks.erase(std::remove_if(online_rush_hooks.begin(), online_rush_hooks.end(),
        [](const auto& hook) { return hook.address != 0xC7900u; }), online_rush_hooks.end());
    if (rush_hooks.size() != 1 || rush_hooks[0].address != 0xC7900u ||
        rush_hooks[0].expected != 0x306300FFu || !rush_hooks[0].callback ||
        online_rush_hooks.size() != 1 || online_rush_hooks[0].expected != 0x306300FFu)
        return fail("actual standalone/Co-op Sigma portal hooks were not registered");
    CPUState rush_cpu{};
    rush_cpu.gpr[16] = 0x80165A30u;
    rush_cpu.gpr[3] = 8;
    escape_play.fill(0);
    escape_play[0] = 6; escape_play[12] = 12;
    escape_play[0x59] = 255; escape_play[0x5F] = 10;
    rush_actor.fill(0);
    rush_actor[0] = 0x41; rush_actor[1] = 27;
    rush_actor[2] = 10; // Original yellow pad: authored destination10.
    rush_actor[4] = 1; rush_actor[5] = 4;
    const auto portal_play = escape_play;
    const auto portal_actor = rush_actor;
    const CPUState portal_cpu = rush_cpu;
    for (unsigned invalid = 0; invalid < 15; ++invalid) {
        rush_cpu = portal_cpu; escape_play = portal_play; rush_actor = portal_actor;
        switch (invalid) {
        case 0: rush_local_view = true; break;
        case 1: escape_play[0] = 3; break;
        case 2: escape_play[1] = 2; break;
        case 3: escape_play[12] = 11; break;
        case 4: escape_play[13] = 1; break;
        case 5: escape_play[0x59] = 254; break;
        case 6: escape_play[0x5F] = 9; break;
        case 7: rush_cpu.gpr[16] += 1; break;
        case 8: rush_actor[0] = 0; break;
        case 9: rush_actor[1] = 26; break;
        case 10: rush_actor[2] = 0; break;
        case 11: rush_actor[4] = 0; break;
        case 12: rush_actor[5] = 3; break;
        case 13: rush_actor[6] = 1; break;
        case 14: rush_cpu.gpr[3] = 7; break;
        }
        const CPUState rejected_cpu = rush_cpu;
        const auto rejected_play = escape_play;
        const auto rejected_actor = rush_actor;
        rush_hooks[0].callback(&rush_cpu, 0x800C7900u);
        if (std::memcmp(&rush_cpu, &rejected_cpu, sizeof rush_cpu) ||
            escape_play != rejected_play || rush_actor != rejected_actor || escape_bad_write)
            return fail("Sigma skip bypassed its native portal/campaign eligibility guard");
        rush_local_view = false;
    }
    escape_play = portal_play; rush_actor = portal_actor;
    for (unsigned remaining = 8; remaining <= 16; ++remaining) {
        rush_cpu = portal_cpu; rush_cpu.gpr[3] = remaining;
        rush_hooks[0].callback(&rush_cpu, 0x800C7900u);
        if (rush_cpu.gpr[3] != 16) return fail("Sigma portal eligibility was not admitted");
        rush_cpu.gpr[3] = portal_cpu.gpr[3];
        if (std::memcmp(&rush_cpu, &portal_cpu, sizeof rush_cpu) ||
            escape_play != portal_play || rush_actor != portal_actor || escape_bad_write)
            return fail("Sigma skip changed native flags, rewards, actor or unrelated CPU state");
    }
    for (unsigned choice = 0; choice < 4; ++choice) {
        rush_cpu = portal_cpu;
        escape_online = choice != 0; escape_option_present = choice != 1;
        rush_online_option = choice == 2 ? "false" : "true";
        online_rush_hooks[0].callback(&rush_cpu, 0x800C7900u);
        if (rush_cpu.gpr[3] != (choice == 3 ? 16 : 8))
            return fail("online Sigma portal did not follow the committed host option");
    }
    escape_online = false; escape_option_present = true;

    PSXRecompV4::mod_clear_plugins_for_tests();
    if (!PSXRecompV4::mod_register_instruction_plugin("mmx4.boss-rush-skip",
            0x800C7900u, 0x306300FFu, rush_hooks[0].callback) ||
        !PSXRecompV4::mod_register_instruction_plugin("mmx4.coop",
            0x800C7900u, 0x306300FFu, online_rush_hooks[0].callback))
        return fail("could not restore actual Sigma portal registrations");
    if (!PSXRecompV4::mod_register_instruction_plugin("mmx4.stage-escape",
            0x8002FF7Cu, 0xA2020024u, escape_hooks[0].callback))
        return fail("could not restore actual stage-escape instruction registration");
    if (!PSXRecompV4::mod_register_instruction_plugin("mmx4.coop",
            0x8002FF7Cu, 0xA2020024u, online_escape_hooks[0].callback))
        return fail("could not restore actual Co-op Escape instruction registration");
    for (const char* id : {
             "mmx4.damage-multiplier",
             "mmx4.resident-loading",
             "mmx4.widescreen", "mmx4.coop", "mmx4.coop-stage-select", "mmx4.diagnostics", "mmx4.diagnostics.osd",
             "mmx4.ultimate_armor", "mmx4.black_zero", "mmx4.nightmare-zero",
             "mmx4.japanese-media", "mmx4.translation",
             "mmx4.japanese-fmvs", "mmx4.complete-story",
             "mmx4.community.zero-buster", "mmx4.community.transparent-hud",
             "mmx4.community.hybrid-buster", "mmx4.community.plasma-boss-filter",
             "psx.playable-replays"}) {
        if (!PSXRecompV4::mod_register_activation_plugin(id, no_op_plugin)) {
            return fail(std::string("could not register test plugin ") + id);
        }
    }

    PSXRecompV4::ModPackageManager manager(root);
    std::string error;
    if (!manager.scan(&error)) return fail("catalog scan failed: " + error);
    if (!manager.load_state(&error)) return fail("default state failed: " + error);
    if (manager.packages().size() != 16)
        return fail("expected sixteen package families");

    const auto default_plan = manager.resolve(kGameId, "", kDiscSha256);
    const char* community_features[]={"zero-buster","transparent-hud","hybrid-buster","plasma-boss-filter"};
    for (unsigned mask=1;mask<16;++mask) {
        for (unsigned bit=0;bit<4;++bit)
            if (!manager.set_feature_enabled("mmx4.community-patches",community_features[bit],(mask&(1u<<bit))!=0,&error))
                return fail(error);
        const auto plan=manager.resolve(kGameId,"",kDiscSha256);
        if (!plan.ok || plan.writes.empty() || plan.fingerprint==default_plan.fingerprint)
            return fail("community patch combination did not resolve: "+std::to_string(mask));
    }
    for (const auto* feature:community_features)
        if (!manager.set_feature_enabled("mmx4.community-patches",feature,false,&error)) return fail(error);
    if (manager.feature_enabled("mmx4.qol.boss-rush-skip", "skip") ||
        manager.feature_option_value("mmx4.coop", "coop", "boss_rush_skip") != "false")
        return fail("standalone/online boss-rush skip must default off");
    if (!manager.set_feature_enabled("mmx4.qol.boss-rush-skip", "skip", true, &error) ||
        !manager.save_state(&error)) return fail(error);
    PSXRecompV4::ModPackageManager rush_saved(root);
    if (!rush_saved.scan(&error) || !rush_saved.load_state(&error) ||
        !rush_saved.feature_enabled("mmx4.qol.boss-rush-skip", "skip"))
        return fail("saved boss-rush skip choice did not survive reload");
    const auto rush_plan = rush_saved.resolve(kGameId, "", kDiscSha256);
    unsigned rush_plugins = 0;
    for (const auto& plugin : rush_plan.plugins) rush_plugins += plugin.id == "mmx4.boss-rush-skip";
    if (!rush_plan.ok || rush_plugins != 1 || !rush_plan.writes.empty() ||
        rush_plan.fingerprint == default_plan.fingerprint)
        return fail("boss-rush skip must have its own settings identity without campaign writes");
    if (!manager.set_feature_enabled("mmx4.qol.boss-rush-skip", "skip", false, &error) ||
        !manager.save_state(&error) || !rush_saved.load_state(&error) ||
        rush_saved.feature_enabled("mmx4.qol.boss-rush-skip", "skip"))
        return fail("saved boss-rush skip off choice did not survive reload");
    if(manager.feature_enabled("mmx4.qol.stage-escape","escape"))
        return fail("unfinished-stage escape must default off");
    if(!manager.set_feature_enabled("mmx4.qol.stage-escape","escape",true,&error) ||
       !manager.save_state(&error))return fail(error);
    PSXRecompV4::ModPackageManager escape_saved(root);
    if(!escape_saved.scan(&error) || !escape_saved.load_state(&error) ||
       !escape_saved.feature_enabled("mmx4.qol.stage-escape","escape"))
        return fail("saved stage-escape choice did not survive reload");
    const auto escape_plan=escape_saved.resolve(kGameId,"",kDiscSha256);
    unsigned escape_plugins=0;
    for(const auto& plugin:escape_plan.plugins)escape_plugins+=plugin.id=="mmx4.stage-escape";
    if(!escape_plan.ok || escape_plugins!=1 || !escape_plan.writes.empty() ||
       escape_plan.fingerprint==default_plan.fingerprint)
        return fail("stage escape must resolve independently without campaign writes");
    if(!manager.set_feature_enabled("mmx4.qol.stage-escape","escape",false,&error) ||
       !manager.save_state(&error) || !escape_saved.load_state(&error) ||
       escape_saved.feature_enabled("mmx4.qol.stage-escape","escape"))
        return fail("saved stage-escape off choice did not survive reload");
    if(manager.feature_option_value("mmx4.coop","coop","stage_escape")!="false")
        return fail("netplay stage escape must default off");
    if(!manager.set_feature_enabled("mmx4.coop","coop",true,&error))return fail(error);
    const auto online_escape_off=manager.resolve(kGameId,"",kDiscSha256);
    if (!manager.set_feature_option("mmx4.coop", "coop", "boss_rush_skip", "true", &error) ||
        !manager.save_state(&error) || !rush_saved.load_state(&error) ||
        rush_saved.feature_option_value("mmx4.coop", "coop", "boss_rush_skip") != "true")
        return fail("saved online boss-rush choice did not survive reload");
    const auto online_rush_on = rush_saved.resolve(kGameId, "", kDiscSha256);
    if (!online_rush_on.ok || online_rush_on.fingerprint == online_escape_off.fingerprint)
        return fail("online boss-rush choice did not change settings identity");
    if (!manager.set_feature_option("mmx4.coop", "coop", "boss_rush_skip", "false", &error) ||
        !manager.save_state(&error) || !rush_saved.load_state(&error) ||
        rush_saved.feature_option_value("mmx4.coop", "coop", "boss_rush_skip") != "false")
        return fail("saved online boss-rush off choice did not survive reload");
    if(!manager.set_feature_option("mmx4.coop","coop","stage_escape","true",&error) ||
       !manager.save_state(&error) || !escape_saved.load_state(&error) ||
       escape_saved.feature_option_value("mmx4.coop","coop","stage_escape")!="true")
        return fail("saved netplay Escape choice did not survive reload");
    const auto online_escape_on=escape_saved.resolve(kGameId,"",kDiscSha256);
    if(!online_escape_on.ok || online_escape_on.fingerprint==online_escape_off.fingerprint)
        return fail("netplay Escape choice did not change the settings plan identity");
    if(!manager.set_feature_option("mmx4.coop","coop","stage_escape","false",&error) ||
       !manager.set_feature_enabled("mmx4.coop","coop",false,&error) ||
       !manager.save_state(&error) || !escape_saved.load_state(&error) ||
       escape_saved.feature_option_value("mmx4.coop","coop","stage_escape")!="false")
        return fail("saved netplay Escape off choice did not survive reload");
    if(manager.feature_enabled("mmx4.coop-stage-select","cast") ||
       manager.feature_option_value("mmx4.coop","coop","stage_select_cast")!="false")
        return fail("stage-select cast must default off locally and online");
    if(!manager.set_feature_enabled("mmx4.coop-stage-select","cast",true,&error))return fail(error);
    const auto cast=manager.resolve(kGameId,"",kDiscSha256);
    bool cast_plugin=false,cast_coop=false;
    for(const auto& plugin:cast.plugins) {
        cast_plugin|=plugin.id=="mmx4.coop-stage-select";
        cast_coop|=plugin.id=="mmx4.coop";
    }
    if(!cast.ok || !cast_plugin || !cast_coop || cast.fingerprint==default_plan.fingerprint)
        return fail("optional stage cast must require co-op and change session identity");
    if(!manager.set_feature_enabled("mmx4.coop-stage-select","cast",false,&error))return fail(error);
    if(manager.feature_enabled("mmx4.complete-story","complete-story"))
        return fail("complete story must default off");
    if(!manager.set_feature_enabled("mmx4.complete-story","complete-story",true,&error))return fail(error);
    const auto complete=manager.resolve(kGameId,"",kDiscSha256);
    bool story_plugin=false;
    for(const auto& plugin:complete.plugins)story_plugin|=plugin.id=="mmx4.complete-story";
    if(!complete.ok || !story_plugin || complete.fingerprint==default_plan.fingerprint)
        return fail("complete story must resolve independently and change session identity");
    if(!manager.set_feature_enabled("mmx4.complete-story","complete-story",false,&error))return fail(error);
    if (!default_plan.ok || !default_plan.writes.empty() ||
        std::count_if(default_plan.plugins.begin(), default_plan.plugins.end(),
            [](const auto& plugin) { return plugin.id == mmx4.damage-multiplier; }) != 1) {
        return fail("normal-damage override was not enabled by default");
    }
    if (manager.feature_enabled("mmx4.nomusic", "no-music"))
        return fail("no-music must default off");
    if (!manager.set_feature_enabled("mmx4.nomusic", "no-music", true, &error))
        return fail(error);
    const auto no_music = manager.resolve(kGameId, "", kDiscSha256);
    if (!no_music.ok || no_music.overlays.size() != 5 ||
        !no_music.writes.empty() ||
        no_music.plugins.size() != default_plan.plugins.size() ||
        no_music.fingerprint == default_plan.fingerprint)
        return fail("no-music package did not resolve five overlays");
    for (size_t i = 0; i < default_plan.plugins.size(); ++i)
        if (no_music.plugins[i].id != default_plan.plugins[i].id)
            return fail("no-music must preserve the default plugins");
    if (!manager.set_feature_enabled("mmx4.nomusic", "no-music", false, &error))
        return fail(error);
    const auto music_restored = manager.resolve(kGameId, "", kDiscSha256);
    if (!music_restored.ok || !music_restored.overlays.empty() ||
        music_restored.fingerprint != default_plan.fingerprint)
        return fail("disabling no-music must restore the default plan");
    bool diagnostics_default=false,display_default=false,playable_default=false;
    for(const auto& plugin:default_plan.plugins) {
        diagnostics_default|=plugin.id=="mmx4.diagnostics";
        playable_default|=plugin.id=="psx.playable-replays";
        display_default|=plugin.id=="mmx4.diagnostics.osd";
    }
    if(!diagnostics_default || playable_default || display_default ||
       !manager.feature_enabled("mmx4.diagnostics","inputs") ||
       manager.feature_enabled("mmx4.diagnostics","osd"))
        return fail("recording must default on independently of display, with replay eligibility owned by diagnostics");
    if(manager.feature_option_value("mmx4.coop","coop","diagnostic_recording")!="true")
        return fail("netplay recording must default on without enabling co-op");
    // The original inputs ID owns the saved recording choice. Display is new
    // and independent; explicitly disabling recording must survive reload.
    if(!manager.set_feature_enabled("mmx4.diagnostics","inputs",false,&error) ||
       !manager.set_feature_enabled("mmx4.diagnostics","osd",true,&error) ||
       !manager.set_feature_option("mmx4.coop","coop","diagnostic_recording","false",&error) ||
       !manager.save_state(&error))return fail(error);
    PSXRecompV4::ModPackageManager restored(root);
    if(!restored.scan(&error) || !restored.load_state(&error) ||
       restored.feature_enabled("mmx4.diagnostics","inputs") ||
       !restored.feature_enabled("mmx4.diagnostics","osd") ||
       restored.feature_option_value("mmx4.coop","coop","diagnostic_recording")!="false")
        return fail("saved recording/display/netplay choices did not survive reload");
    const auto display_plan=restored.resolve(kGameId,"",kDiscSha256);
    bool recording_plugin=false,display_plugin=false;
    for(const auto& plugin:display_plan.plugins) {
        recording_plugin|=plugin.id=="mmx4.diagnostics" || plugin.id=="psx.playable-replays";
        display_plugin|=plugin.id=="mmx4.diagnostics.osd";
    }
    if(!display_plan.ok || recording_plugin || !display_plugin)
        return fail("display-only choice must not activate recording");
    if(!manager.set_feature_enabled("mmx4.diagnostics","inputs",true,&error) ||
       !manager.set_feature_enabled("mmx4.diagnostics","osd",true,&error))return fail(error);
    const auto both_plan=manager.resolve(kGameId,"",kDiscSha256);
    unsigned recorders=0,displays=0,playable_recorders=0;
    for(const auto& plugin:both_plan.plugins) {
        recorders+=plugin.id=="mmx4.diagnostics";
        playable_recorders+=plugin.id=="psx.playable-replays";
        displays+=plugin.id=="mmx4.diagnostics.osd";
    }
    if(!both_plan.ok || recorders!=1 || playable_recorders!=0 || displays!=1)
        return fail("both choices must resolve each owner once");
    if(!manager.save_state(&error) || !restored.load_state(&error) ||
       !restored.feature_enabled("mmx4.diagnostics","inputs") ||
       !restored.feature_enabled("mmx4.diagnostics","osd"))
        return fail("saved recording/display ON choices did not survive reload");
    if(!manager.set_feature_enabled("mmx4.diagnostics","osd",false,&error) ||
       !manager.set_feature_enabled("mmx4.diagnostics","inputs",false,&error))return fail(error);
    for(const char* feature:{"japanese-media","translation"})
        if(manager.feature_enabled("mmx4.localization",feature))return fail("language mods must default off");
    for(const char* option:{"voices","title-screen"})
        if(manager.feature_option_value("mmx4.localization","japanese-media",option)!="false")
            return fail("Japanese media switches must default off independently");
    if(manager.feature_enabled("mmx4.japanese-fmvs","japanese-fmvs") ||
       manager.feature_option_value("mmx4.japanese-fmvs","japanese-fmvs","subtitles")!="none")
        return fail("Japanese FMVs must default off with no subtitles selected");
    if(!manager.set_feature_enabled("mmx4.localization","translation",true,&error))return fail(error);
    const auto spanish=manager.resolve(kGameId,"",kDiscSha256);
    if(!spanish.ok || spanish.resources.size()!=2 || !spanish.resources[0].bytes || !spanish.resources[1].bytes)
        return fail("both bundled catalogs must resolve verified bytes without a Japanese disc");
    if(manager.feature_option_value("mmx4.localization","translation","language")!="es" ||
       !manager.set_feature_option("mmx4.localization","translation","language","pt-br",&error))return fail(error);
    const auto portuguese=manager.resolve(kGameId,"",kDiscSha256);
    if(!portuguese.ok || portuguese.fingerprint==spanish.fingerprint)
        return fail("language selection must participate in the session identity");
    if(!manager.set_feature_enabled("mmx4.localization","translation",false,&error) ||
       !manager.set_feature_enabled("mmx4.localization","japanese-media",true,&error))return fail(error);
    if(manager.resolve(kGameId,"",kDiscSha256).ok)return fail("Japanese media requires an owner-provided donor");
    if(!manager.set_feature_enabled("mmx4.localization","japanese-media",false,&error))return fail(error);
    if(manager.feature_enabled("mmx4.cheat.ultimate-armor","ultimate-armor") ||
       manager.feature_enabled("mmx4.cheat.black-zero","black-zero") ||
       manager.feature_enabled("mmx4.palette.nightmare-zero","nightmare-zero"))
        return fail("elective character cheats must default off");
      if(manager.feature_option_value("mmx4.coop","coop","separate_etanks")!="false")
          return fail("legacy individual-pausing E-tank preference must default off");
      if(manager.feature_option_value("mmx4.coop","coop","simultaneous_menus")!="true")
          return fail("independent simultaneous menus must default on");
      if(!manager.set_feature_option("mmx4.coop","coop","simultaneous_menus","false",&error))return fail(error);
      if(!manager.set_feature_option("mmx4.coop","coop","simultaneous_menus","true",&error))return fail(error);
      if(!manager.set_feature_option("mmx4.coop","coop","separate_etanks","true",&error))return fail(error);
      if(!manager.set_feature_option("mmx4.coop","coop","separate_etanks","false",&error))return fail(error);
      if(manager.feature_option_value("mmx4.coop","coop","ultimate_armor_x")!="false" ||
       manager.feature_option_value("mmx4.coop","coop","black_zero")!="false")
        return fail("netplay character options must default off");
    if(manager.feature_option_value("mmx4.coop","coop","boss_doors")!="either")
        return fail("boss doors must default to either player");
    if(!manager.set_feature_option("mmx4.coop","coop","boss_doors","both",&error))return fail(error);
    if(!manager.set_feature_option("mmx4.coop","coop","boss_doors","either",&error))return fail(error);
    for(const auto& feature:std::vector<std::pair<const char*,const char*>>{
        {"mmx4.cheat.ultimate-armor","ultimate-armor"},{"mmx4.cheat.black-zero","black-zero"}}) {
        if(!manager.set_feature_enabled(feature.first,feature.second,true,&error))return fail(error);
    }
    const auto both_cheats=manager.resolve(kGameId,"",kDiscSha256);
    if(!both_cheats.ok || !both_cheats.writes.empty() || both_cheats.plugins.size()!=5)
        return fail("both independent character cheat plugins must resolve together");
    if(!manager.set_feature_enabled("mmx4.palette.nightmare-zero","nightmare-zero",true,&error))return fail(error);
    const auto nightmare=manager.resolve(kGameId,"",kDiscSha256);
    if(!nightmare.ok || !nightmare.writes.empty() || nightmare.plugins.size()!=both_cheats.plugins.size()+1 ||
       nightmare.fingerprint==both_cheats.fingerprint)
        return fail("Nightmare palette must compose independently with both native character codes and affect plan identity");
    if(manager.resolve(kGameId,"",std::string(64,'0')).ok || manager.resolve("SLPS-00902","",kDiscSha256).ok)
        return fail("Nightmare palette must reject an incorrect disc or regional target");
    if(!manager.set_feature_enabled("mmx4.palette.nightmare-zero","nightmare-zero",false,&error))return fail(error);
    if(manager.resolve(kGameId,"",kDiscSha256).fingerprint!=both_cheats.fingerprint)
        return fail("disabling Nightmare palette must restore the unchanged character-code plan");
    for(const auto& feature:std::vector<std::pair<const char*,const char*>>{
        {"mmx4.cheat.ultimate-armor","ultimate-armor"},{"mmx4.cheat.black-zero","black-zero"}}) {
        if(!manager.set_feature_enabled(feature.first,feature.second,false,&error))return fail(error);
    }

    if (!manager.set_feature_option(
            "mmx4.cheat.damage-multiplier", "damage-multiplier",
            "multiplier", "37", &error)) {
        return fail(error);
    }
    if (manager.set_feature_option(
            "mmx4.cheat.damage-multiplier", "damage-multiplier",
            "multiplier", "256", &error)) {
        return fail("damage multiplier accepted a value above 255");
    }
    const auto damage_plan = manager.resolve(kGameId, "", kDiscSha256);
    if (!damage_plan.ok || !damage_plan.writes.empty() ||
        damage_plan.plugins.size() != 3 ||
        manager.feature_option_value(
            "mmx4.cheat.damage-multiplier", "damage-multiplier",
            "multiplier") != "37") {
        return fail("integer damage multiplier plan was incorrect");
    }

    if (!manager.set_feature_enabled(
            "mmx4.cheat.damage-multiplier", "damage-multiplier", false, &error)) {
        return fail(error);
    }
    if (manager.set_feature_enabled(
            "mmx4.enhancement.fast-loading", "fast-loading", true, &error)) {
        return fail("retired generic loading wrapper unexpectedly selectable");
    }
    error.clear();
    if (!manager.set_feature_enabled(
            "mmx4.enhancement.widescreen", "widescreen", true, &error)) {
        return fail(error);
    }
    const auto widescreen_plan = manager.resolve(kGameId, "", kDiscSha256);
    if (!widescreen_plan.ok || !widescreen_plan.writes.empty() ||
        widescreen_plan.plugins.size() != 2) {
        return fail("widescreen plugin resolution was incorrect");
    }

    if (manager.set_feature_enabled("mmx4.enhancement.frame-interpolation",
                                    "frame-interpolation", true, &error)) {
        return fail("archived interpolation unexpectedly selectable");
    }

    // Private local cameras must resolve authored tiles even if the player
    // disabled the standalone renderer. Unified and disabled co-op retain
    // that preference instead of silently enabling an elective enhancement.
    if(!manager.set_feature_enabled("mmx4.enhancement.widescreen","widescreen",false,&error) ||
       !manager.set_feature_enabled("mmx4.coop","coop",true,&error))return fail(error);
    for(const char* layout:{"unified","split"}) {
        if(!manager.set_feature_option("mmx4.coop","coop","local_cameras",layout,&error))return fail(error);
        const auto plan=manager.resolve(kGameId,"",kDiscSha256);
        bool renderer=false;
        for(const auto& plugin:plan.plugins)renderer|=plugin.id=="mmx4.widescreen";
        if(!plan.ok || renderer!=(std::string(layout)=="split"))
            return fail("local split renderer dependency was not conditional");
    }
    if(!manager.set_feature_enabled("mmx4.coop","coop",false,&error))return fail(error);
    const auto solo_plan=manager.resolve(kGameId,"",kDiscSha256);
    for(const auto& plugin:solo_plan.plugins)
        if(plugin.id=="mmx4.widescreen")return fail("disabled co-op forced the renderer");
    if(!solo_plan.ok)return fail("disabled co-op plan failed");

    constexpr const char* movies="mmx4.japanese-fmvs";
    if(!manager.set_feature_enabled(movies,"japanese-fmvs",true,&error) ||
       !manager.set_feature_option(movies,"japanese-fmvs","subtitles","en",&error))return fail(error);
    auto missing=manager.resolve(kGameId,"",kDiscSha256);
    bool named_missing=false;
    for(const auto& message:missing.errors)named_missing|=message.find("english-movies")!=std::string::npos;
    if(missing.ok || !named_missing)return fail("selected movie resource must reject launch by resource name");
    if(!manager.set_feature_enabled(movies,"japanese-fmvs",false,&error))return fail(error);
    if(argc==5) {
        if(!manager.set_feature_enabled(movies,"japanese-fmvs",true,&error))return fail(error);
        std::string fingerprints[3];
        const char* languages[]={"en","es","none"};
        const char* resources[]={"english-movies","spanish-movies","japanese-disc"};
        for(unsigned i=0;i<3;++i) {
            if(!manager.set_feature_option(movies,"japanese-fmvs","subtitles",languages[i],&error))return fail(error);
            if(i && manager.resolve(kGameId,"",kDiscSha256).ok)return fail("missing selected media must reject");
            if(!manager.set_feature_resource_path(movies,"japanese-fmvs",resources[i],argv[i+2],&error))return fail(error);
            const auto plan=manager.resolve(kGameId,"",kDiscSha256);
            if(!plan.ok || plan.resources.size()!=1 || !plan.resources[0].bytes ||
               plan.resources[0].id!=resources[i] || !plan.writes.empty() ||
               !plan.overlays.empty() || !plan.derived_discs.empty() || !plan.implicit_features.empty())
                return fail("one selector must load only its verified media without a disc patch");
            fingerprints[i]=plan.fingerprint;
        }
        if(fingerprints[0]==fingerprints[1] || fingerprints[1]==fingerprints[2])return fail("subtitle selection must affect session identity");
        if(!manager.set_feature_enabled(movies,"japanese-fmvs",false,&error))return fail(error);
        if(!manager.resolve(kGameId,"",kDiscSha256).ok)return fail("disabled movies must not require media");
        std::cout<<"One Japanese FMVs selector, immutable selected resources and language identity: PASS\n";
    }

    if(fs::weakly_canonical(root).parent_path()!=fs::weakly_canonical(fs::temp_directory_path()))
        return fail("test cleanup path escaped the temporary directory");
    fs::remove_all(root, ec);
    std::cout << "Mega Man X4 preloaded mods: normal damage 1, resident loading and widescreen, "
                 "generic loading wrapper and interpolation absent\n";
    return 0;
}
