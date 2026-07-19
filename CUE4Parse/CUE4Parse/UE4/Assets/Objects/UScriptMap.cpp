// Ported from CUE4Parse/UE4/Assets/Objects/UScriptMap.cs (entry reading).
#include "UScriptMap.h"

#include "FPropertyTagData.h"
#include "../Readers/FAssetArchive.h"
#include "../../Exceptions/ParserException.h"
#include "Properties/StrProperty.h"

namespace CUE4Parse::UE4::Assets::Objects
{
    using Readers::FAssetArchive;
    using Properties::FPropertyTagType;
    using Properties::ReadType;

    UScriptMap::UScriptMap(FAssetArchive& Ar, const FPropertyTagData* tagData, ReadType readType)
    {
        // Pre-PROPERTY_TAG_SET_MAP_SUPPORT game-specific key/value type inference is deferred.
        if (tagData == nullptr || !tagData->InnerType.has_value() || !tagData->ValueType.has_value())
            throw Exceptions::ParserException(Ar, "Can't serialize UScriptMap without key or value type");

        // MapStructTypes table (InnerTypeData/ValueTypeData) is deferred; struct keys/values read via FStructFallback.
        if (readType != ReadType::RAW)
        {
            const int numKeysToRemove = Ar.Read<int32_t>();
            for (int i = 0; i < numKeysToRemove; i++)
                FPropertyTagType::ReadPropertyTagType(Ar, *tagData->InnerType, nullptr, ReadType::MAP);
        }

        const ReadType type = readType == ReadType::RAW ? ReadType::RAW : ReadType::MAP;
        const int numEntries = Ar.Read<int32_t>();
        Properties.reserve(static_cast<size_t>(numEntries));
        for (int i = 0; i < numEntries; i++)
        {
            auto key = FPropertyTagType::ReadPropertyTagType(Ar, *tagData->InnerType, nullptr, type);
            auto value = FPropertyTagType::ReadPropertyTagType(Ar, *tagData->ValueType, nullptr, type);
            if (!key)
                key = std::make_unique<Properties::StrProperty>("UNK_Entry_" + std::to_string(i));
            Properties.emplace_back(std::move(key), std::move(value));
        }
    }

    std::string UScriptMap::ToString() const
    {
        return "{" + std::to_string(Properties.size()) + " entries}";
    }
}
