// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseInitBankCookedData.cs
// The Init bank: a soundbank that additionally lists every other bank, media and language in the project.
#pragma once

#include <vector>

#include "FWwiseLanguageCookedData.h"
#include "FWwiseMediaCookedData.h"
#include "FWwiseSoundBankCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    class FWwiseInitBankCookedData : public FWwiseSoundBankCookedData
    {
    public:
        std::vector<FWwiseSoundBankCookedData> SoundBanks;
        std::vector<FWwiseMediaCookedData> Media;
        std::vector<FWwiseLanguageCookedData> Language;

        FWwiseInitBankCookedData() = default;

        explicit FWwiseInitBankCookedData(const FStructFallback& fallback) : FWwiseSoundBankCookedData(fallback)
        {
            SoundBanks = PropertyUtil::GetStructArray<FWwiseSoundBankCookedData>(fallback, "SoundBanks");
            Media = PropertyUtil::GetStructArray<FWwiseMediaCookedData>(fallback, "Media");
            Language = PropertyUtil::GetStructArray<FWwiseLanguageCookedData>(fallback, "Language");
        }

        void SerializeBulkData(FAssetArchive& Ar) const override
        {
            FWwiseSoundBankCookedData::SerializeBulkData(Ar);
            for (const auto& sb : SoundBanks) sb.SerializeBulkData(Ar);
            for (const auto& media : Media) media.SerializeBulkData(Ar);
            // C# does NOT walk Language here; neither does the port.
        }
    };
}
