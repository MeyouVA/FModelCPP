// Ported from CUE4Parse/UE4/Assets/Objects/FPropertyTag.cs
// A single tagged property: its name, property type, on-disk size, array index, type descriptor and the
// parsed value (Tag).
//
// Deliberate differences from C#:
//   * Only the classic (pre-UE5 PROPERTY_TAG_COMPLETE_TYPE_NAME) tag layout is read; the UE5 complete-type-
//     name path (FPropertyTypeNameNode nodes) throws for now. TODO.
//   * The blueprint-decompiler helper is omitted until that layer is ported. The mappings-based ctor
//     FPropertyTag(Ar, PropertyInfo, ReadType) IS ported (unversioned path).
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "FPropertyTagData.h"
#include "Properties/FPropertyTagType.h"
#include "../../Objects/UObject/FName.h"
#include "../../Objects/Core/Misc/FGuid.h"

namespace CUE4Parse::MappingsProvider { class PropertyInfo; }
namespace CUE4Parse::UE4::Assets::Readers { class FAssetArchive; }

namespace CUE4Parse::UE4::Assets::Objects
{
    using CUE4Parse::UE4::Objects::UObject::FName;
    using CUE4Parse::UE4::Objects::Core::Misc::FGuid;
    using Properties::FPropertyTagType;

    enum class EPropertyTagSerializeType : uint8_t
    {
        Unknown,
        Skipped,
        Property,
        BinaryOrNative,
    };

    // [Flags]
    enum EPropertyTagFlags : uint8_t
    {
        PTF_None                     = 0x00,
        PTF_HasArrayIndex            = 0x01,
        PTF_HasPropertyGuid          = 0x02,
        PTF_HasPropertyExtensions    = 0x04,
        PTF_HasBinaryOrNativeSerialize = 0x08,
        PTF_BoolTrue                 = 0x10,
        PTF_SkippedSerialize         = 0x20,
    };

    class FPropertyTag
    {
    public:
        FName Name;
        FName PropertyType;
        int Size = 0;
        int ArrayIndex = 0;
        std::optional<int> ArraySize;
        std::unique_ptr<FPropertyTagData> TagData;
        bool HasPropertyGuid = false;
        std::optional<FGuid> PropertyGuid;
        std::unique_ptr<FPropertyTagType> Tag;
        EPropertyTagFlags PropertyTagFlags = PTF_None;

        FPropertyTag() = default;
        // Reads a tagged property from the stream. If readData is true, also parses the value into Tag.
        FPropertyTag(Readers::FAssetArchive& Ar, bool readData);
        // Builds a tag from a mappings PropertyInfo and reads its value (unversioned path).
        FPropertyTag(Readers::FAssetArchive& Ar, const MappingsProvider::PropertyInfo& info,
                     Properties::ReadType type);

        // Move-only (owns unique_ptrs).
        FPropertyTag(FPropertyTag&&) = default;
        FPropertyTag& operator=(FPropertyTag&&) = default;
        FPropertyTag(const FPropertyTag&) = delete;
        FPropertyTag& operator=(const FPropertyTag&) = delete;

        EPropertyTagSerializeType SerializeType() const
        {
            if (PropertyTagFlags & PTF_SkippedSerialize) return EPropertyTagSerializeType::Skipped;
            if (PropertyTagFlags & PTF_HasBinaryOrNativeSerialize) return EPropertyTagSerializeType::BinaryOrNative;
            return EPropertyTagSerializeType::Property;
        }

        bool IsIndexed() const { return (PropertyTagFlags & PTF_HasArrayIndex) || ArrayIndex > 0; }

        std::string ToString() const
        {
            return Name.Text() + "  -->  " + (Tag != nullptr ? Tag->ToString() : "Failed to parse");
        }
    };
}
