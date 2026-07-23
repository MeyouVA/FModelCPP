// Ported from CUE4Parse/UE4/Wwise/Enums/EAkGroupType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    // C# tags this [JsonConverter(typeof(StringEnumConverter))] -- it serialises by member
    // name, not by number. The JSON writer is not ported yet; noted here so it is not lost.
    enum class EAkGroupType : uint8_t
    {
        Switch,
        State,
    };
}
