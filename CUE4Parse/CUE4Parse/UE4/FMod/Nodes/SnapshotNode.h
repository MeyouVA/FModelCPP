// Ported from CUE4Parse/UE4/FMod/Nodes/SnapshotNode.cs
#pragma once

#include <vector>

#include "../Objects/FModGuid.h"
#include "../Objects/FSnapshot.h"
#include "../Enums/EAutomationConflictResolutionMethod.h"
#include "../FModReader.h"

namespace CUE4Parse::UE4::FMod::Nodes
{
    class SnapshotNode
    {
    public:
        Objects::FModGuid BaseGuid;
        int32_t Priority = 0;
        std::vector<Objects::FSnapshot> Snapshots;
        bool BlendingSnapshot = false;
        Enums::EAutomationConflictResolutionMethod GroupResolutionMethod{};
        float Intensity = 0.0f;

        explicit SnapshotNode(Readers::FArchive& Ar) : BaseGuid(Ar)
        {
            Priority = Ar.Read<int32_t>();
            Snapshots = FModReader::ReadElemListImp<Objects::FSnapshot>(Ar);
            BlendingSnapshot = Ar.Read<uint8_t>() != 0;
            GroupResolutionMethod = static_cast<Enums::EAutomationConflictResolutionMethod>(Ar.Read<uint32_t>());
            Intensity = Ar.Read<float>();
        }
    };
}
