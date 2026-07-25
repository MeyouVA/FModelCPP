// Ported from CUE4Parse/UE4/Wwise/Plugins/Mindseye/MindseyePluginParams.cs
#pragma once

#include <array>
#include <cstdint>

#include "../../WwiseArchive.h"
#include "../../../Objects/Core/Math/TPair.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::Mindseye
{
    using CUE4Parse::UE4::Objects::Core::Math::TPair;

    // Field names are C#'s: these formats were reverse-engineered and the meanings are not known.
    class AudioDataPassbackFXParams : public IAkPluginParam
    {
    public:
        std::array<bool, 4> unknown1{};

        explicit AudioDataPassbackFXParams(FWwiseArchive& Ar)
        {
            for (auto& b : unknown1) b = Ar.Read<uint8_t>() != 0;
        }
    };

    class BarbDelayFXParams : public IAkPluginParam
    {
    public:
        float unknown1 = 0;
        float unknown2 = 0;
        float unknown3 = 0;
        float unknown4 = 0;
        uint8_t unknown5 = 0;
        uint8_t unknown6 = 0;

        explicit BarbDelayFXParams(FWwiseArchive& Ar)
        {
            unknown1 = Ar.Read<float>();
            unknown2 = Ar.Read<float>() * 0.01f;
            unknown3 = Ar.Read<float>() * 0.01f;
            unknown4 = DbToLinear(Ar.Read<float>());
            unknown5 = Ar.Read<uint8_t>();
            unknown6 = Ar.Read<uint8_t>();
        }
    };

    class BarbRecorderFXParams : public IAkPluginParam
    {
    public:
        float unknown1;

        explicit BarbRecorderFXParams(FWwiseArchive& Ar) : unknown1(Ar.Read<float>()) {}
    };

    class DrunkPMSourceParams : public IAkPluginParam
    {
    public:
        TPair<int32_t> unknown1;
        TPair<float> unknown2;
        TPair<float> unknown3;
        int32_t unknown4;

        explicit DrunkPMSourceParams(FWwiseArchive& Ar)
            : unknown1(Ar.Read<TPair<int32_t>>()),
              unknown2(Ar.Read<TPair<float>>()),
              unknown3(Ar.Read<TPair<float>>()),
              unknown4(Ar.Read<int32_t>()) {}
    };
}
