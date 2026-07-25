// Ported from CUE4Parse/UE4/Wwise/Objects/AkSwitchParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EOnSwitchMode.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EOnSwitchMode;

    struct AkSwitchParams
    {
        uint32_t NodeId = 0;
        bool IsFirstOnly = false;
        bool ContinuePlayback = false;
        EOnSwitchMode OnSwitchMode = static_cast<EOnSwitchMode>(0);
        int32_t FadeOutTime = 0;
        int32_t FadeInTime = 0;

        AkSwitchParams() = default;

        explicit AkSwitchParams(FWwiseArchive& Ar)
        {
            NodeId = Ar.Read<uint32_t>();
            if (Ar.Version <= 89)
            {
                IsFirstOnly = Ar.Read<uint8_t>() != 0;
                ContinuePlayback = Ar.Read<uint8_t>() != 0;
                const uint32_t onSwitchModeBitVector = Ar.Read<uint32_t>();
                OnSwitchMode = static_cast<EOnSwitchMode>(onSwitchModeBitVector & 0b00000001);
            }
            else if (Ar.Version <= 150)
            {
                const uint8_t bitVector = Ar.Read<uint8_t>();
                IsFirstOnly = (bitVector & 0b00000001) != 0;
                ContinuePlayback = (bitVector & 0b00000010) != 0;
                const uint8_t onSwitchModeBitVector = Ar.Read<uint8_t>();
                OnSwitchMode = static_cast<EOnSwitchMode>(onSwitchModeBitVector & 0b00000001);
            }
            else
            {
                // Past 150 the switch mode is gone from the wire entirely.
                const uint8_t bitVector = Ar.Read<uint8_t>();
                IsFirstOnly = (bitVector & 0b00000001) != 0;
                ContinuePlayback = (bitVector & 0b00000010) != 0;
            }

            FadeOutTime = Ar.Read<int32_t>();
            FadeInTime = Ar.Read<int32_t>();
        }
    };
}
