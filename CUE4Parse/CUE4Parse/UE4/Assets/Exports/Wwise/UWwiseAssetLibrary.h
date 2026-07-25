// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UWwiseAssetLibrary.cs
#pragma once

#include <optional>

#include "../../Readers/FAssetArchive.h"
#include "../UObject.h"
#include "FWwiseAssetLibraryCookedData.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UWwiseAssetLibrary : public UObject
    {
    public:
        std::optional<FWwiseAssetLibraryCookedData> CookedData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);
            // C# reads this unconditionally, without the `Ar.Position >= validPos` guard its siblings use.
            CookedData.emplace(Ar);
        }
    };
}
