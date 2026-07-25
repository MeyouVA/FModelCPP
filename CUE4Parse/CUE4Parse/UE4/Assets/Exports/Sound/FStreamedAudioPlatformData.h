// Ported from CUE4Parse/UE4/Assets/Exports/Sound/FStreamedAudioPlatformData.cs
// The cooked, per-platform audio of a sound wave: a format name and the chunk list.
#pragma once

#include <cstdint>
#include <vector>

#include "FStreamedAudioChunk.h"
#include "../../Readers/FAssetArchive.h"
#include "../../../Objects/UObject/FName.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Objects::UObject::FName;

    class FStreamedAudioPlatformData
    {
    public:
        int32_t NumChunks = 0;
        FName AudioFormat;
        std::vector<FStreamedAudioChunk> Chunks;

        explicit FStreamedAudioPlatformData(FAssetArchive& Ar)
        {
            NumChunks = Ar.Read<int32_t>();
            AudioFormat = Ar.ReadFName();
            Chunks.reserve(static_cast<size_t>(NumChunks < 0 ? 0 : NumChunks));
            for (int32_t i = 0; i < NumChunks; i++) Chunks.emplace_back(Ar);
        }
    };
}
