// Retained original PolyForm Noncommercial 1.0.0 implementation; see README.md.
#include "mode2_disc.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <utility>

namespace Mmx4Media {
namespace {
uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
uint32_t be32(const uint8_t* p) {
    return uint32_t(p[3]) | (uint32_t(p[2]) << 8) |
           (uint32_t(p[1]) << 16) | (uint32_t(p[0]) << 24);
}

} // namespace

bool ModMode2Disc::find(const std::string& path,uint32_t& lba,uint32_t& size) const {
    lba=size=0;
    if(!bytes_ || size_%2336 || size_<17*2336)return false;
    const uint8_t* pvd=bytes_+16*2336+8;
    if(pvd[0]!=1 || std::memcmp(pvd+1,"CD001",5) || pvd[6]!=1)return false;
    uint32_t at_lba=le32(pvd+158),at_size=le32(pvd+166);
    bool directory=true;size_t at=0;
    while(at<path.size()) {
        const size_t slash=path.find_first_of("/\\",at);
        const size_t end=slash==std::string::npos?path.size():slash;
        auto want=path.substr(at,end-at);at=end+1;
        if(want.empty())continue;
        if(want=="." || want==".." || !directory)return false;
        std::transform(want.begin(),want.end(),want.begin(),[](unsigned char c){return char(std::toupper(c));});
        const uint64_t count=(uint64_t(at_size)+2047)/2048;
        if(!count || at_lba>size_/2336 || count>size_/2336-at_lba || at_size>16*1024*1024)return false;
        bool found=false;
        for(uint32_t i=0;i<count && !found;++i) {
            const auto* sector=bytes_+uint64_t(at_lba+i)*2336;
            if(sector[2]&0x20)return false;
            const auto* data=sector+8;
            const uint32_t limit=uint32_t(std::min<uint64_t>(2048,uint64_t(at_size)-uint64_t(i)*2048));
            for(uint32_t p=0;p<limit && data[p];p+=data[p]) {
                const uint32_t n=data[p];
                if(n<34 || n>limit-p)return false;
                const uint32_t len=data[p+32];
                if(33+len>n)return false;
                std::string name(reinterpret_cast<const char*>(data+p+33),len);
                name=name.substr(0,name.find(';'));
                std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::toupper(c));});
                if(name!=want)continue;
                if(le32(data+p+2)!=be32(data+p+6) || le32(data+p+10)!=be32(data+p+14))return false;
                at_lba=le32(data+p+2);at_size=le32(data+p+10);directory=(data[p+25]&2)!=0;
                found=true;break;
            }
        }
        if(!found)return false;
    }
    const uint64_t count=(uint64_t(at_size)+2047)/2048;
    if(directory || !at_size || at_lba>size_/2336 || count>size_/2336-at_lba)return false;
    lba=at_lba;size=at_size;return true;
}
bool ModMode2Disc::read(const std::string& path,std::vector<uint8_t>& out) const {
    out.clear();uint32_t lba=0,size=0;
    if(!find(path,lba,size) || size>16*1024*1024)return false;
    std::vector<uint8_t> result(size);
    for(uint32_t p=0;p<size;) {
        const auto* sector=bytes_+uint64_t(lba+p/2048)*2336;
        if(sector[2]&0x20)return false;
        const uint32_t n=std::min(2048u,size-p);
        std::memcpy(result.data()+p,sector+8,n);p+=n;
    }
    out=std::move(result);return true;
}
} // namespace Mmx4Media
