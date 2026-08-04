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
    if (manifest_count != 3) return fail("expected three package manifests");

    PSXRecompV4::mod_clear_plugins_for_tests();
    for (const char* id : {
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
    if (manager.packages().size() != 3)
        return fail("expected three package families");

    const auto default_plan = manager.resolve(kGameId, "", kDiscSha256);
    if (!default_plan.ok || !default_plan.writes.empty() ||
        !default_plan.plugins.empty()) {
        return fail("built-in mods were not disabled by default");
    }

    if (!manager.set_feature_enabled(
            "mmx4.cheat.one-hit-kills", "one-hit-kills", true, &error)) {
        return fail(error);
    }
    const auto damage_plan = manager.resolve(kGameId, "", kDiscSha256);
    if (!damage_plan.ok || !damage_plan.plugins.empty() ||
        damage_plan.writes.size() != 1) {
        return fail("one-hit-kills plan was incorrect");
    }

    const auto& write = damage_plan.writes.front();
    const std::vector<uint8_t> expected = {0x5c, 0x00, 0x83, 0x90};
    const std::vector<uint8_t> replacement = {0x5c, 0x00, 0x86, 0x90};
    if (write.target != PSXRecompV4::ModPatchTarget::MainExe ||
        write.location != 0x8002DEF8u ||
        write.expected != expected ||
        write.replacement != replacement) {
        return fail("one-hit-kills guarded write was incorrect");
    }

    if (!manager.set_feature_enabled(
            "mmx4.cheat.one-hit-kills", "one-hit-kills", false, &error) ||
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
    std::cout << "Mega Man X4 preloaded mods: three disabled-by-default "
                 "features, one testing cheat, validated 16:9, and five "
                 "fixed presentation rates plus display refresh\n";
    return 0;
}
