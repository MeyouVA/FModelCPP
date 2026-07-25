// Ported from CUE4Parse/UE4/Assets/Exports/Sound/USoundWaveProcedural.cs
// Generates its audio at runtime, so it skips USoundWave's payload entirely and runs only the UObject body.
#pragma once

#include "USoundWave.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    class USoundWaveProcedural : public USoundWave
    {
    public:
        void Deserialize(Readers::FAssetArchive& Ar, int64_t validPos) override
        {
            SoundBaseDeserialize(Ar, validPos);
        }

    protected:
        void SerializeCuePoints(Readers::FAssetArchive& Ar) override {}
    };
}
