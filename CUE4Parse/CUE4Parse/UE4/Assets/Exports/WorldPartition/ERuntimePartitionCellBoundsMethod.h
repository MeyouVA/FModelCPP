// Ported from CUE4Parse/UE4/Assets/Exports/WorldPartition/ERuntimePartitionCellBoundsMethod.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::WorldPartition
{
    enum class ERuntimePartitionCellBoundsMethod : uint8_t
    {
        UseContent,
        UseCellBounds,
        UseMinContentCellBounds,
    };
}
