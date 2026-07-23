// Ported from CUE4Parse/UE4/Assets/Exports/WorldPartition/DataLayer/EDataLayerRuntimeState.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::WorldPartition::DataLayer
{
    enum class EDataLayerRuntimeState : uint8_t
    {
        // Unloaded
        Unloaded,
        // Loaded (meaning loaded but not visible)
        Loaded,
        // Activated (meaning loaded and visible)
        Activated,
    };
}
