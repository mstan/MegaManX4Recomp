// The No Music mod's original author-supplied patches are reconstructed
// byte-for-byte from the player's verified disc. No disc bytes ship.
#include "iso_reader.h"
#include "mod_packages.h"
#include "mod_plugins.h"
#include "psx_path_utf8.h"
#include "psx_sha256_file.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <stdexcept>

namespace {
using namespace PSXRecompV4;
namespace fs = std::filesystem;
constexpr uint32_t Sector = 2352;

bool verified(const fs::path& file, const ModOverlay& overlay) {
    std::error_code ec;
    if(fs::file_size(file,ec)!=overlay.size || ec) return false;
    uint8_t sha[32];
    if(psx_sha256_file(file,sha)!=PsxFileHashResult::Ok) return false;
    std::string actual;
    for(auto byte:sha) {
        actual+="0123456789abcdef"[byte>>4];
        actual+="0123456789abcdef"[byte&15];
    }
    return actual==overlay.sha256;
}

struct Temporary {
    fs::path file;
    ~Temporary() { if(!file.empty()) { std::error_code ec;fs::remove(file,ec); } }
};

bool prepare(const ModPrepareContext& ctx,std::map<std::string,fs::path>& out,
             std::string& error,bool cached_only) {
    try {
        const auto cache=ctx.cache_root/"mmx4-no-music";
        PS1::ISOReader disc;
        for(const auto& overlay:ctx.package.overlays) {
            if(overlay.feature_id!=ctx.feature_id) continue;
            if(overlay.target!=ModPatchTarget::DiscRaw || overlay.resource_id.empty() ||
                overlay.resource_offset || !overlay.size || overlay.size%Sector ||
                overlay.location%Sector || overlay.location/Sector+overlay.size/Sector>450000)
                throw std::runtime_error("Invalid No Music sector range");
            const auto path=cache/(overlay.sha256+".bin");
            if(!verified(path,overlay)) {
                if(cached_only) throw std::runtime_error("Prepare No Music using Play before joining");
                if(fs::exists(path)) throw std::runtime_error("No Music cache checksum failed");
                if(!disc.IsOpen() && !disc.Open(psx_path::utf8(ctx.disc_path)))
                    throw std::runtime_error("Cannot open your original disc to prepare No Music");
                fs::create_directories(cache);
                auto temp_path=path;
                temp_path+=fs::u8path(".tmp."+std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count()));
                Temporary temp{std::move(temp_path)};
                std::ofstream write(temp.file,std::ios::binary);
                if(!write)throw std::runtime_error("Cannot create No Music cache file");
                std::array<uint8_t,Sector> bytes{};
                const auto start=uint32_t(overlay.location/Sector);
                const auto count=uint32_t(overlay.size/Sector);
                for(uint32_t i=0;i<count;++i) {
                    if((i%512)==0 && ctx.progress && !ctx.progress(-1.0f,"Preparing No Music"))
                        throw std::runtime_error("Preparation canceled.");
                    if(!disc.ReadRawSector(start+i,bytes.data()) || bytes[15]!=2 ||
                        !std::equal(bytes.begin()+16,bytes.begin()+20,bytes.begin()+20))
                        throw std::runtime_error("No Music needs the original raw Mode 2 disc");
                    if(bytes[18]&4)std::fill(bytes.begin()+24,bytes.end(),0);
                    if(!write.write(reinterpret_cast<const char*>(bytes.data()),Sector))
                        throw std::runtime_error("Cannot write No Music cache file");
                }
                write.close();
                if(!write || !verified(temp.file,overlay))
                    throw std::runtime_error("No Music output does not match the verified disc");
                if(ctx.progress && !ctx.progress(-1.0f,"Verifying No Music"))
                    throw std::runtime_error("Preparation canceled.");
                std::error_code ec;
                fs::rename(temp.file,path,ec);
                if(ec && !verified(path,overlay))
                    throw std::runtime_error("Cannot publish No Music cache");
                if(!ec)temp.file.clear();
            }
            out.emplace(overlay.resource_id,path);
        }
        return true;
    } catch(const std::exception& e) {error=e.what();return false;}
}
}

PSX_MOD_CONSTRUCTOR(mmx4_register_no_music) {
    mod_register_media_preparer("mmx4.nomusic",
        [](const auto& ctx,auto& out,auto& error){return prepare(ctx,out,error,false);},
        [](const auto& ctx,auto& out,auto& error){return prepare(ctx,out,error,true);});
}
