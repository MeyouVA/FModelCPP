// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/BuiltInEffectNode.cs
#pragma once

#include <memory>

#include "BaseEffectNode.h"
#include "ParameterizedEffectNode.h"
#include "../../Objects/FModGuid.h"
#include "../../Enums/EDSPType.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class BuiltInEffectNode : public BaseEffectNode
    {
    public:
        Objects::FModGuid BaseGuid;
        uint32_t InputChannelLayout = 0;
        Enums::EDSPType DSPType{};
        std::unique_ptr<ParameterizedEffectNode> ParamEffectBody;

        explicit BuiltInEffectNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() < 0x5B) InputChannelLayout = Ar.Read<uint32_t>();
            DSPType = static_cast<Enums::EDSPType>(Ar.Read<uint32_t>());

            if (FModReader::Version() >= 0x3D && FModReader::Version() <= 0x91)
                (void) Ar.Read<uint8_t>(); // legacy bypass
        }
    };
}
