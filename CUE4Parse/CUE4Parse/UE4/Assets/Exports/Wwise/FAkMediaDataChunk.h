// Ported from CUE4Parse/UE4/Assets/Exports/Wwise/FAkMediaDataChunk.cs
// One chunk of a legacy UAkMediaAssetData: a four-byte bool saying whether it is the streaming prefetch
// header, then the payload as bulk data.
#pragma once

#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Wwise
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class FAkMediaDataChunk
    {
    public:
        FByteBulkData Data;
        bool IsPrefetch = false;

        explicit FAkMediaDataChunk(FAssetArchive& Ar)
        {
            // ReadBoolean, not a one-byte bool: this field is four bytes wide.
            IsPrefetch = Ar.ReadBoolean();
            Data = FByteBulkData(Ar);
        }
    };
}
