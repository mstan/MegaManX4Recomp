#include "mmx4_community_assets.h"
#include "mmx4_resident_catalog.h"
#include "mod_plugins.h"
#include "crash_trace.h"
#include <cstdio>

namespace {
constexpr const char* Package="mmx4.community-patches";
constexpr const char* Features[]={"zero-buster","transparent-hud","hybrid-buster","plasma-boss-filter"};
constexpr const char* Plugins[]={"mmx4.community.zero-buster","mmx4.community.transparent-hud",
    "mmx4.community.hybrid-buster","mmx4.community.plasma-boss-filter"};
constexpr const char* Paths[]={"ARC/PL00_U.ARC","ARC/PL01_U.ARC","ARC/PL02_U.ARC"};
struct TablePatch {uint32_t address,expected,value;};
std::vector<TablePatch> tables;
bool prepared=false,published=false,entered=false;
void reset() {tables.clear();prepared=published=entered=false;}
void publish() {
    if (!entered || published) return;
    for (const auto& patch:tables) {
        const auto word=psx_mod_read_word(patch.address);
        if (word!=patch.expected && word!=patch.value)
            psx_fatal_halt("X4 community patch: executable asset table guard failed");
    }
    for (const auto& patch:tables) psx_mod_write_word(patch.address,patch.value);
    published=true;
}
void entry(CPUState*,uint32_t) {entered=true;publish();}
void restored() {entered=psx_mod_game_started()!=0;published=false;publish();}
void activate(const char* resource) {
    if (prepared) return;
    try {
        unsigned flags=0;
        for (unsigned i=0;i<4;++i) if (psx_mod_feature_active(Package,Features[i])) flags|=1u<<i;
        const uint8_t* bytes=nullptr;uint64_t size=0;
        if (!flags || !psx_mod_current_resource_bytes(resource,&bytes,&size) || size>1024*1024)
            throw std::runtime_error("Built-in community asset recipe is unavailable");
        const auto recipes=Mmx4Community::recipes(bytes,size);
        Mmx4Community::Archives original;
        for (unsigned i=0;i<3;++i) {
            uint32_t count=0;
            if (!psx_mod_read_disc_file(Paths[i],nullptr,0,&count) || count>4*1024*1024)
                throw std::runtime_error("Cannot read the selected original disc");
            Mmx4Community::Bytes file(count);
            if (!psx_mod_read_disc_file(Paths[i],file.data(),count,&count))
                throw std::runtime_error("Cannot read original player assets");
            original[i]=Mmx4Language::unpack(file);
        }
        const auto derived=Mmx4Community::compose(original,recipes,flags);
        // Finish and guard all three assets before publishing any extent.
        std::array<Mmx4Community::Bytes,3> files;
        for (unsigned i=0;i<3;++i) files[i]=Mmx4Language::pack(derived[i]);
        for (unsigned i=0;i<3;++i) {
            uint32_t lba=0,index=0;
            while (index<138 && std::strcmp(MMX4Files[index].path,Paths[i])) ++index;
            if (index==138 || !psx_mod_replace_disc_file(Paths[i],files[i].data(),uint32_t(files[i].size()),&lba))
                throw std::runtime_error("Cannot mount derived community player assets");
            const auto& contract=MMX4FileContracts[index];
            const uint32_t address=0x800f0e18+index*12;
            tables.push_back({address,contract.lba,lba});
            tables.push_back({address+4,contract.size,uint32_t(files[i].size())});
            tables.push_back({address+8,contract.first,Mmx4Language::u32(files[i].data())});
        }
        prepared=true;
        std::fprintf(stderr,"mmx4.community: patches=%u; by: flpstrike (implementation) and hasungkee (sprites); original image unchanged\n",flags);
    } catch (const std::exception& error) {
        psx_fatal_halt((std::string("X4 community patches: ")+error.what()).c_str());
    }
}
void zero() {activate("assets-zero-buster");}
void hud() {activate("assets-transparent-hud");}
void hybrid() {activate("assets-hybrid-buster");}
void plasma() {activate("assets-plasma-boss-filter");}
}
PSX_MOD_CONSTRUCTOR(mmx4_register_community_patches) {
    void (*activators[])()={zero,hud,hybrid,plasma};
    for (unsigned i=0;i<4;++i) {
        psx_mod_register_session_reset_plugin(Plugins[i],reset);
        psx_mod_register_activation_plugin(Plugins[i],activators[i]);
        psx_mod_register_function_entry_plugin(Plugins[i],0x800dae8c,entry);
        psx_mod_register_vblank_plugin(Plugins[i],publish);
        psx_mod_register_savestate_plugin(Plugins[i],restored);
    }
}
