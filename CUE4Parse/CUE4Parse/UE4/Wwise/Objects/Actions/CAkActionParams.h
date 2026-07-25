// Ported from CUE4Parse/UE4/Wwise/Objects/Actions/CAkActionParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../../Enums/EAkCurveInterpolation.h"

namespace CUE4Parse::UE4::Wwise::Objects::Actions
{
    using CUE4Parse::UE4::Wwise::Enums::EAkCurveInterpolation;

    class CAkActionParams
    {
    public:
        int32_t TTime = 0;
        int32_t TTimeMin = 0;
        int32_t TTimeMax = 0;
        EAkCurveInterpolation FadeCurve = static_cast<EAkCurveInterpolation>(0);

        CAkActionParams() = default;

        explicit CAkActionParams(FWwiseArchive& Ar)
        {
            if (Ar.Version <= 56)
            {
                TTime = Ar.Read<int32_t>();
                TTimeMin = Ar.Read<int32_t>();
                TTimeMax = Ar.Read<int32_t>();
            }

            // The curve is the low five bits of a bit vector; the rest of the byte is unused here.
            const uint8_t byBitVector = Ar.Read<uint8_t>();
            FadeCurve = static_cast<EAkCurveInterpolation>(byBitVector & 0x1F);
        }
    };
}
