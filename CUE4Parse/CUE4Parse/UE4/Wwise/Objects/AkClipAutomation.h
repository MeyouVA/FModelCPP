// Ported from CUE4Parse/UE4/Wwise/Objects/AkClipAutomation.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkClipAutomationType.h"
#include "AkConversionTable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkClipAutomationType;

    struct AkClipAutomation
    {
        uint32_t ClipIndex = 0;
        EAkClipAutomationType AutoType = static_cast<EAkClipAutomationType>(0);
        std::vector<AkRtpcGraphPoint> GraphPoints;

        AkClipAutomation() = default;

        explicit AkClipAutomation(FWwiseArchive& Ar)
        {
            ClipIndex = Ar.Read<uint32_t>();
            AutoType = Ar.Read<EAkClipAutomationType>();
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            GraphPoints = Ar.ReadArrayWith(count, [&Ar] { return AkRtpcGraphPoint(Ar); });
        }
    };
}
