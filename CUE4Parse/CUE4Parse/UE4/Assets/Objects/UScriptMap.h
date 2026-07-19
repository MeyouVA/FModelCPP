// Ported from CUE4Parse/UE4/Assets/Objects/UScriptMap.cs
// The value of a MapProperty: an ordered list of (key, value) property pairs.
//
// Deliberate differences from C#:
//   * C#'s Dictionary<FPropertyTagType, FPropertyTagType?> becomes an ordered vector of unique_ptr pairs —
//     FPropertyTagType has no C++ hash/equality, and the reader only needs insertion order. (A null key falls
//     back to a synthetic StrProperty "UNK_Entry_i", as in C#.)
//   * The pre-PROPERTY_TAG_SET_MAP_SUPPORT game-specific key/value type inference and the MapStructTypes
//     table (InnerTypeData/ValueTypeData) are deferred; struct keys/values still read via FStructFallback.
//     TODO.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Properties/FPropertyTagType.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    class FPropertyTagData;

    class UScriptMap
    {
    public:
        using Entry = std::pair<std::unique_ptr<Properties::FPropertyTagType>, std::unique_ptr<Properties::FPropertyTagType>>;
        std::vector<Entry> Properties;

        UScriptMap() = default;
        UScriptMap(Readers::FAssetArchive& Ar, const FPropertyTagData* tagData, Properties::ReadType readType);

        // Move-only (holds unique_ptrs).
        UScriptMap(UScriptMap&&) = default;
        UScriptMap& operator=(UScriptMap&&) = default;
        UScriptMap(const UScriptMap&) = delete;
        UScriptMap& operator=(const UScriptMap&) = delete;

        std::string ToString() const;
    };
}
