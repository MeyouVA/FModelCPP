// Ported from CUE4Parse/UE4/Assets/Objects/Properties/EnumProperty.cs (reduced resolution — see header).
#include "EnumProperty.h"

#include "../FPropertyTagData.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    EnumProperty::EnumProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
    {
        if (type == ReadType::ZERO)
        {
            Value = FName(IndexToEnum(Ar, tagData, 0));
        }
        else if ((Ar.HasUnversionedProperties() && type == ReadType::NORMAL) || type == ReadType::RAW)
        {
            // C# reads the underlying numeric property via tagData.InnerTypeData; that path is deferred, so we
            // fall back to a single byte (the C# `else` arm) regardless of InnerType.
            const long long index = Ar.Read<uint8_t>();
            Value = FName(IndexToEnum(Ar, tagData, index));
        }
        else
        {
            Value = Ar.ReadFName();
        }
    }

    std::string EnumProperty::IndexToEnum(FAssetArchive& /*Ar*/, const FPropertyTagData* tagData, long long index)
    {
        if (tagData == nullptr || !tagData->EnumName.has_value())
            return std::to_string(index);

        // Serialized UEnum and mappings-based resolution are deferred; emit "EnumName::index".
        return *tagData->EnumName + "::" + std::to_string(index);
    }
}
