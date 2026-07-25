// Ported from CUE4Parse/UE4/Wwise/Plugins/CrankcaseAudioREVModelPlayer/CREVSourceModelPlayerParams.cs
#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

#include "../../WwiseArchive.h"
#include "../IAkPluginParam.h"

namespace CUE4Parse::UE4::Wwise::Plugins::CrankcaseAudioREVModelPlayer
{
    // Each nested block starts with its own endian marker and size -- this plugin serialises its control
    // data as self-describing sub-structs rather than a flat layout.
    struct FEngineSimulationControlData
    {
        int16_t EndianStatus = 0;
        uint16_t Size = 0;

        float UpShiftDuration = 0;
        float UpShiftAttackDuration = 0;
        float UpShiftAttackVolumeSpike = 0;
        float UpShiftAttackRPM = 0;
        float UpShiftAttackThrottleTime = 0;
        bool UpShiftWobbleEnabled = false;
        float UpShiftWobblePitchFreq = 0;
        float UpShiftWobblePitchAmp = 0;
        float UpShiftWobbleVolFreq = 0;
        float UpShiftWobbleVolAmp = 0;
        float UpShiftWobbleDuration = 0;
        float DownShiftDuration = 0;
        float PopDuration = 0;
        float ClutchRPMSpike = 0;
        float ClutchRPMSpikeDuration = 0;
        float ClutchRPMMergeTime = 0;

        FEngineSimulationControlData() = default;

        explicit FEngineSimulationControlData(FWwiseArchive& Ar)
        {
            EndianStatus = Ar.Read<int16_t>();
            Size = Ar.Read<uint16_t>();

            UpShiftDuration = Ar.Read<float>();
            UpShiftAttackDuration = Ar.Read<float>();
            UpShiftAttackVolumeSpike = Ar.Read<float>();
            UpShiftAttackRPM = Ar.Read<float>();
            UpShiftAttackThrottleTime = Ar.Read<float>();
            UpShiftWobbleEnabled = Ar.Read<int32_t>() != 0;
            UpShiftWobblePitchFreq = Ar.Read<float>();
            UpShiftWobblePitchAmp = Ar.Read<float>();
            UpShiftWobbleVolFreq = Ar.Read<float>();
            UpShiftWobbleVolAmp = Ar.Read<float>();
            UpShiftWobbleDuration = Ar.Read<float>();
            DownShiftDuration = Ar.Read<float>();
            PopDuration = Ar.Read<float>();
            ClutchRPMSpike = Ar.Read<float>();
            ClutchRPMSpikeDuration = Ar.Read<float>();
            ClutchRPMMergeTime = Ar.Read<float>();
        }
    };

    struct GranularModelControlData
    {
        int16_t EndianStatus = 0;
        uint16_t SizeOf = 0;

        bool isValid = false;
        float LoadVolumeOff = 0;
        float LoadVolumeOn = 0;
        float RampVsLoopMaxWetDry = 0;
        float RampVsLoopMinWetDry = 0;
        float RampVsLoopSensitivity = 0;
        int32_t LoopCrossfadeStyle = 0;
        int32_t GrainWidth = 0;

        GranularModelControlData() = default;

        explicit GranularModelControlData(FWwiseArchive& Ar)
        {
            EndianStatus = Ar.Read<int16_t>();
            SizeOf = Ar.Read<uint16_t>();

            isValid = Ar.Read<int32_t>() != 0;
            LoadVolumeOff = Ar.Read<float>();
            LoadVolumeOn = Ar.Read<float>();
            RampVsLoopMaxWetDry = Ar.Read<float>();
            RampVsLoopMinWetDry = Ar.Read<float>();
            RampVsLoopSensitivity = Ar.Read<float>();
            LoopCrossfadeStyle = Ar.Read<int32_t>();
            GrainWidth = Ar.Read<int32_t>();
        }
    };

    struct FAccelDecelModelControlData
    {
        int16_t EndianStatus = 0;
        uint16_t SizeOf = 0;

        float MasterVolume = 0;
        float IdleVolume = 0;
        float IdleTechnique = 0;
        float IdleRampIn = 0;
        bool LowPassEnabled = false;
        int32_t HarmonicToTrack = 0;
        float QFactor = 0;
        float FilterDepth = 0;
        int32_t CrossfadeDuration = 0;
        float RPMSmoothness = 0;
        std::vector<GranularModelControlData> GranularModelControlDataArray;

        FAccelDecelModelControlData() = default;

        explicit FAccelDecelModelControlData(FWwiseArchive& Ar)
        {
            EndianStatus = Ar.Read<int16_t>();
            SizeOf = Ar.Read<uint16_t>();

            MasterVolume = Ar.Read<float>();
            IdleVolume = Ar.Read<float>();
            IdleTechnique = Ar.Read<float>();
            IdleRampIn = Ar.Read<float>();
            LowPassEnabled = Ar.Read<int32_t>() != 0;
            HarmonicToTrack = Ar.Read<int32_t>();
            QFactor = Ar.Read<float>();
            FilterDepth = Ar.Read<float>();
            CrossfadeDuration = Ar.Read<int32_t>();
            RPMSmoothness = Ar.Read<float>();
            GranularModelControlDataArray = Ar.ReadArrayWith(2, [&Ar] { return GranularModelControlData(Ar); });
        }
    };

    struct FAccelDecelModelControlData_old
    {
        int16_t EndianStatus = 0;
        uint16_t Size = 0;

        float DecelVolume_Off = 0;
        float DecelVolume_On = 0;
        float PopsEnabled = 0;
        float PopsVolumeMax = 0;
        float PopsVolumeMin = 0;
        float PopsFreqMin = 0;
        float PopsFreqMax = 0;
        float PopsEngineDuck = 0;
        float PopRange = 0;
        float PopDuration = 0;
        float IdleVolume = 0;
        float IdleTechnique = 0;
        float IdleRampIn = 0;

        FAccelDecelModelControlData_old() = default;

        explicit FAccelDecelModelControlData_old(FWwiseArchive& Ar)
        {
            EndianStatus = Ar.Read<int16_t>();
            Size = Ar.Read<uint16_t>();

            DecelVolume_Off = Ar.Read<float>();
            DecelVolume_On = Ar.Read<float>();
            PopsEnabled = Ar.Read<float>();
            PopsVolumeMax = Ar.Read<float>();
            PopsVolumeMin = Ar.Read<float>();
            PopsFreqMin = Ar.Read<float>();
            PopsFreqMax = Ar.Read<float>();
            PopsEngineDuck = Ar.Read<float>();
            PopRange = Ar.Read<float>();
            PopDuration = Ar.Read<float>();
            IdleVolume = Ar.Read<float>();
            IdleTechnique = Ar.Read<float>();
            IdleRampIn = Ar.Read<float>();
        }
    };

    class CREVSourceModelPlayerParams : public IAkPluginParam
    {
    public:
        float Gain = 0;
        float Throttle = 0;
        float RPM = 0;
        int32_t Gear = 0;
        float Velocity = 0;
        bool EnableShifting = false;

        FEngineSimulationControlData EngineSimulationControlData;
        std::optional<FAccelDecelModelControlData> AccelDecelModelControlData;
        std::optional<FAccelDecelModelControlData_old> AccelDecelModelControlData_oldValue;
        float Unknown = 0;

        // The `size` parameter is accepted to match C#'s signature; the reader never consults it.
        CREVSourceModelPlayerParams(FWwiseArchive& Ar, int /*size*/)
        {
            Gain = Ar.Read<float>();
            Throttle = Ar.Read<float>();
            RPM = Ar.Read<float>();
            Gear = Ar.Read<int32_t>();
            Velocity = Ar.Read<float>();
            EnableShifting = Ar.Read<int32_t>() != 0;
            EngineSimulationControlData = FEngineSimulationControlData(Ar);
            if (Ar.Version >= 132)
            {
                AccelDecelModelControlData = FAccelDecelModelControlData(Ar);
                // This trailing float is genuinely big-endian while everything around it is little-endian.
                Unknown = ReadSingleBigEndian(Ar);
            }
            else
            {
                AccelDecelModelControlData_oldValue = FAccelDecelModelControlData_old(Ar);
            }
        }

    private:
        // C#'s BinaryPrimitives.ReadSingleBigEndian(Ar.ReadBytes(4)).
        static float ReadSingleBigEndian(FWwiseArchive& Ar)
        {
            auto bytes = Ar.ReadBytes(4);
            const uint32_t be = (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) |
                                (static_cast<uint32_t>(bytes[2]) << 8) | static_cast<uint32_t>(bytes[3]);
            float result;
            std::memcpy(&result, &be, sizeof(result));
            return result;
        }
    };
}
