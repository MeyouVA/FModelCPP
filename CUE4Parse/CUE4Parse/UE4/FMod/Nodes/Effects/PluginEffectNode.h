// Ported from CUE4Parse/UE4/FMod/Nodes/Effects/PluginEffectNode.cs
#pragma once

#include <memory>
#include <string>

#include "BaseEffectNode.h"
#include "ParameterizedEffectNode.h"
#include "../../Objects/FModGuid.h"
#include "../../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes::Effects
{
    class PluginEffectNode : public BaseEffectNode
    {
    public:
        Objects::FModGuid BaseGuid;
        std::string PluginName;
        std::string Name;
        std::unique_ptr<ParameterizedEffectNode> ParamEffectBody;

        explicit PluginEffectNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            if (FModReader::Version() < 0x5b) (void) Ar.Read<uint32_t>(); // Legacy InputChannelLayout

            PluginName = FModReader::ReadString(Ar);
            if (FModReader::Version() >= 0x36) Name = FModReader::ReadString(Ar);

            if (FModReader::Version() >= 0x3d && FModReader::Version() <= 0x91)
                (void) Ar.Read<uint8_t>(); // legacy bypass
        }
    };
}
