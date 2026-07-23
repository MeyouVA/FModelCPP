// Ported from CUE4Parse/UE4/Wwise/Enums/EAkActionScope.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkActionScope : uint8_t
    {
        None             = 0x0,
        GameObject,
        Global,
        GameObjectId,
        All,
        GlobalGameObject,
        AllExceptId      = 0x09,
        Ducking          = 0x20,
    };
}
