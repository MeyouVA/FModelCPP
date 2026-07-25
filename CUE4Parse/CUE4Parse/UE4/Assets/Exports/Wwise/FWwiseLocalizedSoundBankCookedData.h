// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseLocalizedSoundBankCookedData.cs
// The per-language variants of one soundbank: the same asset cooked once per localisation.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "FWwiseSoundBankCookedData.h"
#include "FWwiseLocalizedCookedDataMap.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseLocalizedSoundBankCookedData
    {
        std::map<FWwiseLanguageCookedData, std::optional<FWwiseSoundBankCookedData>> SoundBankLanguageMap;
        FName DebugName;
        uint32_t SoundBankId = 0;
        std::vector<FName> IncludedEventNames;

        FWwiseLocalizedSoundBankCookedData() = default;

        explicit FWwiseLocalizedSoundBankCookedData(const FStructFallback& fallback)
        {
            SoundBankLanguageMap = ReadLanguageMap<FWwiseSoundBankCookedData>(fallback, "SoundBankLanguageMap");
            DebugName = PropertyUtil::GetOrDefault<FName>(fallback, "DebugName");
            SoundBankId = static_cast<uint32_t>(PropertyUtil::GetOrDefault<int32_t>(fallback, "SoundBankId"));
            // C# reads List<FName> here; the array arm walks the UScriptArray elements directly
            // because FName is a scalar payload, not a struct fallback.
            if (const UScriptArray* names = nullptr;
                PropertyUtil::TryGet<const UScriptArray*>(fallback, "IncludedEventNames", names) && names != nullptr)
            {
                for (const auto& element : names->Properties)
                {
                    FName name;
                    if (element != nullptr && PropertyUtil::PropertyValue(*element, name))
                        IncludedEventNames.push_back(name);
                }
            }
        }
    };
}
