#include "mod_packages.h"

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

}  // namespace

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
    }
    if (manifest_count != 10) return fail("expected ten package manifests, including Complete Story");

    PSXRecompV4::mod_clear_plugins_for_tests();
    for (const char* id : {
             "mmx4.damage-multiplier",
             "mmx4.resident-loading",
             "mmx4.widescreen", "mmx4.coop", "mmx4.diagnostics",
             "mmx4.ultimate_armor", "mmx4.black_zero",
             "mmx4.japanese-media", "mmx4.translation",
             "mmx4.japanese-fmvs", "mmx4.complete-story"}) {
        if (!PSXRecompV4::mod_register_activation_plugin(id, no_op_plugin)) {
            return fail(std::string("could not register test plugin ") + id);
        }
    }

    PSXRecompV4::ModPackageManager manager(root);
    std::string error;
    if (!manager.scan(&error)) return fail("catalog scan failed: " + error);
    if (!manager.load_state(&error)) return fail("default state failed: " + error);
    if (manager.packages().size() != 10)
        return fail("expected ten package families");

    const auto default_plan = manager.resolve(kGameId, "", kDiscSha256);
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
        default_plan.plugins.size() != 3) {
        return fail("normal-damage override was not enabled by default");
    }
    bool diagnostics_default=false;
    for(const auto& plugin:default_plan.plugins)diagnostics_default|=plugin.id=="mmx4.diagnostics";
    if(diagnostics_default || manager.feature_enabled("mmx4.diagnostics","inputs"))
        return fail("input diagnostics must default off");
    if(manager.feature_option_value("mmx4.coop","coop","diagnostic_recording")!="false")
        return fail("netplay diagnostic recording must default off");
    // Recording stays selectable, and an explicit preference survives reload.
    if(!manager.set_feature_enabled("mmx4.diagnostics","inputs",true,&error) ||
       !manager.save_state(&error))return fail(error);
    PSXRecompV4::ModPackageManager restored(root);
    if(!restored.scan(&error) || !restored.load_state(&error) ||
       !restored.feature_enabled("mmx4.diagnostics","inputs"))
        return fail("explicit diagnostic preference did not survive reload");
    const auto recording_plan=restored.resolve(kGameId,"",kDiscSha256);
    bool recording_plugin=false;
    for(const auto& plugin:recording_plan.plugins)recording_plugin|=plugin.id=="mmx4.diagnostics";
    if(!recording_plan.ok || !recording_plugin)return fail("optional diagnostics did not resolve");
    if(!manager.set_feature_enabled("mmx4.diagnostics","inputs",false,&error))return fail(error);
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
       manager.feature_enabled("mmx4.cheat.black-zero","black-zero"))
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
