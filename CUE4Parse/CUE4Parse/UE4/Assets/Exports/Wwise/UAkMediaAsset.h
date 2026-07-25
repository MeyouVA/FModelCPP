// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkMediaAsset.cs
// A media asset: id and name come out of the property bag, then a package index to the platform-specific
// data object follows on the wire.
#pragma once

#include <cstdint>
#include <string>

#include "../PropertyUtil.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/ObjectResource.h"
#include "UAkAudioType.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;
    using CUE4Parse::UE4::Objects::UObject::FPackageIndex;

    class UAkMediaAsset : public UObject
    {
    public:
        uint32_t ID = 0;
        std::string MediaName;
        FPackageIndex CurrentMediaAssetData;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            // C# passes StringComparison.OrdinalIgnoreCase for ID only -- the property is spelled "Id" in
            // some cooked assets and "ID" in others.
            ID = PropertyUtil::GetOrDefault<uint32_t>(*this, "ID", 0, /*ignoreCase*/ true);
            MediaName = PropertyUtil::GetOrDefault<std::string>(*this, "MediaName");
            CurrentMediaAssetData = FPackageIndex(Ar);
        }
    };
}
