// Ported from CUE4Parse/UE4/Wwise/Objects/CAkLayer.cs
#pragma once

#include <cstdint>
#include <vector>

#include "../WwiseArchive.h"
#include "../Enums/EAkGameSyncType.h"
#include "AkConversionTable.h"
#include "AkRTPC.h"

namespace CUE4Parse::UE4::Wwise::Objects
{
    using CUE4Parse::UE4::Wwise::Enums::EAkGameSyncType;

    class CAkLayer
    {
    public:
        struct AkAssociatedLayerChild
        {
            uint32_t AssociatedChildId = 0;
            std::vector<AkRtpcGraphPoint> GraphPoints;

            AkAssociatedLayerChild() = default;

            explicit AkAssociatedLayerChild(FWwiseArchive& Ar)
            {
                AssociatedChildId = Ar.Read<uint32_t>();
                GraphPoints = AkRtpcGraphPoint::ReadArray(Ar);
            }

            // Note this count is a signed int, unlike the uint the graph points themselves use.
            static std::vector<AkAssociatedLayerChild> ReadArray(FWwiseArchive& Ar)
            {
                const int count = Ar.Read<int32_t>();
                return Ar.ReadArrayWith(count, [&Ar] { return AkAssociatedLayerChild(Ar); });
            }
        };

        uint32_t LayerId = 0;
        std::vector<AkRtpc> Rtpcs;
        uint32_t RtpcId = 0;
        EAkGameSyncType RtpcType = static_cast<EAkGameSyncType>(0);
        float RtpcCrossfadingDefaultValue = 0;
        std::vector<AkAssociatedLayerChild> Associations;

        CAkLayer() = default;

        explicit CAkLayer(FWwiseArchive& Ar)
        {
            LayerId = Ar.Read<uint32_t>();
            Rtpcs = AkRtpc::ReadArray(Ar);
            RtpcId = Ar.Read<uint32_t>();

            if (Ar.Version > 89)
            {
                RtpcType = Ar.Read<EAkGameSyncType>();
            }

            if (Ar.Version <= 59)
            {
                RtpcCrossfadingDefaultValue = Ar.Read<float>();
            }

            // CAkLayer::SetChildAssoc
            Associations = AkAssociatedLayerChild::ReadArray(Ar);
        }
    };
}
