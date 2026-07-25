// Ported from CUE4Parse/UE4/FMod/Nodes/VCANode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FMixerStrip.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class VCANode
    {
    public:
        Objects::FModGuid BaseGuid;
        std::vector<Objects::FModGuid> Strips;
        Objects::FMixerStrip MixerStrip;

        explicit VCANode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Strips = FModReader::ReadElemListImp<Objects::FModGuid>(Ar);
            MixerStrip = Objects::FMixerStrip(Ar);
        }
    };
}
