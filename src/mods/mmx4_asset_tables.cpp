#include "mmx4_asset_tables.h"
#include "mmx4_resident_catalog.h"
#include "mod_plugins.h"
#include "crash_trace.h"
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace Mmx4AssetTables {
namespace {
struct Patch { uint32_t address, original, value; };
std::vector<Patch> patches;
std::vector<std::pair<uint32_t, std::vector<uint8_t>>> blobs;
bool applied = false, entered = false;
}

void reset() {
    patches.clear(); blobs.clear(); applied = false; entered = false;
}

void patch(uint32_t address, uint32_t original, uint32_t value) {
    for (auto& existing : patches) if (existing.address == address) {
        if (existing.original != original)
            throw std::runtime_error("Conflicting X4 asset baseline guards");
        existing.value = value;
        applied = false;
        return;
    }
    patches.push_back({address, original, value});
    applied = false;
}

void guest_blob(uint32_t address, std::vector<uint8_t> bytes) {
    blobs.push_back({address, std::move(bytes)});
    applied = false;
}

void mount_file(unsigned index, const std::vector<uint8_t>& bytes) {
    if (index >= std::size(MMX4Files) || bytes.size() < 4 || bytes.size() > UINT32_MAX)
        throw std::runtime_error("Invalid X4 derived file contract");
    uint32_t lba = 0;
    if (!psx_mod_replace_disc_file(MMX4Files[index].path, bytes.data(), uint32_t(bytes.size()), &lba))
        throw std::runtime_error("Cannot mount derived X4 asset");
    const auto& original = MMX4FileContracts[index];
    const uint32_t table = 0x800f0e18 + index * 12;
    const uint32_t first = uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8 |
        uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
    patch(table, original.lba, lba);
    patch(table + 4, original.size, uint32_t(bytes.size()));
    patch(table + 8, original.first, first);
}

void publish() {
    if (!entered || applied || patches.empty()) return;
    if (psx_mod_read_word(0x800f0e18) != MMX4FileContracts[0].lba) return;
    // Retain the original/final expected-byte guards and validate the whole
    // transaction before any table or auxiliary cue data is written.
    for (const auto& patch : patches) {
        const auto word = psx_mod_read_word(patch.address);
        if (word != patch.original && word != patch.value) {
            char detail[160];
            std::snprintf(detail, sizeof detail,
                "X4 asset overlay: Executable table at %08x: expected %08x, found %08x",
                patch.address, patch.original, word);
            psx_fatal_halt(detail);
        }
    }
    for (const auto& patch : patches) psx_mod_write_word(patch.address, patch.value);
    for (const auto& blob : blobs)
        for (size_t i = 0; i < blob.second.size(); ++i)
            psx_mod_write_byte(blob.first + uint32_t(i), blob.second[i]);
    applied = true;
    // Preserve the existing language/media qualification counter.
    psx_mod_counter_add("mmx4.language.tables_published", 1);
}

void entry(CPUState*, uint32_t) { entered = true; publish(); }
void restored() { applied = false; entered = psx_mod_game_started() != 0; publish(); }
}
