// Ported from CUE4Parse/UE4/Objects/Core/i18N/FTextLocalizationResourceString.cs
// One slot of a compact .locres' string lookup table: the string and how many entries still reference it.
// RefCount is -1 below Optimized_CRC32, which is how the reader knows not to decrement it.
#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "../../../IUStruct.h"
#include "ELocResVersion.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Objects::Core::i18N
{
    class FTextLocalizationResourceString : public UE4::IUStruct
    {
    public:
        std::string String;
        int32_t RefCount = -1;

        FTextLocalizationResourceString(Readers::FArchive& Ar, ELocResVersion versionNumber);

        FTextLocalizationResourceString(std::string s, int32_t refCount)
            : String(std::move(s)), RefCount(refCount) {}
    };
}
