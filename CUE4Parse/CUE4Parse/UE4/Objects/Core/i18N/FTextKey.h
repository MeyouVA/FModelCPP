// Ported from CUE4Parse/UE4/Objects/Core/i18N/FTextKey.cs
// A .locres namespace or key: the string, plus the pre-computed hash newer formats store alongside it.
//
// Deliberate difference from C#: FTextKey is a class there with no Equals/GetHashCode override, so the
// Dictionary<FTextKey, ...> in FTextLocalizationResource keys on *reference* identity — two distinct
// FTextKey objects never collide even when their Str matches. FTextLocalizationResource keeps that
// behaviour by storing its entries in insertion-ordered vectors rather than a std::map (see that header),
// so no comparison operator is defined here either.
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "../../../IUStruct.h"
#include "ELocResVersion.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    class FTextKey : public UE4::IUStruct
    {
    public:
        std::string Str;
        // Zero below Optimized_CRC32 — those formats do not store a hash (C# leaves the field at 0 too).
        uint32_t StrHash = 0;

        FTextKey(Readers::FArchive& Ar, ELocResVersion versionNum);

        explicit FTextKey(std::string str, uint32_t hash = 0) : Str(std::move(str)), StrHash(hash) {}
    };
}
