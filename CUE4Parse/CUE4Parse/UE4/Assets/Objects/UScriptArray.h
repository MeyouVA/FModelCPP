// Ported from CUE4Parse/UE4/Assets/Objects/UScriptArray.cs
// The value of an ArrayProperty: a homogeneous list of property values (InnerType) plus the inner type
// descriptor used to read each element.
//
// Deliberate differences from C#:
//   * The InnerTypeData paths (unversioned / RAW / pre-UE4 / UE5 complete-type-name) are deferred with the
//     mappings + UE5 type-name layers, so struct-array inner descriptors only come from the classic
//     INNER_ARRAY_TAG_INFO inline tag. The DaysGone struct-array size heuristic is omitted. TODO.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "FPropertyTagData.h"
#include "Properties/FPropertyTagType.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    class UScriptArray
    {
    public:
        std::string InnerType;
        std::unique_ptr<FPropertyTagData> InnerTagData;
        std::vector<std::unique_ptr<Properties::FPropertyTagType>> Properties;

        explicit UScriptArray(std::string innerType);
        UScriptArray(Readers::FAssetArchive& Ar, const FPropertyTagData* tagData, Properties::ReadType type, int size);

        // Move-only (holds unique_ptrs).
        UScriptArray(UScriptArray&&) = default;
        UScriptArray& operator=(UScriptArray&&) = default;
        UScriptArray(const UScriptArray&) = delete;
        UScriptArray& operator=(const UScriptArray&) = delete;

        std::string ToString() const;
    };
}
