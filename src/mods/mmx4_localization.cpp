// Optional language assets remain on the original ARC / CD / GPU / SPU path.
// See docs/LOCALIZATION.md for verified donor identity and native contracts.
#include "mod_plugins.h"
#include "mod_media.h"
#include "native_text.h"
#include "mod_resident.h"
#include "psx_sha256.h"
#include "crash_trace.h"
#include "mmx4_resident_catalog.h"
#include "mmx4_language_assets.h"
#include "mmx4_fmv_pack.h"
#include <array>
#include <cstdio>
#include <iterator>
#include <string>
#include <cstring>

namespace {
using namespace PSXRecompV4;
using namespace Mmx4Language;
constexpr const char* Package="mmx4.localization";
constexpr const char* Media="mmx4.japanese-media";
constexpr const char* Translation="mmx4.translation";
constexpr const char* Movies="mmx4.japanese-fmvs";
struct Patch {uint32_t address,original,value;};
std::vector<Patch> patches;
std::vector<std::pair<uint32_t,std::vector<uint8_t>>> guest_blobs;
std::map<unsigned,std::vector<Member>> archives;
std::map<unsigned,std::vector<uint8_t>> archive_headers;
bool applied=false,exe_entered=false;
bool fan_movies_active=false;

void fail(const std::string& message) {psx_fatal_halt(("X4 language mod: "+message).c_str());}
std::string sha(const std::vector<uint8_t>& data) {
    uint8_t digest[32];psx_sha256_compute(data.data(),data.size(),digest);
    std::string text;for(auto b:digest) {text+="0123456789abcdef"[b>>4];text+="0123456789abcdef"[b&15];}return text;
}
bool enabled(const char* id) {
    char value[16]{};
    return psx_mod_option_value(Package,"japanese-media",id,value,sizeof value) && !std::strcmp(value,"true");
}
std::vector<uint8_t> original_file(const char* path) {
    uint32_t size=0;
    if(!psx_mod_read_disc_file(path,nullptr,0,&size) || !size || size>4*1024*1024)
        throw std::runtime_error(std::string("Cannot read ")+path);
    std::vector<uint8_t> data(size);
    if(!psx_mod_read_disc_file(path,data.data(),size,&size))throw std::runtime_error("Cannot read effective X4 asset");
    return data;
}
void patch(uint32_t address,uint32_t original,uint32_t value) {
    for(auto& p:patches)if(p.address==address) {p.value=value;return;}
    patches.push_back({address,original,value});applied=false;
}
std::vector<Member>& archive(unsigned index) {
    const auto found=archives.find(index);
    if(found!=archives.end())return found->second;
    const auto data=original_file(MMX4Files[index].path);
    archive_headers[index]={data.begin(),data.begin()+2048};
    return archives.emplace(index,unpack(data)).first->second;
}
void mount_file(unsigned index,const std::vector<uint8_t>& data) {
    uint32_t lba=0;
    if(!psx_mod_replace_disc_file(MMX4Files[index].path,data.data(),uint32_t(data.size()),&lba))
        throw std::runtime_error("Cannot mount derived X4 language archive");
    const auto& c=MMX4FileContracts[index];const uint32_t table=0x800f0e18+index*12;
    patch(table,c.lba,lba);patch(table+4,c.size,uint32_t(data.size()));patch(table+8,c.first,u32(data.data()));
}
void mount_archive(unsigned index) {
    mount_file(index,pack(archive(index),archive_headers.at(index)));
}
unsigned index_for(const char* name) {
    for(unsigned i=0;i<std::size(MMX4Files);++i)if(!std::strcmp(name,MMX4Files[i].path))return i;
    throw std::runtime_error("Missing native X4 archive contract");
}
void reset() {patches.clear();guest_blobs.clear();archives.clear();archive_headers.clear();applied=false;exe_entered=false;fan_movies_active=false;}
void publish() {
    if(!exe_entered || applied || patches.empty())return;
    // Wait for the EXE. Validate the whole transaction before its first write.
    if(psx_mod_read_word(0x800f0e18)!=MMX4FileContracts[0].lba)return;
    for(const auto& p:patches)
        if(psx_mod_read_word(p.address)!=p.original && psx_mod_read_word(p.address)!=p.value)
        {
            char detail[160];std::snprintf(detail,sizeof detail,
                "Executable table at %08x: expected %08x, found %08x",p.address,p.original,psx_mod_read_word(p.address));
            fail(detail);
        }
    for(const auto& p:patches)psx_mod_write_word(p.address,p.value);
    for(const auto& blob:guest_blobs)
        for(size_t i=0;i<blob.second.size();++i)psx_mod_write_byte(blob.first+uint32_t(i),blob.second[i]);
    applied=true;psx_mod_counter_add("mmx4.language.tables_published",1);
}
void entry(CPUState*,uint32_t) {exe_entered=true;publish();}
void restored() {applied=false;exe_entered=psx_mod_game_started()!=0;publish();}

std::string donor_name(const std::string& us,const ModMode2Disc& donor) {
    uint32_t lba=0,size=0;
    std::vector<std::string> names{us};
    auto add=[&](const std::string& from,const std::string& to) {
        auto candidate=us;const auto p=candidate.find(from);
        if(p!=std::string::npos) {candidate.replace(p,from.size(),to);names.push_back(candidate);}
    };
    add("_U.ARC",".ARC");add("_U0.ARC","_00.ARC");add("_U1.ARC","_01.ARC");
    add("_UX.ARC","_0X.ARC");add("_UZ.ARC","_0Z.ARC");
    // Prefer the original Japanese campaign files over JP's alternate US assets.
    std::reverse(names.begin(),names.end());
    for(const auto& n:names)if(donor.find(n,lba,size))return n;
    return {};
}
struct Stream {const char* jp;uint32_t table,lba,size,first;};
// Original US table sizes count 2336-byte sectors; the ISO file sizes count 2048.
constexpr Stream movies[]={
    {"STR/CAPCOM20.STR",0x800f1490,12947,2805536,0x80420100},
    {"STR/OP.STR",0x800f149c,14148,31594400,0x00480100},
    {"STR/X1.STR",0x800f14a8,27673,24436896,0x00480100},
    {"STR/X2.STR",0x800f14b4,38134,30536192,0x00480100},
    {"STR/X3.STR",0x800f14c0,51206,24974176,0x00480100},
    {"STR/X4.STR",0x800f14cc,61897,31246336,0x00480100},
    {"STR/Z1.STR",0x800f14d8,75273,22163968,0x00480100},
    {"STR/Z2.STR",0x800f14e4,84761,28742144,0x00480100},
    {"STR/Z3.STR",0x800f14f0,97065,23397376,0x00480100},
    {"STR/Z4.STR",0x800f14fc,107081,74340864,0x00480100},
    {"STR/Z5.STR",0x800f1508,138905,24014080,0x00480100}};
constexpr Stream voices[]={
    {"XA/BOSSINT.XA",0x800f1550,228114,5101824,0x01640001},
    {"XA/VOICE1.XA",0x800f155c,230298,953088,0x01640001},
    {"XA/VOICE2.XA",0x800f1568,230706,990464,0x01640001},
    {"XA/VOICE3.XA",0x800f1574,231130,1308160,0x01640001},
    {"XA/VOICE4.XA",0x800f1580,231690,1438976,0x01640001},
    {"XA/VOICE5.XA",0x800f158c,232306,1569792,0x01640001}};
void stream(const ModMode2Disc& donor,const uint8_t* raw,const Stream& s) {
    uint32_t lba=0,size=0,mounted=0;
    if(!donor.find(s.jp,lba,size) || size%2048 ||
       !psx_mod_append_disc_extent("japanese-disc",uint64_t(lba)*2336,size/2048,&mounted))
        throw std::runtime_error(std::string("Cannot mount Japanese stream ")+s.jp);
    patch(s.table,s.lba,mounted);patch(s.table+4,s.size,(size/2048)*2336);
    patch(s.table+8,s.first,u32(raw+uint64_t(lba)*2336));
}
void fan_activate(const char* resource,const char* attribution) {
    try {
        if(fan_movies_active)throw std::runtime_error("Choose one fan subtitle language per session");
        const uint8_t* bytes=nullptr;uint64_t size=0;
        if(!psx_mod_current_resource_bytes(resource,&bytes,&size))
            throw std::runtime_error("Select the matching imported .x4fmv under Japanese FMVs (tools/import_fmv_subtitles.py)");
        const auto extents=fmv_pack(bytes,size); // Validate the entire pack before mounting.
        for(unsigned i=0;i<extents.size();++i) {
            const auto& extent=extents[i];const auto& movie=movies[i+1];uint32_t mounted=0;
            if(!psx_mod_append_disc_extent(resource,extent.offset,extent.sectors,&mounted))
                throw std::runtime_error("Cannot mount fan movie stream");
            patch(movie.table,movie.lba,mounted);
            patch(movie.table+4,movie.size,extent.sectors*2336);
            patch(movie.table+8,movie.first,u32(bytes+extent.offset));
        }
        fan_movies_active=true;
        std::fprintf(stderr,"mmx4.language: ten fan-subtitled Japanese movies mounted (%s); see Japanese FMVs CREDITS.md and provenance/\n",attribution);
    } catch(const std::exception& e) {fail(e.what());}
}
void movies_activate() {
    char subtitles[16]{};
    psx_mod_option_value(Movies,"japanese-fmvs","subtitles",subtitles,sizeof subtitles);
    if(!std::strcmp(subtitles,"en")) {
        fan_activate("english-movies","acediez Undub v1.13 / Hondoori translation");
    } else if(!std::strcmp(subtitles,"es")) {
        fan_activate("spanish-movies","El Retro Dan Retraducido Final; based on acediez Undub");
    } else if(!std::strcmp(subtitles,"none")) {
        try {
            const uint8_t* bytes=nullptr;uint64_t size=0;
            if(!psx_mod_current_resource_bytes("japanese-disc",&bytes,&size))
                throw std::runtime_error("Select the verified original Japanese disc for movies without subtitles");
            ModMode2Disc donor(bytes,size);
            for(const auto& s:movies)stream(donor,bytes,s);
            std::fprintf(stderr,"mmx4.language: original Japanese movies mounted without subtitles\n");
        } catch(const std::exception& e) {fail(e.what());}
    } else fail("Unsupported Japanese movie subtitle selection");
}
void media_activate() {
    try {
        if(!enabled("voices") && !enabled("title-screen"))return;
        const uint8_t* bytes=nullptr;uint64_t size=0;
        if(!psx_mod_current_resource_bytes("japanese-disc",&bytes,&size))throw std::runtime_error("Select a verified Japanese disc");
        ModMode2Disc donor(bytes,size);
        if(enabled("title-screen")) {
            // US spawns the wordmark before its separate X/4 objects. JP's
            // word extends across X and must be drawn in front. Reorder only
            // these native object IDs; their coordinates/handlers stay native.
            patch(0x800f21a0,0x0b001312,0x0c0b1312);
            patch(0x800f21a4,0x0911070c,0x09110700);
            std::vector<uint8_t> source;
            if(!donor.read("SLPS_009.02",source) || source.size()<0xe2070)
                throw std::runtime_error("Japanese loading-logo palettes are missing");
            // These two 16-colour CLUTs live in the EXE, not in LOAD.ARC or
            // MOJIPAT. A texture-only swap displays the wrong indexed colours.
            constexpr uint32_t palette[]={0xf8830000,0xfd48fcc4,0xfe31fdcd,0xfaf7fab5,
                0xa8c7f79c,0xd252b94a,0xc8a5bc41,0xd928ad08,0x805f0000,0x85de811f,
                0x96fe869e,0xfbbed7bf,0xb94aa8c7,0xcc62d274,0xe8f3d8ab,0xb21991de};
            for(unsigned i=0;i<std::size(palette);++i)
                patch(0x800f16a0+i*4,palette[i],u32(source.data()+0xe2030+i*4));
            if(!donor.read("ARC/LOAD.ARC",source))throw std::runtime_error("Japanese title atlas is missing");
            auto jp=unpack(source);const unsigned index=index_for("ARC/LOAD_U.ARC");
            auto& logo=member(archive(index),0x10008);
            const auto& replacement=member(jp,0x10008);
            if(logo.size()!=replacement.size())throw std::runtime_error("Japanese title atlas changed dimensions");
            logo=replacement;mount_archive(index);
            // The main title is a separate native scene, with its own sprite
            // frames, tile layout and CLUT. Preserve US audio unless requested.
            for(const char* us_name:{"ARC/ST0E_U0.ARC","ARC/COL0E_U0.ARC"}) {
                const auto jp_name=donor_name(us_name,donor);
                if(jp_name.empty() || !donor.read(jp_name,source))throw std::runtime_error("Japanese title scene is missing");
                auto scene=unpack(source);const unsigned file=index_for(us_name);
                auto& target=archive(file);
                for(auto& m:target) {
                    if(m.type>>16==2 || m.type==4 || m.type==6 || m.type==7 || m.type==8)continue;
                    m.data=member(scene,m.type);
                }
                if(!std::strcmp(us_name,"ARC/ST0E_U0.ARC")) {
                    auto frames=sprite_frames(member(target,0x12));
                    if(frames.size()!=31)throw std::runtime_error("Japanese title frame contract changed");
                    // US animates the 4 around its own centre; JP stores it
                    // beside X. Remove that baked-in 42-pixel displacement.
                    for(size_t q=0;q<frames[28].size();q+=4)
                        frames[28][q+2]=uint8_t(int(int8_t(frames[28][q+2]))-42);
                    // US-only outline/shimmer frames must never index into
                    // Japanese quad data as if it were a frame header.
                    const auto& tex=member(target,0x10002);std::vector<uint8_t> blank;
                    for(unsigned t=0;t<256 && blank.empty();++t) {
                        bool empty=true;
                        for(unsigned y=0;y<16;++y)for(unsigned x=0;x<8;++x)
                            empty=empty && !tex[((t>>4)*16+y)*128+(t&15)*8+x];
                        if(empty)blank={0,uint8_t(t),0,0};
                    }
                    if(blank.empty())throw std::runtime_error("Japanese title has no transparent sprite cell");
                    frames.resize(35,blank);member(target,0x12)=sprite_table(frames);
                }
                mount_archive(file);
            }
        }
        if(enabled("voices")) {
            for(const auto& s:voices)stream(donor,bytes,s);
            std::vector<uint8_t> exe;
            if(!donor.read("SLPS_009.02",exe))throw std::runtime_error("Japanese cue tables are missing");
            constexpr uint32_t expected[]={0x800f1920,0x800f1940,0x800f1960,0x800f1980,0x800f19a0,0x800f19c0};
            for(unsigned i=0;i<6;++i) {
                const auto jp_at=0x800f1b84-0x80010000+0x800+i*4;
                const uint32_t pointer=u32(exe.data()+jp_at);
                const uint32_t end=i==5?0x800f1b70:u32(exe.data()+jp_at+4);
                if(pointer<0x80010000 || end<=pointer || end-pointer>64 ||
                   uint64_t(end)-0x80010000+0x800>exe.size())throw std::runtime_error("Japanese XA cue table changed");
                const uint32_t guest=psx_mod_alloc_guest_memory(end-pointer,4);
                if(!guest)throw std::runtime_error("Cannot allocate native XA cue table");
                guest_blobs.push_back({guest,{exe.begin()+pointer-0x80010000+0x800,exe.begin()+end-0x80010000+0x800}});
                patch(0x800f19f4+i*4,expected[i],guest);
            }
            for(unsigned i=0;i<std::size(MMX4Files);++i) {
                if(!MMX4FileContracts[i].packed)continue;
                auto& us=archive(i);bool has_audio=false;
                for(const auto& m:us)if(m.type>>16==2)has_audio=true;
                if(!has_audio)continue;
                const auto name=donor_name(MMX4Files[i].path,donor);
                std::vector<uint8_t> source;
                if(name.empty() || !donor.read(name,source))throw std::runtime_error("Missing Japanese sound archive counterpart");
                auto jp=unpack(source);std::vector<size_t> a,b,ah,bh;
                auto inventory=[](const std::vector<Member>& bank,std::vector<size_t>& bodies,std::vector<size_t>& headers) {
                    for(size_t n=0;n<bank.size();++n) {
                        if(bank[n].type>>16==2)bodies.push_back(n);
                        if(bank[n].type==4 || bank[n].type==6 || bank[n].type==7 || bank[n].type==8)headers.push_back(n);
                    }
                };
                inventory(us,a,ah);inventory(jp,b,bh);
                if(a.size()!=b.size() || a.size()!=ah.size() || b.size()!=bh.size())
                    throw std::runtime_error("Japanese sound bank inventory changed");
                for(size_t n=0;n<a.size();++n) {
                    const size_t u=a[n],j=b[n],h=ah[n],k=bh[n];
                    if(jp[k].type!=us[h].type || us[u].type!=jp[j].type)
                        throw std::runtime_error(std::string("Japanese VAB pair contract changed: ")+MMX4Files[i].path+
                            " pair "+std::to_string(n));
                    us[h].data=jp[k].data;us[u].data=jp[j].data;
                }
                mount_archive(i);
            }
        }
        std::fprintf(stderr,"mmx4.language: Japanese media prepared voices=%u title=%u\n",enabled("voices"),enabled("title-screen"));
    } catch(const std::exception& e) {fail(e.what());}
}

NativeGlyphMap glyph_map() {
    NativeGlyphMap map;
    for(unsigned i=0;i<10;++i)map['0'+i]=0x40+i;
    for(unsigned i=0;i<26;++i) {map['A'+i]=0x4a+i;map['a'+i]=0x64+i;}
    const std::u32string punctuation=U"/↓↑←→+-,.!?\"\"'";
    for(size_t i=0;i<punctuation.size();++i)map[punctuation[i]]=uint16_t(0x7e + i);
    return map;
}
void translate_activate() {
    try {
        char language[32]{};
        if(!psx_mod_option_value(Package,"translation","language",language,sizeof language))std::strcpy(language,"es");
        const char* resource=!std::strcmp(language,"pt-br")?"catalog-pt-br":"catalog";
        // The selected catalog is verified by the plan; consume its immutable
        // bytes so later file edits cannot change a committed session.
        const uint8_t* data=nullptr;uint64_t size=0;
        if(!psx_mod_current_resource_bytes(resource,&data,&size) || size>8*1024*1024)
            throw std::runtime_error("Translation catalog is not a verified resource");
        NativeTextCatalog catalog;std::string error;
        if(!catalog.load(std::string(reinterpret_cast<const char*>(data),size),language,error))throw std::runtime_error(error);
        auto glyphs=glyph_map();
        const unsigned load=index_for("ARC/LOAD_U.ARC");
        NativeFontAtlas font;font.packed=member(archive(load),0x10001);
        const std::u32string accents=U"áéíóúüñÁÉÍÓÚÜÑ¿¡àâêôãõçÀÂÊÔÃÕÇ";
        const std::string bases="aeiouunAEIOUUN?!aaeoaocAAEOAOC";
        for(size_t i=0;i<accents.size();++i) {
            const uint16_t cell=uint16_t(i<16?0xe0+i:0xd0+i-16);
            const unsigned pt=i>=16?(i-16)%7:99;
            const auto mark=pt==0?NativeGlyphMark::Grave:
                (pt==1 || pt==2 || pt==3)?NativeGlyphMark::Circumflex:
                (pt==4 || pt==5)?NativeGlyphMark::Tilde:
                pt==6?NativeGlyphMark::Cedilla:
                (i==14 || i==15)?NativeGlyphMark::Inverted:
                (i==6 || i==13)?NativeGlyphMark::Tilde:
                (i==5 || i==12)?NativeGlyphMark::Diaeresis:NativeGlyphMark::Acute;
            if(!native_font_compose(font,glyphs.at(uint8_t(bases[i])),cell,mark,error,i==2?6:0))throw std::runtime_error(error);
            glyphs[accents[i]]=cell;
        }
        unsigned translated=0;
        for(unsigned character=0;character<3;++character) {
            const unsigned index=index_for(character==1?"ARC/PL01_U.ARC":character==2?"ARC/PL02_U.ARC":"ARC/PL00_U.ARC");
            auto records=messages(member(archive(index),0x15));
            bool changed=false;
            for(const auto& record:records)for(size_t p=0;p<record.size();p+=2) {
                const unsigned c=(record[p]|unsigned(record[p+1])<<8)&0x1ff;
                if(c>=0xd0 && c<=0xef)throw std::runtime_error("Native message uses a reserved accent cell");
            }
            for(size_t i=0;i<records.size();++i) {
                char id[96];std::snprintf(id,sizeof id,"dialogue.%s.%03u",character==1?"zero":"x",unsigned(i));
                const auto* e=catalog.find(id);
                if(!e || e->translation.empty())continue;
                if(e->source_sha256!=sha(records[i]))throw std::runtime_error(std::string("Stale source for ")+id);
                NativeTextLayout layout;layout.space=0x1ff;layout.line_end=0x4000;
                layout.page_end=0x2000;layout.text_end=0x8000;layout.progressive=0x1000;
                layout.style=uint16_t(records[i][1]&8)<<8;
                std::vector<uint16_t> words;
                if(!native_text_encode(e->translation,glyphs,layout,words,error))throw std::runtime_error(std::string(id)+": "+error);
                records[i].clear();for(auto word:words) {records[i].push_back(uint8_t(word));records[i].push_back(uint8_t(word>>8));}
                changed=true;++translated;
            }
            if(changed) {member(archive(index),0x15)=message_bank(records);mount_archive(index);}
        }
        member(archive(load),0x10001)=std::move(font.packed);mount_archive(load);
        std::fprintf(stderr,"mmx4.language: %s translation prepared %u native bank records; 30 native-font accent cells\n",language,translated);
    } catch(const std::exception& e) {fail(e.what());}
}
}
PSX_MOD_CONSTRUCTOR(mmx4_register_languages) {
    psx_mod_register_session_reset_plugin(Media,reset);
    psx_mod_register_activation_plugin(Media,media_activate);
    psx_mod_register_activation_plugin(Translation,translate_activate);
    psx_mod_register_session_reset_plugin(Movies,reset);
    psx_mod_register_activation_plugin(Movies,movies_activate);
    for(const char* id:{Media,Translation,Movies}) {
        psx_mod_register_function_entry_plugin(id,0x800dae8c,entry);
        psx_mod_register_vblank_plugin(id,publish);
        psx_mod_register_savestate_plugin(id,restored);
    }
}
