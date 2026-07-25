// Ported from CUE4Parse/UE4/Wwise/Plugins/Bitcrush/CBitcrushFXParams.cs
#pragma once

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::Bitcrush
{
    class CBitcrushFXParams : public IAkPluginParam
    {
    public:
        float InputAmplitude;
        float OutputAmplitude;
        float BitRate;
        float SampleRate;
        bool ClipType;
        float Drive;

        explicit CBitcrushFXParams(FWwiseArchive& Ar)
            : InputAmplitude(Ar.Read<float>()),
              OutputAmplitude(Ar.Read<float>()),
              BitRate(Ar.Read<float>()),
              SampleRate(Ar.Read<float>()),
              // C# uses Ar.ReadBool() here (one byte), not the four-byte FArchive.ReadBoolean.
              ClipType(Ar.ReadBool()),
              Drive(Ar.Read<float>()) {}
    };
}
