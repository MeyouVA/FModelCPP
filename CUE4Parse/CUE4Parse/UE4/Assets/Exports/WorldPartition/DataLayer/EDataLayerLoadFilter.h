// Ported from CUE4Parse/UE4/Assets/Exports/WorldPartition/DataLayer/EDataLayerLoadFilter.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Assets::Exports::WorldPartition::DataLayer
{
    enum class EDataLayerLoadFilter : uint8_t
    {
        // Data Layer is considered by the client and the server. Client runtime state is replicated.
        None,
        // Data Layer is only considered by the client.
        ClientOnly,
        // Data layer is only considered by the server.
        ServerOnly,
    };
}
