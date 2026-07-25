// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseSwitchContainerLeafCookedData.cs
// One leaf of a switch container: the group values that select it, plus everything it needs loaded.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Objects/UScriptMap.h"
#include "../../Objects/Properties/MapProperty.h"
#include "../../Objects/UScriptSet.h"
#include "../../Readers/FAssetArchive.h"
#include "FWwiseMediaCookedData.h"
#include "FWwisePackagedFile.h"
#include "FWwiseSoundBankCookedData.h"
#include "FWwiseExternalSourceCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Objects::UScriptSet;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    struct FWwiseSwitchContainerLeafCookedData
    {
        // C# keeps the raw set (documented there as FWwiseGroupValueCookedData[]); so does the port.
        const UScriptSet* GroupValueSet = nullptr;
        std::vector<FWwiseSoundBankCookedData> SoundBanks;
        std::vector<FWwiseMediaCookedData> Media;
        std::vector<FWwiseExternalSourceCookedData> ExternalSources;
        std::shared_ptr<FWwisePackagedFile> PackagedFile;

        FWwiseSwitchContainerLeafCookedData() = default;

        explicit FWwiseSwitchContainerLeafCookedData(const FStructFallback& fallback)
        {
            PropertyUtil::TryGet<const UScriptSet*>(fallback, "GroupValueSet", GroupValueSet);
            SoundBanks = PropertyUtil::GetStructArray<FWwiseSoundBankCookedData>(fallback, "SoundBanks");
            Media = PropertyUtil::GetStructArray<FWwiseMediaCookedData>(fallback, "Media");
            ExternalSources = PropertyUtil::GetStructArray<FWwiseExternalSourceCookedData>(fallback, "ExternalSources");
            PackagedFile = FWwisePackagedFile::CreatePackagedFile(fallback, "PackagedFile");
        }

        void SerializeBulkData(FAssetArchive& Ar) const
        {
            for (const auto& sb : SoundBanks) sb.SerializeBulkData(Ar);
            for (const auto& media : Media) media.SerializeBulkData(Ar);
            if (PackagedFile != nullptr) PackagedFile->SerializeBulkData(Ar);
        }
    };
}
