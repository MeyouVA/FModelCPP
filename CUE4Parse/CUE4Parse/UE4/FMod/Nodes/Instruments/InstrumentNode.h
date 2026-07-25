// Ported from CUE4Parse/UE4/FMod/Nodes/Instruments/InstrumentNode.cs
#pragma once

#include <vector>

#include "../../Objects/FModGuid.h"
#include "../../Objects/FTriggerDelay.h"
#include "../../Objects/FQuantization.h"
#include "../../Objects/FRoutable.h"
#include "../../Objects/FEvaluator.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Instruments
{
    class InstrumentNode
    {
    public:
        Objects::FModGuid TimelineGuid;
        float Volume = 0.0f;
        float Pitch = 0.0f;
        int32_t LoopCount = 0;
        int32_t Flags = 0;
        float Offset3DDistance = 0.0f;
        float TriggerChancePercent = 0.0f;
        Objects::FTriggerDelay TriggerDelay;
        Objects::FQuantization Quantization;
        Objects::FModGuid ControlParameterGuid;
        float AutoPitchReference = 0.0f;
        float InitialSeekPosition = 0.0f;
        int32_t MaximumPolyphony = 0;
        Objects::FRoutable Routable;
        int32_t PolyphonyLimitBehavior = 0;
        uint32_t LeftTrimOffset = 0;
        float InitialSeekPercent = 0.0f;
        float AutoPitchAtMinimum = 0.0f;
        std::vector<Objects::FEvaluator> Evaluators;

        explicit InstrumentNode(Readers::FArchive& Ar) : TimelineGuid(Ar)
        {
            Volume = Ar.Read<float>();
            Pitch = Ar.Read<float>();
            LoopCount = Ar.Read<int32_t>();

            if (FModReader::Version() >= 0x82)
                Flags = Ar.Read<int32_t>();
            else
                Flags = Ar.Read<uint8_t>();

            Offset3DDistance = Ar.Read<float>();
            TriggerChancePercent = Ar.Read<float>();
            TriggerDelay = Objects::FTriggerDelay(Ar);
            Quantization = Objects::FQuantization(Ar);
            ControlParameterGuid = Objects::FModGuid(Ar);
            AutoPitchReference = Ar.Read<float>();
            InitialSeekPosition = Ar.Read<float>();
            MaximumPolyphony = Ar.Read<int32_t>();
            Routable = Objects::FRoutable(Ar);

            if (FModReader::Version() >= 0x35)
                PolyphonyLimitBehavior = Ar.Read<int32_t>();

            if (FModReader::Version() >= 0x47)
                LeftTrimOffset = Ar.Read<uint32_t>();

            if (FModReader::Version() >= 0x48)
                InitialSeekPercent = Ar.Read<float>();

            if (FModReader::Version() >= 0x50)
                AutoPitchAtMinimum = Ar.Read<float>();

            if (FModReader::Version() >= 0x82)
                Evaluators = Objects::FEvaluator::ReadEvaluatorList(Ar);
        }
    };
}
