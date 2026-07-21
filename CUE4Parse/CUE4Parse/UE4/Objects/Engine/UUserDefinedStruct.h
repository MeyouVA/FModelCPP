// Ported from CUE4Parse/UE4/Objects/Engine/UUserDefinedStruct.cs
// A Blueprint-authored struct = a UStruct plus a compile Status, StructFlags, and an optional default instance
// (a tagged property list). Registered under "UserDefinedStruct".
//
// Deliberate differences from C#:
//   * Status is read via the tagged "Status" property (C# uses GetOrDefault; this port has no reflection
//     accessor, so it scans the base Properties list — matching UDataTable's RowStruct pattern).
//   * The FFrameworkObjectVersion >= UserDefinedStructsStoreDefaultInstance gate for the default instance is an
//     un-ported custom version; the modern outcome is assumed (the block is present). TODO.
//   * The unversioned-property branch (DeserializePropertiesUnversioned) is not reachable — UObject::Deserialize
//     already throws on unversioned assets — so only the tagged default-instance path is ported.
#pragma once

#include <cstdint>
#include <vector>

#include "../UObject/UStruct.h"
#include "../../Assets/Objects/FPropertyTag.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::Engine
{
    // Mirrors C#'s EUserDefinedStructureStatus (compile state of the generated struct).
    enum class EUserDefinedStructureStatus : uint8_t
    {
        UDSS_UpToDate,  // up to date (default)
        UDSS_Dirty,     // modified but not recompiled
        UDSS_Error,     // failed to compile
        UDSS_Duplicate  // a duplicate; the original was changed
    };

    class UUserDefinedStruct : public CUE4Parse::UE4::Objects::UObject::UStruct
    {
    public:
        EUserDefinedStructureStatus Status = EUserDefinedStructureStatus::UDSS_UpToDate;
        uint32_t StructFlags = 0;
        std::vector<CUE4Parse::UE4::Assets::Objects::FPropertyTag> DefaultProperties; // the CDO's tagged values

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
