// Ported from CUE4Parse/UE4/Wwise/Objects/AkAdvSettingsParams.cs
#pragma once

#include <cstdint>

#include "../WwiseArchive.h"
#include "../Enums/EAkBelowThresholdBehavior.h"
#include "../Enums/EAkVirtualQueueBehavior.h"
#include "../Enums/Flags/EAkAdvSettingsFlags.h"
#include "../Enums/Flags/EAkHdrEnvelopeFlags.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkBelowThresholdBehavior;
    using CUE4Parse::UE4::Wwise::Enums::EAkVirtualQueueBehavior;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EAkAdvSettingsFlags;
    using CUE4Parse::UE4::Wwise::Enums::Flags::EAkHdrEnvelopeFlags;

    // Before version 90 the flags were individual bools on the wire; the port rebuilds the same flag word
    // from them, exactly as C# does.
    class AkAdvSettingsParams
    {
    public:
        EAkAdvSettingsFlags AdvSettingsFlags = EAkAdvSettingsFlags::None;
        EAkVirtualQueueBehavior VirtualQueueBehavior = static_cast<EAkVirtualQueueBehavior>(0);
        uint16_t MaxNumInstance = 0;
        bool IsGlobalLimit = false;
        EAkBelowThresholdBehavior BelowThresholdBehavior = static_cast<EAkBelowThresholdBehavior>(0);
        EAkHdrEnvelopeFlags HdrEnvelopeFlags = EAkHdrEnvelopeFlags::None;

        AkAdvSettingsParams() = default;

        explicit AkAdvSettingsParams(FWwiseArchive& Ar)
        {
            if (Ar.Version <= 36)
            {
                VirtualQueueBehavior = static_cast<EAkVirtualQueueBehavior>(Ar.Read<uint32_t>());
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::KillNewest;

                MaxNumInstance = Ar.Read<uint16_t>();
                BelowThresholdBehavior = static_cast<EAkBelowThresholdBehavior>(Ar.Read<uint32_t>());

                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::IsMaxNumInstOverrideParent;
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::IsVVoicesOptOverrideParent;
            }
            else if (Ar.Version <= 53)
            {
                VirtualQueueBehavior = static_cast<EAkVirtualQueueBehavior>(Ar.Read<uint8_t>());
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::KillNewest;

                MaxNumInstance = Ar.Read<uint16_t>();
                BelowThresholdBehavior = static_cast<EAkBelowThresholdBehavior>(Ar.Read<uint8_t>());

                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::IsMaxNumInstOverrideParent;
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::IsVVoicesOptOverrideParent;
            }
            else if (Ar.Version <= 89)
            {
                VirtualQueueBehavior = static_cast<EAkVirtualQueueBehavior>(Ar.Read<uint8_t>());
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::KillNewest;
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::UseVirtualBehavior;

                MaxNumInstance = Ar.Read<uint16_t>();
                IsGlobalLimit = Ar.ReadBool();
                BelowThresholdBehavior = static_cast<EAkBelowThresholdBehavior>(Ar.Read<uint8_t>());

                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::IsMaxNumInstOverrideParent;
                if (Ar.ReadBool())
                    AdvSettingsFlags |= EAkAdvSettingsFlags::IsVVoicesOptOverrideParent;

                if (Ar.Version > 72)
                {
                    if (Ar.ReadBool())
                        HdrEnvelopeFlags |= EAkHdrEnvelopeFlags::OverrideHdrEnvelope;
                    if (Ar.ReadBool())
                        HdrEnvelopeFlags |= EAkHdrEnvelopeFlags::OverrideAnalysis;
                    if (Ar.ReadBool())
                        HdrEnvelopeFlags |= EAkHdrEnvelopeFlags::NormalizeLoudness;
                    if (Ar.ReadBool())
                        HdrEnvelopeFlags |= EAkHdrEnvelopeFlags::EnableEnvelope;
                }
            }
            else
            {
                AdvSettingsFlags = static_cast<EAkAdvSettingsFlags>(Ar.Read<uint8_t>());
                VirtualQueueBehavior = static_cast<EAkVirtualQueueBehavior>(Ar.Read<uint8_t>());
                MaxNumInstance = Ar.Read<uint16_t>();
                BelowThresholdBehavior = static_cast<EAkBelowThresholdBehavior>(Ar.Read<uint8_t>());
                HdrEnvelopeFlags = static_cast<EAkHdrEnvelopeFlags>(Ar.Read<uint8_t>());
            }
        }
    };
}
