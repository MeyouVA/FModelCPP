// Ported from CUE4Parse/UE4/Objects/RigVM/ERigVMMemoryType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::RigVM
{
    enum class ERigVMMemoryType : uint8_t
    {
        Work, // Mutable state
        Literal, // Const / fixed state
        External, // Unowned external memory
        Debug,
        Invalid,
    };
}
