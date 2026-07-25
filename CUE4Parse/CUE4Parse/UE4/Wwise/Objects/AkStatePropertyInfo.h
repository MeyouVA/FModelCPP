// Ported from CUE4Parse/UE4/Wwise/Objects/AkStatePropertyInfo.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkRtpcAccum.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkRtpcAccum;

    struct AkStatePropertyInfo
    {
        int PropertyId = 0;
        EAkRtpcAccum AccumType = static_cast<EAkRtpcAccum>(0);
        uint8_t InDb = 0;

        AkStatePropertyInfo() = default;

        explicit AkStatePropertyInfo(FWwiseArchive& Ar)
        {
            PropertyId = Ar.Read7BitEncodedIntBE();
            AccumType = Ar.Read<EAkRtpcAccum>();
            if (Ar.Version > 126)
            {
                InDb = Ar.Read<uint8_t>();
            }
        }
    };
}
