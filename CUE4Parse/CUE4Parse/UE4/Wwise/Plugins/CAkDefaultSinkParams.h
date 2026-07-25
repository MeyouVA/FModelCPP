// Ported from CUE4Parse/UE4/Wwise/Plugins/CAkDefaultSinkParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins
{
    // Reads nothing -- used for the sinks that have no payload at all (C# constructs it without an
    // archive argument).
    class CAkDefaultSinkParams : public IAkPluginParam
    {
    };

    class CAkSystemSinkParams : public IAkPluginParam
    {
    public:
        bool Allow3DAudio;
        uint32_t MainMixHeadphoneConfiguration;
        uint32_t MainMixSpeakerConfiguration;
        bool AllowSystemAudioObjects;
        uint16_t MinSystemAudioObjectsRequired;

        explicit CAkSystemSinkParams(FWwiseArchive& Ar)
            : Allow3DAudio(Ar.Read<uint8_t>() != 0),
              MainMixHeadphoneConfiguration(Ar.Read<uint32_t>()),
              MainMixSpeakerConfiguration(Ar.Read<uint32_t>()),
              AllowSystemAudioObjects(Ar.Read<uint8_t>() != 0),
              MinSystemAudioObjectsRequired(Ar.Read<uint16_t>()) {}
    };

    class CAkDVRSinkParams : public IAkPluginParam
    {
    public:
        bool DVRRecordable;

        explicit CAkDVRSinkParams(FWwiseArchive& Ar) : DVRRecordable(Ar.Read<uint8_t>() != 0) {}
    };
}
