// Ported from CUE4Parse/UE4/Objects/Core/Serialization/FCustomVersionContainer.cs
#pragma once

#include <vector>

#include "FCustomVersion.h"
#include "../Misc/FGuid.h"

namespace CUE4Parse::UE4::Readers { class FArchive; }

namespace CUE4Parse::UE4::Objects::Core::Serialization
{
    class FCustomVersionContainer
    {
    public:
        std::vector<FCustomVersion> Versions;

        FCustomVersionContainer() = default;
        explicit FCustomVersionContainer(std::vector<FCustomVersion> versions) : Versions(std::move(versions)) {}

        // Reads a custom-version table in the given serialization format.
        FCustomVersionContainer(Readers::FArchive& Ar, ECustomVersionSerializationFormat format);

        int32_t GetVersion(const Misc::FGuid& customKey) const
        {
            for (const auto& v : Versions)
                if (v.Key == customKey) return v.Version;
            return -1;
        }

        static ECustomVersionSerializationFormat DetermineSerializationFormat(int32_t legacyVersion);
    };
}
