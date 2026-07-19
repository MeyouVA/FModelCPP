// Ported from CUE4Parse/UE4/Objects/Core/Serialization/FCustomVersion.cs
#pragma once

#include <cstdint>
#include <string>

#include "../Misc/FGuid.h"
#include "../../../Readers/FArchive.h"

namespace CUE4Parse::UE4::Objects::Core::Serialization
{
    using Misc::FGuid;

    // Structure to hold a unique custom key with its version. Blittable POD (Key is 16 bytes, then int).
    struct FCustomVersion
    {
        FGuid Key;       // Unique custom key.
        int32_t Version = 0; // Custom version.

        FCustomVersion() = default;
        FCustomVersion(FGuid key, int32_t version) : Key(key), Version(version) {}

        bool operator==(const FCustomVersion& o) const { return Key == o.Key && Version == o.Version; }
        bool operator!=(const FCustomVersion& o) const { return !(*this == o); }

        std::string ToString() const { return "Key: " + Key.ToString() + ", Version: " + std::to_string(Version); }
    };

    // Deprecated enum-tagged custom version (8-byte POD).
    struct FEnumCustomVersion_DEPRECATED
    {
        uint32_t Tag = 0;
        int32_t Version = 0;

        FCustomVersion ToCustomVersion() const { return FCustomVersion(FGuid(0, 0, 0, Tag), Version); }
    };

    // Deprecated guid-tagged custom version (variable size: FGuid + int + FString).
    struct FGuidCustomVersion_DEPRECATED
    {
        FGuid Tag;
        int32_t Version = 0;
        std::string FriendlyName;

        explicit FGuidCustomVersion_DEPRECATED(Readers::FArchive& Ar)
        {
            Tag = Ar.Read<FGuid>();
            Version = Ar.Read<int32_t>();
            FriendlyName = Ar.ReadFString();
        }

        FCustomVersion ToCustomVersion() const { return FCustomVersion(Tag, Version); }
    };

    enum class ECustomVersionSerializationFormat : uint8_t
    {
        Unknown,
        Guids,
        Enums,
        Optimized,

        // Add new versions above this comment
        CustomVersion_Automatic_Plus_One,
        Latest = CustomVersion_Automatic_Plus_One - 1
    };
}
