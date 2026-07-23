// Ported from CUE4Parse/UE4/Assets/Objects/Properties/EnumProperty.cs
#include "EnumProperty.h"

#include "../FPropertyTagData.h"
#include "../../IPackage.h"
#include "../../../Objects/UObject/UEnum.h"
#include "../../../../MappingsProvider/TypeMappings.h"

namespace CUE4Parse::UE4::Assets::Objects::Properties
{
    namespace
    {
        // C#'s underlyingProp.GenericValue + IsNumericType + Convert.ToInt64, for the numeric TPropertyTagType
        // instantiations the property readers produce.
        bool TryGetNumeric(const FPropertyTagType& prop, long long& out)
        {
            if (const auto* p = dynamic_cast<const TPropertyTagType<uint8_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<int8_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<int16_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<uint16_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<int32_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<uint32_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<int64_t>*>(&prop)) { out = p->Value; return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<uint64_t>*>(&prop)) { out = static_cast<long long>(p->Value); return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<float>*>(&prop)) { out = static_cast<long long>(p->Value); return true; }
            if (const auto* p = dynamic_cast<const TPropertyTagType<double>*>(&prop)) { out = static_cast<long long>(p->Value); return true; }
            return false;
        }
    }

    EnumProperty::EnumProperty(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type)
    {
        if (type == ReadType::ZERO)
        {
            Value = FName(IndexToEnum(Ar, tagData, 0));
        }
        else if ((Ar.HasUnversionedProperties() && type == ReadType::NORMAL) || type == ReadType::RAW)
        {
            // The Ashes of Creation FAoCDBCReader special case is omitted (that reader isn't ported).
            long long index = 0;
            if (tagData != nullptr && tagData->InnerType.has_value())
            {
                const auto underlyingProp = ReadPropertyTagType(
                    Ar, *tagData->InnerType, tagData->InnerTypeData.get(), ReadType::NORMAL);
                long long parsed;
                if (underlyingProp && TryGetNumeric(*underlyingProp, parsed))
                    index = parsed;
            }
            else
            {
                index = Ar.Read<uint8_t>();
            }
            Value = FName(IndexToEnum(Ar, tagData, index));
        }
        else
        {
            Value = Ar.ReadFName();
        }
    }

    std::string EnumProperty::IndexToEnum(FAssetArchive& Ar, const FPropertyTagData* tagData, long long index)
    {
        if (tagData == nullptr || !tagData->EnumName.has_value())
            return std::to_string(index);
        const std::string& enumName = *tagData->EnumName;

        if (tagData->Enum != nullptr) // serialized
        {
            for (const auto& [name, value] : tagData->Enum->Names)
                if (value == index)
                    return name.Text();

            return enumName + "::" + std::to_string(index);
        }

        const auto* mappings = Ar.Owner != nullptr ? Ar.Owner->Mappings() : nullptr;
        if (mappings != nullptr)
        {
            const auto enumIt = mappings->Enums.find(enumName);
            if (enumIt != mappings->Enums.end())
            {
                const auto memberIt = enumIt->second.find(index);
                if (memberIt != enumIt->second.end())
                    return enumName + "::" + memberIt->second;
            }
        }

        return enumName + "::" + std::to_string(index);
    }
}
