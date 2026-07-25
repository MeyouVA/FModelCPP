// Ported from CUE4Parse/UE4/Wwise/Plugins/Auro/CAuroPannerParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::Auro
{
    class CAuroPannerMixerParams : public IAkPluginParam
    {
    public:
        bool EnableDefaultSpatialization;
        float PanningLawdB;

        explicit CAuroPannerMixerParams(FWwiseArchive& Ar)
            : EnableDefaultSpatialization(Ar.Read<uint8_t>() != 0), PanningLawdB(Ar.Read<float>()) {}
    };

    class CAuroPannerFXParams : public IAkPluginParam
    {
    public:
        bool EnableCustomObjectSpread = false;
        float ObjectSpreadX = 0;
        float ObjectSpreadY = 0;
        float ObjectSpreadZ = 0;
        float CenterFactorFC = 0;
        float CenterFactorHC = 0;
        float CenterFactorT = 0;
        bool EnableDownfoldSettings = false;
        float DownfoldGainH = 0;
        float DownfoldGainT = 0;
        float DownfoldTopChannel = 0;
        bool PanningMode = false;
        float ZoomFactor = 0;
        float ZoomAzimuth = 0;
        float ZoomElevation = 0;

        explicit CAuroPannerFXParams(FWwiseArchive& Ar)
        {
            EnableCustomObjectSpread = Ar.Read<uint8_t>() != 0;
            ObjectSpreadX = Ar.Read<float>();
            ObjectSpreadY = Ar.Read<float>();
            ObjectSpreadZ = Ar.Read<float>();
            CenterFactorFC = Ar.Read<float>();
            CenterFactorHC = Ar.Read<float>();
            CenterFactorT = Ar.Read<float>();
            EnableDownfoldSettings = Ar.Read<uint8_t>() != 0;
            DownfoldGainH = Ar.Read<float>();
            DownfoldGainT = Ar.Read<float>();
            DownfoldTopChannel = Ar.Read<float>();
            PanningMode = Ar.Read<uint8_t>() != 0;
            ZoomFactor = Ar.Read<float>();
            ZoomAzimuth = Ar.Read<float>();
            ZoomElevation = Ar.Read<float>();
        }
    };
}
