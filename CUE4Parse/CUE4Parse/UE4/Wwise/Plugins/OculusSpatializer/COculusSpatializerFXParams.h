// Ported from CUE4Parse/UE4/Wwise/Plugins/OculusSpatializer/COculusSpatializerFXParams.cs
#pragma once

#include <cstdint>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::OculusSpatializer
{
    class COculusSpatializerFXAttachmentParams : public IAkPluginParam
    {
    public:
        bool BypassSpatializer = false;
        bool EnableReflections = false;
        bool UseInvSqAttenuation = false;
        float AttenuationRangeMin = 0;
        float AttenuationRangeMax = 0;
        float ReverbSendLevel = 0;
        float VolumetricRadius = 0;
        bool Ambisonic = false;

        explicit COculusSpatializerFXAttachmentParams(FWwiseArchive& Ar)
        {
            BypassSpatializer = Ar.Read<uint8_t>() != 0;
            EnableReflections = Ar.Read<uint8_t>() != 0;
            UseInvSqAttenuation = Ar.Read<uint8_t>() != 0;
            AttenuationRangeMin = Ar.Read<float>();
            AttenuationRangeMax = Ar.Read<float>();
            ReverbSendLevel = Ar.Read<float>();
            VolumetricRadius = Ar.Read<float>();
            Ambisonic = Ar.Read<uint8_t>() != 0;
        }
    };

    class COculusSpatializerFXParams : public IAkPluginParam
    {
    public:
        // Plugin-local version number, unrelated to the bank version on the archive.
        float Version = 0;
        bool Bypass = false;
        bool EnableReflections = false;
        float RoomSizeX = 0;
        float RoomSizeY = 0;
        float RoomSizeZ = 0;
        float ReflectLeft = 0;
        float ReflectRight = 0;
        float ReflectFront = 0;
        float ReflectBehind = 0;
        float ReflectUp = 0;
        float ReflectDown = 0;
        float GlobalScale = 0;
        float Gain = 0;
        bool DEBUG_ClampPos = false;
        bool DEBUG_Misc = false;
        bool ReverbOn = false;
        float ReflectionsRangeMin = 0;
        float ReflectionsRangeMax = 0;
        float ReverbWetMix = 0;
        int32_t VoiceLimit = 0;

        explicit COculusSpatializerFXParams(FWwiseArchive& Ar)
        {
            Version = Ar.Read<float>();
            Bypass = Ar.Read<uint8_t>() != 0;
            EnableReflections = Ar.Read<uint8_t>() != 0;
            RoomSizeX = Ar.Read<float>();
            RoomSizeY = Ar.Read<float>();
            RoomSizeZ = Ar.Read<float>();
            ReflectLeft = Ar.Read<float>();
            ReflectRight = Ar.Read<float>();
            ReflectFront = Ar.Read<float>();
            ReflectBehind = Ar.Read<float>();
            ReflectUp = Ar.Read<float>();
            ReflectDown = Ar.Read<float>();
            GlobalScale = Ar.Read<float>();
            Gain = Ar.Read<float>();
            DEBUG_ClampPos = Ar.Read<uint8_t>() != 0;
            DEBUG_Misc = Ar.Read<uint8_t>() != 0;
            ReverbOn = Ar.Read<uint8_t>() != 0;
            ReflectionsRangeMin = Ar.Read<float>();
            ReflectionsRangeMax = Ar.Read<float>();
            ReverbWetMix = Ar.Read<float>();
            VoiceLimit = Ar.Read<int32_t>();
        }
    };
}
