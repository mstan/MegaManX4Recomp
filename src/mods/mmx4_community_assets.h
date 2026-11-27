#pragma once
#include "mmx4_language_assets.h"
#include "psx_sha256.h"
#include <array>
#include <cstring>
#include <map>
#include <string>

namespace Mmx4Community {
using Bytes = std::vector<uint8_t>;
using Archive = std::vector<Mmx4Language::Member>;
using Archives = std::array<Archive, 3>;
enum : unsigned { ZeroBuster=1, TransparentHud=2, HybridBuster=4, PlasmaFilter=8 };

// The payload holds copy recipes and authored additions, not extracted ARCs
// or a second retail image. Every source member and finished result is guarded.
struct Reader {
    const uint8_t* data; size_t size, at=0;
    Bytes take(size_t n) {
        if (at>size || n>size-at) throw std::runtime_error("Truncated community asset recipe");
        Bytes result(data+at,data+at+n); at+=n; return result;
    }
    uint32_t word() { auto b=take(4); return Mmx4Language::u32(b.data()); }
};
inline bool digest(const Bytes& data,const Bytes& expected) {
    uint8_t hash[32]; psx_sha256_compute(data.data(),data.size(),hash);
    return expected.size()==32 && !std::memcmp(hash,expected.data(),32);
}
inline Bytes compact(const Bytes& original,const std::vector<uint16_t>& dictionary) {
    using namespace Mmx4Language;
    if (dictionary.size()!=258) throw std::runtime_error("Invalid community glyph dictionary");
    const auto messages=Mmx4Language::messages(original);
    Bytes result(messages.size()*2+516);
    std::map<uint16_t,unsigned> indexes;
    for (unsigned i=0;i<258;++i) {
        result[messages.size()*2+i*2]=uint8_t(dictionary[i]);
        result[messages.size()*2+i*2+1]=uint8_t(dictionary[i]>>8);
        indexes.emplace(dictionary[i],i);
    }
    for (size_t i=0;i<messages.size();++i) {
        if (result.size()>65535) throw std::runtime_error("Community message bank overflow");
        result[i*2]=uint8_t(result.size()); result[i*2+1]=uint8_t(result.size()>>8);
        const auto& message=messages[i];
        for (size_t j=0;j<message.size();j+=2) {
            const auto found=indexes.find(u16(message.data()+j));
            // A variant may replace a message containing a removed glyph.
            // Its guarded recipe replaces that span after this basis is made.
            const unsigned index=found==indexes.end()?0:found->second;
            if (index<255) result.push_back(uint8_t(index));
            else { result.push_back(255); result.push_back(uint8_t(index-255)); }
        }
        if (result.size()&1) result.push_back(0);
    }
    return result;
}
struct Recipe {
    unsigned flag,archive,member,type,source_size,target_size;
    Bytes source_sha,target_sha;
    std::vector<uint16_t> dictionary;
    struct Op { unsigned kind,offset,size; Bytes literal; };
    std::vector<Op> operations;
};
inline std::vector<Recipe> recipes(const uint8_t* data,size_t size) {
    Reader reader{data,size};
    if (reader.take(8)!=Bytes({'X','4','P','A','T','C','H','1'}))
        throw std::runtime_error("Invalid community asset recipe signature");
    const unsigned count=reader.word();
    if (!count || count>64) throw std::runtime_error("Invalid community recipe count");
    std::vector<Recipe> result;
    for (unsigned i=0;i<count;++i) {
        Recipe recipe{};
        recipe.flag=reader.word();recipe.archive=reader.word();recipe.member=reader.word();
        recipe.type=reader.word();recipe.source_size=reader.word();recipe.target_size=reader.word();
        if ((recipe.flag!=0 && recipe.flag!=1 && recipe.flag!=2 && recipe.flag!=4) ||
            recipe.archive>2 || recipe.member>16 || recipe.source_size>1024*1024 ||
            !recipe.target_size || recipe.target_size>1024*1024)
            throw std::runtime_error("Community member recipe exceeds its contract");
        recipe.source_sha=reader.take(32);recipe.target_sha=reader.take(32);
        const unsigned words=reader.word();
        if (words && (words!=258 || recipe.type!=0x15))
            throw std::runtime_error("Invalid community text recipe");
        for (unsigned word=0;word<words;++word) {
            auto bytes=reader.take(2);recipe.dictionary.push_back(Mmx4Language::u16(bytes.data()));
        }
        const unsigned operations=reader.word();
        if (operations>4096) throw std::runtime_error("Too many community copy operations");
        uint64_t total=0;
        for (unsigned op=0;op<operations;++op) {
            Recipe::Op operation{};operation.kind=reader.word();operation.offset=reader.word();operation.size=reader.word();
            total+=operation.size;
            if (operation.kind>1 || !operation.size || total>recipe.target_size)
                throw std::runtime_error("Invalid community copy operation");
            if (operation.kind) operation.literal=reader.take(operation.size);
            recipe.operations.push_back(std::move(operation));
        }
        if (total!=recipe.target_size) throw std::runtime_error("Incomplete community member recipe");
        result.push_back(std::move(recipe));
    }
    if (reader.at!=reader.size) throw std::runtime_error("Trailing community recipe data");
    return result;
}
inline Bytes derive(const Recipe& recipe,const Bytes& original) {
    if (original.size()!=recipe.source_size || !digest(original,recipe.source_sha))
        throw std::runtime_error("Community patch needs original English gameplay assets; a source member differs");
    const auto source=recipe.dictionary.empty()?original:compact(original,recipe.dictionary);
    Bytes result;result.reserve(recipe.target_size);
    for (const auto& op:recipe.operations) {
        if (op.kind) result.insert(result.end(),op.literal.begin(),op.literal.end());
        else {
            if (op.offset>source.size() || op.size>source.size()-op.offset)
                throw std::runtime_error("Community copy exceeds its source member");
            result.insert(result.end(),source.begin()+op.offset,source.begin()+op.offset+op.size);
        }
    }
    if (result.size()!=recipe.target_size || !digest(result,recipe.target_sha))
        throw std::runtime_error("Community asset result failed its digest guard");
    return result;
}
inline Archives compose(const Archives& original,const std::vector<Recipe>& recipes,unsigned enabled) {
    if (!enabled) return original;
    auto result=original;
    // Each replacement is derived from the immutable original. HUD's small
    // pixel/assembly edits are then merged over Zero's expanded sprite table.
    for (unsigned phase: {0u,1u,4u,2u}) for (const auto& recipe:recipes) {
        if (recipe.flag!=phase || (phase && !(enabled&phase))) continue;
        if (!phase && ((recipe.archive==1 && (enabled&ZeroBuster)) ||
                      (recipe.archive!=1 && (enabled&HybridBuster)))) continue;
        const auto& members=original[recipe.archive];
        const Bytes empty;
        if (recipe.member>members.size() ||
            (recipe.member<members.size() && members[recipe.member].type!=recipe.type))
            throw std::runtime_error("Community ARC member identity differs");
        const auto& source=recipe.member==members.size()?empty:members[recipe.member].data;
        auto transformed=derive(recipe,source);
        auto& output=result[recipe.archive];
        if (phase==TransparentHud) {
            auto& current=output.at(recipe.member).data;
            if (source.size()!=transformed.size() || current.size()<source.size())
                throw std::runtime_error("Incompatible HUD asset dimensions");
            for (size_t i=0;i<source.size();++i) if (source[i]!=transformed[i]) {
                if (current[i]!=source[i] && current[i]!=transformed[i])
                    throw std::runtime_error("HUD and sprite edits conflict");
                current[i]=transformed[i];
            }
        } else if (recipe.member==output.size()) output.push_back({recipe.type,std::move(transformed)});
        else output[recipe.member].data=std::move(transformed);
    }
    return result;
}
} // namespace Mmx4Community
