// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/SendEffectNode.cs
#pragma once

#include "BaseEffectNode.h"
#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class SendEffectNode : public BaseEffectNode
    {
    public:
        Objects::FModGuid BaseGuid;
        uint32_t InputChannelLayout = 0;
        Objects::FModGuid ReturnGuid;
        float SendLevel = 0.0f;

        explicit SendEffectNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() < 0x5B) InputChannelLayout = Ar.Read<uint32_t>();
            ReturnGuid = Objects::FModGuid(Ar);
            SendLevel = Ar.Read<float>();

            if (FModReader::Version() >= 0x3D && FModReader::Version() <= 0x91)
                (void) Ar.Read<uint8_t>(); // legacy bypass
        }
    };
}
