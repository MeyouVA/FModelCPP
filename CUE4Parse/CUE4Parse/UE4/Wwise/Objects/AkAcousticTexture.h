// Ported from CUE4Parse/UE4/Wwise/Objects/AkAcousticTexture.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "ICAkIndexable.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    struct AkAcousticTexture : ICAkIndexable
    {
        uint32_t Id = 0;
        float AbsorptionOffset = 0;
        float AbsorptionLow = 0;
        float AbsorptionMidLow = 0;
        float AbsorptionMidHigh = 0;
        float AbsorptionHigh = 0;
        float Scattering = 0;

        AkAcousticTexture() = default;

        explicit AkAcousticTexture(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();
            AbsorptionOffset = Ar.Read<float>();
            AbsorptionLow = Ar.Read<float>();
            AbsorptionMidLow = Ar.Read<float>();
            AbsorptionMidHigh = Ar.Read<float>();
            AbsorptionHigh = Ar.Read<float>();
            Scattering = Ar.Read<float>();
        }
    };

    // > 118 <= 122
    struct AkAcousticTexture_v122 : ICAkIndexable
    {
        uint32_t Id = 0;

        // Note these three are ushort on the wire even though they model a bool.
        bool OnOffBand1 = false;
        bool OnOffBand2 = false;
        bool OnOffBand3 = false;

        uint16_t FilterTypeBand1 = 0;
        uint16_t FilterTypeBand2 = 0;
        uint16_t FilterTypeBand3 = 0;

        float FrequencyBand1 = 0;
        float FrequencyBand2 = 0;
        float FrequencyBand3 = 0;

        float QFactorBand1 = 0;
        float QFactorBand2 = 0;
        float QFactorBand3 = 0;

        float GainBand1 = 0;
        float GainBand2 = 0;
        float GainBand3 = 0;

        float OutputGain = 0;

        AkAcousticTexture_v122() = default;

        explicit AkAcousticTexture_v122(FWwiseArchive& Ar)
        {
            Id = Ar.Read<uint32_t>();

            OnOffBand1 = Ar.Read<uint16_t>() != 0;
            OnOffBand2 = Ar.Read<uint16_t>() != 0;
            OnOffBand3 = Ar.Read<uint16_t>() != 0;

            FilterTypeBand1 = Ar.Read<uint16_t>();
            FilterTypeBand2 = Ar.Read<uint16_t>();
            FilterTypeBand3 = Ar.Read<uint16_t>();

            FrequencyBand1 = Ar.Read<float>();
            FrequencyBand2 = Ar.Read<float>();
            FrequencyBand3 = Ar.Read<float>();

            QFactorBand1 = Ar.Read<float>();
            QFactorBand2 = Ar.Read<float>();
            QFactorBand3 = Ar.Read<float>();

            GainBand1 = Ar.Read<float>();
            GainBand2 = Ar.Read<float>();
            GainBand3 = Ar.Read<float>();

            OutputGain = Ar.Read<float>();
        }
    };
}
