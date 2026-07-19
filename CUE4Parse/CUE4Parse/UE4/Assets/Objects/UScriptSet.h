// Ported from CUE4Parse/UE4/Assets/Objects/UScriptSet.cs
// The value of a SetProperty: a list of element property values (InnerType).
//
// Deliberate differences from C#:
//   * The many game-specific InnerType / InnerTypeData inferences (StateOfDecay2, ThroneAndLiberty, Avowed, ...)
//     are deferred; a set relies on the tag's InnerType. Struct sets read elements via FStructFallback. TODO.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Properties/FPropertyTagType.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    class FPropertyTagData;

    class UScriptSet
    {
    public:
        std::vector<std::unique_ptr<Properties::FPropertyTagType>> Properties;

        UScriptSet() = default;
        UScriptSet(Readers::FAssetArchive& Ar, const FPropertyTagData* tagData, Properties::ReadType readType);

        // Move-only (holds unique_ptrs).
        UScriptSet(UScriptSet&&) = default;
        UScriptSet& operator=(UScriptSet&&) = default;
        UScriptSet(const UScriptSet&) = delete;
        UScriptSet& operator=(const UScriptSet&) = delete;

        std::string ToString() const;
    };
}
