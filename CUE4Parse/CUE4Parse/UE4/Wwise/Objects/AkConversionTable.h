// Ported from CUE4Parse/UE4/Wwise/Objects/AkConversionTable.cs
//
// Layout note: AkSwitchGraphPoint and AkRtpcGraphPoint are declared in AkRTPC.cs on the C# side, but they
// live here in the port. CAkConversionTable stores AkRtpcGraphPoint by value and AkRtpc stores a
// CAkConversionTable by value, so at file level the two C# files are mutually dependent -- something C#
// tolerates and C++ headers do not. The graph points are the leaves of that dependency, so they move down
// here and AkRTPC.h includes this file. Namespace and type names are unchanged.
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkCurveInterpolation.h"
#include "../Enums/EAkCurveScaling.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkCurveInterpolation;
    using CUE4Parse::UE4::Wwise::Enums::EAkCurveScaling;

    struct AkSwitchGraphPoint
    {
        float From = 0;
        uint32_t To = 0;
        EAkCurveInterpolation Interp = static_cast<EAkCurveInterpolation>(0);

        AkSwitchGraphPoint() = default;

        explicit AkSwitchGraphPoint(FWwiseArchive& Ar)
        {
            From = Ar.Read<float>();
            To = Ar.Read<uint32_t>();
            Interp = static_cast<EAkCurveInterpolation>(Ar.Read<uint32_t>());
        }
    };

    struct AkRtpcGraphPoint
    {
        float From = 0;
        float To = 0;
        EAkCurveInterpolation Interpolation = static_cast<EAkCurveInterpolation>(0);

        AkRtpcGraphPoint() = default;

        explicit AkRtpcGraphPoint(FWwiseArchive& Ar)
        {
            From = Ar.Read<float>();
            To = Ar.Read<float>();
            Interpolation = static_cast<EAkCurveInterpolation>(Ar.Read<uint32_t>());
        }

        static std::vector<AkRtpcGraphPoint> ReadArray(FWwiseArchive& Ar)
        {
            const int count = static_cast<int>(Ar.Read<uint32_t>());
            return Ar.ReadArrayWith(count, [&Ar] { return AkRtpcGraphPoint(Ar); });
        }
    };

    struct CAkConversionTable
    {
        EAkCurveScaling Scaling = EAkCurveScaling::None;
        int Size = 0; // uint for legacy versions, ushort for modern versions
        std::vector<AkRtpcGraphPoint> GraphPoints;

        CAkConversionTable() = default;

        explicit CAkConversionTable(FWwiseArchive& Ar, bool readScaling = true)
        {
            if (Ar.Version <= 36)
            {
                Scaling = readScaling ? static_cast<EAkCurveScaling>(Ar.Read<uint32_t>()) : EAkCurveScaling::None;
                Size = static_cast<int>(Ar.Read<uint32_t>());
            }
            else
            {
                Scaling = readScaling ? static_cast<EAkCurveScaling>(Ar.Read<uint8_t>()) : EAkCurveScaling::None;
                Size = Ar.Read<uint16_t>();
            }

            GraphPoints = Ar.ReadArrayWith(Size, [&Ar] { return AkRtpcGraphPoint(Ar); });
        }
    };
}
