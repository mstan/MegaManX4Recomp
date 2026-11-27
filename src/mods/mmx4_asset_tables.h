#pragma once
#include <cstdint>
#include <vector>

struct CPUState;

// One transaction owns X4's private executable tables for the complete asset
// plan. Providers compose through the framework's effective disc first; only
// the final mounted extent is published to the guest.
namespace Mmx4AssetTables {
void reset();
// Retain source identity so later providers can leave a community extension
// intact when its donor member is identical to the original source.
void remember_source_member(unsigned file, unsigned member, const std::vector<uint8_t>& bytes);
bool source_member_matches(unsigned file, unsigned member, const std::vector<uint8_t>& bytes);
void patch(uint32_t address, uint32_t original, uint32_t value);
void guest_blob(uint32_t address, std::vector<uint8_t> bytes);
void mount_file(unsigned index, const std::vector<uint8_t>& bytes);
void publish();
void entry(CPUState*, uint32_t);
void restored();
}
