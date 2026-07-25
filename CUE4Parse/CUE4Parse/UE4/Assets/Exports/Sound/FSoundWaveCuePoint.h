// Ported from CUE4Parse/UE4/Assets/Exports/Sound/FSoundWaveCuePoint.cs
// C# reads only the label out of the on-disk cue point.
#pragma once

#include <string>

#include "../../Readers/FAssetArchive.h"

namespace CUE4Parse::UE4::Assets::Exports::Sound
{
    using CUE4Parse::UE4::Assets::Readers::FAssetArchive;

    class FSoundWaveCuePoint
    {
    public:
        std::string Label;

        explicit FSoundWaveCuePoint(FAssetArchive& Ar) : Label(Ar.ReadFString()) {}
    };
}
