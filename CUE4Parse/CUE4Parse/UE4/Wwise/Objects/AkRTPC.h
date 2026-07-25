// Ported from CUE4Parse/UE4/Wwise/Objects/AkRTPC.cs
// AkSwitchGraphPoint and AkRtpcGraphPoint are declared in the same C# file but live in
// AkConversionTable.h here -- see the layout note at the top of that file.
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkGameSyncType.h"
#include "../Enums/EAkRtpcAccum.h"
#include "AkConversionTable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkGameSyncType;
    using CUE4Parse::UE4::Wwise::Enums::EAkRtpcAccum;

    struct AkRtpc
    {
        uint32_t RtpcId = 0;
        EAkGameSyncType RtpcType = static_cast<EAkGameSyncType>(0);
        EAkRtpcAccum RtpcAccum = static_cast<EAkRtpcAccum>(0);
        // AkRTPC_ParameterID
        uint32_t ParamId = 0;
        uint32_t RtpcCurveId = 0;
        CAkConversionTable ConversionTable;

        AkRtpc() = default;

        explicit AkRtpc(FWwiseArchive& Ar)
        {
            RtpcId = Ar.Read<uint32_t>();

            if (Ar.Version > 89)
            {
                RtpcType = Ar.Read<EAkGameSyncType>();
                RtpcAccum = Ar.Read<EAkRtpcAccum>();
            }

            if (Ar.Version <= 89)       ParamId = Ar.Read<uint32_t>();
            else if (Ar.Version <= 113) ParamId = Ar.Read<uint8_t>();
            else                        ParamId = static_cast<uint32_t>(Ar.Read7BitEncodedIntBE());

            RtpcCurveId = Ar.Read<uint32_t>();
            ConversionTable = CAkConversionTable(Ar);
        }

        // Note the count is a ushort here, not the uint used by the graph-point arrays.
        static std::vector<AkRtpc> ReadArray(FWwiseArchive& Ar)
        {
            const int count = Ar.Read<uint16_t>();
            return Ar.ReadArrayWith(count, [&Ar] { return AkRtpc(Ar); });
        }
    };
}
