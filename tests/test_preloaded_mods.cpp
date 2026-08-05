#include "mod_packages.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
    if (argc != 2) return fail("expected the preloaded mods root");

    const fs::path source(argv[1]);
    const fs::path root =
        fs::temp_directory_path() / "mmx4-preloaded-mods-test";
    std::error_code ec;
    fs::remove_all(root, ec);
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
    if (manifest_count != 4) return fail("expected four package manifests");

    PSXRecompV4::mod_clear_plugins_for_tests();
    for (const char* id : {
             "mmx4.damage-multiplier",
             "mmx4.fast-loading",
             "mmx4.widescreen",
             "mmx4.frame-interpolation"}) {
        if (!PSXRecompV4::mod_register_activation_plugin(id, no_op_plugin)) {
            return fail(std::string("could not register test plugin ") + id);
        }
    }

    PSXRecompV4::ModPackageManager manager(root);
    std::string error;
    if (!manager.scan(&error)) return fail("catalog scan failed: " + error);
    if (!manager.load_state(&error)) return fail("default state failed: " + error);
    if (manager.packages().size() != 4)
        return fail("expected four package families");

    const auto default_plan = manager.resolve(kGameId, "", kDiscSha256);
    if (!default_plan.ok || !default_plan.writes.empty() ||
        default_plan.plugins.size() != 1 ||
        default_plan.plugins.front().id != "mmx4.damage-multiplier") {
        return fail("normal-damage override was not enabled by default");
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
        damage_plan.plugins.size() != 1 ||
        damage_plan.plugins.front().id != "mmx4.damage-multiplier" ||
        manager.feature_option_value(
            "mmx4.cheat.damage-multiplier", "damage-multiplier",
            "multiplier") != "37") {
        return fail("integer damage multiplier plan was incorrect");
    }

    if (!manager.set_feature_enabled(
            "mmx4.cheat.damage-multiplier", "damage-multiplier", false, &error)) {
        return fail(error);
    }
    if (!manager.set_feature_enabled(
            "mmx4.enhancement.fast-loading", "fast-loading", true, &error)) {
        return fail(error);
    }
    for (const char* mode : {
             "host-2x", "host-4x", "host-8x", "host-16x",
             "host-uncapped", "disc-2x", "disc-4x", "disc-instant"}) {
        if (!manager.set_feature_option(
                "mmx4.enhancement.fast-loading", "fast-loading",
                "mode", mode, &error)) {
            return fail(error);
        }
        const auto loading_plan =
            manager.resolve(kGameId, "", kDiscSha256);
        if (!loading_plan.ok || !loading_plan.writes.empty() ||
            loading_plan.plugins.size() != 1 ||
            loading_plan.plugins.front().id != "mmx4.fast-loading") {
            return fail(std::string("wrong fast-loading plugin for ") + mode);
        }
    }

    if (!manager.set_feature_enabled(
            "mmx4.enhancement.fast-loading", "fast-loading", false, &error) ||
        !manager.set_feature_enabled(
            "mmx4.enhancement.widescreen", "widescreen", true, &error)) {
        return fail(error);
    }
    const auto widescreen_plan = manager.resolve(kGameId, "", kDiscSha256);
    if (!widescreen_plan.ok || !widescreen_plan.writes.empty() ||
        widescreen_plan.plugins.size() != 1 ||
        widescreen_plan.plugins.front().id != "mmx4.widescreen") {
        return fail("widescreen plugin resolution was incorrect");
    }

    if (!manager.set_feature_enabled(
            "mmx4.enhancement.widescreen", "widescreen", false, &error) ||
        !manager.set_feature_enabled(
            "mmx4.enhancement.frame-interpolation",
            "frame-interpolation", true, &error)) {
        return fail(error);
    }
    for (const char* choice :
         {"display", "90", "120", "144", "165", "240"}) {
        if (!manager.set_feature_option(
                "mmx4.enhancement.frame-interpolation",
                "frame-interpolation", "rate", choice, &error)) {
            return fail(error);
        }
        const auto frame_plan = manager.resolve(kGameId, "", kDiscSha256);
        if (!frame_plan.ok || !frame_plan.writes.empty() ||
            frame_plan.plugins.size() != 1 ||
            frame_plan.plugins.front().id != "mmx4.frame-interpolation") {
            return fail(std::string("wrong frame-rate plugin for ") + choice);
        }
    }

    fs::remove_all(root, ec);
    std::cout << "Mega Man X4 preloaded mods: default integer damage 1, "
                 "default-off unified loading modes, validated 16:9, and five "
                 "fixed presentation rates plus display refresh\n";
    return 0;
}
