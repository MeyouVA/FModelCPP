// Ported from CUE4Parse/UE4/Wwise/Objects/HIRC/Containers/HierarchySettings.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../../../WwiseArchive.h"
#include "../../../Enums/EHierarchyParameterType.h"
#include "../../Setting.h"
#include "../AbstractHierarchy.h"

namespace CUE4Parse::UE4::Wwise::Objects::HIRC::Containers
{
    using CUE4Parse::UE4::Wwise::Enums::EHierarchyParameterType;

    class HierarchySettings : public AbstractHierarchy
    {
    public:
        uint16_t SettingsCount = 0;
        std::vector<Setting<EHierarchyParameterType>> Settings;

        // All the ids come first, then all the values -- not id/value pairs.
        explicit HierarchySettings(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            if (Ar.Version <= 126)
                SettingsCount = Ar.Read<uint8_t>();
            else
                SettingsCount = Ar.Read<uint16_t>();

            auto settingIds = ReadParameterTypes(Ar, SettingsCount);
            auto settingValues = Ar.ReadArray<float>(SettingsCount);
            Settings.reserve(SettingsCount);
            for (int index = 0; index < SettingsCount; index++)
                Settings.emplace_back(settingIds[index], settingValues[index]);
        }

    private:
        // The parameter type widened from byte to ushort at 127, so the old form is zero-extended.
        static std::vector<EHierarchyParameterType> ReadParameterTypes(FWwiseArchive& Ar, int count)
        {
            if (Ar.Version <= 126)
            {
                auto bytes = Ar.ReadArray<uint8_t>(count);
                std::vector<EHierarchyParameterType> result;
                result.reserve(bytes.size());
                for (uint8_t b : bytes)
                    result.push_back(static_cast<EHierarchyParameterType>(static_cast<uint16_t>(b)));
                return result;
            }

            return Ar.ReadArray<EHierarchyParameterType>(count);
        }
    };
}
