// Ported from CUE4Parse/UE4/Assets/Exports/Sound/FStreamedAudioChunk.cs
// One streamed chunk of a cooked sound wave: a bulk-data blob plus its decoded sizes.
#pragma once

#include <cstdint>

#include "../../Objects/FByteBulkData.h"
#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Assets::Objects::FByteBulkData;
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    // [Flags]
    enum class EStreamedAudioChunk : uint32_t
    {
        IsCooked      = 1 << 0,
        HasSeekOffset = 1 << 1,
        IsInlined     = 1 << 2,
    };

    inline bool HasFlag(EStreamedAudioChunk value, EStreamedAudioChunk flag)
    {
        return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) == static_cast<uint32_t>(flag);
    }

    class FStreamedAudioChunk
    {
    public:
        int32_t DataSize = 0;
        int32_t AudioDataSize = 0;
        uint32_t SeekOffsetInAudioFrames = 0;
        FByteBulkData BulkData;

        explicit FStreamedAudioChunk(FAssetArchive& Ar)
        {
            const auto flags = Ar.Read<EStreamedAudioChunk>();

            BulkData = FByteBulkData(Ar);
            DataSize = Ar.Read<int32_t>();
            AudioDataSize = Ar.Read<int32_t>();

            if (HasFlag(flags, EStreamedAudioChunk::HasSeekOffset))
            {
                SeekOffsetInAudioFrames = Ar.Read<uint32_t>();
            }
        }
    };
}
