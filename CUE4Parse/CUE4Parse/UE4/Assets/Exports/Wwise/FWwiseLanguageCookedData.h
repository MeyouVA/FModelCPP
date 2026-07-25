// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FWwiseLanguageCookedData.cs
// Which localisation a cooked Wwise asset belongs to. Doubles as the key of every *LanguageMap below.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../PropertyUtil.h"
#include "../../Objects/FStructFallback.h"
#include "../../Readers/FAssetArchive.h"
#include "EWwiseLanguageRequirement.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FStructFallback;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FName;

    struct FWwiseLanguageCookedData
    {
        int32_t LanguageId = 0;
        FName LanguageName;
        EWwiseLanguageRequirement LanguageRequirement = static_cast<EWwiseLanguageRequirement>(0);

        FWwiseLanguageCookedData() = default;

        explicit FWwiseLanguageCookedData(const FStructFallback& fallback)
        {
            LanguageId = PropertyUtil::GetOrDefault<int32_t>(fallback, "LanguageId");
            LanguageName = PropertyUtil::GetOrDefault<FName>(fallback, "LanguageName");
            LanguageRequirement = PropertyUtil::GetOrDefault<EWwiseLanguageRequirement>(fallback, "LanguageRequirement");
        }

        // The *LanguageMap types key on this, so it needs an ordering. C# relies on the record's
        // structural equality + hash; an ordered map needs a strict weak order instead.
        bool operator<(const FWwiseLanguageCookedData& other) const
        {
            if (LanguageId != other.LanguageId) return LanguageId < other.LanguageId;
            return LanguageName.Text() < other.LanguageName.Text();
        }
    };
}
