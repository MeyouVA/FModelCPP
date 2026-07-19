// Ported from CUE4Parse/UE4/Assets/Objects/UScriptArray.cs (element reading).
#include "UScriptArray.h"

#include "FPropertyTag.h"
#include "../Readers/FAssetArchive.h"
#include "../../Versions/ObjectVersion.h"
#include "../../Versions/EGame.h"
#include "../../Exceptions/ParserException.h"
#include "Properties/ByteProperty.h"
#include "Properties/EnumProperty.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using namespace CUE4Parse::UE4::Versions;
    using Properties::FPropertyTagType;
    using Properties::ReadType;

    UScriptArray::UScriptArray(std::string innerType)
        : InnerType(std::move(innerType)) {}

    UScriptArray::UScriptArray(Readers::FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType type, int size)
    {
        if (tagData == nullptr || !tagData->InnerType.has_value())
            throw Exceptions::ParserException(Ar, "UScriptArray needs inner type");
        InnerType = *tagData->InnerType;

        const int elementCount = Ar.Read<int32_t>();
        if (elementCount > Ar.Length - Ar.Position)
            throw Exceptions::ParserException(Ar,
                "ArrayProperty element count " + std::to_string(elementCount) +
                " is larger than the remaining archive size " + std::to_string(Ar.Length - Ar.Position));

        if (Ar.HasUnversionedProperties() || type == ReadType::RAW || Ar.Game() < GAME_UE4_0)
        {
            // C# uses tagData.InnerTypeData here (mappings / raw path) — deferred. TODO.
        }
        else if (Ar.Ver() >= EUnrealEngineObjectUE5Version::PROPERTY_TAG_COMPLETE_TYPE_NAME && InnerType == "StructProperty")
        {
            // UE5 complete-type-name InnerTypeData — deferred (FPropertyTag ctor throws on this version anyway). TODO.
        }
        else if (Ar.Ver() >= EUnrealEngineObjectUE4Version::INNER_ARRAY_TAG_INFO && InnerType == "StructProperty")
        {
            FPropertyTag innerTag(Ar, false);
            InnerTagData = std::move(innerTag.TagData);
            if (!InnerTagData)
                throw Exceptions::ParserException(Ar, "Couldn't read ArrayProperty with inner type StructProperty");
        }
        else
        {
            // DaysGone struct-array element-size heuristic — deferred. TODO.
        }

        if (elementCount == 0)
            return;
        Properties.reserve(static_cast<size_t>(elementCount));

        const ReadType readType = type == ReadType::RAW ? ReadType::RAW : ReadType::ARRAY;

        // ByteProperty is special: an element may be a raw byte or an enum name.
        if (InnerType == "ByteProperty")
        {
            bool enumprop = (InnerTagData && InnerTagData->EnumName.has_value() && *InnerTagData->EnumName != "None")
                || Ar.TestReadFName();
            if (!Ar.HasUnversionedProperties())
                enumprop = (size - static_cast<int>(sizeof(int))) / elementCount > 1;

            for (int i = 0; i < elementCount; i++)
            {
                std::unique_ptr<FPropertyTagType> property;
                if (enumprop)
                    property = std::make_unique<Properties::EnumProperty>(Ar, InnerTagData.get(), readType);
                else
                    property = std::make_unique<Properties::ByteProperty>(Ar, readType);
                if (property)
                    Properties.push_back(std::move(property));
            }
            return;
        }

        for (int i = 0; i < elementCount; i++)
        {
            auto property = FPropertyTagType::ReadPropertyTagType(Ar, InnerType, InnerTagData.get(), readType);
            if (property)
                Properties.push_back(std::move(property));
        }
    }

    std::string UScriptArray::ToString() const
    {
        return InnerType + "[" + std::to_string(Properties.size()) + "]";
    }
}
