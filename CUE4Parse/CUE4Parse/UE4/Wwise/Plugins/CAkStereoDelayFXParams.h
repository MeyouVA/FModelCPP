// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkStereoDelayFXParams.cs
#pragma once

#include <array>
#include <cstdint>

#include "../WwiseArchive.h"
#include "CAkParameterEQFXParams.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    struct AkStereoDelayFXParams
    {
        enum class AkInputChannelType : uint32_t
        {
            LEFT_OR_RIGHT = 0x0,
            CENTER = 0x1,
            DOWNMIX = 0x2,
            NONE = 0x3
        };

        struct AkStereoDelayChannelParams
        {
            float fDelayTime = 0;
            float fFeedback = 0;
            float fCrossFeed = 0;

            AkStereoDelayChannelParams() = default;

            explicit AkStereoDelayChannelParams(FWwiseArchive& Ar)
            {
                fDelayTime = Ar.Read<float>();
                fFeedback = DbToLinear(Ar.Read<float>());
                fCrossFeed = DbToLinear(Ar.Read<float>());
            }
        };

        std::array<AkInputChannelType, 2> eInputType{};
        std::array<AkStereoDelayChannelParams, 2> StereoDelayParams{};
        AkFilterParams FilterParams{};
        float fDryLevel = 0;
        float fWetLevel = 0;
        float fFrontRearBalance = 0;
        bool bEnableFeedback = false;
        bool bEnableCrossFeed = false;

        AkStereoDelayFXParams() = default;

        // The two channels are interleaved type-then-params, not both types followed by both params.
        explicit AkStereoDelayFXParams(FWwiseArchive& Ar)
        {
            eInputType[0] = Ar.Read<AkInputChannelType>();
            StereoDelayParams[0] = AkStereoDelayChannelParams(Ar);
            eInputType[1] = Ar.Read<AkInputChannelType>();
            StereoDelayParams[1] = AkStereoDelayChannelParams(Ar);
            FilterParams = Ar.Read<AkFilterParams>();
            fDryLevel = DbToLinear(Ar.Read<float>());
            fWetLevel = DbToLinear(Ar.Read<float>());
            fFrontRearBalance = Ar.Read<float>();
            bEnableFeedback = Ar.Read<uint8_t>() != 0;
            bEnableCrossFeed = Ar.Read<uint8_t>() != 0;
        }
    };

    class CAkStereoDelayFXParams : public IAkPluginParam
    {
    public:
        AkStereoDelayFXParams Params;

        explicit CAkStereoDelayFXParams(FWwiseArchive& Ar) : Params(Ar) {}
    };
}
