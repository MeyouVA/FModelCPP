// Ported from CUE4Parse/UE4/Objects/Engine/EdGraph/EPinContainerType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::Engine::EdGraph
{
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EPinContainerType : uint8_t
    {
        None,
        Array,
        Set,
        Map,
    };
}
