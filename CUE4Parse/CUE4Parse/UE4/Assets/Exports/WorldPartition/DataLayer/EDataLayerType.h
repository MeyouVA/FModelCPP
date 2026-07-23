// Ported from CUE4Parse/UE4/Assets/Exports/WorldPartition/DataLayer/EDataLayerType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::WorldPartition::DataLayer
{
    enum class EDataLayerType
    {
        Runtime,
        Editor,
        Unknown,
        Size,
    };
}
