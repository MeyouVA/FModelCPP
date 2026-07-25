// Ported from CUE4Parse/UE4/FMod/Objects/FLegacyTriggerBox.cs
#pragma once

#include "FModGuid.h"

namespace CUE4Parse::UE4::FMod::Objects
{
    struct FLegacyTriggerBox
    {
        FModGuid InstrumentGuid;
        float Position = 0.0f;
        float Length = 0.0f;

        FLegacyTriggerBox() = default;
        explicit FLegacyTriggerBox(Readers::FArchive& Ar) : InstrumentGuid(Ar)
        {
            Position = Ar.Read<float>();
            Length = Ar.Read<float>();
        }
    };
}
