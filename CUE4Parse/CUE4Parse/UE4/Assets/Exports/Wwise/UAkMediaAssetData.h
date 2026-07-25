// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/UAkMediaAssetData.cs
// The legacy (pre-WwisePackagedFile) media payload: a count-prefixed array of prefetch/stream chunks.
#pragma once

#include <vector>

#include "../PropertyUtil.h"
#include "../../Readers/FAssetArchive.h"
#include "../UObject.h"
#include "FAkMediaDataChunk.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class UAkMediaAssetData : public UObject
    {
    public:
        bool IsStreamed = false;
        bool UseDeviceMemory = false;
        std::vector<FAkMediaDataChunk> DataChunks;

        void Deserialize(FAssetArchive& Ar, int64_t validPos) override
        {
            UObject::Deserialize(Ar, validPos);

            // UObject Properties
            IsStreamed = PropertyUtil::GetOrDefault<bool>(*this, "IsStreamed");
            UseDeviceMemory = PropertyUtil::GetOrDefault<bool>(*this, "UseDeviceMemory");
            DataChunks = Ar.ReadArrayWith([&Ar] { return FAkMediaDataChunk(Ar); });
        }
    };
}
