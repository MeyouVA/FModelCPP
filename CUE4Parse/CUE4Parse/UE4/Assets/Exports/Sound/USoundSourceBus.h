// Ported from CUE4Parse/UE4/Assets/Exports/Sound/USoundSourceBus.cs
// A bus has no wave payload at all: both serialization hooks are emptied out.
#pragma once

#include "USoundWave.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    class USoundSourceBus : public USoundWave
    {
    protected:
        void SerializeCuePoints(Readers::FAssetArchive& Ar) override {}
        void SerializeCookedPlatformData(Readers::FAssetArchive& Ar) override {}
    };
}
