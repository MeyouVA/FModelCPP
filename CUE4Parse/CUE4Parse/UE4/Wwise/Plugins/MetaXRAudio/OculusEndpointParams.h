// Ported from CUE4Parse/UE4/Wwise/Plugins/MetaXRAudio/OculusEndpointParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::MetaXRAudio
{
    class OculusEndpointSinkParams : public IAkPluginParam
    {
    public:
        uint16_t SpatializedVoiceLimit = 0;
        float GlobalScale = 0;
        float RoomLength = 0;
        float RoomWidth = 0;
        float RoomHeight = 0;
        uint16_t LeftWallMaterial = 0;
        uint16_t RightWallMaterial = 0;
        uint16_t FrontWallMaterial = 0;
        uint16_t BackWallMaterial = 0;
        uint16_t CeilingMaterial = 0;
        uint16_t FloorMaterial = 0;
        bool EnableEarlyReflections = false;
        bool EnableReverb = false;
        float ReverbWetLevel = 0;
        float ClutterFactor = 0;

        explicit OculusEndpointSinkParams(FWwiseArchive& Ar)
        {
            SpatializedVoiceLimit = Ar.Read<uint16_t>();
            GlobalScale = Ar.Read<float>();
            RoomLength = Ar.Read<float>();
            RoomWidth = Ar.Read<float>();
            RoomHeight = Ar.Read<float>();
            LeftWallMaterial = Ar.Read<uint16_t>();
            RightWallMaterial = Ar.Read<uint16_t>();
            FrontWallMaterial = Ar.Read<uint16_t>();
            BackWallMaterial = Ar.Read<uint16_t>();
            CeilingMaterial = Ar.Read<uint16_t>();
            FloorMaterial = Ar.Read<uint16_t>();
            EnableEarlyReflections = Ar.Read<uint8_t>() != 0;
            EnableReverb = Ar.Read<uint8_t>() != 0;
            ReverbWetLevel = Ar.Read<float>();
            ClutterFactor = Ar.Read<float>();
        }
    };

    class OculusEndpointMetadataParams : public IAkPluginParam
    {
    public:
        bool EnableAcoustics;
        float ReverbSendLevel;
        uint16_t DistanceAttenuationMode;

        explicit OculusEndpointMetadataParams(FWwiseArchive& Ar)
            : EnableAcoustics(Ar.Read<uint8_t>() != 0),
              ReverbSendLevel(Ar.Read<float>()),
              DistanceAttenuationMode(Ar.Read<uint16_t>()) {}
    };

    class OculusEndpointExperimentalMetadataParams : public IAkPluginParam
    {
    public:
        uint16_t DirectivityPattern = 0;
        float ReflectionSendLevel = 0;
        float VolumetricRadius = 0;
        float HRTFIntensity = 0;
        bool SoloReverbSend = false;
        float DirectivityIntensity = 0;
        float ReverbReach = 0;
        float OcclusionIntensity = 0;
        bool MediumAbsorption = false;

        explicit OculusEndpointExperimentalMetadataParams(FWwiseArchive& Ar)
        {
            DirectivityPattern = Ar.Read<uint16_t>();
            ReflectionSendLevel = Ar.Read<float>();
            VolumetricRadius = Ar.Read<float>();
            HRTFIntensity = Ar.Read<float>();
            SoloReverbSend = Ar.Read<uint8_t>() != 0;
            DirectivityIntensity = Ar.Read<float>();
            ReverbReach = Ar.Read<float>();
            OcclusionIntensity = Ar.Read<float>();
            MediumAbsorption = Ar.Read<uint8_t>() != 0;
        }
    };
}
