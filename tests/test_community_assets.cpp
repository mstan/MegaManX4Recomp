#include "mmx4_community_assets.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace Mmx4Community;
namespace fs=std::filesystem;
Bytes read(const fs::path& path) {
    std::ifstream stream(path,std::ios::binary);
    if (!stream) throw std::runtime_error("Cannot read "+path.string());
    return Bytes(std::istreambuf_iterator<char>(stream),{});
}
int main(int argc,char** argv) {
    try {
        if (argc<2 || argc>4) throw std::runtime_error("expected payload [private-original-ARC-directory [private-reference-directory]]");
        auto payload=read(argv[1]);auto decoded=recipes(payload.data(),payload.size());
        if (decoded.size()!=17) throw std::runtime_error("missing patch asset recipes");
        for (size_t length: {size_t(0),size_t(7),payload.size()-1}) {
            bool rejected=false;
            try {recipes(payload.data(),length);} catch (const std::runtime_error&) {rejected=true;}
            if (!rejected) throw std::runtime_error("truncated asset recipe accepted");
        }
        if (argc>=3) {
            Archives original;
            const char* names[]={"PL00_U.ARC","PL01_U.ARC","PL02_U.ARC"};
            for (unsigned i=0;i<3;++i) original[i]=Mmx4Language::unpack(read(fs::path(argv[2])/names[i]));
            for (unsigned mask=1;mask<16;++mask) {
                auto result=compose(original,decoded,mask);
                for (unsigned i=0;i<3;++i) {
                    const auto packed=Mmx4Language::pack(result[i]);
                    if (Mmx4Language::unpack(packed).size()!=result[i].size())
                        throw std::runtime_error("derived ARC fails native container contract");
                }
            }
            // An unrelated asset modification must refuse the whole preparation.
            auto damaged=original;damaged[0][15].data[4]^=1;
            bool rejected=false;
            try {compose(damaged,decoded,15);} catch (const std::runtime_error&) {rejected=true;}
            if (!rejected) throw std::runtime_error("changed original message bank accepted");
            if (argc==4) {
                const char* prefixes[]={"Patch - Zero Buster (X5)","Patch - HUD w Real Transparency",
                    "Patch - Hybrid Buster","Patch - Plasma Ball won_t hit bosses"};
                for (unsigned feature=0;feature<4;++feature) {
                    const auto result=compose(original,decoded,1u<<feature);
                    for (unsigned i=0;i<3;++i) {
                        const auto path=fs::path(argv[3])/(std::string(feature==0 && i==1?"Zero":prefixes[feature])+"-"+names[i]);
                        auto expected=original[i];
                        if (fs::exists(path)) {
                            // Source patches retain unused tail sectors. Compare
                            // the native members, not that irrelevant padding.
                            auto bytes=read(path);const unsigned count=Mmx4Language::u32(bytes.data());
                            size_t end=2048;
                            for (unsigned n=0;n<count;++n) end=(end+Mmx4Language::u32(bytes.data()+12+n*8)+2047)&~size_t(2047);
                            bytes.resize(end);Mmx4Language::put32(bytes,4,uint32_t(end));expected=Mmx4Language::unpack(bytes);
                        }
                        if (expected.size()!=result[i].size()) throw std::runtime_error("reference member count mismatch");
                        for (size_t member=0;member<expected.size();++member)
                            if (expected[member].type!=result[i][member].type || expected[member].data!=result[i][member].data)
                                throw std::runtime_error("supplied patch reference mismatch: "+path.string());
                    }
                }
            }
            std::cout<<"All fifteen combinations compose; original guards reject stale input; supplied member references match.\n";
        } else std::cout<<"Built-in asset payload parses; truncated input is rejected.\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
