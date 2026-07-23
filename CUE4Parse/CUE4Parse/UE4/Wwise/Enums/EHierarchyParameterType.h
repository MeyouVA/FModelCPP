// Ported from CUE4Parse/UE4/Wwise/Enums/EHierarchyParameterType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EHierarchyParameterType : uint16_t
    {
        VoiceVolume,
        VoicePitch                  = 0x02,
        VoiceLowPass,
        VoiceHighPass,
        BusVolume,
        MakeUpGain,
        PlaybackPriority,
        PlaybackPriorityOffset,
        MotionToVolumeOffset,
        MotionLowPass,
        PositioningPannerX          = 0x0C,
        PositioningPannerY,
        PositioningCenterPercentage,
        ActionDelay,
        ActionFadeInTime,
        Probability,
        OverrideAuxBus0Volume       = 0x13,
        OverrideAuxBus1Volume,
        OverrideAuxBus2Volume,
        OverrideAuxBus3Volume,
        GameDefinedAuxSendVolume,
        OverrideBusVolume,
        OverrideBusHighPassFilter,
        OverrideBusLowPassFilter,
        HdrThreshold,
        HdrRatio,
        HdrReleaseTime,
        HdrOutputGameParam,
        HdrOutputGameParamMin,
        HdrOutputGameParamMax,
        HdrEnvelopeActiveRange,
        MidiNoteTrackingUnknown     = 0x2E,
        MidiTranspositionInt,
        MidiVelocityOffsetInt,
        MidiFiltersKeyRangeMin,
        MidiFiltersKeyRangeMax,
        MidiFiltersVelocityRangeMin,
        MidiFiltersVelocityRangeMax,
        PlaybackSpeed               = 0x36,
        MidiClipTempoSourceIsFile,
        LoopTimeUInt                = 0x3A,
        InitialDelay,
    };
}
