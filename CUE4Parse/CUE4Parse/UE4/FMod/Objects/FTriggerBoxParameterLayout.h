// Ported from CUE4Parse/UE4/FMod/Objects/FTriggerBoxParameterLayout.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FTriggerBoxParameterLayout
    {
        FModGuid InstrumentGuid;
        float Start = 0.0f;
        float End = 0.0f;
        bool IncludeEnd = false;

        FTriggerBoxParameterLayout() = default;

        explicit FTriggerBoxParameterLayout(Readers::FArchive& Ar) : InstrumentGuid(Ar)
        {
            Start = Ar.Read<float>();
            End = Ar.Read<float>();
            IncludeEnd = Ar.Read<uint8_t>() != 0;
        }

        FTriggerBoxParameterLayout(const FModGuid& instrument, float start, float end, bool includeEnd)
            : InstrumentGuid(instrument), Start(start), End(end), IncludeEnd(includeEnd) {}
    };
}
