// Ported from CUE4Parse/UE4/Wwise/Objects/AkMeterInfo.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkMeterInfo
    {
        double GridPeriod = 0;
        double GridOffset = 0;
        float Tempo = 0;
        uint8_t TimeSigNumBeatsBar = 0;
        uint8_t TimeSigBeatValue = 0;
        bool MeterInfoFlag = false;

        AkMeterInfo() = default;

        explicit AkMeterInfo(FWwiseArchive& Ar)
        {
            GridPeriod = Ar.Read<double>();
            GridOffset = Ar.Read<double>();
            Tempo = Ar.Read<float>();
            TimeSigNumBeatsBar = Ar.Read<uint8_t>();
            TimeSigBeatValue = Ar.Read<uint8_t>();
            MeterInfoFlag = Ar.ReadBool();
        }
    };
}
