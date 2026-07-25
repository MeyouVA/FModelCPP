// Ported from CUE4Parse/UE4/Wwise/Objects/AkFeedbackInfo.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    class AkFeedbackInfo
    {
    public:
        uint32_t BusId = 0;

        float FeedbackVolume = 0;
        float FeedbackModifierMin = 0;
        float FeedbackModifierMax = 0;
        float FeedbackLPF = 0;
        float FeedbackLPFModMin = 0;
        float FeedbackLPFModMax = 0;

        AkFeedbackInfo() = default;

        explicit AkFeedbackInfo(FWwiseArchive& Ar)
        {
            BusId = Ar.Read<uint32_t>();
            // The payload only exists for old banks *and* a non-zero bus -- both conditions, not either.
            if (Ar.Version <= 56 && BusId != 0)
            {
                FeedbackVolume = Ar.Read<float>();
                FeedbackModifierMin = Ar.Read<float>();
                FeedbackModifierMax = Ar.Read<float>();
                FeedbackLPF = Ar.Read<float>();
                FeedbackLPFModMin = Ar.Read<float>();
                FeedbackLPFModMax = Ar.Read<float>();
            }
        }
    };
}
