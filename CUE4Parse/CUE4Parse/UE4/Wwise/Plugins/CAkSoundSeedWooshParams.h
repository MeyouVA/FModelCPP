// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkSoundSeedWooshParams.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Objects/AkConversionTable.h"
#include "CAkSoundSeedWindParams.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    using CUE4Parse::UE4::Wwise::Objects::CAkConversionTable;

    enum class EAkNoiseColor : uint16_t
    {
        NOISECOLOR_WHITE = 0,
        NOISECOLOR_PINK = 1,
        NOISECOLOR_RED = 2,
        NOISECOLOR_PURPLE = 3
    };

    struct AkWooshDeflectorParams
    {
        float Frequency = 0;
        float QFactor = 0;
        float Gain = 0;

        AkWooshDeflectorParams() = default;

        explicit AkWooshDeflectorParams(FWwiseArchive& Ar)
        {
            Frequency = Ar.Read<float>();
            QFactor = Ar.Read<float>();
            Gain = DbToLinear(Ar.Read<float>());
        }
    };

    // C# declares a parameterless primary constructor and never reads these -- the array is blitted.
    struct AkWooshPathPoint
    {
        float DistanceTravelled;
        float X;
        float Y;
    };

    struct AkWooshParams
    {
        float Duration = 0;
        float DurationRdm = 0;
        uint32_t ChannelMask = 0;
        float MinDistance = 0;
        float AttenuationRolloff = 0;
        float DynamicRange = 0;
        float PlaybackRate = 0;
        int32_t AnchorIndex = 0;
        EAkNoiseColor NoiseColor = static_cast<EAkNoiseColor>(0);
        float RandomSpeedX = 0;
        float RandomSpeedY = 0;
        uint32_t OversamplingFactor = 0;

        std::vector<FSoundSeedParamvalue> Values;
        uint8_t bEnableDistanceBasedAttenuation = 0;

        AkWooshParams() = default;

        // The read order differs from the field order: the oversampling factor and anchor index are
        // narrower on the wire (ushort/short) than the fields that hold them.
        explicit AkWooshParams(FWwiseArchive& Ar)
        {
            Duration = Ar.Read<float>();
            DurationRdm = Ar.Read<float>();
            const uint16_t value = Ar.Read<uint16_t>();
            if (value == 0)      ChannelMask = 4;
            else if (value == 2) ChannelMask = 0x603;
            else                 ChannelMask = value;
            MinDistance = Ar.Read<float>();
            AttenuationRolloff = Ar.Read<float>();
            DynamicRange = Ar.Read<float>();
            PlaybackRate = Ar.Read<float>();
            NoiseColor = Ar.Read<EAkNoiseColor>();
            RandomSpeedX = Ar.Read<float>();
            RandomSpeedY = Ar.Read<float>();
            bEnableDistanceBasedAttenuation = Ar.Read<uint8_t>();
            OversamplingFactor = Ar.Read<uint16_t>();
            Values = Ar.ReadArrayWith(4, [&Ar] { return FSoundSeedParamvalue(Ar); });
            AnchorIndex = Ar.Read<int16_t>();
        }
    };

    class CAkSoundSeedWooshParams : public IAkPluginParam
    {
    public:
        AkWooshParams WooshParams;
        std::vector<AkWooshDeflectorParams> Deflectors;
        std::vector<CAkConversionTable> Curves;
        float TotalPathDistance = 0;
        std::vector<AkWooshPathPoint> Path;

        explicit CAkSoundSeedWooshParams(FWwiseArchive& Ar)
        {
            WooshParams = AkWooshParams(Ar);
            const int deflectorCount = Ar.Read<uint16_t>();
            Deflectors = Ar.ReadArrayWith(deflectorCount, [&Ar] { return AkWooshDeflectorParams(Ar); });
            const int curveCount = Ar.Read<uint16_t>();
            Curves = Ar.ReadArrayWith(curveCount, [&Ar] {
                Ar.Read<int32_t>();
                return CAkConversionTable(Ar, false);
            });
            // The point count comes before the total distance, not immediately before the points.
            const uint16_t pointsNum = Ar.Read<uint16_t>();
            TotalPathDistance = Ar.Read<float>();
            Path = Ar.ReadArray<AkWooshPathPoint>(pointsNum);
        }
    };
}
