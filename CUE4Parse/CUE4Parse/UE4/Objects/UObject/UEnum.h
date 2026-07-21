// Ported from CUE4Parse/UE4/Objects/UObject/UEnum.cs
// A UEnum export = a UField plus a name->value list, its C++ form and underlying integer type. Registered as "Enum".
//
// Deliberate difference from C#: the legacy layout branches (TIGHTLY_PACKED_ENUMS, FCoreObjectVersion.EnumProperties,
// ENUM_CLASS_SUPPORT) and the FFortniteMainBranchObjectVersion.EnumUnderlyingType gate use un-ported custom
// versions; the modern outcome is assumed — Names are (FName, int64) pairs, CppForm and UnderlyingType are read. TODO.
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "UField.h"
#include "FName.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Objects::UObject
{
    class UEnum : public UField
    {
    public:
        enum class ECppForm : uint8_t { Regular, Namespaced, EnumClass };
        enum class EUnderlyingType : uint8_t { int8, int16, int32, int64, uint8, uint16, uint32, uint64 };

        std::vector<std::pair<FName, int64_t>> Names; // all enum names and their values
        ECppForm CppForm = ECppForm::Regular;
        EUnderlyingType UnderlyingType = EUnderlyingType::int64;

        void Deserialize(Assets::Readers::FAssetArchive& Ar, int64_t validPos) override;
    };
}
