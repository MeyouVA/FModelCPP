// Ported from CUE4Parse/UE4/Wwise/Enums/EAkBankTypeEnum.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Wwise::Enums
{
    enum class EAkBankTypeEnum : uint32_t
    {
        User  = 0x0,
        Event = 0x1E,
        Bus   = 0x1F,
    };
}
