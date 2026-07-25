// Ported from CUE4Parse/UE4/Wwise/Objects/AkMusicFade.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkCurveInterpolation.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkCurveInterpolation;

    struct AkMusicFade
    {
        int32_t TransitionTime = 0;
        // Note the curve is a full uint on the wire here even though the enum is byte-backed.
        EAkCurveInterpolation FadeCurve = static_cast<EAkCurveInterpolation>(0);
        int32_t FadeOffset = 0;

        AkMusicFade() = default;

        explicit AkMusicFade(FWwiseArchive& Ar)
        {
            TransitionTime = Ar.Read<int32_t>();
            FadeCurve = static_cast<EAkCurveInterpolation>(Ar.Read<uint32_t>());
            FadeOffset = Ar.Read<int32_t>();
        }
    };
}
