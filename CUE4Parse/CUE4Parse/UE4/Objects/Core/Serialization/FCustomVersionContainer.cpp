// Ported from CUE4Parse/UE4/Objects/Core/Serialization/FCustomVersionContainer.cs
#include "FCustomVersionContainer.h"

#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Serialization
{
    FCustomVersionContainer::FCustomVersionContainer(Readers::FArchive& Ar, ECustomVersionSerializationFormat format)
    {
        switch (format)
        {
            case ECustomVersionSerializationFormat::Enums:
            {
                auto oldTags = Ar.ReadArrayCounted<FEnumCustomVersion_DEPRECATED>();
                Versions.reserve(oldTags.size());
                for (const auto& t : oldTags)
                    Versions.push_back(t.ToCustomVersion());
                break;
            }
            case ECustomVersionSerializationFormat::Guids:
            {
                auto versionArray = Ar.ReadArrayWith([&Ar]() { return FGuidCustomVersion_DEPRECATED(Ar); });
                Versions.reserve(versionArray.size());
                for (const auto& v : versionArray)
                    Versions.push_back(v.ToCustomVersion());
                break;
            }
            case ECustomVersionSerializationFormat::Optimized:
            {
                Versions = Ar.ReadArrayCounted<FCustomVersion>();
                break;
            }
            default:
                break;
        }
    }

    ECustomVersionSerializationFormat FCustomVersionContainer::DetermineSerializationFormat(int32_t legacyVersion)
    {
        if (legacyVersion == -2) return ECustomVersionSerializationFormat::Enums;
        if (legacyVersion < -2 && legacyVersion >= -5) return ECustomVersionSerializationFormat::Guids;
        if (legacyVersion < -5) return ECustomVersionSerializationFormat::Optimized;
        return ECustomVersionSerializationFormat::Unknown;
    }
}
