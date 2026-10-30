#pragma once

// Original psxrecomp revision 9868d30c bounded reader; provenance in README.md.
#include <cstdint>
#include <string>
#include <vector>

namespace Mmx4Media {

/* Bounded ISO9660 access to an already verified immutable psx-mode2 snapshot.
 * Does not reopen paths or touch the guest drive. Stream files expose their
 * raw extent; read() returns ordinary 2048-byte file data (e.g. ARC/EXE). */
class ModMode2Disc {
public:
    ModMode2Disc(const uint8_t* bytes, uint64_t size):bytes_(bytes),size_(size) {}
    bool find(const std::string& path,uint32_t& lba,uint32_t& size) const;
    bool read(const std::string& path,std::vector<uint8_t>& out) const;
private:
    const uint8_t* bytes_; uint64_t size_;
};

} // namespace Mmx4Media
