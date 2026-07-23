// Ported from CUE4Parse/UE4/Assets/Objects/FPropertyTagData.cs
// The descriptor attached to a property tag: the property type plus any type-specific extra data read from
// the tag (struct type/guid, bool value, enum name, container inner/value types).
//
// Deliberate differences from C#:
//   * The Span<FPropertyTypeNameNode> ctor (UE5 PROPERTY_TAG_COMPLETE_TYPE_NAME) is deferred; the classic
//     FAssetArchive ctor, the simple string ctors and the mappings (PropertyType) ctor are ported.
//   * The nested InnerTypeData/ValueTypeData are shared_ptrs (C# shares references under GC; a cloned
//     PropertyInfo shares its MappingType, so the descriptors built from it are shared too).
//   * The UStruct/UEnum back-references are non-owning pointers into provider-owned packages.
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::MappingsProvider { class PropertyType; }
namespace CUE4Parse::UE4::Objects::UObject { class UStruct; class UEnum; }
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
        std::shared_ptr<FPropertyTagData> InnerTypeData;
        std::shared_ptr<FPropertyTagData> ValueTypeData;
        const UE4::Objects::UObject::UStruct* Struct = nullptr;
        const UE4::Objects::UObject::UEnum* Enum = nullptr;

        FPropertyTagData() = default;
        // Reads the type-specific tail of a property tag (classic, non-unversioned path).
        FPropertyTagData(Readers::FAssetArchive& Ar, const std::string& type, const std::string& name = "");
        // StructProperty descriptor from a known struct type.
        explicit FPropertyTagData(const std::string& structType, const std::string& name = "")
            : Name(name), Type("StructProperty"), StructType(structType) {}
        // Descriptor derived from a mappings PropertyType (unversioned path). Defined in the .cpp.
        explicit FPropertyTagData(const MappingsProvider::PropertyType& info);

        std::string ToString() const;
    };
}
