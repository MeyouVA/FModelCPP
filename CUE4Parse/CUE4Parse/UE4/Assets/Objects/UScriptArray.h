// Ported from CUE4Parse/UE4/Assets/Objects/UScriptArray.cs
// The value of an ArrayProperty: a homogeneous list of property values (InnerType) plus the inner type
// descriptor used to read each element.
//
// Deliberate differences from C#:
//   * InnerTagData is a shared_ptr: on the unversioned/RAW path it shares the tag's mappings-built
//     InnerTypeData (as C# shares the reference); on the classic INNER_ARRAY_TAG_INFO path it takes
//     ownership of the inline tag's data. The UE5 complete-type-name path is still deferred, and the
//     DaysGone struct-array size heuristic is omitted. TODO.
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
        std::shared_ptr<FPropertyTagData> InnerTagData;
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
