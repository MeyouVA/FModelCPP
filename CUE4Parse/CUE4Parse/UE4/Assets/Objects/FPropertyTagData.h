// Ported from CUE4Parse/UE4/Assets/Objects/FPropertyTagData.cs
// The descriptor attached to a property tag: the property type plus any type-specific extra data read from
// the tag (struct type/guid, bool value, enum name, container inner/value types).
//
// Deliberate differences from C#:
//   * The UStruct?/UEnum? back-references (Struct/Enum) and the nested InnerTypeData/ValueTypeData are only
//     produced by the mappings-based and UE5 complete-type-name constructors, which are deferred; those
//     members/ctors are omitted for now. TODO.
//   * The Span<FPropertyTypeNameNode> (UE5 PROPERTY_TAG_COMPLETE_TYPE_NAME) and PropertyType-mapping ctors
//     are deferred; only the classic FAssetArchive ctor and the simple string ctors are ported.
#pragma once

#include <optional>
#include <string>

#include "../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;

    class FPropertyTagData
    {
    public:
        std::optional<std::string> Name;
        std::string Type;
        std::optional<std::string> StructType;
        std::optional<FGuid> StructGuid;
        std::optional<bool> Bool;
        std::optional<std::string> EnumName;
        std::optional<std::string> InnerType;
        std::optional<std::string> ValueType;

        FPropertyTagData() = default;
        // Reads the type-specific tail of a property tag (classic, non-unversioned path).
        FPropertyTagData(Readers::FAssetArchive& Ar, const std::string& type, const std::string& name = "");
        // StructProperty descriptor from a known struct type.
        explicit FPropertyTagData(const std::string& structType, const std::string& name = "")
            : Name(name), Type("StructProperty"), StructType(structType) {}

        std::string ToString() const;
    };
}
