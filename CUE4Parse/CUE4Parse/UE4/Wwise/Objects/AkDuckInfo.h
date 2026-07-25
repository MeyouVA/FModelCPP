// Ported from CUE4Parse/UE4/Wwise/Objects/AkDuckInfo.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkCurveInterpolation.h"
#include "../Enums/EAkPropID.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkCurveInterpolation;
    using CUE4Parse::UE4::Wwise::Enums::EAkPropID;

    struct AkDuckInfo
    {
        uint32_t BusId = 0;
        float DuckVolume = 0;
        uint32_t FadeOutTime = 0;
        uint32_t FadeInTime = 0;
        EAkCurveInterpolation FadeCurve = static_cast<EAkCurveInterpolation>(0);
        EAkPropID TargetProp = static_cast<EAkPropID>(0);

        AkDuckInfo() = default;

        explicit AkDuckInfo(FWwiseArchive& Ar)
        {
            BusId = Ar.Read<uint32_t>();
            DuckVolume = Ar.Read<float>();
            FadeOutTime = Ar.Read<uint32_t>();
            FadeInTime = Ar.Read<uint32_t>();

            const uint8_t byBitVector = Ar.Read<uint8_t>();
            FadeCurve = static_cast<EAkCurveInterpolation>(byBitVector & 0x1F);
            if (Ar.Version > 65)
            {
                TargetProp = Ar.Read<EAkPropID>();
            }
        }
    };
}
