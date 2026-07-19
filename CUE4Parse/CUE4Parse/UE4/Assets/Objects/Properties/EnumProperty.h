// Ported from CUE4Parse/UE4/Assets/Objects/Properties/EnumProperty.cs
// A ByteProperty/EnumProperty value resolved to its enum member name (an FName).
//
// Deliberate differences from C#:
//   * Serialized-UEnum (tagData.Enum) and mappings-based (Ar.Owner.Mappings) index->name resolution are
//     deferred; IndexToEnum emits "EnumName::index" when an enum name is known. TODO.
//   * The unversioned/RAW underlying-numeric-property path (which reads InnerTypeData via GenericValue) falls
//     back to reading a single byte. The Ashes of Creation FAoCDBCReader special case is omitted. TODO.
#pragma once

#include <string>

#include "FPropertyTagType.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Objects { class FPropertyTagData; }

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    using CUE4Parse::UE4::Objects::UObject::FName;

    class EnumProperty : public TPropertyTagType<FName>
    {
    public:
        explicit EnumProperty(FName value) { Value = std::move(value); }
        EnumProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type);
        const char* TypeName() const override { return "EnumProperty"; }

    private:
        static std::string IndexToEnum(FAssetArchive& Ar, const FPropertyTagData* tagData, long long index);
    };
}
