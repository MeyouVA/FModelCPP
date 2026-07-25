// Ported from CUE4Parse/UE4/FMod/Nodes/ControllerNode.cs
#pragma once

#include "../Objects/FModGuid.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class ControllerNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid PropertyOwnerGuid;
        Objects::FModGuid CurveGuid;
        int32_t PropertyIndex = 0;

        explicit ControllerNode(Readers::FArchive& Ar) : BaseGuid(Ar), PropertyOwnerGuid(Ar)
        {
            if (FModReader::Version() < 0x5a) (void) Objects::FModGuid(Ar); // legacy guid, discarded
            CurveGuid = Objects::FModGuid(Ar);
            PropertyIndex = Ar.Read<int32_t>();
        }
    };
}
