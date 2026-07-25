// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkRoomVerbFXParams.cs
#pragma once

#include <cmath>
#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    enum class AkFilterInsertType : uint32_t
    {
        Off = 0x0,
        EROnly = 0x1,
        ReverbOnly = 0x2,
        ERAndReverb = 0x3
    };

    enum class AkFilterCurveType : uint32_t
    {
        LowShelf = 0x0,
        Peaking = 0x1,
        HighShelf = 0x2
    };

#pragma pack(push, 4)
    struct AkRoomVerbAlgoTunings
    {
        float DensityDelayMin;
        float DensityDelayMax;
        float DensityDelayRdmPerc;
        float RoomShapeMin;
        float RoomShapeMax;
        float DiffusionDelayScalePerc;
        float DiffusionDelayMax;
        float DiffusionDelayRdmPerc;
        float DCFilterCutFreq;
        float ReverbUnitInputDelay;
        float ReverbUnitInputDelayRdmPerc;
    };
#pragma pack(pop)

    struct AkRoomVerbRTPCParams
    {
        float DecayTime = 0;
        float HFDamping = 0;
        float Diffusion = 0;
        float StereoWidth = 0;
        float Filter1Gain = 0;
        float Filter1Freq = 0;
        float Filter1Q = 0;
        float Filter2Gain = 0;
        float Filter2Freq = 0;
        float Filter2Q = 0;
        float Filter3Gain = 0;
        float Filter3Freq = 0;
        float Filter3Q = 0;
        float FrontLevel = 0;
        float RearLevel = 0;
        float CenterLevel = 0;
        float LFELevel = 0;
        float DryLevel = 0;
        float ERLevel = 0;
        float ReverbLevel = 0;

        AkRoomVerbRTPCParams() = default;

        explicit AkRoomVerbRTPCParams(FWwiseArchive& Ar)
        {
            DecayTime = Ar.Read<float>();
            HFDamping = Ar.Read<float>();
            Diffusion = Ar.Read<float>();
            StereoWidth = Ar.Read<float>();
            Filter1Gain = Ar.Read<float>();
            Filter1Freq = Ar.Read<float>();
            Filter1Q = Ar.Read<float>();
            Filter2Gain = Ar.Read<float>();
            Filter2Freq = Ar.Read<float>();
            Filter2Q = Ar.Read<float>();
            Filter3Gain = Ar.Read<float>();
            Filter3Freq = Ar.Read<float>();
            Filter3Q = Ar.Read<float>();
            FrontLevel = DbToLinear(Ar.Read<float>());
            RearLevel = DbToLinear(Ar.Read<float>());
            CenterLevel = DbToLinear(Ar.Read<float>());
            LFELevel = DbToLinear(Ar.Read<float>());
            DryLevel = DbToLinear(Ar.Read<float>());
            ERLevel = DbToLinear(Ar.Read<float>());
            // The odd one out: this level alone gets an extra -0.15 in the exponent, and C# computes it
            // in double precision (Math.Pow, not MathF.Pow) before narrowing.
            ReverbLevel = static_cast<float>(std::pow(10.0, Ar.Read<float>() * 0.05 - 0.15));
        }
    };

    struct AkRoomVerbInvariantParams
    {
        bool bEnableEarlyReflections = false;
        uint32_t ERPattern = 0;
        float ReverbDelay = 0;
        float RoomSize = 0;
        float ERFrontBackDelay = 0;
        float Density = 0;
        float RoomShape = 0;
        uint32_t NumReverbUnits = 0;
        bool bEnableToneControls = false;
        AkFilterInsertType Filter1Pos = static_cast<AkFilterInsertType>(0);
        AkFilterCurveType Filter1Curve = static_cast<AkFilterCurveType>(0);
        AkFilterInsertType Filter2Pos = static_cast<AkFilterInsertType>(0);
        AkFilterCurveType Filter2Curve = static_cast<AkFilterCurveType>(0);
        AkFilterInsertType Filter3Pos = static_cast<AkFilterInsertType>(0);
        AkFilterCurveType Filter3Curve = static_cast<AkFilterCurveType>(0);
        float InputCenterLevel = 0;
        float InputLFELevel = 0;

        AkRoomVerbInvariantParams() = default;

        explicit AkRoomVerbInvariantParams(FWwiseArchive& Ar)
        {
            bEnableEarlyReflections = Ar.Read<uint8_t>() != 0;
            ERPattern = Ar.Read<uint32_t>();
            ReverbDelay = Ar.Read<float>();
            RoomSize = Ar.Read<float>();
            ERFrontBackDelay = Ar.Read<float>();
            Density = Ar.Read<float>();
            RoomShape = Ar.Read<float>();
            NumReverbUnits = Ar.Read<uint32_t>();
            bEnableToneControls = Ar.Read<uint8_t>() != 0;
            Filter1Pos = Ar.Read<AkFilterInsertType>();
            Filter1Curve = Ar.Read<AkFilterCurveType>();
            Filter2Pos = Ar.Read<AkFilterInsertType>();
            Filter2Curve = Ar.Read<AkFilterCurveType>();
            Filter3Pos = Ar.Read<AkFilterInsertType>();
            Filter3Curve = Ar.Read<AkFilterCurveType>();
            InputCenterLevel = DbToLinear(Ar.Read<float>());
            InputLFELevel = DbToLinear(Ar.Read<float>());
        }
    };

    struct AkRoomVerbFXParams
    {
        AkRoomVerbRTPCParams RTPCParams;
        AkRoomVerbInvariantParams InvariantParams;
        AkRoomVerbAlgoTunings AlgoTunings{};

        AkRoomVerbFXParams() = default;

        explicit AkRoomVerbFXParams(FWwiseArchive& Ar)
        {
            RTPCParams = AkRoomVerbRTPCParams(Ar);
            InvariantParams = AkRoomVerbInvariantParams(Ar);
            AlgoTunings = Ar.Read<AkRoomVerbAlgoTunings>();
        }
    };

    class CAkRoomVerbFXParams : public IAkPluginParam
    {
    public:
        AkRoomVerbFXParams Params;

        explicit CAkRoomVerbFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
