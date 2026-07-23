// Ported from CUE4Parse/UE4/Lua/unluac/EUnluacErrorCode.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Lua::unluac
{
    enum class EUnluacErrorCode
    {
        Ok,
        Error,
        PartialDecompile,
        CorruptedOpCodeMap,
    };
}
