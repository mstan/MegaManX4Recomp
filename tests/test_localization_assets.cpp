#include "mod_plugins.h"
#include "mod_media.h"
#include "iso_reader.h"
#include "psx_sha256.h"
#include "mmx4_language_assets.h"
#include "mmx4_fmv_pack.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <cstring>
using namespace PSXRecompV4;
using namespace Mmx4Language;
namespace fs=std::filesystem;
using Call=void(*)(void);
static auto& activation() {static std::map<std::string,Call> c;return c;}
static auto& resetters() {static std::map<std::string,Call> c;return c;}
static auto& ticks() {static std::map<std::string,Call> c;return c;}
static auto& entries() {static std::map<std::string,PSXModFunctionEntryCallback> c;return c;}
static std::shared_ptr<const std::vector<uint8_t>> donor_bytes;
static std::vector<uint8_t> catalog,ram(2*1024*1024),exe;
static std::map<uint32_t,uint8_t> aperture;
static std::map<std::string,std::vector<uint8_t>> replacements;
static std::string catalog_path,language="es",movie_subtitles="none";
static PS1::ISOReader original;
static unsigned options,appended;static uint32_t next_lba,next_guest;
static void check(bool ok,const std::string& text) {if(!ok)throw std::runtime_error(text);}
extern "C" {
int psx_mod_register_activation_plugin(const char* id,Call cb) {activation()[id]=cb;return 1;}
int psx_mod_register_session_reset_plugin(const char* id,Call cb) {resetters()[id]=cb;return 1;}
int psx_mod_register_vblank_plugin(const char* id,Call cb) {ticks()[id]=cb;return 1;}
int psx_mod_register_savestate_plugin(const char*,Call) {return 1;}
int psx_mod_register_function_entry_plugin(const char* id,uint32_t,PSXModFunctionEntryCallback cb) {entries()[id]=cb;return 1;}
int psx_mod_game_started() {return 1;}
void psx_mod_counter_add(const char*,uint32_t) {}
void psx_fatal_halt(const char* why) {throw std::runtime_error(why);}
int psx_mod_option_value(const char*,const char*,const char* id,char* out,uint32_t n) {
    if(!std::strcmp(id,"subtitles")) {std::snprintf(out,n,"%s",movie_subtitles.c_str());return 1;}
    if(!std::strcmp(id,"language")) {std::snprintf(out,n,"%s",language.c_str());return 1;}
    unsigned bit=!std::strcmp(id,"fmvs")?1:!std::strcmp(id,"voices")?2:4;
    std::snprintf(out,n,"%s",options&bit?"true":"false");return 1;
}
int psx_mod_current_resource_path(const char*,char* out,uint32_t n) {std::snprintf(out,n,"%s",catalog_path.c_str());return 1;}
int psx_mod_current_resource_bytes(const char* id,const uint8_t** out,uint64_t* size) {
    if(!std::strcmp(id,language=="pt-br"?"catalog-pt-br":"catalog")) {*out=catalog.data();*size=catalog.size();return 1;}
    *out=donor_bytes->data();*size=donor_bytes->size();return 1;
}
int psx_mod_read_disc_file(const char* path,void* out,uint32_t capacity,uint32_t* size) {
    auto found=replacements.find(path);
    if(found!=replacements.end()) {
        *size=uint32_t(found->second.size());
        if(!out)return capacity==0;
        if(capacity<*size)return 0;
        std::memcpy(out,found->second.data(),*size);return 1;
    }
    PS1::ISOFileEntry file;
    if(!original.FindFile(path,file))return 0;
    *size=file.size;
    if(!out)return capacity==0;
    return capacity>=*size && original.ReadFile(path,static_cast<uint8_t*>(out),capacity)==*size;
}
int psx_mod_append_disc_extent(const char*,uint64_t offset,uint32_t count,uint32_t* lba) {
    check(offset<=donor_bytes->size() && uint64_t(count)*2336<=donor_bytes->size()-offset,"donor stream bounds");
    check(uint64_t(next_lba)+count+150<=450000,"native CD timeline capacity");
    *lba=next_lba;next_lba+=count;++appended;return 1;
}
int psx_mod_replace_disc_file(const char* path,const void* data,uint32_t size,uint32_t* lba) {
    const auto* p=static_cast<const uint8_t*>(data);replacements[path]={p,p+size};
    *lba=next_lba;next_lba+=(size+2047)/2048;return 1;
}
uint32_t psx_mod_alloc_guest_memory(uint32_t size,uint32_t alignment) {
    next_guest=(next_guest+alignment-1)&~(alignment-1);const auto p=next_guest;next_guest+=size;return p;
}
static uint8_t read(uint32_t address) {return address<0x80200000?ram[address&0x1fffff]:aperture[address];}
uint32_t psx_mod_read_word(uint32_t address) {
    uint32_t w=0;for(unsigned i=0;i<4;++i)w|=uint32_t(read(address+i))<<(i*8);return w;
}
void psx_mod_write_byte(uint32_t address,uint8_t v) {
    if(address<0x80200000)ram[address&0x1fffff]=v;else aperture[address]=v;
}
void psx_mod_write_word(uint32_t address,uint32_t w) {for(unsigned i=0;i<4;++i)psx_mod_write_byte(address+i,uint8_t(w>>(i*8)));}
}
static std::vector<uint8_t> read_file(const std::string& path) {
    uint32_t size=0;check(psx_mod_read_disc_file(path.c_str(),nullptr,0,&size),"file query "+path);
    std::vector<uint8_t> data(size);check(psx_mod_read_disc_file(path.c_str(),data.data(),size,&size),"file read "+path);return data;
}
int main(int argc,char** argv) {
    try {
        std::vector<uint8_t> movie_fixture(11*2336);
        std::memcpy(movie_fixture.data(),"X4FMV001",8);put32(movie_fixture,8,10);put32(movie_fixture,12,2336);
        for(unsigned i=0;i<10;++i) {
            put32(movie_fixture,16+i*16,i+1);put32(movie_fixture,20+i*16,1);
            put32(movie_fixture,24+i*16,(i+1)*2336);
        }
        check(fmv_pack(movie_fixture.data(),movie_fixture.size())[9].offset==23360,"movie pack inventory");
        for(unsigned corruption:{8u,12u,16u,20u,24u,28u,2336u}) {
            auto bad_pack=movie_fixture;bad_pack[corruption]^=1;bool rejected=false;
            try{fmv_pack(bad_pack.data(),bad_pack.size());}catch(const std::exception&){rejected=true;}
            check(rejected,"malformed movie pack rejected before mounting");
        }
        bool trailing_rejected=false;
        movie_fixture.push_back(0);
        try{fmv_pack(movie_fixture.data(),movie_fixture.size());}catch(const std::exception&){trailing_rejected=true;}
        check(trailing_rejected,"trailing movie bytes rejected");
        if(argc==4 && !std::strcmp(argv[1],"--fan")) {
            check(original.Open(argv[2]),"open original US disc for movie-only qualification");
            exe=read_file("SLUS_005.61");
            std::copy(exe.begin()+2048,exe.end(),ram.begin()+0x10000);
            const auto stock_ram=ram;
            std::ifstream pack_file(argv[3],std::ios::binary);
            auto pack=std::make_shared<std::vector<uint8_t>>(std::istreambuf_iterator<char>(pack_file),std::istreambuf_iterator<char>());
            const auto extents=fmv_pack(pack->data(),pack->size());donor_bytes=pack;
            for(const char* selected:{"en","es"}) {
                const char* plugin="mmx4.japanese-fmvs";movie_subtitles=selected;
                for(const auto& pair:resetters())pair.second();
                ram=stock_ram;replacements.clear();next_lba=251793;appended=0;
                activation().at(plugin)();
                check(appended==10 && replacements.empty(),"only ten stream extents are mounted");
                check(ram==stock_ram,"no table publication before executable entry");
                entries().at(plugin)(nullptr,0x800dae8c);
                auto expected=stock_ram;uint32_t lba=251793;
                for(unsigned i=0;i<10;++i) {
                    const unsigned at=0xf149c+i*12;
                    put32(expected,at,lba);put32(expected,at+4,extents[i].sectors*2336);
                    put32(expected,at+8,u32(pack->data()+extents[i].offset));lba+=extents[i].sectors;
                }
                check(ram==expected,"movie-only changes preserve every other executable/script/font byte");
                // A simultaneous Japanese-media selection must not mount a
                // second movie campaign, preserving the native CD LBA limit.
                options=1;activation().at("mmx4.japanese-media")();
                check(appended==10,"one selected fan campaign mounts once; voices/title do not mount movies");
                bool duplicate_rejected=false;
                try{activation().at(plugin)();}catch(const std::exception&){duplicate_rejected=true;}
                check(duplicate_rejected,"second subtitle campaign refused");
            }
            std::cout<<"PASS: real fan movies, native LBA contract and script/font/gameplay isolation\n";return 0;
        }
        std::vector<Member> fixture{{9,{1,2,3}},{0x10001,std::vector<uint8_t>(32768,0)}};
        const auto built=pack(fixture);check(unpack(built)[0].data==fixture[0].data,"ARC round trip");
        std::vector<uint8_t> opaque_header(built.begin(),built.begin()+2048);opaque_header[2047]=0xa7;
        check(pack(fixture,opaque_header)[2047]==0xa7,"opaque original ARC header preserved");
        const std::vector<std::vector<uint8_t>> short_records{{0x4a,0,0x4c,0xa0},{0x4b,0xa0},{0x4c,0xa0}};
        const auto aligned_bank=message_bank(short_records);
        check(aligned_bank.size()%4==0 && messages(aligned_bank)==short_records,"edited native message banks retain word alignment");
        auto bad=built;bad[4]^=1;bool refused=false;
        try{unpack(bad);}catch(...){refused=true;}check(refused,"malformed ARC rejected");
        if(argc!=4 && argc!=5) {std::cout<<"PASS: language archive contracts\n";return 0;}
        if(argc==5)language=argv[4];
        check(original.Open(argv[1]),"open owner US disc");catalog_path=argv[3];
        std::ifstream input(catalog_path,std::ios::binary);catalog={std::istreambuf_iterator<char>(input),{}};
        std::string error;
        check(load_mod_media(argv[2],"psx-mode2",589683488,
            "829185314c9040d74f66c5babdb08a6fa58239d299afae11fd396f4c243874d7",donor_bytes,&error),error);
        ModMode2Disc donor(donor_bytes->data(),donor_bytes->size());
        std::vector<uint8_t> jp_load;check(donor.read("ARC/LOAD.ARC",jp_load),"verified donor font atlas");
        auto jp=unpack(jp_load);
        const auto us_load=unpack(read_file("ARC/LOAD_U.ARC"));
        auto original_us=us_load;
        const auto us_logo=member(original_us,0x10008),us_font=member(original_us,0x10001);
        auto us_title=unpack(read_file("ARC/ST0E_U0.ARC"));
        std::vector<uint8_t> jp_title_bytes;check(donor.read("ARC/ST0E_00.ARC",jp_title_bytes),"Japanese title scene");
        auto jp_title=unpack(jp_title_bytes);
        std::vector<uint8_t> jp_exe;check(donor.read("SLPS_009.02",jp_exe),"Japanese loading palette source");
        exe=read_file("SLUS_005.61");
        const std::array<std::string,3> names{"ARC/PL00_U.ARC","ARC/PL01_U.ARC","ARC/PL02_U.ARC"};
        std::array<std::vector<std::vector<uint8_t>>,3> source_records;
        for(unsigned i=0;i<3;++i) {auto arc=unpack(read_file(names[i]));source_records[i]=messages(member(arc,0x15));}
        for(options=0;options<16;++options) {
            for(const auto& r:resetters())r.second();
            replacements.clear();aperture.clear();appended=0;next_lba=original.GetSectorCount();next_guest=0x8f100000;
            std::fill(ram.begin(),ram.end(),0);std::copy(exe.begin()+0x800,exe.end(),ram.begin()+0x10000);
            if(options&1)activation().at("mmx4.japanese-fmvs")();
            if(options&6)activation().at("mmx4.japanese-media")();
            if(options&8)activation().at("mmx4.translation")();
            const auto not_entered=ram;
            for(const auto& cb:ticks())cb.second();
            check(ram==not_entered,"VBlank during a partially loaded EXE never publishes language tables");
            for(const auto& cb:entries())cb.second(nullptr,0x800dae8c);
            for(const auto& cb:ticks())cb.second();
            check(appended==((options&1)?11u:0u)+((options&2)?6u:0u),"independent FMV/voice selection");
            auto load=unpack(read_file("ARC/LOAD_U.ARC"));
            check(member(load,0x10008)==((options&4)?member(jp,0x10008):us_logo),"independent Rockman title artwork");
            auto title=unpack(read_file("ARC/ST0E_U0.ARC"));
            for(uint32_t type:{0x10002u,0xdu,0x10000u,0u,1u,0xbu})
                check(member(title,type)==member((options&4)?jp_title:us_title,type),"main title scene art and layout are independent");
            if(options&4) {
                for(unsigned i=0;i<16;++i)
                    check(psx_mod_read_word(0x800f16a0+i*4)==u32(jp_exe.data()+0xe2030+i*4),"Japanese loading texture has its matching wordmark and X4 palettes");
                check(psx_mod_read_word(0x800f21a0)==0x0c0b1312 && psx_mod_read_word(0x800f21a4)==0x09110700,
                    "native title spawns wordmark after X/4 so the full Rockman word stays visible");
                const auto frames=sprite_frames(member(title,0x12)),jp_frames=sprite_frames(member(jp_title,0x12));
                check(frames.size()==35,"US executable can access all 35 title frames");
                for(unsigned i=0;i<31;++i)if(i!=28)check(frames[i]==jp_frames[i],"Japanese title animation frame preserved");
                for(size_t q=0;q<frames[28].size();q+=4)
                    check(int(int8_t(frames[28][q+2]))==int(int8_t(jp_frames[28][q+2]))-42,"4 rotates around US native anchor");
                for(unsigned i=31;i<35;++i)check(frames[i].size()==4,"US-only frame is a bounded transparent quad");
                check(!replacements.count("ARC/MOJIPAT.ARC"),"common US menu/portrait sprite tables stay untouched");
            } else check(member(title,0x12)==member(us_title,0x12),"stock title sprite table untouched");
            check(member(title,0x20000)==member((options&2)?jp_title:us_title,0x20000),"title audio belongs to voices option");
            const auto& font=member(load,0x10001);
            check((font==us_font)==!(options&8),"only translation changes native font atlas");
            for(unsigned i=0;i<3;++i) {
                auto arc=unpack(read_file(names[i]));auto records=messages(member(arc,0x15));
                check(records.size()==source_records[i].size(),"stable native message IDs");
                check(records.back()==source_records[i].back(),"non-text sentinel preserved");
                check((records[0]==source_records[i][0])==!(options&8),"Spanish applies independently of Japanese media");
                if(options&8)for(size_t n=0;n+1<records.size();++n)
                    check((records[n].back()&0xa0)==0xa0,"translated native message terminator");
            }
            // Publication is idempotent and preserves unrelated executable data.
            const auto before=ram;for(const auto& cb:ticks())cb.second();check(ram==before,"idempotent language tables");
            std::cout<<"PASS: owner-disc combination "<<options<<" streams="<<appended<<" archives="<<replacements.size()<<'\n';
        }
        std::cout<<"PASS: all 16 "<<language<<" native asset combinations; actual guest playback remains a separate check\n";
        return 0;
    }catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
