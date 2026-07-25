// Ported from CUE4Parse/UE4/FMod/Nodes/CurveNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FCurvePoint.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class CurveNode
    {
    public:
        Objects::FModGuid BaseGuid;
        Objects::FModGuid OwnerGuid;
        std::vector<Objects::FCurvePoint> CurvePoints;

        explicit CurveNode(Readers::FArchive& Ar) : BaseGuid(Ar), OwnerGuid(Ar)
        {
            CurvePoints = FModReader::ReadElemListImp<Objects::FCurvePoint>(Ar);
        }
    };
}
