// Ported from CUE4Parse/UE4/Objects/RigVM/ERigVMRegisterType.cs
#pragma once

#include <cstdint>

namespace CUE4Parse::UE4::Objects::RigVM
{
    enum class ERigVMRegisterType : uint8_t
    {
        Plain, // bool, int32, float, FVector etc.
        String, // FString
        Name, // FName
        Struct, // Any USTRUCT
        Invalid,
    };
}
